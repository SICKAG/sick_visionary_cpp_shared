//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "CoLaCommand.h"

#include <algorithm> // for find
#include <string>

#include "VisionaryEndian.h"

namespace visionary {

bool CoLaCommand::fromBuffer(const ByteBuffer& buffer)
{
  using EndianConv = Endian<endian::big, endian::native>;

  const auto begin = buffer.cbegin();
  auto       it    = begin;
  const auto end   = buffer.end();

  std::string typeStr;
  typeStr.reserve(3u);

  // we extract the 3 character type, s??
  while ((it < end) && (typeStr.size() < 3))
  {
    typeStr.push_back(static_cast<char>(*it++));
  }

  if (typeStr == "sRN")
    m_type = CoLaCommandType::READ_VARIABLE;
  else if (typeStr == "sRA")
    m_type = CoLaCommandType::READ_VARIABLE_RESPONSE;
  else if (typeStr == "sWN")
    m_type = CoLaCommandType::WRITE_VARIABLE;
  else if (typeStr == "sWA")
    m_type = CoLaCommandType::WRITE_VARIABLE_RESPONSE;
  else if (typeStr == "sMN")
    m_type = CoLaCommandType::METHOD_INVOCATION;
  else if (typeStr == "sAN")
    m_type = CoLaCommandType::METHOD_RETURN_VALUE;
  else if (typeStr == "sFA")
    m_type = CoLaCommandType::COLA_ERROR;
  else
    m_type = CoLaCommandType::UNKNOWN; // this also catches a too-short type string

  switch (m_type)
  {
    case CoLaCommandType::COLA_ERROR:
      // Read error code
      m_parameterOffset = static_cast<std::size_t>(it - begin);
      std::uint16_t erroru16;
      if (!EndianConv::convertFrom(erroru16, it, end))
      {
        m_error = CoLaError::UNKNOWN;
        return false;
      }
      m_error = static_cast<CoLaError::Enum>(erroru16);
      break;

    case CoLaCommandType::READ_VARIABLE:
    case CoLaCommandType::READ_VARIABLE_RESPONSE:
    case CoLaCommandType::WRITE_VARIABLE:
    case CoLaCommandType::WRITE_VARIABLE_RESPONSE:
    case CoLaCommandType::METHOD_INVOCATION:
    case CoLaCommandType::METHOD_RETURN_VALUE:
    {
      // we sent a named request, thus expect a named response, where the requested variable/method name is echoed
      // delimited by spaces

      if ((it == end) || (static_cast<char>(*it) != ' '))
      {
        // no space. exit// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        m_parameterOffset = static_cast<std::size_t>(it - begin);
        m_error           = CoLaError::UNKNOWN;
        return false;
      }
      // skip space
      ++it;

      auto name_start = it;
      it              = std::find(it, end, static_cast<std::uint8_t>(' '));
      if (it == end)
      {
        // space not found
        m_parameterOffset = static_cast<std::size_t>(it - begin);
        m_error           = CoLaError::UNKNOWN;
        return false;
      }

      // copy name
      m_name.reserve(static_cast<std::size_t>(it - name_start));
      for (auto nameit = name_start; nameit < it; ++nameit)
      {
        m_name.push_back(static_cast<char>(*nameit));
      }

      // skip space
      ++it;

      m_parameterOffset = static_cast<std::size_t>(it - begin);

      m_error = CoLaError::OK;
    }
    break;

    default:
      // something UNKNOWN
      m_parameterOffset = 0;
      m_type            = CoLaCommandType::UNKNOWN;
      m_error           = CoLaError::UNKNOWN;
      return false;
      break;
  }

  return true;
}

CoLaCommand::CoLaCommand(CoLaCommandType::Enum commandType, CoLaError::Enum error, const std::string& name)
  : m_type(commandType), m_name(name), m_parameterOffset(0u), m_error(error)
{
}

CoLaCommand::CoLaCommand(const ByteBuffer& buffer)
  : m_buffer(buffer), m_type(CoLaCommandType::UNKNOWN), m_parameterOffset(0), m_error(CoLaError::UNKNOWN)
{
  if (!fromBuffer(buffer))
  {
    // no valid package, we reset all values to UNKNOWN
    m_type  = CoLaCommandType::UNKNOWN;
    m_error = CoLaError::UNKNOWN;
  }
}

CoLaCommand::~CoLaCommand() = default;

const CoLaCommand::ByteBuffer& CoLaCommand::getBuffer() const
{
  return m_buffer;
}

CoLaCommandType::Enum CoLaCommand::getType() const
{
  return m_type;
}

const char* CoLaCommand::getName() const
{
  return m_name.c_str();
}

std::size_t CoLaCommand::getParameterOffset() const
{
  return m_parameterOffset;
}

CoLaError::Enum CoLaCommand::getError() const
{
  return m_error;
}

CoLaCommand CoLaCommand::networkErrorCommand()
{
  return {CoLaCommandType::NETWORK_ERROR, CoLaError::NETWORK_ERROR, ""};
}

} // namespace visionary
