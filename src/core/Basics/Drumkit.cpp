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

#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core
{

Drumkit::Drumkit() : __samples_loaded( false ),
					 __instruments( nullptr ),
					 __name( "empty" ),
					 __author( "undefined author" ),
					 __info( "No information available." ),
					 __license( License() ),
					 __imageLicense( License() )
{
	__components = std::make_shared<std::vector<std::shared_ptr<DrumkitComponent>>>();
	__instruments = std::make_shared<InstrumentList>();
}

Drumkit::Drumkit( std::shared_ptr<Drumkit> other ) :
	Object(),
	__path( other->get_path() ),
	__name( other->get_name() ),
	__author( other->get_author() ),
	__info( other->get_info() ),
	__license( other->get_license() ),
	__image( other->get_image() ),
	__imageLicense( other->get_image_license() ),
	__samples_loaded( other->samples_loaded() )
{
	__instruments = std::make_shared<InstrumentList>( other->get_instruments() );

	__components = std::make_shared<std::vector<std::shared_ptr<DrumkitComponent>>>();
	for ( const auto& pComponent : *other->get_components() ) {
		__components->push_back( std::make_shared<DrumkitComponent>( pComponent ) );
	}
}

Drumkit::~Drumkit()
{
}

std::shared_ptr<Drumkit> Drumkit::load( const QString& sDrumkitPath, bool bUpgrade, bool bSilent )
{
	if ( ! Filesystem::drumkit_valid( sDrumkitPath ) ) {
		ERRORLOG( QString( "[%1] is not valid drumkit folder" ).arg( sDrumkitPath ) );
		return nullptr;
	}

	QString sDrumkitFile = Filesystem::drumkit_file( sDrumkitPath );
	
	bool bReadingSuccessful = true;
	
	XMLDoc doc;
	if ( !doc.read( sDrumkitFile, Filesystem::drumkit_xsd_path(), true ) ) {
		// Drumkit does not comply with the XSD schema
		// definition. It's probably an old one. load_from() will try
		// to handle it regardlessly but we should upgrade it in order
		// to avoid this in future loads.
		
		doc.read( sDrumkitFile, nullptr, bSilent );
		
		bReadingSuccessful = false;
	}
	
	XMLNode root = doc.firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_info node not found" );
		return nullptr;
	}

	auto pDrumkit =
		Drumkit::load_from( &root, sDrumkitFile.left( sDrumkitFile.lastIndexOf( "/" ) ),
							bSilent );
	
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to load drumkit [%1]" ).arg( sDrumkitFile ) );
		return nullptr;
	}
	
	if ( ! bReadingSuccessful && bUpgrade ) {
		upgrade_drumkit( pDrumkit, sDrumkitFile );
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "[%1] loaded from [%2]" )
				 .arg( pDrumkit->get_name() )
				 .arg( sDrumkitPath ) );
	}
	
	return pDrumkit;
}

