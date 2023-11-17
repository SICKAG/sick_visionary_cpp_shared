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

class VisionarySData : public VisionaryData
{
public:
  VisionarySData();
  ~VisionarySData() override;

  //-----------------------------------------------
  // Getter Functions
  // Gets the Z distance map
  const std::vector<std::uint16_t>& getZMap() const;

  // Gets the intensity map
  const std::vector<std::uint32_t>& getRGBAMap() const;

  // Gets the state map
  const std::vector<std::uint16_t>& getConfidenceMap() const;

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  void generatePointCloud(std::vector<PointXYZ>& pointCloud) override;

protected:
  //-----------------------------------------------
  // functions for parsing received blob

  // Parse the XML Metadata part to get information about the sensor and the following image data.
  // This function uses boost library. An other XML parser is needed to remove boost from source.
  // Returns true when parsing was successful.
  bool parseXML(const std::string& xmlString, std::uint32_t changeCounter) override;

  // Parse the Binary data part to extract the image data.
  // Returns true when parsing was successful.
  bool parseBinaryData(std::vector<uint8_t>::iterator itBuf, std::size_t size) override;

private:
  /// Byte depth of images
  std::size_t m_zByteDepth, m_rgbaByteDepth, m_confidenceByteDepth;

  // Pointers to the image data
  std::vector<std::uint16_t> m_zMap;
  std::vector<std::uint32_t> m_rgbaMap;
  std::vector<std::uint16_t> m_confidenceMap;
};

} // namespace visionary
