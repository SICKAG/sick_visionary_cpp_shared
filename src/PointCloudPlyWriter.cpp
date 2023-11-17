//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#include "PointCloudPlyWriter.h"

#include "VisionaryEndian.h"
#include <cmath>
#include <fstream>
#include <sstream>

namespace visionary {

bool PointCloudPlyWriter::WriteFormatPLY(const char*                  filename,
                                         const std::vector<PointXYZ>& points,
                                         bool                         useBinary,
                                         InvalidPointPresentation     presentation)
{
  return WriteFormatPLY(filename, points, std::vector<uint32_t>(), std::vector<uint16_t>(), useBinary, presentation);
}

bool PointCloudPlyWriter::WriteFormatPLY(const char*                  filename,
                                         const std::vector<PointXYZ>& points,
                                         const std::vector<uint32_t>& rgbaMap,
                                         bool                         useBinary,
                                         InvalidPointPresentation     presentation)
{
  return WriteFormatPLY(filename, points, rgbaMap, std::vector<uint16_t>(), useBinary, presentation);
}

bool PointCloudPlyWriter::WriteFormatPLY(const char*                  filename,
                                         const std::vector<PointXYZ>& points,
                                         const std::vector<uint16_t>& intensityMap,
                                         bool                         useBinary,
                                         InvalidPointPresentation     presentation)
{
  return WriteFormatPLY(filename, points, std::vector<uint32_t>(), intensityMap, useBinary, presentation);
}

bool PointCloudPlyWriter::WriteFormatPLY(const char*                  filename,
                                         const std::vector<PointXYZ>& points,
                                         const std::vector<uint32_t>& rgbaMap,
                                         const std::vector<uint16_t>& intensityMap,
                                         bool                         useBinary,
                                         InvalidPointPresentation     presentation)
{
  bool success = true;

  bool hasColors      = points.size() == rgbaMap.size();
  bool hasIntensities = points.size() == intensityMap.size();

  std::ofstream stream; // output file stream for PLY file
  std::stringstream
    strstream; // temporary data buffer stream
               // Reason: On presentation mode INVALID_SKIP we only know the number of "element vertex ?"
               //         in the header of the PLY file after iterating through the "vector<PointXYZ>& points"
               //         and checking if it has the value NaN. The header has to be printed before and
               //         therefore the data part has to be temprorarily buffered.

  int numberOfValidPoints = 0;
  if (useBinary == false)
  {
    // Write all points
    for (size_t i = 0; i < points.size(); i++)
    {
      PointXYZ point = points.at(i);

      // Handle PLY file presentation of X Y Z values (nan, 0.0 or SKIP)
      if (presentation == INVALID_AS_NAN)
      {
        strstream << point.x << " " << point.y << " " << point.z;
      }
      if (presentation == INVALID_AS_ZERO)
      {
        if (std::isnan(point.x))
          strstream << "0.0";
        else
          strstream << point.x;
        strstream << " ";

        if (std::isnan(point.y))
          strstream << "0.0";
        else
          strstream << point.y;
        strstream << " ";

        if (std::isnan(point.z))
          strstream << "0.0";
        else
          strstream << point.z;
      }
      if (presentation == INVALID_AS_NAN || presentation == INVALID_AS_ZERO)
      {
        if (hasColors)
        {
          const auto rgba = reinterpret_cast<const uint8_t*>(&rgbaMap.at(i));
          strstream << " " << static_cast<uint32_t>(rgba[0]) << " " << static_cast<uint32_t>(rgba[1]) << " "
                    << static_cast<uint32_t>(rgba[2]);
        }
        if (hasIntensities)
        {
          float intensity = static_cast<float>(intensityMap.at(i)) / 65535.0f;
          strstream << " " << intensity;
        }
        strstream << "\n";
      }
      if (presentation == INVALID_SKIP)
      {
        // The X and Y values are calculated using the Z/distance value which is received by the device.
        // So X and Y should only be NaN if Z/distance is NaN.
        if (std::isnan(point.z))
        {
          continue;
        }
        else
        {
          strstream << point.x << " " << point.y << " " << point.z;

          if (hasColors)
          {
            const auto rgba = reinterpret_cast<const uint8_t*>(&rgbaMap.at(i));
            strstream << " " << static_cast<uint32_t>(rgba[0]) << " " << static_cast<uint32_t>(rgba[1]) << " "
                      << static_cast<uint32_t>(rgba[2]);
          }
          if (hasIntensities)
          {
            float intensity = static_cast<float>(intensityMap.at(i)) / 65535.0f;
            strstream << " " << intensity;
          }
          strstream << "\n";

          numberOfValidPoints++;
        }
      }
    }
  }
  else
  {
    // Write all points
    for (size_t i = 0; i < points.size(); i++)
    {
      PointXYZ point = points.at(i);

      // Handle PLY file presentation of X Y Z values (nan, 0.0 or SKIP)
      if (presentation == INVALID_AS_NAN)
      {
        // Nothing to do, use values as they are available
      }
      if (presentation == INVALID_AS_ZERO)
      {
        if (std::isnan(point.x))
          point.x = 0.0f;
        if (std::isnan(point.y))
          point.y = 0.0f;
        if (std::isnan(point.z))
          point.z = 0.0f;
      }
      if (presentation == INVALID_AS_NAN || presentation == INVALID_AS_ZERO)
      {
        float x = nativeToLittleEndian(point.x);
        float y = nativeToLittleEndian(point.y);
        float z = nativeToLittleEndian(point.z);

        strstream.write(reinterpret_cast<char*>(&x), 4);
        strstream.write(reinterpret_cast<char*>(&y), 4);
        strstream.write(reinterpret_cast<char*>(&z), 4);

        if (hasColors)
        {
          strstream.write(reinterpret_cast<const char*>(&rgbaMap.at(i)), 3);
        }
        if (hasIntensities)
        {
          float intensity = static_cast<float>(intensityMap.at(i)) / 65535.0f;
          strstream.write(reinterpret_cast<const char*>(&intensity), 4);
        }
      }
      if (presentation == INVALID_SKIP)
      {
        // The X and Y values are calculated using the Z/distance value which is received by the device.
        // So X and Y should only be NaN if Z/distance is NaN.
        if (std::isnan(point.z))
        {
          continue;
        }
        else
        {
          float x = nativeToLittleEndian(point.x);
          float y = nativeToLittleEndian(point.y);
          float z = nativeToLittleEndian(point.z);

          strstream.write(reinterpret_cast<char*>(&x), 4);
          strstream.write(reinterpret_cast<char*>(&y), 4);
          strstream.write(reinterpret_cast<char*>(&z), 4);

          if (hasColors)
          {
            strstream.write(reinterpret_cast<const char*>(&rgbaMap.at(i)), 3);
          }
          if (hasIntensities)
          {
            float intensity = static_cast<float>(intensityMap.at(i)) / 65535.0f;
            strstream.write(reinterpret_cast<const char*>(&intensity), 4);
          }

          numberOfValidPoints++;
        }
      }
    }
  }

  // Open file
  stream.open(filename, useBinary ? (std::ios_base::out | std::ios_base::binary) : std::ios_base::out);

  if (stream.is_open())
  {
    // Write header
    stream << "ply\n";
    stream << "format " << (useBinary ? "binary_little_endian" : "ascii") << " 1.0\n";

    if (presentation == INVALID_SKIP)
    {
      stream << "element vertex " << numberOfValidPoints << "\n";
    }
    else
    {
      stream << "element vertex " << points.size() << "\n";
    }

    stream << "property float x\n";
    stream << "property float y\n";
    stream << "property float z\n";
    if (hasColors)
    {
      stream << "property uchar red\n";
      stream << "property uchar green\n";
      stream << "property uchar blue\n";
    }
    if (hasIntensities)
    {
      stream << "property float intensity\n";
    }
    stream << "end_header\n";
  }
  else
  {
    success = false;
  }

  // Add the temporarily buffered data part to the ofstream
  stream << strstream.rdbuf();

  // Close file
  stream.close();

  return success;
}

PointCloudPlyWriter::PointCloudPlyWriter() = default;

PointCloudPlyWriter::~PointCloudPlyWriter() = default;
} // namespace visionary