std::shared_ptr<Drumkit> Drumkit::load_from( XMLNode* node, const QString& sDrumkitPath, bool bSilent )
{
	QString sDrumkitName = node->read_string( "name", "", false, false, bSilent );
	if ( sDrumkitName.isEmpty() ) {
		ERRORLOG( "Drumkit has no name, abort" );
		return nullptr;
	}
	
	std::shared_ptr<Drumkit> pDrumkit = std::make_shared<Drumkit>();

	pDrumkit->__path = sDrumkitPath;
	pDrumkit->__name = sDrumkitName;
	pDrumkit->__author = node->read_string( "author", "undefined author",
											true, true, bSilent );
	pDrumkit->__info = node->read_string( "info", "No information available.",
										  true, true, bSilent  );

	License license( node->read_string( "license", "undefined license",
										true, true, bSilent  ),
					 pDrumkit->__author );
	pDrumkit->set_license( license );

	// As of 2022 we have no drumkits featuring an image in
	// stock. Thus, verbosity of this one will be turned of in order
	// to make to log more concise.
	pDrumkit->set_image( node->read_string( "image", "",
											true, true, true ) );
	License imageLicense( node->read_string( "imageLicense", "undefined license",
											 true, true, bSilent  ),
						  pDrumkit->__author );
	pDrumkit->set_image_license( imageLicense );

	XMLNode componentListNode = node->firstChildElement( "componentList" );
	if ( ! componentListNode.isNull() ) {
		XMLNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
		while ( ! componentNode.isNull()  ) {
			auto pDrumkitComponent = DrumkitComponent::load_from( &componentNode );
			if ( pDrumkitComponent != nullptr ) {
				pDrumkit->get_components()->push_back(pDrumkitComponent);
			}

			componentNode = componentNode.nextSiblingElement( "drumkitComponent" );
		}
	} else {
		WARNINGLOG( "componentList node not found" );
		auto pDrumkitComponent = std::make_shared<DrumkitComponent>( 0, "Main" );
		pDrumkit->get_components()->push_back(pDrumkitComponent);
	}

	auto pInstrumentList = InstrumentList::load_from( node,
													  sDrumkitPath,
													  sDrumkitName,
													  license, false );
	// Required to assure backward compatibility.
	if ( pInstrumentList == nullptr ) {
		WARNINGLOG( "instrument list could not be loaded. Using empty one." );
		pInstrumentList = std::make_shared<InstrumentList>();
	}
		
	pDrumkit->set_instruments( pInstrumentList );

	// Instead of making the *::load_from() functions more complex by
	// passing the license down to each sample, we will make the
	// drumkit assign its license to each sample in here.
	pDrumkit->propagateLicense();

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

License Drumkit::loadLicenseFrom( const QString& sDrumkitDir, bool bSilent )
{
	// Try to retrieve the license from cache first.
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen != nullptr ) {
		auto pDrumkit =
			pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitDir );
		if ( pDrumkit != nullptr ) {
			return pDrumkit->get_license();
		}
	}

	XMLDoc doc;
	if ( Drumkit::loadDoc( sDrumkitDir, &doc, bSilent ) ) {
		XMLNode root = doc.firstChildElement( "drumkit_info" );

		QString sAuthor = root.read_string( "author", "undefined author",
											true, true, bSilent );
		QString sLicenseString = root.read_string( "license", "undefined license",
												   false, true, bSilent  );
		if ( sLicenseString.isNull() ) {
			ERRORLOG( QString( "Unable to retrieve license information from [%1]" )
					  .arg( sDrumkitDir ) );
			return std::move( License() );
		}

		return std::move( License( sLicenseString, sAuthor ) );
	}

	return std::move( License() );
}

QString Drumkit::loadNameFrom( const QString& sDrumkitDir, bool bSilent ) {

	// Try to retrieve the name from cache first.
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen != nullptr ) {
		auto pDrumkit =
			pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitDir );
		if ( pDrumkit != nullptr ) {
			return pDrumkit->get_name();
		}
	}

	// No entry in cache found. Loading it from disk
	XMLDoc doc;
	if ( Drumkit::loadDoc( sDrumkitDir, &doc, bSilent ) ) {
		XMLNode root = doc.firstChildElement( "drumkit_info" );

		return( root.read_string( "name", "", true, true, bSilent ) );
	}

	return "";
}

bool Drumkit::loadDoc( const QString& sDrumkitDir, XMLDoc* pDoc, bool bSilent ) {

	if ( ! Filesystem::drumkit_valid( sDrumkitDir ) ) {
		ERRORLOG( QString( "[%1] is not valid drumkit folder" ).arg( sDrumkitDir ) );
		return false;
	}

	const QString sDrumkitPath = Filesystem::drumkit_file( sDrumkitDir );

	if( ! pDoc->read( sDrumkitPath, Filesystem::drumkit_xsd_path(), true ) ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "[%1] does not validate against drumkit schema. Trying to retrieve its name nevertheless.")
						.arg( sDrumkitPath ) );
		}
		
		if ( ! pDoc->read( sDrumkitPath, nullptr, bSilent ) ) {
			ERRORLOG( QString( "Unable to load drumkit name for [%1]" )
					  .arg( sDrumkitPath ) );
			return false;
		}
	}
	
	XMLNode root = pDoc->firstChildElement( "drumkit_info" );
	if ( root.isNull() ) {
		ERRORLOG( QString( "Unable to load drumkit name for [%1]. 'drumkit_info' node not found" )
				  .arg( sDrumkitPath ) );
		return false;
	}

	return true;
}
	
