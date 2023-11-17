//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "MockTransport.h"

#include <algorithm> // for min, max
#include <cassert>
#include <iostream>

#include <VisionaryEndian.h>

namespace visionary_test {

template <class T, class TSrc>
static void appendTo(T& dest, const TSrc& src)
{
  dest.insert(dest.end(), src.cbegin(), src.cend());
}

MockTransport::MockTransport() : m_state(kSEND_IDLE), m_onRecv(nop), m_onSend(nop)
{
}

MockTransport::MockTransport(const ByteBuffer& buffer)
  : m_state(kSEND_IDLE), m_onRecv(nop), m_onSend(nop), m_mockRecvBuffer(buffer)
{
}

MockTransport::MockTransport(std::initializer_list<ByteBuffer::value_type> init)
  : m_state(kSEND_IDLE), m_onRecv(nop), m_onSend(nop), m_mockRecvBuffer(init)
{
}

MockTransport::~MockTransport() = default;

void MockTransport::nop()
{
}

MockTransport& MockTransport::recvBuffer(const ByteBuffer& buffer)
{
  m_mockRecvBuffer = buffer;
  return *this;
}

MockTransport& MockTransport::recvBuffer(std::initializer_list<ByteBuffer::value_type> init)
{
  m_mockRecvBuffer.assign(init);
  return *this;
}

MockTransport& MockTransport::noFakeSendReturn()
{
  m_fakeSendReturn.reset();
  return *this;
}

MockTransport& MockTransport::fakeSendReturn(send_return_t retval)
{
  m_fakeSendReturn = retval;
  return *this;
}

MockTransport& MockTransport::onRecv(std::function<void()> fct)
{
  m_onRecv = fct;
  return *this;
}

void MockTransport::recvHandler()
{
  m_onRecv();
}

void MockTransport::sendHandler()
{
  m_onSend();
}

visionary::ITransport::send_return_t MockTransport::send(const char* pBuffer, size_t size)
{
  switch (m_state)
  {
    case kSEND_STARTED:
    case kSEND_CONTD:
      m_state = kSEND_CONTD;
      break;
    default:
      m_state = kSEND_STARTED;
      break;
  }

  m_mockSendBuffer.insert(m_mockSendBuffer.begin(),
                          reinterpret_cast<const std::uint8_t*>(pBuffer),
                          reinterpret_cast<const std::uint8_t*>(pBuffer) + size);

  sendHandler();

  return m_fakeSendReturn.value_or(static_cast<visionary::ITransport::send_return_t>(size));
}

visionary::ITransport::recv_return_t MockTransport::recv(ByteBuffer& buffer, std::size_t maxBytesToReceive)
{
  switch (m_state)
  {
    case kRECV_STARTED:
    case kRECV_CONTD:
      m_state = kRECV_CONTD;
      break;
    default:
      m_state = kRECV_STARTED;
      break;
  }

  recvHandler();

  using IterDiffType = ByteBuffer::iterator::difference_type;

  const auto returnSize = std::min(maxBytesToReceive, m_mockRecvBuffer.size());

  ByteBuffer::iterator recvEndPos{std::next(m_mockRecvBuffer.begin(), static_cast<IterDiffType>(returnSize))};

  buffer.assign(m_mockRecvBuffer.begin(), recvEndPos);
  m_mockRecvBuffer.erase(m_mockRecvBuffer.begin(), recvEndPos);

  return static_cast<visionary::ITransport::recv_return_t>(returnSize);
}

visionary::ITransport::recv_return_t MockTransport::read(ByteBuffer& buffer, std::size_t nBytesToReceive)
{
  return recv(buffer, nBytesToReceive);
}

int MockTransport::shutdown()
{
  return 0;
}
int MockTransport::getLastError()
{
  return 0;
}

ByteBuffer MockTransport::buildProtocol(const ProtocolReqRespHeader& header, const ByteBuffer& payload)
{
  using Endian = visionary::Endian<visionary::endian::native, visionary::endian::big>;

  ByteBuffer buffer({0x02u, 0x02u, 0x02u, 0x02u}); // prefix

  buffer.reserve(payload.size() + 4u + 4u + 2u); // 4*STX, length, hubCntr, noC

  const std::uint32_t length = static_cast<std::uint32_t>(payload.size()) + 2u; // hubCntr + noC
  appendTo(buffer, Endian::convertToVector<std::uint32_t>(length));

  buffer.push_back(header.hubCntr);
  buffer.push_back(header.noC);

  appendTo(buffer, payload);

  return buffer;
}

bool MockTransport::parseProtocol(ProtocolReqRespHeader&            header,
                                  ByteBuffer::const_iterator&       it,
                                  const ByteBuffer::const_iterator& end)
{
  using Endian = visionary::Endian<visionary::endian::big, visionary::endian::native>;

  for (unsigned n = 4u; n > 0u; --n)
  {
    if (it == end)
    {
      // premature end of data
      return false;
    }
    if (*it != 0x02u)
    {
      // expected STX not found
      return false;
    }
    ++it;
  }

  if (!Endian::template convertFrom<std::uint32_t>(header.length, it, end))
  {
    return false;
  }
  if (!Endian::template convertFrom<std::uint8_t>(header.hubCntr, it, end))
  {
    return false;
  }
  if (!Endian::template convertFrom<std::uint8_t>(header.noC, it, end))
  {
    return false;
  }

  return true;
}

//-----------------------------------------------------------------------------------------------------

MockCoLa2Transport::MockCoLa2Transport() : m_forceReply(false)
{
}

MockCoLa2Transport& MockCoLa2Transport::sessionId(std::uint32_t sessionId)
{
  m_fakeSessionId = sessionId;
  return *this;
}

MockCoLa2Transport& MockCoLa2Transport::reqId(std::uint16_t reqId)
{
  m_fakeReqId = reqId;
  return *this;
}

MockCoLa2Transport& MockCoLa2Transport::cmdMode(const std::string& cmdMode)
{
  m_header.cmdMode = cmdMode;
  return *this;
}

MockCoLa2Transport& MockCoLa2Transport::name(const std::string& name)
{
  m_name = name;
  return *this;
}

MockCoLa2Transport& MockCoLa2Transport::returnvals(const ByteBuffer& returnvals)
{
  m_returnvals = returnvals;
  return *this;
}

ByteBuffer MockCoLa2Transport::buildCmd(const CmdReqRespHeader& header,
                                        const std::string&      name,
                                        const ByteBuffer&       parametersBuf)
{
  using Endian = visionary::Endian<visionary::endian::big, visionary::endian::native>;

  ByteBuffer buffer;
  buffer.reserve(parametersBuf.size() + 4u + 2u + 2u);

  appendTo(buffer, Endian::convertToVector<std::uint32_t>(header.sessionId));
  appendTo(buffer, Endian::convertToVector<std::uint16_t>(header.reqId));
  appendTo(buffer, header.cmdMode);

  for (auto c : name)
  {
    buffer.push_back(static_cast<std::uint8_t>(c));
  }

  appendTo(buffer, parametersBuf);

  return buildProtocol(header, buffer);
}

bool MockCoLa2Transport::parseCmd(CmdReqRespHeader&                 header,
                                  ByteBuffer::const_iterator&       it,
                                  const ByteBuffer::const_iterator& end)
{
  if (!parseProtocol(header, it, end))
  {
    return false;
  }

  using Endian = visionary::Endian<visionary::endian::big, visionary::endian::native>;

  if (!Endian::template convertFrom<std::uint32_t>(header.sessionId, it, end))
  {
    return false;
  }
  if (!Endian::template convertFrom<std::uint16_t>(header.reqId, it, end))
  {
    return false;
  }

  header.cmdMode.clear();
  header.cmdMode.reserve(2u);

  for (size_t n = 2u; n > 0; --n)
  {
    if (it == end)
    {
      // iterator hits end prematurely
      return false;
    }
    header.cmdMode.push_back(static_cast<char>(*it));
    ++it;
  }

  return true;
}

void MockCoLa2Transport::recvHandler()
{
  // we build a new package only at the send -> recv transition
  // (assumption: only one command was send, for which now the reply is read)
  const bool enableBuildPkg = (kRECV_STARTED == m_state);

  CmdReqRespHeader header;

  if (enableBuildPkg)
  {
    ByteBuffer::const_iterator it{m_mockSendBuffer.cbegin()};
    ByteBuffer::const_iterator end{m_mockSendBuffer.cend()};

    m_optCmdHeader.reset();

    const bool reqOk = parseCmd(header, it, end);
    if (reqOk)
    {
      m_optCmdHeader = header;
      m_cmdpayload.assign(it, end);
    }
  }

  MockTransport::recvHandler();

  if (enableBuildPkg)
  {
    if (m_forceReply || m_optCmdHeader.has_value())
    {
      m_header.hubCntr   = header.hubCntr;
      m_header.noC       = header.noC;
      m_header.sessionId = m_fakeSessionId.value_or(header.sessionId);
      m_header.reqId     = m_fakeReqId.value_or(header.reqId);

      m_mockRecvBuffer = buildCmd(m_header, m_name, m_returnvals);
    }
  }
}

} // namespace visionary_test
