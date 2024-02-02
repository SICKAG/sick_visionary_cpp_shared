//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace visionary {

/**
 * This class is intended to find devices in the network and set a new IP address
 */
class VisionaryAutoIPScan
{
public:
  static constexpr std::uint16_t DEFAULT_PORT{30718u};
  static const std::string       DEFAULT_BROADCAST_ADDR;
  static const std::string       DEFAULT_IP_MASK;
  static const std::string       DEFAULT_GATEWAY;
  static constexpr bool          DEFAULT_DHCP{false};
  static constexpr std::uint16_t DEFAULT_TIMEOUT{5000};

  enum ProtocolType
  {
    INVALID_PROTOCOL = -1,
    COLA_A           = 2111,
    COLA_B           = 2112,
    COLA_2           = 2122
  };

  struct MacAddress
  {
    std::uint8_t macAddress[6];
  };

  struct DeviceInfo
  {
    std::string   deviceName;
    MacAddress    macAddress;
    std::string   ipAddress;
    std::string   subNet;
    std::uint16_t port;
    ProtocolType  protocolType;
  };

  /**
   * Constructor for the AutoIP scan
   * @param serverIP ip address of the server which is executing the search
   * @param prefixLength the network prefix length of the server in the CIDR manner
   */
  VisionaryAutoIPScan(const std::string& serverIP, std::uint8_t prefixLength);
  /**
   * Runs an autoIP scan and returns a list of devices
   * @param timeOut timout of the search in ms
   * @param broadcastAddress broadcast address for the scan
   * @param port the port on which to perform the scan
   * @return a list of devices
   */
  std::vector<DeviceInfo> doScan(unsigned int timeOut, std::uint16_t port = DEFAULT_PORT);

  /**
   * Assigns a new ip configuration to a device based on the MAC address
   * @param destinationMac MAC address of the device on to which to assign the new configuration
   * @param colaVer cola version of the device
   * @param ipAddr new ip address of the device
   * @param prefixLength the network prefix length of the server in the CIDR manner
   * @param ipGateway new gateway of the device
   * @param dhcp true if dhcp should be enabled
   * @param timout timeout in ms
   * @return
   */
  bool assign(const MacAddress&  destinationMac,
              ProtocolType       colaVer,
              const std::string& ipAddr,
              std::uint8_t       prefixLength,
              const std::string& ipGateway = DEFAULT_GATEWAY,
              bool               dhcp      = DEFAULT_DHCP,
              unsigned int       timout    = DEFAULT_TIMEOUT);

  static MacAddress  convertMacToStruct(const std::string& basicString);
  static std::string convertMacToString(const MacAddress& macAddress);

private:
  using ByteBuffer = std::vector<std::uint8_t>;
  std::string m_serverIP;
  std::string m_serverNetMask;

  DeviceInfo                       parseAutoIPXml(std::stringstream& rStringStream);
  DeviceInfo                       parseAutoIPBinary(const ByteBuffer& receivedBuffer);
  static std::vector<std::uint8_t> convertIpToBinary(const std::string& address);
  static std::string               networkPrefixToMask(std::uint8_t prefixLength);
};

} // namespace visionary
