//
// echoHandler.hpp - simple echo handler example
//

#ifndef _ECHO_HANDLER_HPP_INCLUDED
#define _ECHO_HANDLER_HPP_INCLUDED

#include "connectionHandler.hpp"

namespace _SMERP {
	/// The connection handler
	class echoConnection : public Network::connectionHandler
	{
	public:
		echoConnection( const Network::LocalTCPendpoint& local );
		echoConnection( const Network::LocalSSLendpoint& local );
		~echoConnection();

		void setPeer( const Network::RemoteTCPendpoint& remote );
		void setPeer( const Network::RemoteSSLendpoint& remote );

		/// Parse incoming data. The return value indicates how much of the
		/// input has been consumed.
		void networkInput( const void *begin, std::size_t bytesTransferred );

		void timeoutOccured();
		void signalOccured();
		void errorOccured( NetworkSignal );

		/// Handle a request and produce a reply.
		const Network::NetworkOperation nextOperation();

	private:
		enum State	{
			NEW,
			HELLO_SENT,
			READ_INPUT,
			OUTPUT_MSG,
			TIMEOUT,
			SIGNALLED,
			TERMINATE,
			FINISHED
		};

		static const std::size_t ReadBufSize = 8192;
		/// The state of the processor FSM
		State		state_;
		/// Read buffer
		char		readBuf_[ ReadBufSize ];
		char*		dataStart_;
		std::size_t	dataSize_;
		/// Output buffer
		std::string	outMsg_;
		/// Idle timeout value
		unsigned	idleTimeout_;
	};


	/// The server handler container
	class ServerHandler::ServerHandlerImpl
	{
	public:
		Network::connectionHandler* newConnection( const Network::LocalTCPendpoint& local );
		Network::connectionHandler* newSSLconnection( const Network::LocalSSLendpoint& local );
	};

} // namespace _SMERP

#endif // _ECHO_HANDLER_HPP_INCLUDED
