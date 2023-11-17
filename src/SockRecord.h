//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

// Include socket
#ifdef _WIN32 // Windows specific
#  ifdef NOMINMAX
#    undef NOMINMAX
#  endif
#  define NOMINMAX

#  include <winsock2.h>
#  include <ws2tcpip.h>
// to use with other compiler than Visual C++ need to set Linker flag -lws2_32
#  ifdef _MSC_VER
#    pragma comment(lib, "ws2_32.lib")
#  endif
#else // Linux specific
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>

typedef int SOCKET;
#  define INVALID_SOCKET (SOCKET(-1))
#  define SOCKET_ERROR (-1)
#endif

namespace visionary {

class SockRecord
{
public:
  SockRecord() : m_socket(INVALID_SOCKET)
  {
  }

  SockRecord(SOCKET socket)
  {
    m_socket = socket;
  }

  bool isValid() const
  {
    return m_socket != INVALID_SOCKET;
  }
  SOCKET socket() const
  {
    return m_socket;
  }

  void set(SOCKET socket)
  {
    m_socket = socket;
  }
  void invalidate()
  {
    m_socket = INVALID_SOCKET;
  }

private:
  SOCKET m_socket;

}; // end class SockRecord

} // namespace visionary
