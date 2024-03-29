//
// appConfig.cpp
//

#include "appConfig.hpp"
#include "commandLine.hpp"
#include "configFile.hpp"

#include <string>
#include <ostream>

namespace _SMERP {

	ApplicationConfiguration::ApplicationConfiguration( const CmdLineConfig& cmdLine, const CfgFileConfig& cfgFile )
	{
		configFile = cfgFile.file;

#if defined(_WIN32)
		// on Windows the user should either use -f or start in the console
		// (assuming an implicit -f) or the service option should contain
		// --service in the startup parameters of the service impling non-foreground
		if( cmdLine.command == _SMERP::CmdLineConfig::RUN_SERVICE )
			foreground = false;
		else
			foreground = true;
#else
		foreground = cmdLine.foreground;
#endif

#if !defined(_WIN32)
		if ( !cmdLine.user.empty())
			user = cmdLine.user;
		else
			user = cfgFile.user;

		if ( !cmdLine.group.empty())
			group = cmdLine.group;
		else
			group = cfgFile.group;
#endif

		pidFile = cfgFile.pidFile;
		serviceName = cfgFile.serviceName;
		serviceDisplayName = cfgFile.serviceDisplayName;
		serviceDescription = cfgFile.serviceDescription;

		threads = cfgFile.threads;
		maxConnections = cfgFile.maxConnections;

		address = cfgFile.address;
		SSLaddress = cfgFile.SSLaddress;

		idleTimeout = cfgFile.idleTimeout;
		requestTimeout = cfgFile.requestTimeout;
		answerTimeout = cfgFile.answerTimeout;
		processTimeout = cfgFile.processTimeout;

		dbHost = cfgFile.dbHost;
		dbPort = cfgFile.dbPort;
		dbName = cfgFile.dbName;
		dbUser = cfgFile.dbUser;
		dbPassword = cfgFile.dbPassword;

		if ( !foreground )	{
			logToStderr = cfgFile.logToStderr;
			stderrLogLevel = cfgFile.stderrLogLevel;
			logToFile = cfgFile.logToFile;
			logFile = cfgFile.logFile;
			logFileLogLevel = cfgFile.logFileLogLevel;
#if !defined( _WIN32 )
			logToSyslog = cfgFile.logToSyslog;
			syslogFacility = cfgFile.syslogFacility;
			syslogLogLevel = cfgFile.syslogLogLevel;
#endif	// !defined( _WIN32 )
#if defined( _WIN32 )
// Aba, what for?
//			syslogIdent = cfgFile.syslogIdent;
//			logToEventlog = cfgFile.logToEventlog;
#endif	//defined( _WIN32 )
		}
		else	{
			logToStderr = true;
			stderrLogLevel = cmdLine.debugLevel;
			logToFile = false;
#if !defined( _WIN32 )
			logToSyslog = false;
#endif	// !defined( _WIN32 )
#if defined( _WIN32 )
			logToEventlog = false;
#endif	// defined( _WIN32 )
		}

#if defined( _WIN32 )
		// we need the data always (for running, --install and --remove,
		// which run in foreground)
		eventlogLogName = cfgFile.eventlogLogName;
		eventlogSource = cfgFile.eventlogSource;
		eventlogLogLevel = cfgFile.eventlogLogLevel;
#endif	// defined( _WIN32 )
	}


///********************************************************************************************************

// Server configuration functions
	void ServerConfiguration::print( std::ostream& os ) const
	{
		// Unix daemon
#if !defined(_WIN32)
		os << "Run as " << (user.empty() ? "(not specified)" : user) << ":"
				<< (group.empty() ? "(not specified)" : group) << std::endl;
		os << "PID file: " << pidFile << std::endl;
#else
		// Windows service
		os << "When run as service" << std::endl
				<< "  Name: " << serviceName << std::endl
				<< "  Displayed name: " << serviceDisplayName << std::endl
				<< "  Description: " << serviceDescription << std::endl;
#endif
		os << "Number of threads: " << threads << std::endl;
		os << "Maximum number of connections: " << maxConnections << std::endl;

		os << "Network" << std::endl;
		if ( address.size() > 0 )	{
			std::list<Network::ServerTCPendpoint>::const_iterator it = address.begin();
			os << "  Unencrypted: " << it->toString() << std::endl;
			for ( ++it; it != address.end(); ++it )
				os << "               " << it->toString() << std::endl;
		}
		if ( SSLaddress.size() > 0 )	{
			std::list<Network::ServerSSLendpoint>::const_iterator it = SSLaddress.begin();
			os << "          SSL: " << it->toString() << std::endl;
			os << "                  certificate: " << (it->certificate().empty() ? "(none)" : it->certificate()) << std::endl;
			os << "                  key file: " << (it->key().empty() ? "(none)" : it->key()) << std::endl;
			os << "                  CA directory: " << (it->CAdirectory().empty() ? "(none)" : it->CAdirectory()) << std::endl;
			os << "                  CA chain file: " << (it->CAchain().empty() ? "(none)" : it->CAchain()) << std::endl;
			os << "                  verify client: " << (it->verifyClientCert() ? "yes" : "no") << std::endl;
			for ( ++it; it != SSLaddress.end(); ++it )	{
				os << "               " << it->toString() << std::endl;
				os << "                  certificate: " << (it->certificate().empty() ? "(none)" : it->certificate()) << std::endl;
				os << "                  key file: " << (it->key().empty() ? "(none)" : it->key()) << std::endl;
				os << "                  CA directory: " << (it->CAdirectory().empty() ? "(none)" : it->CAdirectory()) << std::endl;
				os << "                  CA chain file: " << (it->CAchain().empty() ? "(none)" : it->CAchain()) << std::endl;
				os << "                  verify client: " << (it->verifyClientCert() ? "yes" : "no") << std::endl;
			}
		}
	}

