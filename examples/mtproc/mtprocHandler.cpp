//
// mtprocHandler.cpp
//
#include "protocol.hpp"
#include "mtprocHandler.hpp"
#include "logger.hpp"
#include "dispatcher.hpp"

using namespace _SMERP;
using namespace _SMERP::mtproc;

struct Connection::Private
{
   //* typedefs for input output blocks and input iterators
   typedef protocol::InputBlock Input;                          //< input buffer type
   typedef protocol::OutputBlock Output;                        //< output buffer type
   typedef protocol::InputBlock::iterator InputIterator;        //< iterator type for protocol commands

   //* typedefs for input output buffers
   typedef protocol::Buffer<128> LineBuffer;                    //< buffer for one line of input/output
   typedef protocol::CmdBuffer CmdBuffer;                       //< buffer for protocol commands
   typedef protocol::CmdParser<CmdBuffer> ProtocolParser;       //< parser for the protocol

   //* typedefs for state variables and buffers
   //list of processor states
   enum State
   {
      Init,
      EnterCommand,
      EmptyLine,
      ProcessingAfterWrite,
      Processing,
      HandleProtocolError,
      HandleCommandError,
      Terminate
   };
   static const char* stateName( State i)
   {
      static const char* ar[] = {"Init","EnterCommand","EmptyLine","ProcessingAfterWrite","Processing","HandleProtocolError","HandleCommandError","Terminate"};
      return ar[i];
   }

   //* all state variables of this processor
   //1. states
   State state;                               //< state of the processor (protocol main statemachine)
   CommandHandler cmdHandler;                 //< state of the processor in the command execution phase (protocol sub statemachine)

   //2. buffers and context
   CmdBuffer cmdBuffer;                       //< context (sub state) for partly parsed protocol commands
   LineBuffer buffer;                         //< context (sub state) for partly parsed input lines
   Input input;                               //< buffer for network read messages
   Output output;                             //< buffer for network write messages
   //3. Iterators
   InputIterator itr;                         //< iterator to scan protocol input
   InputIterator eoM;                         //< iterator pointing to end of message buffer

   //* helper methods for I/O
   //helper function to send a line message with CRLF termination as C string
   Operation WriteLine( const char* str)
   {
      unsigned int ii;
      buffer.init();
      for (ii=0; str[ii]; ii++) buffer.push_back( str[ii]);
      buffer.push_back( '\r');
      buffer.push_back( '\n');
      const char* msg = buffer.c_str();
      buffer.init();
      return Network::SendData( msg, ii+2);
   }

   void networkInput( const void*, std::size_t nofBytes)
   {
       input.setPos( nofBytes);
       itr = input.begin();
       if (state == Processing)
       {
          Input::iterator eoD = input.getEoD( itr);
          eoM = input.end();
          cmdHandler.protocolInput( itr, eoD);
       }
       else
       {
          eoM = input.end();
       }
   }

   void signalTerminate()
   {
        state = Terminate;
   }

   //* interface
   Private( unsigned int inputBufferSize, unsigned int outputBufferSize)   :state(Init),input(inputBufferSize),output(outputBufferSize)
   {
      itr = input.begin();
      eoM = input.end();
   }
   ~Private()  {}

