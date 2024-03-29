//
// server.cpp
//

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "server.hpp"
#include "acceptor.hpp"
#include "logger.hpp"
#include "serverEndpoint.hpp"
#include "connectionHandler.hpp"

#include "unused.h"

namespace _SMERP {
	namespace Network	{

		server::server( const std::list<ServerTCPendpoint>& TCPserver,
				SMERP_UNUSED const std::list<ServerSSLendpoint>& SSLserver,
				_SMERP::ServerHandler& serverHandler,
				unsigned threads, unsigned maxConnections )
					: threadPoolSize_( threads ),
					IOservice_(),
					globalList_( maxConnections )
		{
			int i = 0;
			for ( std::list<ServerTCPendpoint>::const_iterator it = TCPserver.begin();
									it != TCPserver.end(); it++ )	{
				acceptor* acptr = new acceptor( IOservice_,
								it->host(), it->port(), it->maxConnections(),
								globalList_,
								serverHandler );
				acceptor_.push_back( acptr );
				i++;
			}
			LOG_DEBUG << i << " network acceptor(s) created.";
#ifdef WITH_SSL
			i = 0;
			for ( std::list<ServerSSLendpoint>::const_iterator it = SSLserver.begin();
									it != SSLserver.end(); it++ )	{
				SSLacceptor* acptr = new SSLacceptor( IOservice_,
								      it->certificate(), it->key(),
								      it->verifyClientCert(),
								      it->CAchain(), it->CAdirectory(),
								      it->host(), it->port(), it->maxConnections(),
								      globalList_,
								      serverHandler );
				SSLacceptor_.push_back( acptr );
				i++;
			}
			LOG_DEBUG << i << " network SSL acceptor(s) created.";
#endif // WITH_SSL
		}

		/// Network server destructor
		/// simply delete the acceptors and remove them from the
		server::~server()
		{
			LOG_TRACE << "Server destructor called";

			std::size_t	i;
			for ( i = 0; i < acceptor_.size(); i++ )
				delete acceptor_[i];
			LOG_TRACE << i << " acceptor(s) deleted";
#ifdef WITH_SSL
			for ( i = 0; i < SSLacceptor_.size(); i++ )
				delete SSLacceptor_[i];
			LOG_TRACE << i << " SSL acceptor(s) deleted";
#endif // WITH_SSL
		}


		void server::run()
		{
			// Create a pool of threads to run all of the io_services.
			std::vector< boost::shared_ptr<boost::thread> >	threads;
			std::size_t					i;

			for ( i = 0; i < threadPoolSize_; i++ )	{
				boost::shared_ptr<boost::thread> thread( new boost::thread( boost::bind( &boost::asio::io_service::run, &IOservice_ )));
				threads.push_back( thread );
			}
			LOG_TRACE << i << " network server thread(s) started";

			// Wait for all threads in the pool to exit.
			for ( i = 0; i < threads.size(); i++ )
				threads[i]->join();

			// Reset io_services.
			IOservice_.reset();
		}

		/// Stop the server i.e. notify acceptors to stop
		void server::stop()
		{
			LOG_DEBUG << "Network server received a shutdown request";

			std::size_t	i;
			for ( i = 0; i < acceptor_.size(); i++ )
				acceptor_[i]->stop();
			LOG_DEBUG << i << " acceptor(s) signaled to stop";
#ifdef WITH_SSL
			for ( i = 0; i < SSLacceptor_.size(); i++ )
				SSLacceptor_[i]->stop();
			LOG_DEBUG << i << " SSL acceptor(s) signaled to stop";
#endif // WITH_SSL
		}


		/// Stop io_services the hard way.
		void server::abort()
		{
			LOG_DEBUG << "Network server received an abort request";
			IOservice_.stop();
		}

	} // namespace Network
} // namespace _SMERP