	/// Check if the server configuration makes sense
	///
	/// Be aware that this function does NOT test if the configuration
	/// can be used. It only tests if it MAY be valid.
	bool ServerConfiguration::check( std::ostream& os ) const
	{
		bool	correct = true;

		for ( std::list<Network::ServerSSLendpoint>::const_iterator it = SSLaddress.begin();
									it != SSLaddress.end(); ++it )	{
			// if it listens to SSL a certificate file and a key file are required
			if ( it->certificate().empty() )	{
				os << "No SSL certificate specified for " << it->toString() << std::endl;
				correct = false;
			}
			if ( it->key().empty() )	{
				os << "No SSL key specified for " << it->toString() << std::endl;
				correct = false;
			}
			// verify client SSL certificate needs either certificate dir or chain file
			if ( it->verifyClientCert() && it->CAdirectory().empty() && it->CAchain().empty() )	{
				os << "Client SSL certificate verification requested but no CA "
						"directory or CA chain file specified for "
						<< it->toString() << std::endl;
				correct = false;
			}
		}
		return correct;
	}


#if !defined(_WIN32)
	/// Override the server configuration with command line arguments
	void ServerConfiguration::override( std::string usr, std::string grp )
	{
		if ( !usr.empty())
			user = usr;
		if ( !grp.empty())
			group = grp;
	}
#endif

// Logger configuration functions
	void LoggerConfiguration::print( std::ostream& os ) const
	{
		os << "Logging" << std::endl;
		if ( logToStderr )
			os << "   Log to stderr, level " << stderrLogLevel << std::endl;
		else
			os << "   Log to stderr: DISABLED" << std::endl;
		if ( logToFile )
			os << "   Log to file: " << logFile << ", level " << logFileLogLevel << std::endl;
		else
			os << "   Log to file: DISABLED" << std::endl;

#if !defined(_WIN32)
		if ( logToSyslog )
			os << "   Log to syslog: facility " << syslogFacility << ", level " << syslogLogLevel << std::endl;
		else
			os << "   Log to syslog: DISABLED" << std::endl;
#endif	// !defined( _WIN32 )

#if defined(_WIN32)
		if ( logToEventlog )
			os << "   Log to eventlog: name " << eventlogLogName << ", source " << eventlogSource << ", level " << eventlogLogLevel;
		else
			os << "   Log to eventlog: DISABLED" << std::endl;
#endif	// defined( _WIN32 )
	}

	/// Check if the logger configuration makes sense
	///
	/// Be aware that this function does NOT test if the configuration
	/// can be used. It only tests if it MAY be valid.
	bool LoggerConfiguration::check( std::ostream& os ) const
	{
		// if log to file is requested then a file must be specified
		if ( logToFile && logFile.empty() )	{
			os << "Log to file requested but no log file specified";
			return false;
		}
		return true;
	}

///********************************************************************************************************


