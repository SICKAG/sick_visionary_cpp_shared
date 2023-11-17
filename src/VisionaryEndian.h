//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <algorithm> // min, max
#include <cstdint>
#include <cstring> // for memcpy
#include <stdexcept>
#include <vector>

#if __cplusplus >= 202002L
#  include <bit> // for endian
#endif

namespace visionary {

#if __cplusplus >= 202002L
using endian = std::endian;
#else
#  if defined(_WIN32)
enum class endian
{
  little = 1,
  big    = 2,
  native = 1
};
#  else
// gcc or clang
enum class endian
{
  little = __ORDER_LITTLE_ENDIAN__,
  big    = __ORDER_BIG_ENDIAN__,
  native = __BYTE_ORDER__
};
#  endif
#endif

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class T>
void writeUnaligned(void* ptr, std::size_t nBytes, const T& val)
{
  if (sizeof(T) > nBytes)
  {
    throw std::out_of_range("buffer too small");
  }

  std::memcpy(ptr, &val, sizeof(T));
}

template <class T>
T readUnaligned(const void* ptr)
{
  T r;
  std::memcpy(&r, ptr, sizeof(T));
  return r;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename TAlias, typename T>
inline T byteswapAlias(const T& val);

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline std::uint8_t byteswap(std::uint8_t val)
{
  return val;
}

inline std::int8_t byteswap(std::int8_t val)
{
  return byteswapAlias<std::uint8_t>(val);
}

inline char byteswap(char val)
{
  return byteswapAlias<std::uint8_t>(val);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline std::uint16_t byteswap(std::uint16_t val)
{
  return (static_cast<std::uint16_t>(val << 8) & 0xFF00u) | (static_cast<std::uint16_t>(val >> 8) & 0x00FFu);
}

inline std::int16_t byteswap(std::int16_t val)
{
  return byteswapAlias<std::uint16_t>(val);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline std::uint32_t byteswap(std::uint32_t val)
{
  val = ((val << 8) & 0xFF00FF00u) | ((val >> 8) & 0x00FF00FFu);
  return ((val << 16) & 0xFFFF0000u) | ((val >> 16) & 0x0000FFFFu);
}

inline std::int32_t byteswap(std::int32_t val)
{
  return byteswapAlias<std::uint32_t>(val);
}

inline float byteswap(float val)
{
  union
  {
    float         f32;
    std::uint32_t u32;
  } v;
  v.f32 = val;
  v.u32 = byteswap(v.u32);

  return v.f32;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline std::uint64_t byteswap(std::uint64_t val)
{
  val = ((val << 8) & 0xFF00FF00FF00FF00ull) | ((val >> 8) & 0x00FF00FF00FF00FFull);
  val = ((val << 16) & 0xFFFF0000FFFF0000ull) | ((val >> 16) & 0x0000FFFF0000FFFFull);
  return ((val << 32) & 0xFFFFFFFF00000000ull) | ((val >> 32) & 0x00000000FFFFFFFFull);
}

inline std::int64_t byteswap(std::int64_t val)
{
  return byteswapAlias<std::uint64_t>(val);
}

inline double byteswap(double val)
{
  union
  {
    double        f64;
    std::uint64_t u64;
  } v;
  v.f64 = val;
  v.u64 = byteswap(v.u64);

  return v.f64;
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename TAlias, typename T>
inline T byteswapAlias(const T& val)
{
  return static_cast<T>(byteswap(static_cast<TAlias>(val)));
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Endianness convert: byte swap if `frompar` and `topar` endian values are different;
// just return identity if they are the same
template <endian frompar, endian topar>
struct Endian
{
  static constexpr auto from = frompar;
  static constexpr auto to   = topar;

  using ByteVector = std::vector<std::uint8_t>;

  template <typename T>
  static T convert(const T& val)
  {
    return byteswap(val);
  }

  template <typename T>
  static void convertTo(void* pDest, size_t nSize, const T& val)
  {
    writeUnaligned<T>(pDest, nSize, convert(val));
  }

  template <typename T>
  static T convertFrom(const void* pSrc)
  {
    return convert<T>(readUnaligned<T>(pSrc));
  }

  template <typename T>
  static ByteVector convertToVector(const T& val, size_t capacity = 0u)
  {
    ByteVector vec;
    vec.reserve(std::max(sizeof(T), capacity));
    vec.resize(sizeof(T));
    convertTo<T>(vec.data(), vec.size(), val);

    return vec;
  }

  template <typename T, class TInputIt>
  static bool convertFrom(T& rval, TInputIt& first, const TInputIt& last)
  {
    uint8_t buf[sizeof(T)];
    size_t  idx = 0u;
    while (idx < sizeof(T))
    {
      if (first == last)
      {
        // iterator hit the end prematurely
        return false;
      }
      buf[idx++] = *first;
      ++first;
    }

    rval = convertFrom<T>(buf);

    return true;
  }
};

template <endian frompar>
struct Endian<frompar, frompar>
{
  static constexpr auto from = frompar;
  static constexpr auto to   = frompar;

  using ByteVector = std::vector<std::uint8_t>;

  template <typename T>
  static T convert(const T& val)
  {
    return val;
  }

  template <typename T>
  static void convertTo(void* pDest, size_t nSize, const T& val)
  {
    writeUnaligned(pDest, nSize, val);
  }

  template <typename T>
  static T convertFrom(const void* pSrc)
  {
    return readUnaligned<T>(pSrc);
  }

  template <typename T>
  static ByteVector convertToVector(const T& val, size_t capacity = 0u)
  {
    ByteVector vec;
    vec.reserve(std::max(sizeof(T), capacity));
    vec.resize(sizeof(T));
    convertTo<T>(vec.data(), vec.size(), val);

    return vec;
  }

  template <typename T, class TInputIt>
  static T convertFrom(T& rval, TInputIt& first, const TInputIt& last)
  {
    uint8_t buf[sizeof(T)] = {0};

    size_t idx = 0u;

    while ((idx < sizeof(T)) && (first < last))
    {
      if (first == last)
      {
        // iterator hit the end prematurely
        return false;
      }
      buf[idx++] = *first;
      ++first;
    }

    rval = convertFrom<T>(buf);

    return true;
  }
};

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename T>
inline T nativeToLittleEndian(const T& val)
{
  return Endian<endian::native, endian::little>::convert(val);
}

template <typename T>
inline T littleEndianToNative(T val)
{
  return Endian<endian::little, endian::native>::convert(val);
}

template <typename T>
inline T nativeToBigEndian(const T& val)
{
  return Endian<endian::native, endian::big>::convert(val);
}

template <typename T>
inline T bigEndianToNative(T val)
{
  return Endian<endian::big, endian::native>::convert(val);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <typename T>
inline void writeUnalignBigEndian(void* ptr, std::size_t nBytes, const T& value)
{
  Endian<endian::native, endian::big>::convertTo<T>(ptr, nBytes, value);
}

template <typename T>
inline void writeUnalignLittleEndian(void* ptr, std::size_t nBytes, const T& value)
{
  Endian<endian::native, endian::little>::convertTo<T>(ptr, nBytes, value);
}

template <typename T>
inline T readUnalignBigEndian(const void* ptr)
{
  const std::uint8_t* ptr8 = static_cast<const std::uint8_t*>(ptr);

  return Endian<endian::big, endian::native>::convertFrom<T>(ptr8);
}

template <typename T>
inline T readUnalignLittleEndian(const void* ptr)
{
  const std::uint8_t* ptr8 = static_cast<const std::uint8_t*>(ptr);
  return Endian<endian::little, endian::native>::convertFrom<T>(ptr8);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

} // namespace visionary
