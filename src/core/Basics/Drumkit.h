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

#ifndef H2C_DRUMKIT_H
#define H2C_DRUMKIT_H

#include <map>
#include <set>
#include <memory>

#include <core/Basics/DrumkitMap.h>
#include <core/Basics/InstrumentList.h>
#include <core/CoreActionController.h>
#include <core/License.h>
#include <core/Object.h>

namespace H2Core
{

class Instrument;
class XMLDoc;
class XMLNode;

/**
 * Drumkit info
*/
/** \ingroup docCore docDataStructure */
class Drumkit : public H2Core::Object<Drumkit>
{
		H2_OBJECT(Drumkit)
	public:

	/** Indicates usage, storage, and access permissions of a kit.*/
	enum class Context {
		/** Kit is located in the system-level drumkit folder, loaded into the
		 * #H2Core::SoundlibraryDatabase during startup, and is read-only.*/
		System = 0,
		/** Kit is located in the user-level drumkit folder, loaded into the
		 * #H2Core::SoundlibraryDatabase during startup, and can be modified.*/
		User = 1,
		/** Kit is located at an arbitrary location of the host system and was
		 * loaded into Hydrogen during a session using e.g. OSC or its location
		 * was provided during startup. It is transient and located in a place
		 * the user only has read-only access and can not be modified.*/
		SessionReadOnly = 2,
		/** Kit is located at an arbitrary location of the host system and was
		 * loaded into Hydrogen during a session using e.g. OSC or its location
		 * was provided during startup. It is transient and can be modified.*/
		SessionReadWrite = 3,
		/** In contrast to the other contexts this drumkit was not loaded from a
		 * .h2drumkit or a drumkit.xml file within a drumkit folder. Instead, it
		 * is part of a song and loaded with a .h2song or created with a new
		 * song. It is stored with the song when saving the song and can be
		 * converted into a regular kit by saving / exporting the drumkit. All
		 * its metadata, like drumkit image, end up in a cache folder for
		 * Hydrogen.*/
		Song = 4
	};
		static QString ContextToString( const Context& context );
		static Context DetermineContext( const QString& sPath );

		/** drumkit constructor, does nothing */
		Drumkit();
		/** copy constructor */
		Drumkit( std::shared_ptr<Drumkit> pOther );
		/** drumkit destructor, delete #m_pInstruments */
		~Drumkit();

		static std::shared_ptr<Drumkit> getEmptyDrumkit();

		/**
		 * Load drumkit information from a directory.
		 *
		 * \param sDrumkitDir A directory containing a drumkit,
		 * like those returned by
		 * Filesystem::drumkit_dir_search().
		 * \param bUpgrade Whether the loaded drumkit should be
		 * upgraded using upgrade_drumkit() in case it did not comply
		 * with the current XSD file.
		 * \param bSilent if set to true, all log messages except of
		 * errors and warnings are suppressed.
		 *
		 * \return A Drumkit on success, nullptr otherwise.
		 */
		static std::shared_ptr<Drumkit> load( const QString& sDrumkitDir,
											  bool bUpgrade = true,
											  bool bSilent = false );

		/**
		 * load a drumkit from an XMLNode
		 *
		 * \param pNode the XMLDode to read from
		 * \param sPath the directory holding the drumkit data
		 * @param sSongPath If not empty, absolute path to the .h2song file the
		 *   drumkit is contained in. It is used to resolve sample paths
		 *   relative to the .h2song file.
		 * @param bSongKit If true samples are loaded on a
		 *   per-instrument basis. If the filename of the sample is a plain
		 *   filename, it will be searched for in the folder associated with the
		 *   drumkit named in "drumkit" (name for portability) and "drumkitPath"
		 *   (unique identifier locally). If it is an absolute path, it will be
		 *   loaded directly. This mode corresponds to loading the Drumkit as
		 *   part of a song (which allows composition of a drumkit from various
		 *   kits and new instruments/samples). If `false`, it corresponds to
		 *   the kit being loaded as part of the `SoundLibraryDatabase`.
		 * \param pLegacyFormatEncountered will be set to `true` is any of the
		 *   XML elements requires legacy format support and left untouched
		 *   otherwise.
		 * \param bSilent if set to true, all log messages except of
		 * errors and warnings are suppressed.
		 */
		static std::shared_ptr<Drumkit> loadFrom( const XMLNode& pNode,
												  const QString& sPath,
												  const QString& sSongPath,
												  bool bSongKit = false,
												  bool* pLegacyFormatEncountered = nullptr,
												  bool bSilent = false );

