//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include <cstdio>

#include "VisionaryEndian.h"
#include "VisionaryTMiniData.h"

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

static const boost::property_tree::ptree& empty_ptree()
{
  static boost::property_tree::ptree t;
  return t;
}

const float VisionaryTMiniData::DISTANCE_MAP_UNIT = 0.25f;

VisionaryTMiniData::VisionaryTMiniData()
  : VisionaryData(), m_dataSetsActive(), m_distanceByteDepth(0), m_intensityByteDepth(0), m_stateByteDepth(0)
{
}

VisionaryTMiniData::~VisionaryTMiniData() = default;

bool VisionaryTMiniData::parseXML(const std::string& xmlString, uint32_t changeCounter)
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

  //-----------------------------------------------
  // Extract information stored in XML with boost::property_tree
  const boost::property_tree::ptree dataSetsTree = xmlTree.get_child("SickRecord.DataSets", empty_ptree());
  m_dataSetsActive.hasDataSetDepthMap = static_cast<bool>(dataSetsTree.get_child_optional("DataSetDepthMap"));

  // DataSetDepthMap specific data
  {
    boost::property_tree::ptree dataStreamTree =
      dataSetsTree.get_child("DataSetDepthMap.FormatDescriptionDepthMap.DataStream", empty_ptree());

    m_cameraParams.width  = dataStreamTree.get<int>("Width", 0);
    m_cameraParams.height = dataStreamTree.get<int>("Height", 0);

    if (m_dataSetsActive.hasDataSetDepthMap)
    {
      int i = 0;
      BOOST_FOREACH (const boost::property_tree::ptree::value_type& item,
                     dataStreamTree.get_child("CameraToWorldTransform"))
      {
        m_cameraParams.cam2worldMatrix[i] = item.second.get_value<double>(0.);
        ++i;
      }
    }
    else
    {
      std::fill_n(m_cameraParams.cam2worldMatrix, 16, 0.0);
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

    m_distanceByteDepth  = getItemLength(dataStreamTree.get<std::string>("Distance", ""));
    m_intensityByteDepth = getItemLength(dataStreamTree.get<std::string>("Intensity", ""));
    m_stateByteDepth     = getItemLength(dataStreamTree.get<std::string>("Confidence", ""));

    // const auto distanceDecimalExponent = dataStreamTree.get<int>("Distance.<xmlattr>.decimalexponent", 0);
    //  Scaling is fixed to 0.25mm on ToF Mini
    m_scaleZ = DISTANCE_MAP_UNIT;
  }

  return true;
}

bool VisionaryTMiniData::parseBinaryData(std::vector<uint8_t>::iterator itBuf, size_t size)
{
  if (m_cameraParams.height < 1 || m_cameraParams.width < 1)
  {
    std::cout << __FUNCTION__ << ": Invalid image size" << std::endl;
    return false;
  }
  size_t dataSetslength = 0;
  auto   remainingSize  = size;

  if (m_dataSetsActive.hasDataSetDepthMap)
  {
    const size_t numPixel          = static_cast<size_t>(m_cameraParams.width * m_cameraParams.height);
    const size_t numBytesDistance  = numPixel * static_cast<size_t>(m_distanceByteDepth);
    const size_t numBytesIntensity = numPixel * static_cast<size_t>(m_intensityByteDepth);
    const size_t numBytesState     = numPixel * static_cast<size_t>(m_stateByteDepth);
    const size_t headerSize        = 4u + 8u + 2u; // Length(32bit) + TimeStamp(64bit) + version(16bit)

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
    dataSetslength += length;
    if (dataSetslength > size)
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
    const auto imageSetSize = (numBytesDistance + numBytesIntensity + numBytesState);
    if (remainingSize < imageSetSize)
    {
      std::cout << "Malformed data. Did not receive enough data to parse images of binary segment" << std::endl;
      return false;
    }
    remainingSize -= imageSetSize;
    if (numBytesDistance != 0)
    {
      m_distanceMap.resize(numPixel);
      memcpy((m_distanceMap).data(), &*itBuf, numBytesDistance);
      std::advance(itBuf, numBytesDistance);
    }
    else
    {
      m_distanceMap.clear();
    }
    if (numBytesIntensity != 0)
    {
      m_intensityMap.resize(numPixel);
      memcpy((m_intensityMap).data(), &*itBuf, numBytesIntensity);
      std::advance(itBuf, numBytesIntensity);
    }
    else
    {
      m_intensityMap.clear();
    }
    if (numBytesState != 0)
    {
      m_stateMap.resize(numPixel);
      memcpy((m_stateMap).data(), &*itBuf, numBytesState);
      std::advance(itBuf, numBytesState);
    }
    else
    {
      m_stateMap.clear();
    }

    const auto footerSize = (4u + 4u); // CRC(32bit) + LengthCopy(32bit)
    if (remainingSize < footerSize)
    {
      std::cout << "Malformed data. Did not receive enough data to parse footer of binary segment" << std::endl;
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
      std::cout << "Malformed data, length in header(" << length << ") does not match package size(" << lengthCopy
                << ")." << std::endl;
      return false;
    }
  }
  else
  {
    m_distanceMap.clear();
    m_intensityMap.clear();
    m_stateMap.clear();
  }

  return true;
}

void VisionaryTMiniData::generatePointCloud(std::vector<PointXYZ>& pointCloud)
{
  return VisionaryData::generatePointCloud(m_distanceMap, VisionaryData::RADIAL, pointCloud);
}

const std::vector<uint16_t>& VisionaryTMiniData::getDistanceMap() const
{
  return m_distanceMap;
}

const std::vector<uint16_t>& VisionaryTMiniData::getIntensityMap() const
{
  return m_intensityMap;
}

const std::vector<uint16_t>& VisionaryTMiniData::getStateMap() const
{
  return m_stateMap;
}

} // namespace visionary
