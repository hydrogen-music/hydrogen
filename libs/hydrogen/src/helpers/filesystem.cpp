/*
 * Copyright(c) 2009 by Zurcher Jérémy
 *
 * Hydrogen
 * Copyright(c) 2002-2008 Jonathan Dempsey, Alessandro Cominu
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
 

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hydrogen/config.h>
#include <hydrogen/helpers/filesystem.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
//#include <QApplication>

#ifdef Q_OS_MACX
#include <Carbon.h>
#endif

// directories
#define IMG             "/img"
#define I18N            "/i18n"
#define SONGS           "/songs"
#define PATTERNS        "/patterns"
#define DRUMKITS        "/drumkits"
#define PLAYLISTS       "/playlists"
#define DEMOS           "/demo_songs"

#define USER_CFG_DIR    "/.hydrogen"

// files
#define GUI_CONFIG      "/gui.conf"
#define CORE_CONFIG     "/core.conf"
#define CLICK_SAMPLE    "/click.wav"
#define EMPTY_SAMPLE    "/empty_sample.wav"
#define DEFAULT_SONG    "/empty_song.h2song"

// filters
#define SONG_FILTER     "*.h2song"
#define PATTERN_FILTER  "*.h2pattern"
#define DRUMKIT_XML     "drumkit.xml"

namespace H2Core
{

Logger* Filesystem::__logger = 0;
const char* Filesystem::__class_name = "Filesystem";
QString Filesystem::__sys_data_path;
QString Filesystem::__usr_data_path;

/* TODO qApp doesn't exists should be created within H2Core::Hydrogen::bootstrap( boll gui ); QApplication(argc,argv,gui); */
/* TODO fix user path on start, .hydrogen/data is no more used !! */

bool Filesystem::init( Logger* logger ) {
    if(__logger==0 && logger!=0) {
        __logger = logger;
    }
#ifdef Q_OS_MACX
    #ifdef H2CORE_HAVE_BUNDLE
    //Bundle: Prepare hydrogen to use path names which are used in app bundles: http://en.wikipedia.org/wiki/Application_Bundle
	__sys_data_path = qApp->applicationDirPath().append( "/../Resources/data" ) ;
    #else
	__sys_data_path = qApp->applicationDirPath().append( "/data" ) ;
    #endif
	__usr_data_path = QDir::homePath().append( "/Library/Application Support/Hydrogen" );
#elif WIN32
	__sys_data_path = qApp->applicationDirPath().append( "/data" ) ;
	__usr_data_path = qApp->applicationDirPath().append( "/hydrogen/data" ) ;
#else
	__sys_data_path = DATA_PATH;
	__usr_data_path = QDir::homePath().append( USER_CFG_DIR );
#endif
    return check_sys_paths() && check_usr_paths();
}

bool Filesystem::check_permissions( const QString& path, const int perms, bool silent) {
    QFileInfo fi( path );
    if( (perms & is_file) && (perms & is_writable) && !fi.exists() ) {
        QFileInfo folder( path.left( path.lastIndexOf("/") ) );
        if( !folder.isDir() ) {
            if(!silent) ___ERRORLOG( QString("%1 is not a directory").arg(folder.fileName()) );
            return false;
        }
        if( !folder.isWritable() ) {
            if(!silent) ___ERRORLOG( QString("%1 is not writable").arg(folder.fileName()) );
            return false;
        }
        return true;
    }
    if( (perms & is_dir) && !fi.isDir() ) {
        if(!silent) ___ERRORLOG( QString("%1 is not a directory").arg(path) );
        return false;
    }
    if( (perms & is_file) && !fi.isFile() ) {
        if(!silent) ___ERRORLOG( QString("%1 is not a file").arg(path) );
        return false;
    }
    if( (perms & is_readable) && !fi.isReadable() ) {
        if(!silent) ___ERRORLOG( QString("%1 is not readable").arg(path) );
        return false;
    }
    if( (perms & is_writable) && !fi.isWritable() ) {
        if(!silent) ___ERRORLOG( QString("%1 is not writable").arg(path) );
        return false;
    }
    if( (perms & is_executable) && !fi.isExecutable() ) {
        if(!silent) ___ERRORLOG( QString("%1 is not executable").arg(path) );
        return false;
    }
    return true;
}

bool Filesystem::file_readable( const QString& path, bool silent )  { return check_permissions(path, is_file|is_readable, silent); }
bool Filesystem::file_writable( const QString& path, bool silent )  { return check_permissions(path, is_file|is_writable, silent); }
bool Filesystem::dir_readable(  const QString& path, bool silent )  { return check_permissions(path, is_dir|is_readable|is_executable, silent); }
bool Filesystem::dir_writable(  const QString& path, bool silent )  { return check_permissions(path, is_dir|is_writable, silent); }

