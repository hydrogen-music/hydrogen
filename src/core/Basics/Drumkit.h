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

#include <memory>

#include <core/Object.h>
#include <core/Helpers/Filesystem.h>
#include <core/License.h>
#include <core/Basics/InstrumentList.h>

namespace H2Core
{

class XMLDoc;
class XMLNode;
class DrumkitComponent;

/**
 * Drumkit info
*/
/** \ingroup docCore docDataStructure */
class Drumkit : public H2Core::Object<Drumkit>
{
		H2_OBJECT(Drumkit)
	public:
		/** drumkit constructor, does nothing */
		Drumkit();
		/** copy constructor */
		Drumkit( std::shared_ptr<Drumkit> other );
		/** drumkit destructor, delete #__instruments */
		~Drumkit();

		/**
		 * Load drumkit information from a directory.
		 *
		 * \param dk_dir A directory containing a drumkit,
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
		static std::shared_ptr<Drumkit> load( const QString& dk_dir,
											  bool bUpgrade = true,
											  bool bSilent = false );
		/** Calls the InstrumentList::load_samples() member
		 * function of #__instruments.
		 */
		void load_samples();
		/** Calls the InstrumentList::unload_samples() member
		 * function of #__instruments.
		 */
		void unload_samples();

	/**
	 * Loads the license information of a drumkit contained in
	 * directory @a sDrumkitDir.
	 *
	 * \param sDrumkitDir Directory containing a drumkit.xml file.
	 */
	static License loadLicenseFrom( const QString& sDrumkitDir, bool bSilent = false );
	
	/**
	 * Returns a version of #__name stripped of all whitespaces and
	 * other characters which would prevent its use as a valid
	 * filename.
	 *
	 * Attention: The returned string might be used as the name for
	 * the associated drumkit folder but it does not have to.
	 */
	QString getFolderName() const;
	/**
	 * Returns the base name used when exporting the drumkit.
	 *
	 * \param sComponentName Name of a particular component used in
	 * case just a single component should be exported.
	 * \param bRecentVersion Whether the drumkit format should be
	 * supported by Hydrogen 0.9.7 or higher (whether it should be
	 * composed of DrumkitComponents).
	 */
	QString getExportName( const QString& sComponentName, bool bRecentVersion ) const;
		
		/** 
		 * Saves the current drumkit to dk_path, but makes a backup. 
		 * This is used when the drumkit did not comply to 
		 * our xml schema.
		 */
		static void upgrade_drumkit( std::shared_ptr<Drumkit> pDrumkit,
									 const QString& dk_path,
									 bool bSilent = false );

		/**
		 * Save a drumkit to disk.
		 *
		 * It takes care of writing all parameters etc. into a
		 * drumkit.xml file as well as copying both associated samples
		 * and images.
		 *
		 * \param sDrumkitPath the path (folder) to save the #Drumkit
		 * into. If left empty, the path stored in #__path will be
		 * used instead.
		 * \param nComponentID to chose the component to save or -1 for all
		 * \param bSilent if set to true, all log messages except of
		 * errors and warnings are suppressed.
		 *
		 * \return true on success
		 */
		bool save( const QString& sDrumkitPath = "",
				   int nComponentID = -1,
				   bool bRecentVersion = true,
				   bool bSilent = false );

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
	 * The name of the created file will be a concatenation of #__name and
	 * Filesystem::drumkit_ext.
	 *
	 * exportTo() ? well, export is a protected name within C++. So, we needed a
	 * less obvious name.
	 *
	 * \param sTargetDir Folder which will contain the resulting .h2drumkit
	 *   file.
	 * \param sComponentName Name of a particular component used in case just a
	 *   single component should be exported.
	 * \param bRecentVersion Whether the drumkit format should be supported by
	 *   Hydrogen 0.9.7 or higher (whether it should be composed of
	 *   DrumkitComponents).
	 * \param pUtf8Encoded will be set to true in case we were able to enforce
	 *   'UTF-8' as system locale in `libarchive`. If this didn't work, export
	 *   will be done using classic Latin1 encoded filenames.
	 * \param bSilent Whether debug and info messages should be logged.
	 *
	 * \return true on success 
	 */
	bool exportTo( const QString& sTargetDir, const QString& sComponentName = "",
				   bool bRecentVersion = true, bool* pUtf8Encoded = nullptr,
				   bool bSilent = false );
		/**
		 * remove a drumkit from the disk
		 *
		 * \param sDrumkitDir Path to #Drumkit
		 * \return true on success
		 */
		static bool remove( const QString& sDrumkitDir );

		/** set __instruments, delete existing one */
		void set_instruments( std::shared_ptr<InstrumentList> instruments );
		/**  returns #__instruments */
		std::shared_ptr<InstrumentList> get_instruments() const;

		/** #__path setter */
		void set_path( const QString& path );
		/** #__path accessor */
		const QString& get_path() const;
		/** #__name setter */
		void set_name( const QString& name );
		/** #__name accessor */
		const QString& get_name() const;
		/** #__author setter */
		void set_author( const QString& author );
		/** #__author accessor */
		const QString& get_author() const;
		/** #__info setter */
		void set_info( const QString& info );
		/** #__info accessor */
		const QString& get_info() const;
		/** #__license setter */
		void set_license( const License& license );
		/** #__license accessor */
		const License& get_license() const;
		/** #__image setter */
		void set_image( const QString& image );
		/** #__image accessor */
		const QString& get_image() const;
		/** #__imageLicense setter */
		void set_image_license( const License& imageLicense );
		/** #__imageLicense accessor */
		const License& get_image_license() const;
		/** return true if the samples are loaded */
		const bool samples_loaded() const;

