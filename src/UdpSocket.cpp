//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "UdpSocket.h"

#include <algorithm> // for max
#include <cstring>
#include <iostream>

#include "NumericConv.h"
#include "SockRecord.h"

namespace visionary {

// Windows uses int also for send/recv buffer sizes and return values
#ifdef _WIN32
using bufsize_t = int;
#else
using bufsize_t = size_t;
#endif

class SockAddrIn
{
public:
  SockAddrIn()
  {
    reset();
  }

  void reset()
  {
    memset(&sockaddr_in, 0, sizeof(sockaddr_in));
  }

  struct sockaddr_in sockaddr_in;
};

UdpSocket::UdpSocket() : m_pSockRecord(new SockRecord()), m_pSockAddrIn(new SockAddrIn)
{
}

UdpSocket::~UdpSocket()
{
  if (m_pSockRecord->isValid())
  {
    shutdown();
  }
}

int UdpSocket::connect(const std::string& ipaddr, std::uint16_t port)
{
  int  iResult        = 0;
  int  trueVal        = 1;
  long timeoutSeconds = 5L;

  if (m_pSockRecord->isValid())
  {
    shutdown();
  }

#ifdef _WIN32
  //-----------------------------------------------
  // Initialize Winsock
  WSADATA wsaData;
  iResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != NO_ERROR)
  {
    return iResult;
  }
#endif

  //-----------------------------------------------
  // Create a receiver socket to receive datagrams
  SOCKET hsock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (hsock == INVALID_SOCKET)
  {
    m_pSockRecord->invalidate();
    return -1;
  }
  m_pSockRecord->set(hsock);

  //-----------------------------------------------
  // Bind the socket to any address and the specified port.
  m_pSockAddrIn->sockaddr_in.sin_family = AF_INET;
  m_pSockAddrIn->sockaddr_in.sin_port   = htons(port);
  if (::inet_pton(AF_INET, ipaddr.c_str(), &m_pSockAddrIn->sockaddr_in.sin_addr.s_addr) <= 0)
  {
    // invalid address or buffer size
    return -1;
  }

  // Set the timeout for the socket to 5 seconds
#ifdef _WIN32
  // On Windows timeout is a DWORD in milliseconds
  // (https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-setsockopt)
  long timeoutMs = timeoutSeconds * 1000L;
  iResult        = ::setsockopt(
    m_pSockRecord->socket(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
#else
  struct timeval tv;
  tv.tv_sec  = timeoutSeconds; /* 5 seconds Timeout */
  tv.tv_usec = 0L;             // Not init'ing this can cause strange errors
  iResult    = ::setsockopt(
    m_pSockRecord->socket(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(struct timeval));
#endif

  if (iResult >= 0)
  {
    iResult = ::setsockopt(
      m_pSockRecord->socket(), SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&trueVal), sizeof(trueVal));
  }

  return iResult;
}

int UdpSocket::shutdown()
{
  if (m_pSockRecord->isValid())
  {
    // Close the socket when finished receiving datagrams
#ifdef _WIN32
    ::closesocket(m_pSockRecord->socket());
    ::WSACleanup();
#else
    ::close(m_pSockRecord->socket());
#endif
    m_pSockRecord->invalidate();
  }
  return 0;
}

ITransport::send_return_t UdpSocket::send(const char* pData, size_t size)
{
  // send buffer via UDP socket
  return ::sendto(m_pSockRecord->socket(),
                  pData,
                  castClamped<bufsize_t>(size),
                  0,
                  reinterpret_cast<struct sockaddr*>(&m_pSockAddrIn->sockaddr_in),
                  sizeof(m_pSockAddrIn->sockaddr_in));
}

ITransport::recv_return_t UdpSocket::recv(ByteBuffer& buffer, std::size_t maxBytesToReceive)
{
  const bufsize_t eff_maxsize = castClamped<bufsize_t>(maxBytesToReceive);

  buffer.resize(static_cast<std::size_t>(eff_maxsize));
  char* pBuffer = reinterpret_cast<char*>(buffer.data());

  // receive from UDP Socket
  const ITransport::recv_return_t retval = ::recv(m_pSockRecord->socket(), pBuffer, eff_maxsize, 0);

  if (retval >= 0)
  {
    buffer.resize(static_cast<size_t>(retval));
  }

  return retval;
}

ITransport::recv_return_t UdpSocket::read(ByteBuffer& buffer, std::size_t nBytesToReceive)
{
  // receive from UDP Socket
  buffer.resize(nBytesToReceive);

  char* const pBufferStart = reinterpret_cast<char*>(buffer.data());
  char*       pBuffer      = pBufferStart;

  while (nBytesToReceive > 0)
  {
    const bufsize_t eff_maxsize = castClamped<bufsize_t>(nBytesToReceive);

    const ITransport::recv_return_t bytesReceived = ::recv(m_pSockRecord->socket(), pBuffer, eff_maxsize, 0);

    if (bytesReceived == SOCKET_ERROR) // 0 is also possible, but allowed - an
                                       // empty datagram was sent
    {
      return -1;
    }
    pBuffer += bytesReceived;
    nBytesToReceive -= static_cast<size_t>(bytesReceived);
  }

  buffer.resize(static_cast<size_t>(pBuffer - pBufferStart));

  return static_cast<ITransport::recv_return_t>(buffer.size());
}

int UdpSocket::getLastError()
{
  int error_code;
#ifdef _WIN32
  int error_code_size = sizeof(int);
  ::getsockopt(m_pSockRecord->socket(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error_code), &error_code_size);
#else
  socklen_t error_code_size = sizeof error_code;
  if (::getsockopt(m_pSockRecord->socket(), SOL_SOCKET, SO_ERROR, &error_code, &error_code_size) != 0)
  {
    std::cout << "Error getting error code" << std::endl;
  }

#endif
  return error_code;
}

} // namespace visionary
