
#include <hydrogen/config.h>
#include <hydrogen/helpers/filesystem.h>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

// directories
#define LOCAL_DATA_PATH "/data"
#define IMG             "/img"
#define DOC             "/doc"
#define I18N            "/i18n"
#define SONGS           "/songs"
#define PATTERNS        "/patterns"
#define DRUMKITS        "/drumkits"
#define PLAYLISTS       "/playlists"
#define DEMOS           "/demo_songs"
#define XSD             "/xsd"
#define TMP             "/hydrogen"

// files
#define GUI_CONFIG      "/gui.conf"
#define CORE_CONFIG     "/core.conf"
#define CLICK_SAMPLE    "/click.wav"
#define EMPTY_SAMPLE    "/emptySample.wav"
#define EMPTY_SONG      "/DefaultSong.h2song"

// filters
#define SONG_FILTER     "*.h2song"
#define PATTERN_FILTER  "*.h2pattern"
#define DRUMKIT_XML     "drumkit.xml"
#define DRUMKIT_XSD     "drumkit.xsd"
#define DRUMPAT_XSD     "drumkit_pattern.xsd"
#define PATTERN_XSD     "pattern.xsd"

namespace H2Core
{

Logger* Filesystem::__logger = 0;
const char* Filesystem::__class_name = "Filesystem";
QString Filesystem::__sys_data_path;
QString Filesystem::__usr_data_path;

/* TODO QCoreApplication is not instanciated */
bool Filesystem::bootstrap( Logger* logger, const QString& sys_path )
{
    if( __logger==0 && logger!=0 ) {
        __logger = logger;
    } else {
        return false;
    }

#ifdef Q_OS_MACX
#ifdef H2CORE_HAVE_BUNDLE
    //Bundle: Prepare hydrogen to use path names which are used in app bundles: http://en.wikipedia.org/wiki/Application_Bundle
    __sys_data_path = QCoreApplication::applicationDirPath().append( "/../Resources/data" ) ;
#else
    __sys_data_path = QCoreApplication::applicationDirPath().append( "/data" ) ;
#endif
    __usr_data_path = QDir::homePath().append( "/Library/Application Support/Hydrogen" );
#elif WIN32
    __sys_data_path = QCoreApplication::applicationDirPath().append( "/data" ) ;
    __usr_data_path = QCoreApplication::applicationDirPath().append( "/hydrogen/data" ) ;
#else
    __sys_data_path = SYS_DATA_PATH;
    __usr_data_path = QDir::homePath().append( "/"USR_DATA_PATH );
#endif
    if( sys_path!=0 ) __sys_data_path = sys_path;

    if( !dir_readable( __sys_data_path ) ) {
        __sys_data_path = QCoreApplication::applicationDirPath().append( LOCAL_DATA_PATH );
        ERRORLOG( QString( "will use local data path : %1" ).arg( __sys_data_path ) );
    }
    return check_sys_paths() && check_usr_paths();
}

bool Filesystem::check_permissions( const QString& path, const int perms, bool silent )
{
    QFileInfo fi( path );
    if( ( perms & is_file ) && ( perms & is_writable ) && !fi.exists() ) {
        QFileInfo folder( path.left( path.lastIndexOf( "/" ) ) );
        if( !folder.isDir() ) {
            if( !silent ) ERRORLOG( QString( "%1 is not a directory" ).arg( folder.fileName() ) );
            return false;
        }
        if( !folder.isWritable() ) {
            if( !silent ) ERRORLOG( QString( "%1 is not writable" ).arg( folder.fileName() ) );
            return false;
        }
        return true;
    }
    if( ( perms & is_dir ) && !fi.isDir() ) {
        if( !silent ) ERRORLOG( QString( "%1 is not a directory" ).arg( path ) );
        return false;
    }
    if( ( perms & is_file ) && !fi.isFile() ) {
        if( !silent ) ERRORLOG( QString( "%1 is not a file" ).arg( path ) );
        return false;
    }
    if( ( perms & is_readable ) && !fi.isReadable() ) {
        if( !silent ) ERRORLOG( QString( "%1 is not readable" ).arg( path ) );
        return false;
    }
    if( ( perms & is_writable ) && !fi.isWritable() ) {
        if( !silent ) ERRORLOG( QString( "%1 is not writable" ).arg( path ) );
        return false;
    }
    if( ( perms & is_executable ) && !fi.isExecutable() ) {
        if( !silent ) ERRORLOG( QString( "%1 is not executable" ).arg( path ) );
        return false;
    }
    return true;
}

bool Filesystem::file_exists( const QString& path, bool silent )
{
    return check_permissions( path, is_file, silent );
}
bool Filesystem::file_readable( const QString& path, bool silent )
{
    return check_permissions( path, is_file|is_readable, silent );
}
bool Filesystem::file_writable( const QString& path, bool silent )
{
    return check_permissions( path, is_file|is_writable, silent );
}
bool Filesystem::file_executable( const QString& path, bool silent )
{
    return check_permissions( path, is_file|is_executable, silent );
}
bool Filesystem::dir_readable(  const QString& path, bool silent )
{
    return check_permissions( path, is_dir|is_readable|is_executable, silent );
}
bool Filesystem::dir_writable(  const QString& path, bool silent )
{
    return check_permissions( path, is_dir|is_writable, silent );
}

bool Filesystem::mkdir( const QString& path )
{
    if ( !QDir( "/" ).mkpath( QDir( path ).absolutePath() ) ) {
        ERRORLOG( QString( "unable to create directory : %1" ).arg( path ) );
        return false;
    }
    return true;
}

bool Filesystem::path_usable( const QString& path, bool create, bool silent )
{
    if( !QDir( path ).exists() ) {
        if( !silent ) INFOLOG( QString( "create user directory : %1" ).arg( path ) );
        if( create && !QDir( "/" ).mkpath( path ) ) {
            if( !silent ) ERRORLOG( QString( "unable to create user directory : %1" ).arg( path ) );
            return false;
        }
    }
    return dir_readable( path, silent ) && dir_writable( path, silent );
}

bool Filesystem::write_to_file( const QString& dst, const QString& content )
{
    if ( !file_writable( dst ) ) {
        ERRORLOG( QString( "unable to write to %1" ).arg( dst ) );
        return false;
    }
    QFile file( dst );
    if ( !file.open( QIODevice::WriteOnly ) ) {
        ERRORLOG( QString( "unable to write to %1" ).arg( dst ) );
        return false;
    }
    file.write( content.toUtf8().data() );
    file.close();
}

bool Filesystem::file_copy( const QString& src, const QString& dst, bool overwrite )
{
    if( file_exists( dst, true ) && !overwrite ) {
        WARNINGLOG( QString( "do not overwrite %1 with %2 has it already exists" ).arg( dst ).arg( src ) );
        return true;
    }
    if ( !file_readable( src ) ) {
        ERRORLOG( QString( "unable to copy %1 to %2, %1 is not readable" ).arg( src ).arg( dst ) );
        return false;
    }
    if ( !file_writable( dst ) ) {
        ERRORLOG( QString( "unable to copy %1 to %2, %2 is not writable" ).arg( src ).arg( dst ) );
        return false;
    }
    INFOLOG( QString( "copy %1 to %2" ).arg( src ).arg( dst ) );
    return QFile::copy( src,dst );
}

bool Filesystem::rm( const QString& path, bool recursive )
{
    if ( check_permissions( path, is_file, true ) ) {
        QFile file( path );
        bool ret = file.remove();
        if( !ret ) {
            ERRORLOG( QString( "unable to remove file %1" ).arg( path ) );
        }
        return ret;
    }
    if ( !check_permissions( path, is_dir, true )  ) {
        ERRORLOG( QString( "%1 is neither a file nor a directory ?!?!" ).arg( path ) );
        return false;
    }
    if ( !recursive ) {
        ERRORLOG( QString( "unable to remove directory %1 without recursive parameter set to true" ).arg( path ) );
        return false;
    }
    return rm_fr( path );
}

bool Filesystem::rm_fr( const QString& path )
{
    bool ret = true;
    QDir dir( path );
    QFileInfoList entries = dir.entryInfoList( QDir::NoDotAndDotDot | QDir::AllEntries );
    for ( int idx = 0; ( ( idx < entries.size() ) && ret ); idx++ ) {
        QFileInfo entryInfo = entries[idx];
        if ( entryInfo.isDir() && !entryInfo.isSymLink() ) {
            ret = rm_fr( entryInfo.absoluteFilePath() );
        } else {
            QFile file( entryInfo.absoluteFilePath() );
            if ( !file.remove() ) {
                ERRORLOG( QString( "unable to remove %1" ).arg( entryInfo.absoluteFilePath() ) );
                ret = false;
            }
        }
    }
    if ( !dir.rmdir( dir.absolutePath() ) ) {
        ERRORLOG( QString( "unable to remove %1" ).arg( dir.absolutePath() ) );
        ret = false;
    }
    return ret;
}

bool Filesystem::check_sys_paths()
{
    if(  !dir_readable( __sys_data_path ) ) return false;
    if(  !dir_readable( img_dir() ) ) return false;
    if(  !dir_readable( xsd_dir() ) ) return false;
    if(  !dir_readable( doc_dir() ) ) return false;
    if(  !dir_readable( i18n_dir() ) ) return false;
    if(  !dir_readable( demos_dir() ) ) return false;
    if( !file_readable( click_file() ) ) return false;
    if( !file_readable( empty_song() ) ) return false;
    if( !file_readable( empty_sample() ) ) return false;
    if( !file_readable( sys_gui_config() ) ) return false;
    if( !file_readable( sys_core_config() ) ) return false;
    if(  !dir_readable( sys_drumkits_dir() ) ) return false;
    if( !file_readable( drumkit_xsd() ) ) return false;
    if( !file_readable( drumkit_pattern_xsd() ) ) return false;
    if( !file_readable( pattern_xsd() ) ) return false;
    INFOLOG( QString( "system wide data path %1 is usable." ).arg( __sys_data_path ) );
    return true;
}


bool Filesystem::check_usr_paths()
{
    if( !path_usable( __usr_data_path ) ) return false;
    if( !path_usable( songs_dir() ) ) return false;
    if( !path_usable( patterns_dir() ) ) return false;
    if( !path_usable( playlists_dir() ) ) return false;
    if( !path_usable( usr_drumkits_dir() ) ) return false;
    INFOLOG( QString( "user path %1 is usable." ).arg( __usr_data_path ) );
    return true;
}

QString Filesystem::sys_data_path()
{
    return __sys_data_path;
}
QString Filesystem::usr_data_path()
{
    return __usr_data_path;
}

// FILES
QString Filesystem::sys_core_config()
{
    return __sys_data_path + CORE_CONFIG;
}
QString Filesystem::usr_core_config()
{
    return __usr_data_path + CORE_CONFIG;
}
QString Filesystem::sys_gui_config()
{
    return __sys_data_path + GUI_CONFIG;
}
QString Filesystem::usr_gui_config()
{
    return __usr_data_path + GUI_CONFIG;
}
QString Filesystem::empty_sample()
{
    return __sys_data_path + EMPTY_SAMPLE;
}
QString Filesystem::empty_song()
{
    return __sys_data_path + EMPTY_SONG;
}
QString Filesystem::click_file()
{
    return __sys_data_path + CLICK_SAMPLE;
}
QString Filesystem::usr_click_file()
{
    if( file_readable( __usr_data_path + CLICK_SAMPLE, true ) ) return __usr_data_path + CLICK_SAMPLE;
    return click_file();
}
QString Filesystem::drumkit_xsd( )
{
    return xsd_dir() + "/" + DRUMKIT_XSD;
}
QString Filesystem::drumkit_pattern_xsd( )
{
    return xsd_dir() + "/" + DRUMPAT_XSD;
}
QString Filesystem::pattern_xsd( )
{
    return xsd_dir() + "/" + PATTERN_XSD;
}

// DIRS
QString Filesystem::img_dir()
{
    return __sys_data_path + IMG;
}
QString Filesystem::doc_dir()
{
    return __sys_data_path + DOC;
}
QString Filesystem::i18n_dir()
{
    return __sys_data_path + I18N;
}
QString Filesystem::songs_dir()
{
    return __usr_data_path + SONGS;
}
QString Filesystem::patterns_dir()
{
    return __usr_data_path + PATTERNS;
}
QString Filesystem::sys_drumkits_dir()
{
    return __sys_data_path + DRUMKITS;
}
QString Filesystem::usr_drumkits_dir()
{
    return __usr_data_path + DRUMKITS;
}
QString Filesystem::playlists_dir()
{
    return __usr_data_path + PLAYLISTS;
}
QString Filesystem::demos_dir()
{
    return __sys_data_path + DEMOS;
}
QString Filesystem::xsd_dir()
{
    return __sys_data_path + XSD;
}
QString Filesystem::tmp_dir()
{
    return QDir::tempPath() + TMP;
}
QString Filesystem::tmp_file( const QString& base )
{
    QTemporaryFile file( tmp_dir()+"/"+base );
    file.setAutoRemove( false );
    file.open();
    file.close();
    return file.fileName();
}

// DRUMKITS
QStringList Filesystem::drumkits_list( const QString& path )
{
    QStringList ok;
    QStringList possible = QDir( path ).entryList( QDir::Dirs | QDir::NoDotAndDotDot );
    for( int i=0; i<possible.size(); i++ ) {
        if ( file_readable( path+"/"+possible[i]+"/"+DRUMKIT_XML, true ) )
            ok << possible[i];
        else {
            ERRORLOG( QString( "drumkit %1 is not usable" ).arg( path+"/"+possible[i] ) );
        }
    }
    return ok;
}
QStringList Filesystem::sys_drumkits_list( )
{
    return drumkits_list( sys_drumkits_dir() ) ;
}
QStringList Filesystem::usr_drumkits_list( )
{
    return drumkits_list( usr_drumkits_dir() ) ;
}
bool Filesystem::drumkit_exists( const QString& dk_name )
{
    if( usr_drumkits_list().contains( dk_name ) ) return true;
    return sys_drumkits_list().contains( dk_name );
}
QString Filesystem::drumkit_usr_path( const QString& dk_name )
{
    return usr_drumkits_dir() + "/" + dk_name;
}
QString Filesystem::drumkit_path_search( const QString& dk_name )
{
    if( usr_drumkits_list().contains( dk_name ) ) return usr_drumkits_dir() + "/" + dk_name;
    if( sys_drumkits_list().contains( dk_name ) ) return sys_drumkits_dir() + "/" + dk_name;
    ERRORLOG( QString( "drumkit %1 not found" ).arg( dk_name ) );
    return "";
}
QString Filesystem::drumkit_dir_search( const QString& dk_name )
{
    if( usr_drumkits_list().contains( dk_name ) ) return usr_drumkits_dir();
    if( sys_drumkits_list().contains( dk_name ) ) return sys_drumkits_dir();
    ERRORLOG( QString( "drumkit %1 not found" ).arg( dk_name ) );
    return "";
}
bool Filesystem::drumkit_valid( const QString& dk_path )
{
    return file_readable( dk_path + "/" + DRUMKIT_XML );
}
QString Filesystem::drumkit_file( const QString& dk_path )
{
    return dk_path + "/" + DRUMKIT_XML;
}

// PATTERNS
QStringList Filesystem::patterns_list( )
{
    return QDir( patterns_dir() ).entryList( QStringList( PATTERN_FILTER ), QDir::Files | QDir::NoDotAndDotDot );
}

// SONGS
QStringList Filesystem::songs_list( )
{
    return QDir( songs_dir() ).entryList( QStringList( SONG_FILTER ), QDir::Files | QDir::NoDotAndDotDot );
}
bool Filesystem::song_exists( const QString& sg_name )
{
    return QDir( songs_dir() ).exists( sg_name );
}

void Filesystem::info()
{
    INFOLOG( QString( "Tmp dir                    : %1" ).arg( tmp_dir() ) );
    INFOLOG( QString( "Images dir                 : %1" ).arg( img_dir() ) );
    INFOLOG( QString( "Documentation dir          : %1" ).arg( doc_dir() ) );
    INFOLOG( QString( "Internationalization dir   : %1" ).arg( i18n_dir() ) );
    INFOLOG( QString( "Demos dir                  : %1" ).arg( demos_dir() ) );
    INFOLOG( QString( "XSD dir                    : %1" ).arg( xsd_dir() ) );
    INFOLOG( QString( "System drumkit dir         : %1" ).arg( sys_drumkits_dir() ) );
    INFOLOG( QString( "System wide core cfg file  : %1" ).arg( sys_core_config() ) );
    INFOLOG( QString( "System wide gui cfg file   : %1" ).arg( sys_gui_config() ) );
    INFOLOG( QString( "Empty sample               : %1" ).arg( empty_sample() ) );
    INFOLOG( QString( "Empty song                 : %1" ).arg( empty_song() ) );
    INFOLOG( QString( "Click file                 : %1" ).arg( click_file() ) );
    INFOLOG( QString( "User drumkit dir           : %1" ).arg( usr_drumkits_dir() ) );
    INFOLOG( QString( "Songs dir                  : %1" ).arg( songs_dir() ) );
    INFOLOG( QString( "Patterns dir               : %1" ).arg( patterns_dir() ) );
    INFOLOG( QString( "Playlists dir              : %1" ).arg( playlists_dir() ) );
    INFOLOG( QString( "User core cfg file         : %1" ).arg( usr_core_config() ) );
    INFOLOG( QString( "User gui cfg file          : %1" ).arg( usr_gui_config() ) );
}

};

/* vim: set softtabstop=4 expandtab: */
