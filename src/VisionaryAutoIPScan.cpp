//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "VisionaryAutoIPScan.h"

#include <bitset>
#include <chrono>
#include <cstddef> // for size_t
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>

#include "VisionaryEndian.h"

// Boost library used for parseXML function
#if defined(__GNUC__)         // GCC compiler
#  pragma GCC diagnostic push // Save warning levels for later restoration
#  pragma GCC diagnostic ignored "-Wpragmas"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wparentheses"
#  pragma GCC diagnostic ignored "-Wcast-align"
#  pragma GCC diagnostic ignored "-Wstrict-overflow"
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#if defined(__GNUC__)        // GCC compiler
#  pragma GCC diagnostic pop // Restore previous warning levels
#endif

#include "UdpSocket.h"

namespace visionary {

namespace pt = boost::property_tree;

const std::string  VisionaryAutoIPScan::DEFAULT_BROADCAST_ADDR{"255.255.255.255"};
const std::string  VisionaryAutoIPScan::DEFAULT_IP_MASK{"255.255.255.0"};
const std::string  VisionaryAutoIPScan::DEFAULT_GATEWAY{"0.0.0.0"};
const std::uint8_t kRplIpconfig  = 0x91; // replied by sensor; confirmation to IP change
const std::uint8_t kRplNetscan   = 0x95; // replied by sensors; with information like device name, serial number, IP,...
const std::uint8_t kRplScanColaB = 0x90; // replied by sensors; with information like device name, serial number, IP,...

std::vector<VisionaryAutoIPScan::DeviceInfo> VisionaryAutoIPScan::doScan(unsigned int timeOut, std::uint16_t port)
{
  // Init Random generator
  std::random_device                           rd;
  std::default_random_engine                   mt(rd());
  std::uint32_t                                teleIdCounter = mt();
  std::vector<VisionaryAutoIPScan::DeviceInfo> deviceList;

  std::unique_ptr<UdpSocket> pTransport(new UdpSocket());

  // serverIP
  std::vector<std::uint8_t> ipAddrVector = convertIpToBinary(m_serverIP);
  // server net mask
  std::vector<std::uint8_t> maskVector = convertIpToBinary(m_serverNetMask);

  // generate broadcast address according to the IP and the mask
  std::string broadcastAddress{};
  // As the definition of IPv4, ipAddrVector and maskVector have 4 elements correspondingly.
  constexpr std::size_t kIpAddrVectorSize{4u};
  for (std::size_t i = 0; i < kIpAddrVectorSize; ++i)
  {
    broadcastAddress +=
      std::to_string((~maskVector[i] & 0xff) | ipAddrVector[i]) + (i + 1u == kIpAddrVectorSize ? "" : ".");
  }
  if (pTransport->connect(broadcastAddress, port) != 0)
  {
    return deviceList;
  }

  // AutoIP Discover Packet
  ByteBuffer autoIpPacket;
  autoIpPacket.push_back(0x10); // CMD
  autoIpPacket.push_back(0x00); // reserved
  // length of datablock
  autoIpPacket.push_back(0x00);
  autoIpPacket.push_back(0x08);
  // Mac address
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  autoIpPacket.push_back(0xFF);
  // telegram id
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);

  // Additional UCP Scan information
  autoIpPacket.push_back(0x01); // Indicates that telegram is a CoLa Scan telegram
  autoIpPacket.push_back(0x0);

  // server ip
  autoIpPacket.insert(autoIpPacket.end(), ipAddrVector.begin(), ipAddrVector.end());

  // server net mask
  autoIpPacket.insert(autoIpPacket.end(), maskVector.begin(), maskVector.end());

  // Replace telegram id in packet
  std::uint32_t curtelegramID = teleIdCounter++;
  writeUnalignBigEndian<std::uint32_t>(autoIpPacket.data() + 10u, sizeof(curtelegramID), curtelegramID);

  // Send Packet
  pTransport->send(autoIpPacket);

