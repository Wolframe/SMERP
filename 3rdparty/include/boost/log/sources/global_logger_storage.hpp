/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   global_logger_storage.hpp
 * \author Andrey Semashev
 * \date   21.04.2008
 *
 * The header contains implementation of facilities to declare global loggers.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_GLOBAL_LOGGER_STORAGE_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_GLOBAL_LOGGER_STORAGE_HPP_INCLUDED_

#include <typeinfo>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/singleton.hpp>
#include <boost/log/detail/visible_type.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sources {

namespace aux {

//! The base class for logger holders
struct BOOST_LOG_NO_VTABLE BOOST_LOG_VISIBLE logger_holder_base
{
    //! The source file name where the logger was registered
    const char* m_RegistrationFile;
    //! The line number where the logger was registered
    unsigned int m_RegistrationLine;

    logger_holder_base(const char* file, unsigned int line) :
        m_RegistrationFile(file),
        m_RegistrationLine(line)
    {
    }
    virtual ~logger_holder_base() {}
    virtual std::type_info const& logger_type() const = 0;
};

//! The actual logger holder class
template< typename LoggerT >
struct BOOST_LOG_VISIBLE logger_holder :
    public logger_holder_base
{
    //! The logger instance
    LoggerT m_Logger;

    logger_holder(const char* file, unsigned int line, LoggerT const& logger) :
        logger_holder_base(file, line),
        m_Logger(logger)
    {
    }
    std::type_info const& logger_type() const { return typeid(LoggerT); }
};

//! The class implements a global repository of tagged loggers
template< typename CharT >
struct global_storage
{
    typedef shared_ptr< logger_holder_base >(*initializer_t)();

    //! Finds or creates the logger and returns its holder
    BOOST_LOG_EXPORT static shared_ptr< logger_holder_base > get_or_init(
        std::type_info const& key,
        initializer_t initializer);

private:
    //  Non-constructible, non-copyable, non-assignable
    global_storage();
    global_storage(global_storage const&);
    global_storage& operator= (global_storage const&);
};

//! Throws the \c odr_violation exception
BOOST_LOG_EXPORT BOOST_LOG_NORETURN void throw_odr_violation(
    std::type_info const& tag_type,
    std::type_info const& logger_type,
    logger_holder_base const& registered);

//! The class implements a logger singleton
template< typename TagT >
struct logger_singleton :
    public boost::log::aux::lazy_singleton<
        logger_singleton< TagT >,
        shared_ptr< logger_holder< typename TagT::logger_type > >
    >
{
    //! Base type
    typedef boost::log::aux::lazy_singleton<
        logger_singleton< TagT >,
        shared_ptr< logger_holder< typename TagT::logger_type > >
    > base_type;
    //! Logger type
    typedef typename TagT::logger_type logger_type;

    //! Returns the logger instance
    static logger_type& get()
    {
        return base_type::get()->m_Logger;
    }

    //! Initializes the logger instance (called only once)
    static void init_instance()
    {
        typedef global_storage< typename logger_type::char_type > global_storage_t;
        shared_ptr< logger_holder< logger_type > >& instance = base_type::get_instance();
        shared_ptr< logger_holder_base > holder =
            global_storage_t::get_or_init(
                typeid(boost::log::aux::visible_type< TagT >),
                &logger_singleton::construct_logger);
        instance = boost::dynamic_pointer_cast< logger_holder< logger_type > >(holder);
        if (!instance)
        {
            // In pure C++ this should never happen, since there cannot be two
            // different tag types that have equal type_infos. In real life it can
            // happen if the same-named tag is defined differently in two or more
            // dlls. This check is intended to detect such ODR violations. However, there
            // is no protection against different definitions of the logger type itself.
            throw_odr_violation(typeid(TagT), typeid(logger_type), *holder);
        }
    }

private:
    //! Constructs a logger holder
    static shared_ptr< logger_holder_base > construct_logger()
    {
        return boost::make_shared< logger_holder< logger_type > >(
            TagT::registration_file(),
            static_cast< unsigned int >(TagT::registration_line),
            TagT::construct_logger());
    }
};

} // namespace aux

//! The macro forward-declares a global logger with a custom initialization
#define BOOST_LOG_GLOBAL_LOGGER(tag_name, logger)\
    struct tag_name\
    {\
        typedef logger logger_type;\
        enum registration_line_t { registration_line = __LINE__ };\
        static const char* registration_file() { return __FILE__; }\
        static logger_type construct_logger();\
        static inline logger_type& get()\
        {\
            return ::boost::log::sources::aux::logger_singleton< tag_name >::get();\
        }\
    };

//! The macro defines a global logger initialization routine
#define BOOST_LOG_GLOBAL_LOGGER_INIT(tag_name, logger)\
    tag_name::logger_type tag_name::construct_logger()

//! The macro defines a global logger initializer that will default-construct the logger
#define BOOST_LOG_GLOBAL_LOGGER_DEFAULT(tag_name, logger)\
    BOOST_LOG_GLOBAL_LOGGER_INIT\
    {\
        return logger_type();\
    }

//! The macro defines a global logger initializer that will construct the logger with the specified constructor arguments
#define BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(tag_name, logger, args)\
    BOOST_LOG_GLOBAL_LOGGER_INIT\
    {\
        return logger_type(BOOST_PP_SEQ_ENUM(args));\
    }

//! The macro declares a global logger with a custom initialization
#define BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(tag_name, logger)\
    BOOST_LOG_GLOBAL_LOGGER(tag_name, logger)\
    inline BOOST_LOG_GLOBAL_LOGGER_INIT(tag_name, logger)

//! The macro declares a global logger that will be default-constructed
#define BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(tag_name, logger)\
    BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(tag_name, logger)\
    {\
        return logger_type();\
    }

//! The macro declares a global logger that will be constructed with the specified arguments
#define BOOST_LOG_INLINE_GLOBAL_LOGGER_CTOR_ARGS(tag_name, logger, args)\
    BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(tag_name, logger)\
    {\
        return logger_type(BOOST_PP_SEQ_ENUM(args));\
    }

} // namespace sources

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SOURCES_GLOBAL_LOGGER_STORAGE_HPP_INCLUDED_