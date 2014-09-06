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
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_layer.h>

#include <hydrogen/helpers/xml.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/helpers/legacy.h>

namespace H2Core
{

const char* Drumkit::__class_name = "Drumkit";

Drumkit::Drumkit() : Object( __class_name ), __samples_loaded( false ), __instruments( 0 ), __components( NULL )
{
    __components = new std::vector<DrumkitComponent*> ();
}

Drumkit::Drumkit( Drumkit* other ) :
	Object( __class_name ),
	__path( other->get_path() ),
	__name( other->get_name() ),
	__author( other->get_author() ),
	__info( other->get_info() ),
	__license( other->get_license() ),
	__samples_loaded( other->samples_loaded() ),
	__components( NULL )
{
	__instruments = new InstrumentList( other->get_instruments() );

    __components = new std::vector<DrumkitComponent*> ();
    __components->assign( other->get_components()->begin(), other->get_components()->end() );

}

Drumkit::~Drumkit()
{
    __components->clear();
    delete __components;

	if( __instruments ) delete __instruments;
}

Drumkit* Drumkit::load_by_name ( const QString& dk_name, bool load_samples )
{
	QString dir = Filesystem::drumkit_path_search( dk_name );
	if ( dir.isEmpty() ) return NULL;
	return load( dir, load_samples );
}

Drumkit* Drumkit::load( const QString& dk_dir, bool load_samples )
{
	INFOLOG( QString( "Load drumkit %1" ).arg( dk_dir ) );
	if( !Filesystem::drumkit_valid( dk_dir ) ) {
		ERRORLOG( QString( "%1 is not valid drumkit" ).arg( dk_dir ) );
		return NULL;
	}
	return load_file( Filesystem::drumkit_file( dk_dir ), load_samples );
}

Drumkit* Drumkit::load_file( const QString& dk_path, bool load_samples )
{
	XMLDoc doc;
	if( !doc.read( dk_path, Filesystem::drumkit_xsd() ) ) {
		return Legacy::load_drumkit( dk_path );
	}
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_info node not found" );
		return NULL;
	}
	Drumkit* drumkit = Drumkit::load_from( &root, dk_path.left( dk_path.lastIndexOf( "/" ) ) );
	if( load_samples ) drumkit->load_samples();
	return drumkit;
}

Drumkit* Drumkit::load_from( XMLNode* node, const QString& dk_path )
{
	QString drumkit_name = node->read_string( "name", "", false, false );
	if ( drumkit_name.isEmpty() ) {
		ERRORLOG( "Drumkit has no name, abort" );
		return NULL;
	}
	Drumkit* drumkit = new Drumkit();
	drumkit->__path = dk_path;
	drumkit->__name = drumkit_name;
	drumkit->__author = node->read_string( "author", "undefined author" );
	drumkit->__info = node->read_string( "info", "No information available." );
	drumkit->__license = node->read_string( "license", "undefined license" );

    XMLNode componentListNode = node->firstChildElement( "componentList" );
	if ( ! componentListNode.isNull() ) {
		XMLNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
		while ( ! componentNode.isNull()  ) {
            int id = componentNode.read_int( "id", -1 );			// instrument id
			QString sName = componentNode.read_string( "name", "" );		// name
			float fVolume = componentNode.read_float( "volume", 1.0 );	// volume
			DrumkitComponent* pDrumkitComponent = new DrumkitComponent( id, sName );
			pDrumkitComponent->set_volume( fVolume );

            drumkit->get_components()->push_back(pDrumkitComponent);

            componentNode = componentNode.nextSiblingElement( "drumkitComponent" );
		}
	}
	else {
        WARNINGLOG( "componentList node not found" );
        DrumkitComponent* pDrumkitComponent = new DrumkitComponent( 0, "main" );
        drumkit->get_components()->push_back(pDrumkitComponent);
	}

	XMLNode instruments_node = node->firstChildElement( "instrumentList" );
	if ( instruments_node.isNull() ) {
		WARNINGLOG( "instrumentList node not found" );
		drumkit->set_instruments( new InstrumentList() );
	} else {
		drumkit->set_instruments( InstrumentList::load_from( &instruments_node, dk_path, drumkit_name ) );
	}
	return drumkit;
}

