//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "CoLa2ProtocolHandler.h"

#include <cassert>
#include <cstddef> // for size_t

#include "VisionaryEndian.h"

namespace {
constexpr std::uint8_t kStx = 0x02u;
}

namespace visionary {

CoLa2ProtocolHandler::CoLa2ProtocolHandler(ITransport& rTransport)
  : m_rtransport(rTransport), m_reqID(0), m_sessionID(0)
{
}

CoLa2ProtocolHandler::~CoLa2ProtocolHandler() = default;

std::uint16_t CoLa2ProtocolHandler::createReqId()
{
  return ++m_reqID;
}

// parse a protocol response
CoLa2ProtocolHandler::ByteBuffer CoLa2ProtocolHandler::readProtocol()
{
  ByteBuffer buffer;
  buffer.reserve(64u); // typical maximum response size

  // get response
  // check for a run of 4 STX
  constexpr std::size_t numExpectedStx = 4u;
  std::size_t           stxRecvLeft    = numExpectedStx;

  while (stxRecvLeft > 0u)
  {
    ITransport::send_return_t nReceived = m_rtransport.recv(buffer, stxRecvLeft);

    if (nReceived <= 0)
    {
      // error or stream closed
      // return an empty buffer as indicator
      buffer.clear();
      return buffer;
    }

    // check if we have only STX in a row
    // if another byte was encountered, reset all counters so that we are looking for a new run of 4 STX
    ByteBuffer::iterator it{buffer.begin()};
    while (it != buffer.end())
    {
      if (kStx == *it++)
      {
        --stxRecvLeft;
      }
      else
      {
        buffer.erase(buffer.begin(), it);
        stxRecvLeft = numExpectedStx;
        it          = buffer.begin();
      }
    }
  }
  buffer.clear();

  // get length
  if (static_cast<ITransport::recv_return_t>(sizeof(std::uint32_t)) != m_rtransport.read(buffer, sizeof(std::uint32_t)))
  {
    // error or stream closed
    // return an empty buffer as indicator
    buffer.clear();
    return buffer;
  }
  const std::uint32_t length = readUnalignBigEndian<std::uint32_t>(buffer.data());

  buffer.clear();
  if (static_cast<ITransport::recv_return_t>(length) != m_rtransport.read(buffer, length))
  {
    // error or stream closed
    // return an empty buffer as indicator
    buffer.clear();
    return buffer;
  }

  if (length < 2u)
  {
    // invalid length
    // return an empty buffer as indicator
    buffer.clear();
    return buffer;
  }

  // skip HubCtr und NoC
  buffer.erase(buffer.begin(), buffer.begin() + 2u);

  return buffer;
}

// parse a command response
CoLa2ProtocolHandler::ByteBuffer CoLa2ProtocolHandler::readResponse(std::uint32_t& rSessionId, std::uint16_t& rReqId)
{
  ByteBuffer buffer{readProtocol()};

  // read 4 byte sessionId and 2 byte reqId
  if (buffer.size() < 4u + 2u)
  {
    // buffer too short / malformed
    buffer.clear();
    return buffer;
  }

  rSessionId = readUnalignBigEndian<std::uint32_t>(buffer.data());
  rReqId     = readUnalignBigEndian<std::uint16_t>(buffer.data() + 4u);

  buffer.erase(buffer.begin(), buffer.begin() + (4u + 2u));

  return buffer;
}

// build a protocol header
CoLa2ProtocolHandler::ByteBuffer CoLa2ProtocolHandler::createProtocolHeader(std::size_t payloadSize,
                                                                            std::size_t extraReserve)
{
  ByteBuffer header;
  header.reserve(4u + 4u + 1u + 1u + extraReserve);

  // insert magic bytes
  header.insert(header.end(), {kStx, kStx, kStx, kStx});

  // insert length
  {
    const std::uint32_t v = static_cast<std::uint32_t>(payloadSize) + 1u + 1u; // HubCtr + NoC
    std::uint8_t        b[4u];
    writeUnalignBigEndian<std::uint32_t>(b, sizeof(b), v);
    header.insert(header.end(), b, b + 4u);
  }

  // add HubCntr
  header.push_back(0u); // client starts with 0 here

  // add NoC
  header.push_back(0u); // client starts with 0 here

  return header;
}

// build a message header
CoLa2ProtocolHandler::ByteBuffer CoLa2ProtocolHandler::createCommandHeader(std::size_t payloadSize,
                                                                           std::size_t extraReserve)
{
  constexpr std::size_t cmdHeaderSize = 4u + 2u; // sessionID und ReqID;
  ByteBuffer            header{
    createProtocolHeader(payloadSize + cmdHeaderSize, cmdHeaderSize + extraReserve)}; // sessionID und ReqID

  // add SessionID
  {
    std::uint8_t b[4];

    writeUnalignBigEndian(b, sizeof(b), static_cast<std::uint32_t>(m_sessionID));
    header.insert(header.end(), b, b + 4u);
  }

  // add ReqID
  {
    std::uint8_t b[2];

    writeUnalignBigEndian(b, sizeof(b), static_cast<std::uint16_t>(createReqId()));
    header.insert(header.end(), b, b + 2u);
  }

  return header;
}

// send a command using the control session
CoLaCommand CoLa2ProtocolHandler::send(CoLaCommand cmd)
{
  const ByteBuffer& cmdBuffer{cmd.getBuffer()};
  // create buffer with command header in place
  constexpr std::size_t cmdOffset = 1u; // we skip the initial 's' from CoLaCommand buffer, not used in CoLa2
  ByteBuffer            buffer{createCommandHeader(cmdBuffer.size() - cmdOffset, cmdBuffer.size() - cmdOffset)};

  // add cmd
  buffer.insert(buffer.end(), cmdBuffer.begin() + cmdOffset, cmdBuffer.end());

  // send to socket
  if (m_rtransport.send(buffer) != static_cast<ITransport::send_return_t>(buffer.size()))
  {
    return CoLaCommand::networkErrorCommand();
  }

  buffer.clear();

  // get response
  std::uint32_t sessionId;
  std::uint16_t reqId;

  ByteBuffer response{readResponse(sessionId, reqId)};
  if (0u == response.size())
  {
    return CoLaCommand::networkErrorCommand();
  }

  if ((sessionId != m_sessionID) || (reqId != m_reqID))
  {
    // communication stream out of sync
    return CoLaCommand::networkErrorCommand();
  }

  // we re-insert the compatibility 's'  # TODO: get rid of it
  response.insert(response.begin(), 's');

  return CoLaCommand(response);
}

// open a new control session
bool CoLa2ProtocolHandler::openSession(std::uint8_t sessionTimeout /*secs*/)
{
  static const std::string clientid = "svs"; // Sick Visionary Shared - arbitrary client identifier
  const std::size_t        cmdSize  = 2u + 1u + 2u + clientid.size(); // session ID, ReqId, Cmd+Mode, Timeout, clientId

  // create buffer with command header in place
  ByteBuffer buffer{createCommandHeader(cmdSize, cmdSize)};

  // Add command
  buffer.push_back(static_cast<std::uint8_t>('O')); // Open Session
  buffer.push_back(static_cast<std::uint8_t>('x'));
  buffer.push_back(sessionTimeout); // sessionTimeout secs timeout

  {
    const std::uint16_t v = static_cast<std::uint16_t>(clientid.size());
    std::uint8_t        b[2u];
    writeUnalignBigEndian<std::uint16_t>(b, sizeof(b), v);
    buffer.insert(buffer.end(), b, b + 2u);

    for (auto c : clientid)
    {
      buffer.push_back(static_cast<std::uint8_t>(c));
    }
  }

  // send to socket
  if (m_rtransport.send(buffer) != static_cast<ITransport::send_return_t>(buffer.size()))
  {
    return false;
  }

  buffer.clear();

  // get response
  std::uint32_t sessionId = 0u;
  std::uint16_t reqId     = 0u;

  ByteBuffer response{readResponse(sessionId, reqId)};

  if (reqId != m_reqID)
  {
    // communication stream out of sync
    return false;
  }

  m_sessionID = sessionId;

  return true;
}

// close the control session
void CoLa2ProtocolHandler::closeSession()
{
  ByteBuffer cmd{'s', 'C', 'x'};

  CoLaCommand response{send(CoLaCommand(cmd))};
}

} // namespace visionary
