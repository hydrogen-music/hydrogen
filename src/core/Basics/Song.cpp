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

#include "Version.h"

#include <cassert>
#include <memory>

#include <core/LocalFileMng.h>
#include <core/Preferences.h>
#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/Globals.h>
#include <core/Timeline.h>
#include <core/Basics/Song.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/AutomationPath.h>
#include <core/AutomationPathSerializer.h>
#include <core/Helpers/Xml.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#endif

#include <QDomDocument>
#include <QDir>

namespace
{

}//anonymous namespace
namespace H2Core
{

const char* Song::__class_name = "Song";

Song::Song( const QString& name, const QString& author, float bpm, float volume )
	: Object( __class_name )
	, __is_muted( false )
	, __resolution( 48 )
	, __bpm( bpm )
	, __is_modified( false )
	, __name( name )
	, __author( author )
	, __volume( volume )
	, __metronome_volume( 0.5 )
	, __pattern_list( nullptr )
	, __pattern_group_sequence( nullptr )
	, __instrument_list( nullptr )
	, __filename( "" )
	, __is_loop_enabled( false )
	, __humanize_time_value( 0.0 )
	, __humanize_velocity_value( 0.0 )
	, __swing_factor( 0.0 )
	, __song_mode( PATTERN_MODE )
	, __components( nullptr )
	, __playback_track_enabled( false )
	, __playback_track_volume( 0.0 )
	, __velocity_automation_path( nullptr )
{
	INFOLOG( QString( "INIT '%1'" ).arg( __name ) );

	__components = new std::vector<DrumkitComponent*> ();
	__velocity_automation_path = new AutomationPath(0.0f, 1.5f,  1.0f);
}



Song::~Song()
{
	/*
	 * Warning: it is not safe to delete a song without having a lock on the audio engine.
	 * Following the current design, the caller has to care for the lock.
	 */
	
	delete __pattern_list;

	for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
		delete *it;
	}
	delete __components;

	if ( __pattern_group_sequence ) {
		for ( unsigned i = 0; i < __pattern_group_sequence->size(); ++i ) {
			PatternList* pPatternList = ( *__pattern_group_sequence )[i];
			pPatternList->clear();	// pulisco tutto, i pattern non vanno distrutti qua
			delete pPatternList;
		}
		delete __pattern_group_sequence;
	}

	delete __instrument_list;

	delete __velocity_automation_path;

	INFOLOG( QString( "DESTROY '%1'" ).arg( __name ) );
}

void Song::purge_instrument( Instrument* I )
{
	for ( int nPattern = 0; nPattern < ( int )__pattern_list->size(); ++nPattern ) {
		__pattern_list->get( nPattern )->purge_instrument( I );
	}
}

///Load a song from file
Song* Song::load( const QString& filename )
{
	SongReader reader;
	return reader.readSong( filename );
}

/// Save a song to file
bool Song::save( const QString& filename )
{
	SongWriter writer;
	int err;
	err = writer.writeSong( this, filename );

	if( err ) {
		return false;
	}
	return QFile::exists( filename );
}


/// Create default song
Song* Song::get_default_song()
{
	Song* pSong = new Song( "empty", "hydrogen", 120, 0.5 );

	pSong->set_metronome_volume( 0.5 );
	pSong->set_notes( "..." );
	pSong->set_license( "" );
	pSong->set_loop_enabled( false );
	pSong->set_mode( Song::PATTERN_MODE );
	pSong->set_humanize_time_value( 0.0 );
	pSong->set_humanize_velocity_value( 0.0 );
	pSong->set_swing_factor( 0.0 );

	InstrumentList* pInstrList = new InstrumentList();
	Instrument* pNewInstr = new Instrument( EMPTY_INSTR_ID, "New instrument" );
	pInstrList->add( pNewInstr );
	pSong->set_instrument_list( pInstrList );

#ifdef H2CORE_HAVE_JACK
	Hydrogen::get_instance()->renameJackPorts( pSong );
#endif

	PatternList*	pPatternList = new PatternList();
	Pattern*		pEmptyPattern = new Pattern();
	
	pEmptyPattern->set_name( QString( "Pattern 1" ) );
	pEmptyPattern->set_category( QString( "not_categorized" ) );
	pPatternList->add( pEmptyPattern );
	pSong->set_pattern_list( pPatternList );
	
	std::vector<PatternList*>* pPatternGroupVector = new std::vector<PatternList*>;
	PatternList*               patternSequence = new PatternList();
	
	patternSequence->add( pEmptyPattern );
	pPatternGroupVector->push_back( patternSequence );
	pSong->set_pattern_group_vector( pPatternGroupVector );
	pSong->set_is_modified( false );
	pSong->set_filename( "empty_song" );

	return pSong;

}

/// Return an empty song
Song* Song::get_empty_song()
{
	Song* pSong = Song::load( Filesystem::empty_song_path() );

	/* 
	 * If file DefaultSong.h2song is not accessible,
	 * create a simple default song.
	 */
	if( !pSong ) {
		pSong = Song::get_default_song();
	}

	return pSong;
}

DrumkitComponent* Song::get_component( int ID )
{
	for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
		if( (*it)->get_id() == ID ) {
			return *it;
		}
	}

	return nullptr;
}


void Song::set_swing_factor( float factor )
{
	if ( factor < 0.0 ) {
		factor = 0.0;
	} else if ( factor > 1.0 ) {
		factor = 1.0;
	}

	__swing_factor = factor;
}

void Song::set_is_modified(bool is_modified)
{
	bool Notify = false;

	if(__is_modified != is_modified) {
		Notify = true;
	}

	__is_modified = is_modified;

	if(Notify) {
		EventQueue::get_instance()->push_event( EVENT_SONG_MODIFIED, -1 );
	
#ifdef H2CORE_HAVE_OSC
		// If Hydrogen is under session management (NSM), tell the NSM
		// server that the Song was modified.
		NsmClient* pNsmClient = NsmClient::get_instance();
		if ( pNsmClient && pNsmClient->m_bUnderSessionManagement ) {
			NsmClient::get_instance()->sendDirtyState( is_modified );
		}
#endif
	}
	
}

bool Song::has_missing_samples()
{
	InstrumentList *pInstrumentList = get_instrument_list();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		if ( pInstrumentList->get( i )->has_missing_samples() ) {
			return true;
		}
	}
	return false;
}

void Song::clear_missing_samples() {
	InstrumentList *pInstrumentList = get_instrument_list();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		pInstrumentList->get (i )->set_missing_samples( false );
	}
}