void Drumkit::upgrade_drumkit(std::shared_ptr<Drumkit> pDrumkit, const QString& sDrumkitPath, bool bSilent )
{
	if ( pDrumkit != nullptr ) {
		if ( ! Filesystem::file_exists( sDrumkitPath, true ) ) {
			ERRORLOG( QString( "No drumkit found at path %1" ).arg( sDrumkitPath ) );
			return;
		}
		QFileInfo fi( sDrumkitPath );
		if ( ! Filesystem::dir_writable( fi.dir().absolutePath(), true ) ) {
			ERRORLOG( QString( "Drumkit %1 is out of date but can not be upgraded since path is not writable (please copy it to your user's home instead)" ).arg( sDrumkitPath ) );
			return;
		}
		if ( ! bSilent ) {
			INFOLOG( QString( "Upgrading drumkit %1" ).arg( sDrumkitPath ) );
		}

		QString sBackupPath = Filesystem::drumkit_backup_path( sDrumkitPath );
		Filesystem::file_copy( sDrumkitPath, sBackupPath,
		                       false /* do not overwrite existing
										files */,
							   bSilent );
		
		pDrumkit->save( sDrumkitPath, -1, true, bSilent );
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

QString Drumkit::getFolderName() const {
	return Filesystem::validateFilePath( __name );
}

QString Drumkit::getExportName( const QString& sComponentName, bool bRecentVersion ) const {
	QString sExportName = getFolderName();
	if ( ! sComponentName.isEmpty() ) {
		sExportName.append( "_" +
							Filesystem::validateFilePath( sComponentName ) );
		if ( ! bRecentVersion ) {
			sExportName.append( "_legacy" );
		}
	}
	
	return sExportName;
}

bool Drumkit::save( const QString& sDrumkitPath, int nComponentID, bool bRecentVersion, bool bSilent )
{
	QString sDrumkitFolder( sDrumkitPath );
	if ( sDrumkitPath.isEmpty() ) {
		sDrumkitFolder = __path;
	}
	
	if ( ! Filesystem::dir_exists( sDrumkitFolder, true ) &&
		 ! Filesystem::mkdir( sDrumkitFolder ) ) {
		ERRORLOG( QString( "Unable to export drumkit [%1] to [%2]. Could not create drumkit folder." )
			 .arg( __name ).arg( sDrumkitFolder ) );
		return false;
	}

	if ( Filesystem::dir_exists( sDrumkitFolder, bSilent ) &&
		 ! Filesystem::dir_writable( sDrumkitFolder, bSilent ) ) {
		ERRORLOG( QString( "Unable to export drumkit [%1] to [%2]. Drumkit folder not writable." )
			 .arg( __name ).arg( sDrumkitFolder ) );
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "Saving drumkit [%1] into [%2]" )
				 .arg( __name ).arg( sDrumkitFolder ) );
	}

	// Save external files
	if ( ! save_samples( sDrumkitFolder, bSilent ) ) {
		ERRORLOG( QString( "Unable to save samples of drumkit [%1] to [%2]. Abort." )
				  .arg( __name ).arg( sDrumkitFolder ) );
		return false;
	}
	
	if ( ! save_image( sDrumkitFolder, bSilent ) ) {
		ERRORLOG( QString( "Unable to save image of drumkit [%1] to [%2]. Abort." )
				  .arg( __name ).arg( sDrumkitFolder ) );
		return false;
	}

	// Ensure all instruments and associated samples will hold the
	// same license as the overall drumkit and are associated to
	// it. (Not important for saving itself but for consistency and
	// using the drumkit later on).
	propagateLicense();

	// Save drumkit.xml
	XMLDoc doc;
	XMLNode root = doc.set_root( "drumkit_info", "drumkit" );
	
	// In order to comply with the GPL license we have to add a
	// license notice to the file.
	if ( __license.getType() == License::GPL ) {
		root.appendChild( doc.createComment( License::getGPLLicenseNotice( __author ) ) );
	}
	
	save_to( &root, nComponentID, bRecentVersion, bSilent );
	return doc.write( Filesystem::drumkit_file( sDrumkitFolder ) );
}

