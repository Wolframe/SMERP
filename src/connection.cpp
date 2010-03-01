//
// connection.cpp
//

#include "connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "requestHandler.hpp"

namespace _SMERP {

connection::connection(boost::asio::io_service& io_service,
    requestHandler& handler, long timeout_duration_ms)
  : strand_(io_service),
    socket_(io_service),
    requestHandler_(handler),
    timer_(io_service),
    timeout_duration_ms_(timeout_duration_ms)
{
}


boost::asio::ip::tcp::socket& connection::socket()
{
	return socket_;
}


void connection::start()
{
      boost::asio::write(socket_, boost::asio::buffer("Bla bla bla\n"));

  socket_.async_read_some(boost::asio::buffer(buffer_),
      strand_.wrap(
        boost::bind(&connection::handle_read, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred)));

  timer_.expires_from_now(
    boost::posix_time::milliseconds(timeout_duration_ms_));
  timer_.async_wait(
      strand_.wrap(
        boost::bind(&connection::handle_timeout, shared_from_this(),
          boost::asio::placeholders::error)));
}


void connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
  if (!e)
  {
    boost::tribool result;
    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
        request_, buffer_.data(), buffer_.data() + bytes_transferred);

    if (result)
    {
      timer_.cancel();
      timer_.expires_from_now(boost::posix_time::milliseconds(timeout_duration_ms_));
      timer_.async_wait(
          strand_.wrap(
            boost::bind(&connection::handle_timeout, shared_from_this(),
              boost::asio::placeholders::error)));

      requestHandler_.handleRequest(request_, reply_);
      boost::asio::async_write(socket_, reply_.to_buffers(),
          strand_.wrap(
            boost::bind(&connection::handle_write, shared_from_this(),
              boost::asio::placeholders::error)));
    }
    else if (!result)
    {
      timer_.cancel();
      timer_.expires_from_now(boost::posix_time::milliseconds(timeout_duration_ms_));
      timer_.async_wait(
          strand_.wrap(
            boost::bind(&connection::handle_timeout, shared_from_this(),
              boost::asio::placeholders::error)));
/*
      reply_ = reply::stock_reply(reply::bad_request);
      boost::asio::async_write(socket_, reply_.to_buffers(),
          strand_.wrap(
            boost::bind(&connection::handle_write, shared_from_this(),
              boost::asio::placeholders::error)));
*/    }
    else
    {
      socket_.async_read_some(boost::asio::buffer(buffer_),
          strand_.wrap(
            boost::bind(&connection::handle_read, shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred)));
    }
  }

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the connection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The connection class's destructor closes the socket.
}

void connection::handle_write(const boost::system::error_code& e)
{
  if (!e)
  {
    // Cancel timer.
    timer_.cancel();

    // Initiate graceful connection closure.
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  }

  // No new asynchronous operations are started. This means that all shared_ptr
  // references to the connection object will disappear and the object will be
  // destroyed automatically after this handler returns. The connection class's
  // destructor closes the socket.
}


// Initiate graceful connection closure. This will cause all outstanding
// asynchronous read or write operations to be canceled.
void connection::handle_timeout(const boost::system::error_code& e)
{
	if (!e)	{
		boost::asio::write(socket_, boost::asio::buffer("Timeout :P\n"));

		boost::system::error_code ignored_ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
	}
}


} // namespace _SMERP