void Song::readTempPatternList( const QString& filename )
{
	XMLDoc doc;
	if( !doc.read( filename ) ) {
		return;
	}
	XMLNode root = doc.firstChildElement( "sequence" );
	if ( root.isNull() ) {
		ERRORLOG( "sequence node not found" );
		return;
	}

	XMLNode virtualsNode = root.firstChildElement( "virtuals" );
	if ( !virtualsNode.isNull() ) {
		XMLNode virtualNode = virtualsNode.firstChildElement( "virtual" );
		while ( !virtualNode.isNull() ) {
			QString patternName = virtualNode.read_attribute( "pattern", nullptr, false, false );
			XMLNode patternNode = virtualNode.firstChildElement( "pattern" );
			Pattern* pPattern = nullptr;
			while ( !patternName.isEmpty() && !patternNode.isNull() ) {
				QString virtualName = patternNode.read_text( false );
				if ( !virtualName.isEmpty() ) {
					Pattern* pVirtualPattern = nullptr;
					for ( unsigned i = 0; i < get_pattern_list()->size(); i++ ) {
						Pattern* pat = get_pattern_list()->get( i );
						if ( pPattern == nullptr && pat->get_name() == patternName ) {
							pPattern = pat;
						}
						if ( pVirtualPattern == nullptr && pat->get_name() == virtualName ) {
							pVirtualPattern = pat;
						}
						if ( pPattern != nullptr && pVirtualPattern != nullptr) {
							break;
						}
					}
					if ( pPattern == nullptr ) {
						ERRORLOG( QString( "Invalid pattern name %1" ).arg( patternName ) );
					}
					if ( pVirtualPattern == nullptr ) {
						ERRORLOG( QString( "Invalid virtual pattern name %1" ).arg( virtualName ) );
					}
					if ( pPattern != nullptr && pVirtualPattern != nullptr ) {
						pPattern->virtual_patterns_add( pVirtualPattern );
					}
				}
				patternNode = patternNode.nextSiblingElement( "pattern" );
			}
			virtualNode = virtualNode.nextSiblingElement( "virtual" );
		}
	} else {
		WARNINGLOG( "no virtuals node not found" );
	}

	get_pattern_list()->flattened_virtual_patterns_compute();
	get_pattern_group_vector()->clear();

	XMLNode sequenceNode = root.firstChildElement( "groups" );
	if ( !sequenceNode.isNull() ) {
		XMLNode groupNode = sequenceNode.firstChildElement( "group" );
		while ( !groupNode.isNull() ) {
			PatternList* patternSequence = new PatternList();
			XMLNode patternNode = groupNode.firstChildElement( "pattern" );
			while ( !patternNode.isNull() ) {
				QString patternName = patternNode.read_text( false );
				if( !patternName.isEmpty() ) {
					Pattern* p = nullptr;
					for ( unsigned i = 0; i < get_pattern_list()->size(); i++ ) {
						Pattern* pat = get_pattern_list()->get( i );
						if ( pat->get_name() == patternName ) {
							p = pat;
							break;
						}
					}
					if ( p == nullptr ) {
						ERRORLOG( QString( "Invalid pattern name %1" ).arg( patternName ) );
					} else {
						patternSequence->add( p );
					}
				}
				patternNode = patternNode.nextSiblingElement( "pattern" );
			}
			get_pattern_group_vector()->push_back( patternSequence );
			groupNode = groupNode.nextSiblingElement( "group" );
		}
	} else {
		WARNINGLOG( "no sequence node not found" );
	}
}

bool Song::writeTempPatternList( const QString& filename )
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "sequence" );

	XMLNode virtualPatternListNode = root.createNode( "virtuals" );
	for ( unsigned i = 0; i < get_pattern_list()->size(); i++ ) {
		Pattern *pPattern = get_pattern_list()->get( i );
		if ( !pPattern->get_virtual_patterns()->empty() ) {
			XMLNode node = virtualPatternListNode.createNode( "virtual" );
			node.write_attribute( "pattern", pPattern->get_name() );
			for ( Pattern::virtual_patterns_it_t virtIter = pPattern->get_virtual_patterns()->begin(); virtIter != pPattern->get_virtual_patterns()->end(); ++virtIter ) {
				node.write_string( "pattern", (*virtIter)->get_name() );
			}
		}
	}

	XMLNode patternSequenceNode = root.createNode( "groups" );
	for ( unsigned i = 0; i < get_pattern_group_vector()->size(); i++ ) {
		XMLNode node = patternSequenceNode.createNode( "group" );
		PatternList *pList = ( *get_pattern_group_vector() )[i];
		for ( unsigned j = 0; j < pList->size(); j++ ) {
			Pattern *pPattern = pList->get( j );
			node.write_string( "pattern", pPattern->get_name() );
		}
	}

	return doc.write( filename );
}

QString Song::copyInstrumentLineToString( int selectedPattern, int selectedInstrument )
{
	Instrument *pInstr = get_instrument_list()->get( selectedInstrument );
	assert( pInstr );

	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode rootNode = doc.createElement( "instrument_line" );
	//LIB_ID just in work to get better usability
	//LocalFileMng::writeXmlString( &rootNode, "LIB_ID", "in_work" );
	LocalFileMng::writeXmlString( rootNode, "author", get_author() );
	LocalFileMng::writeXmlString( rootNode, "license", get_license() );

	QDomNode patternList = doc.createElement( "patternList" );

	unsigned nPatterns = get_pattern_list()->size();
	for ( unsigned i = 0; i < nPatterns; i++ )
	{
		if (( selectedPattern >= 0) && (selectedPattern != i) ) {
			continue;
		}

		// Export pattern
		Pattern *pPattern = get_pattern_list()->get( i );

		QDomNode patternNode = doc.createElement( "pattern" );
		LocalFileMng::writeXmlString( patternNode, "pattern_name", pPattern->get_name() );

		QString category;
		if ( pPattern->get_category().isEmpty() ) {
			category = "No category";
		} else {
			category = pPattern->get_category();
		}

		LocalFileMng::writeXmlString( patternNode, "info", pPattern->get_info() );
		LocalFileMng::writeXmlString( patternNode, "category", category  );
		LocalFileMng::writeXmlString( patternNode, "size", QString("%1").arg( pPattern->get_length() ) );

		QDomNode noteListNode = doc.createElement( "noteList" );
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it)
		{
			Note *pNote = it->second;
			assert( pNote );

			// Export only specified instrument
			if (pNote->get_instrument() == pInstr)
			{
				XMLNode noteNode = doc.createElement( "note" );
				pNote->save_to( &noteNode );
				noteListNode.appendChild( noteNode );
			}
		}
		patternNode.appendChild( noteListNode );

		patternList.appendChild( patternNode );
	}

	rootNode.appendChild(patternList);

	doc.appendChild( rootNode );

	// Serialize document & return
	return doc.toString();
}

