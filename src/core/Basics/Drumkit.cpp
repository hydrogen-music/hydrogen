/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <QFile>
#include <QDataStream>

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
#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>

#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>
#include <core/Helpers/Legacy.h>

#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/NsmClient.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core
{

Drumkit::Drumkit() : m_context( Context::Song ),
					 m_sName( "empty" ),
					 m_nVersion( 0 ),
					 m_sAuthor( "undefined author" ),
					 m_sInfo( "No information available." ),
					 m_license( License() ),
					 m_sImage( "" ),
					 m_imageLicense( License() ),
					 m_pInstruments( std::make_shared<InstrumentList>() )
{
	QDir usrDrumkitPath( Filesystem::usr_drumkits_dir() );
	m_sPath = usrDrumkitPath.filePath( m_sName );
}

Drumkit::Drumkit( std::shared_ptr<Drumkit> other ) :
	Object(),
	m_context( other->getContext() ),
	m_sPath( other->getPath() ),
	m_sName( other->getName() ),
	m_nVersion( other->m_nVersion ),
	m_sAuthor( other->getAuthor() ),
	m_sInfo( other->getInfo() ),
	m_license( other->getLicense() ),
	m_sImage( other->getImage() ),
	m_imageLicense( other->getImageLicense() )
{
	m_pInstruments = std::make_shared<InstrumentList>( other->getInstruments() );
}

Drumkit::~Drumkit()
{
}

std::shared_ptr<Drumkit> Drumkit::getEmptyDrumkit() {

	/*: Name assigned to a fresh Drumkit created via the Main Menu > Drumkit >
	 *  New. */
	const QString sDrumkitName = QT_TRANSLATE_NOOP( "Drumkit", "New Drumkit");

	auto pDrumkit = std::make_shared<Drumkit>();
	auto pInstrList = std::make_shared<InstrumentList>();
	auto pNewInstr = std::make_shared<Instrument>( 0 );
	pInstrList->add( pNewInstr );
	pDrumkit->setInstruments( pInstrList );
	pDrumkit->setName( sDrumkitName );

	return pDrumkit;
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
		// definition. It's probably an old one. loadFrom() will try
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
		Drumkit::loadFrom( root, sDrumkitFile.left( sDrumkitFile.lastIndexOf( "/" ) ),
						   "", false, bSilent );
	
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unable to load drumkit [%1]" ).arg( sDrumkitFile ) );
		return nullptr;
	}

	pDrumkit->setContext( DetermineContext( pDrumkit->getPath() ) );

	if ( ! bReadingSuccessful && bUpgrade ) {
		pDrumkit->upgrade( bSilent );
	}

	return pDrumkit;
}

std::shared_ptr<Drumkit> Drumkit::loadFrom( const XMLNode& node,
											const QString& sDrumkitPath,
											const QString& sSongPath,
											bool bSongKit,
											bool bSilent )
{
	QString sDrumkitName = node.read_string( "name", "", false, false, bSilent );
	if ( sDrumkitName.isEmpty() ) {
		ERRORLOG( "Drumkit has no name, abort" );
		return nullptr;
	}
	
	std::shared_ptr<Drumkit> pDrumkit = std::make_shared<Drumkit>();

	pDrumkit->m_sPath = sDrumkitPath;
	pDrumkit->m_sName = sDrumkitName;
	pDrumkit->m_nVersion = node.read_int(
		"userVersion", pDrumkit->m_nVersion, true, false, bSilent );
	pDrumkit->m_sAuthor = node.read_string( "author", "undefined author",
											true, true, true );
	pDrumkit->m_sInfo = node.read_string( "info", "No information available.",
										  true, true, bSilent  );

	License license( node.read_string( "license", "undefined license",
										true, true, bSilent  ),
					 pDrumkit->m_sAuthor );
	pDrumkit->setLicense( license );

	// As of 2022 we have no drumkits featuring an image in
	// stock. Thus, verbosity of this one will be turned of in order
	// to make to log more concise.
	pDrumkit->setImage( node.read_string( "image", "",
											true, true, true ) );
	License imageLicense( node.read_string( "imageLicense", "undefined license",
											 true, true, true  ),
						  pDrumkit->m_sAuthor );
	pDrumkit->setImageLicense( imageLicense );

	auto pInstrumentList = InstrumentList::loadFrom(
		node, sDrumkitPath, sDrumkitName, sSongPath, license, bSongKit, false );
	// Required to assure backward compatibility.
	if ( pInstrumentList == nullptr ) {
		WARNINGLOG( "instrument list could not be loaded. Using empty one." );
		pInstrumentList = std::make_shared<InstrumentList>();
	}

	// For kits created between 0.9.7 and 1.2.X, retrieve InstrumentComponent
	// names from former DrumkitComponents.
	XMLNode componentListNode = node.firstChildElement( "componentList" );
	if ( ! componentListNode.isNull() ) {
		Legacy::loadComponentNames( pInstrumentList, node );
	}
		
	pDrumkit->setInstruments( pInstrumentList );

	if ( ! bSongKit ) {
		// Instead of making the *::loadFrom() functions more complex by
		// passing the license down to each sample, we will make the
		// drumkit assign its license to each sample in here.
		pDrumkit->propagateLicense();
	}

	// Sanity checks
	//
	// Check for duplicates in instrument types. If we found one, we replace
	// every but the first occurrence by an empty string.
	std::set<DrumkitMap::Type> types;
	QStringList duplicates;
	for ( const auto& ppInstrument : *pDrumkit->m_pInstruments ) {
		if ( ppInstrument != nullptr && ! ppInstrument->getType().isEmpty() ) {
			const auto [ _, bSuccess ] = types.insert( ppInstrument->getType() );
			if ( ! bSuccess ) {
				duplicates << ppInstrument->getType();
				ppInstrument->setType( "" );
			}
		}
	}
	if ( duplicates.size() > 0 ) {
		ERRORLOG( QString( "Instrument type [%1] has been used more than once!" )
				  .arg( duplicates.join( ", " ) ) );
	}

	pDrumkit->fixupTypes( bSilent );

	return pDrumkit;
}

