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

class CoLa2ProtocolHandler : public IProtocolHandler
{
public:
  CoLa2ProtocolHandler(ITransport& rTransport);
  ~CoLa2ProtocolHandler() override;

  bool openSession(std::uint8_t sessionTimeout /*secs*/) override;
  void closeSession() override;

  // send cola cmd and receive cola response
  CoLaCommand send(CoLaCommand cmd) override;

  std::uint16_t getReqId() const
  {
    return m_reqID;
  }
  std::uint32_t getSessionId() const
  {
    return m_sessionID;
  }

private:
  using ByteBuffer = std::vector<std::uint8_t>;

  std::uint16_t createReqId();

  /// parse a response on protocol level
  ByteBuffer readProtocol();
  /// read a command response packet
  ByteBuffer readResponse(std::uint32_t& rSessionId, uint16_t& rReqId);

  ByteBuffer createProtocolHeader(std::size_t payloadSize, std::size_t extraReserve = 0u);
  ByteBuffer createCommandHeader(std::size_t payloadSize, std::size_t extraReserve = 0u);

  ITransport&   m_rtransport;
  std::uint16_t m_reqID;
  std::uint32_t m_sessionID;
};

} // namespace visionary
