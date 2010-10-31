/**
--------------------------------------------------------------------
    The template library textwolf implements an input iterator over 
    a set of XML path expressions without backward references on an 
    STL conforming input iterator as source. It does no buffering 
    or read ahead and is dedicated for stream processing of XML 
    for a small set of XML queries. 
    Stream processing in this context refers to processing the 
    document without buffering anything but the current result token 
    processed with its tag hierarchy information.
    
    Copyright (C) 2010 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

    The latest version of textwolf can be found at 'http://github.com/patrickfrey/textwolf'
    
--------------------------------------------------------------------
**/
#ifndef __TEXTWOLF_HPP__
#define __TEXTWOLF_HPP__
#include <iterator>
#include <vector>
#include <stack>
#include <map>
#include <exception>
#include <iostream>

namespace textwolf {

struct throws_exception
{
   enum Cause
   {
      Unknown, DimOutOfRange, StateNumbersNotAscending, InvalidParam,
      InvalidState, IllegalParam, IllegalAttributeName, OutOfMem,
      ArrayBoundsReadWrite
   };
};

struct exception   :public std::exception
{
   typedef throws_exception::Cause Cause; 
   Cause cause;
   
   exception (Cause p_cause) throw()                    :cause(p_cause) {};
   exception (const exception& orig) throw()            :cause(orig.cause) {};
   exception& operator= (const exception& orig) throw() {cause=orig.cause; return *this;};
   virtual ~exception() throw() {};
   virtual const char* what() const throw()
   {
      static const char* nameCause[ 9] = { 
         "Unknown","DimOutOfRange","StateNumbersNotAscending","InvalidParam",
         "InvalidState","IllegalParam","IllegalAttributeName","OutOfMem",
         "ArrayBoundsReadWrite"
      };
      return nameCause[ (unsigned int) cause];
   };
};

/**
* character map for fast typing of a character byte
*/
template <typename RESTYPE, RESTYPE nullvalue_, int RANGE=256>
class CharMap
{
public:
   typedef RESTYPE valuetype;
   enum Constant {nullvalue=nullvalue_};
   
private:
   RESTYPE ar[ RANGE];
public:
   CharMap()                                                                         {for (unsigned int ii=0; ii<RANGE; ii++) ar[ii]=(valuetype)nullvalue;};
   CharMap& operator()( unsigned char from, unsigned char to, valuetype value)       {for (unsigned int ii=from; ii<=to; ii++) ar[ii]=value; return *this;};
   CharMap& operator()( unsigned char at, valuetype value)                           {ar[at] = value; return *this;};
   valuetype operator []( unsigned char ii) const                                    {return ar[ii];};
};

/*
* unicode character range type used for processing
*/
typedef unsigned int UChar;

namespace charset {
/**
* Default character set definitions: 
* 1) Iso-Latin-1
* 2) UCS2  (little and big endian, not very efficient implementation)
* 3) UCS4  (little and big endian, not very efficient implementation)
* 4) UTF-8 (see http://de.wikipedia.org/wiki/UTF-8 for algorithms)
*/
struct IsoLatin1
{
   enum {HeadSize=1,Size=1,MaxChar=0xFF};
   
   static unsigned int asize()                                                 {return HeadSize;};
   static unsigned int size( const char*)                                      {return Size;};  
   static char achar( const char* buf)                                         {return buf[0];};
   static UChar value( const char* buf)                                        {return (unsigned char)buf[0];};
   static unsigned int print( UChar chr, char* buf, unsigned int bufsize)      {if (bufsize < 1) return 0; buf[0] = (chr <= 255)?(char)(unsigned char)chr:-1; return 1;};
};

struct ByteOrder
{
   enum {LE=1,BE=2,Machine=1};
};

template <int encoding=ByteOrder::Machine>
struct UCS2
{
   enum {LSB=(encoding==ByteOrder::BE),MSB=(encoding==ByteOrder::LE),HeadSize=2,Size=2,MaxChar=0xFFFF};

   static unsigned int asize()                                                 {return HeadSize;};
   static unsigned int size( const char*)                                      {return Size;};  
   static char achar( const char* buf)                                         {return (buf[MSB])?(char)-1:buf[LSB];};
   static UChar value( const char* buf)                                        {UChar res = (unsigned char)buf[MSB]; return (res << 8) + (unsigned char)buf[LSB];};
   static unsigned int print( UChar chr, char* buf, unsigned int bufsize)      {if (bufsize<2) return 0; if (chr>0xFFFF) {buf[0]=(char)0xFF; buf[1]=(char)0xFF;} else {buf[LSB]=(char)chr; buf[MSB]=(char)(chr>>8);} return 2;};
};

template <int encoding=ByteOrder::Machine>
struct UCS4
{
   enum {B0=(encoding==ByteOrder::BE)?3:0,B1=(encoding==ByteOrder::BE)?2:1,B2=(encoding==ByteOrder::BE)?1:2,B3=(encoding==ByteOrder::BE)?0:3,HeadSize=4,Size=4,MaxChar=0xFFFFFFFF};

   static unsigned int asize()                                                 {return HeadSize;};
   static unsigned int size( const char*)                                      {return Size;};  
   static char achar( const char* buf)                                         {return (buf[B3]|buf[B2]|buf[B1])?(char)-1:buf[B0];};
   static UChar value( const char* buf)                                        {UChar res = (unsigned char)buf[B3]; res = (res << 8) + (unsigned char)buf[B2]; res = (res << 8) + (unsigned char)buf[B1]; return (res << 8) + (unsigned char)buf[B0];};
   static unsigned int print( UChar chr, char* buf, unsigned int bufsize)      {buf[B0]=(char)chr; chr>>=8; buf[B1]=(char)chr; chr>>=8; buf[B2]=(char)chr; chr>>=8; buf[B3]=(char)chr; chr>>=8; return 4;};
};

struct UCS2LE :public UCS2<ByteOrder::LE> {};
struct UCS2BE :public UCS2<ByteOrder::BE> {};
struct UCS4LE :public UCS4<ByteOrder::LE> {};
struct UCS4BE :public UCS4<ByteOrder::BE> {};


struct UTF8
{   
   enum {MaxChar=0xFFFFFFFF};
   enum {B11111111=0xFF,
         B01111111=0x7F,
         B00111111=0x3F,
         B00011111=0x1F,
         B00001111=0x0F,
         B00000111=0x07,
         B00000011=0x03,
         B00000001=0x01,
         B00000000=0x00,
         B10000000=0x80,
         B11000000=0xC0,
         B11100000=0xE0,
         B11110000=0xF0,
         B11111000=0xF8,
         B11111100=0xFC,
         B11111110=0xFE,
         
         B11011111=B11000000|B00011111,
         B11101111=B11100000|B00001111,
         B11110111=B11110000|B00000111,
         B11111011=B11111000|B00000011,
         B11111101=B11111100|B00000001
        };

   enum {HeadSize=1};

   struct CharLengthTab   :public CharMap<unsigned char, 0>
   {
      CharLengthTab()
      {
         (*this)
         (B00000000,B01111111,1)
         (B11000000,B11011111,2)
         (B11100000,B11101111,3)
         (B11110000,B11110111,4)
         (B11111000,B11111011,5)
         (B11111100,B11111101,6)
         (B11111110,B11111110,7)
         (B11111111,B11111111,8);
      };
   };

   static unsigned int asize()                                                 {return HeadSize;};
   static char achar( const char* buf)                                         {return buf[0];};
   static unsigned int size( const char* buf)                                  {static CharLengthTab charLengthTab; return charLengthTab[ (unsigned char)buf[ 0]];};

   static UChar value( const char* buf)
   {
      const UChar invalid = (UChar)0-1;
      UChar res;
      int gg;
      int ii;
      unsigned char ch = (unsigned char)*buf;

      if (ch < 128) return ch;

      gg = size(buf)-2;
      if (gg < 0) return invalid;
      
      res = (ch)&(B00011111>>gg);
      for (ii=0; ii<=gg; ii++)
      {
         unsigned char xx = (unsigned char)buf[ii+1]; 
         res = (res<<6) | (xx & B00111111);
         if ((unsigned char)(xx & B11000000) != B10000000)
         {
            return invalid;
         }
      }
      return res;
   };
   
