/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Basics/Drumkit.h>
#include <core/config.h>
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

#include <core/Basics/Sample.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>

#include <core/Helpers/Xml.h>
#include <core/Helpers/Legacy.h>

namespace H2Core
{

Drumkit::Drumkit() : __samples_loaded( false ),
					 __instruments( nullptr ),
					 __name( "empty" ),
					 __author( "undefined author" ),
					 __info( "No information available." ),
					 __license( "undefined license" ),
					 __imageLicense( "undefined license" )
{
	__components = new std::vector<DrumkitComponent*> ();
	__instruments = new InstrumentList();
}

Drumkit::Drumkit( Drumkit* other ) :
	Object(),
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
	for (auto it = other->get_components()->begin(); it != other->get_components()->end(); ++it) {
		__components->push_back(new DrumkitComponent(*it));
	}
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

Drumkit* Drumkit::load_by_name( const QString& dk_name, const bool load_samples, Filesystem::Lookup lookup )
{
	QString dir = Filesystem::drumkit_path_search( dk_name, lookup );
	if ( dir.isEmpty() ) {
		return nullptr;
	}
	
	return load( dir, load_samples );
}

	Drumkit* Drumkit::load( const QString& dk_dir, const bool load_samples, bool bUpgrade )
{
	INFOLOG( QString( "Load drumkit %1" ).arg( dk_dir ) );
	if( !Filesystem::drumkit_valid( dk_dir ) ) {
		ERRORLOG( QString( "%1 is not valid drumkit" ).arg( dk_dir ) );
		return nullptr;
	}
	return load_file( Filesystem::drumkit_file( dk_dir ), load_samples, bUpgrade );
}

	Drumkit* Drumkit::load_file( const QString& dk_path, const bool load_samples, bool bUpgrade )
{
	bool bReadingSuccessful = true;
	
	XMLDoc doc;
	if( !doc.read( dk_path, Filesystem::drumkit_xsd_path() ) ) {
		//Something went wrong. Lets see how old this drumkit is..
		
		//Do we have any components? 
		doc.read( dk_path );
		auto nodeList = doc.elementsByTagName( "instrumentComponent" );
		if( nodeList.size() == 0 )
		{
			//No components. That drumkit seems to be quite old. Use legacy code..
			
			Drumkit* pDrumkit = Legacy::load_drumkit( dk_path );
			if ( bUpgrade ) {
				upgrade_drumkit(pDrumkit, dk_path);
			}
			
			return pDrumkit;
		} else {
			//If the drumkit does not comply with the current xsd, but
			// has components, it may suffer from problems with
			// invalid values (for example float ADSR values, see
			// #658). Lets try to load it with our current drumkit.
			bReadingSuccessful = false;
		}
	}
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_info node not found" );
		return nullptr;
	}

