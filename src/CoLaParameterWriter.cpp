//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "CoLaParameterWriter.h"

#include "MD5.h"
#include "VisionaryEndian.h"

#include <algorithm> // for min
#include <cstring>   // for strlen
#include <limits>    // for int max

namespace visionary {

CoLaParameterWriter::CoLaParameterWriter(CoLaCommandType::Enum type, const char* name) : m_type(type), m_name(name)
{
  writeHeader(m_type, m_name);
}

CoLaParameterWriter::~CoLaParameterWriter() = default;

CoLaParameterWriter& CoLaParameterWriter::parameterSInt(const int8_t sInt)
{
  m_buffer.push_back(static_cast<uint8_t>(sInt));
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterUSInt(const uint8_t uSInt)
{
  m_buffer.push_back(uSInt);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterInt(const int16_t integer)
{
  const int16_t bigEndianValue = nativeToBigEndian(integer);
  m_buffer.insert(m_buffer.end(),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue) + 2);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterUInt(const uint16_t uInt)
{
  const uint16_t bigEndianValue = nativeToBigEndian(uInt);
  m_buffer.insert(m_buffer.end(),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue) + 2);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterDInt(const int32_t dInt)
{
  const int32_t bigEndianValue = nativeToBigEndian(dInt);
  m_buffer.insert(m_buffer.end(),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue) + 4);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterUDInt(const uint32_t uDInt)
{
  const uint32_t bigEndianValue = nativeToBigEndian(uDInt);
  m_buffer.insert(m_buffer.end(),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue) + 4);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterReal(const float real)
{
  const float bigEndianValue = nativeToBigEndian(real);
  m_buffer.insert(m_buffer.end(),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue) + 4);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterLReal(const double lReal)
{
  const double bigEndianValue = nativeToBigEndian(lReal);
  m_buffer.insert(m_buffer.end(),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue),
                  reinterpret_cast<const uint8_t*>(&bigEndianValue) + 8);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterBool(const bool boolean)
{
  *this << static_cast<uint8_t>(boolean);
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterPasswordMD5(const std::string& str)
{
  uint32_t valueUDInt = 0;

  const unsigned char* byteData = MD5(str).getDigest();

  // 128 bit to 32 bit using XOR
  int byte0  = byteData[0] ^ byteData[4] ^ byteData[8] ^ byteData[12];
  int byte1  = byteData[1] ^ byteData[5] ^ byteData[9] ^ byteData[13];
  int byte2  = byteData[2] ^ byteData[6] ^ byteData[10] ^ byteData[14];
  int byte3  = byteData[3] ^ byteData[7] ^ byteData[11] ^ byteData[15];
  valueUDInt = static_cast<uint32_t>(byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24));

  // Add as UDInt, it is already big endian
  parameterUDInt(valueUDInt);

  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::parameterFlexString(const std::string& str)
{
  // Add length of string
  const size_t slen     = str.length();
  const size_t max_slen = std::numeric_limits<uint16_t>::max();
  const size_t efflen   = std::min(slen, max_slen);
  parameterUInt(static_cast<uint16_t>(efflen));

  // Add string
  m_buffer.insert(
    m_buffer.end(), str.begin(), str.begin() + static_cast<std::string::iterator::difference_type>(efflen));

  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const char* str)
{
  m_buffer.insert(m_buffer.end(), str, str + std::strlen(str));
  return *this;
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const int8_t sInt)
{
  return parameterSInt(sInt);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const uint8_t uSInt)
{
  return parameterUSInt(uSInt);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const int16_t integer)
{
  return parameterInt(integer);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const uint16_t uInt)
{
  return parameterUInt(uInt);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const int32_t dInt)
{
  return parameterDInt(dInt);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const uint32_t uDInt)
{
  return parameterUDInt(uDInt);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const float real)
{
  return parameterReal(real);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const double lReal)
{
  return parameterLReal(lReal);
}

CoLaParameterWriter& CoLaParameterWriter::operator<<(const bool boolean)
{
  return parameterBool(boolean);
}

const CoLaCommand CoLaParameterWriter::build()
{
  // Copy buffer
  std::vector<uint8_t> buffer = m_buffer;

  return CoLaCommand(buffer);
}

void CoLaParameterWriter::writeHeader(CoLaCommandType::Enum type, const char* name)
{
  // Write command type
  switch (type)
  {
    case CoLaCommandType::READ_VARIABLE:
      *this << "sRN ";
      break;
    case CoLaCommandType::READ_VARIABLE_RESPONSE:
      *this << "sRA ";
      break;
    case CoLaCommandType::WRITE_VARIABLE:
      *this << "sWN ";
      break;
    case CoLaCommandType::WRITE_VARIABLE_RESPONSE:
      *this << "sWA ";
      break;
    case CoLaCommandType::METHOD_INVOCATION:
      *this << "sMN ";
      break;
    case CoLaCommandType::METHOD_RETURN_VALUE:
      *this << "sAN ";
      break;
    case CoLaCommandType::COLA_ERROR:
      *this << "sFA";
      break;
    default:
      return;
  }

  // Write command name
  *this << name << " ";
}

} // namespace visionary
