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

const char* Drumkit::m_sClassName = "Drumkit";

Drumkit::Drumkit() : Object( m_sClassName ), __samples_loaded( false ), __instruments( nullptr ), __components( nullptr )
{
	__components = new std::vector<DrumkitComponent*> ();
}

Drumkit::Drumkit( Drumkit* other ) :
	Object( m_sClassName ),
	__path( other->get_path() ),
	__name( other->get_name() ),
	__author( other->get_author() ),
	__info( other->get_info() ),
	__license( other->get_license() ),
	__image( other->get_image() ),
	__imageLicense( other->get_image_license() ),
	__samples_loaded( other->samples_loaded() ),
	__components( nullptr )
{
	__instruments = new InstrumentList( other->get_instruments() );

	__components = new std::vector<DrumkitComponent*> ();
	__components->assign( other->get_components()->begin(), other->get_components()->end() );
}

Drumkit::~Drumkit()
{
	for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
		delete *it;
	}
	delete __components;

	if( __instruments ) {
		delete __instruments;
	}
}
	
Drumkit* Drumkit::load_by_name ( const QString& dk_name, const bool load_samples )
{
	QString dir = Filesystem::drumkit_path_search( dk_name );
	if ( dir.isEmpty() ) {
		return nullptr;
	}
	
	return load( dir, load_samples );
}

Drumkit* Drumkit::load( const QString& dk_dir, const bool load_samples )
{
	INFOLOG( QString( "Load drumkit %1" ).arg( dk_dir ) );
	if( !Filesystem::drumkit_valid( dk_dir ) ) {
		ERRORLOG( QString( "%1 is not valid drumkit" ).arg( dk_dir ) );
		return nullptr;
	}
	return load_file( Filesystem::drumkit_file( dk_dir ), load_samples );
}

Drumkit* Drumkit::load_file( const QString& dk_path, const bool load_samples )
{
	XMLDoc doc;
	if( !doc.read( dk_path, Filesystem::drumkit_xsd_path() ) ) {
		Drumkit* pDrumkit = Legacy::load_drumkit( dk_path );
		
		if(pDrumkit != nullptr)
		{
			WARNINGLOG( QString( "update drumkit %1" ).arg( dk_path ) );
			pDrumkit->save_file( dk_path, true, -1 );
		}

		return pDrumkit;
	}
	
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_info node not found" );
		return nullptr;
	}
	
	Drumkit* pDrumkit = Drumkit::load_from( &root, dk_path.left( dk_path.lastIndexOf( "/" ) ) );
	if( load_samples ){
		pDrumkit->load_samples();
	}
	return pDrumkit;
}

Drumkit* Drumkit::load_from( XMLNode* node, const QString& dk_path )
{
	QString drumkit_name = node->read_string( "name", "", false, false );
	if ( drumkit_name.isEmpty() ) {
		ERRORLOG( "Drumkit has no name, abort" );
		return nullptr;
	}
	
	Drumkit* pDrumkit = new Drumkit();
	pDrumkit->__path = dk_path;
	pDrumkit->__name = drumkit_name;
	pDrumkit->__author = node->read_string( "author", "undefined author" );
	pDrumkit->__info = node->read_string( "info", "No information available." );
	pDrumkit->__license = node->read_string( "license", "undefined license" );
	pDrumkit->__image = node->read_string( "image", "" );
	pDrumkit->__imageLicense = node->read_string( "imageLicense", "undefined license" );

	XMLNode componentListNode = node->firstChildElement( "componentList" );
	if ( ! componentListNode.isNull() ) {
		XMLNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
		while ( ! componentNode.isNull()  ) {
			int id = componentNode.read_int( "id", -1 );			// instrument id
			QString sName = componentNode.read_string( "name", "" );		// name
			float fVolume = componentNode.read_float( "volume", 1.0 );	// volume
			DrumkitComponent* pDrumkitComponent = new DrumkitComponent( id, sName );
			pDrumkitComponent->set_volume( fVolume );

			pDrumkit->get_components()->push_back(pDrumkitComponent);

			componentNode = componentNode.nextSiblingElement( "drumkitComponent" );
		}
	} else {
		WARNINGLOG( "componentList node not found" );
		DrumkitComponent* pDrumkitComponent = new DrumkitComponent( 0, "Main" );
		pDrumkit->get_components()->push_back(pDrumkitComponent);
	}

	XMLNode instruments_node = node->firstChildElement( "instrumentList" );
	if ( instruments_node.isNull() ) {
		WARNINGLOG( "instrumentList node not found" );
		pDrumkit->set_instruments( new InstrumentList() );
	} else {
		pDrumkit->set_instruments( InstrumentList::load_from( &instruments_node, dk_path, drumkit_name ) );
	}
	return pDrumkit;
}

