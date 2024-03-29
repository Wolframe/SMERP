//
// FileFormLoader.hpp
//

#ifndef _FILE_FORM_LOADER_HPP_INCLUDED
#define _FILE_FORM_LOADER_HPP_INCLUDED

#include <QObject>
#include <QString>

#include "AbstractFormLoader.hpp"

namespace _SMERP {
	namespace QtClient {

	class FileFormLoader : public AbstractFormLoader
	{
		// intentionally omitting Q_OBJECT here, is done in AbstractFormLoader!
		
		public:
			FileFormLoader( QString dir );
			virtual ~FileFormLoader( ) {};
			virtual void initiateListLoad( );
			virtual void initiateFormLoad( QString &name );
			virtual QStringList getFormNames( );
		
		private:
			QString m_dir;
	};
} // namespace QtClient
} // namespace _SMERP

#endif // _FILE_FORM_LOADER_HPP_INCLUDED