bool Song::pasteInstrumentLineFromString( const QString& serialized, int selectedPattern, int selectedInstrument, std::list<Pattern *>& patterns )
{
	QDomDocument doc;
	if (!doc.setContent(serialized)) {
		return false;
	}

	// Get current instrument
	Instrument *pInstr = get_instrument_list()->get( selectedInstrument );
	assert( pInstr );

	// Get pattern list
	PatternList *pList = get_pattern_list();
	Pattern *pSelected = (selectedPattern >= 0) ? pList->get(selectedPattern) : nullptr;
	QDomNode patternNode;
	bool bIsNoteSelection = false;
	bool is_single = true;

	// Check if document has correct structure
	QDomNode rootNode = doc.firstChildElement( "instrument_line" );	// root element
	if ( ! rootNode.isNull() ) {
		// Find pattern list
		QDomNode patternList = rootNode.firstChildElement( "patternList" );
		if ( patternList.isNull() ) {
			return false;
		}

		// Parse each pattern if needed
		patternNode = patternList.firstChildElement( "pattern" );
		if (!patternNode.isNull()) {
			is_single = (( QDomNode )patternNode.nextSiblingElement( "pattern" )).isNull();
		}

	} else {
		rootNode = doc.firstChildElement( "noteSelection" );
		if ( ! rootNode.isNull() ) {
			// Found a noteSelection. This contains a noteList, as a <pattern> does, so treat this as an anonymous pattern.
			bIsNoteSelection = true;
			is_single = true;
			patternNode = rootNode;

		} else {
			ERRORLOG( "Error pasting Clipboard:instrument_line or noteSelection node not found ");
			return false;
		}
	}

	while (!patternNode.isNull())
	{
		QString patternName(LocalFileMng::readXmlString(patternNode, "pattern_name", ""));

		// Check if pattern name specified
		if (patternName.length() > 0 || bIsNoteSelection )
		{
			// Try to find pattern by name
			Pattern* pat = pList->find(patternName);

			// If OK - check if need to add this pattern to result
			// If there is only one pattern, we always add it to list
			// If there is no selected pattern, we add all existing patterns to list (match by name)
			// Otherwise we add only existing selected pattern to list (match by name)
			if ((is_single) || ((pat != nullptr) && ((selectedPattern < 0) || (pat == pSelected))))
			{
				// Load additional pattern info & create pattern
				QString sInfo;
				sInfo = LocalFileMng::readXmlString(patternNode, "info", sInfo, false, false);
				QString sCategory;
				sCategory = LocalFileMng::readXmlString(patternNode, "category", sCategory, false, false);
				int nSize = -1;
				nSize = LocalFileMng::readXmlInt(patternNode, "size", nSize, false, false);

				// Change name of pattern to selected pattern
				if (pSelected != nullptr) {
					patternName = pSelected->get_name();
				}

				pat = new Pattern( patternName, sInfo, sCategory, nSize );

				// Parse pattern data
				QDomNode pNoteListNode = patternNode.firstChildElement( "noteList" );
				if ( ! pNoteListNode.isNull() )
				{
					// Parse note-by-note
					XMLNode noteNode = pNoteListNode.firstChildElement( "note" );
					while ( ! noteNode.isNull() )
					{
						QDomNode instrument = noteNode.firstChildElement( "instrument" );
						QDomNode instrumentText = instrument.firstChild();

						instrumentText.setNodeValue( QString::number( pInstr->get_id() ) );
						Note *pNote = Note::load_from( &noteNode, get_instrument_list() );

						pat->insert_note( pNote ); // Add note to created pattern

						noteNode = ( QDomNode ) noteNode.nextSiblingElement( "note" );
					}
				}

				// Add loaded pattern to apply-list
				patterns.push_back(pat);
			}
		}

		patternNode = ( QDomNode ) patternNode.nextSiblingElement( "pattern" );
	}

	return true;
}

//-----------------------------------------------------------------------------
//	Implementation of SongReader class
//-----------------------------------------------------------------------------

const char* SongReader::__class_name = "SongReader";

SongReader::SongReader()
	: Object( __class_name )
{
//	infoLog("init");
}

SongReader::~SongReader()
{
//	infoLog("destroy");
}


const QString SongReader::getPath ( const QString& filename )
{
	/* Try direct path */
	if ( QFile( filename ).exists() ) {
		return QFileInfo ( filename ).absoluteFilePath();
	}

	/* Try search in Session Directory */
	char* sesdir = getenv ( "SESSION_DIR" );
	if ( sesdir ) {
		INFOLOG ( "Try SessionDirectory " + QString( sesdir ) );
		QDir SesDir( sesdir );
		QString BaseFileName = QFileInfo( filename ).fileName();
		QString SesFileName = SesDir.filePath( BaseFileName );
		if ( QFile( SesFileName ).exists() ) {
			return QFileInfo( SesFileName ).absoluteFilePath();
		}
	}

	ERRORLOG( "Song file " + filename + " not found." );
	return nullptr;
}