		/*
		 * save the drumkit within the given XMLNode
		 *
		 * \param pNode the XMLNode to feed
		 * \param bSongKit Whether the instruments are part of a
		 *   stand-alone kit or part of a song. In the latter case all samples
		 *   located in the corresponding drumkit folder and are referenced by
		 *   filenames. In the former case, each instrument might be
		 *   associated with a different kit and the lookup folder for the
		 *   samples are stored on a per-instrument basis.
		 */
		void saveTo( XMLNode& pNode,
					 bool bSongKit = false,
					 bool bSilent = false ) const;


		/**
		 * Save a drumkit to disk.
		 *
		 * It takes care of writing all parameters etc. into a
		 * drumkit.xml file as well as copying both associated samples
		 * and images.
		 *
		 * \param sDrumkitDir the path (folder) to save the #Drumkit
		 * into. If left empty, the path stored in #m_sPath will be
		 * used instead.
		 * \param bSilent if set to true, all log messages except of
		 * errors and warnings are suppressed.
		 *
		 * \return true on success
		 */
		bool save( const QString& sDrumkitDir = "",
				   bool bSilent = false );


		/** Calls the InstrumentList::loadSamples() member
		 * function of #m_pInstruments.
		 */
		void loadSamples( float fBpm = 120 );
		/** Calls the InstrumentList::unloadSamples() member
		 * function of #m_pInstruments.
		 */
		void unloadSamples();
		/** return true if the samples are loaded */
		const bool areSamplesLoaded() const;
		bool hasMissingSamples() const;

	/**
	 * Returns the base name used when exporting the drumkit.
	 *
	 * This is a version of #m_sName stripped of all whitespaces and other
	 * characters which would prevent its use as a valid filename.
	 *
	 * Attention: The returned string might be used as the name for
	 * the associated drumkit folder but it does not have to.
	 */
	QString getExportName() const;

		/**
		 * Extract a .h2drumkit file.
		 *
		 * \param sSourcePath Absolute path to the new drumkit archive
		 * \param sTargetPath Absolute path to where the new drumkit should be
		 *   extracted to. If left empty, the user's drumkit folder will be
		 *   used.
		 * \param pInstalledPath Will contain the actual name of the folder the
		 *   kit was installed to. In most cases this will coincide with a
		 *   folder within @a sTargetPath named like the kit itself. But in case
		 *   the system does not support UTF-8 encoding and @a sTargetPath
		 *   contains characters other than those whitelisted in
		 *   #Filesystem::removeUtf8Characters, those might be omitted and the
		 *   directory and files created using `libarchive` might differ.
		 * \param pEncodingIssuesDetected will be set to `true` in case at least
		 *   one filepath of extracted kit had to be altered in order to not run
		 *   into UTF-8 issues.
		 * \param bSilent Whether debug and info messages should be logged.
		 *
		 * \return true on success
		 */
	static bool install( const QString& sSourcePath,
						 const QString& sTargetPath = "",
						 QString* pInstalledPath = nullptr,
						 bool* pEncodingIssuesDetected = nullptr,
						 bool bSilent = false );

