//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "CoLaParameterReader.h"
#include "VisionaryEndian.h"
#include <stdexcept>

namespace visionary {

CoLaParameterReader::CoLaParameterReader(CoLaCommand command) : m_command(command)
{
  m_currentPosition = command.getParameterOffset();
}

CoLaParameterReader::~CoLaParameterReader() = default;

void CoLaParameterReader::checkSize(size_t pos, size_t size)
{
  if ((pos + size) > m_command.getBuffer().size())
  {
    throw std::out_of_range("");
  }
}

void CoLaParameterReader::rewind()
{
  m_currentPosition = m_command.getParameterOffset();
}

int8_t CoLaParameterReader::readSInt()
{
  const auto value = static_cast<int8_t>(m_command.getBuffer().at(m_currentPosition));
  m_currentPosition += 1;
  return value;
}

uint8_t CoLaParameterReader::readUSInt()
{
  const uint8_t value = m_command.getBuffer().at(m_currentPosition);
  m_currentPosition += 1;
  return value;
}

int16_t CoLaParameterReader::readInt()
{
  checkSize(m_currentPosition, 2u);
  const auto value = readUnalignBigEndian<int16_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 2;
  return value;
}

uint16_t CoLaParameterReader::readUInt()
{
  checkSize(m_currentPosition, 2u);
  const auto value = readUnalignBigEndian<uint16_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 2;
  return value;
}

int32_t CoLaParameterReader::readDInt()
{
  checkSize(m_currentPosition, 4u);
  const auto value = readUnalignBigEndian<int32_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 4;
  return value;
}

uint32_t CoLaParameterReader::readUDInt()
{
  checkSize(m_currentPosition, 4u);
  const auto value = readUnalignBigEndian<uint32_t>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 4;
  return value;
}

float CoLaParameterReader::readReal()
{
  checkSize(m_currentPosition, 4u);
  const auto value = readUnalignBigEndian<float>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 4;
  return value;
}

double CoLaParameterReader::readLReal()
{
  checkSize(m_currentPosition, 8u);
  const auto value = readUnalignBigEndian<double>(&m_command.getBuffer()[m_currentPosition]);
  m_currentPosition += 8;
  return value;
}

bool CoLaParameterReader::readBool()
{
  return readUSInt() == 1;
}

std::string CoLaParameterReader::readFlexString()
{
  std::string str;
  uint16_t    len = readUInt();
  return readFixedString(len);
}

std::string CoLaParameterReader::readFixedString(uint16_t len)
{
  std::string str;
  if (len)
  {
    checkSize(m_currentPosition, len);
    str = std::string(reinterpret_cast<const char*>(&m_command.getBuffer()[m_currentPosition]), len);
  }
  m_currentPosition += len;
  return str;
}

} // namespace visionary
