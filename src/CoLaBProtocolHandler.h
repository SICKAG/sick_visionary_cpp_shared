//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once
#include <cstddef> // for size_t
#include <cstdint>
#include <vector>

#include "CoLaCommand.h"
#include "IProtocolHandler.h"
#include "ITransport.h"

namespace visionary {

class CoLaBProtocolHandler : public IProtocolHandler
{
public:
  CoLaBProtocolHandler(ITransport& rTransport);
  ~CoLaBProtocolHandler() override;

  bool openSession(std::uint8_t sessionTimeout /*secs*/) override;
  void closeSession() override;

  /// send cola cmd and receive cola response
  CoLaCommand send(CoLaCommand cmd) override;

private:
  using ByteBuffer = std::vector<std::uint8_t>;

  std::uint8_t calculateChecksum(ByteBuffer::const_iterator begin, ByteBuffer::const_iterator end) const;
  /// parse a response on protocol level
  ByteBuffer readProtocol();
  /// read a command response packet
  ByteBuffer readResponse();

  ByteBuffer createProtocolHeader(std::size_t payloadSize, std::size_t extraReserve = 0u);
  ByteBuffer createCommandHeader(std::size_t payloadSize, std::size_t extraReserve = 0u);

  ITransport& m_rtransport;
};

} // namespace visionary