  // Check for answers to Discover Packet
  const std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  while (true)
  {
    ByteBuffer                                  receiveBuffer;
    const std::chrono::steady_clock::time_point now(std::chrono::steady_clock::now());
    if ((now - startTime) > std::chrono::milliseconds(timeOut))
    {
      std::cout << __FUNCTION__ << " Timeout" << '\n';
      break;
    }
    if (pTransport->recv(receiveBuffer, 1400) > 16) // 16 bytes minsize
    {
      unsigned int pos = 0;
      if (receiveBuffer[0] == kRplNetscan)
      {
        DeviceInfo dI = parseAutoIPBinary(receiveBuffer);
        deviceList.push_back(dI);
        continue;
      }
      if (receiveBuffer[0] == kRplScanColaB)
      {
        pos++;
        pos += 1; // unused byte
        std::uint16_t payLoadSize = readUnalignBigEndian<std::uint16_t>(receiveBuffer.data() + pos);
        pos += 2;
        pos += 6; // Skip mac address(part of xml)
        std::uint32_t recvTelegramID = readUnalignBigEndian<std::uint32_t>(receiveBuffer.data() + pos);
        pos += 4;
        // check if it is a response to our scan
        if (recvTelegramID != curtelegramID)
        {
          continue;
        }
        pos += 2; // unused
        // Get XML Payload
        if (receiveBuffer.size() >= pos + payLoadSize)
        {
          std::stringstream stringStream(std::string(reinterpret_cast<char*>(&receiveBuffer[pos]), payLoadSize));
          try
          {
            DeviceInfo dI = parseAutoIPXml(stringStream);
            deviceList.push_back(dI);
          }
          catch (...)
          {
          }
        }
        else
        {
          std::cout << __FUNCTION__ << "Received invalid AutoIP Packet" << '\n';
        }
      }
    }
  }
  return deviceList;
}

VisionaryAutoIPScan::DeviceInfo VisionaryAutoIPScan::parseAutoIPXml(std::stringstream& rStringStream)
{
  pt::ptree tree;
  pt::read_xml(rStringStream, tree);
  const std::string& macAddress = tree.get_child("NetScanResult.<xmlattr>.MACAddr").get_value<std::string>();
  std::string        ipAddress;
  std::string        subNet;
  std::string        port;
  std::string        deviceType;

  for (const auto& it : tree.get_child("NetScanResult"))
  {
    if (it.first != "<xmlattr>")
    {
      const std::string& key   = it.second.get<std::string>("<xmlattr>.key");
      const std::string& value = it.second.get<std::string>("<xmlattr>.value");
      if (key == "IPAddress")
      {
        ipAddress = value;
      }

      if (key == "IPMask")
      {
        subNet = value;
      }

      if (key == "HostPortNo")
      {
        port = value;
      }

      if (key == "DeviceType")
      {
        deviceType = value;
      }
    }
  }
  DeviceInfo dI;
  dI.deviceName        = deviceType;
  dI.ipAddress         = ipAddress;
  dI.macAddress        = convertMacToStruct(macAddress);
  std::size_t   idx    = 0u;
  unsigned long portul = std::stoul(port, &idx);
  if ((idx < port.length()) || (portul > std::numeric_limits<std::uint16_t>::max()) || (portul == 0u))
  {
    // invalid port number or range
    std::cerr << "invalid port number '" << port << "' (must be an unsigned number in range 1..65535) for "
              << deviceType << " device at ip " << ipAddress << std::endl;
    dI.port = 0u;
  }
  else
  {
    dI.port = static_cast<std::uint16_t>(portul);
  }
  dI.subNet = subNet;

  return dI;
}