	Drumkit* pDrumkit = Drumkit::load_from( &root, dk_path.left( dk_path.lastIndexOf( "/" ) ) );
	if ( ! bReadingSuccessful && bUpgrade ) {
		upgrade_drumkit( pDrumkit, dk_path );
	}
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

void Drumkit::upgrade_drumkit(Drumkit* pDrumkit, const QString& dk_path)
{
	if( pDrumkit != nullptr ) {
		if ( ! Filesystem::file_exists( dk_path, true ) ) {
			ERRORLOG( QString( "No drumkit found at path %1" ).arg( dk_path ) );
			return;
		}
		QFileInfo fi( dk_path );
		if ( ! Filesystem::dir_writable( fi.dir().absolutePath(), true ) ) {
			ERRORLOG( QString( "Drumkit %1 is out of date but can not be upgraded since path is not writable (please copy it to your user's home instead)" ).arg( dk_path ) );
			return;
		}
		WARNINGLOG( QString( "Upgrading drumkit %1" ).arg( dk_path ) );

		QString sBackupPath = Filesystem::drumkit_backup_path( dk_path );
		Filesystem::file_copy( dk_path, sBackupPath,
		                       false /* do not overwrite existing files */ );
		
		pDrumkit->save_file( dk_path, true, -1 );
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

bool Drumkit::save( const QString&					sName,
                    const QString&					sAuthor,
                    const QString&					sInfo,
                    const QString&					sLicense,
                    const QString& 					sImage,
                    const QString& 					sImageLicense,
                    InstrumentList*					pInstruments,
                    std::vector<DrumkitComponent*>* pComponents,
                    bool 							bOverwrite )
{
	Drumkit* pDrumkit = new Drumkit();
	pDrumkit->set_name( sName );
	pDrumkit->set_author( sAuthor );
	pDrumkit->set_info( sInfo );
	pDrumkit->set_license( sLicense );

	// Before storing the absolute path to the image of the drumkit it
	// has to be checked whether an actual path was supplied. If not,
	// the construction of QFileInfo will fail.
	if ( !sImage.isEmpty() ) {
		QFileInfo fi( sImage );
		pDrumkit->set_path( fi.absolutePath() );
		pDrumkit->set_image( fi.fileName() );
	}
	pDrumkit->set_image_license( sImageLicense );

	pDrumkit->set_instruments( new InstrumentList( pInstruments ) );      // FIXME: why must we do that ? there is something weird with updateInstrumentLines
	
	std::vector<DrumkitComponent*>* pCopiedVector = new std::vector<DrumkitComponent*> ();
	for ( auto& pSrcComponent : *pComponents ) {
		pCopiedVector->push_back( new DrumkitComponent( pSrcComponent ) );
	}
	pDrumkit->set_components( pCopiedVector );
	
	bool bRet = pDrumkit->save( bOverwrite );
	delete pDrumkit;

	return bRet;
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
		if ( __components->size() > 0 ) {
			for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
				DrumkitComponent* pComponent = *it;
				pComponent->save_to( &components_node );
			}
		} else {
			WARNINGLOG( "Drumkit has no components. Storing an empty one as fallback." );
			DrumkitComponent* pDrumkitComponent = new DrumkitComponent( 0, "Main" );
			pDrumkitComponent->save_to( &components_node );
			delete pDrumkitComponent;
		}	
	}

	if ( __instruments != nullptr && __instruments->size() > 0 ) {
		__instruments->save_to( node, component_id );
	} else {
		WARNINGLOG( "Drumkit has no instruments. Storing an InstrumentList with a single empty Instrument as fallback." );
		InstrumentList* pInstrumentList = new InstrumentList();
		auto pInstrument = std::make_shared<Instrument>();
		pInstrumentList->insert( 0, pInstrument );
		pInstrumentList->save_to( node, component_id );
		delete pInstrumentList;
	}
}

bool Drumkit::save_samples( const QString& dk_dir, bool overwrite )
{
	INFOLOG( QString( "Saving drumkit %1 samples into %2" ).arg( __name ).arg( dk_dir ) );
	if( !Filesystem::mkdir( dk_dir ) ) {
		return false;
	}

	InstrumentList* pInstrList = get_instruments();
	for( int i = 0; i < pInstrList->size(); i++ ) {
		auto pInstrument = ( *pInstrList )[i];
		for ( const auto& pComponent : *pInstrument->get_components() ) {

			for ( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
				auto pLayer = pComponent->get_layer( n );
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

bool Drumkit::remove( const QString& sDrumkitName, Filesystem::Lookup lookup )
{
	QString sDrumkitDir = Filesystem::drumkit_path_search( sDrumkitName, lookup );
	if( !Filesystem::drumkit_valid( sDrumkitDir ) ) {
		ERRORLOG( QString( "%1 is not valid drumkit" ).arg( sDrumkitDir ) );
		return false;
	}
	_INFOLOG( QString( "Removing drumkit: %1" ).arg( sDrumkitDir ) );
	if( !Filesystem::rm( sDrumkitDir, true ) ) {
		_ERRORLOG( QString( "Unable to remove drumkit: %1" ).arg( sDrumkitDir ) );
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
		auto instrument = ( *__instruments )[i];
		DEBUGLOG( QString( "  |- (%1 of %2) Name = %3" )
		          .arg( i )
		          .arg( __instruments->size()-1 )
		          .arg( instrument->get_name() )
		        );
		for ( const auto& pComponent : *instrument->get_components() ) {
			for ( int j = 0; j < InstrumentComponent::getMaxLayers(); j++ ) {
				auto pLayer = pComponent->get_layer( j );
				if ( pLayer ) {
					auto pSample = pLayer->get_sample();
					if ( pSample != nullptr ) {
						DEBUGLOG( QString( "   |- %1 [%2]" ).arg( pSample->get_filepath() ).arg( pSample->is_empty() ) );
					} else {
						DEBUGLOG( "   |- NULL sample" );
					}
				}
			}
		}
	}
}

bool Drumkit::isUserDrumkit() const {
	if ( __path.contains( Filesystem::sys_drumkits_dir() ) ) {
		return false;
	} 
	return true;
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
	if ( archive_read_open_file( arch, path.toLocal8Bit(), 10240 ) ) {
#else
	if ( archive_read_open_filename( arch, path.toLocal8Bit(), 10240 ) ) {
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

QString Drumkit::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Drumkit]\n" ).arg( sPrefix )
			.append( QString( "%1%2path: %3\n" ).arg( sPrefix ).arg( s ).arg( __path ) )
			.append( QString( "%1%2name: %3\n" ).arg( sPrefix ).arg( s ).arg( __name ) )
			.append( QString( "%1%2author: %3\n" ).arg( sPrefix ).arg( s ).arg( __author ) )
			.append( QString( "%1%2info: %3\n" ).arg( sPrefix ).arg( s ).arg( __info ) )
			.append( QString( "%1%2license: %3\n" ).arg( sPrefix ).arg( s ).arg( __license ) )
			.append( QString( "%1%2image: %3\n" ).arg( sPrefix ).arg( s ).arg( __image ) )
			.append( QString( "%1%2imageLicense: %3\n" ).arg( sPrefix ).arg( s ).arg( __imageLicense ) )
			.append( QString( "%1%2samples_loaded: %3\n" ).arg( sPrefix ).arg( s ).arg( __samples_loaded ) )
			.append( QString( "%1" ).arg( __instruments->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2components:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto cc : *__components ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( "%1" ).arg( cc->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
	} else {
		
		sOutput = QString( "[Drumkit]" )
			.append( QString( ", path: %1" ).arg( __path ) )
			.append( QString( ", name: %1" ).arg( __name ) )
			.append( QString( ", author: %1" ).arg( __author ) )
			.append( QString( ", info: %1" ).arg( __info ) )
			.append( QString( ", license: %1" ).arg( __license ) )
			.append( QString( ", image: %1" ).arg( __image ) )
			.append( QString( ", imageLicense: %1" ).arg( __imageLicense ) )
			.append( QString( ", samples_loaded: %1" ).arg( __samples_loaded ) )
			.append( QString( ", [%1]" ).arg( __instruments->toQString( sPrefix + s, bShort ) ) )
			.append( QString( ", components: [ " ) );
		for ( auto cc : *__components ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( "[%1]" ).arg( cc->toQString( sPrefix + s + s, bShort ).replace( "\n", " " ) ) );
			}
		}
		sOutput.append( "]\n" );
	}
	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