   static unsigned int print( UChar chr, char* buf, unsigned int bufsize)
   {
      unsigned int rt;
      if (bufsize < 8) return 0;
      if (chr <= 127) {
         buf[0] = (char)(unsigned char)chr;
         return 1;
      }
      unsigned int pp,sf;
      for (pp=1,sf=5; pp<5; pp++,sf+=5)
      {
         if (chr < (unsigned int)((1<<6)<<sf))
         {
            rt = pp+1;
            while (pp > 0)
            {
               buf[pp--] = (char)(unsigned char)((chr & B00111111) | B10000000);
               chr >>= 6;
            }
            unsigned char HB = (unsigned char)(B11111111 << (8-rt));
            buf[0] = (char)(((unsigned char)chr & (~HB >> 1)) | HB); 
            return rt;
         }
      }
      rt = pp+1;
      while (pp > 0)
      {
         buf[pp--] = (char)(unsigned char)((chr & B00111111) | B10000000);
         chr >>= 6;
      }
      unsigned char HB = (unsigned char)(B11111111 << (8-rt));
      buf[0] = (char)(((unsigned char)chr & (~HB >> 1)) | HB); 
      return rt;
   };
};
}//namespace charset


/**
* control characters needed for XML scanner statemachine
*/
enum ControlCharacter 
{ 
   Undef=0, EndOfText, EndOfLine, Cntrl, Space, Amp, Lt, Equal, Gt, Slash, Exclam, Questm, Sq, Dq, Osb, Csb, Any,
   NofControlCharacter=17
};
const char* controlCharacterName( ControlCharacter c)
{
   static const char* name[ NofControlCharacter] = {"Undef", "EndOfText", "EndOfLine", "Cntrl", "Space", "Amp", "Lt", "Equal", "Gt", "Slash", "Exclam", "Questm", "Sq", "Dq", "Osb", "Csb", "Any"};
   return name[ (unsigned int)c];   
}

struct DefaultCharProd
{
   DefaultCharProd(){};
   char operator[]( char ch) const {return ch;};
};

/**
* reads the input and provides the items to control the parsing:
*   control characters, ascii characters, unicode characters
* @TODO Implement better skip for iterator += i if available
*/
template <class Iterator, class CharSet, class CharProd=DefaultCharProd>
class TextScanner
{
private:
   Iterator input;
   CharProd charProd;
   
   char buf[8];
   UChar val;
   char cur;
   unsigned int state;
   
public:
   struct ControlCharMap  :public CharMap<ControlCharacter,Undef>
   {
      ControlCharMap()
      {
         (*this)  (0,EndOfText)  (1,31,Cntrl)      (5,Undef)         (33,127,Any)   (128,255,Undef) 
                  ('\t',Space)   ('\r',EndOfLine)  ('\n',EndOfLine)  (' ',Space)    ('&',Amp)
                  ('<',Lt)       ('=',Equal)       ('>',Gt)          ('/',Slash)    ('!',Exclam)
                  ('?',Questm)   ('\'',Sq)         ('\"',Dq)         ('[',Osb)      (']',Osb);
      };
   };   
   
   TextScanner( const Iterator& p_iterator)    :input(p_iterator),val(0),cur(0),state(0) {};
   TextScanner( const TextScanner& orig)       :val(orig.val),cur(orig.cur),state(orig.state)
   {
      for (unsigned int ii=0; ii<sizeof(buf); ii++) buf[ii]=orig.buf[ii];
   };
   
   UChar chr()
   {
      if (val == 0)
      {
         while (state < CharSet::size(buf))
         {
            buf[state] = *input;        
            input++;
            state++;
         }
         val = CharSet::value(buf);
      }
      return val;
   };

   void getcur()
   {
      while (state < CharSet::asize())
      {
         buf[state] = *input;        
         input++;
         state++;
      }
      cur = charProd[ CharSet::achar(buf)];
   };
   
   ControlCharacter control()
   {
      static ControlCharMap controlCharMap;
      getcur();
      return controlCharMap[ (unsigned char)cur];
   };
   
   char ascii()
   {
      getcur();
      return cur>=0?cur:0;
   };
   
   TextScanner& skip()
   {
      while (state < CharSet::asize())
      {
         input++; 
         state++;
      }
      state = 0;
      cur = 0;
      val = 0;
      return *this;
   };
   
   TextScanner& operator ++()     {return skip();};
   TextScanner operator ++(int)   {TextScanner tmp(*this); skip(); return tmp;};
};


/**
* with this class we build up the XML element scanner state machine in a descriptive way
*/ 
class ScannerStatemachine :public throws_exception
{
public:
   enum Constant   {MaxNofStates=128};
   struct Element
   {
      int fallbackState;
      int missError;
      struct 
      {
         int op;
         int arg;
      } action;
      char next[ NofControlCharacter];

      Element()
            :fallbackState(-1),missError(-1)
      {
         action.op = -1;
         action.arg = 0;
         for (unsigned int ii=0; ii<NofControlCharacter; ii++) next[ii] = -1;
      };
   };
   Element* get( int stateIdx) throw(exception)
   {
      if ((unsigned int)stateIdx>size) throw exception(InvalidState); 
      return tab + stateIdx;
   };

private:
   Element tab[ MaxNofStates];
   unsigned int size;

   void newState( int stateIdx) throw(exception)
   {
      if (size != (unsigned int)stateIdx) throw exception( StateNumbersNotAscending);
      if (size >= MaxNofStates) throw exception( DimOutOfRange);
      size++;
   };
   void addOtherTransition( int nextState) throw(exception)
   {
      if (size == 0) throw exception( InvalidState);
      if (nextState < 0 || nextState > MaxNofStates) throw exception( InvalidParam);
      for (unsigned int inputchr=0; inputchr<NofControlCharacter; inputchr++)
      {
         if (tab[ size-1].next[ inputchr] == -1) tab[ size-1].next[ inputchr] = (unsigned char)nextState;
      }
   };
   void addTransition( ControlCharacter inputchr, int nextState) throw(exception)
   {
      if (size == 0) throw exception( InvalidState);
      if ((unsigned int)inputchr >= (unsigned int)NofControlCharacter)  throw exception( InvalidParam);
      if (nextState < 0 || nextState > MaxNofStates)  throw exception( InvalidParam);
      if (tab[ size-1].next[ inputchr] != -1)  throw exception( InvalidParam);
      if (size == 0)  throw exception( InvalidState);
      tab[ size-1].next[ inputchr] = (unsigned char)nextState;
   };
   void addTransition( ControlCharacter inputchr) throw(exception)
   {
      addTransition( inputchr, size-1);
   };
   void addAction( int action_op, int action_arg=0) throw(exception)
   {
      if (size == 0) throw exception( InvalidState);
      if (tab[ size-1].action.op != -1) throw exception( InvalidState);
      tab[ size-1].action.op = action_op;
      tab[ size-1].action.arg = action_arg;
   };
   void addMiss( int error) throw(exception)
   {
      if (size == 0) throw exception( InvalidState);
      if (tab[ size-1].missError != -1) throw exception( InvalidState);
      tab[ size-1].missError = error;
   };
   void addFallback( int stateIdx) throw(exception)
   {
      if (size == 0) throw exception( InvalidState);
      if (tab[ size-1].fallbackState != -1) throw exception( InvalidState);
      if (stateIdx < 0 || stateIdx > MaxNofStates) throw exception( InvalidParam);
      tab[ size-1].fallbackState = stateIdx;      
   };
public:
   ScannerStatemachine()  
      :size(0)
   {};