void Drumkit::load_samples()
{
	INFOLOG( QString( "Loading drumkit %1 instrument samples" ).arg( __name ) );
	if( !__samples_loaded ) {
		__instruments->load_samples();
		__samples_loaded = true;
	}
}

void Drumkit::unload_samples()
{
	INFOLOG( QString( "Unloading drumkit %1 instrument samples" ).arg( __name ) );
	if( __samples_loaded ) {
		__instruments->unload_samples();
		__samples_loaded = false;
	}
}

bool Drumkit::save( const QString&					name,
                    const QString&					author,
                    const QString&					info,
                    const QString&					license,
                    const QString& 					image,
                    const QString& 					imageLicense,
                    InstrumentList*					pInstruments,
                    std::vector<DrumkitComponent*>* pComponents,
                    bool overwrite )
{
	Drumkit* pDrumkit = new Drumkit();
	pDrumkit->set_name( name );
	pDrumkit->set_author( author );
	pDrumkit->set_info( info );
	pDrumkit->set_license( license );

	// save the original path
	QFileInfo fi( image );
	pDrumkit->set_path( fi.absolutePath() );
	pDrumkit->set_image( fi.fileName() );
	pDrumkit->set_image_license( imageLicense );

	pDrumkit->set_instruments( new InstrumentList( pInstruments ) );      // FIXME: why must we do that ? there is something weird with updateInstrumentLines
	
	std::vector<DrumkitComponent*>* pCopiedVector = new std::vector<DrumkitComponent*> ();
	for (std::vector<DrumkitComponent*>::iterator it = pComponents->begin() ; it != pComponents->end(); ++it) {
		DrumkitComponent* pSrcComponent = *it;
		pCopiedVector->push_back( new DrumkitComponent( pSrcComponent ) );
	}
	pDrumkit->set_components( pCopiedVector );
	
	bool ret = pDrumkit->save( overwrite );
	delete pDrumkit;

	return ret;
}

bool Drumkit::user_drumkit_exists( const QString& name)
{
	return Filesystem::file_exists( Filesystem::drumkit_file( Filesystem::usr_drumkits_dir() + name ), true /*silent*/ );
}

bool Drumkit::save( bool overwrite )
{
	return  save( QString( Filesystem::usr_drumkits_dir() + __name ), overwrite );
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

bool Drumkit::save_file( const QString& dk_path, bool overwrite, int component_id )
{
	INFOLOG( QString( "Saving drumkit definition into %1" ).arg( dk_path ) );
	if( !overwrite && Filesystem::file_exists( dk_path, true ) ) {
		ERRORLOG( QString( "drumkit %1 already exists" ).arg( dk_path ) );
		return false;
	}
	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_info", "drumkit" );
	save_to( &root, component_id );
	return doc.write( dk_path );
}

void Drumkit::save_to( XMLNode* node, int component_id )
{
	node->write_string( "name", __name );
	node->write_string( "author", __author );
	node->write_string( "info", __info );
	node->write_string( "license", __license );
	node->write_string( "image", __image );
	node->write_string( "imageLicense", __imageLicense );

	if( component_id == -1 ) {
		XMLNode components_node = node->createNode( "componentList" );
		for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
			DrumkitComponent* pComponent = *it;
			pComponent->save_to( &components_node );
		}
	}
	__instruments->save_to( node, component_id );
}

