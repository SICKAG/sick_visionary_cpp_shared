
#ifndef BOOST_MPL_AUX_CONFIG_GCC_HPP_INCLUDED
#define BOOST_MPL_AUX_CONFIG_GCC_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: gcc.hpp 13472 2017-08-22 07:53:44Z richean $
// $Date: 2017-08-22 09:53:44 +0200 (Di, 22 Aug 2017) $
// $Revision: 13472 $

#if defined(__GNUC__) && !defined(__EDG_VERSION__)
#   define BOOST_MPL_CFG_GCC ((__GNUC__ << 8) | __GNUC_MINOR__)
#else
#   define BOOST_MPL_CFG_GCC 0
#endif

#endif // BOOST_MPL_AUX_CONFIG_GCC_HPP_INCLUDED