VisionaryAutoIPScan::DeviceInfo VisionaryAutoIPScan::parseAutoIPBinary(const VisionaryAutoIPScan::ByteBuffer& buffer)
{
  DeviceInfo deviceInfo;

  deviceInfo.subNet       = "";
  deviceInfo.port         = 0;
  deviceInfo.protocolType = COLA_2;

  int offset = 16;
  //  auto deviceInfoVersion = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;

  auto cidNameLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string cidName;
  for (int i = 0; i < cidNameLen; ++i)
  {
    cidName += readUnalignBigEndian<char>(buffer.data() + offset);
    offset++;
  }
  deviceInfo.deviceName = cidName;

  //  auto cidMajorVersion = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  //  auto cidMinorVersion = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  //  auto cidPatchVersion = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  //  auto cidBuildVersion = readUnalignBigEndian<std::uint32_t>(buffer.data() + offset);
  offset += 4;
  //  auto cidVersionClassifier = readUnalignBigEndian<std::uint8_t>(buffer.data() + offset);
  offset += 1;

  //  auto deviceState = readUnalignBigEndian<char>(buffer.data() + offset);
  offset += 1;

  //  auto reqUserAction = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;

  // Device name
  auto deviceNameLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string deviceName(reinterpret_cast<const char*>(buffer.data() + offset), deviceNameLen);
  offset += deviceNameLen;

  // App name
  auto appNameLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string appName(reinterpret_cast<const char*>(buffer.data() + offset), appNameLen);
  offset += appNameLen;

  // Project name
  auto projNameLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string projName(reinterpret_cast<const char*>(buffer.data() + offset), projNameLen);
  offset += projNameLen;

  // Serial number
  auto serialNumLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string serialNum(reinterpret_cast<const char*>(buffer.data() + offset), serialNumLen);
  offset += serialNumLen;

  // Type code
  auto typeCodeLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string typeCode(reinterpret_cast<const char*>(buffer.data() + offset), typeCodeLen);
  offset += typeCodeLen;

  // Firmware version
  auto firmwareVersionLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string firmwareVersion(reinterpret_cast<const char*>(buffer.data() + offset), firmwareVersionLen);
  offset += firmwareVersionLen;

  // Order number
  auto orderNumberLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  std::string orderNumber(reinterpret_cast<const char*>(buffer.data() + offset), orderNumberLen);
  offset += orderNumberLen;

  // # unused: flags, = struct.unpack('>B', rpl[offset:offset + 1])
  offset += 1;

  auto auxArrayLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;

  for (int i = 0; i < auxArrayLen; ++i)
  {
    std::string key;
    for (int k = 0; k < 4; ++k)
    {
      key += readUnalignBigEndian<char>(buffer.data() + offset);
      offset++;
    }
    auto innerArrayLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
    offset += 2;
    for (int j = 0; j < innerArrayLen; ++j)
    {
      //      auto v = readUnalignBigEndian<std::uint8_t>(buffer.data() + offset);
      offset += 1;
    }
  }
  auto scanIfLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;
  for (int i = 0; i < scanIfLen; ++i)
  {
    //    auto ifaceNum = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
    offset += 2;
    auto ifaceNameLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
    offset += 2;
    std::string ifaceName(reinterpret_cast<const char*>(buffer.data() + offset), ifaceNameLen);
    offset += ifaceNameLen;
  }
  auto comSettingsLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;

  for (int i = 0; i < comSettingsLen; ++i)
  {
    std::string key;
    for (int k = 0; k < 4; ++k)
    {
      key += readUnalignBigEndian<char>(buffer.data() + offset);
      offset++;
    }
    auto innerArrayLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
    offset += 2;
    if (key == "EMAC")
    {
      std::memcpy(deviceInfo.macAddress.macAddress, buffer.data() + offset, sizeof(deviceInfo.macAddress.macAddress));
      offset += 6;
      continue;
    }
    if (key == "EIPa")
    {
      std::string ipAddr;
      for (int k = 0; k < 4; ++k)
      {
        ipAddr += std::to_string(static_cast<unsigned>(readUnalignBigEndian<uint8_t>(buffer.data() + offset)));
        if (k < 3)
        {
          ipAddr += ".";
        }
        offset++;
      }
      deviceInfo.ipAddress = ipAddr;
      continue;
    }
    if (key == "ENMa")
    {
      std::string subNet;
      for (int k = 0; k < 4; ++k)
      {
        subNet += std::to_string(static_cast<unsigned>(readUnalignBigEndian<uint8_t>(buffer.data() + offset)));
        if (k < 3)
        {
          subNet += ".";
        }
        offset++;
      }
      deviceInfo.subNet = subNet;
      continue;
    }
    if (key == "EDGa")
    {
      std::string stdGw;
      for (int k = 0; k < 4; ++k)
      {
        stdGw += std::to_string(static_cast<unsigned>(readUnalignBigEndian<char>(buffer.data() + offset)));
        if (k < 3)
        {
          stdGw += ".";
        }
        offset++;
      }
      continue;
    }
    if (key == "EDhc")
    {
      //      dhcp = readUnalignBigEndian<std::uint8_t>(buffer.data() + offset);
      offset += 1;
      continue;
    }
    if (key == "ECDu")
    {
      //      auto configTime = readUnalignBigEndian<std::uint32_t>(buffer.data() + offset);
      offset += 4;
      continue;
    }
    for (int j = 0; j < innerArrayLen; ++j)
    {
      //      auto v = readUnalignBigEndian<std::uint8_t>(buffer.data() + offset);
      offset += 1;
    }
  }
  auto endPointsLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
  offset += 2;

  std::vector<std::uint16_t> ports;
  for (int i = 0; i < endPointsLen; ++i)
  {
    //    auto colaVersion = readUnalignBigEndian<std::uint8_t>(buffer.data() + offset);
    offset += 1;
    auto innerArrayLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
    offset += 2;
    for (int j = 0; j < innerArrayLen; ++j)
    {
      std::string key(reinterpret_cast<const char*>(buffer.data() + offset), 4u);
      offset += 4;

      auto mostInnerArrayLen = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
      offset += 2;

      if (key == "DPNo") // PortNumber [UInt]
      {
        auto port = readUnalignBigEndian<std::uint16_t>(buffer.data() + offset);
        offset += 2;
        ports.push_back(port);
      }
      else
      {
        for (int k = 0; k < mostInnerArrayLen; ++k)
        {
          //          auto v = readUnalignBigEndian<std::uint8_t>(buffer.data() + offset);
          offset += 1;
        }
      }
    }
  }
  if (!ports.empty())
  {
    deviceInfo.port = ports[0];
  }
  return deviceInfo;
}