///
/// Reads a song.
/// return nullptr = error reading song file.
///
Song* SongReader::readSong( const QString& filename )
{
	QString FileName = getPath ( filename );
	if ( FileName.isEmpty() ) {
		return nullptr;
	}

	INFOLOG( "Reading " + FileName );
	Song* pSong = nullptr;

	QDomDocument doc = LocalFileMng::openXmlDocument( FileName );
	QDomNodeList nodeList = doc.elementsByTagName( "song" );

	if( nodeList.isEmpty() ) {
		ERRORLOG( "Error reading song: song node not found" );
		return nullptr;
	}

	QDomNode songNode = nodeList.at( 0 );

	m_sSongVersion = LocalFileMng::readXmlString( songNode, "version", "Unknown version" );

	if ( m_sSongVersion != QString( get_version().c_str() ) ) {
		WARNINGLOG( "Trying to load a song created with a different version of hydrogen." );
		WARNINGLOG( "Song [" + FileName + "] saved with version " + m_sSongVersion );
	}

	float fBpm = LocalFileMng::readXmlFloat( songNode, "bpm", 120 );
	Hydrogen::get_instance()->setNewBpmJTM( fBpm );
	float fVolume = LocalFileMng::readXmlFloat( songNode, "volume", 0.5 );
	float fMetronomeVolume = LocalFileMng::readXmlFloat( songNode, "metronomeVolume", 0.5 );
	QString sName( LocalFileMng::readXmlString( songNode, "name", "Untitled Song" ) );
	QString sAuthor( LocalFileMng::readXmlString( songNode, "author", "Unknown Author" ) );
	QString sNotes( LocalFileMng::readXmlString( songNode, "notes", "..." ) );
	QString sLicense( LocalFileMng::readXmlString( songNode, "license", "Unknown license" ) );
	bool bLoopEnabled = LocalFileMng::readXmlBool( songNode, "loopEnabled", false );
	Preferences::get_instance()->setPatternModePlaysSelected( LocalFileMng::readXmlBool( songNode, "patternModeMode", true ) );
	Song::SongMode nMode = Song::PATTERN_MODE;	// Mode (song/pattern)
	QString sMode = LocalFileMng::readXmlString( songNode, "mode", "pattern" );
	if ( sMode == "song" ) {
		nMode = Song::SONG_MODE;
	}

	QString sPlaybackTrack( LocalFileMng::readXmlString( songNode, "playbackTrackFilename", "" ) );
	bool bPlaybackTrackEnabled = LocalFileMng::readXmlBool( songNode, "playbackTrackEnabled", false );
	float fPlaybackTrackVolume = LocalFileMng::readXmlFloat( songNode, "playbackTrackVolume", 0.0 );


	float fHumanizeTimeValue = LocalFileMng::readXmlFloat( songNode, "humanize_time", 0.0 );
	float fHumanizeVelocityValue = LocalFileMng::readXmlFloat( songNode, "humanize_velocity", 0.0 );
	float fSwingFactor = LocalFileMng::readXmlFloat( songNode, "swing_factor", 0.0 );

	pSong = new Song( sName, sAuthor, fBpm, fVolume );
	pSong->set_metronome_volume( fMetronomeVolume );
	pSong->set_notes( sNotes );
	pSong->set_license( sLicense );
	pSong->set_loop_enabled( bLoopEnabled );
	pSong->set_mode( nMode );
	pSong->set_humanize_time_value( fHumanizeTimeValue );
	pSong->set_humanize_velocity_value( fHumanizeVelocityValue );
	pSong->set_swing_factor( fSwingFactor );
	pSong->set_playback_track_filename( sPlaybackTrack );
	pSong->set_playback_track_enabled( bPlaybackTrackEnabled );
	pSong->set_playback_track_volume( fPlaybackTrackVolume );

	QDomNode componentListNode = songNode.firstChildElement( "componentList" );
	if ( ( ! componentListNode.isNull()  ) ) {
		QDomNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
		while ( ! componentNode.isNull()  ) {
			int id = LocalFileMng::readXmlInt( componentNode, "id", -1 );			// instrument id
			QString sName = LocalFileMng::readXmlString( componentNode, "name", "" );		// name
			float fVolume = LocalFileMng::readXmlFloat( componentNode, "volume", 1.0 );	// volume
			
			DrumkitComponent* pDrumkitComponent = new DrumkitComponent( id, sName );
			pDrumkitComponent->set_volume( fVolume );

			pSong->get_components()->push_back(pDrumkitComponent);

			componentNode = ( QDomNode ) componentNode.nextSiblingElement( "drumkitComponent" );
		}
	} else {
		DrumkitComponent* pDrumkitComponent = new DrumkitComponent( 0, "Main" );
		pSong->get_components()->push_back(pDrumkitComponent);
	}

	//  Instrument List
	InstrumentList* pInstrList = new InstrumentList();

	QDomNode instrumentListNode = songNode.firstChildElement( "instrumentList" );
	if ( ( ! instrumentListNode.isNull()  ) ) {
		// INSTRUMENT NODE
		int instrumentList_count = 0;
		QDomNode instrumentNode;
		instrumentNode = instrumentListNode.firstChildElement( "instrument" );
		while ( ! instrumentNode.isNull()  ) {
			instrumentList_count++;

			int id = LocalFileMng::readXmlInt( instrumentNode, "id", -1 );			// instrument id
			QString sDrumkit = LocalFileMng::readXmlString( instrumentNode, "drumkit", "" );	// drumkit
			Hydrogen::get_instance()->setCurrentDrumkitname( sDrumkit );
			QString sName = LocalFileMng::readXmlString( instrumentNode, "name", "" );		// name
			float fVolume = LocalFileMng::readXmlFloat( instrumentNode, "volume", 1.0 );	// volume
			bool bIsMuted = LocalFileMng::readXmlBool( instrumentNode, "isMuted", false );	// is muted
			bool bIsSoloed = LocalFileMng::readXmlBool( instrumentNode, "isSoloed", false );	// is soloed
			float fPan_L = LocalFileMng::readXmlFloat( instrumentNode, "pan_L", 0.5 );	// pan L
			float fPan_R = LocalFileMng::readXmlFloat( instrumentNode, "pan_R", 0.5 );	// pan R
			float fFX1Level = LocalFileMng::readXmlFloat( instrumentNode, "FX1Level", 0.0 );	// FX level
			float fFX2Level = LocalFileMng::readXmlFloat( instrumentNode, "FX2Level", 0.0 );	// FX level
			float fFX3Level = LocalFileMng::readXmlFloat( instrumentNode, "FX3Level", 0.0 );	// FX level
			float fFX4Level = LocalFileMng::readXmlFloat( instrumentNode, "FX4Level", 0.0 );	// FX level
			float fGain = LocalFileMng::readXmlFloat( instrumentNode, "gain", 1.0, false, false );	// instrument gain

			int fAttack = LocalFileMng::readXmlInt( instrumentNode, "Attack", 0, false, false );		// Attack
			int fDecay = LocalFileMng::readXmlInt( instrumentNode, "Decay", 0, false, false );		// Decay
			float fSustain = LocalFileMng::readXmlFloat( instrumentNode, "Sustain", 1.0, false, false );	// Sustain
			int fRelease = LocalFileMng::readXmlFloat( instrumentNode, "Release", 1000.0, false, false );	// Release

			float fRandomPitchFactor = LocalFileMng::readXmlFloat( instrumentNode, "randomPitchFactor", 0.0f, false, false );

			bool bApplyVelocity = LocalFileMng::readXmlBool( instrumentNode, "applyVelocity", true );
			bool bFilterActive = LocalFileMng::readXmlBool( instrumentNode, "filterActive", false );
			float fFilterCutoff = LocalFileMng::readXmlFloat( instrumentNode, "filterCutoff", 1.0f, false );
			float fFilterResonance = LocalFileMng::readXmlFloat( instrumentNode, "filterResonance", 0.0f, false );
			QString sMuteGroup = LocalFileMng::readXmlString( instrumentNode, "muteGroup", "-1", false );
			QString sMidiOutChannel = LocalFileMng::readXmlString( instrumentNode, "midiOutChannel", "-1", false, false );
			QString sMidiOutNote = LocalFileMng::readXmlString( instrumentNode, "midiOutNote", "60", false, false );
			int nMuteGroup = sMuteGroup.toInt();
			bool isStopNote = LocalFileMng::readXmlBool( instrumentNode, "isStopNote", false );
			QString sRead_sample_select_algo = LocalFileMng::readXmlString( instrumentNode, "sampleSelectionAlgo", "VELOCITY" );

			int nMidiOutChannel = sMidiOutChannel.toInt();
			int nMidiOutNote = sMidiOutNote.toInt();

			if ( id==-1 ) {
				ERRORLOG( "Empty ID for instrument '" + sName + "'. skipping." );
				instrumentNode = ( QDomNode ) instrumentNode.nextSiblingElement( "instrument" );
				continue;
			}

			int iIsHiHat = LocalFileMng::readXmlInt( instrumentNode, "isHihat", -1, true );
			int iLowerCC = LocalFileMng::readXmlInt( instrumentNode, "lower_cc", 0, true );
			int iHigherCC = LocalFileMng::readXmlInt( instrumentNode, "higher_cc", 127, true );

			// create a new instrument
			Instrument* pInstrument = new Instrument( id, sName, new ADSR( fAttack, fDecay, fSustain, fRelease ) );
			pInstrument->set_volume( fVolume );
			pInstrument->set_muted( bIsMuted );
			pInstrument->set_soloed( bIsSoloed );
			pInstrument->set_pan_l( fPan_L );
			pInstrument->set_pan_r( fPan_R );
			pInstrument->set_drumkit_name( sDrumkit );
			pInstrument->set_apply_velocity( bApplyVelocity );
			pInstrument->set_fx_level( fFX1Level, 0 );
			pInstrument->set_fx_level( fFX2Level, 1 );
			pInstrument->set_fx_level( fFX3Level, 2 );
			pInstrument->set_fx_level( fFX4Level, 3 );
			pInstrument->set_random_pitch_factor( fRandomPitchFactor );
			pInstrument->set_filter_active( bFilterActive );
			pInstrument->set_filter_cutoff( fFilterCutoff );
			pInstrument->set_filter_resonance( fFilterResonance );
			pInstrument->set_gain( fGain );
			pInstrument->set_mute_group( nMuteGroup );
			pInstrument->set_stop_notes( isStopNote );
			pInstrument->set_hihat_grp( iIsHiHat );
			pInstrument->set_lower_cc( iLowerCC );
			pInstrument->set_higher_cc( iHigherCC );
			if ( sRead_sample_select_algo.compare("VELOCITY") == 0 ) {
				pInstrument->set_sample_selection_alg( Instrument::VELOCITY );
			} else if ( sRead_sample_select_algo.compare("ROUND_ROBIN") == 0 ) {
				pInstrument->set_sample_selection_alg( Instrument::ROUND_ROBIN );
			} else if ( sRead_sample_select_algo.compare("RANDOM") == 0 ) {
				pInstrument->set_sample_selection_alg( Instrument::RANDOM );
			}
			pInstrument->set_midi_out_channel( nMidiOutChannel );
			pInstrument->set_midi_out_note( nMidiOutNote );

			QString drumkitPath;
			if ( ( !sDrumkit.isEmpty() ) && ( sDrumkit != "-" ) ) {
				drumkitPath = Filesystem::drumkit_path_search( sDrumkit );
			} else {
				ERRORLOG( "Missing drumkit path" );
			}


			QDomNode filenameNode = instrumentNode.firstChildElement( "filename" );

			// back compatibility code ( song version <= 0.9.0 )
			if ( ! filenameNode.isNull() ) {
				WARNINGLOG( "Using back compatibility code. filename node found" );
				QString sFilename = LocalFileMng::readXmlString( instrumentNode, "filename", "" );

				if ( !QFile( sFilename ).exists() && !drumkitPath.isEmpty() ) {
					sFilename = drumkitPath + "/" + sFilename;
				}
				auto pSample = Sample::load( sFilename );
				if ( pSample == nullptr ) {
					// nel passaggio tra 0.8.2 e 0.9.0 il drumkit di default e' cambiato.
					// Se fallisce provo a caricare il corrispettivo file in formato flac
//					warningLog( "[readSong] Error loading sample: " + sFilename + " not found. Trying to load a flac..." );
					sFilename = sFilename.left( sFilename.length() - 4 );
					sFilename += ".flac";
					pSample = Sample::load( sFilename );
				}
				if ( pSample == nullptr ) {
					ERRORLOG( "Error loading sample: " + sFilename + " not found" );
					pInstrument->set_muted( true );
					pInstrument->set_missing_samples( true );
				}
				InstrumentComponent* pCompo = new InstrumentComponent ( 0 );
				InstrumentLayer* pLayer = new InstrumentLayer( pSample );
				pCompo->set_layer( pLayer, 0 );
				pInstrument->get_components()->push_back( pCompo );
			}
			//~ back compatibility code
			else {
				bool bFoundAtLeastOneComponent = false;
				QDomNode componentNode = instrumentNode.firstChildElement( "instrumentComponent" );
				while (  ! componentNode.isNull()  ) {
					bFoundAtLeastOneComponent = true;
					int id = LocalFileMng::readXmlInt( componentNode, "component_id", 0 );
					InstrumentComponent* pCompo = new InstrumentComponent( id );
					float fGainCompo = LocalFileMng::readXmlFloat( componentNode, "gain", 1.0 );
					pCompo->set_gain( fGainCompo );

					unsigned nLayer = 0;
					QDomNode layerNode = componentNode.firstChildElement( "layer" );
					while (  ! layerNode.isNull()  ) {
						if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
							ERRORLOG( QString( "nLayer (%1) > m_nMaxLayers (%2)" ).arg ( nLayer ).arg( InstrumentComponent::getMaxLayers() ) );
							continue;
						}
						//bool sIsModified = false;
						QString sFilename = LocalFileMng::readXmlString( layerNode, "filename", "" );
						bool sIsModified = LocalFileMng::readXmlBool( layerNode, "ismodified", false );
						Sample::Loops lo;
						lo.mode = Sample::parse_loop_mode( LocalFileMng::readXmlString( layerNode, "smode", "forward" ) );
						lo.start_frame = LocalFileMng::readXmlInt( layerNode, "startframe", 0 );
						lo.loop_frame = LocalFileMng::readXmlInt( layerNode, "loopframe", 0 );
						lo.count = LocalFileMng::readXmlInt( layerNode, "loops", 0 );
						lo.end_frame = LocalFileMng::readXmlInt( layerNode, "endframe", 0 );
						Sample::Rubberband ro;
						ro.use = LocalFileMng::readXmlInt( layerNode, "userubber", 0, false );
						ro.divider = LocalFileMng::readXmlFloat( layerNode, "rubberdivider", 0.0 );
						ro.c_settings = LocalFileMng::readXmlInt( layerNode, "rubberCsettings", 1 );
						ro.pitch = LocalFileMng::readXmlFloat( layerNode, "rubberPitch", 0.0 );

						float fMin = LocalFileMng::readXmlFloat( layerNode, "min", 0.0 );
						float fMax = LocalFileMng::readXmlFloat( layerNode, "max", 1.0 );
						float fGain = LocalFileMng::readXmlFloat( layerNode, "gain", 1.0 );
						float fPitch = LocalFileMng::readXmlFloat( layerNode, "pitch", 0.0, false, false );

						if ( !QFile( sFilename ).exists() && !drumkitPath.isEmpty() && !sFilename.startsWith("/")) {
							sFilename = drumkitPath + "/" + sFilename;
						}

						QString program = Preferences::get_instance()->m_rubberBandCLIexecutable;
						//test the path. if test fails, disable rubberband
						if ( QFile( program ).exists() == false ) {
							ro.use = false;
						}

						std::shared_ptr<Sample> pSample;
						if ( !sIsModified ) {
							pSample = Sample::load( sFilename );
						} else {
							EnvelopePoint pt;
							int Frame = 0;
							int Value = 0;

							Sample::VelocityEnvelope velocity;
							QDomNode volumeNode = layerNode.firstChildElement( "volume" );
							while (  ! volumeNode.isNull()  ) {
								Frame = LocalFileMng::readXmlInt( volumeNode, "volume-position", 0 );
								Value = LocalFileMng::readXmlInt( volumeNode, "volume-value", 0 );
								velocity.push_back( std::make_unique<EnvelopePoint>(Frame, Value) );
								volumeNode = volumeNode.nextSiblingElement( "volume" );
								//ERRORLOG( QString("volume-posi %1").arg(LocalFileMng::readXmlInt( volumeNode, "volume-position", 0)) );
							}

							Sample::VelocityEnvelope pan;
							QDomNode  panNode = layerNode.firstChildElement( "pan" );
							while (  ! panNode.isNull()  ) {
								Frame = LocalFileMng::readXmlInt( panNode, "pan-position", 0 );
								Value = LocalFileMng::readXmlInt( panNode, "pan-value", 0 );
								pan.push_back( std::make_unique<EnvelopePoint>(Frame, Value) );
								panNode = panNode.nextSiblingElement( "pan" );
							}

							pSample = Sample::load( sFilename, lo, ro, velocity, pan );
						}
						if ( pSample == nullptr ) {
							ERRORLOG( "Error loading sample: " + sFilename + " not found" );
							pInstrument->set_muted( true );
							pInstrument->set_missing_samples( true );
						}
						InstrumentLayer* pLayer = new InstrumentLayer( pSample );
						pLayer->set_start_velocity( fMin );
						pLayer->set_end_velocity( fMax );
						pLayer->set_gain( fGain );
						pLayer->set_pitch( fPitch );
						pCompo->set_layer( pLayer, nLayer );
						nLayer++;

						layerNode = ( QDomNode ) layerNode.nextSiblingElement( "layer" );
					}

					pInstrument->get_components()->push_back( pCompo );
					componentNode = ( QDomNode ) componentNode.nextSiblingElement( "instrumentComponent" );
				}
				if(!bFoundAtLeastOneComponent) {
					InstrumentComponent* pCompo = new InstrumentComponent( 0 );
					float fGainCompo = LocalFileMng::readXmlFloat( componentNode, "gain", 1.0 );
					pCompo->set_gain( fGainCompo );

					unsigned nLayer = 0;
					QDomNode layerNode = instrumentNode.firstChildElement( "layer" );
					while (  ! layerNode.isNull()  ) {
						if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
							ERRORLOG( QString( "nLayer (%1) > m_nMaxLayers (%2)" ).arg ( nLayer ).arg( InstrumentComponent::getMaxLayers() ) );
							continue;
						}
						QString sFilename = LocalFileMng::readXmlString( layerNode, "filename", "" );
						bool sIsModified = LocalFileMng::readXmlBool( layerNode, "ismodified", false );
						Sample::Loops lo;
						lo.mode = Sample::parse_loop_mode( LocalFileMng::readXmlString( layerNode, "smode", "forward" ) );
						lo.start_frame = LocalFileMng::readXmlInt( layerNode, "startframe", 0 );
						lo.loop_frame = LocalFileMng::readXmlInt( layerNode, "loopframe", 0 );
						lo.count = LocalFileMng::readXmlInt( layerNode, "loops", 0 );
						lo.end_frame = LocalFileMng::readXmlInt( layerNode, "endframe", 0 );
						Sample::Rubberband ro;
						ro.use = LocalFileMng::readXmlInt( layerNode, "userubber", 0, false );
						ro.divider = LocalFileMng::readXmlFloat( layerNode, "rubberdivider", 0.0 );
						ro.c_settings = LocalFileMng::readXmlInt( layerNode, "rubberCsettings", 1 );
						ro.pitch = LocalFileMng::readXmlFloat( layerNode, "rubberPitch", 0.0 );

						float fMin = LocalFileMng::readXmlFloat( layerNode, "min", 0.0 );
						float fMax = LocalFileMng::readXmlFloat( layerNode, "max", 1.0 );
						float fGain = LocalFileMng::readXmlFloat( layerNode, "gain", 1.0 );
						float fPitch = LocalFileMng::readXmlFloat( layerNode, "pitch", 0.0, false, false );

						if ( !QFile( sFilename ).exists() && !drumkitPath.isEmpty() ) {
							sFilename = drumkitPath + "/" + sFilename;
						}

						QString program = Preferences::get_instance()->m_rubberBandCLIexecutable;
						//test the path. if test fails, disable rubberband
						if ( QFile( program ).exists() == false ) {
							ro.use = false;
						}

						std::shared_ptr<Sample> pSample = nullptr;
						if ( !sIsModified ) {
							pSample = Sample::load( sFilename );
						} else {
							int Frame = 0;
							int Value = 0;

							Sample::VelocityEnvelope velocity;
							QDomNode volumeNode = layerNode.firstChildElement( "volume" );
							while (  ! volumeNode.isNull()  ) {
								Frame = LocalFileMng::readXmlInt( volumeNode, "volume-position", 0 );
								Value = LocalFileMng::readXmlInt( volumeNode, "volume-value", 0 );
								velocity.push_back( std::make_unique<EnvelopePoint>(Frame, Value) );
								volumeNode = volumeNode.nextSiblingElement( "volume" );
								//ERRORLOG( QString("volume-posi %1").arg(LocalFileMng::readXmlInt( volumeNode, "volume-position", 0)) );
							}

							Sample::VelocityEnvelope pan;
							QDomNode  panNode = layerNode.firstChildElement( "pan" );
							while (  ! panNode.isNull()  ) {
								Frame = LocalFileMng::readXmlInt( panNode, "pan-position", 0 );
								Value = LocalFileMng::readXmlInt( panNode, "pan-value", 0 );
								pan.push_back( std::make_unique<EnvelopePoint>(Frame, Value) );
								panNode = panNode.nextSiblingElement( "pan" );
							}

							pSample = Sample::load( sFilename, lo, ro, velocity, pan );
						}
						if ( pSample == nullptr ) {
							ERRORLOG( "Error loading sample: " + sFilename + " not found" );
							pInstrument->set_muted( true );
							pInstrument->set_missing_samples( true );
						}
						InstrumentLayer* pLayer = new InstrumentLayer( pSample );
						pLayer->set_start_velocity( fMin );
						pLayer->set_end_velocity( fMax );
						pLayer->set_gain( fGain );
						pLayer->set_pitch( fPitch );
						pCompo->set_layer( pLayer, nLayer );
						nLayer++;

						layerNode = ( QDomNode ) layerNode.nextSiblingElement( "layer" );
					}
					pInstrument->get_components()->push_back( pCompo );
				}
			}
			pInstrList->add( pInstrument );
			instrumentNode = ( QDomNode ) instrumentNode.nextSiblingElement( "instrument" );
		}

		if ( instrumentList_count == 0 ) {
			WARNINGLOG( "0 instruments?" );
		}
		pSong->set_instrument_list( pInstrList );
	} else {
		ERRORLOG( "Error reading song: instrumentList node not found" );
		delete pSong;
		delete pInstrList;
		return nullptr;
	}

	// Pattern list
	QDomNode patterns = songNode.firstChildElement( "patternList" );

	PatternList* pPatternList = new PatternList();
	int pattern_count = 0;

	QDomNode patternNode =  patterns.firstChildElement( "pattern" );
	while (  !patternNode.isNull()  ) {
		pattern_count++;
		Pattern* pPattern = getPattern( patternNode, pInstrList );
		if ( pPattern ) {
			pPatternList->add( pPattern );
		} else {
			ERRORLOG( "Error loading pattern" );
			delete pPatternList;
			delete pSong;
			return nullptr;
		}
		patternNode = ( QDomNode ) patternNode.nextSiblingElement( "pattern" );
	}
	if ( pattern_count == 0 ) {
		WARNINGLOG( "0 patterns?" );
	}
	pSong->set_pattern_list( pPatternList );

	// Virtual Patterns
	QDomNode  virtualPatternListNode = songNode.firstChildElement( "virtualPatternList" );
	QDomNode virtualPatternNode = virtualPatternListNode.firstChildElement( "pattern" );
	if ( !virtualPatternNode.isNull() ) {

		while (  ! virtualPatternNode.isNull()  ) {
			QString sName = "";
			sName = LocalFileMng::readXmlString( virtualPatternNode, "name", sName );

			Pattern* pCurPattern = nullptr;
			unsigned nPatterns = pPatternList->size();
			for ( unsigned i = 0; i < nPatterns; i++ ) {
				Pattern* pPattern = pPatternList->get( i );

				if ( pPattern->get_name() == sName ) {
					pCurPattern = pPattern;
					break;
				}//if
			}//for

			if ( pCurPattern != nullptr ) {
				QDomNode  virtualNode = virtualPatternNode.firstChildElement( "virtual" );
				while (  !virtualNode.isNull()  ) {
					QString virtName = virtualNode.firstChild().nodeValue();

					Pattern* virtPattern = nullptr;
					for ( unsigned i = 0; i < nPatterns; i++ ) {
						Pattern* pat = pPatternList->get( i );

						if ( pat->get_name() == virtName ) {
							virtPattern = pat;
							break;
						}//if
					}//for

					if ( virtPattern != nullptr ) {
						pCurPattern->virtual_patterns_add( virtPattern );
					} else {
						ERRORLOG( "Song had invalid virtual pattern list data (virtual)" );
					}//if
					virtualNode = ( QDomNode ) virtualNode.nextSiblingElement( "virtual" );
				}//while
			} else {
				ERRORLOG( "Song had invalid virtual pattern list data (name)" );
			}//if
			virtualPatternNode = ( QDomNode ) virtualPatternNode.nextSiblingElement( "pattern" );
		}//while
	}//if

	pPatternList->flattened_virtual_patterns_compute();

	// Pattern sequence
	QDomNode patternSequenceNode = songNode.firstChildElement( "patternSequence" );

	std::vector<PatternList*>* pPatternGroupVector = new std::vector<PatternList*>;

	// back-compatibility code..
	QDomNode pPatternIDNode = patternSequenceNode.firstChildElement( "patternID" );
	while ( ! pPatternIDNode.isNull()  ) {
		WARNINGLOG( "Using old patternSequence code for back compatibility" );
		PatternList* pPatternSequence = new PatternList();
		QString patId = pPatternIDNode.firstChildElement().text();
		ERRORLOG( patId );

		Pattern* pPattern = nullptr;
		for ( unsigned i = 0; i < pPatternList->size(); i++ ) {
			Pattern* pTmpPattern = pPatternList->get( i );
			if ( pTmpPattern ) {
				if ( pTmpPattern->get_name() == patId ) {
					pPattern = pTmpPattern;
					break;
				}
			}
		}
		if ( pPattern == nullptr ) {
			WARNINGLOG( "patternid not found in patternSequence" );
			pPatternIDNode = ( QDomNode ) pPatternIDNode.nextSiblingElement( "patternID" );
			
			delete pPatternSequence;
			
			continue;
		}
		pPatternSequence->add( pPattern );

		pPatternGroupVector->push_back( pPatternSequence );

		pPatternIDNode = ( QDomNode ) pPatternIDNode.nextSiblingElement( "patternID" );
	}

	QDomNode groupNode = patternSequenceNode.firstChildElement( "group" );
	while (  !groupNode.isNull()  ) {
		PatternList* patternSequence = new PatternList();
		QDomNode patternId = groupNode.firstChildElement( "patternID" );
		while (  !patternId.isNull()  ) {
			QString patId = patternId.firstChild().nodeValue();

			Pattern* pPattern = nullptr;
			for ( unsigned i = 0; i < pPatternList->size(); i++ ) {
				Pattern* pTmpPattern = pPatternList->get( i );
				if ( pTmpPattern ) {
					if ( pTmpPattern->get_name() == patId ) {
						pPattern = pTmpPattern;
						break;
					}
				}
			}
			if ( pPattern == nullptr ) {
				WARNINGLOG( "patternid not found in patternSequence" );
				patternId = ( QDomNode ) patternId.nextSiblingElement( "patternID" );
				continue;
			}
			patternSequence->add( pPattern );
			patternId = ( QDomNode ) patternId.nextSiblingElement( "patternID" );
		}
		pPatternGroupVector->push_back( patternSequence );

		groupNode = groupNode.nextSiblingElement( "group" );
	}

	pSong->set_pattern_group_vector( pPatternGroupVector );

