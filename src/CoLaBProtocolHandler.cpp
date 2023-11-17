//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "CoLaBProtocolHandler.h"

#include <cassert>
#include <cstddef> // for size_t

#include "VisionaryEndian.h"

namespace {
constexpr std::uint8_t kStx = 0x02u;
}

namespace visionary {

CoLaBProtocolHandler::CoLaBProtocolHandler(ITransport& rTransport) : m_rtransport(rTransport)
{
}

CoLaBProtocolHandler::~CoLaBProtocolHandler() = default;

std::uint8_t CoLaBProtocolHandler::calculateChecksum(ByteBuffer::const_iterator begin,
                                                     ByteBuffer::const_iterator end) const
{
  std::uint8_t checksum = 0;
  for (auto it = begin; it != end; ++it)
  {
    checksum ^= *it;
  }
  return checksum;
}

// parse a protocol response
CoLaBProtocolHandler::ByteBuffer CoLaBProtocolHandler::readProtocol()
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
      if (kStx == *it)
      {
        --stxRecvLeft;
        ++it;
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
  // read payload + 1 byte checksum
  if (static_cast<ITransport::recv_return_t>(length + 1u) != m_rtransport.read(buffer, length + 1u))
  {
    // error or stream closed
    // return an empty buffer as indicator
    buffer.clear();
    return buffer;
  }

  // drop checksum
  buffer.pop_back();

  return buffer;
}

// parse a command response
CoLaBProtocolHandler::ByteBuffer CoLaBProtocolHandler::readResponse()
{
  return readProtocol();
}

// build a protocol header
CoLaBProtocolHandler::ByteBuffer CoLaBProtocolHandler::createProtocolHeader(std::size_t payloadSize,
                                                                            std::size_t extraReserve)
{
  ByteBuffer header;
  header.reserve(4u + 4u + extraReserve);

  // insert magic bytes
  header.insert(header.end(), {kStx, kStx, kStx, kStx});

  // insert length
  {
    const std::uint32_t v = static_cast<std::uint32_t>(payloadSize);
    std::uint8_t        b[4u];
    writeUnalignBigEndian<std::uint32_t>(b, sizeof(b), v);
    header.insert(header.end(), b, b + 4u);
  }

  return header;
}

// build a message header
CoLaBProtocolHandler::ByteBuffer CoLaBProtocolHandler::createCommandHeader(std::size_t payloadSize,
                                                                           std::size_t extraReserve)
{
  return createProtocolHeader(payloadSize, extraReserve);
}

// send a command using the control session
CoLaCommand CoLaBProtocolHandler::send(CoLaCommand cmd)
{
  const ByteBuffer& cmdBuffer{cmd.getBuffer()};

  // create buffer with command header in place (and reserve space for the cmd and the checksum byte)
  ByteBuffer buffer{createCommandHeader(cmdBuffer.size(), cmdBuffer.size() + 1u)};

  // add cmd
  buffer.insert(buffer.end(), cmdBuffer.begin(), cmdBuffer.end());

  // Add checksum to end
  constexpr std::size_t checksum_offset = 4u + 4u; // after STX + length
  buffer.insert(buffer.end(), calculateChecksum(buffer.cbegin() + checksum_offset, buffer.cend()));

  // send to socket
  if (m_rtransport.send(buffer) != static_cast<ITransport::send_return_t>(buffer.size()))
  {
    return CoLaCommand::networkErrorCommand();
  }
  buffer.clear();

  // get response
  ByteBuffer response{readResponse()};
  if (0u == response.size())
  {
    return CoLaCommand::networkErrorCommand();
  }

  return CoLaCommand(response);
}

// open a new control session
bool CoLaBProtocolHandler::openSession(std::uint8_t /*sessionTimeout secs*/)
{
  // we don't have a session id byte in CoLaB protocol. Nothing to do here.
  return true;
}

// close the control session
void CoLaBProtocolHandler::closeSession()
{
  // we don't have a session id byte in CoLaB protocol. Nothing to do here.
}

} // namespace visionary
