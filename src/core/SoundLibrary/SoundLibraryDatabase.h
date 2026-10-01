/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef SOUND_LIBRARY_DATABASE_H
#define SOUND_LIBRARY_DATABASE_H

#include <QStringList>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Event.h>
#include <core/Object.h>

namespace H2Core
{
/**
 * @class SoundLibraryDatabase
 *
 * @brief This class holds information about all installed soundlibrary items.
 *
 * This class organizes the metadata of all locally installed soundlibrary
 * items.
 *
 * @author Sebastian Moors
 *
 */

class Hydrogen;
class SoundLibraryInfo;

/** \ingroup docGUI*/
class SoundLibraryDatabase : public H2Core::Object<SoundLibraryDatabase> {
	H2_OBJECT( SoundLibraryDatabase )
   public:
	/** Immutable, coherent view of all sound library content.
	 *
	 * A fresh snapshot is published by every content-affecting
	 * operation (update(), updateDrumkits(), updatePatterns(),
	 * updateSongs(), a lazy getDrumkit() insert, and the custom
	 * drumkit registrations). Holding a #std::shared_ptr to a
	 * snapshot allows lock-free reads for an arbitrary duration:
	 * later publications do not affect it. */
	struct Snapshot {
		/** Loaded drumkits keyed by the absolute path of their
		 * drumkit.xml file. */
		std::map<QString, std::shared_ptr<Drumkit>> drumkitDatabase;
		std::vector<std::shared_ptr<SoundLibraryInfo>> drumkitInfos;
		std::vector<std::shared_ptr<SoundLibraryInfo>> patternInfos;
		std::vector<std::shared_ptr<SoundLibraryInfo>> songInfos;
		/** Exact drumkit paths registered via
		 * #registerCustomDrumkitPath (e.g. supplied via CLI or OSC
		 * command or lazily loaded via getDrumkit()). They are not
		 * part of the scanned folders and would be lost by a plain
		 * folder rescan. */
		QStringList customDrumkitPaths;
		/** Whole folders scanned for drumkits in addition to the
		 * system and user drumkit folders (e.g. NSM session
		 * folders, registered via #registerDrumkitFolder). */
		QStringList customDrumkitFolders;
	};

	/** @param pHydrogen Owning Hydrogen instance; stored as the back-pointer
	 * through which the database reaches its per-instance context (ADR 0015). */
	SoundLibraryDatabase( Hydrogen* pHydrogen );
	~SoundLibraryDatabase();

	/** Currently published snapshot. */
	std::shared_ptr<const Snapshot> getSnapshot() const;

	/** Copies of the current snapshot's content. For repeated or
	 * cross-container accesses prefer #getSnapshot() for a coherent
	 * view. */
	std::vector<std::shared_ptr<SoundLibraryInfo>> getDrumkitInfos() const;
	std::vector<std::shared_ptr<SoundLibraryInfo>> getPatternInfos() const;
	std::vector<std::shared_ptr<SoundLibraryInfo>> getSongInfos() const;
	std::map<QString, std::shared_ptr<Drumkit>> getDrumkitDatabase() const;

	void update();

	void updateDrumkits( Event::Trigger trigger );
	/**
	 * Retrieve a drumkit from the database.
	 *
	 * If the kit is not already present, it will be loaded from disk.
	 *
	 * @param sDrumkitPath Absolute path to the drumkit.xml file holding the
	 *   definition of the drumkit. If empty, it will be read from @a pNode.
	 * @param bUpgrade In case the drumkit is not part of the DB and needs to be
	 *   loaded, should it be upgrade while doing so?
	 */
	std::shared_ptr<Drumkit>
	getDrumkit( const QString& sDrumkitPath, bool bUpgrade = true );

	/** Based on #Song::m_sLastLoadedDrumkitPath get the previous drumkit in
	 * the data base (the one shown above the last loaded one in the Sound
	 * Library widget) */
	std::shared_ptr<Drumkit> getPreviousDrumkit() const;
	/** Based on #Song::m_sLastLoadedDrumkitPath get the next drumkit in the
	 * data base (the one shown below the last loaded one in the Sound
	 * Library widget) */
	std::shared_ptr<Drumkit> getNextDrumkit() const;

	/** Add a custom folder #SoundLibraryDatabase will look of drumkits in
	 * during an updateDrumkits()
	 *
	 * @param sDrumkitFolder Absolute path. */
	void registerDrumkitFolder( const QString& sDrumkitFolder );

	QStringList getDrumkitFolders() const;
	QStringList getCustomDrumkitFolders() const;

	/** Returns the list of individual drumkit paths registered via
	 * #registerCustomDrumkitPath (not folder-scanned — exact kit paths). */
	QStringList getCustomDrumkitPaths() const;

	/** Register an individual drumkit path to be scanned on the next
	 * updateDrumkits(). Unlike #registerDrumkitFolder (which scans a whole
	 * folder), this points at a single drumkit directory. */
	void registerCustomDrumkitPath( const QString& sPath );

	/** Retrieves all #H2Core::Instrument::Type found in the registered
	 * drumkits.
	 *
	 * @return The list of unique types sorted alphabetically.*/
	std::set<Instrument::Type> getAllTypes() const;

	void updatePatterns( Event::Trigger trigger );
	void updateSongs( Event::Trigger trigger );

	/** Checks whether an artifact of type @a artifact holding the name @a
	 * sName exists in context @a context and returns the full path to the
	 * first artifact matching.
	 *
	 * In @a bStacked retrieval @a context is ignored and the we will search 1.
	 * session, 2. user, and 3. system context for the artifact. */
	QString findArtifact(
		Filesystem::Artifact artifact,
		Filesystem::Context context,
		const QString& sName,
		bool bStacked = false
	) const;

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	/** Back-pointer to the owning Hydrogen instance (ADR 0015). */
	Hydrogen* m_pHydrogen;

	/** Guards the publication of #m_pSnapshot. Building new content
	 * happens without the lock; only the merge of the custom drumkit
	 * registrations and the pointer swap are done while holding
	 * it. */
	mutable std::mutex m_writerMutex;
	/** Currently published snapshot. Never null. */
	std::shared_ptr<const Snapshot> m_pSnapshot;

	/** Ensure the label of @a pInfo is unique within the containers
	 * of @a pSnapshot (the snapshot under construction). */
	void registerUniqueLabel(
		std::shared_ptr<SoundLibraryInfo> pInfo,
		Snapshot* pSnapshot
	);

	/** Merge the custom drumkit registrations of the currently
	 * published snapshot into @a pNewSnapshot and publish the
	 * latter.
	 *
	 * Registrations added while @a pNewSnapshot was being built
	 * (e.g. a lazy getDrumkit() insert or a
	 * registerCustomDrumkitPath() call) must not be lost to the
	 * rebuild. */
	void publish( std::shared_ptr<Snapshot> pNewSnapshot );
};
inline QStringList SoundLibraryDatabase::getCustomDrumkitFolders() const
{
	return getSnapshot()->customDrumkitFolders;
}
inline QStringList SoundLibraryDatabase::getCustomDrumkitPaths() const
{
	return getSnapshot()->customDrumkitPaths;
}
};	// namespace H2Core

#endif	// SOUND_LIBRARY_DATABASE_H