bool Filesystem::mkdir( const QString& path ) {
    if ( !QDir("/").mkpath( path ) ) {
        ___ERRORLOG( QString("unable to create directory : %1").arg(path) );
        return false;
    }
    return true;
}

bool Filesystem::path_usable( const QString& path ) {
    if( !QDir( path ).exists() ) {
        ___INFOLOG( QString("create user directory : %1").arg(path) );
        if( !QDir("/").mkpath( path ) ) {
            ___ERRORLOG( QString("unable to create user directory : %1").arg(path) );
            return false;
        }
    }
    return dir_readable( path ) && dir_writable( path );
}

bool Filesystem::write_to_file( const QString& dst, const QString& content ) {
    if ( !file_writable( dst ) ) {
        ___ERRORLOG( QString("unable to write to %1").arg(dst) );
        return false;
    }
	QFile file( dst );
	if ( !file.open( QIODevice::WriteOnly ) ) {
        ___ERRORLOG( QString("unable to write to %1").arg(dst) );
		return false;
    }
    file.write( content.toUtf8().data() );
	file.close();
}

bool Filesystem::file_copy( const QString& src, const QString& dst ) {
    if ( !file_readable( src ) ) {
        ___ERRORLOG( QString("unable to copy %1 to %2, %1 is not readable").arg(src).arg(dst) );
        return false;
    }
    if ( !file_writable( dst ) ) {
        ___ERRORLOG( QString("unable to copy %1 to %2, %2 is not writable").arg(src).arg(dst) );
        return false;
    }
	___INFOLOG( QString("copy %1 to %2").arg(src).arg(dst) );
	return QFile::copy(src,dst);
}

bool Filesystem::rm_fr( const QString& path ) {
    bool ret = true;
    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList( QDir::NoDotAndDotDot | QDir::AllEntries );
    for (int idx = 0; ((idx < entries.size()) && ret); idx++) {
        QFileInfo entryInfo = entries[idx];
        if (entryInfo.isDir() && !entryInfo.isSymLink() ) {
             ret = rm_fr( entryInfo.absoluteFilePath() );
        } else {
            QFile file( entryInfo.absoluteFilePath() );
            if (!file.remove()) {
                ___ERRORLOG( QString("unable to remove %1").arg(entryInfo.absoluteFilePath()) );
                ret = false;
            }
        }
    }
    if (!dir.rmdir(dir.absolutePath())) {
        ___ERRORLOG( QString("unable to remove %1").arg(dir.absolutePath()) );
        ret = false;
    }
    return ret;
}

bool Filesystem::check_sys_paths() {
	if( !QFile(__sys_data_path).exists() ) {
        // TODO maybe quit ??
        ___ERRORLOG( QString("system wide data path : %1 doesn't exists, forced to %2").arg(__sys_data_path).arg(DATA_PATH) );
        __sys_data_path = DATA_PATH;
    }
    if(  !dir_readable( __sys_data_path ) ) return false;
    if(  !dir_readable( __sys_data_path + IMG ) ) return false;
    if(  !dir_readable( __sys_data_path + I18N ) ) return false;
    if(  !dir_readable( __sys_data_path + DEMOS ) ) return false;
    if(  !dir_readable( __sys_data_path + DRUMKITS ) ) return false;
    if( !file_readable( __sys_data_path + CLICK_SAMPLE ) ) return false;
    if( !file_readable( __sys_data_path + EMPTY_SAMPLE ) ) return false;
    if( !file_readable( __sys_data_path + GUI_CONFIG ) ) return false;
    if( !file_readable( __sys_data_path + CORE_CONFIG ) ) return false;
    if( !file_readable( __sys_data_path + DEFAULT_SONG ) ) return false;
    ___INFOLOG( QString("system wide data path %1 is usable.").arg(__sys_data_path) );
    return true;
}


bool Filesystem::check_usr_paths() {
	if( !path_usable( __usr_data_path ) ) return false;
	if( !path_usable( __usr_data_path+SONGS ) ) return false;
	if( !path_usable( __usr_data_path+PATTERNS ) ) return false;
	if( !path_usable( __usr_data_path+DRUMKITS ) ) return false;
	if( !path_usable( __usr_data_path+PLAYLISTS ) ) return false;
    ___INFOLOG( QString("user path %1 is usable.").arg(__usr_data_path) );
    return true;
}

QString Filesystem::sys_data_path()             { return __sys_data_path; }
QString Filesystem::usr_data_path()             { return __usr_data_path; }

// FILES
QString Filesystem::sys_core_config()           { return __sys_data_path + CORE_CONFIG; }
QString Filesystem::usr_core_config()           { return __usr_data_path + CORE_CONFIG; }
QString Filesystem::sys_gui_config()            { return __sys_data_path + GUI_CONFIG; }
QString Filesystem::usr_gui_config()            { return __usr_data_path + GUI_CONFIG; }
QString Filesystem::empty_sample()              { return __sys_data_path + EMPTY_SAMPLE; }
QString Filesystem::empty_song()                { return __sys_data_path + DEFAULT_SONG; }
QString Filesystem::click_file() {
    if(file_readable( __usr_data_path + CLICK_SAMPLE, true )) return __usr_data_path + CLICK_SAMPLE;
    return __sys_data_path + CLICK_SAMPLE;
}