   ScannerStatemachine& operator[]( int stateIdx)                           {newState(stateIdx); return *this;};   
   ScannerStatemachine& operator()( ControlCharacter inputchr, int ns)      {addTransition(inputchr,ns); return *this;};
   ScannerStatemachine& operator()( ControlCharacter inputchr1, 
                                    ControlCharacter inputchr2, int ns)     {addTransition(inputchr1,ns); addTransition(inputchr2,ns); return *this;};
   ScannerStatemachine& operator()( ControlCharacter inputchr1, 
                                    ControlCharacter inputchr2, 
                                    ControlCharacter inputchr3, int ns)     {addTransition(inputchr1,ns); addTransition(inputchr2,ns); addTransition(inputchr3,ns); return *this;};
   ScannerStatemachine& operator()( ControlCharacter inputchr)              {addTransition(inputchr); return *this;};
   ScannerStatemachine& action( int aa, int arg=0)            {addAction(aa,arg); return *this;};
   ScannerStatemachine& miss( int ee)                                       {addMiss(ee); return *this;};
   ScannerStatemachine& fallback( int stateIdx)                             {addFallback(stateIdx); return *this;};
   ScannerStatemachine& other( int stateIdx)                                {addOtherTransition(stateIdx); return *this;};
};

/**
* the template XMLScanner provides you the XML elements like tags, attributes, etc. with an STL conform input iterator 
* with XMLScannerBase we define the common elements
*/
class XMLScannerBase
{
public:
   enum ElementType
   {
      None, ErrorOccurred, HeaderAttribName, HeaderAttribValue, TagAttribName, TagAttribValue, OpenTag, CloseTag, CloseTagIm, Content, Exit
   };
   static const char* getElementTypeName( ElementType ee)
   {
      static const char* names[ (unsigned int)Exit+1] = {0,"ErrorOccurred","HeaderAttribName","HeaderAttribValue","TagAttribName","TagAttribValue","OpenTag","CloseTag","CloseTagIm","Content","Exit"};
      return names[ (unsigned int)ee];
   };
   enum Error
   {
      Ok,ErrMemblockTooSmall, ErrExpectedOpenTag, ErrUnexpectedState,
      ErrExpectedXMLTag, ErrSyntaxString, ErrUnexpectedEndOfText, ErrOutputBufferTooSmall,
      ErrSyntaxToken, ErrStringNotTerminated, ErrEntityEncodesCntrlChar, ErrExpectedIdentifier,
      ErrExpectedToken, ErrUndefinedCharacterEntity, ErrInternalErrorSTM, ErrExpectedTagEnd,
      ErrExpectedEqual, ErrExpectedTagAttribute, ErrExpectedCDATATag, ErrInternal
   };
   static const char* getErrorString( Error ee)
   {
      enum Constant {NofErrors=20};
      static const char* sError[NofErrors]
      = {0,"MemblockTooSmall","ExpectedOpenTag","UnexpectedState",
         "ExpectedXMLTag","SyntaxString","UnexpectedEndOfText","OutputBufferTooSmall",
         "SyntaxToken","StringNotTerminated","EntityEncodesCntrlChar","ExpectedIdentifier",
         "ExpectedToken", "UndefinedCharacterEntity","InternalErrorSTM","ExpectedTagEnd",
         "ExpectedEqual", "ExpectedTagAttribute","ExpectedCDATATag","Internal"
      };
      return sError[(unsigned int)ee];
   };
   enum STMState 
   {
      START,  STARTTAG, XTAG, XTAGEND, XTAGAISK, XTAGANAM, XTAGAESK, XTAGAVSK, XTAGAVID, XTAGAVSQ, XTAGAVDQ, XTAGAVQE, CONTENT, 
      TOKEN, XMLTAG, OPENTAG, CLOSETAG, TAGCLSK, TAGAISK, TAGANAM, TAGAESK, TAGAVSK, TAGAVID, TAGAVSQ, TAGAVDQ, TAGAVQE,
      TAGCLIM, ENTITYSL, ENTITY, CDATA, CDATA1, CDATA2, CDATA3, EXIT
   };
   static const char* getStateString( STMState s)
   {
      enum Constant {NofStates=34};
      static const char* sState[NofStates]
      = {
         "START", "STARTTAG", "XTAG", "XTAGEND", "XTAGAISK", "XTAGANAM", "XTAGAESK", "XTAGAVSK", "XTAGAVID", "XTAGAVSQ", "XTAGAVDQ", "XTAGAVQE", "CONTENT",
         "TOKEN", "XMLTAG", "OPENTAG", "CLOSETAG", "TAGCLSK", "TAGAISK", "TAGANAM", "TAGAESK", "TAGAVSK", "TAGAVID", "TAGAVSQ", "TAGAVDQ", "TAGAVQE",
         "TAGCLIM", "ENTITYSL", "ENTITY", "CDATA", "CDATA1", "CDATA2", "CDATA3", "EXIT"
      };      
      return sState[(unsigned int)s];      
   };
   
   enum STMAction 
   { 
      Return, ReturnToken, ReturnIdentifier, ReturnSQString, ReturnDQString, ExpectIdentifierXML, ExpectIdentifierCDATA, ReturnEOF,
      NofSTMActions = 8     
   };
   static const char* getActionString( STMAction a)
   {
      static const char* name[ NofSTMActions] = {"Return", "ReturnToken", "ReturnIdentifier", "ReturnSQString", "ReturnDQString", "ExpectIdentifierXML", "ExpectIdentifierCDATA", "ReturnEOF"};
      return name[ (unsigned int)a];
   };

   //@TODO return entity definitions as events back
   struct Statemachine :public ScannerStatemachine
   {
      Statemachine()
      {
         (*this)
         [ START    ](EndOfLine)(Cntrl)(Space)(Lt,STARTTAG).miss(ErrExpectedOpenTag)
         [ STARTTAG ](EndOfLine)(Cntrl)(Space)(Questm,XTAG )(Exclam,ENTITYSL).fallback(OPENTAG)
         [ XTAG     ].action(ExpectIdentifierXML)(EndOfLine,Cntrl,Space,XTAGAISK)(Questm,XTAGEND).miss(ErrExpectedXMLTag)
         [ XTAGEND  ](Gt,CONTENT)(EndOfLine)(Cntrl)(Space).miss(ErrExpectedTagEnd)
         [ XTAGAISK ](EndOfLine)(Cntrl)(Space)(Questm,XTAGEND).fallback(XTAGANAM)
         [ XTAGANAM ].action(ReturnIdentifier,HeaderAttribName)(EndOfLine,Cntrl,Space,XTAGAESK)(Equal,XTAGAVSK).miss(ErrExpectedEqual)
         [ XTAGAESK ](EndOfLine)(Cntrl)(Space)(Equal,XTAGAVSK).miss(ErrExpectedEqual)
         [ XTAGAVSK ](EndOfLine)(Cntrl)(Space)(Sq,XTAGAVSQ)(Dq,XTAGAVDQ).fallback(XTAGAVID)
         [ XTAGAVID ].action(ReturnIdentifier,HeaderAttribValue)(EndOfLine,Cntrl,Space,XTAGAISK)(Questm,XTAGEND).miss(ErrExpectedTagAttribute)
         [ XTAGAVSQ ].action(ReturnSQString,HeaderAttribValue)(Sq,XTAGAVQE).miss(ErrStringNotTerminated)
         [ XTAGAVDQ ].action(ReturnDQString,HeaderAttribValue)(Dq,XTAGAVQE).miss(ErrStringNotTerminated)
         [ XTAGAVQE ](EndOfLine,Cntrl,Space,XTAGAISK)(Questm,XTAGEND).miss(ErrExpectedTagAttribute)
         [ CONTENT  ](EndOfText,EXIT)(EndOfLine)(Cntrl)(Space)(Lt,XMLTAG).fallback(TOKEN)
         [ TOKEN    ].action(ReturnToken,Content)(EndOfText,EXIT)(EndOfLine,Cntrl,Space,CONTENT)(Lt,XMLTAG).fallback(CONTENT)
         [ XMLTAG   ](EndOfLine)(Cntrl)(Space)(Questm,XTAG)(Slash,CLOSETAG).fallback(OPENTAG)
         [ OPENTAG  ].action(ReturnIdentifier,OpenTag)(EndOfLine,Cntrl,Space,TAGAISK)(Slash,TAGCLIM)(Gt,CONTENT).miss(ErrExpectedTagAttribute)
         [ CLOSETAG ].action(ReturnIdentifier,CloseTag)(EndOfLine,Cntrl,Space,TAGCLSK)(Gt,CONTENT).miss(ErrExpectedTagEnd)
         [ TAGCLSK  ](EndOfLine)(Cntrl)(Space)(Gt,CONTENT).miss(ErrExpectedTagEnd)
         [ TAGAISK  ](EndOfLine)(Cntrl)(Space)(Gt,CONTENT)(Slash,TAGCLIM).fallback(TAGANAM)
         [ TAGANAM  ].action(ReturnIdentifier,TagAttribName)(EndOfLine,Cntrl,Space,TAGAESK)(Equal,TAGAVSK).miss(ErrExpectedEqual)
         [ TAGAESK  ](EndOfLine)(Cntrl)(Space)(Equal,TAGAVSK).miss(ErrExpectedEqual)
         [ TAGAVSK  ](EndOfLine)(Cntrl)(Space)(Sq,TAGAVSQ)(Dq,TAGAVDQ).fallback(TAGAVID)
         [ TAGAVID  ].action(ReturnIdentifier,TagAttribValue)(EndOfLine,Cntrl,Space,TAGAISK)(Slash,TAGCLIM)(Gt,CONTENT).miss(ErrExpectedTagAttribute)
         [ TAGAVSQ  ].action(ReturnSQString,TagAttribValue)(Sq,TAGAVQE).miss(ErrStringNotTerminated)
         [ TAGAVDQ  ].action(ReturnDQString,TagAttribValue)(Dq,TAGAVQE).miss(ErrStringNotTerminated)
         [ TAGAVQE  ](EndOfLine,Cntrl,Space,TAGAISK)(Slash,TAGCLIM)(Gt,CONTENT).miss(ErrExpectedTagAttribute)
         [ TAGCLIM  ].action(Return,CloseTagIm)(EndOfLine)(Cntrl)(Space)(Gt,CONTENT).miss(ErrExpectedTagEnd)
         [ ENTITYSL ](Osb,CDATA).fallback(ENTITY)
         [ ENTITY   ](Exclam,TAGCLSK).other( ENTITY)
         [ CDATA    ].action(ExpectIdentifierCDATA)(Osb,CDATA1).miss(ErrExpectedCDATATag)
         [ CDATA1   ](Csb,CDATA2).other(CDATA1)
         [ CDATA2   ](Csb,CDATA3).other(CDATA1)
         [ CDATA3   ](Gt,CONTENT).other(CDATA1)
         [ EXIT     ].action(Return,Exit);
      };
   };
};


template <
      class InputIterator,                         //< STL conform input iterator with ++ and read only * returning 0 als last character of the input
      class InputCharSet_=charset::UTF8,           //Character set encoding of the input, read as stream of bytes
      class OutputCharSet_=charset::UTF8,          //Character set encoding of the output, printed as string of the item type of the character set
      class EntityMap=std::map<const char*,UChar>, //< STL like map from ASCII const char* to UChar
      class CharProd=DefaultCharProd
>
class XMLScanner :public XMLScannerBase
{
private:
   struct TokState
   {
      enum Id {Start,ParsingKey,ParsingEntity,ParsingNumericEntity,ParsingNumericBaseEntity,ParsingNamedEntity,ParsingToken};
      Id id;
      unsigned int pos;
      unsigned int base;
      unsigned long long value;
      char buf[ 16];
      UChar curchr_saved;
      
