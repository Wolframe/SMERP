//
// version.cpp
//

#include <string>
#include <sstream>
#include "version.hpp"

namespace _SMERP {

Version::Version( unsigned short M, unsigned short m )
{
	major_ = M, minor_ = m;
	revision_ = build_ = 0;
	hasRevision_ = hasBuild_ = false;
}

Version::Version( unsigned short M, unsigned short m, unsigned short r )
{
	major_ = M, minor_ = m, revision_ = r;
	build_ = 0;
	hasRevision_ = true;
	hasBuild_ = false;
}

Version::Version( unsigned short M, unsigned short m, unsigned short r, unsigned b )
{
	major_ = M, minor_ = m, revision_ = r, build_ = b;
	hasRevision_ = hasBuild_ = true;
}


bool Version::operator== ( const Version &other ) const
{
	if ( major_ != other.major_ )		return false;
	if ( minor_ != other.minor_ )		return false;
	if ( revision_ != other.revision_ )	return false;
	if ( build_ != other.build_ )		return false;
	return true;
}

bool Version::operator> ( const Version &other ) const
{
	if ( major_ > other.major_ )		return true;
	if ( major_ < other.major_ )		return false;
	if ( minor_ > other.minor_ )		return true;
	if ( minor_ < other.minor_ )		return false;
	if ( revision_ > other.revision_ )	return true;
	if ( revision_ < other.revision_ )	return false;
	if ( build_ > other.build_ )		return true;

	return false;
}


std::string Version::toString()
{
	std::ostringstream	o;

	o << major_ << "." << minor_;
	if ( hasRevision_ )	{
		o << "." << revision_;
		if ( hasBuild_ )	{
			o << "." << build_;
		}
	}
	return o.str();
}

} // namespace _SMERP

