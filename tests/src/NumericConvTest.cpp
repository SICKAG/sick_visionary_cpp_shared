//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense
#include <cstdint>
#include <limits>

#include "gtest/gtest.h"

#include "NumericConv.h"

TEST(NumericConvTest, subtype)
{
  EXPECT_EQ(visionary::castClamped<std::uint64_t>(std::uint8_t(9)), 9u);
  EXPECT_EQ(visionary::castClamped<std::int32_t>(std::int16_t(-10000)), -10000);
}

TEST(NumericConvTest, same_type)
{
  EXPECT_EQ(visionary::castClamped<std::uint8_t>(std::uint8_t(9)), 9u);
  EXPECT_EQ(visionary::castClamped<std::int16_t>(std::int16_t(-10000)), -10000);
}

TEST(NumericConvTest, int_unsigned_and_unsigned)
{
  EXPECT_EQ(visionary::castClamped<std::uint64_t>(std::uint8_t(9)), 9u);
  EXPECT_EQ(visionary::castClamped<std::uint8_t>(std::uint64_t(9)), 9u);
  EXPECT_EQ(visionary::castClamped<std::uint8_t>(std::uint64_t(100000000)), 255u);
}

TEST(NumericConvTest, int_signed_and_signed)
{
  EXPECT_EQ(visionary::castClamped<std::int64_t>(std::int8_t(9)), 9);
  EXPECT_EQ(visionary::castClamped<std::int64_t>(std::int8_t(-9)), -9);
  EXPECT_EQ(visionary::castClamped<std::int8_t>(std::int64_t(-9)), -9);
  EXPECT_EQ(visionary::castClamped<std::int8_t>(std::int64_t(-100000000)), -128);
  EXPECT_EQ(visionary::castClamped<std::int8_t>(std::int64_t(1000000000)), 127);
}

TEST(NumericConvTest, int_signed_and_unsigned)
{
  EXPECT_EQ(visionary::castClamped<std::int64_t>(std::uint8_t(9)), 9);
  EXPECT_EQ(visionary::castClamped<std::int8_t>(std::uint64_t(100000000)), 127);
  EXPECT_EQ(visionary::castClamped<std::int8_t>(std::uint64_t(0)), 0);
}

TEST(NumericConvTest, int_unsigned_and_signed)
{
  EXPECT_EQ(visionary::castClamped<std::uint64_t>(std::int8_t(9)), 9u);
  EXPECT_EQ(visionary::castClamped<std::uint64_t>(std::int8_t(-9)), 0u);
  EXPECT_EQ(visionary::castClamped<std::uint64_t>(std::int8_t(0)), 0u);
  EXPECT_EQ(visionary::castClamped<std::uint8_t>(std::int64_t(-100000000)), 0u);
  EXPECT_EQ(visionary::castClamped<std::uint8_t>(std::int64_t(100000000)), 255u);
}

TEST(NumericConvTest, float_and_int)
{
  EXPECT_NEAR(visionary::castClamped<float>(std::uint64_t(0xffffffffffffffffull)), 1.88e19, 0.1e19);
  EXPECT_EQ(visionary::castClamped<std::int64_t>(float(-1.235e38f)), -0x7fffffffffffffffll - 1);

#if !(defined(_MSC_VER) && (_MSC_VER >= 1900) && (_MSC_VER < 2000))
  // MSVC 2019 gives an warning (don't know why), though results are ok
  EXPECT_NEAR(visionary::castClamped<float>(double(1.234e308)), 3.4e38f, 0.1e38f);
#endif

  EXPECT_NEAR(visionary::castClamped<double>(float(-1.234e27f)), -1.234e27, 1e20);
}