void Drumkit::fixupTypes( bool bSilent ) {
	// In case no instrument types are defined in the loaded drumkit, we
	// check whether there is .h2map file in the shipped with the installation
	// corresponding to the name of the kit.
	bool bMissingType = false;
	for ( const auto& ppInstrument : *m_pInstruments ) {
		if ( ppInstrument != nullptr && ppInstrument->getType().isEmpty() ) {
			bMissingType = true;
			break;
		}
	}

	if ( ! bMissingType ) {
		return;
	}

	if ( bMissingType ) {
		const QString sMapFile =
			Filesystem::getDrumkitMap( getExportName(), bSilent );

		if ( ! sMapFile.isEmpty() ) {
			const auto pDrumkitMap = DrumkitMap::load( sMapFile, bSilent );
			if ( pDrumkitMap != nullptr ) {
				// We do not replace any type but only set those not defined
				// yet.
				for ( const auto& ppInstrument : *m_pInstruments ) {
					if ( ppInstrument != nullptr &&
						 ppInstrument->getType().isEmpty() &&
						 ! pDrumkitMap->getType( ppInstrument->getId() ).isEmpty() ) {
						ppInstrument->setType(
							pDrumkitMap->getType( ppInstrument->getId() ) );
					}
				}
			}
			else {
				ERRORLOG( QString( "Unable to load .h2map file [%1] to replace missing Types of instruments for drumkit [%2]" )
						  .arg( sMapFile ).arg( getExportName() ) );
			}
		}
		else if ( ! bSilent ) {
			INFOLOG( QString( "There are missing Types for instruments in drumkit [%1] and no corresponding .h2map file found." )
					 .arg( getExportName() ) );
		}
	}
}

void Drumkit::upgrade( bool bSilent ) {
	if ( !bSilent ) {
		INFOLOG( QString( "Upgrading drumkit [%1] in [%2]" )
				 .arg( m_sName ).arg( m_sPath ) );
	}

	QString sBackupFile = Filesystem::drumkit_backup_path(
		Filesystem::drumkit_file( m_sPath ) );
	Filesystem::file_copy( Filesystem::drumkit_file( m_sPath ),
						   sBackupFile,
						   false, // do not overwrite existing files
						   bSilent );

	save( "", bSilent);
}

void Drumkit::loadSamples( float fBpm ) {
	INFOLOG( QString( "Loading drumkit %1 instrument samples" ).arg( m_sName ) );
	m_pInstruments->loadSamples( fBpm );
}

void Drumkit::unloadSamples() {
	INFOLOG( QString( "Unloading drumkit %1 instrument samples" ).arg( m_sName ) );
	m_pInstruments->unloadSamples();
}

const bool Drumkit::areSamplesLoaded() const {
	return m_pInstruments->isAnyInstrumentSampleLoaded();
}

bool Drumkit::hasMissingSamples() const {
	for ( const auto& ppInstrument : *m_pInstruments ) {
		if ( ppInstrument != nullptr && ppInstrument->hasMissingSamples() ) {
			return true;
		}
	}

	return false;
}

QString Drumkit::getExportName() const {
	return Filesystem::validateFilePath( m_sName );
}

bool Drumkit::save( const QString& sDrumkitPath, bool bSilent )
{
	QString sDrumkitFolder( sDrumkitPath );
	if ( sDrumkitPath.isEmpty() ) {
		sDrumkitFolder = m_sPath;
	}
	else {
		// We expect the path to a folder in sDrumkitPath. But in case
		// the user or developer provided the path to the drumkit.xml
		// file within this folder we don't play dumb as such things
		// happen and are plausible when just looking at the
		// function's signature
		QFileInfo fi( sDrumkitPath );
		if ( fi.isFile() && fi.fileName() == Filesystem::drumkit_xml() ) {
			WARNINGLOG( QString( "Please provide the path to the drumkit folder instead to the drumkit.xml file within: [%1]" )
					 .arg( sDrumkitPath ) );
			sDrumkitFolder = fi.dir().absolutePath();
		}
	}
	
	if ( ! Filesystem::dir_exists( sDrumkitFolder, true ) &&
		 ! Filesystem::mkdir( sDrumkitFolder ) ) {
		ERRORLOG( QString( "Unable to export drumkit [%1] to [%2]. Could not create drumkit folder." )
			 .arg( m_sName ).arg( sDrumkitFolder ) );
		return false;
	}

	if ( Filesystem::dir_exists( sDrumkitFolder, bSilent ) &&
		 ! Filesystem::dir_writable( sDrumkitFolder, bSilent ) ) {
		ERRORLOG( QString( "Unable to export drumkit [%1] to [%2]. Drumkit folder not writable." )
			 .arg( m_sName ).arg( sDrumkitFolder ) );
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "Saving drumkit [%1] into [%2]" )
				 .arg( m_sName ).arg( sDrumkitFolder ) );
	}

	// Save external files
	if ( ! saveSamples( sDrumkitFolder, bSilent ) ) {
		ERRORLOG( QString( "Unable to save samples of drumkit [%1] to [%2]. Abort." )
				  .arg( m_sName ).arg( sDrumkitFolder ) );
		return false;
	}
	
	if ( ! saveImage( sDrumkitFolder, bSilent ) ) {
		ERRORLOG( QString( "Unable to save image of drumkit [%1] to [%2]. Abort." )
				  .arg( m_sName ).arg( sDrumkitFolder ) );
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
	if ( m_license.getType() == License::GPL ) {
		root.appendChild( doc.createComment( License::getGPLLicenseNotice( m_sAuthor ) ) );
	}
	
	saveTo( root, false, bSilent );
	return doc.write( Filesystem::drumkit_file( sDrumkitFolder ) );
}

