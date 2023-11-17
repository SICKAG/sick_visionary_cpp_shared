//
// Copyright (c) 2023 SICK AG, Waldkirch
//
// SPDX-License-Identifier: Unlicense

#pragma once

#include <limits>      // for type value ranges
#include <type_traits> // for meta programming

namespace visionary {

// Helper for chosing the right clamping method
// depending on signedness of target and source types
template <typename TTrg, typename TSrc, bool u1, bool u2>
struct NumericCastImpl
{
  // here both types are signed
  static TTrg clamped(const TSrc& src)
  {
    static constexpr TTrg trgMinVal = std::numeric_limits<TTrg>::lowest();
    static constexpr TTrg trgMaxVal = std::numeric_limits<TTrg>::max();

    using max_type = typename std::common_type<TTrg, TSrc>::type;

    if (static_cast<max_type>(src) >= static_cast<max_type>(trgMaxVal))
    {
      return trgMaxVal;
    }
    else if (static_cast<max_type>(src) <= static_cast<max_type>(trgMinVal))
    {
      return trgMinVal;
    }
    else
    {
      return static_cast<TTrg>(src);
    }
  }
};

template <typename TTrg, typename TSrc>
struct NumericCastImpl<TTrg, TSrc, true, false>
{
  static TTrg clamped(const TSrc& src)
  {
    static constexpr TTrg trgMinVal = std::numeric_limits<TTrg>::lowest();
    static constexpr TTrg trgMaxVal = std::numeric_limits<TTrg>::max();

    using max_type = typename std::common_type<TTrg, TSrc>::type;

    // a negative src value might be converted to unsigned yielding wrong results
    // since 0 is (for unsigned types) also the ::min, we can omit an additional test for src < trgMinVal
    if (src < 0)
    {
      return trgMinVal;
    }
    if (static_cast<max_type>(src) > static_cast<max_type>(trgMaxVal))
    {
      return trgMaxVal;
    }
    else
    {
      return static_cast<TTrg>(src);
    }
  }
};

template <typename TTrg, typename TSrc>
struct NumericCastImpl<TTrg, TSrc, false, true>
{
  static TTrg clamped(const TSrc& src)
  {
    static constexpr TTrg trgMaxVal = std::numeric_limits<TTrg>::max();

    using max_type = typename std::common_type<TTrg, TSrc>::type;

    // only need to clip to max value
    if (static_cast<max_type>(src) > static_cast<max_type>(trgMaxVal))
    {
      return trgMaxVal;
    }
    else
    {
      return static_cast<TTrg>(src);
    }
  }
};

/// Template to cast from a source numeric to a destination type
///
/// \tparam TTrg cast target type
/// \tparam TSrc cast source type
template <typename TTrg, typename TSrc>
struct NumericCast
{
  /// Cast from source to target type
  /// values outside the destination types range will be clamped to
  /// lie within the target types value range.
  /// \param[in] src value to be casted
  /// \return \a src valeus casted to the target type and clamped to the target types value range
  static TTrg clamped(const TSrc& src)
  {
    return NumericCastImpl<TTrg, TSrc, std::is_unsigned<TTrg>::value, std::is_unsigned<TSrc>::value>::clamped(src);
  }
};

template <typename TSrc>
struct NumericCast<TSrc, TSrc>
{
  static TSrc clamped(const TSrc& src)
  {
    return src;
  }
};

template <typename TTrg, typename TSrc>
TTrg castClamped(const TSrc& src)
{
  return NumericCast<TTrg, TSrc>::clamped(src);
}

} // namespace visionary
