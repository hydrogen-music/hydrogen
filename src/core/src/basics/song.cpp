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

#include "hydrogen/version.h"

#include <cassert>


#include <hydrogen/LocalFileMng.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/fx/Effects.h>
#include <hydrogen/globals.h>
#include <hydrogen/timeline.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/automation_path.h>
#include <hydrogen/automation_path_serializer.h>
#include <hydrogen/helpers/xml.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/hydrogen.h>

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
	, __pattern_list( NULL )
	, __pattern_group_sequence( NULL )
	, __instrument_list( NULL )
	, __filename( "" )
	, __is_loop_enabled( false )
	, __humanize_time_value( 0.0 )
	, __humanize_velocity_value( 0.0 )
	, __swing_factor( 0.0 )
	, __song_mode( PATTERN_MODE )
	, __components( NULL )
	, __playback_track_enabled( false )
	, __playback_track_volume( 0.0 )
	, __velocity_automation_path( NULL )
	, __fill_value(0.5)
	, __fill_randomize(1)
{
	INFOLOG( QString( "INIT '%1'" ).arg( __name ) );

	__components = new std::vector<DrumkitComponent*> ();
	__velocity_automation_path = new AutomationPath(0.0f, 1.5f,  1.0f);
}



Song::~Song()
{
	// delete all patterns
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

	Song* song = new Song( "empty", "hydrogen", 120, 0.5 );

	song->set_metronome_volume( 0.5 );
	song->set_notes( "..." );
	song->set_license( "" );
	song->set_loop_enabled( false );
	song->set_mode( Song::PATTERN_MODE );
	song->set_humanize_time_value( 0.0 );
	song->set_humanize_velocity_value( 0.0 );
	song->set_swing_factor( 0.0 );

	InstrumentList* pList = new InstrumentList();
	Instrument* pNewInstr = new Instrument( EMPTY_INSTR_ID, "New instrument" );
	pList->add( pNewInstr );
	song->set_instrument_list( pList );

#ifdef H2CORE_HAVE_JACK
	Hydrogen::get_instance()->renameJackPorts( song );
#endif

	PatternList* patternList = new PatternList();
	Pattern* emptyPattern = new Pattern();
	emptyPattern->set_name( QString( "Pattern 1" ) );
	emptyPattern->set_category( QString( "not_categorized" ) );
	patternList->add( emptyPattern );
	song->set_pattern_list( patternList );
	std::vector<PatternList*>* pPatternGroupVector = new std::vector<PatternList*>;
	PatternList* patternSequence = new PatternList();
	patternSequence->add( emptyPattern );
	pPatternGroupVector->push_back( patternSequence );
	song->set_pattern_group_vector( pPatternGroupVector );
	song->set_is_modified( false );
	song->set_filename( "empty_song" );

	return song;

}

/// Return an empty song
Song* Song::get_empty_song()
{

	Song* song = Song::load( Filesystem::empty_song_path() );

	/* if file DefaultSong.h2song not accessible
	 * create a simple default song.
	 */
	if( !song ) {
		song = Song::get_default_song();
	}

	return song;
}

