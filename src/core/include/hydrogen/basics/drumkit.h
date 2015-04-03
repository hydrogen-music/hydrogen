/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef H2C_DRUMKIT_H
#define H2C_DRUMKIT_H

#include <hydrogen/object.h>

namespace H2Core
{

class XMLNode;
class InstrumentList;
class DrumkitComponent;

/**
 * Drumkit info
*/
class Drumkit : public H2Core::Object
{
		H2_OBJECT
	public:
		/** drumkit constructor, does nothing */
		Drumkit();
		/** copy constructor */
		Drumkit( Drumkit* other );
		/** drumkit destructor, delete__ instruments */
		~Drumkit();

		/**
		 * load drumkit information from a directory
		 * \param dk_dir like one returned by Filesystem::drumkit_path
		 * \param load_samples automatically load sample data if set to true
		 * \return a Drumkit on success, NULL otherwise
		 */
		static Drumkit* load( const QString& dk_dir, bool load_samples=false );
		/**
		 * Simple wrapper for 'load' - use Filesystem::drumkit_path_search
		 */
		static Drumkit* load_by_name( const QString& dk_name, bool load_samples=false );
		/**
		 * load drumkit information from a file
		 * \param dk_path is a path to an xml file
		 * \param load_samples automatically load sample data if set to true
		 * \return a Drumkit on success, NULL otherwise
		 */
		static Drumkit* load_file( const QString& dk_path, bool load_samples=false );
		/**
		 * load the instrument samples
		 */
		void load_samples( );
		/**
		 * unload the instrument samples
		 */
		void unload_samples();
		/**
		 * save a drumkit, xml file and samples
		 * \param overwrite allows to write over existing drumkit files
		 * \return true on success
		 */
		bool save( bool overwrite=false );
		/**
		 * save a drumkit, xml file and samples
		 * neither __path nor __name are updated
		 * \param dk_dir the directory to save the drumkit into
		 * \param overwrite allows to write over existing drumkit files
		 * \return true on success
		 */
		bool save( const QString& dk_dir, bool overwrite=false );
		/**
		 * save a drumkit into an xml file
		 * \param dk_path the path to save the drumkit into
		 * \param overwrite allows to write over existing drumkit file
		 * \return true on success
		 */
		bool save_file( const QString& dk_path, bool overwrite=false );
		/**
		 * save a drumkit instruments samples into a directory
		 * \param dk_dir the directory to save the samples into
		 * \param overwrite allows to write over existing drumkit samples files
		 * \return true on success
		 */
		bool save_samples( const QString& dk_dir, bool overwrite=false );
		/**
		 * save the drumkit image into the new directory
		 * \param dk_dir the directory to save the image into
		 * \param overwrite allows to write over existing drumkit image file
		 * \param orig_dir holds the directory we are copying image from
		 * \return true on success
		 */
		bool save_image( const QString& dk_dir, bool overwrite=false );
		/**
		 * save a drumkit using given parameters and an instrument list
		 * \param name the name of the drumkit
		 * \param author the author of the drumkit
		 * \param info the info of the drumkit
		 * \param license the license of the drumkit
		 * \param image the image filename (with full path) of the drumkit
		 * \þaram instruments the instruments to be saved within the drumkit
		 * \oaram overwrite allows to write over existing drumkit files
		 * \return true on success
		 */
		static bool save( const QString& name, const QString& author, const QString& info, const QString& license, const QString& image, const QString& imageLicense, InstrumentList* instruments, std::vector<DrumkitComponent*>* components, bool overwrite=false );
		/**
		 * install a drumkit from a filename
		 * \param path the path to the new drumkit archive
		 * \return true on success
		 */
		static bool install( const QString& path );
		/**
		 * remove a drumkit from the disk
		 * \param dk_name the drumkit name
		 * \return true on success
		 */
		static bool remove( const QString& dk_name );

		/** set __instruments, delete existing one */
		void set_instruments( InstrumentList* instruments );
		/**  returns __instruments */
		InstrumentList* get_instruments() const;

		/** __path setter */
		void set_path( const QString& path );
		/** __path accessor */
		const QString& get_path() const;
		/** __name setter */
		void set_name( const QString& name );
		/** __name accessor */
		const QString& get_name() const;
		/** __author setter */
		void set_author( const QString& author );
		/** __author accessor */
		const QString& get_author() const;
		/** __info setter */
		void set_info( const QString& info );
		/** __info accessor */
		const QString& get_info() const;
		/** __license setter */
		void set_license( const QString& license );
		/** __license accessor */
		const QString& get_license() const;
		/** __image setter */
		void set_image( const QString& image );
		/** __image accessor */
		const QString& get_image() const;
		/** __imageLicense setter */
		void set_image_license( const QString& imageLicense );
		/** __imageLicense accessor */
		const QString& get_image_license() const;
		/** return true if the samples are loaded */
		const bool samples_loaded() const;

		void dump();

		std::vector<DrumkitComponent*>* get_components();
		void set_components( std::vector<DrumkitComponent*>* components );

	private:
		QString __path;                 ///< absolute drumkit path
		QString __name;                 ///< drumkit name
		QString __author;               ///< drumkit author
		QString __info;                 ///< drumkit free text
		QString __license;              ///< drumkit license description
		QString __image;		///< drumkit image filename
		QString __imageLicense;		///< drumkit image license

		bool __samples_loaded;          ///< true if the instrument samples are loaded
		InstrumentList* __instruments;  ///< the list of instruments
		/*
		 * save the drumkit within the given XMLNode
		 * \param node the XMLNode to feed
		 */
		void save_to( XMLNode* node );
		/**
		 * load a drumkit from an XMLNode
		 * \param node the XMLDode to read from
		 * \param dk_path the directory holding the drumkit data
		 */
		static Drumkit* load_from( XMLNode* node, const QString& dk_path );
    	std::vector<DrumkitComponent*>* __components;  ///< list of drumkit component
};

// DEFINITIONS

inline InstrumentList* Drumkit::get_instruments() const
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

inline void Drumkit::set_license( const QString& license )
{
	__license = license;
}

inline const QString& Drumkit::get_license() const
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

inline void Drumkit::set_image_license( const QString& imageLicense )
{
	__imageLicense = imageLicense;
}

inline const QString& Drumkit::get_image_license() const
{
	return __imageLicense;
}

inline const bool Drumkit::samples_loaded() const
{
	return __samples_loaded;
}

inline std::vector<DrumkitComponent*>* Drumkit::get_components()
{
    return __components;
}

};

#endif // H2C_DRUMKIT_H

/* vim: set softtabstop=4 expandtab: */
