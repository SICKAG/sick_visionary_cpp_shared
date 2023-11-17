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

class VisionaryTMiniData : public VisionaryData
{
public:
  VisionaryTMiniData();
  ~VisionaryTMiniData() override;

  //-----------------------------------------------
  // Getter Functions

  // Gets the radial distance map
  // The unit of the distancemap is 1/4 mm
  const std::vector<std::uint16_t>& getDistanceMap() const;

  // Gets the intensity map
  const std::vector<std::uint16_t>& getIntensityMap() const;

  // Gets the state map
  const std::vector<std::uint16_t>& getStateMap() const;

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  void generatePointCloud(std::vector<PointXYZ>& pointCloud) override;

  // factor to convert Radial distance map from fixed point to floating point
  static const float DISTANCE_MAP_UNIT;

protected:
  //-----------------------------------------------
  // functions for parsing received blob

  // Parse the XML Metadata part to get information about the sensor and the following image data.
  // This function uses boost library. An other XML parser is needed to remove boost from source.
  // Returns true when parsing was successful.
  bool parseXML(const std::string& xmlString, std::uint32_t changeCounter) override;

  // Parse the Binary data part to extract the image data.
  // some variables are commented out, because they are not used in this sample.
  // Returns true when parsing was successful.
  bool parseBinaryData(std::vector<uint8_t>::iterator itBuf, std::size_t size) override;

private:
  // Indicator for the received data sets
  DataSetsActive m_dataSetsActive;

  // Byte depth of images
  std::size_t m_distanceByteDepth, m_intensityByteDepth, m_stateByteDepth;

  // Pointers to the image data
  std::vector<std::uint16_t> m_distanceMap;
  std::vector<std::uint16_t> m_intensityMap;
  std::vector<std::uint16_t> m_stateMap;
};

} // namespace visionary
