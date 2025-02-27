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

#include <memory>
#include <QFile>
#include <QByteArray>
#include <QTextCodec>

#include <core/Helpers/Legacy.h>

#include <core/Helpers/Xml.h>
#include <core/License.h>
#include <core/Basics/Song.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Note.h>
#include <core/Basics/Adsr.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core {

void Legacy::loadComponentNames( std::shared_ptr<InstrumentList> pInstrumentList,
								 const XMLNode &rootNode, bool bSilent ) {
	if ( pInstrumentList == nullptr ) {
		ERRORLOG( "Invalid instrument list" );
	}

	// Formerly `InstrumentComponent`s features an int parameter - called
	// `component_id` - linking it with a `DrumkitComponent` of the same Id. The
	// latter did hold the name of the component. The following map will search
	// for all components and uses ID as key and name as value.
	std::map<int, QString> componentMap;

	XMLNode componentListNode = rootNode.firstChildElement( "componentList" );
	if ( ! componentListNode.isNull() ) {
		 XMLNode componentNode =
			 componentListNode.firstChildElement( "drumkitComponent" );
		 while ( ! componentNode.isNull()  ) {
			 const int nId = componentNode.read_int(
				 "id", -1, false, false, bSilent );
			 if ( nId != -1 ) {
				 // -1 was used as the null element.
				 componentMap[ nId ] = componentNode.read_string(
					 "name", "", false, false, bSilent );
			 }

			 componentNode = componentNode.nextSiblingElement( "drumkitComponent" );
		 }
	}

	if ( componentMap.size() > 0 ) {
		const XMLNode instrumentListNode =
			rootNode.firstChildElement( "instrumentList" );
		if ( instrumentListNode.isNull() ) {
			ERRORLOG( "No <instrumentList> node found" );
			return;
		}

		XMLNode instrumentNode =
			instrumentListNode.firstChildElement( "instrument" );
		while ( !instrumentNode.isNull() ) {


			const int nInstrumentId = instrumentNode.read_int(
				"id", -2, false, false, bSilent );
			auto pInstrument = pInstrumentList->get( nInstrumentId );
			if ( pInstrument == nullptr ) {
				if ( ! bSilent ) {
					WARNINGLOG( QString( "No instrument found for ID [%1]" )
								.arg( nInstrumentId ) );
				}
				instrumentNode = instrumentNode.nextSiblingElement( "instrument" );
				continue;
			}

			// In current versions of Hydrogen there is no dedicated ID but the
			// component is defined by its position in the component vector.
			int nnComponent = 0;

			XMLNode instrumentComponentNode =
				instrumentNode.firstChildElement( "instrumentComponent" );
			while ( ! instrumentComponentNode.isNull() ) {
				const int nComponentId = instrumentComponentNode.read_int(
					"component_id", -1, false, false, bSilent );
				if ( auto search = componentMap.find( nComponentId );
					 search != componentMap.end() ){
					// There is a name available for this instrument component.
					auto pInstrumentComponent =
						pInstrument->get_component( nnComponent );
					if ( pInstrumentComponent != nullptr ) {
						pInstrumentComponent->setName(
							componentMap[ nComponentId ] );
					}
					else if ( ! bSilent ) {
						WARNINGLOG( QString( "No InstrumentComponent found for [%1]" )
									.arg( nnComponent ) );
					}
				}
				++nnComponent;
				instrumentComponentNode =
					instrumentComponentNode.nextSiblingElement( "instrumentComponent" );
			}

			instrumentNode = instrumentNode.nextSiblingElement( "instrument" );
		}
	}
}

