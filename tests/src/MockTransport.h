//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

#include <ITransport.h>

namespace visionary_test {

using ByteBuffer = std::vector<std::uint8_t>;

// poor man's optional; since std::optional is only be available on C++17+
template <typename T>
class Opt
{
public:
  Opt()
  {
    reset();
  }
  Opt(const T& val)
  {
    *this = val;
  }

  Opt& operator=(const T& val)
  {
    m_value     = val;
    m_has_value = true;
    return *this;
  }
  operator bool() const
  {
    return has_value();
  }
  const T& operator*() const
  {
    return value();
  }
  T& operator*()
  {
    return value();
  }
  const T* operator->() const
  {
    return &(value());
  }
  T* operator->()
  {
    return &(value());
  }

  bool has_value() const
  {
    return m_has_value;
  }
  T& value()
  {
    assert_value();
    return m_value;
  }
  const T& value() const
  {
    assert_value();
    return m_value;
  }
  void reset()
  {
    m_has_value = false;
  }

  template <typename U>
  T value_or(const U& default_value) const
  {
    return has_value() ? value() : default_value;
  }

protected:
  void assert_value() const
  {
    if (!m_has_value)
      throw std::runtime_error("value not set");
  }

private:
  bool m_has_value;
  T    m_value;
};

class MockTransport : public visionary::ITransport
{
public:
  struct ProtocolReqRespHeader
  {
    ProtocolReqRespHeader() : length(0u), hubCntr(0u), noC(0u)
    {
    }
    std::uint32_t length;
    std::uint8_t  hubCntr;
    std::uint8_t  noC;
  };

  static void nop();

  MockTransport();
  MockTransport(const ByteBuffer& buffer);
  MockTransport(std::initializer_list<ByteBuffer::value_type> init);

  virtual ~MockTransport();

  MockTransport& recvBuffer(const ByteBuffer& buffer);
  MockTransport& recvBuffer(std::initializer_list<ByteBuffer::value_type> init);
  MockTransport& noFakeSendReturn();
  MockTransport& fakeSendReturn(send_return_t retval);

  MockTransport& onSend(std::function<void()> fct);
  MockTransport& onRecv(std::function<void()> fct);

  ByteBuffer& sendBuffer()
  {
    return m_mockSendBuffer;
  }

  send_return_t send(const char* buffer, size_t size) override;
  recv_return_t recv(ByteBuffer& buffer, std::size_t maxBytesToReceive) override;
  recv_return_t read(ByteBuffer& buffer, std::size_t nBytesToReceive) override;

  int shutdown() override;
  int getLastError() override;

  static ByteBuffer buildProtocol(const ProtocolReqRespHeader& header, const ByteBuffer& payload);
  static bool       parseProtocol(ProtocolReqRespHeader&            header,
                                  ByteBuffer::const_iterator&       it,
                                  const ByteBuffer::const_iterator& end);

protected:
  virtual void recvHandler();
  virtual void sendHandler();

  // optional send return value override
  Opt<send_return_t> m_fakeSendReturn;

  enum
  {
    kSEND_IDLE,
    kSEND_STARTED,
    kSEND_CONTD,
    kRECV_STARTED,
    kRECV_CONTD
  } m_state;

  // callback when a recv was called (on the beginning of recv)
  std::function<void()> m_onRecv;
  // callback when a send was called (on the beginning of send)
  std::function<void()> m_onSend;
  ByteBuffer            m_mockRecvBuffer;
  ByteBuffer            m_mockSendBuffer;
};

class MockCoLa2Transport : public MockTransport
{
public:
  struct CmdReqRespHeader : ProtocolReqRespHeader
  {
    CmdReqRespHeader() : sessionId(0u), reqId(0u), cmdMode("xx")
    {
    }
    std::uint32_t sessionId;
    std::uint16_t reqId;
    std::string   cmdMode;
  };

  MockCoLa2Transport();
  virtual ~MockCoLa2Transport() = default;

  MockCoLa2Transport& sessionId(std::uint32_t sessionId);
  MockCoLa2Transport& reqId(std::uint16_t reqId);
  MockCoLa2Transport& cmdMode(const std::string& cmdMode);
  MockCoLa2Transport& name(const std::string& name);
  MockCoLa2Transport& returnvals(const ByteBuffer& parameterBuf);
  MockCoLa2Transport& forceReply(bool force = true);

  const Opt<CmdReqRespHeader>& cmdHeader() const
  {
    return m_optCmdHeader;
  }
  const ByteBuffer& cmdpayload() const
  {
    return m_cmdpayload;
  }

  static ByteBuffer buildCmd(const CmdReqRespHeader& header, const std::string& name, const ByteBuffer& parametersBuf);
  static bool parseCmd(CmdReqRespHeader& header, ByteBuffer::const_iterator& it, const ByteBuffer::const_iterator& end);

protected:
  void recvHandler() override;

  // header used in the mocked respose package
  CmdReqRespHeader m_header;
  // name of variablke or method to use in mocked response. needs to be space-delimited!
  std::string m_name;
  // buffer containing the parameter payload
  ByteBuffer m_returnvals;

  // parsed command header (if successfully parsed)
  // if the parsing was not successful, the Opt is empty
  Opt<CmdReqRespHeader> m_optCmdHeader;

  // buffer containing the parsed parameter payload
  ByteBuffer m_cmdpayload;

  // optional session overwrite value
  Opt<std::uint32_t> m_fakeSessionId;
  // optional requiest overwrite value
  Opt<std::uint16_t> m_fakeReqId;
  // flag whether the response should given even if the package could not be parsed
  bool m_forceReply;
};

} // namespace visionary_test