      TokState()                       :id(Start),pos(0),base(0),value(0),curchr_saved(0) {};
      void init(Id id_=Start)          {id=id_;pos=0;base=0;value=0;curchr_saved=0;};
   };
   TokState tokstate;

public:
   typedef InputCharSet_ InputCharSet;
   typedef OutputCharSet_ OutputCharSet;
   typedef unsigned int size_type;
   class iterator;
   
public:
   typedef TextScanner<InputIterator,InputCharSet_,CharProd> InputReader;
   typedef XMLScanner<InputIterator,InputCharSet_,OutputCharSet_,EntityMap,CharProd> ThisXMLScanner;
   typedef typename EntityMap::iterator EntityMapIterator;
   
   unsigned int print( UChar ch)
   {
      unsigned int nn = OutputCharSet::print( ch, outputBuf+outputSize, outputBufSize-outputSize);
      if (nn == 0)
      {
         error = ErrOutputBufferTooSmall;
         tokstate.curchr_saved = ch;
      }
      return nn;     
   };
   
   bool push( UChar ch)
   {
      unsigned int nn = print( ch);
      outputSize += nn; 
      return (nn != 0);     
   };
   
   static unsigned char HEX( unsigned char ch)
   {
      struct HexCharMap :public CharMap<unsigned char, 0xFF>
      {
         HexCharMap()
         {
            (*this)
               ('0',0) ('1', 1)('2', 2)('3', 3)('4', 4)('5', 5)('6', 6)('7', 7)('8', 8)('9', 9)
               ('A',10)('B',11)('C',12)('D',13)('E',14)('F',15)('a',10)('b',11)('c',12)('d',13)('e',14)('f',15);
         };
      };
      static HexCharMap hexCharMap;
      return hexCharMap[ch];
   };

   static UChar parseStaticNumericEntityValue( InputReader& ir)
   {
      signed long long value = 0;
      unsigned char ch = ir.ascii();
      unsigned int base;
      if (ch != '#') return 0;
      ir.skip();
      ch = ir.ascii();
      if (ch == 'x')
      {
         ir.skip();
         ch = ir.ascii();
         base = 16;
      }
      else
      {
         base = 10;
      }
      while (ch != ';')
      {
         unsigned char chval = HEX(ch);
         if (value >= base) return 0;
         value = value * base + chval;
         if (value >= 0xFFFFFFFF) return 0;
         ir.skip();
         ch = ir.ascii();
      }
      return (UChar)value;
   };

   bool fallbackEntity()
   {
      switch (tokstate.id)
      {
         case TokState::Start:
         case TokState::ParsingKey:
         case TokState::ParsingToken:
            break;
            
         case TokState::ParsingEntity:
            return push('&');            
         case TokState::ParsingNumericEntity:
            return push('&') && push('#');
         case TokState::ParsingNumericBaseEntity:            
            if (!push('&') || !push('#')) return false;
            for (unsigned int ii=0; ii<tokstate.pos; ii++) if (!push( tokstate.buf[ii])) return false;
            return true;
         case TokState::ParsingNamedEntity:
            if (!push('&')) return false;
            for (unsigned int ii=0; ii<tokstate.pos; ii++) if (!push( tokstate.buf[ii])) return false;
            return true;
      }
      error = ErrInternal;
      return false;
   };
   
   bool parseEntity()
   {
      unsigned char ch;
      tokstate.id = TokState::ParsingEntity;
      ch = src.ascii();
      if (ch == '#')
      {
         src.skip();
         return parseNumericEntity();
      }
      else
      {
         return parseNamedEntity();
      }
   };
      
   bool parseNumericEntity()
   {
      unsigned char ch;
      tokstate.id = TokState::ParsingNumericEntity;
      ch = src.ascii();
      if (ch == 'x')
      {
          tokstate.base = 16;
          src.skip();
          return parseNumericBaseEntity();
      }
      else
      {
          tokstate.base = 10;
          return parseNumericBaseEntity();          
      }
   };
   
   bool parseNumericBaseEntity()
   {
      unsigned char ch;
      tokstate.id = TokState::ParsingNumericBaseEntity;
   
      while (tokstate.pos < sizeof(tokstate.buf))
      {
         tokstate.buf[tokstate.pos++] = ch = src.ascii();
         if (ch == ';')
         {
            if (tokstate.value > 0xFFFFFFFF) return fallbackEntity();
            if (tokstate.value < 32)
            {
               error = ErrEntityEncodesCntrlChar; 
               return false;
            }
            if (tokstate.value > 0xFFFFFFFF) fallbackEntity();
            src.skip();
            return push( tokstate.value);
         }
         else
         {
            unsigned char chval = HEX(ch);
            if (tokstate.value >= tokstate.base) fallbackEntity();
            tokstate.value = tokstate.value * tokstate.base + chval;
         }
      }
      return fallbackEntity();
   };

   bool parseNamedEntity()
   {
      unsigned char ch;
      tokstate.id = TokState::ParsingNamedEntity;
   
      ch = src.ascii();
      while (tokstate.pos < sizeof(tokstate.buf)-1 && ch != ';' && src.control() == Any)
      {   
          tokstate.buf[ tokstate.pos] = ch;
          src.skip();
          tokstate.pos++;
          ch = src.ascii();
      }
      if (ch == ';')
      {
          tokstate.buf[ tokstate.pos] = '\0';
          src.skip();
          return pushEntity( tokstate.buf);
      }
      else
      {
          return fallbackEntity();
      }
   };
   
   typedef CharMap<bool,false,NofControlCharacter> IsTokenCharMap;
   
   struct IsTagCharMap :public IsTokenCharMap
   {
      IsTagCharMap()
      {
         (*this)(Undef,true)(Any,true);
      };
   };

   struct IsContentCharMap :public IsTokenCharMap
   {
      IsContentCharMap()
      {
         (*this)(Undef,true)(Equal,true)(Gt,true)(Slash,true)(Exclam,true)(Questm,true)(Sq,true)(Dq,true)(Osb,true)(Csb,true)(Any,true);
      };
   };
   
   struct IsSQStringCharMap :public IsContentCharMap
   {
      IsSQStringCharMap()
      {         
         (*this)(Sq,false);
      };
   };
   
   struct IsDQStringCharMap :public IsContentCharMap
   {
      IsDQStringCharMap()
      {         
         (*this)(Dq,false);
      };
   };