std::shared_ptr<Drumkit> Legacy::loadEmbeddedSongDrumkit(
	const XMLNode& node, const QString& sSongPath, bool bSilent )
{

	// These old kits contain only an instrument list and former
	// DrumkitComponent and rely on sample loading per-instrument. How the kit
	// itself is called was only introduced somewhere after 1.0.0 and might not
	// be present at all. We try to determine the name and use all metadata of
	// the kit in case it is installed. If not, we just fall back to sane
	// defaults and a drumkit name indicating legacy loading.

	// Since drumkit parts were stored at root level, we have access to all
	// other data in here too.
	auto license = License( node.read_string( "license", "", false,
												false, true ) );

	// Instrument List
	//
	// By supplying no drumkit path the individual drumkit meta infos
	// stored in the 'instrument' nodes will be used.
	auto pInstrumentList = InstrumentList::load_from( node,
													  "", // sDrumkitPath
													  "", // sDrumkitName
													  sSongPath,
													  license, // per-instrument licenses
													  true, // allow composition
													  bSilent );
	if ( pInstrumentList == nullptr ) {
		return nullptr;
	}

	Legacy::loadComponentNames( pInstrumentList, node );

	QString sLastLoadedDrumkitPath =
		node.read_string( "last_loaded_drumkit", "", true, false, true );
	QString sLastLoadedDrumkitName =
		node.read_string( "last_loaded_drumkit_name", "", true, false, true );

	if ( sLastLoadedDrumkitPath.isEmpty() ) {
		// Prior to version 1.2.0 the last loaded drumkit was read
		// from the last instrument loaded and was not written to disk
		// explicitly. This caused problems the moment the user put an
		// instrument from a different drumkit at the end of the
		// instrument list. To nevertheless retrieve the last loaded
		// drumkit we will use a heuristic by taking the majority vote
		// among the loaded instruments.
		std::map<QString,int> loadedDrumkits;
		for ( const auto& ppInstrument : *pInstrumentList ) {
			if ( loadedDrumkits.find( ppInstrument->get_drumkit_path() ) !=
				 loadedDrumkits.end() ) {
				loadedDrumkits[ ppInstrument->get_drumkit_path() ] += 1;
			}
			else {
				loadedDrumkits[ ppInstrument->get_drumkit_path() ] = 1;
			}
		}

		QString sMostCommonDrumkit;
		int nMax = -1;
		for ( const auto& xx : loadedDrumkits ) {
			if ( xx.second > nMax ) {
				sMostCommonDrumkit = xx.first;
				nMax = xx.second;
			}
		}

		sLastLoadedDrumkitPath = sMostCommonDrumkit;
	}

#ifdef H2CORE_HAVE_APPIMAGE
	if ( !sLastLoadedDrumkitPath.isEmpty() ) {
		// The drumkit path contains an absolute path to the last drumkit used.
		// Since the system kits are mounted at a different (temporary) path on
		// each run of the AppImage, we need to manually adjust the path to
		// ensure consistency.
		sLastLoadedDrumkitPath =
			Filesystem::rerouteDrumkitPath( sLastLoadedDrumkitPath );
	}
#endif

	// Attempt to access the last loaded drumkit to load it into the
	// SoundLibraryDatabase in case it was a custom one (e.g. loaded via OSC or
	// from a different system data folder due to a different install prefix).
	auto pSoundLibraryDatabase = Hydrogen::get_instance()->getSoundLibraryDatabase();
	auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sLastLoadedDrumkitPath );

	if ( pDrumkit == nullptr && !sLastLoadedDrumkitName.isEmpty() ) {
		// Loading by path did not worked. But maybe loading by name will do
		// (per-path loading guarantees to uniquely identify kits on one system
		// but is in general not protable to other systems. Name-based lookup,
		// however, is portable as long as btoh systems have the required kit
		// installed).
		pDrumkit = pSoundLibraryDatabase->getDrumkit( sLastLoadedDrumkitName );
	}

	// Ensure we do not overwrite the original drumkit when altering the one
	// associated with the current song.
	std::shared_ptr<Drumkit> pNewDrumkit;
	if ( pDrumkit == nullptr ) {
		// We could not load a dedicated kit. Falling back to the default one.
		pNewDrumkit = std::make_shared<Drumkit>();
	} else {
		pNewDrumkit = std::make_shared<Drumkit>( pDrumkit );
	}

	// Assign the loaded parts and load samples.
	pNewDrumkit->setInstruments( pInstrumentList );

	pNewDrumkit->fixupTypes( bSilent );

	return pNewDrumkit;
}

