//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense
#include <algorithm>

#include "MockTransport.h"
#include "VisionaryDataStream.h"
#include "VisionaryEndian.h"
#include "VisionaryTMiniData.h"
#include "gtest/gtest.h"

namespace {
using ByteBuffer = std::vector<std::uint8_t>;

void appendToVector(const ByteBuffer& src, ByteBuffer& dst)
{
  dst.reserve(src.size() + dst.size());
  dst.insert(dst.end(), src.begin(), src.end());
}

ByteBuffer uint32ToBEVector(std::uint32_t n)
{
  ByteBuffer retVal;
  retVal.push_back(static_cast<uint8_t>(n >> 24));
  retVal.push_back(static_cast<uint8_t>(n >> 16));
  retVal.push_back(static_cast<uint8_t>(n >> 8));
  retVal.push_back(static_cast<uint8_t>(n));
  return retVal;
}

void setBlobLength(ByteBuffer& blobVector)
{
  auto blobLengthVector = uint32ToBEVector(static_cast<std::uint32_t>(blobVector.size() - 8u));
  memcpy(&blobVector[4], &blobLengthVector[0], 4u);
}

const ByteBuffer    kMagicBytes      = {0x02, 0x02, 0x02, 0x02};
const ByteBuffer    kProtocolVersion = {0x0, 0x1};
const ByteBuffer    kPackageType     = {0x62u};
const ByteBuffer    kBlobId          = {0x0u, 0x0u};
const ByteBuffer    kNumSegements    = {0x0u, 0x3u};
const ByteBuffer    kXMLOffset       = {0x0u, 0x0u, 0x0u, 0x1Cu};
const ByteBuffer    kBlobVersion     = {0x0u, 0x2u};
const std::uint32_t kDataSetSize     = 1302528u;
const std::string   kXMLStr =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?><SickRecord xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
  "xsi:noNamespaceSchemaLocation=\"SickRecord_schema.xsd\"><Revision>SICK V1.10 in "
  "work</Revision><SchemaChecksum>01020304050607080910111213141516</SchemaChecksum><ChecksumFile>checksum.hex</"
  "ChecksumFile><RecordDescription><Location>V3SXX5-1</Location><StartDateTime>2023-03-31T11:09:33+02:00</"
  "StartDateTime><EndDateTime>2023-03-31T11:09:37+02:00</EndDateTime><UserName>default</UserName><RecordToolName>Sick "
  "Scandata "
  "Recorder</RecordToolName><RecordToolVersion>v0.4</RecordToolVersion><ShortDescription></ShortDescription></"
  "RecordDescription><DataSets><DataSetDepthMap id=\"1\" "
  "datacount=\"1\"><DeviceDescription><Family>V3SXX5-1</Family><Ident>Visionary-T Mini CX V3S105-1x "
  "2.0.0.457B</Ident><Version>3.0.0.2334</Version><SerialNumber>12345678</SerialNumber><LocationName>not "
  "defined</LocationName><IPAddress>192.168.136.10</IPAddress></"
  "DeviceDescription><FormatDescriptionDepthMap><TimestampUTC/><Version>uint16</"
  "Version><DataStream><Interleaved>false</Interleaved><Width>512</Width><Height>424</"
  "Height><CameraToWorldTransform><value>1.000000</value><value>0.000000</value><value>0.000000</"
  "value><value>0.000000</value><value>0.000000</value><value>1.000000</value><value>0.000000</value><value>0.000000</"
  "value><value>0.000000</value><value>0.000000</value><value>1.000000</value><value>-10.000000</"
  "value><value>0.000000</value><value>0.000000</value><value>0.000000</value><value>1.000000</value></"
  "CameraToWorldTransform><CameraMatrix><FX>-366.964999</FX><FY>-367.057999</FY><CX>252.118999</CX><CY>205.213999</"
  "CY></CameraMatrix><CameraDistortionParams><K1>-0.076050</K1><K2>0.217518</K2><P1>0.000000</P1><P2>0.000000</"
  "P2><K3>0.000000</K3></CameraDistortionParams><FrameNumber>uint32</FrameNumber><Quality>uint8</"
  "Quality><Status>uint8</Status><PixelSize><X>1.000000</X><Y>1.000000</Y><Z>0.250000</Z></PixelSize><Distance "
  "decimalexponent=\"0\" min=\"1\" max=\"16384\">uint16</Distance><Intensity decimalexponent=\"0\" min=\"1\" "
  "max=\"20000\">uint16</Intensity><Confidence decimalexponent=\"0\" min=\"0\" "
  "max=\"65535\">uint16</Confidence></DataStream><DeviceInfo><Status>OK</Status></DeviceInfo></"
  "FormatDescriptionDepthMap><DataLink><FileName>data.bin</FileName><Checksum>01020304050607080910111213141516</"
  "Checksum></DataLink><OverlayLink><FileName>overlay.xml</FileName></OverlayLink></DataSetDepthMap></DataSets></"
  "SickRecord>";
const ByteBuffer kXMLVec(kXMLStr.begin(), kXMLStr.end());
} // namespace

