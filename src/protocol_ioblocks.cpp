#include "protocol.hpp"
#include <cstring>

using namespace _SMERP;
using namespace protocol;

#ifdef _SMERP_LOWLEVEL_DEBUG
static const char* eodStateName( EODState e)
{
   static const char* ar[]={"SRC","LF","LF_DOT","LF_DOT_CR","LF_DOT_CR_LF"}; 
   return ar[e-SRC];
}
#endif

MemBlock::MemBlock()                                   :m_ptr(0),m_size(0),m_pos(0),m_allocated(false) {}
MemBlock::MemBlock( unsigned int p_size)               :m_ptr(0),m_size(p_size),m_pos(0),m_allocated(false)
{
   m_ptr = new unsigned char[ m_size];
   m_allocated = true;
}

MemBlock::MemBlock( void* p_ptr, unsigned int p_size)   :m_ptr(p_ptr),m_size(p_size),m_pos(0),m_allocated(false){}
MemBlock::MemBlock( const MemBlock& o)                  :m_ptr(0),m_size(0),m_pos(0),m_allocated(false) {*this = o;}

MemBlock::~MemBlock()
{
   if (m_allocated) delete [] (unsigned char*)m_ptr;
}

MemBlock& MemBlock::operator=( const MemBlock& o)
{
   if (m_allocated) delete [] (unsigned char*)m_ptr;
   m_size = o.m_size;
   m_pos = o.m_pos;
   m_allocated = o.m_allocated;
   
   if (o.m_allocated)
   {
      m_ptr = new unsigned char[ m_size];
      std::memcpy( m_ptr, o.m_ptr, m_size); 
   }
   else
   {
      m_ptr = o.m_ptr;
   }
   return *this;
}


static void moveInput( char* buf, unsigned int& dstsize, unsigned int& eatsize, unsigned int& bufpos)
{
   if (dstsize != eatsize) std::memmove( buf+dstsize, buf+eatsize, bufpos-eatsize);
   dstsize += bufpos-eatsize;
   eatsize = bufpos;
} 

InputBlock::iterator InputBlock::getEoD( InputBlock::iterator start)
{
   unsigned int offset = start-begin();
   if (size()<=offset) return start;

   unsigned int bufsize = size()-offset;
   char* buf = charptr()+offset;
   unsigned int bufpos=0,eatsize=0,dstsize=0;
   int eodpos = -1;
   
   while (bufpos<bufsize)
   {
      if (m_eodState == EoD::SRC)
      {
         char* cc = (char*)std::memchr( buf+bufpos, '\n', bufsize-bufpos);
         if (cc)
         {
            bufpos = cc - buf + 1;
            if (bufpos == bufsize) moveInput( buf, dstsize, eatsize, bufpos);
            m_eodState = EoD::LF;
         }
         else
         {
            bufpos = bufsize;
            moveInput( buf, dstsize, eatsize, bufpos);
         }
      }
      else if (m_eodState == EoD::LF)
      {
         if (buf[bufpos] == '.')
         {
            m_eodState = EoD::LF_DOT;
            moveInput( buf, dstsize, eatsize, bufpos);
            eatsize = ++bufpos;
         }
         else
         {
            bufpos++;
            if (bufpos == bufsize) moveInput( buf, dstsize, eatsize, bufpos);
            m_eodState = EoD::SRC;
         }
      }
      else if (m_eodState == EoD::LF_DOT)
      {
         if (buf[bufpos] == '\r')
         {            
            eodpos = (int)dstsize++;  //< define EoD
            m_eodState = EoD::LF_DOT_CR;
            bufpos++; 
            eatsize = bufpos;
         }
         else if (buf[bufpos] == '\n')
         {
            eodpos = dstsize++;   //< define EoD
            m_eodState = EoD::LF_DOT_CR_LF;
            bufpos++;
            eatsize = bufpos;
         }
         else
         {
            bufpos++;
            if (bufpos == bufsize) moveInput( buf, dstsize, eatsize, bufpos);
            m_eodState = EoD::SRC;
         }
      }
      else if (m_eodState == EoD::LF_DOT_CR)
      {
         if (buf[bufpos] == '\n') bufpos++; 
         if (bufpos == bufsize) moveInput( buf, dstsize, eatsize, bufpos);
         m_eodState = EoD::LF_DOT_CR_LF;
      }
      else //if (m_eodState == EoD::LF_DOT_CR_LF)
      {
         bufpos = bufsize;
         moveInput( buf, dstsize, eatsize, bufpos);
      }
   }
   setPos( offset+dstsize);   //.. adjust buffer size to the bytes really printed
   if (eodpos >= 0)
   {
      return start+eodpos;
   }
   else
   {
      return end();
   }
}

