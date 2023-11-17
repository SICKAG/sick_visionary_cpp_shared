//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include <cmath>
#include <cstdio>

#include "VisionaryEndian.h"
#include "VisionarySData.h"

#include <iostream>
// Boost library used for parseXML function
#if defined(__GNUC__)         // GCC compiler
#  pragma GCC diagnostic push // Save warning levels for later restoration
#  pragma GCC diagnostic ignored "-Wpragmas"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wdeprecated-copy"
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wparentheses"
#  pragma GCC diagnostic ignored "-Wcast-align"
#  pragma GCC diagnostic ignored "-Wstrict-overflow"
#  pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>

#if defined(__GNUC__)        // GCC compiler
#  pragma GCC diagnostic pop // Restore previous warning levels
#endif

namespace visionary {

VisionarySData::VisionarySData() : VisionaryData(), m_zByteDepth(0), m_rgbaByteDepth(0), m_confidenceByteDepth(0)
{
}

VisionarySData::~VisionarySData() = default;

bool VisionarySData::parseXML(const std::string& xmlString, uint32_t changeCounter)
{
  //-----------------------------------------------
  // Check if the segment data changed since last receive
  if (m_changeCounter == changeCounter)
  {
    return true; // Same XML content as on last received blob
  }
  m_changeCounter      = changeCounter;
  m_preCalcCamInfoType = VisionaryData::UNKNOWN;

  //-----------------------------------------------
  // Build boost::property_tree for easy XML handling
  boost::property_tree::ptree xmlTree;
  std::istringstream          ss(xmlString);
  try
  {
    boost::property_tree::xml_parser::read_xml(ss, xmlTree);
  }
  catch (...)
  {
    std::cout << "Reading XML tree in BLOB failed." << std::endl;
    return false;
  }

  boost::property_tree::ptree dataStreamTree =
    xmlTree.get_child("SickRecord.DataSets.DataSetStereo.FormatDescriptionDepthMap.DataStream");

  //-----------------------------------------------
  // Extract information stored in XML with boost::property_tree
  m_cameraParams.width  = dataStreamTree.get<int>("Width", 0);
  m_cameraParams.height = dataStreamTree.get<int>("Height", 0);

  int i = 0;

  BOOST_FOREACH (const boost::property_tree::ptree::value_type& item,
                 dataStreamTree.get_child("CameraToWorldTransform"))
  {
    m_cameraParams.cam2worldMatrix[i] = item.second.get_value<double>(0.);
    ++i;
  }

  m_cameraParams.fx = dataStreamTree.get<double>("CameraMatrix.FX", 0.0);
  m_cameraParams.fy = dataStreamTree.get<double>("CameraMatrix.FY", 0.0);
  m_cameraParams.cx = dataStreamTree.get<double>("CameraMatrix.CX", 0.0);
  m_cameraParams.cy = dataStreamTree.get<double>("CameraMatrix.CY", 0.0);

  m_cameraParams.k1 = dataStreamTree.get<double>("CameraDistortionParams.K1", 0.0);
  m_cameraParams.k2 = dataStreamTree.get<double>("CameraDistortionParams.K2", 0.0);
  m_cameraParams.p1 = dataStreamTree.get<double>("CameraDistortionParams.P1", 0.0);
  m_cameraParams.p2 = dataStreamTree.get<double>("CameraDistortionParams.P2", 0.0);
  m_cameraParams.k3 = dataStreamTree.get<double>("CameraDistortionParams.K3", 0.0);

  m_cameraParams.f2rc = dataStreamTree.get<double>("FocalToRayCross", 0.0);

  m_zByteDepth          = getItemLength(dataStreamTree.get<std::string>("Z", ""));
  m_rgbaByteDepth       = getItemLength(dataStreamTree.get<std::string>("Intensity", ""));
  m_confidenceByteDepth = getItemLength(dataStreamTree.get<std::string>("Confidence", ""));

  const auto distanceDecimalExponent = dataStreamTree.get<int>("Z.<xmlattr>.decimalexponent", 0);
  m_scaleZ                           = powf(10.0f, static_cast<float>(distanceDecimalExponent));

  return true;
}

bool VisionarySData::parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  if (m_cameraParams.height < 1 || m_cameraParams.width < 1)
  {
    std::cout << __FUNCTION__ << ": Invalid Image size" << std::endl;
    return false;
  }
  auto         remainingSize      = size;
  const size_t numPixel           = static_cast<size_t>(m_cameraParams.width * m_cameraParams.height);
  const size_t numBytesZ          = numPixel * static_cast<size_t>(m_zByteDepth);
  const size_t numBytesRGBA       = numPixel * static_cast<size_t>(m_rgbaByteDepth);
  const size_t numBytesConfidence = numPixel * static_cast<size_t>(m_confidenceByteDepth);
  const size_t headerSize         = 4u + 8u + 2u; // Length(32bit) + TimeStamp(64bit) + version(16bit)

