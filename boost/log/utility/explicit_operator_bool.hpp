/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   explicit_operator_bool.hpp
 * \author Andrey Semashev
 * \date   08.03.2009
 *
 * This header defines a compatibility macro that implements an unspecified
 * \c bool operator idiom, which is superceded with explicit conversion operators in
 * C++0x.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_EXPLICIT_OPERATOR_BOOL_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_EXPLICIT_OPERATOR_BOOL_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#if defined(BOOST_LOG_DOXYGEN_PASS) || !defined(BOOST_NO_EXPLICIT_CONVERSION_OPERATORS)

/*!
 * \brief The macro defines an explicit operator of conversion to \c bool
 *
 * The macro should be used inside the definition of a class that has to
 * support the conversion. The class should also implement <tt>operator!</tt>,
 * in terms of which the conversion operator will be implemented.
 */
#define BOOST_LOG_EXPLICIT_OPERATOR_BOOL()\
    explicit operator bool () const\
    {\
        return !this->operator! ();\
    }

#elif !defined(BOOST_LOG_NO_UNSPECIFIED_BOOL)

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    struct unspecified_bool_helper
    {
        static void true_value(unspecified_bool_helper) {}
    };
    typedef void (*unspecified_bool)(unspecified_bool_helper);

} // namespace aux

} // namespace log

} // namespace boost

#define BOOST_LOG_EXPLICIT_OPERATOR_BOOL()\
    operator boost::log::aux::unspecified_bool () const\
    {\
        if (!this->operator!())\
            return &boost::log::aux::unspecified_bool_helper::true_value;\
        else\
            return 0;\
    }

#else

#define BOOST_LOG_EXPLICIT_OPERATOR_BOOL()\
    operator bool () const\
    {\
        return !this->operator! ();\
    }

#endif

#endif // BOOST_LOG_UTILITY_EXPLICIT_OPERATOR_BOOL_HPP_INCLUDED_
