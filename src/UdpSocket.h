//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <memory> // for unique_ptr
#include <string>

#include "ITransport.h"

namespace visionary {

class SockRecord; // forward definition
class SockAddrIn;

class UdpSocket : public ITransport
{
  class Conv;
  friend class Conv;

public:
  UdpSocket();
  virtual ~UdpSocket();

  /// connect to a peer via UDP
  ///
  /// actually UDP is connectionless, so this method just stored the target address and
  /// uses thestored adresse for successive calls to \a send, \ recv and \a read.
  ///
  /// \param[in] ipaddr string representation of the device ip address ("xx.xx.xx.xx")
  /// \param[in] port number of the device port to connect to (in host byte order)
  /// \retval 0 connect successful (in this case: the socket could be created)
  /// \retval -1 connect failed (in this case: no socket could be created or the ip address format is invalid)
  int connect(const std::string& ipaddr, std::uint16_t port);
  int shutdown() override;
  int getLastError() override;

  using ITransport::send;

  send_return_t send(const char* pData, size_t size) override;
  recv_return_t recv(ByteBuffer& buffer, std::size_t maxBytesToReceive) override;
  recv_return_t read(ByteBuffer& buffer, std::size_t nBytesToReceive) override;

private:
  std::unique_ptr<SockRecord> m_pSockRecord; // buffer for a SOCKET
  std::unique_ptr<SockAddrIn> m_pSockAddrIn; // buffer for sockaddr_in
};

} // namespace visionary
