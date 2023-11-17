//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense
#include <cstdint>
#include <cstring>

#include "gtest/gtest.h"

#include "VisionaryEndian.h"

template <visionary::endian par>
struct Values;

static const union BE
{
  BE() : bytes{0x01u, 0x02u, 0x03u, 0x04u, 0x05u, 0x06u, 0x07u, 0x08u}
  {
  }

  std::uint8_t  bytes[8];
  std::uint8_t  u8;
  std::int8_t   i8;
  std::uint16_t u16;
  std::int16_t  i16;
  std::uint32_t u32;
  std::int32_t  i32;
  std::uint64_t u64;
  std::int64_t  i64;
} be;

static const union LE
{
  LE() : bytes{0x08u, 0x07u, 0x06u, 0x05u, 0x04u, 0x03u, 0x02u, 0x01u}
  {
  }

  std::uint8_t  bytes[8];
  std::uint8_t  u8;
  std::int8_t   i8;
  std::uint16_t u16;
  std::int16_t  i16;
  std::uint32_t u32;
  std::int32_t  i32;
  std::uint64_t u64;
  std::int64_t  i64;
} le;

TEST(VisionaryEndian, convertFrom_little)
{
  using namespace visionary;

  using EndianDut = Endian<endian::little, endian::native>;
  EXPECT_EQ(EndianDut::convertFrom<std::uint8_t>(le.bytes), 0x08u);
  EXPECT_EQ(EndianDut::convertFrom<std::int16_t>(le.bytes), 0x0708);
  EXPECT_EQ(EndianDut::convertFrom<std::uint32_t>(le.bytes), 0x05060708u);
  EXPECT_EQ(EndianDut::convertFrom<std::int64_t>(le.bytes), 0x0102030405060708ll);
}

TEST(VisionaryEndian, convertFrom_big)
{
  using namespace visionary;

  using EndianDut = Endian<endian::big, endian::native>;
  EXPECT_EQ(EndianDut::convertFrom<std::uint8_t>(be.bytes), 0x01u);
  EXPECT_EQ(EndianDut::convertFrom<std::int16_t>(be.bytes), 0x0102);
  EXPECT_EQ(EndianDut::convertFrom<std::uint32_t>(be.bytes), 0x01020304u);
  EXPECT_EQ(EndianDut::convertFrom<std::int64_t>(be.bytes), 0x0102030405060708ll);
}

TEST(VisionaryEndian, readUnalignedLittleEndian_is_same_as_convertFrom_little)
{
  using namespace visionary;

  using EndianDut = Endian<endian::little, endian::native>;

  EXPECT_EQ(EndianDut::convertFrom<std::uint8_t>(le.bytes), readUnalignLittleEndian<std::uint8_t>(le.bytes));
  EXPECT_EQ(EndianDut::convertFrom<std::int16_t>(le.bytes), readUnalignLittleEndian<std::int16_t>(le.bytes));
  EXPECT_EQ(EndianDut::convertFrom<std::uint32_t>(le.bytes), readUnalignLittleEndian<std::uint32_t>(le.bytes));
  EXPECT_EQ(EndianDut::convertFrom<std::int64_t>(le.bytes), readUnalignLittleEndian<std::int64_t>(le.bytes));
}

TEST(VisionaryEndian, readUnalignedBigEndian_is_same_as_convertFrom_big)
{
  using namespace visionary;

  using EndianDut = Endian<endian::big, endian::native>;

  EXPECT_EQ(EndianDut::convertFrom<std::uint8_t>(be.bytes), readUnalignBigEndian<std::uint8_t>(be.bytes));
  EXPECT_EQ(EndianDut::convertFrom<std::int16_t>(be.bytes), readUnalignBigEndian<std::int16_t>(be.bytes));
  EXPECT_EQ(EndianDut::convertFrom<std::uint32_t>(be.bytes), readUnalignBigEndian<std::uint32_t>(be.bytes));
  EXPECT_EQ(EndianDut::convertFrom<std::int64_t>(be.bytes), readUnalignBigEndian<std::int64_t>(be.bytes));
}

TEST(VisionaryEndian, writeUnalignedLittleEndian_is_same_as_convertTo_little)
{
  using namespace visionary;

  using EndianDut = Endian<endian::native, endian::little>;

  struct
  {
    std::uint8_t cf[8u];
    std::uint8_t ru[8u];
  } buf;

  EndianDut::convertTo<char>(buf.cf, sizeof(buf), 'A');
  writeUnalignLittleEndian<char>(buf.ru, sizeof(buf), 'A');
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 1u));

  EndianDut::convertTo<std::uint16_t>(buf.cf, sizeof(buf), 0xbeefu);
  writeUnalignLittleEndian<std::uint16_t>(buf.ru, sizeof(buf), 0xbeefu);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 2u));

  EndianDut::convertTo<std::int32_t>(buf.cf, sizeof(buf), -123456789);
  writeUnalignLittleEndian<std::int32_t>(buf.ru, sizeof(buf), -123456789);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 4u));

  EndianDut::convertTo<std::uint64_t>(buf.cf, sizeof(buf), 0xfeedbe117ee14e11ull);
  writeUnalignLittleEndian<std::uint64_t>(buf.ru, sizeof(buf), 0xfeedbe117ee14e11ull);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 8u));
}