#ifdef H2CORE_HAVE_LADSPA
	// reset FX
	for ( int fx = 0; fx < MAX_FX; ++fx ) {
		//LadspaFX* pFX = Effects::get_instance()->getLadspaFX( fx );
		//delete pFX;
		Effects::get_instance()->setLadspaFX( nullptr, fx );
	}
#endif

	// LADSPA FX
	QDomNode ladspaNode = songNode.firstChildElement( "ladspa" );
	if ( !ladspaNode.isNull() ) {
		int nFX = 0;
		QDomNode fxNode = ladspaNode.firstChildElement( "fx" );
		while (  !fxNode.isNull()  ) {
			QString sName = LocalFileMng::readXmlString( fxNode, "name", "" );
			QString sFilename = LocalFileMng::readXmlString( fxNode, "filename", "" );
			bool bEnabled = LocalFileMng::readXmlBool( fxNode, "enabled", false );
			float fVolume = LocalFileMng::readXmlFloat( fxNode, "volume", 1.0 );

			if ( sName != "no plugin" ) {
				// FIXME: il caricamento va fatto fare all'engine, solo lui sa il samplerate esatto
#ifdef H2CORE_HAVE_LADSPA
				LadspaFX* pFX = LadspaFX::load( sFilename, sName, 44100 );
				Effects::get_instance()->setLadspaFX( pFX, nFX );
				if ( pFX ) {
					pFX->setEnabled( bEnabled );
					pFX->setVolume( fVolume );
					QDomNode inputControlNode = fxNode.firstChildElement( "inputControlPort" );
					while ( !inputControlNode.isNull() ) {
						QString sName = LocalFileMng::readXmlString( inputControlNode, "name", "" );
						float fValue = LocalFileMng::readXmlFloat( inputControlNode, "value", 0.0 );

						for ( unsigned nPort = 0; nPort < pFX->inputControlPorts.size(); nPort++ ) {
							LadspaControlPort* port = pFX->inputControlPorts[ nPort ];
							if ( QString( port->sName ) == sName ) {
								port->fControlValue = fValue;
							}
						}
						inputControlNode = ( QDomNode ) inputControlNode.nextSiblingElement( "inputControlPort" );
					}
				}
#endif
			}
			nFX++;
			fxNode = ( QDomNode ) fxNode.nextSiblingElement( "fx" );
		}
	} else {
		WARNINGLOG( "ladspa node not found" );
	}

	Timeline* pTimeline = Hydrogen::get_instance()->getTimeline();
	pTimeline->deleteAllTempoMarkers();
	QDomNode bpmTimeLine = songNode.firstChildElement( "BPMTimeLine" );
	if ( !bpmTimeLine.isNull() ) {
		QDomNode newBPMNode = bpmTimeLine.firstChildElement( "newBPM" );
		while( !newBPMNode.isNull() ) {
			pTimeline->addTempoMarker( LocalFileMng::readXmlInt( newBPMNode, "BAR", 0 ),
									   LocalFileMng::readXmlFloat( newBPMNode, "BPM", 120.0 ) );
			newBPMNode = newBPMNode.nextSiblingElement( "newBPM" );
		}
	} else {
		WARNINGLOG( "bpmTimeLine node not found" );
	}

	pTimeline->deleteAllTags();
	QDomNode timeLineTag = songNode.firstChildElement( "timeLineTag" );
	if ( !timeLineTag.isNull() ) {
		QDomNode newTAGNode = timeLineTag.firstChildElement( "newTAG" );
		while( !newTAGNode.isNull() ) {
			pTimeline->addTag( LocalFileMng::readXmlInt( newTAGNode, "BAR", 0 ),
							   LocalFileMng::readXmlString( newTAGNode, "TAG", "" ) );
			newTAGNode = newTAGNode.nextSiblingElement( "newTAG" );
		}
	} else {
		WARNINGLOG( "TagTimeLine node not found" );
	}

	// Automation Paths
	QDomNode automationPathsNode = songNode.firstChildElement( "automationPaths" );
	if ( !automationPathsNode.isNull() ) {
		AutomationPathSerializer pathSerializer;

		QDomElement pathNode = automationPathsNode.firstChildElement( "path" );
		while( !pathNode.isNull()) {
			QString sAdjust = pathNode.attribute( "adjust" );

			// Select automation path to be read based on "adjust" attribute
			AutomationPath *pPath = nullptr;
			if (sAdjust == "velocity") {
				pPath = pSong->get_velocity_automation_path();
			}

			if (pPath) {
				pathSerializer.read_automation_path( pathNode, *pPath );
			}

			pathNode = pathNode.nextSiblingElement( "path" );
		}
	}

	pSong->set_is_modified( false );
	pSong->set_filename( FileName );

	return pSong;
}

