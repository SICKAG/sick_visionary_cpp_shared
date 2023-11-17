//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#if !defined(_WIN32)
// we assume something Linux'ish here
#  include <sys/types.h> // for ssize_t
#endif

namespace visionary {

class ITransport
{
public:
  using ByteBuffer = std::vector<std::uint8_t>;

#if defined(_WIN32)
  using send_return_t = int;
  using recv_return_t = int;
#else
  using send_return_t = ssize_t;
  using recv_return_t = ssize_t;
#endif

  virtual ~ITransport() = default; // destructor, use it to call destructor of the inherit classes

  virtual int shutdown()     = 0;
  virtual int getLastError() = 0;

  /// Send data on socket to device
  ///
  /// \e All bytes are sent on the socket. It is regarded as error if this is not possible.
  ///
  /// \param[in] buffer buffer containing the bytes that shall be sent.
  ///
  /// \return Number of bytes sent or (-1) on error
  template <typename T>
  send_return_t send(const std::vector<T>& buffer)
  {
    return send(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(T));
  }

  /// Receive data on socket to device
  ///
  /// Receive at most \a maxBytesToReceive bytes.
  ///
  /// \param[in] buffer buffer containing the bytes that shall be sent.
  /// \param[in] maxBytesToReceive maximum number of bytes to receive.
  ///
  /// \return number of received bytes or (-1) on error
  virtual recv_return_t recv(ByteBuffer& buffer, std::size_t maxBytesToReceive) = 0;

  /// Read a number of bytes
  ///
  /// Contrary to recv this method reads precisely \a nBytesToReceive bytes.
  ///
  /// \param[in] buffer buffer containing the bytes that shall be sent.
  /// \param[in] nBytesToReceive maximum number of bytes to receive.
  ///
  /// \return number of received bytes or (-1) on error
  virtual recv_return_t read(ByteBuffer& buffer, std::size_t nBytesToReceive) = 0;

protected:
  virtual send_return_t send(const char* pData, size_t size) = 0;
};

} // namespace visionary
