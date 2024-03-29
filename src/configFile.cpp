//
// configFile.cpp
//

#include "configFile.hpp"
#include "serverEndpoint.hpp"

#include "miscUtils.hpp"

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/property_tree/ptree.hpp>

#include <boost/property_tree/info_parser.hpp>

#include <vector>
#include <string>


static const unsigned short	DEFAULT_PORT = 7660;
static const unsigned short	SSL_DEFAULT_PORT = 7960;

static const char*		DEFAULT_SERVICE_NAME = "smerp";
static const char*		DEFAULT_SERVICE_DISPLAY_NAME = "Smerp Daemon";
static const char*		DEFAULT_SERVICE_DESCRIPTION = "a daemon for smerping";


static boost::logic::tribool getBoolValue( boost::property_tree::ptree& pt, const std::string& label, std::string& val )
{
	std::string s = pt.get<std::string>( label, std::string() );
	val = s;
	boost::to_upper( s );
	boost::trim( s );
	if ( s == "NO" || s == "FALSE" || s == "0" || s == "OFF" )
		return false;
	if ( s == "YES" || s == "TRUE" || s == "1" || s == "ON" )
		return true;
	return boost::logic::indeterminate;
}


namespace _SMERP {

	const char* CfgFileConfig::chooseFile( const char *globalFile, const char *userFile, const char *localFile )
	{
		if ( globalFile != NULL )
			if ( boost::filesystem::exists( globalFile ))
				return globalFile;
		if ( userFile != NULL )
			if ( boost::filesystem::exists( userFile ))
				return userFile;
		if ( localFile != NULL )
			if ( boost::filesystem::exists( localFile ))
				return localFile;
		return NULL;
	}


