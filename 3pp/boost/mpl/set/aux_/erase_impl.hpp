
#ifndef BOOST_MPL_SET_AUX_ERASE_IMPL_HPP_INCLUDED
#define BOOST_MPL_SET_AUX_ERASE_IMPL_HPP_INCLUDED

// Copyright Aleksey Gurtovoy 2003-2004
// Copyright David Abrahams 2003-2004
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Id: erase_impl.hpp 13472 2017-08-22 07:53:44Z richean $
// $Date: 2017-08-22 09:53:44 +0200 (Di, 22 Aug 2017) $
// $Revision: 13472 $

#include <boost/mpl/erase_fwd.hpp>
#include <boost/mpl/set/aux_/erase_key_impl.hpp>
#include <boost/mpl/set/aux_/tag.hpp>

namespace boost { namespace mpl {

template<>
struct erase_impl< aux::set_tag >
{
    template< 
          typename Set
        , typename Pos
        , typename unused_
        > 
    struct apply
        : erase_key_impl<aux::set_tag>
            ::apply<Set,typename Pos::type>
    {
    };
};

}}

#endif // BOOST_MPL_SET_AUX_ERASE_IMPL_HPP_INCLUDED