   bool parseTokenRecover()
   {
      bool rt;
      if (tokstate.curchr_saved)
      {
         if (!push( tokstate.curchr_saved)) return false;
         tokstate.curchr_saved = 0;
      }
      switch (tokstate.id)
      {
         case TokState::Start:
         case TokState::ParsingKey:
         case TokState::ParsingToken:
            error = ErrInternal;
            return false;
         case TokState::ParsingEntity: rt = parseEntity(); break;
         case TokState::ParsingNumericEntity: rt = parseNumericEntity(); break;
         case TokState::ParsingNumericBaseEntity: rt = parseNumericBaseEntity(); break;
         case TokState::ParsingNamedEntity: rt = parseNamedEntity(); break;
      }
      tokstate.init( TokState::ParsingToken);
      return rt;
   };
   
   bool parseToken( const IsTokenCharMap& isTok)
   {
      if (tokstate.id == TokState::Start)
      {
         tokstate.id = TokState::ParsingToken;
      }
      else if (tokstate.id != TokState::ParsingToken)
      {
         if (!parseTokenRecover())
         {
            tokstate.init();
            return false;
         }
      }
      for (;;)
      {
         ControlCharacter ch;
         while (isTok[ ch=src.control()])
         {
            if (!push( src.chr()))
            {
               tokstate.curchr_saved = src.chr();
               return false;
            }
            src.skip();
         }
         if (ch == Amp) 
         {
            src.skip();
            if (!parseEntity()) break;
            tokstate.init( TokState::ParsingToken);
            continue;
         }
         else if (outputSize == 0)
         {
            tokstate.init();
            error = ErrExpectedToken;
            break;
         }
         else
         {
            tokstate.init();
            return true;
         }
      }
      tokstate.init();
      return false;
   };

   static bool parseStaticToken( const IsTokenCharMap& isTok, InputReader ir, char* buf, size_type bufsize, size_type* p_outputBufSize)
   {
      for (;;)
      {
         ControlCharacter ch;
         size_type ii=0;
         *p_outputBufSize = 0;
         for (;;)
         {
            UChar pc;
            if (isTok[ ch=ir.control()])
            {
               pc = ir.chr();
            }
            else if (ch == Amp)
            {
               pc = parseStaticNumericEntityValue( ir);
            }
            else
            {
               *p_outputBufSize = ii;
               return true;
            }
            unsigned int chlen = OutputCharSet::print( pc, buf+ii, bufsize-ii);
            if (chlen == 0 || pc == 0)
            {
               *p_outputBufSize = ii;
               return false; 
            }
            ii += chlen;
            ir.skip();
         }
      }      
   };

   bool skipToken( const IsTokenCharMap& isTok)
   {
      for (;;)
      {
         ControlCharacter ch;
         while (isTok[ ch=src.control()] || ch == Amp)
         {
            src.skip();
         }
         if (src.control() != Any) return true;
      }
   };
   
   bool expectStr( const char* str)
   {
      bool rt = true;
      tokstate.id = TokState::ParsingKey;
      for (; str[tokstate.pos] != '\0'; src.skip(),tokstate.pos++)
      {
         if (src.ascii() == str[ tokstate.pos]) continue;
         ControlCharacter ch = src.control();
         if (ch == EndOfText)
         {
            error = ErrUnexpectedEndOfText;
         }
         else
         {
            error = ErrSyntaxToken; 
         }
         rt = false;
         break;
      }
      tokstate.init();
      return rt;
   }; 
   
   bool pushPredefinedEntity( const char* str)
   {
      switch (str[0])
      {
         case 'q':
            if (str[1] == 'u' && str[2] == 'o' && str[3] == 't' && str[4] == '\0')
            {
               if (!push( '\"')) return false;
               return true;
            }
            break;

         case 'a':
            if (str[1] == 'm')
            {
               if (str[2] == 'p' && str[3] == '\0')
               {
                  if (!push( '&')) return false;
                  return true;
               }
            }
            else if (str[1] == 'p')
            {
               if (str[2] == 'o' && str[3] == 's' && str[4] == '\0')
               {
                  if (!push( '\'')) return false;
                  return true;
               }
            }
            break;

         case 'l':
            if (str[1] == 't' && str[2] == '\0')
            {
               if (!push( '<')) return false;
               return true;
            }
            break;

         case 'g':
            if (str[1] == 't' && str[2] == '\0')
            {
               if (!push( '>')) return false;
               return true;
            }
            break;

         case 'n':
            if (str[1] == 'b' && str[2] == 's' && str[3] == 'p' && str[4] == '\0')
            {
               if (!push( ' ')) return false;
               return true;
            }
            break;
      }
      return false;
   };
   
   bool pushEntity( const char* str)
   {
      if (pushPredefinedEntity( str))
      {
         return true;
      }
      else if (entityMap)
      {
         EntityMapIterator itr = entityMap->find( str);
         if (itr == entityMap->end())
         {
            error = ErrUndefinedCharacterEntity;
            return false;
         }
         else
         {
            UChar ch = itr->second;
            if (ch < 32)
            {
               error = ErrEntityEncodesCntrlChar;
               return false;
            }
            return push( ch);
         }
      }
      else
      {
         error = ErrUndefinedCharacterEntity;
         return false;
      }
   };
   
private:
   STMState state;
   Error error;      
   InputReader src;
   EntityMap* entityMap;
   char* outputBuf;
   size_type outputBufSize;
   size_type outputSize;
   
public:
   XMLScanner( InputIterator& p_src, char* p_outputBuf, size_type p_outputBufSize, EntityMap* p_entityMap=0)
         :state(START),error(Ok),src(p_src),entityMap(p_entityMap),outputBuf(p_outputBuf),outputBufSize(p_outputBufSize),outputSize(0)
   {};
   
   XMLScanner( XMLScanner& o)
         :state(o.state),error(o.error),src(o.src),entityMap(o.entityMap),outputBuf(o.outputBuf),outputBufSize(o.outputBufSize),outputSize(o.outputSize)
   {};

   void setOutputBuffer( char* p_outputBuf, size_type p_outputBufSize)
   {
      outputBuf = p_outputBuf;
      outputBufSize = p_outputBufSize;
   };
   
   template <class CharSet>
   static bool getTagName( const char* src, char* p_outputBuf, size_type p_outputBufSize, size_type* p_outputSize)
   {
      static IsTagCharMap isTagCharMap;
      typedef XMLScanner<const char*, charset::UTF8, CharSet> Scan;
      char* itr = const_cast<char*>(src);
      return parseStaticToken( isTagCharMap, itr, p_outputBuf, p_outputBufSize, p_outputSize);
   }
   
   char* getItem() const {return outputBuf;};
   size_type getItemSize() const {return outputSize;};
   ScannerStatemachine::Element* getState()
   {
      static Statemachine STM;
      return STM.get( state);
   }
   
   Error getError( const char** str=0)
   {
      Error rt = error; 
      error = Ok; 
      if (str) *str=getErrorString(rt); 
      return rt;
   };

   ElementType nextItem( unsigned short mask=0xFFFF)
   {
      static const IsContentCharMap contentC;
      static const IsTagCharMap tagC;
      static const IsSQStringCharMap sqC;
      static const IsDQStringCharMap dqC;
      static const IsTokenCharMap* tokenDefs[ NofSTMActions] = {0,&contentC,&tagC,&sqC,&dqC,0,0,0};
      static const char* stringDefs[ NofSTMActions] = {0,0,0,0,0,"xml","CDATA",0};

      ElementType rt = None;
		if (tokstate.id == TokState::Start)
      {
         outputSize = 0;
         outputBuf[0] = 0;
      }
      do
      {      
         ScannerStatemachine::Element* sd = getState();
         if (sd->action.op != -1)
         {
            if (tokenDefs[sd->action.op])
            {
               if ((mask&(1<<sd->action.arg)) != 0)
               {
                  if (!parseToken( *tokenDefs[ sd->action.op])) return ErrorOccurred;
               }
               else
               {
                  if (!skipToken( *tokenDefs[ sd->action.op])) return ErrorOccurred;
               }
               if (!print(0)) return ErrorOccurred;
               rt = (ElementType)sd->action.arg;
            }
            else if (stringDefs[sd->action.op])
            {
               if (!expectStr( stringDefs[sd->action.op])) return ErrorOccurred;
            }
            else
            {                  
               rt = (ElementType)sd->action.arg;
               if (rt == Exit) return rt;
            }
         }
         ControlCharacter ch = src.control();

         if (sd->next[ ch] != -1) 
         {
            state = (STMState)sd->next[ ch];
            src.skip();
         }
         else if (sd->fallbackState != -1)
         {
            state = (STMState)sd->fallbackState;            
         }
         else if (sd->missError != -1)
         {
            print(0);
            error = (Error)sd->missError;
            return ErrorOccurred;
         }
         else if (ch == EndOfText)
         {
            print(0);
            error = ErrUnexpectedEndOfText;
            return ErrorOccurred;
         }
         else
         {
            print(0);
            error = ErrInternal;
            return ErrorOccurred;
         }
      }
      while (rt == None);
      return rt;
   };