	bool CfgFileConfig::parse ( const char *filename )
	{
		unsigned short	port;

		file = resolvePath( boost::filesystem::absolute( filename ).string() );
		if ( !boost::filesystem::exists( file ))	{
			errMsg_ = "Configuration file ";
			errMsg_ += file;
			errMsg_ += " does not exist.";
			return false;
		}

		// Create an empty property tree object
		boost::property_tree::ptree	pt;
		try	{

		read_info( filename, pt );

		BOOST_FOREACH( boost::property_tree::ptree::value_type &v, pt.get_child( "server.listen" ))	{
			std::string hostStr = v.second.get<std::string>( "address", std::string() );
			if ( hostStr.empty() )	{
				errMsg_ = "Interface must be defined";
				return false;
			}
			if ( hostStr == "*" )
				hostStr = "0.0.0.0";
			std::string portStr = v.second.get<std::string>( "port", std::string() );
			if ( portStr.empty() )	{
				if ( v.first == "socket" )
					port = DEFAULT_PORT;
				else
					port = SSL_DEFAULT_PORT;
			}
			else	{
				try	{
					port = boost::lexical_cast<unsigned short>( portStr );
				}
				catch( boost::bad_lexical_cast& )	{
					errMsg_ = "Invalid value for port: ";
					errMsg_ += portStr;
					return false;
				}
				if ( port == 0 )	{
					errMsg_ = "Port out of range: ";
					errMsg_ += portStr;
					return false;
				}
			}

			unsigned maxConn = v.second.get<unsigned>( "maxConnections", 0 );

			if ( v.first == "socket" )	{
				Network::ServerTCPendpoint lep( hostStr, port, maxConn );
				address.push_back( lep );
			}
			else if ( v.first == "SSLsocket" )	{
// get SSL certificate / CA param
				std::string certFile = boost::filesystem::absolute(
									v.second.get<std::string>( "certificate", std::string() ),
									boost::filesystem::path( file ).branch_path() ).string();
				std::string keyFile = boost::filesystem::absolute(
									v.second.get<std::string>( "key", std::string() ),
									boost::filesystem::path( file ).branch_path() ).string();
				std::string CAdirectory = boost::filesystem::absolute(
									v.second.get<std::string>( "CAdirectory", std::string() ),
									boost::filesystem::path( file ).branch_path() ).string();
				std::string CAchainFile = boost::filesystem::absolute(
									v.second.get<std::string>( "CAchainFile", std::string() ),
									boost::filesystem::path( file ).branch_path() ).string();

				std::string tmpStr;
				boost::logic::tribool flag = getBoolValue( v.second, "verify", tmpStr );
				bool verify;
				if ( flag )
					verify = true;
				else if ( !flag )
					verify = false;
				else	{
					verify = true;
					errMsg_ = "Unknown value \"";
					errMsg_ += tmpStr;
					errMsg_ += "\" for SSL verify client. WARNING: enabling verification";
				}
				Network::ServerSSLendpoint lep( hostStr, port, maxConn,
								certFile, keyFile,
								verify, CAdirectory, CAchainFile );
				SSLaddress.push_back( lep );
			}
			else	{
				errMsg_ = "Invalid listen type: ";
				errMsg_ += v.first;
				return false;
			}
		}

		threads = pt.get<unsigned short>( "server.threads", 4 );
		maxConnections = pt.get<unsigned short>( "server.maxConnections", 256 );

		user = pt.get<std::string>( "server.daemon.user", std::string() );
		group = pt.get<std::string>( "server.daemon.group", std::string() );
		pidFile = pt.get<std::string>( "server.daemon.pidFile", std::string( ) );

		serviceName = pt.get<std::string>( "server.service.name", DEFAULT_SERVICE_NAME );
		serviceDisplayName = pt.get<std::string>( "server.service.displayName", DEFAULT_SERVICE_DISPLAY_NAME );
		serviceDescription = pt.get<std::string>( "server.service.description", DEFAULT_SERVICE_DESCRIPTION );

		idleTimeout = pt.get<unsigned>( "server.timeout.idle", 900 );
		requestTimeout = pt.get<unsigned>( "server.timeout.request", 30 );
		answerTimeout = pt.get<unsigned>( "server.timeout.answer", 30 );
		processTimeout = pt.get<unsigned>( "server.timeout.process", 30 );

// database
		dbHost = pt.get<std::string>( "database.host", std::string() );
		dbPort = pt.get<unsigned short>( "database.port", 0 );
		dbName = pt.get<std::string>( "database.name", std::string() );
		dbUser = pt.get<std::string>( "database.user", std::string() );
		dbPassword = pt.get<std::string>( "database.password", std::string() );

		if ( pt.get_child_optional( "logging.stderr" ))	{
			logToStderr = true;
			std::string str = pt.get<std::string>( "logging.stderr.level", "NOTICE" );
			std::string s = str;
			boost::trim( s );
			boost::to_upper( s );

			if ( ( stderrLogLevel = LogLevel::str2LogLevel( s )) == LogLevel::LOGLEVEL_UNDEFINED )	{
				errMsg_ = "unknown log level \"";
				errMsg_ += str;
				errMsg_ += " for stderr";
				return false;
			}
		}
		else
			logToStderr = false;

		if ( pt.get_child_optional( "logging.logFile" ))	{
			logToFile = true;
			logFile = boost::filesystem::absolute(
						pt.get<std::string>( "logging.logFile.filename", std::string() ),
							boost::filesystem::path( file ).branch_path() ).string();
			std::string str = pt.get<std::string>( "logging.logFile.level", "ERROR" );
			std::string s = str;
			boost::trim( s );
			boost::to_upper( s );

			if ( ( logFileLogLevel = LogLevel::str2LogLevel( s )) == LogLevel::LOGLEVEL_UNDEFINED )	{
				errMsg_ = "unknown log level \"";
				errMsg_ += str;
				errMsg_ += " for logfile";
				return false;
			}
		}
		else
			logToFile = false;
#if !defined( _WIN32 )
		if ( pt.get_child_optional( "logging.syslog" ))	{
			logToSyslog = true;
			std::string str = pt.get<std::string>( "logging.syslog.facility", "LOCAL4" );
			std::string s = str;
			boost::trim( s );
			boost::to_upper( s );

			if ( ( syslogFacility = SyslogFacility::str2SyslogFacility( s )) == SyslogFacility::_SMERP_SYSLOG_FACILITY_UNDEFINED )	{
				errMsg_ = "unknown syslog facility \"";
				errMsg_ += str;
				errMsg_ += "\"";
				return false;
			}
			str = pt.get<std::string>( "logging.syslog.level", "NOTICE" );
			s = str;
			boost::trim( s );
			boost::to_upper( s );

			if ( ( syslogLogLevel = LogLevel::str2LogLevel( s )) == LogLevel::LOGLEVEL_UNDEFINED )	{
				errMsg_ = "unknown log level \"";
				errMsg_ += str;
				errMsg_ += " for syslog";
				return false;
			}

			syslogIdent = pt.get<std::string>( "logging.syslog.ident", "smerpd" );
		}
		else
			logToSyslog = false;
#endif	// !defined( _WIN32 )

#if defined( _WIN32 )
		if ( pt.get_child_optional( "logging.eventlog" )) {
			logToEventlog = true;
			eventlogLogName = pt.get<std::string>( "logging.eventlog.name", "smerpd" );
			eventlogSource = pt.get<std::string>( "logging.eventlog.source", "unknown" );
			std::string str = pt.get<std::string>( "logging.eventlog.level", "NOTICE" );
			std::string s = str;
			boost::trim( s );
			boost::to_upper( s );

			if ( ( eventlogLogLevel = LogLevel::str2LogLevel( s )) == LogLevel::LOGLEVEL_UNDEFINED )	{
				errMsg_ = "unknown log level \"";
				errMsg_ += s;
				errMsg_ += " for Event Log";
				return false;
			}
		}
		else
			logToEventlog = false;
#endif	// defined( _WIN32 )
	}
		catch( std::exception& e)	{
			errMsg_ = e.what();
			return false;
		}

		return true;
	}

} // namespace _SMERP