void Drumkit::load_samples( )
{
	INFOLOG( QString( "Loading drumkit %1 instrument samples" ).arg( __name ) );
	if( !__samples_loaded ) {
		__instruments->load_samples();
		__samples_loaded = true;
	}
}

void Drumkit::unload_samples( )
{
	INFOLOG( QString( "Unloading drumkit %1 instrument samples" ).arg( __name ) );
	if( __samples_loaded ) {
		__instruments->unload_samples();
		__samples_loaded = false;
	}
}

bool Drumkit::save( const QString& name, const QString& author, const QString& info, const QString& license, InstrumentList* instruments, std::vector<DrumkitComponent*>* components, bool overwrite )
{

	Drumkit* drumkit = new Drumkit();
	drumkit->set_name( name );
	drumkit->set_author( author );
	drumkit->set_info( info );
	drumkit->set_license( license );
	drumkit->set_instruments( new InstrumentList( instruments ) );      // FIXME: why must we do that ? there is something weird with updateInstrumentLines
	std::vector<DrumkitComponent*>* p_copiedVector = new std::vector<DrumkitComponent*> ();
	for (std::vector<DrumkitComponent*>::iterator it = components->begin() ; it != components->end(); ++it) {
        DrumkitComponent* src_component = *it;
        p_copiedVector->push_back( new DrumkitComponent( src_component ) );
    }
	drumkit->set_components( p_copiedVector );
	bool ret = drumkit->save( overwrite );
	delete drumkit;
	return ret;
}

bool Drumkit::save( bool overwrite )
{
	return  save( QString( Filesystem::usr_drumkits_dir() + "/" + __name ), overwrite );
}

bool Drumkit::save( const QString& dk_dir, bool overwrite )
{
	INFOLOG( QString( "Saving drumkit %1 into %2" ).arg( __name ).arg( dk_dir ) );
	if( !Filesystem::mkdir( dk_dir ) ) {
		return false;
	}
	bool ret = save_samples( dk_dir, overwrite );
	if ( ret ) {
		ret = save_file( Filesystem::drumkit_file( dk_dir ), overwrite );
	}
	return ret;
}

bool Drumkit::save_file( const QString& dk_path, bool overwrite )
{
	INFOLOG( QString( "Saving drumkit definition into %1" ).arg( dk_path ) );
	if( Filesystem::file_exists( dk_path, true ) && !overwrite ) {
		ERRORLOG( QString( "drumkit %1 already exists" ).arg( dk_path ) );
		return false;
	}
	XMLDoc doc;
	doc.set_root( "drumkit_info", "drumkit" );
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	save_to( &root );
	return doc.write( dk_path );
}

void Drumkit::save_to( XMLNode* node )
{
	node->write_string( "name", __name );
	node->write_string( "author", __author );
	node->write_string( "info", __info );
	node->write_string( "license", __license );
	XMLNode components_node = node->ownerDocument().createElement( "componentList" );
	for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
        DrumkitComponent* component = *it;
        component->save_to( &components_node );
    }
	node->appendChild( components_node );
	__instruments->save_to( node );
}