	void ApplicationConfiguration::print( std::ostream& os ) const
	{

		os << "Configuration file: " << configFile << std::endl;
// Unix daemon
#if !defined(_WIN32)
		os << "Run in foreground: " << (foreground ? "yes" : "no") << std::endl;
		os << "Run as " << (user.empty() ? "(not specified)" : user) << ":"
				<< (group.empty() ? "(not specified)" : group) << std::endl;
		os << "PID file: " << pidFile << std::endl;
#else
// Windows service
		os << "When run as service" << std::endl
			<< "  Name: " << serviceName << std::endl
			<< "  Displayed name: " << serviceDisplayName << std::endl
			<< "  Description: " << serviceDescription << std::endl;
#endif
		os << "Number of threads: " << threads << std::endl;
		os << "Maximum number of connections: " << maxConnections << std::endl;

		os << "Network" << std::endl;
		if ( address.size() > 0 )	{
			std::list<Network::ServerTCPendpoint>::const_iterator it = address.begin();
			os << "  Unencrypted: " << it->toString() << std::endl;
			for ( ++it; it != address.end(); ++it )
				os << "               " << it->toString() << std::endl;
		}
		if ( SSLaddress.size() > 0 )	{
			std::list<Network::ServerSSLendpoint>::const_iterator it = SSLaddress.begin();
			os << "          SSL: " << it->toString() << std::endl;
			os << "                  certificate: " << (it->certificate().empty() ? "(none)" : it->certificate()) << std::endl;
			os << "                  key file: " << (it->key().empty() ? "(none)" : it->key()) << std::endl;
			os << "                  CA directory: " << (it->CAdirectory().empty() ? "(none)" : it->CAdirectory()) << std::endl;
			os << "                  CA chain file: " << (it->CAchain().empty() ? "(none)" : it->CAchain()) << std::endl;
			os << "                  verify client: " << (it->verifyClientCert() ? "yes" : "no") << std::endl;
			for ( ++it; it != SSLaddress.end(); ++it )	{
				os << "               " << it->toString() << std::endl;
				os << "                  certificate: " << (it->certificate().empty() ? "(none)" : it->certificate()) << std::endl;
				os << "                  key file: " << (it->key().empty() ? "(none)" : it->key()) << std::endl;
				os << "                  CA directory: " << (it->CAdirectory().empty() ? "(none)" : it->CAdirectory()) << std::endl;
				os << "                  CA chain file: " << (it->CAchain().empty() ? "(none)" : it->CAchain()) << std::endl;
				os << "                  verify client: " << (it->verifyClientCert() ? "yes" : "no") << std::endl;
			}
		}

		os << "Timeouts" << std::endl;
		os << "   idle: " << idleTimeout << std::endl;
		os << "   request: " << requestTimeout << std::endl;
		os << "   answer: " << answerTimeout << std::endl;
		os << "   process: " << processTimeout << std::endl;

		os << "Database" << std::endl;
		if ( dbHost.empty())
			os << "   DB host: local unix domain socket" << std::endl;
		else
			os << "   DB host: " << dbHost << ":" << dbPort << std::endl;
		os << "   DB name: " << (dbName.empty() ? "(not specified - server user default)" : dbName) << std::endl;
		os << "   DB user / password: " << (dbUser.empty() ? "(not specified - same as server user)" : dbUser) << " / "
						<< (dbPassword.empty() ? "(not specified - no password used)" : dbPassword) << std::endl;

		os << "Logging" << std::endl;
		if ( logToStderr )
			os << "   Log to stderr, level " << stderrLogLevel << std::endl;
		else
			os << "   Log to stderr: DISABLED" << std::endl;
		if ( logToFile )	{
			os << "   Log to file: " << logFile << ", level " << logFileLogLevel << std::endl;
		}
		else
			os << "   Log to file: DISABLED" << std::endl;

#if !defined(_WIN32)
		if ( logToSyslog )
			os << "   Log to syslog: facility " << syslogFacility << ", level " << syslogLogLevel << std::endl;
		else
			os << "   Log to syslog: DISABLED" << std::endl;
#endif	// !defined( _WIN32 )

#if defined(_WIN32)
		if ( logToEventlog )
			os << "   Log to eventlog: name " << eventlogLogName << ", source " << eventlogSource << ", level " << eventlogLogLevel;
		else
			os << "   Log to eventlog: DISABLED" << std::endl;
#endif	// defined( _WIN32 )
	}

	/// Check if the application configuration makes sense
	///
	/// Be aware that this function does NOT test if the configuration
	/// can be used. It only tests if it MAY be valid.
	bool ApplicationConfiguration::check()
	{
		// if log to file is requested a file must be specified
		if ( logToFile )
			if ( logFile.empty())	{
				errMsg_ = "Log to file requested but no log file specified";
				return false;
			}
		return true;
	}

} // namespace _SMERP
