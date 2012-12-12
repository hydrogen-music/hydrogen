#ifndef SOUNDLIBRARYDATASTRUCTURES_H
#define SOUNDLIBRARYDATASTRUCTURES_H

#include <hydrogen/object.h>
#include <vector>


/**
* @class SoundLibraryInfo
*
* @brief This class holds informations about a soundlibrary..
*
* This class is used to represent soundlibrary items. It contains
* the metadata for songs, pattern and drumkits.
*
* @author Sebastian Moors
*
*/

class SoundLibraryInfo :  public H2Core::Object
{
	H2_OBJECT
	public:
		SoundLibraryInfo();
		~SoundLibraryInfo();

		QString getName() const {
			return m_sName;
		}

		QString getUrl() const{
			return m_sURL;
		}

		QString getInfo() const {
			return m_sInfo;
		}

		QString getAuthor() const {
			return m_sAuthor;
		}

		QString getType() const {
			return m_sType;
		}

		QString getLicense() const {
			return m_sLicense;
		}

		void setName( const QString& name ){
			m_sName = name;
		}

		void setUrl(const QString& url){
			m_sURL = url;
		}

		void setInfo( const QString& info){
			m_sInfo = info;
		}

		void setAuthor( const QString& author ){
			m_sAuthor = author;
		}

		void setType( const QString& type){
			m_sType = type;
		}

		void setLicense( const QString& license ){
			m_sLicense = license;
		}

	private:
		QString m_sName;
		QString m_sURL;
		QString m_sInfo;
		QString m_sAuthor;
		QString m_sType;
		QString m_sLicense;
};

/**
* @class SoundLibraryLocalInfo
*
* @brief This class holds informations about a local soundlibrary..
*
* This class is used to represent soundlibrary items which are kept on disk.
*
* @author Sebastian Moors
*
*/

class SoundLibraryLocalInfo : SoundLibraryInfo
{
	public:
		SoundLibraryLocalInfo();

		void setPath( const QString& path){
			m_sPath = path;
		}

		QString getPath(){
			return m_sPath;
		}

	private:
		QString m_sPath;
};






#endif // SOUNDLIBRARYDATASTRUCTURES_H
