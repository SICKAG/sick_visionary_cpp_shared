
#ifndef BOOST_MPL_AUX_EMPTY_IMPL_HPP_INCLUDED
#define BOOST_MPL_AUX_EMPTY_IMPL_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2000-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: empty_impl.hpp 13472 2017-08-22 07:53:44Z richean $
// $Date: 2017-08-22 09:53:44 +0200 (Di, 22 Aug 2017) $
// $Revision: 13472 $

#include <boost/mpl/empty_fwd.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/aux_/traits_lambda_spec.hpp>
#include <boost/type_traits/is_same.hpp>

namespace boost { namespace mpl {

// default implementation; conrete sequences might override it by 
// specializing either the 'empty_impl' or the primary 'empty' template

template< typename Tag >
struct empty_impl
{
    template< typename Sequence > struct apply
        : is_same<
              typename begin<Sequence>::type
            , typename end<Sequence>::type
            >
    {
    };
};

BOOST_MPL_ALGORITM_TRAITS_LAMBDA_SPEC(1,empty_impl)

}}

#endif // BOOST_MPL_AUX_EMPTY_IMPL_HPP_INCLUDED