bool Drumkit::save_samples( const QString& dk_dir, bool overwrite )
{
	INFOLOG( QString( "Saving drumkit %1 samples into %2" ).arg( __name ).arg( dk_dir ) );
	if( !Filesystem::mkdir( dk_dir ) ) {
		return false;
	}

	InstrumentList* instruments = get_instruments();
	for( int i = 0; i < instruments->size(); i++ ) {
		Instrument* instrument = ( *instruments )[i];
		for (std::vector<InstrumentComponent*>::iterator it = instrument->get_components()->begin() ; it != instrument->get_components()->end(); ++it) {
        InstrumentComponent* component = *it;

            for( int n = 0; n < MAX_LAYERS; n++ ) {
                InstrumentLayer* layer = component->get_layer( n );
                if( layer ) {
                    QString src = layer->get_sample()->get_filepath();
                    QString dst = dk_dir + "/" + layer->get_sample()->get_filename();

                    if( src != dst ) {
                        QString original_dst = dst;

                        // If the destination path does not have an extension and there is a dot in the path, hell will break loose. QFileInfo maybe?
                        int insertPosition = original_dst.length();
                        if( original_dst.lastIndexOf(".") > 0 )
                            insertPosition = original_dst.lastIndexOf(".");

                        // If the destination path already exists, try to use basename_1, basename_2, etc. instead of basename.
                        int tries = 0;
                        while( Filesystem::file_exists( dst )) {
                            tries++;
                            dst = original_dst;
                            dst.insert( insertPosition, QString("_%1").arg(tries) );
                        }

                        layer->get_sample()->set_filename( dst );

                        if( !Filesystem::file_copy( src, dst ) ) {
                            return false;
                        }
                    }
				}
			}
		}
	}
	return true;
}

void Drumkit::set_instruments( InstrumentList* instruments )
{
	if( __instruments!=0 ) delete __instruments;
	__instruments = instruments;
}

void Drumkit::set_components( std::vector<DrumkitComponent*>* components )
{
    if( __components != 0 ) delete __components;
    __components = components;
}

bool Drumkit::remove( const QString& dk_name )
{
	QString dk_dir = Filesystem::drumkit_path_search( dk_name );
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

void Drumkit::dump()
{
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
        for (std::vector<InstrumentComponent*>::iterator it = instrument->get_components()->begin() ; it != instrument->get_components()->end(); ++it) {
            InstrumentComponent* component = *it;

            for ( int j=0; j<MAX_LAYERS; j++ ) {
                InstrumentLayer* layer = component->get_layer( j );
                if ( layer ) {
                    Sample* sample = layer->get_sample();
                    if ( sample ) {
                        DEBUGLOG( QString( "   |- %1 [%2]" ).arg( sample->get_filepath() ).arg( sample->is_empty() ) );
                    } else {
                        DEBUGLOG( "   |- NULL sample" );
                    }
                }
			}
		}
	}
}

bool Drumkit::install( const QString& path )
{
	_INFOLOG( QString( "Install drumkit %1" ).arg( path ) );
#ifdef H2CORE_HAVE_LIBARCHIVE
	int r;
	struct archive* arch;
	struct archive_entry* entry;
	char newpath[1024];
	arch = archive_read_new();

#if ARCHIVE_VERSION_NUMBER < 3000000
	archive_read_support_compression_all( arch );
#else
	archive_read_support_filter_all( arch );
#endif

	archive_read_support_format_all( arch );

#if ARCHIVE_VERSION_NUMBER < 3000000
	if ( ( r = archive_read_open_file( arch, path.toLocal8Bit(), 10240 ) ) ) {
#else
	if ( ( r = archive_read_open_filename( arch, path.toLocal8Bit(), 10240 ) ) ) {
#endif
		_ERRORLOG( QString( "archive_read_open_file() [%1] %2" ).arg( archive_errno( arch ) ).arg( archive_error_string( arch ) ) );
		archive_read_close( arch );

		#if ARCHIVE_VERSION_NUMBER < 3000000
			archive_read_finish( arch );
		#else
			archive_read_free( arch );
		#endif

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

	#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_read_finish( arch );
	#else
		archive_read_free( arch );
	#endif

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
	if ( tar_open( &tar_file, tar_path, NULL, O_RDONLY, 0,  TAR_GNU ) == -1 ) {
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