  if (remainingSize < headerSize)
  {
    std::cout << "Malformed data. Did not receive enough data to parse header of binary segment" << std::endl;
    return false;
  }
  remainingSize -= headerSize;

  //-----------------------------------------------
  // The binary part starts with entries for length, a timestamp
  // and a version identifier
  const auto length = readUnalignLittleEndian<uint32_t>(&*itBuf);
  if (length > size)
  {
    std::cout << "Malformed data, length in depth map header does not match package size." << std::endl;
    return false;
  }

  itBuf += sizeof(uint32_t);

  m_blobTimestamp = readUnalignLittleEndian<uint64_t>(&*itBuf);
  itBuf += sizeof(uint64_t);

  const auto version = readUnalignLittleEndian<uint16_t>(&*itBuf);
  itBuf += sizeof(uint16_t);

  //-----------------------------------------------
  // The content of the Data part inside a data set has changed since the first released version.
  if (version > 1)
  {
    const size_t extendedHeaderSize = 4u + 1u + 1u; // Framenumber(32bit) + dataQuality(8bit) + deviceStatus(8bit)
    if (remainingSize < extendedHeaderSize)
    {
      std::cout << "Malformed data. Did not receive enough data to parse extended header of binary segment"
                << std::endl;
      return false;
    }
    remainingSize -= extendedHeaderSize;
    // more frame information follows in this case: frame number, data quality, device status
    m_frameNum = readUnalignLittleEndian<uint32_t>(&*itBuf);
    itBuf += sizeof(uint32_t);

    // const uint8_t dataQuality = readUnalignLittleEndian<uint8_t>(&*itBuf);
    itBuf += sizeof(uint8_t);

    // const uint8_t deviceStatus = readUnalignLittleEndian<uint8_t>(&*itBuf);
    itBuf += sizeof(uint8_t);
  }
  else
  {
    ++m_frameNum;
  }

  //-----------------------------------------------
  // Extract the Images depending on the informations extracted from the XML part
  const auto imageSetSize = (numBytesZ + numBytesRGBA + numBytesConfidence);
  if (remainingSize < imageSetSize)
  {
    std::cout << "Malformed data. Did not receive enough data to parse images of binary segment" << std::endl;
    return false;
  }
  remainingSize -= imageSetSize;
  m_zMap.resize(numPixel);
  memcpy((m_zMap).data(), &*itBuf, numBytesZ);
  std::advance(itBuf, numBytesZ);

  m_rgbaMap.resize(numPixel);
  memcpy((m_rgbaMap).data(), &*itBuf, numBytesRGBA);
  std::advance(itBuf, numBytesRGBA);

  m_confidenceMap.resize(numPixel);
  memcpy((m_confidenceMap).data(), &*itBuf, numBytesConfidence);
  std::advance(itBuf, numBytesConfidence);

  const auto footerSize = (4u + 4u); // CRC(32bit) + LengthCopy(32bit)
  if (remainingSize < footerSize)
  {
    std::cout << "Malformed data. Did not receive enough data to parse images of binary segment" << std::endl;
    return false;
  }

  //-----------------------------------------------
  // Data ends with a (unused) 4 Byte CRC field and a copy of the length byte
  // const uint32_t unusedCrc = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  const auto lengthCopy = readUnalignLittleEndian<uint32_t>(&*itBuf);
  itBuf += sizeof(uint32_t);

  if (length != lengthCopy)
  {
    std::cout << "Malformed data, length in header does not match package size." << std::endl;
    return false;
  }

  return true;
}

void VisionarySData::generatePointCloud(std::vector<PointXYZ>& pointCloud)
{
  return VisionaryData::generatePointCloud(m_zMap, VisionaryData::PLANAR, pointCloud);
}

const std::vector<uint16_t>& VisionarySData::getZMap() const
{
  return m_zMap;
}

const std::vector<uint32_t>& VisionarySData::getRGBAMap() const
{
  return m_rgbaMap;
}

const std::vector<uint16_t>& VisionarySData::getConfidenceMap() const
{
  return m_confidenceMap;
}

} // namespace visionary
