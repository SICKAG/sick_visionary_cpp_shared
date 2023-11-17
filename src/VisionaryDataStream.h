//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include "TcpSocket.h"
#include "VisionaryData.h"
#include <memory>

namespace visionary {

class VisionaryDataStream
{
public:
  using ByteBuffer = std::vector<std::uint8_t>;

  VisionaryDataStream(std::shared_ptr<VisionaryData> dataHandler);
  ~VisionaryDataStream();

  /// Opens a connection to a Visionary sensor
  ///
  /// \param[in] hostname name or IP address of the Visionary sensor.
  /// \param[in] port     control command port of the sensor, usually 2112 for CoLa-B or 2122 for CoLa-2 (given in
  /// host-byte order). \param[in] timeoutMs controls the socket timeout, default 5000ms
  ///
  /// \retval true The connection to the sensor successfully was established.
  /// \retval false The connection attempt failed; the sensor is either
  ///               - switched off or has a different IP address or name
  ///               - not available using for PCs network settings (different subnet)
  ///               - the protocol type or the port did not match. Please check your sensor documentation.
  bool open(const std::string& hostname, std::uint16_t port, std::uint32_t timeoutMs = 5000u);

  /// Sets a socket used for the connection to a Visionary sensor
  /// The socket must already be ready to use and opened.
  ///
  /// \param[in] pTransport the socket
  ///
  /// \retval true Socket has been set
  bool open(std::unique_ptr<ITransport>& pTransport);

  /// Close a connection
  ///
  /// Closes the connection. It is allowed to call close of a connection
  /// that is not open. In this case this call is a no-op.
  void close();

  bool syncCoLa() const;

  //-----------------------------------------------
  // Receive a single blob from the connected device and store it in buffer.
  // Returns true when valid frame completely received.
  bool getNextFrame();

  /// Checks if connection is established
  ///
  /// \attention To check if the connection is estabilished data has to be
  ///  sent to the camera which means this is a costly operation. Should only be
  ///  used when get getNextFrame fails.
  ///
  /// \retval true The connection to the sensor is established.
  /// \retval false The connection to the sensor is lost. A reconnection by
  ///               calling close + open is necessary.
  bool isConnected() const;

  /// Sets a new data handler
  ///
  /// \param[in] dataHandler a Datahandler.
  void setDataHandler(std::shared_ptr<VisionaryData> dataHandler);

  /// Gets the data handler
  ///
  /// \retval the dataHandler
  std::shared_ptr<VisionaryData> getDataHandler();

private:
  std::shared_ptr<VisionaryData> m_dataHandler;
  std::unique_ptr<ITransport>    m_pTransport;

  // Parse the Segment-Binary-Data (Blob data without protocol version and packet type).
  // Returns true when parsing was successful.
  bool parseSegmentBinaryData(const ByteBuffer::iterator itBuf, std::size_t bufferSize);
};

} // namespace visionary
