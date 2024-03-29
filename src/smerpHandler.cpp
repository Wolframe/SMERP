//
// connectionHandler.cpp
//

#include "smerpHandler.hpp"
#include "logger.hpp"

#include <string>
#include <cstring>

using namespace _SMERP;

smerpConnection::smerpConnection( const Network::LocalTCPendpoint& local )
{
	LOG_TRACE << "Created connection handler for " << local.toString();
	state_ = NEW;
	dataStart_ = NULL;
	dataSize_ = 0;
	idleTimeout_ = 30;
}


smerpConnection::smerpConnection( const Network::LocalSSLendpoint& local )
{
	LOG_TRACE << "Created connection handler (SSL) for " << local.toString();
	state_ = NEW;
	dataStart_ = NULL;
	dataSize_ = 0;
	idleTimeout_ = 30;
}

smerpConnection::~smerpConnection()
{
	LOG_TRACE << "Connection handler destroyed";
}

void smerpConnection::setPeer( const Network::RemoteTCPendpoint& remote )
{
	LOG_TRACE << "Peer set to " << remote.toString();
}

void smerpConnection::setPeer( const Network::RemoteSSLendpoint& remote )
{
	LOG_TRACE << "Peer set to " << remote.toString();
	LOG_TRACE << "Peer Common Name: " << remote.commonName();
}


/// Handle a request and produce a reply.
const Network::NetworkOperation smerpConnection::nextOperation()
{
	switch( state_ )	{
	case NEW:	{
			state_ = HELLO_SENT;
			return Network::NetworkOperation( Network::SendString( "Welcome to SMERP.\n" ));
		}

	case HELLO_SENT:	{
			state_ = READ_INPUT;
			return Network::NetworkOperation( Network::ReadData( readBuf_, ReadBufSize, idleTimeout_ ));
		}

	case READ_INPUT:
		dataStart_ = readBuf_;
		// Yes, it continues with OUTPUT_MSG, sneaky, sneaky, sneaky :P

	case OUTPUT_MSG:
		if ( !strncmp( "quit", dataStart_, 4 ))	{
			state_ = TERMINATE;
			return Network::NetworkOperation( Network::SendString( "Thanks for using SMERP.\n" ));
		}
		else	{
			char *s = dataStart_;
			for ( std::size_t i = 0; i < dataSize_; i++ )	{
				if ( *s == '\n' )	{
					s++;
					outMsg_ = std::string( dataStart_, s - dataStart_ );
					dataSize_ -= s - dataStart_;
					dataStart_ = s;
					state_ = OUTPUT_MSG;
					return Network::NetworkOperation( Network::SendString( outMsg_ ));
				}
				s++;
			}
			// If we got here, no \n was found, we need to read more
			// or close the connection if the buffer is full
			if ( dataSize_ >= ReadBufSize )	{
				state_ = TERMINATE;
				return Network::NetworkOperation( Network::SendString( "Line too long. Bye.\n" ));
			}
			else {
				memmove( readBuf_, dataStart_, dataSize_ );
				state_ = READ_INPUT;
				return Network::NetworkOperation( Network::ReadData( readBuf_ + dataSize_,
										     ReadBufSize - dataSize_,
										     idleTimeout_ ));
			}
		}

		case TIMEOUT:	{
				state_ = TERMINATE;
				return Network::NetworkOperation( Network::SendString( "Timeout. :P\n" ));
			}

		case SIGNALLED:	{
				state_ = TERMINATE;
				return Network::NetworkOperation( Network::SendString( "Server is shutting down. :P\n" ));
			}

		case TERMINATE:	{
				state_ = FINISHED;
				return Network::NetworkOperation( Network::CloseConnection() );
			}

		case FINISHED:
			LOG_DEBUG << "Processor in FINISHED state";
			break;

		} /* switch( state_ ) */
	return Network::NetworkOperation( Network::CloseConnection() );
}


/// Parse incoming data. The return value indicates how much of the
/// input has been consumed.
void smerpConnection::networkInput( const void*, std::size_t bytesTransferred )
{
	LOG_DATA << "network Input: Read " << bytesTransferred << " bytes";
	dataSize_ += bytesTransferred;
}

void smerpConnection::timeoutOccured()
{
	state_ = TIMEOUT;
	LOG_TRACE << "Processor received timeout";
}

void smerpConnection::signalOccured()
{
	state_ = SIGNALLED;
	LOG_TRACE << "Processor received signal";
}

void smerpConnection::errorOccured( NetworkSignal signal )
{
	switch( signal )	{
	case END_OF_FILE:
		LOG_TRACE << "Processor received EOF (read on closed connection)";
		break;

	case BROKEN_PIPE:
		LOG_TRACE << "Processor received BROKEN PIPE (write on closed connection)";
		break;

	case OPERATION_CANCELLED:
		LOG_TRACE << "Processor received OPERATION_CANCELED (should have been requested by us)";
		break;

	case UNKNOWN_ERROR:
		LOG_TRACE << "Processor received an UNKNOWN error from the framework";
		break;
	}
	state_ = TERMINATE;
}


/// ServerHandler PIMPL
Network::connectionHandler* ServerHandler::ServerHandlerImpl::newConnection( const Network::LocalTCPendpoint& local )
{
	return new smerpConnection( local );
}

Network::connectionHandler* ServerHandler::ServerHandlerImpl::newSSLconnection( const Network::LocalSSLendpoint& local )
{
	return new smerpConnection( local );
}


ServerHandler::ServerHandler( const HandlerConfiguration& ) : impl_( new ServerHandlerImpl )	{}

ServerHandler::~ServerHandler()	{ delete impl_; }

Network::connectionHandler* ServerHandler::newConnection( const Network::LocalTCPendpoint& local )
{
	return impl_->newConnection( local );
}

Network::connectionHandler* ServerHandler::newSSLconnection( const Network::LocalSSLendpoint& local )
{
	return impl_->newSSLconnection( local );
}
