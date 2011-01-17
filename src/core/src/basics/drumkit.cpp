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

#include <hydrogen/basics/drumkit.h>

#include <hydrogen/config.h>
#ifdef H2CORE_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#else
#ifndef WIN32
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>
#include <libtar.h>
#endif
#endif

#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>

#include <hydrogen/helpers/xml.h>
#include <hydrogen/helpers/filesystem.h>

namespace H2Core {

const char* Drumkit::__class_name = "Drumkit";

Drumkit::Drumkit() : Object( __class_name ), __samples_loaded( false ), __instruments( 0 ) { }

Drumkit::Drumkit( Drumkit* other ) :
    Object( __class_name ),
    __path( other->get_path() ),
    __name( other->get_name() ),
    __author( other->get_author() ),
    __info( other->get_info() ),
    __license( other->get_license() ),
    __samples_loaded( other->samples_loaded() ) {
    __instruments = new InstrumentList( other->get_instruments() );
}

Drumkit::~Drumkit() {
    if( __instruments ) delete __instruments;
}

Drumkit* Drumkit::load( const QString& dk_dir, bool load_samples ) {
    INFOLOG( QString( "Load drumkit %1" ).arg( dk_dir ) );
    if( !Filesystem::drumkit_valid( dk_dir ) ) {
        ERRORLOG( QString( "%1 is not valid drumkit" ).arg( dk_dir ) );
        return 0;
    }
    return load_file( Filesystem::drumkit_file( dk_dir ), load_samples );
}

Drumkit* Drumkit::load_file( const QString& dk_path, bool load_samples ) {
    XMLDoc doc;
    if( !doc.read( dk_path, Filesystem::drumkit_xsd() ) ) return 0;
    XMLNode root = doc.firstChildElement( "drumkit_info" );
    if ( root.isNull() ) {
        ERRORLOG( "drumkit_info node not found" );
        return 0;
    }
    Drumkit* drumkit = Drumkit::load_from( &root, dk_path.left( dk_path.lastIndexOf( "/" ) ) );
    if( load_samples ) drumkit->load_samples();
    return drumkit;
}

Drumkit* Drumkit::load_from( XMLNode* node, const QString& dk_path ) {
    QString drumkit_name = node->read_string( "name", "", false, false );
    if ( drumkit_name.isEmpty() ) {
        ERRORLOG( "Drumkit has no name, abort" );
        return 0;
    }
    Drumkit* drumkit = new Drumkit();
    drumkit->__path = dk_path;
    drumkit->__name = drumkit_name;
    drumkit->__author = node->read_string( "author", "undefined author" );
    drumkit->__info = node->read_string( "info", "defaultInfo" );
    drumkit->__license = node->read_string( "license", "undefined license" );
    drumkit->__samples_loaded = false;
    XMLNode instruments_node = node->firstChildElement( "instrumentList" );
    if ( instruments_node.isNull() ) {
        WARNINGLOG( "instrumentList node not found" );
        drumkit->set_instruments( new InstrumentList() );
    } else {
        drumkit->set_instruments( InstrumentList::load_from( &instruments_node, dk_path ) );
    }
    //if( drumkit->__logger->should_log( Logger::Debug ) ) drumkit->dump();
    return drumkit;
}

bool Drumkit::load_samples( ) {
    INFOLOG( QString( "Loading drumkit %1 instrument samples" ).arg( __name ) );
    if( __samples_loaded ) return true;
    if( __instruments->load_samples() ) { //__path.left( __path.lastIndexOf( "/" ) ) ) ) {
        __samples_loaded = true;
        return true;
    }
    return false;
}

bool Drumkit::unload_samples( ) {
    INFOLOG( QString( "Unloading drumkit %1 instrument samples" ).arg( __name ) );
    if( !__samples_loaded ) return true;
    if( __instruments->unload_samples() ) {
        __samples_loaded = false;
        return true;
    }
    return false;
}

bool Drumkit::save( const QString& name, const QString& author, const QString& info, const QString& license, InstrumentList* instruments, bool overwrite ) {
    Drumkit* drumkit = new Drumkit();
    drumkit->set_name( name );
    drumkit->set_author( author );
    drumkit->set_info( info );
    drumkit->set_license( license );
    drumkit->set_instruments( instruments );
    bool ret = drumkit->save( overwrite );
    drumkit->set_instruments( 0 );
    delete drumkit;
    return ret;
}

bool Drumkit::save( bool overwrite ) {
    INFOLOG( "Saving drumkit" );
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/" + __name;
    if( !Filesystem::mkdir( dk_dir ) ) {
        ERRORLOG( QString( "unable to create %1" ).arg( dk_dir ) );
        return false;
    }
    bool ret = save_file( Filesystem::drumkit_file( dk_dir ), overwrite );
    if ( ret ) {
        ret = save_samples( dk_dir, overwrite );
    }
    return ret;
}

bool Drumkit::save_file( const QString& dk_path, bool overwrite ) {
    INFOLOG( QString( "Saving drumkit into %1" ).arg( dk_path ) );
    if( Filesystem::file_exists( dk_path, true ) && !overwrite ) {
        ERRORLOG( QString( "drumkit %1 already exists" ).arg( dk_path ) );
        return false;
    }
    XMLDoc doc;
    QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" );
    doc.appendChild( header );
    XMLNode root = doc.createElement( "drumkit_info" );
    QDomElement el = root.toElement();
    el.setAttribute( "xmlns",XMLNS_BASE"/drumkit" );
    el.setAttribute( "xmlns:xsi",XMLNS_XSI );
    save_to( &root );
    doc.appendChild( root );
    return doc.write( dk_path );
}

void Drumkit::save_to( XMLNode* node ) {
    node->write_string( "name", __name );
    node->write_string( "author", __author );
    node->write_string( "info", __info );
    node->write_string( "license", __license );
    __instruments->save_to( node );
}

bool Drumkit::save_samples( const QString& dk_dir, bool overwrite ) {
    INFOLOG( QString( "Saving drumkit %1 samples into %2" ).arg( dk_dir ) );
    if( !Filesystem::mkdir( dk_dir ) ) {
        ERRORLOG( QString( "unable to create %1" ).arg( dk_dir ) );
        return false;
    }
    InstrumentList* instruments = get_instruments();
    for( int i=0; i<instruments->size(); i++ ) {
        Instrument* instrument = ( *instruments )[i];
        for ( int n = 0; n < MAX_LAYERS; n++ ) {
            InstrumentLayer* layer = instrument->get_layer( n );
            if( layer ) {
                QString src = __path.left( __path.lastIndexOf( "/" ) ) + "/" + layer->get_sample()->get_filename();
                QString dst = dk_dir + "/" + layer->get_sample()->get_filename();
                if( !Filesystem::file_copy( src, dst ) ) {
                    return false;
                }
            }
        }
    }
    return true;
}

void Drumkit::set_instruments( InstrumentList* instruments ) {
    if( __instruments!=0 ) delete __instruments;
    __instruments = instruments;
}

bool Drumkit::remove( const QString& dk_name ) {
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/" + dk_name;
    if( !Filesystem::drumkit_valid( dk_dir ) ) {
        ERRORLOG( QString( "%1 is not valid drumkit" ).arg( dk_dir ) );
        return false;
    }
    _INFOLOG( QString( "Removing drumkit: %1" ).arg( dk_dir ) );
    if( !Filesystem::rm( dk_dir, true ) ) {
        _ERRORLOG( QString( "Unable to remove drumkit: %1" ).arg( dk_dir ) );
        return false;
    }
    return true;
}

void Drumkit::dump() {
    DEBUGLOG( "Drumkit dump" );
    DEBUGLOG( " |- Path = " + __path );
    DEBUGLOG( " |- Name = " + __name );
    DEBUGLOG( " |- Author = " + __author );
    DEBUGLOG( " |- Info = " + __info );
    DEBUGLOG( " |- Instrument list" );
    for ( int i=0; i<__instruments->size(); i++ ) {
        Instrument* instrument = ( *__instruments )[i];
        DEBUGLOG( QString( "  |- (%1 of %2) Name = %3" )
                  .arg( i )
                  .arg( __instruments->size()-1 )
                  .arg( instrument->get_name() )
                );
        for ( int j=0; j<MAX_LAYERS; j++ ) {
            InstrumentLayer* layer = instrument->get_layer( j );
            if ( layer ) {
                Sample* sample = layer->get_sample();
                if ( sample ) {
                    DEBUGLOG( "   |- " + sample->get_filepath() );
                } else {
                    DEBUGLOG( "   |- NULL sample" );
                }
            }
        }
    }
}

bool Drumkit::install( const QString& path ) {
    _INFOLOG( QString( "Install drumkit %1" ).arg( path ) );
#ifdef H2CORE_HAVE_LIBARCHIVE
    int r;
    struct archive* arch;
    struct archive_entry* entry;
    char newpath[1024];
    arch = archive_read_new();
    archive_read_support_compression_all( arch );
    archive_read_support_format_all( arch );
    if ( ( r = archive_read_open_file( arch, path.toLocal8Bit(), 10240 ) ) ) {
        _ERRORLOG( QString( "archive_read_open_file() [%1] %2" ).arg( archive_errno( arch ) ).arg( archive_error_string( arch ) ) );
        archive_read_close( arch );
        archive_read_finish( arch );
        return false;
    }
    bool ret = true;
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/";
    while ( ( r = archive_read_next_header( arch, &entry ) ) != ARCHIVE_EOF ) {
        if ( r != ARCHIVE_OK ) {
            _ERRORLOG( QString( "archive_read_next_header() [%1] %2" ).arg( archive_errno( arch ) ).arg( archive_error_string( arch ) ) );
            ret = false;
            break;
        }
        QString np = dk_dir + archive_entry_pathname( entry );
        strncpy( newpath, np.toLocal8Bit(), 1024 );
        archive_entry_set_pathname( entry, newpath );
        r = archive_read_extract( arch, entry, 0 );
        if ( r == ARCHIVE_WARN ) {
            _WARNINGLOG( QString( "archive_read_extract() [%1] %2" ).arg( archive_errno( arch ) ).arg( archive_error_string( arch ) ) );
        } else if ( r != ARCHIVE_OK ) {
            _ERRORLOG( QString( "archive_read_extract() [%1] %2" ).arg( archive_errno( arch ) ).arg( archive_error_string( arch ) ) );
            ret = false;
            break;
        }
    }
    archive_read_close( arch );
    archive_read_finish( arch );
    return ret;
#else // H2CORE_HAVE_LIBARCHIVE
#ifndef WIN32
    // GUNZIP
    QString gzd_name = path.left( path.indexOf( "." ) ) + ".tar";
    FILE* gzd_file = fopen( gzd_name.toLocal8Bit(), "wb" );
    gzFile gzip_file = gzopen( path.toLocal8Bit(), "rb" );
    if ( !gzip_file ) {
        _ERRORLOG( QString( "Error reading drumkit file: %1" ).arg( path ) );
        gzclose( gzip_file );
        fclose( gzd_file );
        return false;
    }
    uchar buf[4096];
    while ( gzread( gzip_file, buf, 4096 ) > 0 ) {
        fwrite( buf, sizeof( uchar ), 4096, gzd_file );
    }
    gzclose( gzip_file );
    fclose( gzd_file );
    // UNTAR
    TAR* tar_file;
    char tar_path[1024];
    strncpy( tar_path, gzd_name.toLocal8Bit(), 1024 );
    if ( tar_open( &tar_file, tar_path, NULL, O_RDONLY, 0, TAR_VERBOSE | TAR_GNU ) == -1 ) {
        _ERRORLOG( QString( "tar_open(): %1" ).arg( QString::fromLocal8Bit( strerror( errno ) ) ) );
        return false;
    }
    bool ret = true;
    char dst_dir[1024];
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/";
    strncpy( dst_dir, dk_dir.toLocal8Bit(), 1024 );
    if ( tar_extract_all( tar_file, dst_dir ) != 0 ) {
        _ERRORLOG( QString( "tar_extract_all(): %1" ).arg( QString::fromLocal8Bit( strerror( errno ) ) ) );
        ret = false;
    }
    if ( tar_close( tar_file ) != 0 ) {
        _ERRORLOG( QString( "tar_close(): %1" ).arg( QString::fromLocal8Bit( strerror( errno ) ) ) );
        ret = false;
    }
    return ret;
#else // WIN32
    _ERRORLOG( "WIN32 NOT IMPLEMENTED" );
    return false;
#endif
#endif
}

};

/* vim: set softtabstop=4 expandtab: */