Pattern* SongReader::getPattern( QDomNode pattern, InstrumentList* instrList )
{
	Pattern* pPattern = nullptr;

	QString sName;	// name
	sName = LocalFileMng::readXmlString( pattern, "name", sName );
	QString sInfo;
	sInfo = LocalFileMng::readXmlString( pattern, "info", sInfo,false,false );
	QString sCategory; // category
	sCategory = LocalFileMng::readXmlString( pattern, "category", sCategory,false,false );

	int nSize = -1;
	nSize = LocalFileMng::readXmlInt( pattern, "size", nSize, false, false );

	pPattern = new Pattern( sName, sInfo, sCategory, nSize );

	QDomNode pNoteListNode = pattern.firstChildElement( "noteList" );
	if ( ! pNoteListNode.isNull() ) {
		// new code :)
		QDomNode noteNode = pNoteListNode.firstChildElement( "note" );
		while ( ! noteNode.isNull()  ) {

			Note* pNote = nullptr;

			unsigned nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
			float fLeadLag = LocalFileMng::readXmlFloat( noteNode, "leadlag", 0.0, false, false );
			float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );
			float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
			float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );
			int nLength = LocalFileMng::readXmlInt( noteNode, "length", -1, true );
			float nPitch = LocalFileMng::readXmlFloat( noteNode, "pitch", 0.0, false, false );
			float fProbability = LocalFileMng::readXmlFloat( noteNode, "probability", 1.0, false, false );
			QString sKey = LocalFileMng::readXmlString( noteNode, "key", "C0", false, false );
			QString nNoteOff = LocalFileMng::readXmlString( noteNode, "note_off", "false", false, false );

			int instrId = LocalFileMng::readXmlInt( noteNode, "instrument", -1 );

			Instrument* pInstrumentRef = nullptr;
			// search instrument by ref
			pInstrumentRef = instrList->find( instrId );
			if ( !pInstrumentRef ) {
				ERRORLOG( QString( "Instrument with ID: '%1' not found. Note skipped." ).arg( instrId ) );
				noteNode = ( QDomNode ) noteNode.nextSiblingElement( "note" );
				continue;
			}
			//assert( instrRef );
			bool noteoff = false;
			if ( nNoteOff == "true" ) {
				noteoff = true;
			}

			pNote = new Note( pInstrumentRef, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch );
			pNote->set_key_octave( sKey );
			pNote->set_lead_lag( fLeadLag );
			pNote->set_note_off( noteoff );
			pNote->set_probability( fProbability );
			pPattern->insert_note( pNote );

			noteNode = ( QDomNode ) noteNode.nextSiblingElement( "note" );
		}
	} else {
		// Back compatibility code. Version < 0.9.4
		QDomNode sequenceListNode = pattern.firstChildElement( "sequenceList" );

		int sequence_count = 0;
		QDomNode sequenceNode = sequenceListNode.firstChildElement( "sequence" );
		while ( ! sequenceNode.isNull()  ) {
			sequence_count++;

			QDomNode noteListNode = sequenceNode.firstChildElement( "noteList" );
			QDomNode noteNode = noteListNode.firstChildElement( "note" );
			while (  !noteNode.isNull() ) {

				Note* pNote = nullptr;

				unsigned nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
				float fLeadLag = LocalFileMng::readXmlFloat( noteNode, "leadlag", 0.0, false, false );
				float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );
				float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
				float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );
				int nLength = LocalFileMng::readXmlInt( noteNode, "length", -1, true );
				float nPitch = LocalFileMng::readXmlFloat( noteNode, "pitch", 0.0, false, false );

				int instrId = LocalFileMng::readXmlInt( noteNode, "instrument", -1 );

				Instrument* instrRef = instrList->find( instrId );
				assert( instrRef );

				pNote = new Note( instrRef, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch );
				pNote->set_lead_lag( fLeadLag );

				//infoLog( "new note!! pos: " + toString( pNote->m_nPosition ) + "\t instr: " + instrId );
				pPattern->insert_note( pNote );

				noteNode = ( QDomNode ) noteNode.nextSiblingElement( "note" );
			}
			sequenceNode = ( QDomNode ) sequenceNode.nextSiblingElement( "sequence" );
		}
	}

	return pPattern;
}
};