TEST(VisionaryEndian, writeUnalignedBigEndian_is_same_as_convertTo_big)
{
  using namespace visionary;

  using EndianDut = Endian<endian::native, endian::big>;

  struct
  {
    std::uint8_t cf[8u];
    std::uint8_t ru[8u];
  } buf;

  EndianDut::convertTo<char>(buf.cf, sizeof(buf), 'b');
  writeUnalignBigEndian<char>(buf.ru, sizeof(buf), 'b');
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 1u));

  EndianDut::convertTo<std::uint16_t>(buf.cf, sizeof(buf), 0xbeefu);
  writeUnalignBigEndian<std::uint16_t>(buf.ru, sizeof(buf), 0xbeefu);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 2u));

  EndianDut::convertTo<std::int32_t>(buf.cf, sizeof(buf), -123456789);
  writeUnalignBigEndian<std::int32_t>(buf.ru, sizeof(buf), -123456789);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 4u));

  EndianDut::convertTo<std::uint64_t>(buf.cf, sizeof(buf), 0xfeedbe117ee14e11ull);
  writeUnalignBigEndian<std::uint64_t>(buf.ru, sizeof(buf), 0xfeedbe117ee14e11ull);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 8u));
}

TEST(VisionaryEndian, nativeToLittleEndian_is_same_as_convert_native_little)
{
  using namespace visionary;

  using EndianDut = Endian<endian::native, endian::little>;

  EXPECT_EQ(nativeToLittleEndian<std::int8_t>(0x7f), EndianDut::convert<std::int8_t>(0x7f));
  EXPECT_EQ(nativeToLittleEndian<std::uint16_t>(0xaffeu), EndianDut::convert<std::uint16_t>(0xaffeu));
  EXPECT_EQ(nativeToLittleEndian<std::uint32_t>(123456789), EndianDut::convert<std::uint32_t>(123456789));
  EXPECT_FLOAT_EQ(nativeToLittleEndian<float>(3.14159265e-27f), EndianDut::convert<float>(3.14159265e-27f));
  EXPECT_EQ(nativeToLittleEndian<std::int64_t>(-12345678917636455ll),
            EndianDut::convert<std::int64_t>(-12345678917636455ll));
  EXPECT_DOUBLE_EQ(nativeToLittleEndian<double>(3.14159265e-127), EndianDut::convert<double>(3.14159265e-127));
}

TEST(VisionaryEndian, nativeToBigEndian_is_same_as_convert_native_big)
{
  using namespace visionary;

  using EndianDut = Endian<endian::native, endian::big>;

  EXPECT_EQ(nativeToBigEndian<std::int8_t>(0x7f), EndianDut::convert<std::int8_t>(0x7f));
  EXPECT_EQ(nativeToBigEndian<std::uint16_t>(0xaffeu), EndianDut::convert<std::uint16_t>(0xaffeu));
  EXPECT_EQ(nativeToBigEndian<std::uint32_t>(123456789), EndianDut::convert<std::uint32_t>(123456789));
  EXPECT_FLOAT_EQ(nativeToBigEndian<float>(3.14159265e-27f), EndianDut::convert<float>(3.14159265e-27f));
  EXPECT_EQ(nativeToBigEndian<std::int64_t>(-12345678917636455ll),
            EndianDut::convert<std::int64_t>(-12345678917636455ll));
  EXPECT_DOUBLE_EQ(nativeToBigEndian<double>(3.14159265e-127), EndianDut::convert<double>(3.14159265e-127));
}

TEST(VisionaryEndian, littleEndianToNative_is_same_as_convert_little_native)
{
  using namespace visionary;

  using EndianDut = Endian<endian::little, endian::native>;

  EXPECT_EQ(littleEndianToNative<std::int8_t>(0x7f), EndianDut::convert<std::int8_t>(0x7f));
  EXPECT_EQ(littleEndianToNative<std::uint16_t>(0xaffeu), EndianDut::convert<std::uint16_t>(0xaffeu));
  EXPECT_EQ(littleEndianToNative<std::uint32_t>(123456789), EndianDut::convert<std::uint32_t>(123456789));
  EXPECT_FLOAT_EQ(littleEndianToNative<float>(3.14159265e-27f), EndianDut::convert<float>(3.14159265e-27f));
  EXPECT_EQ(littleEndianToNative<std::int64_t>(-12345678917636455ll),
            EndianDut::convert<std::int64_t>(-12345678917636455ll));
  EXPECT_DOUBLE_EQ(littleEndianToNative<double>(3.14159265e-127), EndianDut::convert<double>(3.14159265e-127));
}

