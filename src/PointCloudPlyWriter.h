//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <cstdint>
#include <vector>

#include "PointXYZ.h"

namespace visionary {

enum InvalidPointPresentation
{
  /// Not a Number = nan
  INVALID_AS_NAN = 0,

  /// Zero = 0.0
  INVALID_AS_ZERO = 1,

  /// Skip => don't print the number
  INVALID_SKIP = 2
};

/// <summary>Class for writing point clouds to PLY files.</summary>
class PointCloudPlyWriter
{
public:
  PointCloudPlyWriter(const PointCloudPlyWriter&)                  = delete;
  const PointCloudPlyWriter& operator=(const PointCloudPlyWriter&) = delete;
  /// <summary>Save a point cloud to a file in Polygon File Format (PLY), see:
  /// https://en.wikipedia.org/wiki/PLY_%28file_format%29 </summary> <param name="filename">The file to save the point
  /// cloud to</param> <param name="points">The points to save</param> <param name="useBinary">If the output file is
  /// binary or ascii</param> <param name="presentation">Definition how invalid points should be presented inside the
  /// PLY file [optional]</param> <returns>Returns true if write was successful and false otherwise</returns>
  static bool WriteFormatPLY(const char*                  filename,
                             const std::vector<PointXYZ>& points,
                             bool                         useBinary,
                             InvalidPointPresentation     presentation = INVALID_AS_NAN);

  /// <summary>Save a point cloud to a file in Polygon File Format (PLY) which has colors for each point, see:
  /// https://en.wikipedia.org/wiki/PLY_%28file_format%29 </summary> <param name="filename">The file to save the point
  /// cloud to</param> <param name="points">The points to save</param> <param name="rgbaMap">RGBA colors for each point,
  /// must be same length as points</param> <param name="useBinary">If the output file is binary or ascii</param> <param
  /// name="presentation">Definition how invalid points should be presented inside the PLY file [optional]</param>
  /// <returns>Returns true if write was successful and false otherwise</returns>
  static bool WriteFormatPLY(const char*                  filename,
                             const std::vector<PointXYZ>& points,
                             const std::vector<uint32_t>& rgbaMap,
                             bool                         useBinary,
                             InvalidPointPresentation     presentation = INVALID_AS_NAN);

  /// <summary>Save a point cloud to a file in Polygon File Format (PLY) which has intensities for each point, see:
  /// https://en.wikipedia.org/wiki/PLY_%28file_format%29 </summary> <param name="filename">The file to save the point
  /// cloud to</param> <param name="points">The points to save</param> <param name="intensityMap">Intensities for each
  /// point, must be same length as points</param> <param name="useBinary">If the output file is binary or ascii</param>
  /// <param name="presentation">Definition how invalid points should be presented inside the PLY file
  /// [optional]</param> <returns>Returns true if write was successful and false otherwise</returns>
  static bool WriteFormatPLY(const char*                  filename,
                             const std::vector<PointXYZ>& points,
                             const std::vector<uint16_t>& intensityMap,
                             bool                         useBinary,
                             InvalidPointPresentation     presentation = INVALID_AS_NAN);

  /// <summary>Save a point cloud to a file in Polygon File Format (PLY) which has intensities and colors for each
  /// point, see: https://en.wikipedia.org/wiki/PLY_%28file_format%29 </summary> <param name="filename">The file to save
  /// the point cloud to</param> <param name="points">The points to save</param> <param name="rgbaMap">RGBA colors for
  /// each point, must be same length as points</param> <param name="intensityMap">Intensities for each point, must be
  /// same length as points</param> <param name="useBinary">If the output file is binary or ascii</param> <param
  /// name="presentation">Definition how invalid points should be presented inside the PLY file [optional]</param>
  /// <returns>Returns true if write was successful and false otherwise</returns>
  static bool WriteFormatPLY(const char*                  filename,
                             const std::vector<PointXYZ>& points,
                             const std::vector<uint32_t>& rgbaMap,
                             const std::vector<uint16_t>& intensityMap,
                             bool                         useBinary,
                             InvalidPointPresentation     presentation = INVALID_AS_NAN);

private:
  // No instantiations
  PointCloudPlyWriter();
  virtual ~PointCloudPlyWriter();
};

} // namespace visionary
