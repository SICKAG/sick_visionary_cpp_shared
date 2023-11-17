
#ifndef BOOST_MPL_AUX_CONFIG_GPU_HPP_INCLUDED
#define BOOST_MPL_AUX_CONFIG_GPU_HPP_INCLUDED

// Copyright Eric Niebler 2014
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: gpu.hpp 13472 2017-08-22 07:53:44Z richean $
// $Date: 2017-08-22 09:53:44 +0200 (Di, 22 Aug 2017) $
// $Revision: 13472 $

#include <boost/config.hpp>

#if !defined(BOOST_MPL_CFG_GPU_ENABLED) \

#   define BOOST_MPL_CFG_GPU_ENABLED BOOST_GPU_ENABLED

#endif

#if defined __CUDACC__

#    define BOOST_MPL_CFG_GPU 1

#else

#    define BOOST_MPL_CFG_GPU 0

#endif

#endif // BOOST_MPL_AUX_CONFIG_GPU_HPP_INCLUDED
