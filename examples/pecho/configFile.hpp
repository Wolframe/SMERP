#ifndef _CONFIGFILE_HPP_INCLUDED
#define _CONFIGFILE_HPP_INCLUDED

#include "logLevel.hpp"
#include "logSyslogFacility.hpp"
#include "serverEndpoint.hpp"

#include <string>
#include <vector>

namespace _SMERP	{

	struct CfgFileConfig	{
		std::string	file;
// daemon configuration
		std::string	user;
		std::string	group;
		std::string	pidFile;

// service configuration
		std::string	serviceName;
		std::string	serviceDisplayName;
		std::string	serviceDescription;

// server configuration
		unsigned short	threads;
		unsigned short	maxConnections;
// network configuration
		std::vector<Network::ServerTCPendpoint> address;
		std::vector<Network::ServerSSLendpoint> SSLaddress;

		unsigned	idleTimeout;

// logger configuration
		bool		logToStderr;
		LogLevel::Level	stderrLogLevel;
		bool		logToFile;
		std::string	logFile;
		LogLevel::Level	logFileLogLevel;
		bool		logToSyslog;
		SyslogFacility::Facility syslogFacility;
		LogLevel::Level	syslogLogLevel;
		std::string	syslogIdent;
		bool		logToEventlog;
		std::string	eventlogLogName;
		std::string	eventlogSource;
		LogLevel::Level	eventlogLogLevel;

	private:
		std::string	errMsg_;

	public:
		static const char* chooseFile( const char *globalFile,
						const char *userFile,
						const char *localFile );

		bool parse( const char *filename );
		std::string errMsg( void )		{ return errMsg_; }
	};

} // namespace _SMERP

#endif // _CONFIGFILE_HPP_INCLUDED