void Drumkit::save_to( XMLNode* node, int component_id, bool bRecentVersion, bool bSilent ) const
{
	node->write_string( "name", __name );
	node->write_string( "author", __author );
	node->write_string( "info", __info );
	node->write_string( "license", __license.getLicenseString() );
	node->write_string( "image", __image );
	node->write_string( "imageLicense", __imageLicense.getLicenseString() );

	// Only drumkits used for Hydrogen v0.9.7 or higher are allowed to
	// have components. If the user decides to export the kit to
	// legacy version, the components will be omitted and Instrument
	// layers corresponding to component_id will be exported.
	if ( bRecentVersion ) {
		XMLNode components_node = node->createNode( "componentList" );
		if ( component_id == -1 && __components->size() > 0 ) {
			for ( const auto& pComponent : *__components ){
				pComponent->save_to( &components_node );
			}
		}
		else {
			bool bComponentFound = false;

			if ( component_id != -1 ) {
				for ( const auto& pComponent : *__components ){
					if ( pComponent != nullptr &&
						 pComponent->get_id() == component_id ) {
						bComponentFound = true;
						pComponent->save_to( &components_node );
					}
				}
			} else {
				WARNINGLOG( "Drumkit has no components. Storing an empty one as fallback." );
			}

			if ( ! bComponentFound ) {
				if ( component_id != -1 ) {
					ERRORLOG( QString( "Unable to retrieve DrumkitComponent [%1]. Storing an empty one as fallback." )
							  .arg( component_id ) );
				}
				auto pDrumkitComponent = std::make_shared<DrumkitComponent>( 0, "Main" );
				pDrumkitComponent->save_to( &components_node );
			}
		}
	} else {
		// Legacy export
		if ( component_id == -1 ) {
			ERRORLOG( "Exporting the full drumkit with all components is allowed when targeting the legacy versions >= 0.9.6" );
			return;
		}
	}

	if ( __instruments != nullptr && __instruments->size() > 0 ) {
		__instruments->save_to( node, component_id, bRecentVersion, false );
	} else {
		WARNINGLOG( "Drumkit has no instruments. Storing an InstrumentList with a single empty Instrument as fallback." );
		auto pInstrumentList = std::make_shared<InstrumentList>();
		auto pInstrument = std::make_shared<Instrument>();
		pInstrumentList->insert( 0, pInstrument );
		pInstrumentList->save_to( node, component_id, bRecentVersion );
	}
}

