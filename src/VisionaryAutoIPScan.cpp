//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "VisionaryAutoIPScan.h"

#include <limits>
#include <memory>
#include <sstream>

#include <chrono>
#include <cstddef> // for size_t
#include <cstdint>
#include <iostream>
#include <random>
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

const std::string VisionaryAutoIPScan::DEFAULT_BROADCAST_ADDR{"255.255.255.255"};

std::vector<VisionaryAutoIPScan::DeviceInfo> VisionaryAutoIPScan::doScan(unsigned           timeOut,
                                                                         const std::string& broadcastAddress,
                                                                         std::uint16_t      port)
{
  // Init Random generator
  std::random_device                           rd;
  std::default_random_engine                   mt(rd());
  unsigned int                                 teleIdCounter = mt();
  std::vector<VisionaryAutoIPScan::DeviceInfo> deviceList;

  std::unique_ptr<UdpSocket> pTransport(new UdpSocket());

  if (pTransport->connect(broadcastAddress, port) != 0)
  {
    return deviceList;
  }

  // AutoIP Discover Packet
  ByteBuffer autoIpPacket;
  autoIpPacket.push_back(0x10); // CMD
  autoIpPacket.push_back(0x0);  // reserved
  // length of datablock
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);
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
  // reserved
  autoIpPacket.push_back(0x0);
  autoIpPacket.push_back(0x0);

  // Replace telegram id in packet
  unsigned int curtelegramID = teleIdCounter++;
  writeUnalignBigEndian<std::uint32_t>(autoIpPacket.data() + 10u, autoIpPacket.size() - 10u, curtelegramID);

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
      break;
    }
    if (pTransport->recv(receiveBuffer, 1400) > 16) // 16 bytes minsize
    {
      unsigned int pos = 0;
      if (receiveBuffer[pos++] != 0x90) // 0x90 = answer package id and 16 bytes minsize
      {
        continue;
      }
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
        std::cout << __FUNCTION__ << "Received invalid AutoIP Packet" << std::endl;
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
  dI.macAddress        = macAddress;
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

} // namespace visionary