   //statemachine of the processor
   const Operation nextOperation()
   {
      for (;;)
      {
         LOG_DATA << "\nState: " << stateName(state);

         switch( state)
         {
            case Init:
            {
                //start or restart:
                state = EnterCommand;
                return WriteLine( "OK expecting command");
            }

            case EnterCommand:
            {
                switch (cmdHandler.getCommand( itr, eoM))
                {
                   case CommandHandler::empty:
                   {
                      state = EmptyLine;
                      continue;
                   }
                   case CommandHandler::caps:
                   {
                      state = EmptyLine;
                      return WriteLine( cmdHandler.getCaps());
                   }
                   case CommandHandler::quit:
                   {
                      state = Terminate;
                      return WriteLine( "BYE");
                   }
                   case CommandHandler::method:
                   {
                      state = Processing;
                      continue;
                   }
                   case CommandHandler::unknown:
                   {
                      if (itr == eoM)
                      {
                         input.setPos( 0);
                         return Network::ReadData( input.ptr(), input.size());
                      }
                      else
                      {
                         state = HandleProtocolError;
                         return WriteLine( "BAD unknown command");
                      }
                   }
                }
            }

            case EmptyLine:
            {
               if (!ProtocolParser::skipSpaces( itr, eoM))
               {
                  input.setPos( 0);
                  return Network::ReadData( input.ptr(), input.size());
               }
               if (!ProtocolParser::consumeEOLN( itr, eoM))
               {
                  if (itr == eoM)
                  {
                     input.setPos( 0);
                     return Network::ReadData( input.ptr(), input.size());
                  }
                  else
                  {
                     state = Init;
                     buffer.init();
                     return WriteLine( "BAD command line");
                  }
               }
               else
               {
                  state = EnterCommand;
                  continue;
               }
            }

            case ProcessingAfterWrite:
            {
                //do processing but first release the output buffer content that has been written in the processing state:
                state = Processing;
                output.release();
                continue;
            }

            case Processing:
            {
                int returnCode = 0;
                switch (cmdHandler.call( returnCode))
                {
                   case CommandHandler::ReadInput:
                   {
                      input.setPos( 0);
                      return Network::ReadData( input.ptr(), input.size());
                   }
                   case CommandHandler::WriteOutput:
                   {
                      state = ProcessingAfterWrite;
                      break;
                   }
                   case CommandHandler::Close:
                   {
                      input.resetEoD();
                      state = (returnCode)?HandleCommandError:Init;
                   }
                   void* content = output.ptr();
                   std::size_t size = output.pos();
                   if (size == 0) continue;
                   return Network::SendData( content, size);
                }
            }

            case HandleCommandError:
            case HandleProtocolError:
            {
                if (!ProtocolParser::skipLine( itr, eoM) || !ProtocolParser::consumeEOLN( itr, eoM))
                {
                   input.setPos( 0);
                   return Network::ReadData( input.ptr(), input.size());
                }            
                state = Init;
                continue;
            }

            case Terminate:
            {
                state = Terminate;
                return Network::CloseConnection();
            }
         }//switch(..)
      }//for(,,)
      return Network::CloseConnection();
   }
};


Connection::Connection( const Network::LocalTCPendpoint& local, unsigned int inputBufferSize, unsigned int outputBufferSize)
{
   data = new Private( inputBufferSize, outputBufferSize);
   LOG_TRACE << "Created connection handler for " << local.toString();
}

Connection::Connection( const Network::LocalSSLendpoint& local)
{
   data = new Private(8,8);
   LOG_TRACE << "Created connection handler (SSL) for " << local.toString();
}

Connection::~Connection()
{
   LOG_TRACE << "Connection handler destroyed";
   delete data;
}

void Connection::setPeer( const Network::RemoteTCPendpoint& remote)
{
   LOG_TRACE << "Peer set to " << remote.toString();
}

void Connection::setPeer( const Network::RemoteSSLendpoint& remote)
{
   LOG_TRACE << "Peer set to " << remote.toString();
}

void Connection::networkInput( const void* bytes, std::size_t nofBytes)
{
   data->networkInput( bytes, nofBytes);
}

void Connection::initObject( Instance* instance)
{
   data->cmdHandler.init( instance);
}

void Connection::timeoutOccured()
{
   data->signalTerminate();
}

void Connection::signalOccured()
{
   data->signalTerminate();
}

void Connection::errorOccured( NetworkSignal )
{
   data->signalTerminate();
}

const Connection::Operation Connection::nextOperation()
{
   return data->nextOperation();
}

/// ServerHandler PIMPL
Network::connectionHandler* ServerHandler::ServerHandlerImpl::newConnection( const Network::LocalTCPendpoint& local )
{
   return new mtproc::Connection( local );
}

Network::connectionHandler* ServerHandler::ServerHandlerImpl::newSSLconnection( const Network::LocalSSLendpoint& local )
{
   return new mtproc::Connection( local );
}

ServerHandler::ServerHandler( const HandlerConfiguration& ) : impl_( new ServerHandlerImpl ) {}

ServerHandler::~ServerHandler()  { delete impl_; }

Network::connectionHandler* ServerHandler::newConnection( const Network::LocalTCPendpoint& local )
{
   return impl_->newConnection( local );
}

Network::connectionHandler* ServerHandler::newSSLconnection( const Network::LocalSSLendpoint& local )
{
   return impl_->newSSLconnection( local );
}
