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

#include <core/Helpers/Legacy.h>

#include "Version.h"
#include <core/Helpers/Xml.h>
#include <core/License.h>
#include <core/Basics/Song.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/DrumkitComponent.h>
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

namespace H2Core {

std::shared_ptr<InstrumentComponent> Legacy::loadInstrumentComponent(
	XMLNode* pNode,
	const QString& sDrumkitPath,
	const QString& sSongPath,
	const License& drumkitLicense,
	bool bSilent )
{
	if ( ! bSilent ) {
		WARNINGLOG( "Using back compatibility code to load instrument component" );
	}

	if ( pNode->firstChildElement( "filename" ).isNull() ) {
		// not that old but no component yet.
		auto pCompo = std::make_shared<InstrumentComponent>( 0 );

		XMLNode layerNode = pNode->firstChildElement( "layer" );
		int nLayer = 0;
		while ( ! layerNode.isNull() ) {
			if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
				ERRORLOG( QString( "Layer #%1 >= m_nMaxLayers (%2). This as well as all further layers will be omitted." )
						  .arg( nLayer )
						  .arg( InstrumentComponent::getMaxLayers() ) );
				break;
			}

			auto pLayer = InstrumentLayer::load_from(
				&layerNode, sDrumkitPath, sSongPath, drumkitLicense, bSilent );
			if ( pLayer != nullptr ) {
				pCompo->set_layer( pLayer, nLayer );
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
		QString sFilename = pNode->read_string( "filename", "", false, false, bSilent );

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
	
		auto pCompo = std::make_shared<InstrumentComponent>( 0 );
		auto pLayer = std::make_shared<InstrumentLayer>( pSample );
		pCompo->set_layer( pLayer, 0 );
		return pCompo;
	}
}

Playlist* Legacy::load_playlist( Playlist* pPlaylist, const QString& pl_path )
{
	if ( version_older_than( 0, 9, 8 ) ) {
		WARNINGLOG( QString( "this code should not be used anymore, it belongs to 0.9.6" ) );
	} else {
		WARNINGLOG( QString( "loading playlist with legacy code" ) );
	}
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

	pPlaylist->setFilename( pl_path );

	XMLNode songsNode = root.firstChildElement( "Songs" );
	if ( !songsNode.isNull() ) {
		XMLNode nextNode = songsNode.firstChildElement( "next" );
		while ( !nextNode.isNull() ) {

			QString songPath = nextNode.read_string( "song", "", false, false );
			if ( !songPath.isEmpty() ) {
				Playlist::Entry* entry = new Playlist::Entry();
				QFileInfo songPathInfo( fileInfo.absoluteDir(), songPath );
				entry->filePath = songPathInfo.absoluteFilePath();
				entry->fileExists = songPathInfo.isReadable();
				entry->scriptPath = nextNode.read_string( "script", "" );
				entry->scriptEnabled = nextNode.read_bool( "enabled", false );
				pPlaylist->add( entry );
			}

			nextNode = nextNode.nextSiblingElement( "next" );
		}
	} else {
		WARNINGLOG( "Songs node not found" );
	}
	return pPlaylist;
}

std::vector<PatternList*>* Legacy::loadPatternGroupVector( XMLNode* pNode, PatternList* pPatternList, bool bSilent ) {;

	std::vector<PatternList*>* pPatternGroupVector = new std::vector<PatternList*>;

	if ( ! bSilent ) {
		WARNINGLOG( "Using old pattern group vector code for back compatibility" );
	}
	
	XMLNode pPatternIDNode = pNode->firstChildElement( "patternID" );
	while ( ! pPatternIDNode.isNull() ) {
	
		PatternList* pPatternSequence = new PatternList();
		QString sPatId = pPatternIDNode.firstChildElement().text();

		Pattern* pPattern = nullptr;
		for ( const auto& ppPat : *pPatternList ) {
			if ( ppPat != nullptr ) {
				if ( ppPat->get_name() == sPatId ) {
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
			delete pPatternSequence;
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

	QByteArray line;
	QByteArray buf = "<?xml version='1.0' ?>\n";

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