using namespace visionary;

//---------------------------------------------------------------------------------------
TEST(VisionaryTMiniDataTest, InvalidMagicBytes)
{
  ByteBuffer buffer{kMagicBytes};
  buffer[3] = 0x01;
  buffer.insert(buffer.end(), 5000, 0);

  std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
  VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

  dataStream.open(pTransport);

  ASSERT_FALSE(dataStream.getNextFrame());
}

//---------------------------------------------------------------------------------------
TEST(VisionaryTMiniDataTest, MissingHeader)
{
  std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{kMagicBytes}};
  VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

  dataStream.open(pTransport);

  ASSERT_FALSE(dataStream.getNextFrame());
}

//---------------------------------------------------------------------------------------
TEST(VisionaryTMiniDataTest, WrongHeader)
{
  ByteBuffer buffer{kMagicBytes};
  ByteBuffer length        = {0x0, 0x0, 0x0, 0x2};
  ByteBuffer wrongProtocol = {0x0, 0x0};
  appendToVector(length, buffer);

  {
    std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
    VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

    dataStream.open(pTransport);

    EXPECT_FALSE(dataStream.getNextFrame());
  }

  // Wrong Protocol
  {
    buffer = kMagicBytes;
    length = {0x0, 0x0, 0x0, 0x3};
    appendToVector(length, buffer);
    appendToVector(wrongProtocol, buffer);
    appendToVector(kPackageType, buffer);

    std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
    VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

    dataStream.open(pTransport);

    EXPECT_FALSE(dataStream.getNextFrame());
  }

  // Wrong Package Type
  {
    buffer = kMagicBytes;
    length = {0x0, 0x0, 0x0, 0x3};
    appendToVector(length, buffer);
    appendToVector(kProtocolVersion, buffer);
    buffer.push_back(0x61);

    std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
    VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

    dataStream.open(pTransport);

    EXPECT_FALSE(dataStream.getNextFrame());
  }
}

//---------------------------------------------------------------------------------------
TEST(VisionaryTMiniDataTest, NoDataHandler)
{
  ByteBuffer buffer{kMagicBytes};
  ByteBuffer length = {0x0, 0x0, 0xFFu, 0xFFu};
  appendToVector(length, buffer);
  appendToVector(kProtocolVersion, buffer);
  appendToVector(kPackageType, buffer);
  // Append dummy data
  buffer.insert(buffer.end(), 65536u + 3u, 0u);

  std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
  VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

  dataStream.setDataHandler(nullptr);
  dataStream.open(pTransport);
  EXPECT_FALSE(dataStream.getNextFrame());
}