   //STL conform input iterator for the output of this XMLScanner:   
   struct End {};
   class iterator
   {
   public:
      struct Element
      {
         ElementType type;
         const char* name() const      {return getElementTypeName( type);};
         char* content;
         size_type size;
         
         Element()                     :type(None),content(0),size(0)
         {};
         Element( const End&)          :type(Exit),content(0),size(0)
         {};
         Element( const Element& orig) :type(orig.type),content(orig.content),size(orig.size)
         {};
      };
      typedef Element value_type;
      typedef size_type difference_type;
      typedef Element* pointer;
      typedef Element& reference;
      typedef std::input_iterator_tag iterator_category;

   private:
      Element element;
      ThisXMLScanner* input;

      iterator& skip( unsigned short mask=0xFFFF)
      {
         if (input != 0)
         {
            element.type = input->nextItem(mask);
            element.content = input->getItem();
            element.size = input->getItemSize();
         }
         return *this;
      };
      bool compare( const iterator& iter) const
      {
         if (element.type == iter.element.type)
         {
            if (element.type == Exit || element.type == None) return true;  //equal only at beginning and end
         }
         return false;              
      };
   public:
      void assign( const iterator& orig)
      {
         input = orig.input;
         element.type = orig.element.type;
         element.content = orig.element.content;
         element.size = orig.element.size;
      };
      iterator( const iterator& orig) 
      {
         assign( orig);
      };
      iterator( ThisXMLScanner& p_input)
             :input( &p_input)
      {
         element.type = input->nextItem();
         element.content = input->getItem();
         element.size = input->getItemSize();
      };
      iterator( const End& et)  :element(et),input(0) {};
      iterator()  :input(0) {};
      iterator& operator = (const iterator& orig)
      {
         assign( orig);
         return *this;
      }      
      const Element& operator*()
      {
         return element;
      };
      const Element* operator->()
      {
         return &element;
      };
      iterator& operator++()     {return skip();};
      iterator operator++(int)   {iterator tmp(*this); skip(); return tmp;};      

      bool operator==( const iterator& iter) const   {return compare( iter);};
      bool operator!=( const iterator& iter) const   {return !compare( iter);};
   };
   
   iterator begin()
   {
      return iterator( *this);
   };
   iterator end()
   {
      return iterator( End());
   };
}; 

template <class CharSet_=charset::UTF8>
class XMLPathSelectAutomaton :public throws_exception
{
public:
   enum {defaultMemUsage=3*1024,defaultMaxDepth=32};
   unsigned int memUsage;
   unsigned int maxDepth;   
   unsigned int maxScopeStackSize;
   unsigned int maxFollows;
   unsigned int maxTokens;

public:
   XMLPathSelectAutomaton()
         :memUsage(defaultMemUsage),maxDepth(defaultMaxDepth),maxScopeStackSize(0),maxFollows(0),maxTokens(0)
   {
      if (!setMemUsage( memUsage, maxDepth)) throw exception( DimOutOfRange);
   };

   typedef int Hash;
   typedef XMLPathSelectAutomaton<CharSet_> ThisXMLPathSelectAutomaton;

public:
   enum Operation 
   {
      Content, Tag, Attribute, ThisAttributeValue, AttributeValue
   };
   static const char* operationName( Operation op)
   {
      static const char* name[ 5] = {"Content", "Tag", "Attribute", "ThisAttributeValue", "AttributeValue"};
      return name[ (unsigned int)op];
   };

   struct Mask
   {
      unsigned short pos;
      unsigned short neg;
      Mask( unsigned short p_pos=0, unsigned short p_neg=0):pos(p_pos),neg(p_neg) {};
      Mask( const Mask& orig)                              :pos(orig.pos),neg(orig.neg) {};
      Mask( Operation op)                                  :pos(0),neg(0) {this->match(op);};
      void reset()                                         {pos=0; neg=0;};
      void reject( XMLScannerBase::ElementType e)          {neg |= (1<<(unsigned short)e);};
      void match( XMLScannerBase::ElementType e)           {pos |= (1<<(unsigned short)e);};
      void seekop( Operation op)
      {
         switch (op)
         {
            case Tag:                this->match( XMLScannerBase::OpenTag); break;
            case Attribute:          this->match( XMLScannerBase::TagAttribName); break;               
            case ThisAttributeValue: this->match( XMLScannerBase::TagAttribValue); 
                                     this->reject( XMLScannerBase::TagAttribName); 
                                     this->reject( XMLScannerBase::Content); 
                                     this->reject( XMLScannerBase::OpenTag); break;
            case AttributeValue:     this->match( XMLScannerBase::TagAttribValue); 
                                     this->reject( XMLScannerBase::Content); 
                                     break;
            case Content:            this->match( XMLScannerBase::Content); break; 
         }         
      };
      void join( const Mask& mask)                         {pos |= mask.pos; neg |= mask.neg;};
      bool matches( XMLScannerBase::ElementType e) const   {return (0 != (pos & (1<<(unsigned short)e)));};
      bool rejects( XMLScannerBase::ElementType e) const   {return (0 != (neg & (1<<(unsigned short)e)));};
   };

   struct Core
   {
      Mask mask;
      bool follow;
      int typeidx;
      int cnt;

      Core()                      :follow(false),typeidx(0),cnt(-1) {};
      Core( const Core& orig)     :mask(orig.mask),follow(orig.follow),typeidx(orig.typeidx),cnt(orig.cnt) {};
   };

   struct State
   {
      Core core;
      unsigned int keysize;
      char* key;
      char* srckey;
      int next;
      int link;
            
      State()                        :keysize(0),key(0),srckey(0),next(-1),link(-1) {};
      State( const State& orig)      :core(orig.core),keysize(orig.keysize),key(0),srckey(0),next(orig.next),link(orig.link)
      {
         defineKey( orig.keysize, orig.key, orig.srckey);
      };
      ~State()
      {
         if (key) delete [] key;
      };
      
      bool isempty()                 {return key==0&&core.typeidx==0;};
      
      void defineKey( unsigned int p_keysize, const char* p_key, const char* p_srckey)
      {
         unsigned int ii;
         if (key)
         {
            delete [] key;
            key = 0;
         }
         if (srckey) 
         {
            delete [] srckey;
            srckey = 0;
         }
         if (p_key)
         {
            key = new char[ keysize=p_keysize];
            for (ii=0; ii<keysize; ii++) key[ii]=p_key[ii];
         }
         if (p_srckey)
         {
            for (ii=0; p_srckey[ii]!=0; ii++);
            srckey = new char[ ii+1];
            for (ii=0; p_srckey[ii]!=0; ii++) srckey[ii]=p_srckey[ii];
            srckey[ ii] = 0;
         }
      };
      
      void defNext( Operation op, unsigned int p_keysize, const char* p_key, const char* p_srckey, int p_next, bool p_follow=false) 
      {
         core.mask.seekop( op);
         defineKey( p_keysize, p_key, p_srckey);
         next = p_next;
         core.follow = p_follow; 
      };
      
      void defOutput( const Mask& mask, int p_typeidx, bool p_follow=false, int p_cnt=-1)
      {
         core.mask.join( mask);
         core.typeidx = p_typeidx;
         if (p_cnt >= 0) core.cnt = p_cnt;
         core.follow = p_follow; 
      };
      
      void defLink( int p_link)
      {
         link = p_link;
      };
   };
   std::vector<State> states;   
         
   struct Token
   {
      Core core;
      int stateidx;
      
      Token()                                       :stateidx(-1) {};
      Token( const Token& orig)                     :core(orig.core),stateidx(orig.stateidx) {};
      Token( const State& state, int p_stateidx)    :core(state.core),stateidx(p_stateidx) {};
   };

   struct Scope
   {
      Mask mask;
      Mask followMask;
      struct Range
      {
         unsigned int tokenidx_from;
         unsigned int tokenidx_to;
         unsigned int followidx;
         
         Range()                            :tokenidx_from(0),tokenidx_to(0),followidx(0) {};
         Range( const Scope& orig)          :tokenidx_from(orig.tokenidx_from),tokenidx_to(orig.tokenidx_to),followidx(orig.followidx) {};
      };
      Range range;
      
