/*!
 * (C) 2007 Andrey Semashev
 *
 * Use, modification and distribution is subject to the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 * \file   text_ostream_backend.cpp
 * \author Andrey Semashev
 * \date   19.04.2007
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#include <vector>
#include <algorithm>
#include <boost/log/sinks/text_ostream_backend.hpp>

#include <boost/log/unused.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

//! Sink implementation
template< typename CharT >
struct basic_text_ostream_backend< CharT >::implementation
{
    //! Type of the container that holds all aggregated streams
    typedef std::vector< shared_ptr< stream_type > > ostream_sequence;

    //! Output stream list
    ostream_sequence m_Streams;
    //! Auto-flush flag
    bool m_fAutoFlush;

    implementation() : m_fAutoFlush(false)
    {
    }
};


//! Constructor
template< typename CharT >
basic_text_ostream_backend< CharT >::basic_text_ostream_backend() : m_pImpl(new implementation())
{
}

//! Destructor (just to make it link from the shared library)
template< typename CharT >
basic_text_ostream_backend< CharT >::~basic_text_ostream_backend()
{
    delete m_pImpl;
}

//! The method adds a new stream to the sink
template< typename CharT >
void basic_text_ostream_backend< CharT >::add_stream(shared_ptr< stream_type > const& strm)
{
    typename implementation::ostream_sequence::iterator it =
        std::find(m_pImpl->m_Streams.begin(), m_pImpl->m_Streams.end(), strm);
    if (it == m_pImpl->m_Streams.end())
    {
        m_pImpl->m_Streams.push_back(strm);
    }
}

//! The method removes a stream from the sink
template< typename CharT >
void basic_text_ostream_backend< CharT >::remove_stream(shared_ptr< stream_type > const& strm)
{
    typename implementation::ostream_sequence::iterator it =
        std::find(m_pImpl->m_Streams.begin(), m_pImpl->m_Streams.end(), strm);
    if (it != m_pImpl->m_Streams.end())
        m_pImpl->m_Streams.erase(it);
}

//! Sets the flag to automatically flush buffers after each logged line
template< typename CharT >
void basic_text_ostream_backend< CharT >::auto_flush(bool f)
{
    m_pImpl->m_fAutoFlush = f;
}

//! The method writes the message to the sink
template< typename CharT >
void basic_text_ostream_backend< CharT >::do_consume(
    record_type const& record UNUSED, target_string_type const& message)
{
    typename string_type::const_pointer const p = message.data();
    typename string_type::size_type const s = message.size();
    typename implementation::ostream_sequence::const_iterator it = m_pImpl->m_Streams.begin();
    for (; it != m_pImpl->m_Streams.end(); ++it)
    {
        register stream_type* const strm = it->get();
        if (strm->good())
        {
            strm->write(p, static_cast< std::streamsize >(s));
            strm->put(static_cast< char_type >('\n'));

            if (m_pImpl->m_fAutoFlush)
                strm->flush();
        }
    }
}

//! Explicitly instantiate sink backend implementation
#ifdef BOOST_LOG_USE_CHAR
template class basic_text_ostream_backend< char >;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
template class basic_text_ostream_backend< wchar_t >;
#endif

} // namespace sinks

} // namespace log

} // namespace boost