std::shared_ptr<InstrumentComponent> Legacy::loadInstrumentComponent(
	const XMLNode& node,
	const QString& sDrumkitPath,
	const QString& sSongPath,
	const License& drumkitLicense,
	bool bSilent )
{
	if ( ! bSilent ) {
		WARNINGLOG( "Using back compatibility code to load instrument component" );
	}

	if ( node.firstChildElement( "filename" ).isNull() ) {
		// not that old but no component yet.
		auto pCompo = std::make_shared<InstrumentComponent>();

		XMLNode layerNode = node.firstChildElement( "layer" );
		int nLayer = 0;
		while ( ! layerNode.isNull() ) {
			if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
				ERRORLOG( QString( "Layer #%1 >= m_nMaxLayers (%2). This as well as all further layers will be omitted." )
						  .arg( nLayer )
						  .arg( InstrumentComponent::getMaxLayers() ) );
				break;
			}

			auto pLayer = InstrumentLayer::load_from(
				layerNode, sDrumkitPath, sSongPath, drumkitLicense, bSilent );
			if ( pLayer != nullptr ) {
				pCompo->setLayer( pLayer, nLayer );
				nLayer++;
			}
			layerNode = layerNode.nextSiblingElement( "layer" );
		}
		
		if ( nLayer == 0 ) {
			ERRORLOG( "Unable to load instrument component. Neither 'filename', 'instrumentComponent', nor 'layer' node found. Aborting." );
			return nullptr;
		}
		
		return pCompo;
	}
	else {
		// back compatibility code ( song version <= 0.9.0 )
		QString sFilename = node.read_string( "filename", "", false, false, bSilent );

		if ( ! Filesystem::file_exists( sFilename ) && ! sDrumkitPath.isEmpty() ) {
			sFilename = sDrumkitPath + "/" + sFilename;
		}
	
		auto pSample = Sample::load( sFilename, drumkitLicense );
		if ( pSample == nullptr ) {
			// nel passaggio tra 0.8.2 e 0.9.0 il drumkit di default e' cambiato.
			// Se fallisce provo a caricare il corrispettivo file in
			// formato flac
			if ( ! bSilent ) {
				WARNINGLOG( "[readSong] Error loading sample: " +
							sFilename + " not found. Trying to load a flac..." );
			}
			sFilename = sFilename.left( sFilename.length() - 4 );
			sFilename += ".flac";
			pSample = Sample::load( sFilename, drumkitLicense );
		}
		if ( pSample == nullptr ) {
			ERRORLOG( "Error loading sample: " + sFilename + " not found" );
		}
	
		auto pCompo = std::make_shared<InstrumentComponent>();
		auto pLayer = std::make_shared<InstrumentLayer>( pSample );
		pCompo->setLayer( pLayer, 0 );
		return pCompo;
	}
}

