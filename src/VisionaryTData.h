//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include "VisionaryData.h"

#include <cstddef> // for size_t
#include <cstdint>
#include <string>
#include <vector>

namespace visionary {

typedef struct
{
  float angleFirstScanPoint;
  float angularResolution;
  float polarScale;
  float polarOffset;
} PolarParameters;

class VisionaryTData : public VisionaryData
{
public:
  VisionaryTData();
  ~VisionaryTData() override;

  //-----------------------------------------------
  // Getter Functions
  const std::vector<std::uint16_t>& getDistanceMap() const;
  const std::vector<std::uint16_t>& getIntensityMap() const;
  const std::vector<std::uint16_t>& getConfidenceMap() const;
  // Returns Number of points get by the polar reduction.
  // 0 when no data is available.
  std::uint8_t              getPolarSize() const;
  float                     getPolarStartAngle() const;
  float                     getPolarAngularResolution() const;
  const std::vector<float>& getPolarDistanceData() const;
  const std::vector<float>& getPolarConfidenceData() const;
  // Returns Number of points get by the cartesian reduction.
  // 0 when no data is available.
  std::uint32_t                 getCartesianSize() const;
  const std::vector<PointXYZC>& getCartesianData() const;

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  void generatePointCloud(std::vector<PointXYZ>& pointCloud) override;

protected:
  using ByteBuffer = std::vector<std::uint8_t>;

  //-----------------------------------------------
  // functions for parsing received blob

  // Parse the XML Metadata part to get information about the sensor and the following image data.
  // This function uses boost library. An other XML parser is needed to remove boost from source.
  // Returns true when parsing was successful.
  bool parseXML(const std::string& xmlString, std::uint32_t changeCounter) override;

  // Parse the Binary data part to extract the image data.
  // some variables are commented out, because they are not used in this sample.
  // Returns true when parsing was successful.
  bool parseBinaryData(ByteBuffer::iterator itBuf, std::size_t size) override;

private:
  // Indicator for the received data sets
  DataSetsActive m_dataSetsActive;

  // Byte depth of images
  std::size_t m_distanceByteDepth, m_intensityByteDepth, m_confidenceByteDepth;

  // Angle information of polar scan
  float m_angleFirstScanPoint;
  float m_angularResolution;

  // Number of values for polar data reduction
  std::uint_fast8_t m_numPolarValues;
  // Number of values for cartesian data reduction
  std::uint_fast32_t m_numCartesianValues;

  // Pointers to the image data
  std::vector<std::uint16_t> m_distanceMap;
  std::vector<std::uint16_t> m_intensityMap;
  std::vector<std::uint16_t> m_confidenceMap;
  std::vector<float>         m_polarDistanceData;
  std::vector<float>         m_polarConfidenceData;
  std::vector<PointXYZC>     m_cartesianData;
};

} // namespace visionary