      Scope( const Scope& orig)             :mask(orig.mask),followMask(orig.followMask),range(orig.range) {};
      Scope& operator =( const Scope& orig) {mask=orig.mask; followMask=orig.followMask; range=orig.range; return *this;};
      Scope()                               {};  
   };

   bool setMemUsage( unsigned int p_memUsage, unsigned int p_maxDepth)
   {
      memUsage = p_memUsage;
      maxDepth = p_maxDepth;
      maxScopeStackSize = maxDepth;
      if (p_memUsage < maxScopeStackSize * sizeof(Scope))
      {
         maxScopeStackSize = 0;
      }
      else
      {
         p_memUsage -= maxScopeStackSize * sizeof(Scope);
      }
      maxFollows = (p_memUsage / sizeof(unsigned int)) / 32 + 2;
      p_memUsage -= sizeof(unsigned int) * maxFollows;
      maxTokens = p_memUsage / sizeof(Token);
      return (maxScopeStackSize != 0 && maxTokens != 0 && maxFollows != 0);
   };

private:      
   int defNext( int stateidx, Operation op, unsigned int keysize, const char* key, const char* srckey, bool follow=false) throw(exception)
   {
      try
      {
         State state;
         if (states.size() == 0)
         {
            stateidx = states.size();
            states.push_back( state);
         }
         for (int ee=stateidx; ee != -1; stateidx=ee,ee=states[ee].link)
         {
            if (states[ee].key != 0 && keysize == states[ee].keysize && states[ee].core.follow == follow)
            {
               unsigned int ii;
               for (ii=0; ii<keysize && states[ee].key[ii]==key[ii]; ii++);
               if (ii == keysize) return states[ee].next;
            }
         }
         if (!states[stateidx].isempty())
         {
            stateidx = states[stateidx].link = states.size();
            states.push_back( state);
         }
         states.push_back( state);
         unsigned int lastidx = states.size()-1;
         states[ stateidx].defNext( op, keysize, key, srckey, lastidx, follow); 
         return stateidx=lastidx;
      }
      catch (std::bad_alloc)
      {
         throw exception( OutOfMem);
      }
      catch (...)
      {
         throw exception( Unknown); 
      };
   };   

   int defOutput( int stateidx, const Mask& pushOpMask, int typeidx, bool follow=false, int cnt=-1) throw(exception)
   {
      try
      {
         State state;
         if (states.size() == 0)
         {
            stateidx = states.size();
            states.push_back( state);
         }
         if ((unsigned int)stateidx >= states.size()) throw exception( IllegalParam);
         
         if (!states[stateidx].isempty())
         {
            stateidx = states[stateidx].link = states.size();
            states.push_back( state);
         }
         states[ stateidx].defOutput( pushOpMask, typeidx, follow, cnt);
         return stateidx;
      }
      catch (std::bad_alloc)
      {
         throw exception( OutOfMem);
      }
      catch (...)
      {
         throw exception( Unknown); 
      };
   };
   
public:
   struct PathElement :throws_exception
   {
   private:
      enum {MaxSize=1024};
      
      XMLPathSelectAutomaton* xs;
      int stateidx;
      int count;
      bool follow;
      Mask pushOpMask; 

   private:
      PathElement& doSelect( Operation op)
      {
         pushOpMask.reset();
         switch (op)
         {
            case Content:              pushOpMask.match( XMLScannerBase::Content); 
                                       break;
            case Tag:                  pushOpMask.match( XMLScannerBase::Content); 
                                       pushOpMask.match( XMLScannerBase::OpenTag); 
                                       pushOpMask.match( XMLScannerBase::TagAttribName); 
                                       pushOpMask.match( XMLScannerBase::TagAttribValue); 
                                       break;
            case Attribute:            pushOpMask.match( XMLScannerBase::TagAttribName);
            case ThisAttributeValue:   pushOpMask.match( XMLScannerBase::TagAttribValue);
            case AttributeValue:       pushOpMask.match( XMLScannerBase::TagAttribValue);
         }
         return *this;
      };
      PathElement& doSelect( Operation op, const char* value) throw(exception)
      {
         if (xs != 0)
         {
            if (value)
            {
               char buf[ 1024];
               XMLScanner<char*>::size_type size;
               if (!XMLScanner<char*>::getTagName<CharSet_>( value, buf, sizeof(buf), &size))
               {
                  throw exception( IllegalAttributeName);
               }
               stateidx = xs->defNext( stateidx, op, size, buf, value, follow);
            }
            else
            {
               stateidx = xs->defNext( stateidx, op, 0, 0, 0, follow);
            }
         }
         return doSelect( op);
      };
      PathElement& doFollow()
      {
         follow = true;
         return *this;  
      };
      PathElement& doCount( int p_count)
      {
         if (count == -1)
         {
            count = p_count;
         }
         else if (p_count < count)
         {
            count = p_count;
         }
         return *this;
      };
      PathElement& push( int typeidx) throw(exception)
      {
         if (xs != 0) stateidx = xs->defOutput( stateidx, pushOpMask, typeidx, follow, count);
         return *this;
      };

   public:
      PathElement()                                                  :xs(0),stateidx(0),count(-1),follow(false),pushOpMask(0) {};
      PathElement( XMLPathSelectAutomaton* p_xs, int p_stateidx=0)   :xs(p_xs),stateidx(p_stateidx),count(-1),follow(false),pushOpMask(0) {doSelect(Tag);};
      PathElement( const PathElement& orig)                          :xs(orig.xs),stateidx(orig.stateidx),count(orig.count),follow(orig.follow),pushOpMask(orig.pushOpMask) {};

      //corresponds to "//" in abbreviated syntax of XPath
      PathElement& operator --(int)                                                     {return doFollow();};
      //find tag
      PathElement& operator []( const char* name) throw(exception)                      {return doSelect( Tag, name);};
      //find tag with one attribute
      PathElement& operator ()( const char* name) throw(exception)                      {return doSelect( Attribute, name);};
      //find tag with one attribute
      PathElement& operator ()( const char* name, const char* value) throw(exception)   {return doSelect( Attribute, name).doSelect( ThisAttributeValue, value);};
      //define maximum element count to push
      PathElement& operator /(int cnt)   throw(exception)                               {doCount(cnt);};
      //define element type to push
      PathElement& operator =(int type)  throw(exception)                               {return push( type);};
      //grab content
      PathElement& operator ()()  throw(exception)                                      {return doSelect(Content);};
   };
   PathElement operator*()
   {
      return PathElement( this);
   };
};


template <
      class InputIterator,                         //< STL conform input iterator with ++ and read only * returning 0 als last character of the input
      class InputCharSet_=charset::UTF8,           //Character set encoding of the input, read as stream of bytes
      class OutputCharSet_=charset::UTF8,          //Character set encoding of the output, printed as string of the item type of the character set
      class EntityMap=std::map<const char*,UChar>, //< STL like map from ASCII const char* to UChar
      class CharProd=DefaultCharProd
>
class XMLPathSelect :public throws_exception
{
public:
   typedef XMLPathSelectAutomaton<OutputCharSet_> Automaton;
   typedef XMLScanner<InputIterator,InputCharSet_,OutputCharSet_,EntityMap,CharProd> ThisXMLScanner;
   typedef XMLPathSelect<InputIterator,InputCharSet_,OutputCharSet_,EntityMap> ThisXMLPathSelect;
   
private:
   ThisXMLScanner scan;
   Automaton* atm;
   typedef typename Automaton::Mask Mask;
   typedef typename Automaton::Token Token;
   typedef typename Automaton::Hash Hash;
   typedef typename Automaton::State State;
   typedef typename Automaton::Scope Scope;
   
   //static array of POD types. I decided to implement it on my own
   template <typename Element>
   class Array :public throws_exception
   {
      Element* m_ar;
      unsigned int m_size;
      unsigned int m_maxSize;
   public:
      Array( unsigned int p_maxSize) :m_size(0),m_maxSize(p_maxSize)
      {
         m_ar = new (std::nothrow) Element[ m_maxSize];
         if (m_ar == 0) throw exception( OutOfMem);
      };
      ~Array()
      {
         if (m_ar) delete [] m_ar;
      };
      void push_back( const Element& elem)
      {
         if (m_size == m_maxSize) throw exception( OutOfMem);
         m_ar[ m_size++] = elem;
      };
      Element& operator[]( unsigned int idx)
      {
         if (idx >= m_size) throw exception( ArrayBoundsReadWrite);
         return m_ar[ idx];
      };
      void resize( unsigned int p_size)
      {
         if (p_size > m_size) throw exception( ArrayBoundsReadWrite);
         m_size = p_size;
      };
      Element& pop()
      {
         if (m_size == 0) throw exception( ArrayBoundsReadWrite);
         return m_ar[ --m_size];
      };
      unsigned int size() const  {return m_size;};
      bool empty() const         {return m_size==0;};
   };
   
