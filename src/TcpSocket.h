//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <memory> // for unique_ptr
#include <string>
#include <vector>

#include "ITransport.h"

namespace visionary {

class SockRecord; // forward definition

class TcpSocket : public ITransport
{
public:
  using ByteBuffer = std::vector<std::uint8_t>;

  TcpSocket();
  virtual ~TcpSocket();

  /// connect to a peer via TCP
  ///
  /// \param[in] ipaddr string representation of the device ip address ("xx.xx.xx.xx")
  /// \param[in] port number of the device port to connect to (in host byte order)
  /// \retval 0 connect successful
  /// \retval -1 connect failed (the socket could be created, the ip address format is invalid or the device cannot be
  /// contacted)
  int connect(const std::string& ipaddr, std::uint16_t port, std::uint32_t timeoutMs = 5000);
  int shutdown() override;
  int getLastError() override;

  using ITransport::send;
  send_return_t send(const char* pData, size_t size) override;
  recv_return_t recv(ByteBuffer& buffer, std::size_t maxBytesToReceive) override;
  recv_return_t read(ByteBuffer& buffer, std::size_t nBytesToReceive) override;

private:
  std::unique_ptr<SockRecord> m_pSockRecord; // buffer for a SOCKET
};

} // namespace visionary