VisionaryAutoIPScan::VisionaryAutoIPScan(const std::string& serverIP, std::uint8_t prefixLength) : m_serverIP(serverIP)
{
  m_serverNetMask = networkPrefixToMask(prefixLength);
}

std::string VisionaryAutoIPScan::networkPrefixToMask(std::uint8_t prefixLength)
{
  // For unsigned a, the value of a << b is the value of a * 2b , reduced modulo 2N where N is the number of bits in the
  // return type (that is, bitwise left shift is performed and the bits that get shifted out of the destination type are
  // discarded)
  std::bitset<32> hostPrefix(0xffffffffull << (32u - prefixLength));
  std::bitset<32> mask(0xff000000);

  std::string hostMask{};

  constexpr std::size_t kIpAddressLength{32u};
  for (std::size_t i = 0; i < kIpAddressLength; i += 8u)
  {
    hostMask +=
      std::to_string((((hostPrefix << i & mask) >> 24u).to_ulong())) + (i + 8u == kIpAddressLength ? "" : ".");
  }

  return hostMask;
}

std::vector<std::uint8_t> VisionaryAutoIPScan::convertIpToBinary(const std::string& address)
{
  std::vector<std::uint8_t> ipInts;
  std::istringstream        ss(address);
  while (ss)
  {
    std::string token;
    if (!getline(ss, token, '.'))
      break;
    ipInts.push_back(static_cast<uint8_t>(std::stoi(token)));
  }
  return ipInts;
}

