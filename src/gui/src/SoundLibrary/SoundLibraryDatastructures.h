/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef SOUNDLIBRARYDATASTRUCTURES_H
#define SOUNDLIBRARYDATASTRUCTURES_H

#include <core/Object.h>
#include <vector>

class SoundLibraryInfo;

/**
* @class SoundLibraryDatabase
*
* @brief This class holds information about all installed soundlibrary items.
*
* This class organizes the metadata of all locally installed soundlibrary items.
*
* @author Sebastian Moors
*
*/

typedef std::vector<SoundLibraryInfo*> soundLibraryInfoVector;

class SoundLibraryDatabase:  public H2Core::Object
{
	H2_OBJECT
	public:
		SoundLibraryDatabase();
		~SoundLibraryDatabase();

		//bool isItemInstalled( const SoundLibraryInfo& item );
		soundLibraryInfoVector* getAllPatterns() const;
		QStringList getAllPatternCategories() const {
			return patternCategories;
		}

		void update();
		void updatePatterns();
		void printPatterns();
		void getPatternFromDirectory(const QString& path, soundLibraryInfoVector* );
		bool isPatternInstalled( const QString& patternName);

		static void create_instance();
		static SoundLibraryDatabase* get_instance() { assert(__instance); return __instance; }


	private:
		static SoundLibraryDatabase *__instance;
		soundLibraryInfoVector* patternVector;
		QStringList patternCategories;
};


/**
* @class SoundLibraryInfo
*
* @brief This class holds information about a soundlibrary.
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
		explicit SoundLibraryInfo( const QString& path);
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

		QString getCategory() const {
			return m_sCategory;
		}

		QString getType() const {
			return m_sType;
		}

		QString getLicense() const {
			return m_sLicense;
		}

		QString getImage() const {
			return m_sImage;
		}

		QString getImageLicense() const {
			return m_sImageLicense;
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

		void setLicense( const QString& license ){
			m_sLicense = license;
		}

		void setImage( const QString& image ){
			m_sImage = image;
		}

		void setImageLicense( const QString& imageLicense ){
			m_sImageLicense = imageLicense;
		}

		void setPath( const QString& path){
			m_sPath = path;
		}

		QString getPath(){
			return m_sPath;
		}


	private:
		QString m_sName;
		QString m_sURL;
		QString m_sInfo;
		QString m_sAuthor;
		QString m_sCategory;
		QString m_sType;
		QString m_sLicense;
		QString m_sImage;
		QString m_sImageLicense;
		QString m_sPath;
};

#endif // SOUNDLIBRARYDATASTRUCTURES_H