std::shared_ptr<Pattern> Legacy::loadPattern( const QString& pattern_path ) {
	WARNINGLOG( QString( "loading pattern with legacy code" ) );

	XMLDoc doc;
	if( !doc.read( pattern_path ) ) {
		return nullptr;
	}
	XMLNode root = doc.firstChildElement( "drumkit_pattern" );
	if ( root.isNull() ) {
		ERRORLOG( "drumkit_pattern node not found" );
		return nullptr;
	}
	XMLNode pattern_node = root.firstChildElement( "pattern" );
	if ( pattern_node.isNull() ) {
		WARNINGLOG( "pattern node not found" );
		return nullptr;
	}

	QString sName = pattern_node.read_string( "pattern_name", "", false, false );
	if ( sName.isEmpty() ) {
	    sName = pattern_node.read_string( "pattern_name", "unknown", false, false );
	}
	QString sInfo = pattern_node.read_string( "info", "" );
	QString sCategory = pattern_node.read_string( "category", "" );
	int nSize = pattern_node.read_int( "size", -1, false, false );
		
	//default nDenominator = 4 since old patterns have not <denominator> setting
	auto pPattern = std::make_shared<Pattern>( sName, sInfo, sCategory, nSize, 4 );

	// Try to load author and license present in .h2pattern created in Hydrogen
	// version 1.0.0-beta till 1.2.X (within the <drumkit_pattern> element).
	pPattern->setAuthor( root.read_string(
							 "author", pPattern->getAuthor(),
							 true, false, true ) );
	const License license( root.read_string(
							   "license", pPattern->getLicense().getLicenseString(),
							   true, false, true ) );
	pPattern->setLicense( license );

	XMLNode note_list_node = pattern_node.firstChildElement( "noteList" );

	if ( ! note_list_node.isNull() ) {
		// Less old version of the pattern format.
		XMLNode note_node = note_list_node.firstChildElement( "note" );

		while ( !note_node.isNull() ) {
			std::shared_ptr<Note> pNote = nullptr;
			unsigned nPosition = note_node.read_int( "position", 0 );
			float fLeadLag = note_node.read_float(
				"leadlag", LEAD_LAG_DEFAULT, false , false);
			float fVelocity = note_node.read_float( "velocity", VELOCITY_DEFAULT );
			float fPanL = note_node.read_float( "pan_L", 0.5 );
			float fPanR = note_node.read_float( "pan_R", 0.5 );
			float fPan = Sampler::getRatioPan( fPanL, fPanR ); // convert to single pan parameter

			int nLength = note_node.read_int(
				"length", LENGTH_ENTIRE_SAMPLE, true );
			float nPitch = note_node.read_float(
				"pitch", PITCH_DEFAULT, false, false );
			float fProbability = note_node.read_float(
				"probability", PROBABILITY_DEFAULT , false , false );
			QString sKey = note_node.read_string( "key", "C0", false, false );
			QString nNoteOff = note_node.read_string( "note_off", "false", false, false );
			int instrId = note_node.read_int( "instrument", 0, true );

			bool noteoff = false;
			if ( nNoteOff == "true" ) {
				noteoff = true;
			}

			pNote = std::make_shared<Note>( nullptr, nPosition, fVelocity, fPan,
											nLength, nPitch );
			pNote->setKeyOctave( sKey );
			pNote->setLeadLag( fLeadLag );
			pNote->setNoteOff( noteoff );
			pNote->setProbability( fProbability );
			pPattern->insertNote( pNote );

			note_node = note_node.nextSiblingElement( "note" );
		}
	}
	else {
		// Back compatibility code for versions < 0.9.4
		XMLNode sequenceListNode = pattern_node.firstChildElement( "sequenceList" );

		int sequence_count = 0;
		XMLNode sequenceNode = sequenceListNode.firstChildElement( "sequence" );
		while ( ! sequenceNode.isNull()  ) {
			sequence_count++;

			XMLNode noteListNode = sequenceNode.firstChildElement( "noteList" );
			XMLNode noteNode = noteListNode.firstChildElement( "note" );
			while ( !noteNode.isNull() ) {

				int nInstrId = noteNode.read_int( "instrument", EMPTY_INSTR_ID );

				// convert to single pan parameter
				float fPanL = noteNode.read_float( "pan_L", 0.5 );
				float fPanR = noteNode.read_float( "pan_R", 0.5 );
				float fPan = Sampler::getRatioPan( fPanL, fPanR );

				auto pNote = std::make_shared<Note>(
					nullptr,
					noteNode.read_int( "position", 0 ),
					noteNode.read_float( "velocity", VELOCITY_DEFAULT ),
					fPan,
					noteNode.read_int( "length", LENGTH_ENTIRE_SAMPLE, true ),
					noteNode.read_float( "pitch", PITCH_DEFAULT, false, false ) );
				pNote->setLeadLag( noteNode.read_float(
										 "leadlag", LEAD_LAG_DEFAULT, false, false ) );

				pPattern->insertNote( pNote );

				noteNode = noteNode.nextSiblingElement( "note" );
			}
			sequenceNode = sequenceNode.nextSiblingElement( "sequence" );
		}
	}
	
	return pPattern;
}