void Drumkit::saveTo( XMLNode& node,
					  bool bSongKit,
					  bool bSilent ) const
{
	node.write_int( "formatVersion", nCurrentFormatVersion );
	node.write_string( "name", m_sName );
	node.write_int( "userVersion", m_nVersion );
	node.write_string( "author", m_sAuthor );
	node.write_string( "info", m_sInfo );
	node.write_string( "license", m_license.getLicenseString() );

	QString sImage;
	if ( bSongKit ) {
		sImage = m_sImage;
	}
	else {
		// Other routines take care of copying the image into the (top level of
		// the) selected drumkit folder. We can thus just store the file name of
		// the image.
		sImage = QFileInfo( Filesystem::removeUniquePrefix( m_sImage, true ) )
			.fileName();
	}
	node.write_string( "image", sImage );
	node.write_string( "imageLicense", m_imageLicense.getLicenseString() );

	if ( m_pInstruments != nullptr && m_pInstruments->size() > 0 ) {
		m_pInstruments->saveTo( node, bSongKit );
	}
	else {
		WARNINGLOG( "Drumkit has no instruments. Storing an InstrumentList with a single empty Instrument as fallback." );
		auto pInstrumentList = std::make_shared<InstrumentList>();
		auto pInstrument = std::make_shared<Instrument>();
		pInstrumentList->insert( 0, pInstrument );
		pInstrumentList->saveTo( node, bSongKit );
	}
}