bool Drumkit::save_samples( const QString& sDrumkitFolder, bool bSilent ) const
{
	if ( ! bSilent ) {
		INFOLOG( QString( "Saving drumkit [%1] samples into [%2]" )
				 .arg( __name ).arg( sDrumkitFolder ) );
	}

	auto pInstrList = get_instruments();
	for ( int i = 0; i < pInstrList->size(); i++ ) {
		auto pInstrument = ( *pInstrList )[i];
		for ( const auto& pComponent : *pInstrument->get_components() ) {

			for ( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
				auto pLayer = pComponent->get_layer( n );
				if ( pLayer != nullptr && pLayer->get_sample() != nullptr ) {
					QString src = pLayer->get_sample()->get_filepath();
					QString dst = sDrumkitFolder + "/" + pLayer->get_sample()->get_filename();

					if ( src != dst ) {
						QString original_dst = dst;

						// If the destination path does not have an extension and there is a dot in the path, hell will break loose. QFileInfo maybe?
						int insertPosition = original_dst.length();
						if ( original_dst.lastIndexOf(".") > 0 ) {
							insertPosition = original_dst.lastIndexOf(".");
						}

						pLayer->get_sample()->set_filename( dst );

						if( ! Filesystem::file_copy( src, dst, bSilent ) ) {
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

bool Drumkit::save_image( const QString& dk_dir, bool bSilent ) const
{
	if ( __image.length() > 0 ) {
		QString src = __path + "/" + __image;
		QString dst = dk_dir + "/" + __image;
		if ( Filesystem::file_exists( src, bSilent ) ) {
			if ( ! Filesystem::file_copy( src, dst, bSilent ) ) {
				ERRORLOG( QString( "Error copying %1 to %2").arg( src ).arg( dst ) );
				return false;
			}
		}
	}
	return true;
}

void Drumkit::set_instruments( std::shared_ptr<InstrumentList> instruments )
{
	__instruments = instruments;
}

void Drumkit::set_components( std::shared_ptr<std::vector<std::shared_ptr<DrumkitComponent>>> components )
{
	__components = components;
}

void Drumkit::propagateLicense(){

	for ( const auto& ppInstrument : *__instruments ) {
		if ( ppInstrument != nullptr ) {

			ppInstrument->set_drumkit_path( __path );
			ppInstrument->set_drumkit_name( __name );
			for ( const auto& ppInstrumentComponent : *ppInstrument->get_components() ) {
				if ( ppInstrumentComponent != nullptr ) {
					for ( const auto& ppInstrumentLayer : *ppInstrumentComponent ) {
						if ( ppInstrumentLayer != nullptr ) {
							auto pSample = ppInstrumentLayer->get_sample();
							if ( pSample != nullptr ) {
								pSample->setLicense( get_license() );
							}
						}
					}
				}
			}
		}
	}
}

std::vector<std::shared_ptr<InstrumentList::Content>> Drumkit::summarizeContent() const {
	return __instruments->summarizeContent( __components );
}

bool Drumkit::remove( const QString& sDrumkitDir )
{
	if( ! Filesystem::drumkit_valid( sDrumkitDir ) ) {
		ERRORLOG( QString( "%1 is not valid drumkit folder" ).arg( sDrumkitDir ) );
		return false;
	}
	
	INFOLOG( QString( "Removing drumkit: %1" ).arg( sDrumkitDir ) );
	if ( ! Filesystem::rm( sDrumkitDir, true ) ) {
		ERRORLOG( QString( "Unable to remove drumkit: %1" ).arg( sDrumkitDir ) );
		return false;
	}

	Hydrogen::get_instance()->getSoundLibraryDatabase()->updateDrumkits();
	return true;
}

bool Drumkit::isUserDrumkit() const {
	if ( __path.contains( Filesystem::sys_drumkits_dir() ) ) {
		return false;
	} else if ( ! Filesystem::dir_writable( __path ) ) {
		return false;
	}
	
	return true;
}
	
bool Drumkit::install( const QString& sSourcePath, const QString& sTargetPath, bool bSilent )
{
	if ( sTargetPath.isEmpty() ) {
		if ( ! bSilent ) {
			_INFOLOG( QString( "Install drumkit [%1]" ).arg( sSourcePath ) );
		}
		
	} else {
		if ( ! Filesystem::path_usable( sTargetPath, true, false ) ) {
			return false;
		}
		
		if ( ! bSilent ) {		
			_INFOLOG( QString( "Extract drumkit from [%1] to [%2]" )
					  .arg( sSourcePath ).arg( sTargetPath ) );
		}
	}
	
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
	if ( archive_read_open_file( arch, sSourcePath.toLocal8Bit(), 10240 ) ) {
#else
	if ( archive_read_open_filename( arch, sSourcePath.toLocal8Bit(), 10240 ) ) {
#endif
		_ERRORLOG( QString( "archive_read_open_file() [%1] %2" )
				   .arg( archive_errno( arch ) )
				   .arg( archive_error_string( arch ) ) );
		archive_read_close( arch );

#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_read_finish( arch );
#else
		archive_read_free( arch );
#endif

		return false;
	}
	bool ret = true;

	QString dk_dir;
	if ( ! sTargetPath.isEmpty() ) {
		dk_dir = sTargetPath + "/";
	} else {
		dk_dir = Filesystem::usr_drumkits_dir() + "/";
	}
		
	while ( ( r = archive_read_next_header( arch, &entry ) ) != ARCHIVE_EOF ) {
		if ( r != ARCHIVE_OK ) {
			_ERRORLOG( QString( "archive_read_next_header() [%1] %2" )
					   .arg( archive_errno( arch ) )
					   .arg( archive_error_string( arch ) ) );
			ret = false;
			break;
		}
		QString np = dk_dir + archive_entry_pathname( entry );

		QByteArray newpath = np.toLocal8Bit();

		archive_entry_set_pathname( entry, newpath.data() );
		r = archive_read_extract( arch, entry, 0 );
		if ( r == ARCHIVE_WARN ) {
			_WARNINGLOG( QString( "archive_read_extract() [%1] %2" )
						 .arg( archive_errno( arch ) )
						 .arg( archive_error_string( arch ) ) );
		} else if ( r != ARCHIVE_OK ) {
			_ERRORLOG( QString( "archive_read_extract() [%1] %2" )
					   .arg( archive_errno( arch ) )
					   .arg( archive_error_string( arch ) ) );
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
	
	QString gzd_name = sSourcePath.left( sSourcePath.indexOf( "." ) ) + ".tar";
	FILE* gzd_file = fopen( gzd_name.toLocal8Bit(), "wb" );
	gzFile gzip_file = gzopen( sSourcePath.toLocal8Bit(), "rb" );
	if ( !gzip_file ) {
		_ERRORLOG( QString( "Error reading drumkit file: %1" )
				   .arg( sSourcePath ) );
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

	QString dk_dir;
	if ( ! sTargetPath.isEmpty() ) {
		dk_dir = sTargetPath + "/";
	} else {
		dk_dir = Filesystem::usr_drumkits_dir() + "/";
	}

	strncpy( dst_dir, dk_dir.toLocal8Bit(), 1024 );
	if ( tar_extract_all( tar_file, dst_dir ) != 0 ) {
		_ERRORLOG( QString( "tar_extract_all(): %1" )
				   .arg( QString::fromLocal8Bit( strerror( errno ) ) ) );
		ret = false;
	}
	if ( tar_close( tar_file ) != 0 ) {
		_ERRORLOG( QString( "tar_close(): %1" )
				   .arg( QString::fromLocal8Bit( strerror( errno ) ) ) );
		ret = false;
	}
	return ret;
#else // WIN32
	_ERRORLOG( "WIN32 NOT IMPLEMENTED" );
	return false;
#endif
#endif
}

bool Drumkit::exportTo( const QString& sTargetDir, const QString& sComponentName, bool bRecentVersion, bool bSilent ) {

	if ( ! Filesystem::path_usable( sTargetDir, true, false ) ) {
		ERRORLOG( QString( "Provided destination folder [%1] is not valid" )
				  .arg( sTargetDir ) );
		return false;
	}

	if ( ! bRecentVersion && sComponentName.isEmpty() ) {
		ERRORLOG( "A DrumkiComponent name is required to exported a drumkit in a format similar to the one prior to version 0.9.7" );
		return false;
	}

	// When performing an export of a single component, the resulting
	// file will be <DRUMKIT_NAME>_<COMPONENT_NAME>.h2drumkit. This
	// itself is nice because the user can not choose the name of the
	// resulting file and it would not be possible to store the export
	// of multiple components in a single folder otherwise. But if all
	// those different .h2drumkit would be extracted into the same
	// folder there would be easily confusion or maybe even loss of
	// data. We thus temporary rename the drumkit within this
	// function.
	// If a legacy export is asked for (!bRecentVersion) the suffix
	// "_legacy" will be appended as well in order to provide unique
	// filenames for all export options of a drumkit that can be
	// selected in the GUI.
	QString sOldDrumkitName = __name;
	QString sDrumkitName = getExportName( sComponentName, bRecentVersion );
	
	QString sTargetName = sTargetDir + "/" + sDrumkitName +
		Filesystem::drumkit_ext;
	
	if ( ! bSilent ) {
		QString sMsg( "Export ");
		
		if ( sComponentName.isEmpty() && bRecentVersion ) {
			sMsg.append( "drumkit " );
		} else {
			sMsg.append( QString( "component: [%1] " ).arg( sComponentName ) );
		}

		sMsg.append( QString( "to [%1] " ).arg( sTargetName ) );

		if ( bRecentVersion ) {
			sMsg.append( "using the most recent format" );
		} else {
			sMsg.append( "using the legacy format supported by Hydrogen versions <= 0.9.6" );
		}

		INFOLOG( sMsg );
	}
	
	// Unique temporary folder to save intermediate drumkit.xml and
	// component files. The uniqueness is required in case several
	// threads or instances of Hydrogen do export a drumkit at once.
	QTemporaryDir tmpFolder( Filesystem::tmp_dir() + "/XXXXXX" );
	if ( ! sComponentName.isEmpty() ) {
		tmpFolder.setAutoRemove( false );
	}

	// In case we just export a single component, we store a pruned
	// version of the drumkit with all other DrumkitComponents removed
	// from the Instruments in a temporary folder and use this one as
	// a basis for further compression.
	int nComponentID = -1;
	if ( ! sComponentName.isEmpty() ) {
		for ( auto pComponent : *__components ) {
			if( pComponent->get_name().compare( sComponentName ) == 0) {
				nComponentID = pComponent->get_id();
				set_name( sDrumkitName );
				break;
			}
		}
		if ( nComponentID == -1 ) {
			ERRORLOG( QString( "Component [%1] could not be found in current Drumkit [%2]" )
					  .arg( sComponentName )
					  .arg( toQString( "", true ) ) );
			set_name( sOldDrumkitName );
			return false;
		}
		if ( ! save( tmpFolder.path(), nComponentID, bRecentVersion, bSilent ) ) {
			ERRORLOG( QString( "Unable to save backup drumkit to [%1] using component ID [%2]" )
					  .arg( tmpFolder.path() ).arg( nComponentID ) );
		}
	}

	if ( ! Filesystem::dir_readable( __path, true ) ) {
		ERRORLOG( QString( "Unabled to access folder associated with drumkit [%1]" )
				  .arg( __path ) );
		set_name( sOldDrumkitName );
		return false;
	}
	
	QDir sourceDir( __path );

	QStringList sourceFilesList = sourceDir.entryList( QDir::Files );
	// In case just a single component is exported, we only add
	// samples associated with it to the .h2drumkit file.
	QStringList filesUsed;

	// List of formats libsndfile is able to import (see
	// https://libsndfile.github.io/libsndfile/api.html#open).
	// This list is used to decide what will happen to a file on a
	// single-component export in case the file is not associated with
	// a sample of an instrument belonging to the exported
	// component. If its suffix is contained in this list, the file is
	// considered to be part of an instrument we like to drop. If not,
	// it might be a metafile, like LICENSE, README, or the kit's
	// image.
	// The list does not have to be comprehensive as a "leakage" of
	// audio files in the resulting .h2drumkit is not a big problem.
	QStringList suffixBlacklist;
	suffixBlacklist << "wav" << "flac" << "aifc" << "aif" << "aiff" << "au"
					 << "caf" << "w64" << "ogg" << "pcm" << "l16" << "vob"
					 << "mp1" << "mp2" << "mp3";
	
	bool bSampleFound;
	
	for ( const auto& ssFile : sourceFilesList ) {
		if( ssFile.compare( Filesystem::drumkit_xml() ) == 0 &&
			nComponentID != -1 ) {
			filesUsed << Filesystem::drumkit_file( tmpFolder.path() );
		} else {

			bSampleFound = false;
			for( const auto& pInstr : *( get_instruments() ) ){
				if( pInstr != nullptr ) {
					for ( auto const& pComponent : *( pInstr->get_components() ) ) {
						if ( pComponent != nullptr &&
							 ( nComponentID == -1 || 
							   pComponent->get_drumkit_componentID() == nComponentID ) ) {
							for( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
								const auto pLayer = pComponent->get_layer( n );
								if( pLayer != nullptr && pLayer->get_sample() != nullptr ) {
									if( pLayer->get_sample()->get_filename().compare( ssFile ) == 0 ) {
										filesUsed << sourceDir.filePath( ssFile );
										bSampleFound = true;
										break;
									}
								}
							}
						}
					}
				}
			}

			// Should we drop the file?
			if ( ! bSampleFound ) {
				QFileInfo ffileInfo( sourceDir.filePath( ssFile ) );
				if ( ! suffixBlacklist.contains( ffileInfo.suffix(),
												 Qt::CaseInsensitive ) ) {

					// We do not want to export any old backups
					// created during the upgrade process of the
					// drumkits.
					if ( ! ( ssFile.contains( Filesystem::drumkit_xml() ) &&
							 ssFile.contains( ".bak" ) ) ) {
						filesUsed << sourceDir.filePath( ssFile );
					}
				}
			}
		}
	}

#if defined(H2CORE_HAVE_LIBARCHIVE)

	struct archive *a;
	struct archive_entry *entry;
	struct stat st;
	char buff[8192];
	int len;
	FILE *f;

	a = archive_write_new();

	#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_write_set_compression_gzip(a);
	#else
		archive_write_add_filter_gzip(a);
	#endif

	archive_write_set_format_pax_restricted(a);
	
	int ret = archive_write_open_filename(a, sTargetName.toUtf8().constData());
	if ( ret != ARCHIVE_OK ) {
		ERRORLOG( QString("Couldn't create archive [%0]" )
			.arg( sTargetName ) );
		set_name( sOldDrumkitName );
		return false;
	}

	bool bFoundFileInRightComponent;
	for ( const auto& sFilename : filesUsed ) {
		QFileInfo ffileInfo( sFilename );
		QString sTargetFilename = sDrumkitName + "/" + ffileInfo.fileName();

		// Small sanity check since the libarchive code won't fail
		// gracefully but segfaults if the provided file does not
		// exist.
		if ( ! Filesystem::file_readable( sFilename, true ) ) {
			ERRORLOG( QString( "Unable to export drumkit. File [%1] does not exists or is not readable." )
					  .arg( sFilename ) );
			set_name( sOldDrumkitName );
			return false;
		}

		stat( sFilename.toUtf8().constData(), &st );
		entry = archive_entry_new();
		archive_entry_set_pathname(entry, sTargetFilename.toUtf8().constData());
		archive_entry_set_size(entry, st.st_size);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0644);
		archive_write_header(a, entry);
		f = fopen( sFilename.toUtf8().constData(), "rb" );
		len = fread(buff, sizeof(char), sizeof(buff), f);
		while ( len > 0 ) {
				archive_write_data(a, buff, len);
				len = fread(buff, sizeof(char), sizeof(buff), f);
		}
		fclose(f);
		archive_entry_free(entry);
	}
	archive_write_close(a);

	#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_write_finish(a);
	#else
		archive_write_free(a);
	#endif

	sourceFilesList.clear();

	// Only clean up the temp folder when everything was
	// working. Else, it's probably worth inspecting its content (and
	// the system will clean it up anyway).
	Filesystem::rm( tmpFolder.path(), true, true );
	
	set_name( sOldDrumkitName );

	return true;
#else // No LIBARCHIVE

#ifndef WIN32
	if ( nComponentID != -1 ) {
		// In order to add components name to the folder name we have
		// to copy _all_ files to a temporary folder holding the same
		// name. This is unarguably a quite expensive operation. But
		// exporting is only down sparsely and almost all versions of
		// Hydrogen should come with libarchive support anyway. On the
		// other hand, being consistent and prevent confusion and loss
		// of data beats sparsely excessive copying.
		QString sDirName = getFolderName();

		QDir sTmpSourceDir( tmpFolder.path() + "/" + sDirName );
		if ( sTmpSourceDir.exists() ) {
			sTmpSourceDir.removeRecursively();
		}
		if ( ! Filesystem::path_usable( tmpFolder.path() + "/" + sDirName,
									  true, true ) ) {
			ERRORLOG( QString( "Unable to create tmp folder [%1]" )
					  .arg( tmpFolder.path() + "/" + sDirName ) );
			set_name( sOldDrumkitName );
			return false;
		}

		QString sNewFilePath;
		QStringList copiedFiles;
		for ( const auto& ssFile : filesUsed ) {
			QString sNewFilePath( ssFile );
			sNewFilePath.replace( sNewFilePath.left( sNewFilePath.lastIndexOf( "/" ) ),
								  tmpFolder.path() + "/" + sDirName );
			if ( ! Filesystem::file_copy( ssFile, sNewFilePath, true, true ) ) {
				ERRORLOG( QString( "Unable to copy file [%1] to [%2]." )
						  .arg( ssFile ).arg( sNewFilePath ) );
				set_name( sOldDrumkitName );
				return false;
			}

			copiedFiles << sNewFilePath;
		}

		filesUsed = copiedFiles;
		sourceDir = QDir( tmpFolder.path() + "/" + sDirName );
	}

	// Since there is no way to alter the target names of the files
	// provided to command line `tar` and we want the output to be
	// identically to the only created used libarchive, we need to do
	// some string replacement in here. If not, the unpack to
	// ./home/USER_NAME_RUNNING_THE_EXPORT/.hydrogen/data/drumkits/DRUMKIT_NAME/
	// but we instead want it to unpack to ./DRUMKIT_NAME/
	filesUsed = filesUsed.replaceInStrings( sourceDir.absolutePath(),
											sourceDir.dirName() );

	QString sCmd = QString( "tar czf %1 -C %2 -- \"%3\"" )
		.arg( sTargetName )
		.arg( sourceDir.absolutePath().left( sourceDir.absolutePath().lastIndexOf( "/" ) ) )
		.arg( filesUsed.join( "\" \"" ) );
	int nRet = std::system( sCmd.toLocal8Bit() );

	if ( nRet != 0 ) {
		ERRORLOG( QString( "Unable to export drumkit using system command:\n%1" )
				  .arg( sCmd ) );
			set_name( sOldDrumkitName );
		return false;
	}

	// Only clean up the temp folder when everything was
	// working. Else, it's probably worth inspecting its content (and
	// the system will clean it up anyway).
	Filesystem::rm( tmpFolder.path(), true, true );

	set_name( sOldDrumkitName );
			
	return true;
#else // WIN32
	ERRORLOG( "Operation not supported on Windows" );
	
	return false;
#endif
#endif // LIBARCHIVE

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
			.append( QString( "%1%2license: %3\n" ).arg( sPrefix ).arg( s ).arg( __license.toQString() ) )
			.append( QString( "%1%2image: %3\n" ).arg( sPrefix ).arg( s ).arg( __image ) )
			.append( QString( "%1%2imageLicense: %3\n" ).arg( sPrefix ).arg( s ).arg( __imageLicense.toQString() ) )
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
			.append( QString( " path: %1" ).arg( __path ) )
			.append( QString( ", name: %1" ).arg( __name ) )
			.append( QString( ", author: %1" ).arg( __author ) )
			.append( QString( ", info: %1" ).arg( __info ) )
			.append( QString( ", license: %1" ).arg( __license.toQString() ) )
			.append( QString( ", image: %1" ).arg( __image ) )
			.append( QString( ", imageLicense: %1" ).arg( __imageLicense.toQString() ) )
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