std::shared_ptr<Playlist> Legacy::load_playlist( const QString& pl_path )
{
	WARNINGLOG( QString( "loading playlist with legacy code" ) );

	XMLDoc doc;
	if( !doc.read( pl_path ) ) {
		return nullptr;
	}
	XMLNode root = doc.firstChildElement( "playlist" );
	if ( root.isNull() ) {
		ERRORLOG( "playlist node not found" );
		return nullptr;
	}
	QFileInfo fileInfo = QFileInfo( pl_path );
	QString filename = root.read_string( "Name", "", false, false );
	if ( filename.isEmpty() ) {
		WARNINGLOG( "Playlist has no name, abort" );
	}

	auto pPlaylist = std::make_shared<Playlist>();
	pPlaylist->setFilename( pl_path );

	XMLNode songsNode = root.firstChildElement( "Songs" );
	if ( !songsNode.isNull() ) {
		XMLNode nextNode = songsNode.firstChildElement( "next" );
		while ( !nextNode.isNull() ) {

			QString songPath = nextNode.read_string( "song", "", false, false );
			if ( !songPath.isEmpty() ) {
				QFileInfo songPathInfo( fileInfo.absoluteDir(), songPath );
				auto pEntry = std::make_shared<PlaylistEntry>(
					songPathInfo.absoluteFilePath(),
					nextNode.read_string( "script", "" ),
					nextNode.read_bool( "enabled", false ) );
				pPlaylist->add( pEntry );
			}

			nextNode = nextNode.nextSiblingElement( "next" );
		}
	} else {
		WARNINGLOG( "Songs node not found" );
	}
	return pPlaylist;
}

std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > Legacy::loadPatternGroupVector(
	const XMLNode& node,
	std::shared_ptr<PatternList> pPatternList,
	bool bSilent )
{

	auto pPatternGroupVector =
		std::make_shared< std::vector< std::shared_ptr<PatternList> > >();

	if ( ! bSilent ) {
		WARNINGLOG( "Using old pattern group vector code for back compatibility" );
	}
	
	XMLNode pPatternIDNode = node.firstChildElement( "patternID" );
	while ( ! pPatternIDNode.isNull() ) {
	
		auto pPatternSequence = std::make_shared<PatternList>();
		QString sPatId = pPatternIDNode.firstChildElement().text();

		std::shared_ptr<Pattern> pPattern = nullptr;
		for ( const auto& ppPat : *pPatternList ) {
			if ( ppPat != nullptr ) {
				if ( ppPat->getName() == sPatId ) {
					pPattern = ppPat;
					break;
				}
			}
		}
		
		if ( pPattern == nullptr ) {
			if ( ! bSilent ) {
				WARNINGLOG( QString( "Pattern [%1] not found in patternList." )
							.arg( sPatId ) );
			}
		}
		else {
			pPatternSequence->add( pPattern );
			pPatternGroupVector->push_back( pPatternSequence );
		}

		pPatternIDNode = pPatternIDNode.nextSiblingElement( "patternID" );
	}

	return pPatternGroupVector;
}