   Array<Scope> scopestk;                   //stack of scopes opened
   Array<unsigned int> follows;             //indices of tokens active in all descendant scopes
   Array<Token> tokens;                     //list of waiting tokens
   
   struct Context
   {
      XMLScannerBase::ElementType type;     //element type processed
      const char* key;                      //string value of element processed
      unsigned int keysize;                 //sizeof string value in bytes of element processed
      Scope scope;                          //active scope
      unsigned int scope_iter;              //position of currently visited token in the active scope
      
      Context()                   :type(XMLScannerBase::Content),key(0),keysize(0) {};
      
      void init( XMLScannerBase::ElementType p_type, const char* p_key, int p_keysize)
      {
         type = p_type;
         key = p_key;
         keysize = p_keysize;
         scope_iter = scope.range.tokenidx_from;
      };
   };
   Context context;

   void expand( int stateidx)
   {
      while (stateidx!=-1)
      {
         const State& st = atm->states[ stateidx];
         context.scope.mask.join( st.core.mask);
         if (st.core.follow) 
         {
            context.scope.followMask.join( st.core.mask);
            follows.push_back( tokens.size());
         }
         tokens.push_back( Token( st, stateidx));
         stateidx = st.link;
      }
   };

   //declares the currently processed element of the XMLScanner input. By calling fetch we get the output elements from it
   void initProcessElement( XMLScannerBase::ElementType type, const char* key, int keysize)
   {
      if (context.type == XMLScannerBase::OpenTag)
      {
         //last step of open scope has to be done after all tokens were visited,
         //e.g. with the next element initialization
         
         context.scope.range.tokenidx_from = context.scope.range.tokenidx_to;
      }
      context.scope.range.tokenidx_to = tokens.size();
      context.scope.range.followidx = follows.size();
      
      context.init( type, key, keysize);

      if (type == XMLScannerBase::OpenTag) 
      {
         //first step of open scope saves the context context on stack
         scopestk.push_back( context.scope);
         context.scope.mask = context.scope.followMask;
         context.scope.mask.match( XMLScannerBase::OpenTag);
         //... we reset the mask but ensure that this 'OpenTag' is processed for sure
      }
      else if (type == XMLScannerBase::CloseTag || type == XMLScannerBase::CloseTagIm)
      {
         if (!scopestk.empty())
         {
            context.scope = scopestk.pop();
            follows.resize( context.scope.range.followidx);
            tokens.resize( context.scope.range.tokenidx_to);
         }
      }
   };
   
   int match( unsigned int tokenidx)
   {
      if (context.key != 0)
      {
         if (tokenidx >= context.scope.range.tokenidx_to) return 0;

         const Token& tk = tokens[ tokenidx];
         if (tk.core.cnt != 0 && tk.core.mask.matches( context.type))
         {
            const State& st = atm->states[ tk.stateidx];
            if (st.key)
            {
               unsigned int ii;
               for (ii=0; ii<context.keysize && st.key[ii] == context.key[ii]; ii++);
               if (ii==context.keysize)
               {
                  expand( st.next);
                  if (tk.core.cnt > 0) tokens[ tokenidx].core.cnt--;
               }
            }
            else
            {
               expand( st.next);
               if (tk.core.cnt > 0) tokens[ tokenidx].core.cnt--;
            }
            if (tk.core.typeidx != 0)
            {
               if (tk.core.cnt > 0) tokens[ tokenidx].core.cnt--;
               return tk.core.typeidx;
            }
         }
         if (tk.core.mask.rejects( context.type))
         {
            //The token must not match anymore after encountering a reject item
            tokens[ tokenidx].core.mask.reset();
         }
      }
      return 0;
   };
   
   int fetch()
   {
      int type = 0;

      if (context.scope.mask.matches( context.type))
      {
         while (!type)
         {
            if (context.scope_iter < context.scope.range.tokenidx_to)
            {
               type = match( context.scope_iter++);
            }
            else 
            {
               unsigned int ii = context.scope_iter - context.scope.range.tokenidx_to;
               //we match all follows that are not yet been checked in the current scope
               if (ii < context.scope.range.followidx
               &&  context.scope.range.tokenidx_from > follows[ ii])
               {
                  type = match( follows[ ii]);
                  context.scope_iter ++;
               }
               else
               {
                  context.key = 0;
                  return 0; //end of all candidates
               }
            }
         }
      }
      else
      {
         context.key = 0;
      }
      return type;
   };

public:
   XMLPathSelect( Automaton* p_atm, InputIterator& src, char* obuf, unsigned int obufsize, EntityMap* entityMap=0)  :scan(src,obuf,obufsize,entityMap),atm(p_atm),scopestk(p_atm->maxScopeStackSize),follows(p_atm->maxFollows),tokens(p_atm->maxTokens)
   {
      if (atm->states.size() > 0) expand(0);
   };
   XMLPathSelect( const XMLPathSelect& o)                                                                           :scan(o.scan),atm(o.atm),scopestk(o.maxScopeStackSize),follows(o.maxFollows),tokens(o.maxTokens) 
   {};

   void setOutputBuffer( char* outputBuf, unsigned int outputBufSize)
   {
      scan.setOutputBuffer( outputBuf, outputBufSize);
   };
   
   //STL conform input iterator for the output of this XMLScanner:   
   struct End {};
   class iterator
   {
   public:
      struct Element
      {
         enum State {Ok,EndOfOutput,EndOfInput,ErrorState};
         State state;
         int type;
         const char* content;
         unsigned int size;

         Element()                     :state(Ok),type(0),content(0),size(0) {};
         Element( const End&)          :state(EndOfInput),type(0),content(0),size(0) {};
         Element( const Element& orig) :state(orig.state),type(orig.type),content(orig.content),size(orig.size) {};
      };
      typedef Element value_type;
      typedef unsigned int difference_type;
      typedef Element* pointer;
      typedef Element& reference;
      typedef std::input_iterator_tag iterator_category;

   private:
      Element element;
      ThisXMLPathSelect* input;

      iterator& skip() throw(exception)
      {
         try
         {
            if (input != 0)
            {
               do
               {
                  if (!input->context.key)
                  {
                     XMLScannerBase::ElementType et = input->scan.nextItem( input->context.scope.mask.pos);
                     if (et == XMLScannerBase::Exit)
                     {
                        element.state = Element::EndOfInput;
                        return *this;
                     }
                     if (et == XMLScannerBase::ErrorOccurred)
                     {
                        XMLScannerBase::Error err = input->scan.getError( &element.content);
                        if (err == XMLScannerBase::ErrOutputBufferTooSmall)
                        {
                           element.state = Element::EndOfOutput;
                        }
                        else
                        {
                           element.state = Element::ErrorState;
                        }
                        return *this;
                     }
                     input->initProcessElement( et, input->scan.getItem(), input->scan.getItemSize());
                  }
                  element.type = input->fetch();

               } while (element.type == 0);

               element.content = input->context.key;
               element.size = input->context.keysize;
            }
            return *this;
         }
         catch (exception e)
         {
            throw exception( e.cause); 
         };
         return *this;
      };
      bool compare( const iterator& iter) const
      {
         return (element.state != Element::Ok && iter.element.state != Element::Ok);
      };
   public:
      void assign( const iterator& orig)
      {
         input = orig.input;
         element.state = orig.element.state;
         element.type = orig.element.type;
         element.content = orig.element.content;
         element.size = orig.element.size;
      };
      iterator( const iterator& orig)
      {
         assign( orig);
      };
      iterator( ThisXMLPathSelect& p_input)
             :input( &p_input)
      {
         skip();
      };
      iterator( const End& et)  :element(et),input(0) {};
      iterator()  :input(0) {};
      iterator& operator = (const iterator& orig)
      {
         assign( orig);
         return *this;
      };    
      const Element& operator*()
      {
         return element;
      };
      const Element* operator->()
      {
         return &element;
      };
      iterator& operator++()     {return skip();};
      iterator operator++(int)   {iterator tmp(*this); skip(); return tmp;};

      bool operator==( const iterator& iter) const   {return compare( iter);};
      bool operator!=( const iterator& iter) const   {return !compare( iter);};
   };
   
   iterator begin()
   {
      return iterator( *this);
   };
   iterator end()
   {
      return iterator( End());
   };
};

} //namespace textwolf
#endif 