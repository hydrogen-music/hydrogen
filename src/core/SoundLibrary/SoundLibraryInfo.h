/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef SOUNDLIBRARYINFO_H
#define SOUNDLIBRARYINFO_H

#include <core/License.h>
#include <core/Object.h>
#include <vector>

namespace H2Core
{

/**
* @class SoundLibraryInfo
*
* @brief This class holds information about a soundlibrary.
*
* This class is used to represent soundlibrary items. It contains
* the metadata for (songs,) pattern, and drumkits.
*
* @author Sebastian Moors
*
*/

/** \ingroup docCore docDataStructure */
class SoundLibraryInfo : public H2Core::Object<SoundLibraryInfo>
{
	H2_OBJECT(SoundLibraryInfo)
	public:
		SoundLibraryInfo();
		~SoundLibraryInfo();

	/**
	 * Reads the content found in @a sPath.
	 *
	 * @param sPath Path to .h2pattern XML file
	 * @return `true` on success
	 */
		bool load( const QString& sPath );

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

		QString getCategory() const {
			return m_sCategory;
		}

		QString getType() const {
			return m_sType;
		}

		H2Core::License getLicense() const {
			return m_license;
		}

		QString getImage() const {
			return m_sImage;
		}

		H2Core::License getImageLicense() const {
			return m_imageLicense;
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

		void setCategory( const QString& category){
			m_sCategory = category;
		}

		void setLicense( const H2Core::License& license ){
			m_license = license;
		}

		void setImage( const QString& image ){
			m_sImage = image;
		}

		void setImageLicense( const H2Core::License& imageLicense ){
			m_imageLicense = imageLicense;
		}

		void setPath( const QString& path){
			m_sPath = path;
		}

		QString getPath(){
			return m_sPath;
		}
	
		void setDrumkitName( const QString& sDrumkitName ){
			m_sDrumkitName = sDrumkitName;
		}
		QString getDrumkitName(){
			return m_sDrumkitName;
		}

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

	private:
		QString m_sName;
		QString m_sURL;
		QString m_sInfo;
		QString m_sAuthor;
		QString m_sCategory;
		QString m_sType;
		H2Core::License m_license;
		QString m_sImage;
		H2Core::License m_imageLicense;
		QString m_sPath;

	/** Drumkit the pattern was created with */
	QString m_sDrumkitName;
};
}; // namespace H2Core

#endif // SOUNDLIBRARYINFO_H