//---------------------------------------------------------------------------------------
TEST(VisionaryTMiniDataTest, InvalidBlobData)
{
  ByteBuffer buffer{kMagicBytes};
  ByteBuffer length = {0x0u, 0x0u, 0xFFu, 0xFFu};
  appendToVector(length, buffer);
  appendToVector(kProtocolVersion, buffer);
  appendToVector(kPackageType, buffer);
  ByteBuffer bufferBase = buffer;
  // Append dummy data
  buffer.insert(buffer.end(), 65536u + 3u, 0u);
  // Invalid segment count
  {
    std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
    VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

    dataStream.open(pTransport);
    EXPECT_FALSE(dataStream.getNextFrame());
  }
  // Corrupted XML part
  buffer = bufferBase;
  appendToVector(kBlobId, buffer);
  appendToVector(kNumSegements, buffer);
  appendToVector(kXMLOffset, buffer);
  buffer.insert(buffer.end(), 3u, 0x0u);
  buffer.push_back(0x1u); // set change counter to 1
  std::uint32_t binaryOffset    = static_cast<std::uint32_t>(kXMLVec.size() + 28u);
  const auto    binaryOffsetVec = uint32ToBEVector(binaryOffset);
  appendToVector(binaryOffsetVec, buffer);
  buffer.insert(buffer.end(), 4u, 0x0u);
  const auto footerOffsetVec = uint32ToBEVector(binaryOffset + kDataSetSize + 4u + 8u + 2u + 6u + 8u);
  appendToVector(footerOffsetVec, buffer);
  buffer.insert(buffer.end(), 4u, 0x0u);
  appendToVector(kXMLVec, buffer);
  const auto bufferWithXMLBase = buffer;
  buffer.erase(buffer.end() - 10, buffer.end());
  setBlobLength(buffer);

  {
    std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
    VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

    dataStream.open(pTransport);
    EXPECT_FALSE(dataStream.getNextFrame());
  }

  {
    // Corrupted Binary part
    buffer                  = bufferWithXMLBase;
    ByteBuffer binLengthVec = uint32ToBEVector(kDataSetSize);
    std::reverse(binLengthVec.begin(), binLengthVec.end());
    appendToVector(binLengthVec, buffer);
    buffer.insert(buffer.end(), 8u, 0x0u); // Timestamp
    appendToVector(kBlobVersion, buffer);
    buffer.insert(buffer.end(), 6u, 0x0u);           // Extended Header
    buffer.insert(buffer.end(), kDataSetSize, 0x0u); // Add Image Data (4 bytes(CRC) are missing)
    setBlobLength(buffer);

    {
      std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
      VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

      dataStream.open(pTransport);
      EXPECT_FALSE(dataStream.getNextFrame());
    }
  }
}

//---------------------------------------------------------------------------------------
TEST(VisionaryTMiniDataTest, ValidBlobData)
{
  ByteBuffer buffer{kMagicBytes};
  ByteBuffer length = {0x0u, 0x0u, 0x00u, 0x00u};
  appendToVector(length, buffer);
  appendToVector(kProtocolVersion, buffer);
  appendToVector(kPackageType, buffer);
  appendToVector(kBlobId, buffer);
  appendToVector(kNumSegements, buffer);
  appendToVector(kXMLOffset, buffer);
  buffer.insert(buffer.end(), 3u, 0x0u);
  buffer.push_back(0x1u); // set change counter to 1
  std::uint32_t binaryOffset    = static_cast<std::uint32_t>(kXMLVec.size() + 28u);
  const auto    binaryOffsetVec = uint32ToBEVector(binaryOffset);
  appendToVector(binaryOffsetVec, buffer);
  buffer.insert(buffer.end(), 4u, 0x0u);
  const auto footerOffsetVec = uint32ToBEVector(binaryOffset + kDataSetSize + 4u + 8u + 2u + 6u + 8u);
  appendToVector(footerOffsetVec, buffer);
  buffer.insert(buffer.end(), 4u, 0x0u);
  appendToVector(kXMLVec, buffer);
  ByteBuffer binLengthVec = uint32ToBEVector(kDataSetSize);
  std::reverse(binLengthVec.begin(), binLengthVec.end());
  appendToVector(binLengthVec, buffer);
  buffer.insert(buffer.end(), 8u, 0x0u); // Timestamp
  appendToVector(kBlobVersion, buffer);
  buffer.insert(buffer.end(), 6u, 0x0u);           // Extended Header
  buffer.insert(buffer.end(), kDataSetSize, 0x0u); // Add Image Data
  buffer.insert(buffer.end(), 4u, 0x0u);           // CRC
  appendToVector(binLengthVec, buffer);
  setBlobLength(buffer);

  std::unique_ptr<ITransport> pTransport{new visionary_test::MockTransport{buffer}};
  VisionaryDataStream         dataStream{std::make_shared<VisionaryTMiniData>()};

  dataStream.open(pTransport);
  EXPECT_TRUE(dataStream.getNextFrame());
}
