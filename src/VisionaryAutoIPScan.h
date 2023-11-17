//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace visionary {

class VisionaryAutoIPScan
{
public:
  static constexpr std::uint16_t DEFAULT_PORT{30718u};
  static const std::string       DEFAULT_BROADCAST_ADDR;

  struct DeviceInfo
  {
    std::string   deviceName;
    std::string   macAddress;
    std::string   ipAddress;
    std::string   subNet;
    std::uint16_t port;
  };

  /// <summary>
  /// Runs an autoIP scan and returns a list of devices
  /// </summary>
  /// <returns>A list of devices.</returns>
  std::vector<DeviceInfo> doScan(unsigned           timeOut,
                                 const std::string& broadcastAddress = DEFAULT_BROADCAST_ADDR,
                                 std::uint16_t      port             = DEFAULT_PORT);

private:
  using ByteBuffer = std::vector<std::uint8_t>;

  DeviceInfo parseAutoIPXml(std::stringstream& rStringStream);
};

} // namespace visionary
