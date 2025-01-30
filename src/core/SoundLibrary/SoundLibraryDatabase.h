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

#ifndef SOUNDLIBRARYDATASTRUCTURES_H
#define SOUNDLIBRARYDATASTRUCTURES_H

#include <core/Basics/Drumkit.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>
#include <core/Object.h>
#include <map>
#include <memory>
#include <vector>

namespace H2Core
{
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

/** \ingroup docGUI*/
class SoundLibraryDatabase :    public H2Core::Object<SoundLibraryDatabase>
{
	H2_OBJECT(SoundLibraryDatabase)
	public:
	SoundLibraryDatabase();
	~SoundLibraryDatabase();

	std::vector<std::shared_ptr<SoundLibraryInfo>> getPatternInfoVector() const {
		return m_patternInfoVector;
	}
	QStringList getPatternCategories() const {
		return m_patternCategories;
	}

	void update();

	void updateDrumkits( bool bTriggerEvent = true );
	void updateDrumkit( const QString& sDrumkitPath, bool bTriggerEvent = true );
	/**
	 * Retrieve a drumkit from the database.
	 *
	 * @param sDrumkitPath Absolute path to the drumkit directory
	 *   (containing a drumkit.xml) file as unique identifier.
	 * @param bLoad Whether the drumkit should be loaded into the
	 *   datebase in case it is not present yet.
	 */
	std::shared_ptr<Drumkit> getDrumkit( const QString& sDrumkitPath, bool bLoad = true );
	const std::map<QString,std::shared_ptr<Drumkit>> getDrumkitDatabase() const {
		return m_drumkitDatabase;
	}
	
	void updatePatterns( bool bTriggerEvent = true );
	void printPatterns() const;
	void loadPatternFromDirectory( const QString& path );
	bool isPatternInstalled( const QString& sPatternName ) const;

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
	std::map<QString,std::shared_ptr<Drumkit>> m_drumkitDatabase;
	
	std::vector<std::shared_ptr<SoundLibraryInfo>> m_patternInfoVector;
	QStringList m_patternCategories;

	/**
	 * List of drumkits the user supplied via CLI or OSC command but
	 * couldn't be found in either the system's or user's drumkit
	 * folders. This drumkit might still be present an valid. But it
	 * would be lost upon updating when just checking the
	 * aforementioned folders.
	 */
	QStringList m_customDrumkitPaths;
};
}; // namespace H2Core

#endif // SOUNDLIBRARYDATASTRUCTURES_H
