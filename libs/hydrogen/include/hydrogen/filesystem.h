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

class Filesystem : public Object
{
    H2_OBJECT
    public:
        static bool init( Logger* logger );

        static QString sys_data_path();
        static QString usr_data_path();

        static QString sys_core_config();
        static QString usr_core_config();
        static QString sys_gui_config();
        static QString usr_gui_config();
        static QString empty_sample();
        static QString empty_song();
        static QString click_file();

        static QString img_dir();
        static QString i18n_dir();
        static QString songs_dir();
        static QString patterns_dir();
        static QString sys_drumkits_dir();
        static QString usr_drumkits_dir();
        static QString playlists_dir();
        static QString demos_dir();

        static QStringList sys_drumkits_list( );
        static QStringList usr_drumkits_list( );
        static bool drumkit_exists( const QString& filename );

        static void show();

        static bool file_readable( const QString& path );
        static bool file_writable( const QString& path );
        static bool dir_readable( const QString& path );
        static bool path_usable( const QString& path );
        static bool write_to_file( const QString& path, const QString& content );

    private:
        static Logger* __logger;
        static bool check_sys_paths();
        static bool check_usr_paths();

        static QString __sys_data_path;
        static QString __usr_data_path;
};

};

#endif  // H2_FILESYSTEM_H

/* vim: set softtabstop=4 expandtab: */
