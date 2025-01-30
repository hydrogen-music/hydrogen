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
#include <core/Basics/DrumkitMap.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>
#include <core/Object.h>
#include <QStringList>
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

		/** Null element of the category list*/
		static QString m_sPatternBaseCategory;

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
	 * If the kit is not already present, it will be loaded from disk.
	 *
	 * @param sDrumkitPath Absolute path to the drumkit directory
	 *   (containing a drumkit.xml) file as unique identifier.
	 */
	std::shared_ptr<Drumkit> getDrumkit( const QString& sDrumkitPath );

		/** Based on #Song::m_sLastLoadedDrumkitPath get the previous drumkit in
		 * the data base (the one shown above the last loaded one in the Sound
		 * Library widget) */
		std::shared_ptr<Drumkit> getPreviousDrumkit() const;
		/** Based on #Song::m_sLastLoadedDrumkitPath get the next drumkit in the
		 * data base (the one shown below the last loaded one in the Sound
		 * Library widget) */
		std::shared_ptr<Drumkit> getNextDrumkit() const;

	const std::map<QString, std::shared_ptr<Drumkit>>& getDrumkitDatabase() const {
		return m_drumkitDatabase;
	}
		/** Retrieves an unique label for the kit associated with @a
		 * sDrumkitPath. This may serve as a more accessible alternative to the
		 * absolute path of the kit in the GUI. */
		QString getUniqueLabel( const QString& sDrumkitPath ) const;

		/** Add a custom folder #SoundLibraryDatabase will look of drumkits in
		 * during an updateDrumkits()
		 *
		 * @param sDrumkitFolder Absolute path. */
		void registerDrumkitFolder( const QString& sDrumkitFolder );

		QStringList getDrumkitFolders() const;

	/** Retrieves all #H2Core::DrumkitMap::Type found in the registered
	 * drumkits.
	 *
	 * @return The list of unique types sorted alphabetically.*/
	 std::set<DrumkitMap::Type> getAllTypes() const;
	
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
		void registerUniqueLabel( const QString& sDrumkitPath,
								  std::shared_ptr<Drumkit> pDrumkit );

	std::map<QString, std::shared_ptr<Drumkit>> m_drumkitDatabase;
		/** The absolute path to a drumkit folder is not the most accessible way
		 * to refer to a kit in the GUI. Instead, each kit will also have an
		 * unique label. It is derived from the name of the drumkit. But as
		 * there can be duplicates, the following rules are applied:
		 * - Drumkit of system-level will carry the suffix " (system)"
		 * - Drumkit added during a session but not installed in the system or
		 *   user drumkit folder will carry the suffix " (session)"
		 * - If a label is already present a number will be appended, like " (1)"
		 * */
		std::map<QString, QString> m_drumkitUniqueLabels;

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

		/** Whole folders that will be scanned for drumkits in addition to the
		 * system and user drumkti folder. */
		QStringList m_customDrumkitFolders;
};
}; // namespace H2Core

#endif // SOUNDLIBRARYDATASTRUCTURES_H