	/**
	 * Compresses the drumkit into a .h2drumkit file.
	 *
	 * The name of the created file will be a concatenation of #m_sName and
	 * Filesystem::drumkit_ext.
	 *
	 * exportTo() ? well, export is a protected name within C++. So, we needed a
	 * less obvious name.
	 *
	 * \param sTargetDir Folder which will contain the resulting .h2drumkit
	 *   file.
	 * \param pUtf8Encoded will be set to true in case we were able to enforce
	 *   'UTF-8' as system locale in `libarchive`. If this didn't work, export
	 *   will be done using classic Latin1 encoded filenames.
	 * \param bSilent Whether debug and info messages should be logged.
	 *
	 * \return true on success
	 */
	bool exportTo( const QString& sTargetDir, bool* pUtf8Encoded = nullptr,
				   bool bSilent = false );

		/** set m_pInstruments, delete existing one */
		void setInstruments( std::shared_ptr<InstrumentList> instruments );

		/**  returns #m_pInstruments */
		std::shared_ptr<InstrumentList> getInstruments() const;

		void setContext( const Context& context );
		const Context& getContext() const;
		/** #m_sPath setter */
		void setPath( const QString& path );
		/** #m_sPath accessor */
		const QString& getPath() const;
		/** #m_sName setter */
		void setName( const QString& name );
		/** #m_sName accessor */
		const QString& getName() const;
		/** #m_sAuthor setter */
		int getVersion() const;
		void setVersion( int nVersion );
		void setAuthor( const QString& author );
		/** #m_sAuthor accessor */
		const QString& getAuthor() const;
		/** #m_sInfo setter */
		void setInfo( const QString& info );
		/** #m_sInfo accessor */
		const QString& getInfo() const;
		/** #m_license setter */
		void setLicense( const License& license );
		/** #m_license accessor */
		const License& getLicense() const;
		/** #m_sImage setter */
		void setImage( const QString& image );
		/** #m_sImage accessor */
		const QString& getImage() const;
		/** As #m_sImage can be a plain file name specifying an image file
		 * located in the drumkit folder or an absolute path to an image
		 * somewhere on the users disk (in our cache folder in case of song
		 * kits), this function allows to access it in a common way.*/
		QString getAbsoluteImagePath() const;
		/** #m_imageLicense setter */
		void setImageLicense( const License& imageLicense );
		/** #m_imageLicense accessor */
		const License& getImageLicense() const;

	/**
	 * Returns vector of lists containing instrument name, component
	 * name, file name, the license of all associated samples.
	 */
	std::vector<std::shared_ptr<InstrumentList::Content>> summarizeContent() const;

		/** Recalculates all Samples using RubberBand for a specific
		* tempo @a fBpm.
		*
		* This function requires the calling function to lock the
		* #AudioEngine first.
		*/
		void recalculateRubberband( float fBpm );

		/** Returns all types of the contained instruments. */
		std::set<DrumkitMap::Type> getAllTypes() const;

		/** In case no instrument types are defined in the loaded drumkit, we
		 * check whether there is a .h2map file shipped with the installation
		 * corresponding to getExportName() of the kit. From this is retrieve
		 * only missing types.*/
		void fixupTypes( bool bSilent = false );

		bool hasMissingTypes() const;

		std::shared_ptr<DrumkitMap> toDrumkitMap() const;

		/** This method tell whether an instrument of another kit (specified via
		 * @a sType and @a nInstrumentId) will be mapped to an instrument of
		 * this kit.
		 *
		 * This is vital when switching drumkits or importing patterns. If
		 * known, the former kit @a pOldDrumkit should be provided. This allows
		 * to check whether the provided instrument is indeed a different one or
		 * basically the same with just its type being edited. If so, the new
		 * type will be stored in @a pNewType. */
		std::shared_ptr<Instrument> mapInstrument( const QString& sType,
												   int nInstrumentId,
												   std::shared_ptr<Drumkit> pOldDrumkit = nullptr,
												   DrumkitMap::Type* pNewType = nullptr );

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

