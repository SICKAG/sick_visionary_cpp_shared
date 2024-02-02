//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include <cassert>
#include <limits> // for int max

#include "AuthenticationLegacy.h"
#include "AuthenticationSecure.h"
#include "CoLa2ProtocolHandler.h"
#include "CoLaBProtocolHandler.h"
#include "CoLaParameterReader.h"
#include "CoLaParameterWriter.h"
#include "ControlSession.h"
#include "TcpSocket.h"
#include "VisionaryControl.h"
#include "VisionaryEndian.h"

namespace visionary {

VisionaryControl::VisionaryControl()
  : m_protocolType(INVALID_PROTOCOL), m_sessionTimeout_ms(0), m_connectTimeout_ms(0), m_autoReconnect(false)
{
}

VisionaryControl::~VisionaryControl() = default;

bool VisionaryControl::open(ProtocolType       type,
                            const std::string& hostname,
                            uint32_t           sessionTimeout_ms,
                            bool               autoReconnect,
                            uint32_t           connectTimeout_ms)
{
  m_protocolType      = type;
  m_hostname          = hostname;
  m_sessionTimeout_ms = sessionTimeout_ms;
  m_connectTimeout_ms = connectTimeout_ms;
  m_autoReconnect     = autoReconnect;
  m_pProtocolHandler  = nullptr;
  m_pTransport        = nullptr;

  std::unique_ptr<TcpSocket> pTransport(new TcpSocket());

  if (INVALID_PROTOCOL == type)
  {
    return false;
  }

  const uint16_t portno = static_cast<uint16_t>(type); // type equals to port number
  if (pTransport->connect(hostname, portno, connectTimeout_ms) != 0)
  {
    return false;
  }

  std::unique_ptr<IProtocolHandler> pProtocolHandler;

  switch (type)
  {
    case COLA_B:
      pProtocolHandler = std::unique_ptr<IProtocolHandler>(new CoLaBProtocolHandler(*pTransport));
      break;
    case COLA_2:
      pProtocolHandler = std::unique_ptr<IProtocolHandler>(new CoLa2ProtocolHandler(*pTransport));
      break;
    default:
      assert(false /* unsupported protocol*/);
      return false;
  }

  const uint32_t sessionTimeout_s     = sessionTimeout_ms / 1000;
  const uint32_t max_sessionTimeout_s = std::numeric_limits<uint8_t>::max();

  if (sessionTimeout_s > max_sessionTimeout_s)
  {
    return false;
  }

  if (!pProtocolHandler->openSession(static_cast<uint8_t>(sessionTimeout_s)))
  {
    pTransport->shutdown();
    return false;
  }

  std::unique_ptr<ControlSession> pControlSession;
  pControlSession = std::unique_ptr<ControlSession>(new ControlSession(*pProtocolHandler));

  std::unique_ptr<IAuthentication> pAuthentication;
  switch (type)
  {
    case COLA_B:
      pAuthentication = std::unique_ptr<IAuthentication>(new AuthenticationLegacy(*this));
      break;
    case COLA_2:
      pAuthentication = std::unique_ptr<IAuthentication>(new AuthenticationSecure(*this));
      break;
    default:
      assert(false /* unsupported protocol*/);
      return false;
  }

  m_pTransport       = std::move(pTransport);
  m_pProtocolHandler = std::move(pProtocolHandler);
  m_pControlSession  = std::move(pControlSession);
  m_pAuthentication  = std::move(pAuthentication);

  return true;
}

void VisionaryControl::close()
{
  if (m_pAuthentication)
  {
    (void)m_pAuthentication->logout();
    m_pAuthentication = nullptr;
  }
  if (m_pProtocolHandler)
  {
    m_pProtocolHandler->closeSession();
    m_pProtocolHandler = nullptr;
  }
  if (m_pTransport)
  {
    m_pTransport->shutdown();
    m_pTransport = nullptr;
  }
  if (m_pControlSession)
  {
    m_pControlSession = nullptr;
  }
}

bool VisionaryControl::login(IAuthentication::UserLevel userLevel, const std::string& password)
{
  return m_pAuthentication->login(userLevel, password);
}

bool VisionaryControl::logout()
{
  return m_pAuthentication->logout();
}

std::string VisionaryControl::getDeviceIdent()
{
  const CoLaCommand command = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "DeviceIdent").build();

  CoLaCommand response = sendCommand(command);
  if (response.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(response).readFlexString();
  }
  else
  {
    return "";
  }
}

bool VisionaryControl::burstAcquisition(uint16_t burstLen)
{
  const CoLaCommand command =
    CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYBURST").parameterUInt(burstLen).build();

  CoLaCommand response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::startAcquisition()
{
  const CoLaCommand command = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYSTART").build();

  CoLaCommand response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::stepAcquisition()
{
  const CoLaCommand command  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYNEXT").build();
  CoLaCommand       response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::stopAcquisition()
{
  const CoLaCommand command  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "PLAYSTOP").build();
  CoLaCommand       response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

bool VisionaryControl::getDataStreamConfig()
{
  const CoLaCommand command  = CoLaParameterWriter(CoLaCommandType::METHOD_INVOCATION, "GetBlobClientConfig").build();
  CoLaCommand       response = sendCommand(command);

  return response.getError() == CoLaError::OK;
}

CoLaCommand VisionaryControl::sendCommand(const CoLaCommand& command)
{
  CoLaCommand response =
    (m_pControlSession != nullptr) ? m_pControlSession->send(command) : CoLaCommand(std::vector<uint8_t>());

  if (m_autoReconnect
      && (response.getError() == CoLaError::SESSION_UNKNOWN_ID || response.getError() == CoLaError::NETWORK_ERROR))
  {
    if (m_pTransport)
    {
      m_pTransport->shutdown();
    }
    const bool success = open(m_protocolType, m_hostname, m_sessionTimeout_ms, m_autoReconnect, m_connectTimeout_ms);
    if (success)
    {
      response = m_pControlSession->send(command);
    }
  }

  return response;
}

uint16_t VisionaryControl::GetBlobPort()
{
  const CoLaCommand command = CoLaParameterWriter(CoLaCommandType::READ_VARIABLE, "BlobTcpPortAPI").build();

  CoLaCommand response = sendCommand(command);
  if (response.getError() == CoLaError::OK)
  {
    return CoLaParameterReader(response).readUInt();
  }
  else
  {
    return 2114u; // default blob port
  }
}

} // namespace visionary