bool Drumkit::saveSamples( const QString& sDrumkitFolder, bool bSilent ) const
{
	if ( ! bSilent ) {
		INFOLOG( QString( "Saving drumkit [%1] samples into [%2]" )
				 .arg( m_sName ).arg( sDrumkitFolder ) );
	}

	auto pInstrList = getInstruments();
	for ( int i = 0; i < pInstrList->size(); i++ ) {
		auto pInstrument = ( *pInstrList )[i];
		for ( const auto& pComponent : *pInstrument->getComponents() ) {
			if ( pComponent != nullptr ) {
				for ( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
					auto pLayer = pComponent->getLayer( n );
					if ( pLayer != nullptr && pLayer->getSample() != nullptr ) {
						QString src = pLayer->getSample()->getFilepath();
						QString dst = sDrumkitFolder + "/" + pLayer->getSample()->getFilename();

						if ( src != dst ) {
							QString original_dst = dst;

							// If the destination path does not have an extension and there is a dot in the path, hell will break loose. QFileInfo maybe?
							int insertPosition = original_dst.length();
							if ( original_dst.lastIndexOf(".") > 0 ) {
								insertPosition = original_dst.lastIndexOf(".");
							}

							pLayer->getSample()->setFilename( dst );

							if( ! Filesystem::file_copy( src, dst, bSilent ) ) {
								return false;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

bool Drumkit::saveImage( const QString& sDrumkitDir, bool bSilent ) const
{
	if ( m_sImage.isEmpty() ) {
		// No image
		return true;
	}

	// In case we deal with a song kit the associated image was stored in
	// Hydrogen's cache folder (as saving it next to the .h2song file would be
	// not practical and error prone). In order to preserve the original
	// filename, a random prefix was introduced. This has to be stripped first.
	QString sTargetImagePath( getAbsoluteImagePath() );
	QString sTargetImageName( getAbsoluteImagePath() );
	if ( m_context == Context::Song ) {
		sTargetImageName = Filesystem::removeUniquePrefix( sTargetImagePath );
	}

	if ( sTargetImagePath.contains( sDrumkitDir ) ) {
		// Image is already present in target folder.
		return true;
	}

	QFileInfo info( sTargetImageName );
	const QString sDestination =
		QDir( sDrumkitDir ).absoluteFilePath( info.fileName() );

	if ( Filesystem::file_exists( sTargetImagePath, bSilent ) ) {
		if ( ! Filesystem::file_copy( sTargetImagePath, sDestination, bSilent ) ) {
			ERRORLOG( QString( "Error copying image [%1] to [%2]")
					  .arg( sTargetImagePath ).arg( sDestination ) );
			return false;
		}
	}
	return true;
}

QString Drumkit::getAbsoluteImagePath() const {
	// No image set.
	if ( m_sImage.isEmpty() ) {
		return std::move( QString() );
	}

	QFileInfo info( m_sImage );
	if ( info.isRelative() ) {
		// Image was stored as plain filename and is located in drumkit folder.
		return std::move( QDir( m_sPath ).absoluteFilePath( m_sImage ) );
	}
	else {
		return m_sImage;
	}
}

void Drumkit::setInstruments( std::shared_ptr<InstrumentList> pInstruments ) {
	m_pInstruments = pInstruments;
}


void Drumkit::removeInstrument( std::shared_ptr<Instrument> pInstrument ) {
	m_pInstruments->del( pInstrument );
}

void Drumkit::addInstrument( std::shared_ptr<Instrument> pInstrument,
							 int nIndex ) {
	if ( pInstrument == nullptr ) {
		ERRORLOG( "invalid instrument" );
		return;
	}

	// Check whether the instrument's id is valid and not present yet.
	bool bIdValid = true;
	if ( pInstrument->getId() >= 0 ) {
		for ( const auto& ppInstrument : *m_pInstruments ) {
			if ( ppInstrument != nullptr &&
				 ppInstrument->getId() == pInstrument->getId() ) {
				bIdValid = false;
				break;
			}
		}
	}
	else {
		bIdValid = false;
	}

	if ( ! bIdValid ) {
		// create a new valid ID for this instrument
		int nNewId = m_pInstruments->size();
		for ( int ii = 0; ii < m_pInstruments->size(); ++ii ) {
			bool bIsPresent = false;
			for ( const auto& ppInstrument : *m_pInstruments ) {
				if ( ppInstrument != nullptr &&
					 ppInstrument->getId() == ii ) {
					bIsPresent = true;
					break;
				}
			}

			if ( ! bIsPresent ) {
				nNewId = ii;
				break;
			}
		}

		pInstrument->setId( nNewId );
	}

	// Instrument types must be unique in a kit.
	if ( ! pInstrument->getType().isEmpty() ) {
		bool bTypeExists = false;
		const auto pDrumkitMap = toDrumkitMap();
		pDrumkitMap->getId( pInstrument->getType(), &bTypeExists );

		if ( bTypeExists ) {
			// We add a number to the type and increment it till it is unique.
			// In case there already is a trailing number, we use this one right
			// as a starting point.
			auto parts = pInstrument->getType().split( ' ' );
			bool bHasTrailingNumber = false;
			int nTrailingNumber = parts.last().toInt( &bHasTrailingNumber, 10 );
			if ( bHasTrailingNumber ) {
				parts.removeLast();
			} else {
				// No trailing number in type string.
				nTrailingNumber = 0;
			}
			QString sNewType = pInstrument->getType();

			const int nMaxAttempts = 150;
			int nnAttempt = 0;
			while ( bTypeExists ) {
				sNewType = parts.join( ' ' ).append( " " )
					.append( QString::number( ++nTrailingNumber ) );
				pDrumkitMap->getId( sNewType, &bTypeExists );

				++nnAttempt;
				if ( nnAttempt >= nMaxAttempts ) {
					ERRORLOG( "Could not find unique instrument type in time" );
					break;
				}
			}

			INFOLOG( QString( "Instrument type [%1] is already present in kit. It will be replaced by [%2]" )
					 .arg( pInstrument->getType() ).arg( sNewType ) );

			pInstrument->setType( sNewType );
		}
	}

	if ( nIndex > -1 && nIndex < m_pInstruments->size() ) {
		m_pInstruments->insert( nIndex, pInstrument );
	} else {
		m_pInstruments->add( pInstrument );
	}
}

void Drumkit::propagateLicense(){

	for ( const auto& ppInstrument : *m_pInstruments ) {
		if ( ppInstrument != nullptr ) {

			ppInstrument->setDrumkitPath( m_sPath );
			ppInstrument->setDrumkitName( m_sName );
			for ( const auto& ppInstrumentComponent : *ppInstrument->getComponents() ) {
				if ( ppInstrumentComponent != nullptr ) {
					for ( const auto& ppInstrumentLayer : *ppInstrumentComponent ) {
						if ( ppInstrumentLayer != nullptr ) {
							auto pSample = ppInstrumentLayer->getSample();
							if ( pSample != nullptr ) {
								pSample->setLicense( getLicense() );
							}
						}
					}
				}
			}
		}
	}
}

std::vector<std::shared_ptr<InstrumentList::Content>> Drumkit::summarizeContent() const {
	return m_pInstruments->summarizeContent();
}

bool Drumkit::install( const QString& sSourcePath, const QString& sTargetPath,
					   QString* pInstalledPath, bool* pEncodingIssuesDetected,
					   bool bSilent )
{
	// Ensure variables are always set/initialized.
	if ( pInstalledPath != nullptr ) {
		*pInstalledPath = "";
	}
	if ( pEncodingIssuesDetected != nullptr ) {
		*pEncodingIssuesDetected = false;
	}

	if ( sTargetPath.isEmpty() ) {
		if ( ! bSilent ) {
			INFOLOG( QString( "Install drumkit [%1]" ).arg( sSourcePath ) );
		}
		
	} else {
		if ( ! Filesystem::path_usable( sTargetPath, true, false ) ) {
			return false;
		}
		
		if ( ! bSilent ) {		
			INFOLOG( QString( "Extract drumkit from [%1] to [%2]" )
					  .arg( sSourcePath ).arg( sTargetPath ) );
		}
	}
	
#ifdef H2CORE_HAVE_LIBARCHIVE
	int nRet;

	bool bUseUtf8Encoding = true;
	if ( nullptr == setlocale( LC_ALL, "en_US.UTF-8" ) ) {
		INFOLOG( "No en_US.UTF-8 locale not available on this system" );
		bUseUtf8Encoding = false;
	}

	struct archive* a;
	struct archive_entry* entry;

	if ( ! bSilent ) {
		INFOLOG( QString( "Importing using `libarchive` version [%1]" )
				 .arg( ARCHIVE_VERSION_STRING ) );
	}

	a = archive_read_new();
	if ( a == nullptr ) {
		ERRORLOG( "Unable to create new archive" );
		return false;
	}

#if ARCHIVE_VERSION_NUMBER < 3000000
	archive_read_support_compression_all( a );
#else
	nRet = archive_read_support_filter_all( a );
	if ( nRet != ARCHIVE_OK ) {
		WARNINGLOG( QString("Couldn't add support for all filters: %1" )
				  .arg( archive_error_string( a ) ) );
	}
#endif

	nRet = archive_read_support_format_all( a );
	if ( nRet != ARCHIVE_OK ) {
		WARNINGLOG( QString("Couldn't add support for all formats: %1" )
				  .arg( archive_error_string( a ) ) );
	}

	// Shutdown version used on error. Therefore, contained commands are not
	// checked for errors themselves.
	auto tearDown = [&]() {
		archive_read_close( a );

#if ARCHIVE_VERSION_NUMBER < 3000000
		archive_read_finish( a );
#else
		archive_read_free( a );
#endif
	};

#if ARCHIVE_VERSION_NUMBER < 3000000
	const auto sSourcePathUtf8 = sSourcePath.toUtf8();
	nRet = archive_read_open_file( a, sSourcePathUtf8.constData(), 10240 );
#else
  #ifdef WIN32
	QString sSourcePathPadded = sSourcePath;
	sSourcePathPadded.append( '\0' );
	auto sourcePathW = sSourcePathPadded.toStdWString();
	nRet = archive_read_open_filename_w( a, sourcePathW.c_str(), 10240 );
  #else
	const auto sSourcePathUtf8 = sSourcePath.toUtf8();
	nRet = archive_read_open_filename( a, sSourcePathUtf8.constData(),
									   10240 );
  #endif
#endif
	if ( nRet != ARCHIVE_OK ) {
		ERRORLOG( QString( "Unable to open archive [%1] for reading: %2" )
				   .arg( sSourcePath )
				   .arg( archive_error_string( a ) ) );
		tearDown();
		return false;
	}

	QString sDrumkitDir;
	if ( ! sTargetPath.isEmpty() ) {
		sDrumkitDir = sTargetPath + "/";
	} else {
		sDrumkitDir = Filesystem::usr_drumkits_dir() + "/";
	}

	// Keep track of where the artifacts where extracted to
	QString sExtractedDir = "";
		
	while ( ( nRet = archive_read_next_header( a, &entry ) ) != ARCHIVE_EOF ) {
		if ( nRet != ARCHIVE_OK ) {
			ERRORLOG( QString( "Unable to read next archive header: %1" )
					   .arg( archive_error_string( a ) ) );
			tearDown();
			return false;
		}
		if ( entry == nullptr ) {
			ERRORLOG( "Couldn't read in next archive entry" );
			return false;
		}

		QString sNewPath = QString::fromUtf8( archive_entry_pathname_utf8( entry ) );
		if ( sNewPath.isEmpty() ) {
			sNewPath = QString( archive_entry_pathname( entry ) );
		}

		if ( sNewPath.contains( Filesystem::drumkit_xml() ) ) {
			QFileInfo newPathInfo( sNewPath );
			sExtractedDir = newPathInfo.absoluteDir().absolutePath();
		}

		if ( ! bUseUtf8Encoding ) {
			// In case `libarchive` is not able to support UTF-8 on the system,
			// we remove (a lot of) characters. Else they will be represented by
			// wacky ones and the calling routine would have no idea where the
			// resulting kit did end up.
			const auto sNewPathTrimmed = Filesystem::removeUtf8Characters( sNewPath );
			if ( sNewPathTrimmed != sNewPath ) {
				ERRORLOG( QString( "Encoding error (no UTF-8 available)! File was renamed [%1] -> [%2]" )
						  .arg( sNewPath ).arg( sNewPathTrimmed ) );
				if ( pEncodingIssuesDetected != nullptr ) {
					*pEncodingIssuesDetected = true;
				}
				sNewPath = sNewPathTrimmed;
			}
		}
		sNewPath.prepend( sDrumkitDir );

		if ( pInstalledPath != nullptr &&
			 sNewPath.contains( Filesystem::drumkit_xml() ) ) {
			// This file must be part of every kit and allows us to set this
			// variable only once.
			QFileInfo installInfo( sNewPath );
			*pInstalledPath = installInfo.absoluteDir().absolutePath();
		}
		QByteArray newpath = sNewPath.toUtf8();

		archive_entry_set_pathname( entry, newpath.data() );
		nRet = archive_read_extract( a, entry, 0 );
		if ( nRet == ARCHIVE_WARN ) {
			WARNINGLOG( QString( "While extracting content of [%1] from archive: %2" )
						 .arg( sNewPath )
						 .arg( archive_error_string( a ) ) );
		}
		else if ( nRet != ARCHIVE_OK ) {
			ERRORLOG( QString( "Unable to extract content of [%1] from archive: %2" )
					   .arg( sNewPath )
					   .arg( archive_error_string( a ) ) );
			tearDown();
			return false;
		}
	}
	nRet = archive_read_close( a );
	if ( nRet != ARCHIVE_OK ) {
		ERRORLOG( QString("Couldn't close archive: %1" )
				  .arg( archive_error_string( a ) ) );
		return false;
	}

#if ARCHIVE_VERSION_NUMBER < 3000000
	archive_read_finish( a );
#else
	nRet = archive_read_free( a );
	if ( nRet != ARCHIVE_OK ) {
		WARNINGLOG( QString("Couldn't free memory associated with archive: %1" )
				  .arg( archive_error_string( a ) ) );
	}
#endif

	return true;
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

bool Drumkit::exportTo( const QString& sTargetDir, bool* pUtf8Encoded,
						bool bSilent ) {
	if ( pUtf8Encoded != nullptr ) {
		// Ensure the variable is always set/initialized.
		*pUtf8Encoded = false;
	}

	if ( ! Filesystem::path_usable( sTargetDir, true, false ) ) {
		ERRORLOG( QString( "Provided destination folder [%1] is not valid" )
				  .arg( sTargetDir ) );
		return false;
	}

	if ( ! Filesystem::dir_readable( m_sPath, true ) ) {
		ERRORLOG( QString( "Unabled to access folder associated with drumkit [%1]" )
				  .arg( m_sPath ) );
		return false;
	}

	const QString sOldDrumkitName = m_sName;
	const QString sDrumkitName = getExportName();
	const QString sTargetName = sTargetDir + "/" + sDrumkitName +
		Filesystem::drumkit_ext;
	
	if ( ! bSilent ) {
		INFOLOG( QString( "Export drumkit to [%1]") .arg( sTargetName ) );
	}

	QDir sourceDir( m_sPath );

	QStringList sourceFilesList = sourceDir.entryList( QDir::Files );

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
	for ( const auto& fformat : Filesystem::supportedAudioFormats() ) {
		suffixBlacklist << Filesystem::AudioFormatToSuffix( fformat );
	}

	QStringList filesUsed;
	bool bSampleFound;
	for ( const auto& ssFile : sourceFilesList ) {
		bSampleFound = false;
		for ( const auto& pInstr : *( getInstruments() ) ) {
			if ( pInstr != nullptr ) {
				for ( const auto& pComponent : *( pInstr->getComponents() ) ) {
					if ( pComponent != nullptr ) {
						for ( int n = 0; n < InstrumentComponent::getMaxLayers(); n++ ) {
							const auto pLayer = pComponent->getLayer( n );
							if ( pLayer != nullptr && pLayer->getSample() != nullptr ) {
								if ( pLayer->getSample()->getFilename().compare( ssFile ) == 0 ) {
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

				// We do not want to export any old backups created during
				// the upgrade process of the drumkits. As these were
				// introduced using suffixes, like .bak.1, .bak.2, etc,
				// adding `bak` to the blacklist will not work.
				if ( ! ( ssFile.contains( Filesystem::drumkit_xml() ) &&
						 ssFile.contains( ".bak" ) ) ) {
					filesUsed << sourceDir.filePath( ssFile );
				}
			}
		}
	}

#if defined(H2CORE_HAVE_LIBARCHIVE)

	if ( ! bSilent ) {
		INFOLOG( QString( "Exporting using `libarchive` version [%1]" )
				 .arg( ARCHIVE_VERSION_STRING ) );
	}

	bool bUseUtf8Encoding = true;
	if ( nullptr == setlocale( LC_ALL, "en_US.UTF-8" ) ) {
		ERRORLOG( "No en_US.UTF-8 locale not available on this system" );
		bUseUtf8Encoding = false;
	}

	struct archive *a;
	struct archive_entry *entry;
	struct stat st;
	const int nBufferSize = 8192;
	char buff[ nBufferSize ];
	int nBytesRead, nRet;

	// Write it back for the calling routine.
	if ( pUtf8Encoded != nullptr ) {
		*pUtf8Encoded = bUseUtf8Encoding;
	}

	a = archive_write_new();
	if ( a == nullptr ) {
		ERRORLOG( "Unable to create new archive" );
		setName( sOldDrumkitName );
		return false;
	}

#if ARCHIVE_VERSION_NUMBER < 3000000
	archive_write_set_compression_gzip( a );
#else
	nRet = archive_write_add_filter_gzip( a );
	if ( nRet != ARCHIVE_OK ) {
		ERRORLOG( QString("Couldn't add GZIP filter: %1" )
				  .arg( archive_error_string( a ) ) );
		setName( sOldDrumkitName );
		return false;
	}
#endif

	nRet = archive_write_set_format_pax_restricted( a );
	if ( nRet != ARCHIVE_OK ) {
		ERRORLOG( QString("Couldn't set archive format to 'pax restricted': %1" )
				  .arg( archive_error_string( a ) ) );
		setName( sOldDrumkitName );
		return false;
	}


#ifdef WIN32
	QString sTargetNamePadded = QString( sTargetName );
	sTargetNamePadded.append( '\0' );
	const auto targetPath = sTargetNamePadded.toStdWString();
	nRet = archive_write_open_filename_w( a, targetPath.c_str() );
#else
	const auto targetPathUtf8 = sTargetName.toUtf8();
	const auto targetPath = targetPathUtf8.constData();
	nRet = archive_write_open_filename( a, targetPath );
#endif
	if ( nRet != ARCHIVE_OK ) {
		ERRORLOG( QString("Couldn't create archive [%1]: %2" )
				  .arg( targetPath )
				  .arg( archive_error_string( a ) ) );
		setName( sOldDrumkitName );
		return false;
	}

	for ( const auto& sFilename : filesUsed ) {
		QFileInfo ffileInfo( sFilename );
		QString sTargetFilename = sDrumkitName + "/" + ffileInfo.fileName();

		// Small sanity check since the libarchive code won't fail
		// gracefully but segfaults if the provided file does not
		// exist.
		if ( ! Filesystem::file_readable( sFilename, true ) ) {
			ERRORLOG( QString( "Unable to export drumkit. File [%1] does not exists or is not readable." )
					  .arg( sFilename ) );
			setName( sOldDrumkitName );
			return false;
		}

		const auto sFilenameUtf8 = sFilename.toUtf8();
		stat( sFilenameUtf8.constData(), &st );
		entry = archive_entry_new();
		if ( entry == nullptr ) {
			ERRORLOG( "Unable to create new archive entry" );
			setName( sOldDrumkitName );
			return false;
		}

		const auto sTargetFilenameUtf8 = sTargetFilename.toUtf8();
#if defined(WIN32) and ARCHIVE_VERSION_NUMBER >= 3005000
		if ( bUseUtf8Encoding ) {
			archive_entry_set_pathname_utf8(
				entry, sTargetFilenameUtf8.constData());
		} else {
#else
		{
#endif
			archive_entry_set_pathname(entry, sTargetFilenameUtf8.constData());
		}
		archive_entry_set_size(entry, st.st_size);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0644);
		nRet = archive_write_header(a, entry);
		if ( nRet != ARCHIVE_OK ) {
			ERRORLOG( QString("Couldn't write entry for [%1] to archive header: %2" )
					  .arg( sFilename )
					  .arg( archive_error_string( a ) ) );
			setName( sOldDrumkitName );
			return false;
		}

		QFile file( sFilename );
		if ( ! file.open( QIODevice::ReadOnly ) ) {
			ERRORLOG( QString( "Unable to open file [%1] for reading" )
				  .arg( sFilename ) );
			archive_entry_free( entry );
			continue;
		}

		QDataStream stream( &file );
		nBytesRead = stream.readRawData( buff, nBufferSize );
		while ( nBytesRead > 0 ) {
			nRet = archive_write_data( a, buff, nBytesRead );
			if ( nRet < 0 ) {
				ERRORLOG( QString( "Error while writing data to entry of [%1]: %2" )
						  .arg( sFilename ).arg( archive_error_string( a ) ) );
				break;
			}
			else if ( nRet != nBytesRead ) {
				WARNINGLOG( QString( "Only [%1/%2] bytes written to archive entry of [%3]" )
							.arg( nRet ).arg( nBytesRead ).arg( sFilename ) );
			}

			nBytesRead = stream.readRawData( buff, nBufferSize );
		}
		file.close();
		archive_entry_free(entry);
	}
	nRet = archive_write_close(a);
	if ( nRet != ARCHIVE_OK ) {
		ERRORLOG( QString("Couldn't close archive: %1" )
				  .arg( archive_error_string( a ) ) );
		setName( sOldDrumkitName );
		return false;
	}


#if ARCHIVE_VERSION_NUMBER < 3000000
	archive_write_finish(a);
#else
	nRet = archive_write_free(a);
	if ( nRet != ARCHIVE_OK ) {
		WARNINGLOG( QString("Couldn't free memory associated with archive: %1" )
				  .arg( archive_error_string( a ) ) );
	}
#endif

	sourceFilesList.clear();

	setName( sOldDrumkitName );

	return true;
#else // No LIBARCHIVE

#ifndef WIN32

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
			setName( sOldDrumkitName );
		return false;
	}

	setName( sOldDrumkitName );
			
	return true;
#else // WIN32
	ERRORLOG( "Operation not supported on Windows" );
	
	return false;
#endif
#endif // LIBARCHIVE

}

const QString& Drumkit::getPath() const {
	return m_sPath;
}

void Drumkit::recalculateRubberband( float fBpm ) {

	if ( !Preferences::get_instance()->getRubberBandBatchMode() ) {
		return;
	}

	if ( m_pInstruments != nullptr ) {
		for ( unsigned nnInstr = 0; nnInstr < m_pInstruments->size(); ++nnInstr ) {
			auto pInstr = m_pInstruments->get( nnInstr );
			if ( pInstr == nullptr ) {
				continue;
			}
			if ( pInstr != nullptr ){
				for ( int nnComponent = 0; nnComponent < pInstr->getComponents()->size();
					  ++nnComponent ) {
					auto pInstrumentComponent = pInstr->getComponent( nnComponent );
					if ( pInstrumentComponent == nullptr ) {
						continue;
					}

					for ( int nnLayer = 0; nnLayer < InstrumentComponent::getMaxLayers(); nnLayer++ ) {
						auto pLayer = pInstrumentComponent->getLayer( nnLayer );
						if ( pLayer != nullptr ) {
							auto pSample = pLayer->getSample();
							if ( pSample != nullptr ) {
								if( pSample->getRubberband().use ) {
									auto pNewSample = std::make_shared<Sample>( pSample );

									if ( ! pNewSample->load( fBpm ) ){
										continue;
									}

									// insert new sample from newInstrument
									pLayer->setSample( pNewSample );
								}
							}
						}
					}
				}
			}
		}
	} else {
		ERRORLOG( "No InstrumentList present" );
	}
}

Drumkit::Context Drumkit::DetermineContext( const QString& sPath ) {
	if ( ! sPath.isEmpty() ) {
		const QString sAbsolutePath = Filesystem::absolute_path( sPath );
		if ( sAbsolutePath.contains( Filesystem::sys_drumkits_dir() ) ) {
			return Context::System;
		}
		else if ( sAbsolutePath.contains( Filesystem::usr_drumkits_dir() ) ) {
			return Context::User;
		}
		else {
			if ( Filesystem::dir_writable( sAbsolutePath, true ) ) {
				return Context::SessionReadWrite;
			} else {
				return Context::SessionReadOnly;
			}
		}
	} else {
		return Context::Song;
	}
}

QString Drumkit::ContextToString( const Context& context ) {
	switch( context ) {
	case Context::System:
		return "System";
	case Context::User:
		return "User";
	case Context::SessionReadOnly:
		return "SessionReadOnly";
	case Context::SessionReadWrite:
		return "SessionReadWrite";
	case Context::Song:
		return "Song";
	default:
		return QString( "Unknown context [%1]" ).arg( static_cast<int>(context) );
	}
}

std::set<DrumkitMap::Type> Drumkit::getAllTypes() const {
	std::set<DrumkitMap::Type> types;

	for ( const auto ppInstrument : *m_pInstruments ) {
		if ( ppInstrument != nullptr && ! ppInstrument->getType().isEmpty() ) {
			const auto [ _, bSuccess ] = types.insert( ppInstrument->getType() );
			if ( ! bSuccess ) {
				WARNINGLOG( QString( "Instrument types must be unique! Type [%1] of instrument (id: %2, name: %3) will be omitted." )
							.arg( ppInstrument->getType() )
							.arg( ppInstrument->getId() )
							.arg( ppInstrument->getName() ) );
			}
		}
	}

	return types;
}

std::shared_ptr<DrumkitMap> Drumkit::toDrumkitMap() const {
	auto pMap = std::make_shared<DrumkitMap>();

	for ( const auto& ppInstrument : *m_pInstruments ) {
		if ( ppInstrument != nullptr && ! ppInstrument->getType().isEmpty() ) {
			if ( ! pMap->addMapping( ppInstrument->getId(),
									 ppInstrument->getType() ) ) {
				ERRORLOG( QString( "Unable to add type [%1] for instrument (id: %2, name: %3)" )
						  .arg( ppInstrument->getType() )
						  .arg( ppInstrument->getId() )
						  .arg( ppInstrument->getName() ) );
			}
		}
	}

	return pMap;
}

std::shared_ptr<Instrument> Drumkit::mapInstrument( const QString& sType,
													int nInstrumentId,
													std::shared_ptr<Drumkit> pOldDrumkit,
													DrumkitMap::Type* pNewType )
{
	const auto pDrumkitMap = toDrumkitMap();

	if ( pNewType != nullptr ) {
		*pNewType = QString( "" );
	}

	std::shared_ptr<Instrument> pInstrument = nullptr;

	if ( ! sType.isEmpty() ) {
		// A non-empty type can only be mapped to an instrument bearing the
		// exact same type string. (At least automatically/in here. The user has
		// various options e.g. to assign a note to arbitrary instruments in the
		// pattern editor.)
		if ( pDrumkitMap->getAllTypes().size() > 0 ) {
			bool bFound;
			const int nId = pDrumkitMap->getId( sType, &bFound );
			if ( bFound ) {
				pInstrument = m_pInstruments->find( nId );
			}

			if ( pOldDrumkit != nullptr ) {
				// Check whether we deal with the same kit and the type of an
				// instrument was changed. If so, we indicate this by providing
				// the new type string.
				//
				// Note that this is not supposed to work for an empty type.
				// Initial type adding and type removal has to be done
				// explicitly.
				const auto pOldDrumkitMap = pOldDrumkit->toDrumkitMap();
				const int nOldId = pOldDrumkitMap->getId( sType, &bFound );
				if ( getPath() == pOldDrumkit->getPath() &&
					 getName() == pOldDrumkit->getName() &&
					 bFound && nId != nOldId ) {
					pInstrument = m_pInstruments->find( nOldId );
					if ( pInstrument != nullptr && pNewType != nullptr &&
						 ! pInstrument->getType().isEmpty() ) {
						*pNewType = pInstrument->getType();
					}
				}
			}
		}
	}
	else {
		// We resort to the "historical" loading using instrument IDs. This is
		// used both for patterns created prior to version 2.0 of Hydrogen and
		// notes added to instruments without a type (either a legacy or freshly
		// created instrument).
		//
		// In case we map to a kit without or incomplete types, we try to match
		// the behavior of Hydrogen prior to version 2.0. Back then, although
		// itself being ID-based, notes were mapped according to the _order_ of
		// instruments in the source kit. This was caused by the way drumkits
		// were loaded. Instead of loading the corresponding drumkit.xml file
		// as is, the IDs of the instruments were overwritten in such a way it
		// matched the order of the previous kit.
		if ( pOldDrumkit != nullptr ) {
			auto pOldInstruments = pOldDrumkit->getInstruments();
			const auto pOldInstrument =
				pOldDrumkit->getInstruments()->find( nInstrumentId );
			if ( pOldInstrument != nullptr ) {
				const int nOldIndex = pOldInstruments->index( pOldInstrument );

				if ( nOldIndex != -1 ) {
					pInstrument = m_pInstruments->get( nOldIndex );
				}
			}
		}

		if ( pInstrument == nullptr ) {
			pInstrument = m_pInstruments->find( nInstrumentId );
		}

		// For a clean and easy to grasp concept of the automated mapping,
		// matching ID/order will only be mapped if the corresponding instrument
		// does _not_ feature a type.
		if ( pInstrument != nullptr && ! pInstrument->getType().isEmpty() ) {
			pInstrument = nullptr;
		}
	}

	return pInstrument;
}

bool Drumkit::hasMissingTypes() const {
	for ( const auto& ppInstrument : *m_pInstruments ) {
		if ( ppInstrument != nullptr && ppInstrument->getType().isEmpty() ) {
			return true;
		}
	}

	return false;
}

QString Drumkit::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Drumkit]\n" ).arg( sPrefix )
			.append( QString( "%1%2context: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( ContextToString( m_context ) ) )
			.append( QString( "%1%2path: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sPath ) )
			.append( QString( "%1%2name: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sName ) )
			.append( QString( "%1%2m_nVersion: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nVersion ) )
			.append( QString( "%1%2author: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sAuthor ) )
			.append( QString( "%1%2info: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sInfo ) )
			.append( QString( "%1%2license: %3\n" ).arg( sPrefix ).arg( s ).arg( m_license.toQString() ) )
			.append( QString( "%1%2image: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sImage ) )
			.append( QString( "%1%2imageLicense: %3\n" ).arg( sPrefix ).arg( s ).arg( m_imageLicense.toQString() ) )
			.append( QString( "%1%2samples_loaded: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_pInstruments->isAnyInstrumentSampleLoaded() ) )
			.append( QString( "%1" ).arg( m_pInstruments->toQString( sPrefix + s, bShort ) ) );
		sOutput.append( QString( "%1%2]\n" ).arg( sPrefix ).arg( s ) );

	} else {
		
		sOutput = QString( "[Drumkit]" )
			.append( QString( " context: %1" ).arg( ContextToString( m_context ) ) )
			.append( QString( ", path: %1" ).arg( m_sPath ) )
			.append( QString( ", name: %1" ).arg( m_sName ) )
			.append( QString( ", version: %1" ).arg( m_nVersion ) )
			.append( QString( ", author: %1" ).arg( m_sAuthor ) )
			.append( QString( ", info: %1" ).arg( m_sInfo ) )
			.append( QString( ", license: %1" ).arg( m_license.toQString() ) )
			.append( QString( ", image: %1" ).arg( m_sImage ) )
			.append( QString( ", imageLicense: %1" ).arg( m_imageLicense.toQString() ) )
			.append( QString( ", samples_loaded: %1" )
					 .arg( m_pInstruments->isAnyInstrumentSampleLoaded() ) )
			.append( QString( ", [%1]" ).arg( m_pInstruments->toQString( sPrefix + s, bShort ) ) );
		sOutput.append( "]\n" );
	}
	
	return sOutput;
}

};

/* vim: set softtabstop=4 noexpandtab: */
