/*
 * (C) 2009 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/libs/log/doc/log.html.
 */
/*!
 * \file   keywords/custom_severity_mapping.hpp
 * \author Andrey Semashev
 * \date   14.03.2009
 *
 * The header contains the \c custom_severity_mapping keyword declaration.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_KEYWORDS_CUSTOM_SEVERITY_MAPPING_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_CUSTOM_SEVERITY_MAPPING_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace keywords {

    //! The keyword for passing custom severity type flag to a syslog sink backend initialization
    BOOST_PARAMETER_KEYWORD(tag, custom_severity_mapping)

} // namespace keywords

} // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_CUSTOM_SEVERITY_MAPPING_HPP_INCLUDED_