		friend bool CoreActionController::addInstrument(
			std::shared_ptr<Instrument>, int );
		friend bool CoreActionController::removeInstrument(
			std::shared_ptr<Instrument> );
		friend bool CoreActionController::replaceInstrument(
			std::shared_ptr<Instrument>, std::shared_ptr<Instrument> );

	private:
		/** Transient property neither written to a drumkit.xml nor to a .h2song
		 * but determined when loading the kit. */
		Context m_context;
		QString m_sPath;					///< absolute drumkit path
		QString m_sName;					///< drumkit name
		int m_nVersion;
		QString m_sAuthor;				///< drumkit author
		QString m_sInfo;					///< drumkit free text
		License m_license;				///< drumkit license description
		QString m_sImage;				///< drumkit image filename
		License m_imageLicense;			///< drumkit image license

		std::shared_ptr<InstrumentList> m_pInstruments;  ///< the list of instruments


		/** Add an instrument to the kit*/
		void addInstrument( std::shared_ptr<Instrument> pInstrument,
							int nIndex = -1 );

		/** Removes an instrument from the drumkit. */
		void removeInstrument( std::shared_ptr<Instrument> pInstrument );

		/**
		 * save the drumkit image into the new directory
		 * \param dk_dir the directory to save the image into
		 * \param bSilent Whether to suppress info and warning log
		 * level messages.
		 * \return true on success
		 */
	bool saveImage( const QString& dk_dir, bool bSilent = false ) const;
		/**
		 * save a drumkit instruments samples into a directory
		 * \param dk_dir the directory to save the samples into
		 * \param bSilent Whether to suppress info and warning log
		 * level messages.
		 * \return true on success
		 */
	bool saveSamples( const QString& dk_dir, bool bSilent = false ) const;

		/**
		 * Upgrades the drumkit by saving the latest version.
		 *
		 * This is a wrapper around #H2Core::Drumkit::save() which also creates
		 * a backup of the drumkit definition.
		 */
		void upgrade( bool bSilent = false );

	/**
	 * Assign the license stored in #m_license to all samples
	 * contained in the kit.
	 */
	void propagateLicense();

		/** Used to indicate changes in the underlying XSD file. */
		static constexpr int nCurrentFormatVersion = 2;

};

// DEFINITIONS

inline std::shared_ptr<InstrumentList> Drumkit::getInstruments() const
{
	return m_pInstruments;
}

inline void Drumkit::setContext( const Drumkit::Context& context ) {
	m_context = context;
}
inline const Drumkit::Context& Drumkit::getContext() const {
	return m_context;
}

inline void Drumkit::setPath( const QString& path )
{
	m_sPath = path;
}

inline void Drumkit::setName( const QString& name )
{
	m_sName = name;
}

inline const QString& Drumkit::getName() const
{
	return m_sName;
}
inline int Drumkit::getVersion() const {
	return m_nVersion;
}
inline void Drumkit::setVersion( int nVersion ) {
	m_nVersion = nVersion;
}

inline void Drumkit::setAuthor( const QString& author )
{
	m_sAuthor = author;
	m_license.setCopyrightHolder( author );
	m_imageLicense.setCopyrightHolder( author );
}

inline const QString& Drumkit::getAuthor() const
{
	return m_sAuthor;
}

inline void Drumkit::setInfo( const QString& info )
{
	m_sInfo = info;
}

inline const QString& Drumkit::getInfo() const
{
	return m_sInfo;
}

inline void Drumkit::setLicense( const License& license )
{
	m_license = license;
}

inline const License& Drumkit::getLicense() const
{
	return m_license;
}

inline void Drumkit::setImage( const QString& image )
{
	m_sImage = image;
}

inline const QString& Drumkit::getImage() const
{
	return m_sImage;
}

inline void Drumkit::setImageLicense( const License& imageLicense )
{
	m_imageLicense = imageLicense;
}

inline const License& Drumkit::getImageLicense() const
{
	return m_imageLicense;
}

};

#endif // H2C_DRUMKIT_H

/* vim: set softtabstop=4 noexpandtab: */