		std::shared_ptr<DrumkitComponent> getComponent( int nID ) const;
		int findUnusedComponentId() const;

		void addComponent( std::shared_ptr<DrumkitComponent> pComponent );
		/** Adds an instrument and takes care of registering `DrumkitComponent`s
		 * missing for contained `InstrumentComponent`s (based on their ID). */
		void addInstrument( std::shared_ptr<Instrument> pInstrument );

	std::shared_ptr<std::vector<std::shared_ptr<DrumkitComponent>>> get_components();
	void set_components( std::shared_ptr<std::vector<std::shared_ptr<DrumkitComponent>>> components );

	/**
	 * Assign the license stored in #m_license to all samples
	 * contained in the kit.
	 */
	void propagateLicense();
	/**
	 * Returns vector of lists containing instrument name, component
	 * name, file name, the license of all associated samples.
	 */
	std::vector<std::shared_ptr<InstrumentList::Content>> summarizeContent() const;
	
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
		QString __path;					///< absolute drumkit path
		QString __name;					///< drumkit name
		QString __author;				///< drumkit author
		QString __info;					///< drumkit free text
		License __license;				///< drumkit license description
		QString __image;				///< drumkit image filename
		License __imageLicense;			///< drumkit image license

		bool __samples_loaded;			///< true if the instrument samples are loaded
		std::shared_ptr<InstrumentList> __instruments;  ///< the list of instruments
	std::shared_ptr<std::vector<std::shared_ptr<DrumkitComponent>>> __components;  ///< list of drumkit component

		/**
		 * save the drumkit image into the new directory
		 * \param dk_dir the directory to save the image into
		 * \param bSilent Whether to suppress info and warning log
		 * level messages.
		 * \return true on success
		 */
	bool save_image( const QString& dk_dir, bool bSilent = false ) const;
		/**
		 * save a drumkit instruments samples into a directory
		 * \param dk_dir the directory to save the samples into
		 * \param bSilent Whether to suppress info and warning log
		 * level messages.
		 * \return true on success
		 */
	bool save_samples( const QString& dk_dir, bool bSilent = false ) const;
		/*
		 * save the drumkit within the given XMLNode
		 * \param node the XMLNode to feed
		 * \param component_id to chose the component to save or -1 for all
		 * \param bRecentVersion Whether the drumkit format should be
		 * supported by Hydrogen 0.9.7 or higher (whether it should be
		 * composed of DrumkitComponents).
		 */
	void save_to( XMLNode* node, int component_id=-1, bool bRecentVersion = true, bool bSilent = false ) const;
		/**
		 * load a drumkit from an XMLNode
		 * \param node the XMLDode to read from
		 * \param dk_path the directory holding the drumkit data
		 * \param pLegacyFormatEncountered will be set to `true` is any of the
		 *   XML elements requires legacy format support and left untouched
		 *   otherwise.
		 * \param bSilent if set to true, all log messages except of
		 * errors and warnings are suppressed.
		 */
	static std::shared_ptr<Drumkit> load_from(
		XMLNode* node,
		const QString& dk_path,
		bool* pLegacyFormatEncountered = nullptr,
		bool bSilent = false );

	/**
	 * Loads the drumkit stored in @a sDrumkitDir into @a pDoc and
	 * takes care of all the error handling.
	 *
	 * \return true on success.
	 */
	static bool loadDoc( const QString& sDrumkitDir, XMLDoc* pDoc, bool bSilent = false );

};

// DEFINITIONS

inline std::shared_ptr<InstrumentList> Drumkit::get_instruments() const
{
	return __instruments;
}

inline void Drumkit::set_path( const QString& path )
{
	__path = path;
}

inline const QString& Drumkit::get_path() const
{
	return __path;
}

inline void Drumkit::set_name( const QString& name )
{
	__name = name;
}

inline const QString& Drumkit::get_name() const
{
	return __name;
}

inline void Drumkit::set_author( const QString& author )
{
	__author = author;
	__license.setCopyrightHolder( author );
	__imageLicense.setCopyrightHolder( author );
}

inline const QString& Drumkit::get_author() const
{
	return __author;
}

inline void Drumkit::set_info( const QString& info )
{
	__info = info;
}

inline const QString& Drumkit::get_info() const
{
	return __info;
}

inline void Drumkit::set_license( const License& license )
{
	__license = license;
}

inline const License& Drumkit::get_license() const
{
	return __license;
}

inline void Drumkit::set_image( const QString& image )
{
	__image = image;
}

inline const QString& Drumkit::get_image() const
{
	return __image;
}

inline void Drumkit::set_image_license( const License& imageLicense )
{
	__imageLicense = imageLicense;
}

inline const License& Drumkit::get_image_license() const
{
	return __imageLicense;
}

inline const bool Drumkit::samples_loaded() const
{
	return __samples_loaded;
}

inline std::shared_ptr<std::vector<std::shared_ptr<DrumkitComponent>>> Drumkit::get_components()
{
	return __components;
}

};

#endif // H2C_DRUMKIT_H

/* vim: set softtabstop=4 noexpandtab: */
