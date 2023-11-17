//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstddef> // for size_t
#include <cstdint>
#include <string>
#include <vector>

#include "PointXYZ.h"

namespace visionary {

// Parameters to be extracted from the XML metadata part
struct CameraParameters
{
  /// The height of the frame in pixels
  int height;
  /// The width of the frame in pixels
  int width;
  /// Camera to world transformation matrix
  double cam2worldMatrix[4 * 4];
  /// Camera Matrix
  double fx, fy, cx, cy;
  /// Camera Distortion Parameters
  double k1, k2, p1, p2, k3;
  /// FocalToRayCross - Correction Offset for depth info
  double f2rc;
};

struct DataSetsActive
{
  bool hasDataSetDepthMap;
  bool hasDataSetPolar2D;
  bool hasDataSetCartesian;
};

struct PointXYZC
{
  float x;
  float y;
  float z;
  float c;
};

class VisionaryData
{
public:
  VisionaryData();
  virtual ~VisionaryData();

  //-----------------------------------------------
  // Getter Functions

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  virtual void generatePointCloud(std::vector<PointXYZ>& pointCloud) = 0;

  // Transform the XYZ point cloud with the Cam2World matrix got from device
  // IN/OUT pointCloud  - Reference to the point cloud to be transformed. Contains the transformed point cloud
  // afterwards.
  void transformPointCloud(std::vector<PointXYZ>& pointCloud) const;

  int getHeight() const;
  int getWidth() const;

  // Returns the Byte length compared to data types
  std::uint32_t getFrameNum() const;

  // Returns the timestamp in device format
  // Bits of the devices timestamp: 5 unused - 12 Year - 4 Month - 5 Day - 11 Timezone - 5 Hour - 6 Minute - 6 Seconds -
  // 10 Milliseconds
  // .....YYYYYYYYYYYYMMMMDDDDDTTTTTTTTTTTHHHHHMMMMMMSSSSSSmmmmmmmmmm
  std::uint64_t getTimestamp() const;
  // Returns the timestamp in milliseconds (UTC)
  std::uint64_t getTimestampMS() const;

  // Returns a reference to the camera parameter struct
  const CameraParameters& getCameraParameters() const;

  //-----------------------------------------------
  // functions for parsing received blob

  // Parse the XML Metadata part to get information about the sensor and the following image data.
  // Returns true when parsing was successful.
  virtual bool parseXML(const std::string& xmlString, std::uint32_t changeCounter) = 0;

  // Parse the Binary data part to extract the image data.
  // Returns true when parsing was successful.
  virtual bool parseBinaryData(std::vector<uint8_t>::iterator inputBuffer, std::size_t length) = 0;

protected:
  // Device specific image types
  enum ImageType
  {
    UNKNOWN,
    PLANAR,
    RADIAL
  };

  // Returns the Byte length compared to data type given as String
  std::size_t getItemLength(const std::string& dataType) const;

  // Pre-calculate lookup table for lens distortion correction,
  // which is needed for point cloud calculation.
  void preCalcCamInfo(const ImageType& type);

  // Calculate and return the Point Cloud in the camera perspective. Units are in meters.
  // IN  map         - Image to be transformed
  // IN  imgType     - Type of the image (needed for correct transformation)
  // OUT pointCloud  - Reference to pass back the point cloud. Will be resized and only contain new point cloud.
  void generatePointCloud(const std::vector<std::uint16_t>& map,
                          const ImageType&                  imgType,
                          std::vector<PointXYZ>&            pointCloud);

  //-----------------------------------------------
  // Camera parameters to be read from XML Metadata part
  CameraParameters m_cameraParams{};

  /// Factor to convert unit of distance image to mm
  float m_scaleZ;

  /// Change counter to detect changes in XML
  std::uint_fast32_t m_changeCounter;

  // Framenumber of the frame
  /// Dataset Version 1: incremented on each received image
  /// Dataset Version 2: framenumber received with dataset
  std::uint_fast32_t m_frameNum;

  // Timestamp in blob format
  // To get timestamp in milliseconds call getTimestampMS()
  std::uint64_t m_blobTimestamp;

  // Camera undistort pre-calculations (look-up-tables) are generated to speed up computations. True if this has been
  // done.
  ImageType m_preCalcCamInfoType;
  // The look-up-tables containing pre-calculations
  std::vector<PointXYZ> m_preCalcCamInfo;

private:
  // Bitmasks to calculate the timestamp in milliseconds
  // Bits of the devices timestamp: 5 unused - 12 Year - 4 Month - 5 Day - 11 Timezone - 5 Hour - 6 Minute - 6 Seconds -
  // 10 Milliseconds
  // .....YYYYYYYYYYYYMMMMDDDDDTTTTTTTTTTTHHHHHMMMMMMSSSSSSmmmmmmmmmm
  static constexpr std::uint64_t BITMASK_YEAR =
    0x7FF800000000000ull; // 0000011111111111100000000000000000000000000000000000000000000000
  static constexpr std::uint64_t BITMASK_MONTH =
    0x780000000000ull; // 0000000000000000011110000000000000000000000000000000000000000000
  static constexpr std::uint64_t BITMASK_DAY =
    0x7C000000000ull; // 0000000000000000000001111100000000000000000000000000000000000000
  static constexpr std::uint64_t BITMASK_HOUR =
    0x7C00000ull; // 0000000000000000000000000000000000000111110000000000000000000000
  static constexpr std::uint64_t BITMASK_MINUTE =
    0x3F0000ull; // 0000000000000000000000000000000000000000001111110000000000000000
  static constexpr std::uint64_t BITMASK_SECOND =
    0xFC00ull; // 0000000000000000000000000000000000000000000000001111110000000000
  static constexpr std::uint64_t BITMASK_MILLISECOND =
    0x3FFull; // 0000000000000000000000000000000000000000000000000000001111111111
};

} // namespace visionary
