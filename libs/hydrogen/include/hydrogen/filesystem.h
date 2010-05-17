/*
 * Copyright(c) 2009 by Zurcher Jérémy
 *
 * Hydrogen
 * Copyright(c) 2002-200/ Alessandro Cominu
 *
 * http://www.hydrogen-music.org
 *
 * Header to define the path to the data files for Hydrogen in such a
 * way that self-contained Mac OS X application bundles can be built.
 * Copyright (c) 2005 Jonathan Dempsey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef H2_FILESYSTEM_H
#define H2_FILESYSTEM_H

#include <hydrogen/Object.h>
#include <QtCore/QString>

namespace H2Core
{

/**
\ingroup H2Core
\brief	Filesystem is a thin layer over QDir, QFile and QFileInfo
*/
class Filesystem : public Object
{
    H2_OBJECT
    public:
	    typedef enum _perms {
            is_dir =0x01,
            is_file=0x02,
            is_readable=0x04,
            is_writable=0x08,
            is_executable=0x10
        } file_perms;
        /** \brief check user and system filesystem usability
         * \param logger is a pointer to the logger instance which will be used
         * \return true on success
         */
        static bool init( Logger* logger );

        /** \brief returns system data path */
        static QString sys_data_path();
        /** \brief returns user data path */
        static QString usr_data_path();

        /** \brief returns system core config path */
        static QString sys_core_config();
        /** \brief returns user core config path */
        static QString usr_core_config();
        /** \brief returns system gui config path */
        static QString sys_gui_config();
        /** \brief returns user gui config path */
        static QString usr_gui_config();
        /** \brief returns system empty sample file path */
        static QString empty_sample();
        /** \brief returns system empty song file path */
        static QString empty_song();
        /** \brief returns click file path from user directory if exists, otherwise from system */
        static QString click_file();

        /** \brief returns gui image path */
        static QString img_dir();
        /** \brief returns internationalization path */
        static QString i18n_dir();
        /** \brief returns user songs path */
        static QString songs_dir();
        /** \brief returns user patterns path */
        static QString patterns_dir();
        /** \brief returns system drumkits path */
        static QString sys_drumkits_dir();
        /** \brief returns user drumkits path */
        static QString usr_drumkits_dir();
        /** \brief returns user playlist path */
        static QString playlists_dir();
        /** \brief returns system demos path */
        static QString demos_dir();

        /* DRUMKIT */
        /** \brief returns list of usable system drumkits ( see Filesystem::drumkits_list ) */
        static QStringList sys_drumkits_list( );
        /** \brief returns list of usable user drumkits ( see Filesystem::drumkits_list ) */
        static QStringList usr_drumkits_list( );
        /** \brief returns true if the drumkit exists within usable system or user drumkits */
        static bool drumkit_exists( const QString& dk_name );
        /** \brief returns path for a drumkit searching within usable system then user drumkits */
        static QString drumkit_path( const QString& dk_name );
        /** \brief returns true if the path contains a usable drumkit */
        static bool drumkit_valid( const QString& dk_path );
        /** \brief returns the path to the xml file within a suposed drumkit path */
        static QString drumkit_file( const QString& dk_path );
        
        /* PATTERNS */
        /** \brief returns */
        static QStringList patterns_list( );

        /* SONGS */
        /** \brief returns */
        static QStringList songs_list( );
        /** \brief returns */
        static bool song_exists( const QString& sg_name );

        /** \brief send current settings information to logger with INFO severity */
        static void info();

        /** \brief returns true if the given path is an existing readable regular file */
        static bool file_readable( const QString& path, bool silent=false );
        /** \brief returns true if the given path is a possibly writable file (may exist or not) */
        static bool file_writable( const QString& path, bool silent=false );
        /** \brief returns true if the given path is a readable regular directory */
        static bool dir_readable( const QString& path, bool silent=false );
        /** \brief returns true if the given path is a writable regular directory */
        static bool dir_writable( const QString& path, bool silent=false );
        /** \brief returns true if the path is a readable and writable regular directory, create if it not exists */
        static bool path_usable( const QString& path );
        /** \brief writes to a file
         * \param dst the destination path
         * \param content then string to write
         * \return true on success
         * */
        static bool write_to_file( const QString& dst, const QString& content );
        /** \brief copy a source file to a destination */
        static bool file_copy( const QString& src, const QString& dst );
        /** \brief recursively remove a path */
        static bool rm_fr( const QString& path );
        /** \brief create a path */
        static bool mkdir( const QString& path );

    private:
        static Logger* __logger;            ///< a pointer to the logger
        static bool check_sys_paths();      ///< returns true if the system path is consistent
        static bool check_usr_paths();      ///< returns true if the user path is consistent

        /** \brief returns a list of usable drumkits, which means having a readable drumkit.xml file */
        static QStringList drumkits_list( const QString& path );
        static bool check_permissions( const QString& path, const int perms, bool silent );  ///< return true if all the asked permissions are ok

        static QString __sys_data_path;     ///< the path to the system files
        static QString __usr_data_path;     ///< the path to the user files
};

};

#endif  // H2_FILESYSTEM_H

/* vim: set softtabstop=4 expandtab: */
