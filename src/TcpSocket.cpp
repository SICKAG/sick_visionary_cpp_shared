//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "TcpSocket.h"

#include <fcntl.h>

#include <iostream>
#include <stdexcept>

#include "NumericConv.h"
#include "SockRecord.h"

namespace visionary {

#ifdef _WIN32
using bufsize_t = int;
#else
using bufsize_t = size_t;
#endif

TcpSocket::TcpSocket() : m_pSockRecord(new SockRecord())
{
}

TcpSocket::~TcpSocket()
{
  if (m_pSockRecord->isValid())
  {
    shutdown();
  }
}

int TcpSocket::connect(const std::string& ipaddr, std::uint16_t port, std::uint32_t timeoutMs)
{
  int iResult = 0;

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
  SOCKET hsock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (hsock == INVALID_SOCKET)
  {
    m_pSockRecord->invalidate();
    return -1;
  }

  m_pSockRecord->set(hsock);

  //-----------------------------------------------
  // Bind the socket to any address and the specified port.
  sockaddr_in recvAddr{};
  recvAddr.sin_family = AF_INET;
  recvAddr.sin_port   = htons(port);
  if (::inet_pton(AF_INET, ipaddr.c_str(), &recvAddr.sin_addr.s_addr) <= 0)
  {
    // invalid address or buffer size
    return -1;
  }
  {
#ifdef _WIN32
    unsigned long block = 1;
    ::ioctlsocket(static_cast<unsigned int>(m_pSockRecord->socket()), static_cast<int>(FIONBIO), &block);
#else
    int flags = fcntl(m_pSockRecord->socket(), F_GETFL, 0);
    if (flags == -1)
    {
      close(m_pSockRecord->socket());

      m_pSockRecord->invalidate();
      return -1;
    }
    flags |= O_NONBLOCK;
    if (::fcntl(m_pSockRecord->socket(), F_SETFL, flags) == -1)
    {
      close(m_pSockRecord->socket());
      m_pSockRecord->invalidate();
      return -1;
    }
#endif
  }
  // calculate the timeout in seconds and microseconds
#ifdef _WIN32
  long timeoutSeconds  = static_cast<long>(timeoutMs / 1000U);
  long timeoutUSeconds = static_cast<long>((timeoutMs % 1000U) * 1000U);
#else
  time_t      timeoutSeconds  = static_cast<time_t>(timeoutMs / 1000U);
  suseconds_t timeoutUSeconds = static_cast<time_t>((timeoutMs % 1000U) * 1000U);
#endif
  // applying the calculated values
  struct timeval tv;
  tv.tv_sec  = timeoutSeconds;
  tv.tv_usec = timeoutUSeconds;

  iResult = ::connect(m_pSockRecord->socket(), reinterpret_cast<sockaddr*>(&recvAddr), sizeof(recvAddr));
  if (iResult != 0)
  {
#ifdef _WIN32
    if (::WSAGetLastError() != WSAEWOULDBLOCK)
    {
      ::closesocket(m_pSockRecord->socket());
      m_pSockRecord->invalidate();
      return -1;
    }
#else
    if (errno != EINPROGRESS)
    {
      close(m_pSockRecord->socket());
      m_pSockRecord->invalidate();
      return -1;
    }
#endif
    fd_set setW, setE;
    FD_ZERO(&setW);
    FD_SET(m_pSockRecord->socket(), &setW);
    FD_ZERO(&setE);
    FD_SET(m_pSockRecord->socket(), &setE);
    int ret = ::select(static_cast<int>(m_pSockRecord->socket() + 1), nullptr, &setW, &setE, &tv);
#ifdef _WIN32
    if (ret <= 0)
    {
      // select() failed or connection timed out
      ::closesocket(m_pSockRecord->socket());
      m_pSockRecord->invalidate();
      if (ret == 0)
      {
        ::WSASetLastError(WSAETIMEDOUT);
      }
      return -1;
    }
    if (FD_ISSET(m_pSockRecord->socket(), &setE))
    {
      // connection failed
      int error_code;
      int error_code_size = sizeof(error_code);
      ::getsockopt(
        m_pSockRecord->socket(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error_code), &error_code_size);
      ::closesocket(m_pSockRecord->socket());
      m_pSockRecord->invalidate();
      ::WSASetLastError(error_code);
      return -1;
    }
#else
    {
      int       so_error;
      socklen_t len = sizeof(so_error);
      ::getsockopt(m_pSockRecord->socket(), SOL_SOCKET, SO_ERROR, &ret, &len);
      if (ret != 0)
      {
        ::close(m_pSockRecord->socket());
        m_pSockRecord->invalidate();
        return ret;
      }
    }
#endif
  }

  {
#ifdef _WIN32
    unsigned long block = 0;
    if (::ioctlsocket(m_pSockRecord->socket(), static_cast<int>(FIONBIO), &block) == SOCKET_ERROR)
    {
      ::closesocket(m_pSockRecord->socket());
      m_pSockRecord->invalidate();
      return -1;
    }
#else
    {
      int flags = ::fcntl(m_pSockRecord->socket(), F_GETFL, 0);
      if (flags == -1)
      {
        ::close(m_pSockRecord->socket());
        m_pSockRecord->invalidate();
        return -1;
      }
      flags &= ~O_NONBLOCK;
      if (::fcntl(m_pSockRecord->socket(), F_SETFL, flags) == -1)
      {
        ::close(m_pSockRecord->socket());
        m_pSockRecord->invalidate();
        return -1;
      }
    }
#endif
  }
  // Set the timeout for the socket
#ifdef _WIN32
  // On Windows timeout is a DWORD in milliseconds
  // (https://docs.microsoft.com/en-us/windows/desktop/api/winsock/nf-winsock-setsockopt)
  iResult = ::setsockopt(
    m_pSockRecord->socket(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(DWORD));
#else
  iResult = ::setsockopt(
    m_pSockRecord->socket(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(struct timeval));
#endif
  return iResult;
}

int TcpSocket::shutdown()
{
  // Close the socket when finished receiving datagrams
#ifdef _WIN32
  ::closesocket(m_pSockRecord->socket());
  ::WSACleanup();
#else
  ::close(m_pSockRecord->socket());
#endif
  m_pSockRecord->invalidate();
  return 0;
}

ITransport::send_return_t TcpSocket::send(const char* pData, size_t size)
{
  const bufsize_t eff_buflen = castClamped<bufsize_t>(size);
  // send buffer via TCP socket
  return ::send(m_pSockRecord->socket(), pData, eff_buflen, 0);
}

ITransport::recv_return_t TcpSocket::recv(ByteBuffer& buffer, std::size_t maxBytesToReceive)
{
  const bufsize_t eff_maxsize = castClamped<bufsize_t>(maxBytesToReceive);

  // receive from TCP Socket
  buffer.resize(static_cast<std::size_t>(eff_maxsize));
  char* pBuffer = reinterpret_cast<char*>(buffer.data());

  const ITransport::recv_return_t retval = ::recv(m_pSockRecord->socket(), pBuffer, eff_maxsize, 0);

  if (retval >= 0)
  {
    buffer.resize(static_cast<size_t>(retval));
  }

  return retval;
}

ITransport::recv_return_t TcpSocket::read(ByteBuffer& buffer, std::size_t nBytesToReceive)
{
  // receive from TCP Socket
  try
  {
    buffer.resize(nBytesToReceive);
  }
  catch (std::length_error&)
  {
    // Supress warning for sizes >= 125MB, because it is very likely an invalid
    // size
    if (nBytesToReceive < 1024u * 1024u * 125u)
    {
      std::cout << "TcpSocket::" << __FUNCTION__ << ": Unable to allocate buffer of size " << nBytesToReceive
                << std::endl;
    }
    return -1;
  }

  char* const pBufferStart = reinterpret_cast<char*>(buffer.data());
  char*       pBuffer      = pBufferStart;

  while (nBytesToReceive > 0)
  {
    const bufsize_t eff_maxsize = castClamped<bufsize_t>(nBytesToReceive);

    const ITransport::recv_return_t bytesReceived = ::recv(m_pSockRecord->socket(), pBuffer, eff_maxsize, 0);

    if (bytesReceived == SOCKET_ERROR)
    {
      return -1;
    }
    else if (bytesReceived == 0)
    {
      // stream was properly closed
      break;
    }
    pBuffer += bytesReceived;
    nBytesToReceive -= static_cast<size_t>(bytesReceived);
  }

  buffer.resize(static_cast<size_t>(pBuffer - pBufferStart));

  return static_cast<ITransport::recv_return_t>(buffer.size());
}

int TcpSocket::getLastError()
{
  int error_code;
#ifdef _WIN32
  int error_code_size = sizeof(int);
  if (::getsockopt(
        m_pSockRecord->socket(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error_code), &error_code_size)
      != 0)
  {
    std::cout << "Error getting error code" << std::endl;
  }
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