bool VisionaryAutoIPScan::assign(const MacAddress&                 destinationMac,
                                 VisionaryAutoIPScan::ProtocolType colaVer,
                                 const std::string&                ipAddr,
                                 std::uint8_t                      prefixLength,
                                 const std::string&                ipGateway,
                                 bool                              dhcp,
                                 unsigned int                      timeout)
{
  if (colaVer != ProtocolType::COLA_B && colaVer != ProtocolType::COLA_2)
  {
    return false;
  }

  std::string ipMask{networkPrefixToMask(prefixLength)};

  ByteBuffer payload;
  if (colaVer == ProtocolType::COLA_B)
  {
    std::string dhcpString = "FALSE";
    if (dhcp)
    {
      dhcpString = "TRUE";
    }
    std::string request = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<IPconfig MACAddr=\"" + convertMacToString(destinationMac) + "\">"
      "<Item key=\"IPAddress\" value=\"" + ipAddr + "\" />"
      "<Item key=\"IPMask\" value=\"" + ipMask + "\" />"
      "<Item key=\"IPGateway\" value=\"" + ipGateway + "\" />"
      "<Item key=\"DHCPClientEnabled\" value=\"" + dhcpString +"\" /></IPconfig>";

    payload.insert(payload.end(), request.c_str(), request.c_str() + request.size());
  }
  else if (colaVer == COLA_2)
  {
    std::vector<uint8_t> ipAddrVector = convertIpToBinary(ipAddr);
    payload.insert(payload.end(), ipAddrVector.begin(), ipAddrVector.end());

    std::vector<uint8_t> ipMaskVector = convertIpToBinary(ipMask);
    payload.insert(payload.end(), ipMaskVector.begin(), ipMaskVector.end());

    std::vector<uint8_t> ipGatewayVector = convertIpToBinary(ipGateway);
    payload.insert(payload.end(), ipGatewayVector.begin(), ipGatewayVector.end());

    payload.push_back(dhcp);
  }

  // Init Random generator
  std::random_device         rd;
  std::default_random_engine mt(rd());
  unsigned int               teleIdCounter = mt();

  std::unique_ptr<UdpSocket> pTransport(new UdpSocket());

  std::vector<std::uint8_t> addrVector = convertIpToBinary(m_serverIP);
  std::vector<std::uint8_t> maskVector = convertIpToBinary(m_serverNetMask);
  // generate broadcast address according to the IP and the mask
  std::string broadcastAddress{};
  // As the definition of IPv4, ipAddrVector and maskVector have 4 elements correspondingly.
  constexpr std::size_t kIpAddrVectorSize{4u};
  for (std::size_t i = 0; i < kIpAddrVectorSize; ++i)
  {
    broadcastAddress +=
      std::to_string((~maskVector[i] & 0xff) | addrVector[i]) + (i + 1u == kIpAddrVectorSize ? "" : ".");
  }

  if (pTransport->connect(broadcastAddress, DEFAULT_PORT) != 0)
  {
    return false;
  }

  // AutoIP Discover Packet
  ByteBuffer autoIpPacket;
  autoIpPacket.push_back(0x11); // CMD ip config
  autoIpPacket.push_back(0x0);  // not defined rfu

  // length of datablock
  std::uint8_t blockLenght[2];
  writeUnalignBigEndian(blockLenght, sizeof(blockLenght), static_cast<std::uint16_t>(payload.size()));
  autoIpPacket.insert(autoIpPacket.end(), blockLenght, blockLenght + 2u);

  // Mac address
  autoIpPacket.insert(autoIpPacket.end(), destinationMac.macAddress, destinationMac.macAddress + 6u);

  // telegram id
  unsigned int curtelegramID = teleIdCounter++;
  std::uint8_t b[4];
  writeUnalignBigEndian(b, sizeof(b), static_cast<std::uint32_t>(curtelegramID));
  autoIpPacket.insert(autoIpPacket.end(), b, b + 4u);

  // reserved
  autoIpPacket.push_back(0x01); // Indicates that telegram is a CoLa Scan telegram
  autoIpPacket.push_back(0x00);

  // payload
  autoIpPacket.insert(autoIpPacket.end(), payload.begin(), payload.end());

  pTransport->send(autoIpPacket);

  const std::chrono::steady_clock::time_point startTime(std::chrono::steady_clock::now());
  while (true)
  {
    ByteBuffer                                  receiveBuffer;
    const std::chrono::steady_clock::time_point now(std::chrono::steady_clock::now());
    if ((now - startTime) > std::chrono::milliseconds(timeout))
    {
      std::cout << __FUNCTION__ << " Timeout" << '\n';
      break;
    }
    if (pTransport->recv(receiveBuffer, 1400) > 16) // 16 bytes minsize
    {
      if (receiveBuffer[0] == kRplIpconfig)
      {
        return true;
      }
    }
  }

  return false;
}

VisionaryAutoIPScan::MacAddress VisionaryAutoIPScan::convertMacToStruct(const std::string& basicString)
{
  std::vector<std::uint8_t> ipInts;
  std::istringstream        ss(basicString);
  MacAddress                macAddress{};
  int                       i = 0;
  while (ss)
  {
    std::string token;
    size_t      pos;
    if (!getline(ss, token, ':'))
      break;
    macAddress.macAddress[i] = static_cast<std::uint8_t>(std::stoi(token, &pos, 16));
    i++;
  }
  return macAddress;
}

std::string VisionaryAutoIPScan::convertMacToString(const VisionaryAutoIPScan::MacAddress& macAddress)
{
  std::string macAddrString;
  for (int k = 0; k < 6; ++k)
  {
    std::stringstream ss;
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(macAddress.macAddress[k]);

    macAddrString += ss.str();
    if (k < 5)
    {
      macAddrString += ":";
    }
  }
  return macAddrString;
}

} // namespace visionary