bool Drumkit::save_samples( const QString& dk_dir, bool overwrite )
{
	INFOLOG( QString( "Saving drumkit %1 samples into %2" ).arg( __name ).arg( dk_dir ) );
	if( !Filesystem::mkdir( dk_dir ) ) {
		return false;
	}

	InstrumentList* pInstrList = get_instruments();
	for( int i = 0; i < pInstrList->size(); i++ ) {
		Instrument* pInstrument = ( *pInstrList )[i];
		for (std::vector<InstrumentComponent*>::iterator it = pInstrument->get_components()->begin() ; it != pInstrument->get_components()->end(); ++it) {
			InstrumentComponent* pComponent = *it;

			for ( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
				InstrumentLayer* pLayer = pComponent->get_layer( n );
				if( pLayer ) {
					QString src = pLayer->get_sample()->get_filepath();
					QString dst = dk_dir + "/" + pLayer->get_sample()->get_filename();

					if( src != dst ) {
						QString original_dst = dst;

						// If the destination path does not have an extension and there is a dot in the path, hell will break loose. QFileInfo maybe?
						int insertPosition = original_dst.length();
						if( original_dst.lastIndexOf(".") > 0 ) {
							insertPosition = original_dst.lastIndexOf(".");
						}

						if(overwrite == false) {
							// If the destination path already exists, try to use basename_1, basename_2, etc. instead of basename.
							int tries = 0;
							while( Filesystem::file_exists( dst, true )) {
								tries++;
								dst = original_dst;
								dst.insert( insertPosition, QString("_%1").arg(tries) );
							}
						}

						pLayer->get_sample()->set_filename( dst );

						if( !Filesystem::file_copy( src, dst ) ) {
							return false;
						}
					}
				}
			}
		}
	}
	if ( !save_image( dk_dir, overwrite ) ) {
		return false;
	}

	return true;
}

bool Drumkit::save_image( const QString& dk_dir, bool overwrite )
{
	if ( __image.length() > 0 ) {
		QString src = __path + "/" + __image;
		QString dst = dk_dir + "/" + __image;
		if ( Filesystem::file_exists ( src ) ) {
			if( !Filesystem::file_copy( src, dst ) ) {
				ERRORLOG( QString( "Error copying %1 to %2").arg( src ).arg( dst ) );
				return false;
			}
		}
	}
	return true;
}

void Drumkit::set_instruments( InstrumentList* instruments )
{
	if( __instruments != nullptr ) {
		delete __instruments;
	}
	
	__instruments = instruments;
}

void Drumkit::set_components( std::vector<DrumkitComponent*>* components )
{
	for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
		delete *it;
	}
	
	delete __components;
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
	DEBUGLOG( " |- Image = " + __image );
	DEBUGLOG( " |- Image = " + __imageLicense );

	DEBUGLOG( " |- Instrument list" );
	for ( int i=0; i<__instruments->size(); i++ ) {
		Instrument* instrument = ( *__instruments )[i];
		DEBUGLOG( QString( "  |- (%1 of %2) Name = %3" )
		          .arg( i )
		          .arg( __instruments->size()-1 )
		          .arg( instrument->get_name() )
		        );
		for (std::vector<InstrumentComponent*>::iterator it = instrument->get_components()->begin() ; it != instrument->get_components()->end(); ++it) {
			InstrumentComponent* pComponent = *it;

			for ( int j = 0; j < InstrumentComponent::getMaxLayers(); j++ ) {
				InstrumentLayer* pLayer = pComponent->get_layer( j );
				if ( pLayer ) {
					Sample* pSample = pLayer->get_sample();
					if ( pSample ) {
						DEBUGLOG( QString( "   |- %1 [%2]" ).arg( pSample->get_filepath() ).arg( pSample->is_empty() ) );
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

		QByteArray newpath = np.toLocal8Bit();

		archive_entry_set_pathname( entry, newpath.data() );
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

	QByteArray tar_path = gzd_name.toLocal8Bit();

	if ( tar_open( &tar_file, tar_path.data(), NULL, O_RDONLY, 0,  TAR_GNU ) == -1 ) {
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

/* vim: set softtabstop=4 noexpandtab: */
