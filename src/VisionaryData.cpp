//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "VisionaryData.h"

#include <algorithm>
#include <cassert>
#include <cctype> // for tolower
#include <chrono>
#include <cmath>
#include <cstddef> // for size_t
#include <ctime>
#include <iostream>
#include <limits>
#include <sstream>

namespace visionary {

const float bad_point = std::numeric_limits<float>::quiet_NaN();

VisionaryData::VisionaryData()
  : m_scaleZ(0.0f)
  , m_changeCounter(0u)
  , m_frameNum(0u)
  , m_blobTimestamp(0u)
  , m_preCalcCamInfoType(VisionaryData::UNKNOWN)
{
  m_cameraParams.width  = 0;
  m_cameraParams.height = 0;
}

VisionaryData::~VisionaryData() = default;

std::size_t VisionaryData::getItemLength(const std::string& dataType) const
{
  std::string lcstr(dataType);
  // Transform String to lower case String
  std::transform(
    lcstr.begin(), lcstr.end(), lcstr.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if (lcstr == "uint8")
  {
    return sizeof(std::uint8_t);
  }
  else if (lcstr == "uint16")
  {
    return sizeof(std::uint16_t);
  }
  else if (lcstr == "uint32")
  {
    return sizeof(std::uint32_t);
  }
  else if (lcstr == "uint64")
  {
    return sizeof(std::uint64_t);
  }
  return 0;
}

void VisionaryData::preCalcCamInfo(const ImageType& imgType)
{
  assert(imgType != UNKNOWN); // Unknown image type for the point cloud transformation
  if (m_cameraParams.height < 1 || m_cameraParams.width < 1)
  {
    std::cout << __FUNCTION__ << ": Invalid Image size" << std::endl;
  }
  assert(m_cameraParams.height > 0);
  assert(m_cameraParams.width > 0);

  m_preCalcCamInfo.clear();
  m_preCalcCamInfo.reserve(static_cast<size_t>(m_cameraParams.height * m_cameraParams.width));

  //-----------------------------------------------
  // transform each pixel into Cartesian coordinates
  for (int row = 0; row < m_cameraParams.height; row++)
  {
    double yp  = (m_cameraParams.cy - row) / m_cameraParams.fy;
    double yp2 = yp * yp;

    for (int col = 0; col < m_cameraParams.width; col++)
    {
      // we map from image coordinates with origin top left and x
      // horizontal (right) and y vertical
      // (downwards) to camera coordinates with origin in center and x
      // to the left and y upwards (seen
      // from the sensor position)
      const double xp = (m_cameraParams.cx - col) / m_cameraParams.fx;

      // correct the camera distortion
      const double r2 = xp * xp + yp2;
      const double r4 = r2 * r2;
      const double k  = 1 + m_cameraParams.k1 * r2 + m_cameraParams.k2 * r4;

      // Undistorted direction vector of the point
      const auto  x  = static_cast<float>(xp * k);
      const auto  y  = static_cast<float>(yp * k);
      const float z  = 1.0f;
      double      s0 = 0;
      if (RADIAL == imgType)
      {
        s0 = std::sqrt(x * x + y * y + z * z) * 1000;
      }
      else if (PLANAR == imgType)
      {
        s0 = 1000;
      }
      else
      {
        std::cout << "Unknown image type for the point cloud transformation" << std::endl;
        assert(false);
      }
      PointXYZ point{};
      point.x = static_cast<float>(x / s0);
      point.y = static_cast<float>(y / s0);
      point.z = static_cast<float>(z / s0);

      m_preCalcCamInfo.push_back(point);
    }
  }
  m_preCalcCamInfoType = imgType;
}

void VisionaryData::generatePointCloud(const std::vector<uint16_t>& map,
                                       const ImageType&             imgType,
                                       std::vector<PointXYZ>&       pointCloud)
{
  // Calculate disortion data from XML metadata once.
  if (m_preCalcCamInfoType != imgType)
  {
    preCalcCamInfo(imgType);
  }
  size_t cloudSize = map.size();
  pointCloud.resize(cloudSize);

  const auto f2rc = static_cast<float>(m_cameraParams.f2rc / 1000.f); // PointCloud should be in [m] and not in [mm]

  const float pixelSizeZ = m_scaleZ;

  //-----------------------------------------------
  // transform each pixel into Cartesian coordinates
  auto itMap         = map.begin();
  auto itUndistorted = m_preCalcCamInfo.begin();
  auto itPC          = pointCloud.begin();
  for (uint32_t i = 0; i < cloudSize; ++i, ++itPC, ++itMap, ++itUndistorted)
  // for (std::vector<PointXYZ>::iterator itPC = pointCloud.begin(), itEnd = pointCloud.end(); itPC != itEnd; ++itPC,
  // ++itMap, ++itUndistorted)
  {
    PointXYZ point{};
    // If point is valid put it to point cloud
    if (*itMap == 0 || *itMap == uint16_t(0xFFFF))
    {
      point.x = bad_point;
      point.y = bad_point;
      point.z = bad_point;
    }
    else
    {
      // calculate coordinates & store in point cloud vector
      float distance = static_cast<float>((*itMap)) * pixelSizeZ;
      point.x        = itUndistorted->x * distance;
      point.y        = itUndistorted->y * distance;
      point.z        = itUndistorted->z * distance - f2rc;
    }
    *itPC = point;
  }
  return;
}

void VisionaryData::transformPointCloud(std::vector<PointXYZ>& pointCloud) const
{
  // turn cam 2 world translations from [m] to [mm]
  const double tx = m_cameraParams.cam2worldMatrix[3] / 1000.;
  const double ty = m_cameraParams.cam2worldMatrix[7] / 1000.;
  const double tz = m_cameraParams.cam2worldMatrix[11] / 1000.;

  for (auto& it : pointCloud)
  {
    const double x = it.x;
    const double y = it.y;
    const double z = it.z;

    it.x = static_cast<float>(x * m_cameraParams.cam2worldMatrix[0] + y * m_cameraParams.cam2worldMatrix[1]
                              + z * m_cameraParams.cam2worldMatrix[2] + tx);
    it.y = static_cast<float>(x * m_cameraParams.cam2worldMatrix[4] + y * m_cameraParams.cam2worldMatrix[5]
                              + z * m_cameraParams.cam2worldMatrix[6] + ty);
    it.z = static_cast<float>(x * m_cameraParams.cam2worldMatrix[8] + y * m_cameraParams.cam2worldMatrix[9]
                              + z * m_cameraParams.cam2worldMatrix[10] + tz);
  }
}

int VisionaryData::getHeight() const
{
  return m_cameraParams.height;
}

int VisionaryData::getWidth() const
{
  return m_cameraParams.width;
}

uint32_t VisionaryData::getFrameNum() const
{
  return m_frameNum;
}

uint64_t VisionaryData::getTimestamp() const
{
  return m_blobTimestamp;
}

uint64_t VisionaryData::getTimestampMS() const
{
  std::tm tm{};
  tm.tm_sec   = static_cast<int>((m_blobTimestamp & BITMASK_SECOND) >> 10);
  tm.tm_min   = static_cast<int>((m_blobTimestamp & BITMASK_MINUTE) >> 16);
  tm.tm_hour  = static_cast<int>((m_blobTimestamp & BITMASK_HOUR) >> 22);
  tm.tm_mday  = static_cast<int>((m_blobTimestamp & BITMASK_DAY) >> 38);
  tm.tm_mon   = static_cast<int>(((m_blobTimestamp & BITMASK_MONTH) >> 43) - 1u);
  tm.tm_year  = static_cast<int>(((m_blobTimestamp & BITMASK_YEAR) >> 47u) - 1900);
  tm.tm_isdst = -1; // Use DST value from local time zone
#ifdef _WIN32
  auto seconds{std::chrono::seconds{::_mkgmtime(&tm)}};
#else
  auto seconds{std::chrono::seconds{::timegm(&tm)}};
#endif
  const uint64_t timestamp =
    static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(seconds).count())
    + (m_blobTimestamp & BITMASK_MILLISECOND);

  return timestamp;
}

const CameraParameters& VisionaryData::getCameraParameters() const
{
  return m_cameraParams;
}

} // namespace visionary