// DIRS
QString Filesystem::img_dir()                   { return __sys_data_path + IMG; }
QString Filesystem::i18n_dir()                  { return __sys_data_path + I18N; }
QString Filesystem::songs_dir()                 { return __usr_data_path + SONGS; }
QString Filesystem::patterns_dir()              { return __usr_data_path + PATTERNS; }
QString Filesystem::sys_drumkits_dir()          { return __sys_data_path + DRUMKITS; }
QString Filesystem::usr_drumkits_dir()          { return __usr_data_path + DRUMKITS; }
QString Filesystem::playlists_dir()             { return __usr_data_path + PLAYLISTS; }
QString Filesystem::demos_dir()                 { return __sys_data_path + DEMOS; }

// DRUMKITS
QStringList Filesystem::drumkits_list( const QString& path )    {
    QStringList ok;
    QStringList possible = QDir( path ).entryList( QDir::Dirs | QDir::NoDotAndDotDot );
    for(int i=0; i<possible.size(); i++) {
        if ( file_readable( path+"/"+possible[i]+"/"+DRUMKIT_XML, true ) )
            ok << possible[i];
        else {
            ___ERRORLOG( QString("drumkit %1 is not usable").arg(path+"/"+possible[i]) );
        }
    }
    return ok;
}
QStringList Filesystem::sys_drumkits_list( )    { return drumkits_list( sys_drumkits_dir() ) ; }
QStringList Filesystem::usr_drumkits_list( )    { return drumkits_list( usr_drumkits_dir() ) ; }
bool Filesystem::drumkit_exists( const QString& dk_name ) {
     if( sys_drumkits_list().contains( dk_name ) ) return true;
     return usr_drumkits_list().contains( dk_name );
}
QString Filesystem::drumkit_path( const QString& dk_name ) {
     if( sys_drumkits_list().contains( dk_name ) ) return sys_drumkits_dir() + "/" + dk_name;
     if( usr_drumkits_list().contains( dk_name ) ) return usr_drumkits_dir() + "/" + dk_name;
     ___ERRORLOG( QString("drumkit %1 not found").arg(dk_name) );
     return "";
}
bool Filesystem::drumkit_valid( const QString& dk_path )   { return file_readable( dk_path + "/" + DRUMKIT_XML ); }
QString Filesystem::drumkit_file( const QString& dk_path ) { return dk_path + "/" + DRUMKIT_XML; }

// PATTERNS
QStringList Filesystem::patterns_list( )    { return QDir( patterns_dir() ).entryList( QStringList(PATTERN_FILTER), QDir::Files | QDir::NoDotAndDotDot ); }

// SONGS
QStringList Filesystem::songs_list( )       { return QDir( songs_dir() ).entryList( QStringList(SONG_FILTER), QDir::Files | QDir::NoDotAndDotDot ); }
bool Filesystem::song_exists( const QString& sg_name ) { return QDir( songs_dir() ).exists( sg_name ); }

void Filesystem::info() {
    ___INFOLOG( QString("System wide core cfg file  : %1").arg( sys_core_config() ) );
    ___INFOLOG( QString("User core cfg file         : %1").arg( usr_core_config() ) );
    ___INFOLOG( QString("System wide gui cfg file   : %1").arg( sys_gui_config() ) );
    ___INFOLOG( QString("User gui cfg file          : %1").arg( usr_gui_config() ) );
    ___INFOLOG( QString("Empty sample               : %1").arg( empty_sample() ) );
    ___INFOLOG( QString("Empty song                 : %1").arg( empty_song() ) );
    ___INFOLOG( QString("Click file                 : %1").arg( click_file() ) );
    ___INFOLOG( QString("Images dir                 : %1").arg( img_dir() ) );
    ___INFOLOG( QString("Internationalization dir   : %1").arg( i18n_dir() ) );
    ___INFOLOG( QString("Songs dir                  : %1").arg( songs_dir() ) );
    ___INFOLOG( QString("Patterns dir               : %1").arg( patterns_dir() ) );
    ___INFOLOG( QString("Playlists dir              : %1").arg( playlists_dir() ) );
    ___INFOLOG( QString("Demos dir                  : %1").arg( demos_dir() ) );
    ___INFOLOG( QString("System drumkit dir         : %1").arg( sys_drumkits_dir() ) );
    ___INFOLOG( QString("User drumkit dir           : %1").arg( usr_drumkits_dir() ) );
}

};

/* vim: set softtabstop=4 expandtab: */