bool Legacy::checkTinyXMLCompatMode( QFile* pFile, bool bSilent ) {
	if ( pFile == nullptr ) {
		ERRORLOG( "Supplied file not valid" );
		return false;
	}
	
	if ( ! pFile->seek( 0 ) ) {
		ERRORLOG( QString( "Unable to move to the beginning of file [%1]. Compatibility check mmight fail." )
				  .arg( pFile->fileName() ) );
	}
	
	QString sFirstLine = pFile->readLine();
	if ( ! sFirstLine.startsWith( "<?xml" ) ) {
		WARNINGLOG( QString( "File [%1] is being read in TinyXML compatibility mode")
					.arg( pFile->fileName() ) );
		return true;
	}
	
   	return false;

}

QByteArray Legacy::convertFromTinyXML( QFile* pFile, bool bSilent ) {
	if ( pFile == nullptr ) {
		ERRORLOG( "Supplied file not valid" );
		return QByteArray();
	}
	
	if ( ! pFile->seek( 0 ) ) {
		ERRORLOG( QString( "Unable to move to the beginning of file [%1]. Converting mmight fail." )
				  .arg( pFile->fileName() ) );
	}

	QString sEncoding = QTextCodec::codecForLocale()->name();
	if ( sEncoding == "System" ) {
		sEncoding = "UTF-8";
	}
	QByteArray line;
	QByteArray buf = QString("<?xml version='1.0' encoding='%1' ?>\n")
		.arg( sEncoding )
		.toLocal8Bit();

	while ( ! pFile->atEnd() ) {
		line = pFile->readLine();
		Legacy::convertStringFromTinyXML( &line );
		buf += line;
	}

	return std::move( buf );
}
	
void Legacy::convertStringFromTinyXML( QByteArray* pString ) {

	/* When TinyXML encountered a non-ASCII character, it would
	 * simply write the character as "&#xx;" -- where "xx" is
	 * the hex character code.  However, this doesn't respect
	 * any encodings (e.g. UTF-8, UTF-16).  In XML, &#xx; literally
	 * means "the Unicode character # xx."  However, in a UTF-8
	 * sequence, this could be an escape character that tells
	 * whether we have a 2, 3, or 4-byte UTF-8 sequence.
	 *
	 * For example, the UTF-8 sequence 0xD184 was being written
	 * by TinyXML as "&#xD1;&#x84;".  However, this is the UTF-8
	 * sequence for the cyrillic small letter EF (which looks
	 * kind of like a thorn or a greek phi).  This letter, in
	 * XML, should be saved as &#x00000444;, or even literally
	 * (no escaping).  As a consequence, when &#xD1; is read
	 * by an XML parser, it will be interpreted as capital N
	 * with a tilde (~).  Then &#x84; will be interpreted as
	 * an unknown or control character.
	 *
	 * So, when we know that TinyXML wrote the file, we can
	 * simply exchange these hex sequences to literal bytes.
	 */
	int nPos = 0;

	nPos = pString->indexOf( "&#x" );
	while ( nPos != -1 ) {
		if ( isxdigit( pString->at( nPos + 3 ) ) &&
			 isxdigit( pString->at( nPos + 4 ) ) &&
			 pString->at( nPos + 5 ) == ';' ) {
			
			char w1 = pString->at( nPos + 3 );
			char w2 = pString->at( nPos + 4 );

			w1 = tolower( w1 ) - 0x30;  // '0' = 0x30
			if ( w1 > 9 ) {
				w1 -= 0x27;  // '9' = 0x39, 'a' = 0x61
			}
			w1 = ( w1 & 0xF );

			w2 = tolower( w2 ) - 0x30;  // '0' = 0x30
			if ( w2 > 9 ) {
				w2 -= 0x27;  // '9' = 0x39, 'a' = 0x61
			}
			w2 = ( w2 & 0xF );

			char ch = ( w1 << 4 ) | w2;
			(*pString)[nPos] = ch;
			++nPos;
			pString->remove( nPos, 5 );
		}
		nPos = pString->indexOf( "&#x" );
	}
}
};

/* vim: set softtabstop=4 noexpandtab: */
