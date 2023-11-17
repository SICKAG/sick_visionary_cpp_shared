//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include "CoLaCommand.h"
#include "ControlSession.h"
#include "IAuthentication.h"
#include "IProtocolHandler.h"
#include "TcpSocket.h"
#include <cstdint>
#include <memory>
#include <string>

namespace visionary {

class VisionaryControl
{
public:
  /// The numbers used for the protocols are the port numbers.
  enum ProtocolType
  {
    INVALID_PROTOCOL = -1,
    COLA_A           = 2111,
    COLA_B           = 2112,
    COLA_2           = 2122
  };

  /// Default session timeout
  static constexpr uint32_t kSessionTimeout_ms = 5000;

  VisionaryControl();
  ~VisionaryControl();

  /// Opens a connection to a Visionary sensor
  ///
  /// \param[in] type     protocol type the sensor understands (CoLa-A, CoLa-B or CoLa-2).
  ///                     This information is found in the sensor documentation.
  /// \param[in] hostname name or IP address of the Visionary sensor.
  /// \param[in] sessionTimeout_ms Timeout for Session (only used for Cola2)
  /// \param[in] autoReconnect Auto reconnect when connection was lost
  /// \param[in] connectTimeout_ms Timeout for Connection
  ///
  /// \retval true The connection to the sensor successfully was established.
  /// \retval false The connection attempt failed; the sensor is either
  ///               - switched off or has a different IP address or name
  ///               - not available using for PCs network settings (different subnet)
  ///               - the protocol type or the port did not match. Please check your sensor documentation.
  bool open(ProtocolType       type,
            const std::string& hostname,
            uint32_t           sessionTimeout_ms = kSessionTimeout_ms,
            bool               autoReconnect     = true,
            uint32_t           connectTimeout_ms = kSessionTimeout_ms);

  /// Close a connection
  ///
  /// Closes the control connection. It is allowed to call close of a connection
  /// that is not open. In this case this call is a no-op.
  void close();

  /// Login to the device.
  ///
  /// \param[in] userLevel The user level to login as.
  /// \param[in] password   Password for the selected user level.
  /// \return error code, 0 on success
  bool login(IAuthentication::UserLevel userLevel, const std::string& password);

  /// <summary>Logout from the device.</summary>
  /// <returns>True if logout was successful, false otherwise.</returns>
  bool logout();

  /// <summary>
  /// Get device information by calling the "DeviceIdent" method on the device.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  std::string getDeviceIdent();

  /// <summary>
  /// Get blob port address.
  /// </summary>
  /// <returns>Blob port, typically 2114.</returns>
  uint16_t GetBlobPort();

  /// <summary>
  /// Start streaming a burst of data by calling the "PLAYBURST" method on the device. Works only when acquisition is
  /// stopped.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  bool burstAcquisition(uint16_t burstLen);

  /// <summary>
  /// Start streaming the data by calling the "PLAYSTART" method on the device. Works only when acquisition is stopped.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  bool startAcquisition();

  /// <summary>
  /// Trigger a single image on the device. Works only when acquisition is stopped.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  bool stepAcquisition();

  /// <summary>
  /// Stops the data stream. Works always, also when acquisition is already stopped before.
  /// </summary>
  bool stopAcquisition();

  /// <summary>
  /// Tells the device that there is a streaming channel by invoking a method named GetBlobClientConfig.
  /// </summary>
  /// <returns>True if successful, false otherwise.</returns>
  bool getDataStreamConfig();

  /// <summary>Send a <see cref="CoLaBCommand" /> to the device and waits for the result.</summary>
  /// <param name="command">Command to send</param>
  /// <returns>The response.</returns>
  CoLaCommand sendCommand(const CoLaCommand& command);

private:
  std::string receiveCoLaResponse();
  CoLaCommand receiveCoLaCommand();

  std::unique_ptr<TcpSocket>        m_pTransport;
  std::unique_ptr<IProtocolHandler> m_pProtocolHandler;
  std::unique_ptr<IAuthentication>  m_pAuthentication;
  std::unique_ptr<ControlSession>   m_pControlSession;

  ProtocolType m_protocolType;
  std::string  m_hostname;
  uint32_t     m_sessionTimeout_ms;
  uint32_t     m_connectTimeout_ms;
  bool         m_autoReconnect;
};

} // namespace visionary