TEST(VisionaryEndian, bigEndianToNative_is_same_as_convert_big_native)
{
  using namespace visionary;

  using EndianDut = Endian<endian::big, endian::native>;

  EXPECT_EQ(bigEndianToNative<std::int8_t>(0x7f), EndianDut::convert<std::int8_t>(0x7f));
  EXPECT_EQ(bigEndianToNative<std::uint16_t>(0xaffeu), EndianDut::convert<std::uint16_t>(0xaffeu));
  EXPECT_EQ(bigEndianToNative<std::uint32_t>(123456789), EndianDut::convert<std::uint32_t>(123456789));
  EXPECT_FLOAT_EQ(bigEndianToNative<float>(3.14159265e-27f), EndianDut::convert<float>(3.14159265e-27f));
  EXPECT_EQ(bigEndianToNative<std::int64_t>(-12345678917636455ll),
            EndianDut::convert<std::int64_t>(-12345678917636455ll));
  EXPECT_DOUBLE_EQ(bigEndianToNative<double>(3.14159265e-127), EndianDut::convert<double>(3.14159265e-127));
}

// The list of types we want to test.
using Implementations1 = ::testing::Types<visionary::Endian<visionary::endian::little, visionary::endian::native>,
                                          visionary::Endian<visionary::endian::big, visionary::endian::native>,
                                          visionary::Endian<visionary::endian::native, visionary::endian::little>,
                                          visionary::Endian<visionary::endian::native, visionary::endian::big>>;

template <typename T>
class VisionaryEndianTest1 : public testing::Test
{
public:
  using DUT = T;
};

TYPED_TEST_SUITE(VisionaryEndianTest1, Implementations1);

TYPED_TEST(VisionaryEndianTest1, convert_is_revertable)
{
  using namespace visionary;

  using Forward = typename TestFixture::DUT;
  using Inverse = Endian<Forward::to, Forward::from>;

  EXPECT_EQ(Inverse::template convert(Forward::template convert<signed char>(-'A')), -'A');
  EXPECT_EQ(Inverse::template convert(Forward::template convert<std::int16_t>(-12345)), -12345);
  EXPECT_EQ(Inverse::template convert(Forward::template convert<std::uint16_t>(55453u)), 55453u);
  EXPECT_EQ(Inverse::template convert(Forward::template convert<std::int32_t>(-98765453)), -98765453);
  EXPECT_FLOAT_EQ(Inverse::template convert(Forward::template convert<float>(-2.7182818e32f)), -2.7182818e32f);
  EXPECT_EQ(Inverse::template convert(Forward::template convert<std::uint64_t>(9876545367236465ull)),
            9876545367236465ull);
  EXPECT_DOUBLE_EQ(Inverse::template convert(Forward::template convert<double>(-2.7182818e132)), -2.7182818e132);
}

using Implementations2 = ::testing::Types<visionary::Endian<visionary::endian::little, visionary::endian::little>,
                                          visionary::Endian<visionary::endian::big, visionary::endian::big>,
                                          visionary::Endian<visionary::endian::native, visionary::endian::native>>;

template <typename T>
class VisionaryEndianTest2 : public testing::Test
{
public:
  using DUT = T;
};

TYPED_TEST_SUITE(VisionaryEndianTest2, Implementations2);

TYPED_TEST(VisionaryEndianTest2, convert_x_to_x_is_ident)
{
  using namespace visionary;

  using Forward = typename TestFixture::DUT;

  EXPECT_EQ(Forward::template convert<unsigned char>('A'), 'A');
  EXPECT_EQ(Forward::template convert<std::int16_t>(-12345), -12345);
  EXPECT_EQ(Forward::template convert<std::uint16_t>(55453u), 55453u);
  EXPECT_EQ(Forward::template convert<std::int32_t>(-98765453), -98765453);
  EXPECT_EQ(Forward::template convert<float>(-2.7182818e32f), -2.7182818e32f);
  EXPECT_EQ(Forward::template convert<std::uint64_t>(9876545367236465ull), 9876545367236465ull);
  EXPECT_EQ(Forward::template convert<double>(-2.7182818e132), -2.7182818e132);
}

TYPED_TEST(VisionaryEndianTest2, convertTo_x_is_ident)
{
  using namespace visionary;

  using Forward = typename TestFixture::DUT;

  struct
  {
    std::uint8_t cf[8u];
    std::uint8_t ru[8u];
  } buf;

  Forward::template convertTo<unsigned char>(buf.cf, sizeof(buf.cf), 'A');
  writeUnaligned<unsigned char>(buf.ru, sizeof(buf.ru), 'A');
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 1u));

  Forward::template convertTo<std::int16_t>(buf.cf, sizeof(buf.cf), -12345);
  writeUnaligned<std::int16_t>(buf.ru, sizeof(buf.ru), -12345);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 2u));

  Forward::template convertTo<float>(buf.cf, sizeof(buf.cf), -2.7182818e32f);
  writeUnaligned<float>(buf.ru, sizeof(buf.ru), -2.7182818e32f);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 4u));

  Forward::template convertTo<std::uint64_t>(buf.cf, sizeof(buf.cf), 9876545367236465ull);
  writeUnaligned<std::uint64_t>(buf.ru, sizeof(buf.ru), 9876545367236465ull);
  EXPECT_EQ(0, std::memcmp(buf.cf, buf.ru, 4u));
}