DrumkitComponent* Song::get_component( int ID )
{
	for (std::vector<DrumkitComponent*>::iterator it = __components->begin() ; it != __components->end(); ++it) {
		if( (*it)->get_id() == ID )
			return *it;
	}

	return NULL;
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
			QString patternName = virtualNode.read_attribute( "pattern", NULL, false, false );
			XMLNode patternNode = virtualNode.firstChildElement( "pattern" );
			Pattern* p = NULL;
			while ( !patternName.isEmpty() && !patternNode.isNull() ) {
				QString virtualName = patternNode.read_text( false );
				if ( !virtualName.isEmpty() ) {
					Pattern* v = NULL;
					for ( unsigned i = 0; i < get_pattern_list()->size(); i++ ) {
						Pattern* pat = get_pattern_list()->get( i );
						if ( p == NULL && pat->get_name() == patternName ) {
							p = pat;
						}
						if ( v == NULL && pat->get_name() == virtualName ) {
							v = pat;
						}
						if ( p != NULL && v != NULL) {
							break;
						}
					}
					if ( p == NULL ) {
						ERRORLOG( QString( "Invalid pattern name %1" ).arg( patternName ) );
					}
					if ( v == NULL ) {
						ERRORLOG( QString( "Invalid virtual pattern name %1" ).arg( virtualName ) );
					}
					if ( p != NULL && v != NULL ) {
						p->virtual_patterns_add( v );
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
					Pattern* p = NULL;
					for ( unsigned i = 0; i < get_pattern_list()->size(); i++ ) {
						Pattern* pat = get_pattern_list()->get( i );
						if ( pat->get_name() == patternName ) {
							p = pat;
							break;
						}
					}
					if ( p == NULL ) {
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
		Pattern *pat = get_pattern_list()->get( i );
		if ( !pat->get_virtual_patterns()->empty() ) {
			XMLNode node = virtualPatternListNode.createNode( "virtual" );
			node.write_attribute( "pattern", pat->get_name() );
			for ( Pattern::virtual_patterns_it_t virtIter = pat->get_virtual_patterns()->begin(); virtIter != pat->get_virtual_patterns()->end(); ++virtIter ) {
				node.write_string( "pattern", (*virtIter)->get_name() );
			}
		}
	}

	XMLNode patternSequenceNode = root.createNode( "groups" );
	for ( unsigned i = 0; i < get_pattern_group_vector()->size(); i++ ) {
		XMLNode node = patternSequenceNode.createNode( "group" );
		PatternList *pList = ( *get_pattern_group_vector() )[i];
		for ( unsigned j = 0; j < pList->size(); j++ ) {
			Pattern *pat = pList->get( j );
			node.write_string( "pattern", pat->get_name() );
		}
	}

	return doc.write( filename );
}

QString Song::copyInstrumentLineToString( int selectedPattern, int selectedInstrument )
{
	Instrument *instr = get_instrument_list()->get( selectedInstrument );
	assert( instr );

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
		if ((selectedPattern >= 0) && (selectedPattern != i))
			continue;

		// Export pattern
		Pattern *pat = get_pattern_list()->get( i );

		QDomNode patternNode = doc.createElement( "pattern" );
		LocalFileMng::writeXmlString( patternNode, "pattern_name", pat->get_name() );

		QString category;
		if ( pat->get_category().isEmpty() )
			category = "No category";
		else
			category = pat->get_category();

		LocalFileMng::writeXmlString( patternNode, "info", pat->get_info() );
		LocalFileMng::writeXmlString( patternNode, "category", category  );
		LocalFileMng::writeXmlString( patternNode, "size", QString("%1").arg( pat->get_length() ) );

		QDomNode noteListNode = doc.createElement( "noteList" );
		const Pattern::notes_t* notes = pat->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it)
		{
			Note *pNote = it->second;
			assert( pNote );

			// Export only specified instrument
			if (pNote->get_instrument() == instr)
			{
				QDomNode noteNode = doc.createElement( "note" );
				LocalFileMng::writeXmlString( noteNode, "position", QString("%1").arg( pNote->get_position() ) );
				LocalFileMng::writeXmlString( noteNode, "leadlag", QString("%1").arg( pNote->get_lead_lag() ) );
				LocalFileMng::writeXmlString( noteNode, "velocity", QString("%1").arg( pNote->get_velocity() ) );
				LocalFileMng::writeXmlString( noteNode, "pan_L", QString("%1").arg( pNote->get_pan_l() ) );
				LocalFileMng::writeXmlString( noteNode, "pan_R", QString("%1").arg( pNote->get_pan_r() ) );
				LocalFileMng::writeXmlString( noteNode, "pitch", QString("%1").arg( pNote->get_pitch() ) );
				LocalFileMng::writeXmlString( noteNode, "probability", QString("%1").arg( pNote->get_probability() ) );

				LocalFileMng::writeXmlString( noteNode, "key", pNote->key_to_string() );

				LocalFileMng::writeXmlString( noteNode, "length", QString("%1").arg( pNote->get_length() ) );
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
	if (!doc.setContent(serialized))
		return false;

	// Get current instrument
	Instrument *instr = get_instrument_list()->get( selectedInstrument );
	assert( instr );

	// Get pattern list
	PatternList *pList = get_pattern_list();
	Pattern *pSelected = (selectedPattern >= 0) ? pList->get(selectedPattern) : NULL;

	// Check if document has correct structure
	QDomNode rootNode = doc.firstChildElement( "instrument_line" );	// root element

	if ( rootNode.isNull() )
	{
		ERRORLOG( "Error pasting Clipboard:Instrument_line_info node not found ");
		return false;
	}

	// Find pattern list
	QDomNode patternList = rootNode.firstChildElement( "patternList" );
	if (patternList.isNull())
		return false;

	// Parse each pattern if needed
	QDomNode patternNode = patternList.firstChildElement( "pattern" );
	bool is_single = true;
	if (!patternNode.isNull())
		is_single = (( QDomNode )patternNode.nextSiblingElement( "pattern" )).isNull();

	while (!patternNode.isNull())
	{
		QString patternName(LocalFileMng::readXmlString(patternNode, "pattern_name", ""));

		// Check if pattern name specified
		if (patternName.length() > 0)
		{
			// Try to find pattern by name
			Pattern* pat = pList->find(patternName);

			// If OK - check if need to add this pattern to result
			// If there is only one pattern, we always add it to list
			// If there is no selected pattern, we add all existing patterns to list (match by name)
			// Otherwise we add only existing selected pattern to list (match by name)
			if ((is_single) || ((pat != NULL) && ((selectedPattern < 0) || (pat == pSelected))))
			{
				// Load additional pattern info & create pattern
				QString sInfo;
				sInfo = LocalFileMng::readXmlString(patternNode, "info", sInfo, false, false);
				QString sCategory;
				sCategory = LocalFileMng::readXmlString(patternNode, "category", sCategory, false, false);
				int nSize = -1;
				nSize = LocalFileMng::readXmlInt(patternNode, "size", nSize, false, false);

				// Change name of pattern to selected pattern
				if (pSelected != NULL)
					patternName = pSelected->get_name();

				pat = new Pattern( patternName, sInfo, sCategory, nSize );

				// Parse pattern data
				QDomNode pNoteListNode = patternNode.firstChildElement( "noteList" );
				if ( ! pNoteListNode.isNull() )
				{
					// Parse note-by-note
					QDomNode noteNode = pNoteListNode.firstChildElement( "note" );
					while ( ! noteNode.isNull() )
					{
						Note* pNote = NULL;

						unsigned nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
						float fLeadLag = LocalFileMng::readXmlFloat( noteNode, "leadlag", 0.0 , false , false );
						float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );
						float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
						float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );
						int nLength = LocalFileMng::readXmlInt( noteNode, "length", -1, true );
						float nPitch = LocalFileMng::readXmlFloat( noteNode, "pitch", 0.0, false, false );
						float fProbability = LocalFileMng::readXmlFloat( noteNode, "probability", 1.0 , false , false );
						QString sKey = LocalFileMng::readXmlString( noteNode, "key", "C0", false, false );
						QString nNoteOff = LocalFileMng::readXmlString( noteNode, "note_off", "false", false, false );

						bool noteoff = ( nNoteOff == "true" );

						pNote = new Note( instr, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch );
						pNote->set_key_octave( sKey );
						pNote->set_lead_lag( fLeadLag );
						pNote->set_note_off( noteoff );
						pNote->set_probability( fProbability );
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
	if ( QFile( filename ).exists() )
		return QFileInfo ( filename ).absoluteFilePath();

	/* Try search in Session Directory */
	char* sesdir = getenv ( "SESSION_DIR" );
	if ( sesdir ) {
		INFOLOG ( "Try SessionDirectory " + QString( sesdir ) );
		QDir SesDir( sesdir );
		QString BaseFileName = QFileInfo( filename ).fileName();
		QString SesFileName = SesDir.filePath( BaseFileName );
		if ( QFile( SesFileName ).exists() )
			return QFileInfo( SesFileName ).absoluteFilePath();
	}

	ERRORLOG( "Song file " + filename + " not found." );
	return NULL;
}

///
/// Reads a song.
/// return NULL = error reading song file.
///
Song* SongReader::readSong( const QString& filename )
{
	QString FileName = getPath ( filename );
	if ( FileName.isEmpty() ) return NULL;

	INFOLOG( "Reading " + FileName );
	Song* song = NULL;

	QDomDocument doc = LocalFileMng::openXmlDocument( FileName );
	QDomNodeList nodeList = doc.elementsByTagName( "song" );

	if( nodeList.isEmpty() ) {
		ERRORLOG( "Error reading song: song node not found" );
		return NULL;
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

	song = new Song( sName, sAuthor, fBpm, fVolume );
	song->set_metronome_volume( fMetronomeVolume );
	song->set_notes( sNotes );
	song->set_license( sLicense );
	song->set_loop_enabled( bLoopEnabled );
	song->set_mode( nMode );
	song->set_humanize_time_value( fHumanizeTimeValue );
	song->set_humanize_velocity_value( fHumanizeVelocityValue );
	song->set_swing_factor( fSwingFactor );
	song->set_playback_track_filename( sPlaybackTrack );
	song->set_playback_track_enabled( bPlaybackTrackEnabled );
	song->set_playback_track_volume( fPlaybackTrackVolume );

	QDomNode componentListNode = songNode.firstChildElement( "componentList" );
	if ( ( ! componentListNode.isNull()  ) ) {
		QDomNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
		while ( ! componentNode.isNull()  ) {
			int id = LocalFileMng::readXmlInt( componentNode, "id", -1 );			// instrument id
			QString sName = LocalFileMng::readXmlString( componentNode, "name", "" );		// name
			float fVolume = LocalFileMng::readXmlFloat( componentNode, "volume", 1.0 );	// volume
			DrumkitComponent* pDrumkitComponent = new DrumkitComponent( id, sName );
			pDrumkitComponent->set_volume( fVolume );

			song->get_components()->push_back(pDrumkitComponent);

			componentNode = ( QDomNode ) componentNode.nextSiblingElement( "drumkitComponent" );
		}
	} else {
		DrumkitComponent* pDrumkitComponent = new DrumkitComponent( 0, "Main" );
		song->get_components()->push_back(pDrumkitComponent);
	}

	//  Instrument List
	InstrumentList* instrumentList = new InstrumentList();

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
			if ( sRead_sample_select_algo.compare("VELOCITY") == 0 )
				pInstrument->set_sample_selection_alg( Instrument::VELOCITY );
			else if ( sRead_sample_select_algo.compare("ROUND_ROBIN") == 0 )
				pInstrument->set_sample_selection_alg( Instrument::ROUND_ROBIN );
			else if ( sRead_sample_select_algo.compare("RANDOM") == 0 )
				pInstrument->set_sample_selection_alg( Instrument::RANDOM );
			pInstrument->set_midi_out_channel( nMidiOutChannel );
			pInstrument->set_midi_out_note( nMidiOutNote );

			QString drumkitPath;
			if ( ( !sDrumkit.isEmpty() ) && ( sDrumkit != "-" ) ) {
				drumkitPath = Filesystem::drumkit_path_search( sDrumkit );
			}


			QDomNode filenameNode = instrumentNode.firstChildElement( "filename" );

			// back compatibility code ( song version <= 0.9.0 )
			if ( ! filenameNode.isNull() ) {
				WARNINGLOG( "Using back compatibility code. filename node found" );
				QString sFilename = LocalFileMng::readXmlString( instrumentNode, "filename", "" );

				if ( !QFile( sFilename ).exists() && !drumkitPath.isEmpty() ) {
					sFilename = drumkitPath + "/" + sFilename;
				}
				Sample* pSample = Sample::load( sFilename );
				if ( pSample == NULL ) {
					// nel passaggio tra 0.8.2 e 0.9.0 il drumkit di default e' cambiato.
					// Se fallisce provo a caricare il corrispettivo file in formato flac
//					warningLog( "[readSong] Error loading sample: " + sFilename + " not found. Trying to load a flac..." );
					sFilename = sFilename.left( sFilename.length() - 4 );
					sFilename += ".flac";
					pSample = Sample::load( sFilename );
				}
				if ( pSample == NULL ) {
					ERRORLOG( "Error loading sample: " + sFilename + " not found" );
					pInstrument->set_muted( true );
				}
				InstrumentComponent* pCompo = new InstrumentComponent ( 0 );
				InstrumentLayer* pLayer = new InstrumentLayer( pSample );
				pCompo->set_layer( pLayer, 0 );
				pInstrument->get_components()->push_back( pCompo );
			}
			//~ back compatibility code
			else {
				bool p_foundAtLeastOneComponent = false;
				QDomNode componentNode = instrumentNode.firstChildElement( "instrumentComponent" );
				while (  ! componentNode.isNull()  ) {
					p_foundAtLeastOneComponent = true;
					int id = LocalFileMng::readXmlInt( componentNode, "component_id", 0 );
					InstrumentComponent* pCompo = new InstrumentComponent( id );
					float fGainCompo = LocalFileMng::readXmlFloat( componentNode, "gain", 1.0 );
					pCompo->set_gain( fGainCompo );

					unsigned nLayer = 0;
					QDomNode layerNode = componentNode.firstChildElement( "layer" );
					while (  ! layerNode.isNull()  ) {
						if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
							ERRORLOG( QString( "nLayer (%1) > maxLayers (%2)" ).arg ( nLayer ).arg( InstrumentComponent::getMaxLayers() ) );
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

						Sample* pSample = NULL;
						if ( !sIsModified ) {
							pSample = Sample::load( sFilename );
						} else {
							Sample::EnvelopePoint pt;

							Sample::VelocityEnvelope velocity;
							QDomNode volumeNode = layerNode.firstChildElement( "volume" );
							while (  ! volumeNode.isNull()  ) {
								pt.frame = LocalFileMng::readXmlInt( volumeNode, "volume-position", 0 );
								pt.value = LocalFileMng::readXmlInt( volumeNode, "volume-value", 0 );
								velocity.push_back( pt );
								volumeNode = volumeNode.nextSiblingElement( "volume" );
								//ERRORLOG( QString("volume-posi %1").arg(LocalFileMng::readXmlInt( volumeNode, "volume-position", 0)) );
							}

							Sample::VelocityEnvelope pan;
							QDomNode  panNode = layerNode.firstChildElement( "pan" );
							while (  ! panNode.isNull()  ) {
								pt.frame = LocalFileMng::readXmlInt( panNode, "pan-position", 0 );
								pt.value = LocalFileMng::readXmlInt( panNode, "pan-value", 0 );
								pan.push_back( pt );
								panNode = panNode.nextSiblingElement( "pan" );
							}

							pSample = Sample::load( sFilename, lo, ro, velocity, pan );
						}
						if ( pSample == NULL ) {
							ERRORLOG( "Error loading sample: " + sFilename + " not found" );
							pInstrument->set_muted( true );
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
				if(!p_foundAtLeastOneComponent) {
					InstrumentComponent* pCompo = new InstrumentComponent( 0 );
					float fGainCompo = LocalFileMng::readXmlFloat( componentNode, "gain", 1.0 );
					pCompo->set_gain( fGainCompo );

					unsigned nLayer = 0;
					QDomNode layerNode = instrumentNode.firstChildElement( "layer" );
					while (  ! layerNode.isNull()  ) {
						if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
							ERRORLOG( QString( "nLayer (%1) > maxLayers (%2)" ).arg ( nLayer ).arg( InstrumentComponent::getMaxLayers() ) );
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

						Sample* pSample = NULL;
						if ( !sIsModified ) {
							pSample = Sample::load( sFilename );
						} else {
							Sample::EnvelopePoint pt;

							Sample::VelocityEnvelope velocity;
							QDomNode volumeNode = layerNode.firstChildElement( "volume" );
							while (  ! volumeNode.isNull()  ) {
								pt.frame = LocalFileMng::readXmlInt( volumeNode, "volume-position", 0 );
								pt.value = LocalFileMng::readXmlInt( volumeNode, "volume-value", 0 );
								velocity.push_back( pt );
								volumeNode = volumeNode.nextSiblingElement( "volume" );
								//ERRORLOG( QString("volume-posi %1").arg(LocalFileMng::readXmlInt( volumeNode, "volume-position", 0)) );
							}

							Sample::VelocityEnvelope pan;
							QDomNode  panNode = layerNode.firstChildElement( "pan" );
							while (  ! panNode.isNull()  ) {
								pt.frame = LocalFileMng::readXmlInt( panNode, "pan-position", 0 );
								pt.value = LocalFileMng::readXmlInt( panNode, "pan-value", 0 );
								pan.push_back( pt );
								panNode = panNode.nextSiblingElement( "pan" );
							}

							pSample = Sample::load( sFilename, lo, ro, velocity, pan );
						}
						if ( pSample == NULL ) {
							ERRORLOG( "Error loading sample: " + sFilename + " not found" );
							pInstrument->set_muted( true );
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
			instrumentList->add( pInstrument );
			instrumentNode = ( QDomNode ) instrumentNode.nextSiblingElement( "instrument" );
		}

		if ( instrumentList_count == 0 ) {
			WARNINGLOG( "0 instruments?" );
		}

		song->set_instrument_list( instrumentList );
	} else {
		ERRORLOG( "Error reading song: instrumentList node not found" );
		delete song;
		return NULL;
	}

	// Pattern list
	QDomNode patterns = songNode.firstChildElement( "patternList" );

	PatternList* patternList = new PatternList();
	int pattern_count = 0;

	QDomNode patternNode =  patterns.firstChildElement( "pattern" );
	while (  !patternNode.isNull()  ) {
		pattern_count++;
		Pattern* pat = getPattern( patternNode, instrumentList );
		if ( pat ) {
			patternList->add( pat );
		} else {
			ERRORLOG( "Error loading pattern" );
			delete patternList;
			delete song;
			return NULL;
		}
		patternNode = ( QDomNode ) patternNode.nextSiblingElement( "pattern" );
	}
	if ( pattern_count == 0 ) {
		WARNINGLOG( "0 patterns?" );
	}
	song->set_pattern_list( patternList );

	// Virtual Patterns
	QDomNode  virtualPatternListNode = songNode.firstChildElement( "virtualPatternList" );
	QDomNode virtualPatternNode = virtualPatternListNode.firstChildElement( "pattern" );
	if ( !virtualPatternNode.isNull() ) {

		while (  ! virtualPatternNode.isNull()  ) {
			QString sName = "";
			sName = LocalFileMng::readXmlString( virtualPatternNode, "name", sName );

			Pattern* curPattern = NULL;
			unsigned nPatterns = patternList->size();
			for ( unsigned i = 0; i < nPatterns; i++ ) {
				Pattern* pat = patternList->get( i );

				if ( pat->get_name() == sName ) {
					curPattern = pat;
					break;
				}//if
			}//for

			if ( curPattern != NULL ) {
				QDomNode  virtualNode = virtualPatternNode.firstChildElement( "virtual" );
				while (  !virtualNode.isNull()  ) {
					QString virtName = virtualNode.firstChild().nodeValue();

					Pattern* virtPattern = NULL;
					for ( unsigned i = 0; i < nPatterns; i++ ) {
						Pattern* pat = patternList->get( i );

						if ( pat->get_name() == virtName ) {
							virtPattern = pat;
							break;
						}//if
					}//for

					if ( virtPattern != NULL ) {
						curPattern->virtual_patterns_add( virtPattern );
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

	patternList->flattened_virtual_patterns_compute();

	// Pattern sequence
	QDomNode patternSequenceNode = songNode.firstChildElement( "patternSequence" );

	std::vector<PatternList*>* pPatternGroupVector = new std::vector<PatternList*>;

	// back-compatibility code..
	QDomNode pPatternIDNode = patternSequenceNode.firstChildElement( "patternID" );
	while ( ! pPatternIDNode.isNull()  ) {
		WARNINGLOG( "Using old patternSequence code for back compatibility" );
		PatternList* patternSequence = new PatternList();
		QString patId = pPatternIDNode.firstChildElement().text();
		ERRORLOG( patId );

		Pattern* pat = NULL;
		for ( unsigned i = 0; i < patternList->size(); i++ ) {
			Pattern* tmp = patternList->get( i );
			if ( tmp ) {
				if ( tmp->get_name() == patId ) {
					pat = tmp;
					break;
				}
			}
		}
		if ( pat == NULL ) {
			WARNINGLOG( "patternid not found in patternSequence" );
			pPatternIDNode = ( QDomNode ) pPatternIDNode.nextSiblingElement( "patternID" );
			continue;
		}
		patternSequence->add( pat );

		pPatternGroupVector->push_back( patternSequence );

		pPatternIDNode = ( QDomNode ) pPatternIDNode.nextSiblingElement( "patternID" );
	}

	QDomNode groupNode = patternSequenceNode.firstChildElement( "group" );
	while (  !groupNode.isNull()  ) {
		PatternList* patternSequence = new PatternList();
		QDomNode patternId = groupNode.firstChildElement( "patternID" );
		while (  !patternId.isNull()  ) {
			QString patId = patternId.firstChild().nodeValue();

			Pattern* pat = NULL;
			for ( unsigned i = 0; i < patternList->size(); i++ ) {
				Pattern* tmp = patternList->get( i );
				if ( tmp ) {
					if ( tmp->get_name() == patId ) {
						pat = tmp;
						break;
					}
				}
			}
			if ( pat == NULL ) {
				WARNINGLOG( "patternid not found in patternSequence" );
				patternId = ( QDomNode ) patternId.nextSiblingElement( "patternID" );
				continue;
			}
			patternSequence->add( pat );
			patternId = ( QDomNode ) patternId.nextSiblingElement( "patternID" );
		}
		pPatternGroupVector->push_back( patternSequence );

		groupNode = groupNode.nextSiblingElement( "group" );
	}

	song->set_pattern_group_vector( pPatternGroupVector );

#ifdef H2CORE_HAVE_LADSPA
	// reset FX
	for ( int fx = 0; fx < MAX_FX; ++fx ) {
		//LadspaFX* pFX = Effects::get_instance()->getLadspaFX( fx );
		//delete pFX;
		Effects::get_instance()->setLadspaFX( NULL, fx );
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
	pTimeline->m_timelinevector.clear();
	Timeline::HTimelineVector tlvector;
	QDomNode bpmTimeLine = songNode.firstChildElement( "BPMTimeLine" );
	if ( !bpmTimeLine.isNull() ) {
		QDomNode newBPMNode = bpmTimeLine.firstChildElement( "newBPM" );
		while( !newBPMNode.isNull() ) {
			tlvector.m_htimelinebeat = LocalFileMng::readXmlInt( newBPMNode, "BAR", 0 );
			tlvector.m_htimelinebpm = LocalFileMng::readXmlFloat( newBPMNode, "BPM", 120.0 );
			pTimeline->m_timelinevector.push_back( tlvector );
			pTimeline->sortTimelineVector();
			newBPMNode = newBPMNode.nextSiblingElement( "newBPM" );
		}
	} else {
		WARNINGLOG( "bpmTimeLine node not found" );
	}

	pTimeline->m_timelinetagvector.clear();
	Timeline::HTimelineTagVector tltagvector;
	QDomNode timeLineTag = songNode.firstChildElement( "timeLineTag" );
	if ( !timeLineTag.isNull() ) {
		QDomNode newTAGNode = timeLineTag.firstChildElement( "newTAG" );
		while( !newTAGNode.isNull() ) {
			tltagvector.m_htimelinetagbeat = LocalFileMng::readXmlInt( newTAGNode, "BAR", 0 );
			tltagvector.m_htimelinetag = LocalFileMng::readXmlString( newTAGNode, "TAG", "" );
			pTimeline->m_timelinetagvector.push_back( tltagvector );
			pTimeline->sortTimelineTagVector();
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
			AutomationPath *pPath = NULL;
			if (sAdjust == "velocity") {
				pPath = song->get_velocity_automation_path();
			}

			if (pPath) {
				pathSerializer.read_automation_path( pathNode, *pPath );
			}

			pathNode = pathNode.nextSiblingElement( "path" );
		}
	}

	song->set_is_modified( false );
	song->set_filename( FileName );

	return song;
}

Pattern* SongReader::getPattern( QDomNode pattern, InstrumentList* instrList )
{
	Pattern* pPattern = NULL;

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

			Note* pNote = NULL;

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

			Instrument* instrRef = NULL;
			// search instrument by ref
			instrRef = instrList->find( instrId );
			if ( !instrRef ) {
				ERRORLOG( QString( "Instrument with ID: '%1' not found. Note skipped." ).arg( instrId ) );
				noteNode = ( QDomNode ) noteNode.nextSiblingElement( "note" );
				continue;
			}
			//assert( instrRef );
			bool noteoff = false;
			if ( nNoteOff == "true" )
				noteoff = true;

			pNote = new Note( instrRef, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch );
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

				Note* pNote = NULL;

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
