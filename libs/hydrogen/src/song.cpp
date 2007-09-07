/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#include <cassert>

#include "xml/tinyxml.h"

#include <hydrogen/adsr.h>
#include <hydrogen/data_path.h>
#include <hydrogen/LocalFileMng.h>

#include <hydrogen/fx/Effects.h>
#include <hydrogen/globals.h>
#include <hydrogen/Song.h>
#include <hydrogen/Sample.h>
#include <hydrogen/instrument.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/note.h>

namespace H2Core {

Song::Song( const std::string& sName, const std::string& sAuthor, float fBpm, float fVolume)
 : Object( "Song" )
 , m_bIsMuted( false )
 , m_nResolution( 48 )
 , m_fBPM( fBpm )
 , m_bIsModified( false )
 , m_sName( sName )
 , m_sAuthor( sAuthor )
 , m_fVolume( fVolume )
 , m_fMetronomeVolume( 0.5 )
 , m_sNotes( "Song info" )	///\todo: attenzione..questo non verra' tradotto
 , m_pPatternList( NULL )
 , m_pPatternSequence( NULL )
 , m_pInstrumentList( NULL )
 , m_sFilename( "" )
 , m_bIsLoopEnabled( false )
 , m_fHumanizeTimeValue( 0.0 )
 , m_fHumanizeVelocityValue( 0.0 )
 , m_fSwingFactor( 0.0 )
 , m_songMode( PATTERN_MODE )
{
	INFOLOG( "INIT \"" + m_sName + "\"" );

	//m_bDelayFXEnabled = false;
	//m_fDelayFXWetLevel = 0.8;
	//m_fDelayFXFeedback = 0.5;
	//m_nDelayFXTime = MAX_NOTES / 8;
}



Song::~Song()
{
	// delete all patterns
	delete m_pPatternList;

	if (m_pPatternSequence) {
		for (unsigned i = 0; i < m_pPatternSequence->size(); ++i) {
			PatternList *pPatternList = (*m_pPatternSequence)[i];
			pPatternList->clear();	// pulisco tutto, i pattern non vanno distrutti qua
			delete pPatternList;
		}
		delete m_pPatternSequence;
	}

	delete m_pInstrumentList;

	INFOLOG( "DESTROY \"" + m_sName + "\"" );
}



///Load a song from file
Song* Song::load( const std::string& filename )
{
	Song *song = NULL;

	SongReader reader;
	song = reader.readSong( filename );

	return song;
}




/// Save a song to file
void Song::save( const std::string& filename )
{
//	LocalFileMng mng;
//	mng.saveSong(this, filename);

	SongWriter writer;
	writer.writeSong( this, filename);
}



/// Return an empty song
Song* Song::getEmptySong() {
	string dataDir = DataPath::get_data_path();
	string filename = dataDir + "/DefaultSong.h2song";
	Song *song = Song::load( filename );

	return song;
}





void Song::setSwingFactor( float factor ) {
	if (factor < 0.0) {
		factor = 0.0;
	}
	else if (factor > 1.0) {
		factor = 1.0;
	}

	m_fSwingFactor = factor;
}



//::::::::::::::::::::







//-----------------------------------------------------------------------------
//	Implementation of SongReader class
//-----------------------------------------------------------------------------


SongReader::SongReader()
 : Object( "SongReader" )
{
//	infoLog("init");
}



SongReader::~SongReader() {
//	infoLog("destroy");
}



///
/// Reads a song.
/// return NULL = error reading song file.
///
Song* SongReader::readSong( const std::string& filename)
{
	INFOLOG( filename );
	Song* song = NULL;

	std::ifstream verify(filename.c_str() , std::ios::in | std::ios::binary);	// song file exists??
	if (verify == NULL) {
		ERRORLOG( "Song file " + filename + " not found.");
		return NULL;
	}


	TiXmlDocument doc(filename.c_str());
	doc.LoadFile();

	TiXmlNode* songNode;	// root element
	if ( !( songNode = doc.FirstChild("song") ) ) {
		ERRORLOG( "Error reading song: song node not found");
		return NULL;
	}


	m_sSongVersion = LocalFileMng::readXmlString( songNode, "version", "Unknown version" );
	if ( m_sSongVersion != VERSION ) {
		WARNINGLOG( "Trying to load a song created with a different version of hydrogen.");
		WARNINGLOG( "Song [" + filename + "] saved with version " + m_sSongVersion );
	}

	float fBpm = LocalFileMng::readXmlFloat( songNode, "bpm", 120 );
	float fVolume = LocalFileMng::readXmlFloat( songNode, "volume", 0.5 );
	float fMetronomeVolume = LocalFileMng::readXmlFloat( songNode, "metronomeVolume", 0.5 );
	string sName = LocalFileMng::readXmlString( songNode, "name", "Untitled Song" );
	string sAuthor = LocalFileMng::readXmlString( songNode, "author", "Unknown Author" );
	string sNotes = LocalFileMng::readXmlString( songNode, "notes", "..." );
	bool bLoopEnabled = LocalFileMng::readXmlBool( songNode, "loopEnabled", false );

	Song::SongMode nMode = Song::PATTERN_MODE;	// Mode (song/pattern)
	string sMode = LocalFileMng::readXmlString( songNode, "mode", "pattern" );
	if ( sMode == "song" ) {
		nMode = Song::SONG_MODE;
	}

	float fHumanizeTimeValue = LocalFileMng::readXmlFloat( songNode, "humanize_time", 0.0 );
	float fHumanizeVelocityValue = LocalFileMng::readXmlFloat( songNode, "humanize_velocity", 0.0 );
	float fSwingFactor = LocalFileMng::readXmlFloat( songNode, "swing_factor", 0.0 );

	song = new Song( sName, sAuthor, fBpm, fVolume );
	song->setMetronomeVolume( fMetronomeVolume );
	song->setNotes( sNotes );
	song->setLoopEnabled( bLoopEnabled );
	song->setMode( nMode );
	song->setHumanizeTimeValue( fHumanizeTimeValue );
	song->setHumanizeVelocityValue( fHumanizeVelocityValue );
	song->setSwingFactor( fSwingFactor );

	/*
	song->m_bDelayFXEnabled = LocalFileMng::readXmlBool( songNode, "delayFXEnabled", false, false );
	song->m_fDelayFXWetLevel = LocalFileMng::readXmlFloat( songNode, "delayFXWetLevel", 1.0, false, false );
	song->m_fDelayFXFeedback= LocalFileMng::readXmlFloat( songNode, "delayFXFeedback", 0.4, false, false );
	song->m_nDelayFXTime = LocalFileMng::readXmlInt( songNode, "delayFXTime", MAX_NOTES / 4, false, false );
	*/


	//  Instrument List
	LocalFileMng localFileMng;
	InstrumentList *instrumentList = new InstrumentList();

	TiXmlNode* instrumentListNode;
	if ( (instrumentListNode = songNode->FirstChild("instrumentList") ) ) {
		// INSTRUMENT NODE
		int instrumentList_count = 0;
		TiXmlNode* instrumentNode = 0;
		for( instrumentNode = instrumentListNode->FirstChild("instrument"); instrumentNode; instrumentNode = instrumentNode->NextSibling("instrument")) {
			instrumentList_count++;

			string sId = LocalFileMng::readXmlString( instrumentNode, "id", "" );			// instrument id
			string sDrumkit = LocalFileMng::readXmlString( instrumentNode, "drumkit", "" );	// drumkit
			string sName = LocalFileMng::readXmlString( instrumentNode, "name", "" );		// name
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
			int fRelease = LocalFileMng::readXmlInt( instrumentNode, "Release", 1000, false, false );	// Release

			float fRandomPitchFactor = LocalFileMng::readXmlFloat( instrumentNode, "randomPitchFactor", 0.0f, false, false );

			bool bFilterActive = LocalFileMng::readXmlBool( instrumentNode, "filterActive", false );
			float fFilterCutoff = LocalFileMng::readXmlFloat( instrumentNode, "filterCutoff", 1.0f, false );
			float fFilterResonance = LocalFileMng::readXmlFloat( instrumentNode, "filterResonance", 0.0f, false );
			string sMuteGroup = LocalFileMng::readXmlString( instrumentNode, "muteGroup", "-1", false );
			int nMuteGroup = atoi( sMuteGroup.c_str() );


			if ( sId == "" ) {
				ERRORLOG( "Empty ID for instrument '" + sName + "'. skipping." );
				continue;
			}


			// create a new instrument
			Instrument *pInstrument = new Instrument( sId, sName, new ADSR( fAttack, fDecay, fSustain, fRelease ));
			pInstrument->set_volume(fVolume);
			pInstrument->set_muted(bIsMuted);
			pInstrument->set_pan_l(fPan_L);
			pInstrument->set_pan_r(fPan_R);
			pInstrument->set_drumkit_name(sDrumkit );
			pInstrument->set_fx_level(fFX1Level, 0 );
			pInstrument->set_fx_level(fFX2Level, 1 );
			pInstrument->set_fx_level(fFX3Level, 2 );
			pInstrument->set_fx_level(fFX4Level, 3 );
			pInstrument->set_random_pitch_factor( fRandomPitchFactor);
			pInstrument->set_filter_active( bFilterActive );
			pInstrument->set_filter_cutoff( fFilterCutoff );
			pInstrument->set_filter_resonance( fFilterResonance );
			pInstrument->set_gain( fGain );
			pInstrument->set_mute_group( nMuteGroup );

			string drumkitPath = "";
			if ( ( sDrumkit != "" ) && (sDrumkit != "-" ) ) {
//				drumkitPath = localFileMng.getDrumkitDirectory( sDrumkit ) + sDrumkit + "/";
				drumkitPath = localFileMng.getDrumkitDirectory( sDrumkit ) + sDrumkit;
			}

			// back compatibility code ( song version <= 0.9.0 )
			TiXmlNode* filenameNode = instrumentNode->FirstChild( "filename" );
			if ( filenameNode ) {
				WARNINGLOG( "Using back compatibility code. filename node found" );
				string sFilename = LocalFileMng::readXmlString( instrumentNode, "filename", "" );

				if (drumkitPath != "") {
					sFilename = drumkitPath + "/" + sFilename;
				}
				Sample *pSample = Sample::load( sFilename );
				if (pSample == NULL) {
					// nel passaggio tra 0.8.2 e 0.9.0 il drumkit di default e' cambiato.
					// Se fallisce provo a caricare il corrispettivo file in formato flac
//					warningLog( "[readSong] Error loading sample: " + sFilename + " not found. Trying to load a flac..." );
					sFilename = sFilename.substr( 0, sFilename.length() - 4 );
					sFilename += ".flac";
					pSample = Sample::load( sFilename );
				}
				if (pSample == NULL) {
					ERRORLOG( "Error loading sample: " + sFilename + " not found" );
					pInstrument->set_muted( true );
				}
				InstrumentLayer *pLayer = new InstrumentLayer( pSample );
				pInstrument->set_layer( pLayer, 0 );
			}
			//~ back compatibility code
			else {
				unsigned nLayer = 0;
				for( TiXmlNode* layerNode = instrumentNode->FirstChild( "layer" ); layerNode; layerNode = layerNode->NextSibling( "layer" ) ) {
					if (nLayer >= MAX_LAYERS) {
						ERRORLOG( "nLayer > MAX_LAYERS" );
						continue;
					}
					string sFilename = LocalFileMng::readXmlString( layerNode, "filename", "" );
					float fMin = LocalFileMng::readXmlFloat( layerNode, "min", 0.0 );
					float fMax = LocalFileMng::readXmlFloat( layerNode, "max", 1.0 );
					float fGain = LocalFileMng::readXmlFloat( layerNode, "gain", 1.0 );
					float fPitch = LocalFileMng::readXmlFloat( layerNode, "pitch", 0.0, false, false );

					if (drumkitPath != "") {
						sFilename = drumkitPath + "/" + sFilename;
					}
					Sample *pSample = Sample::load( sFilename );
					if (pSample == NULL) {
						ERRORLOG( "Error loading sample: " + sFilename + " not found" );
						pInstrument->set_muted( true );
					}
					InstrumentLayer *pLayer = new InstrumentLayer( pSample );
					pLayer->set_start_velocity( fMin );
					pLayer->set_end_velocity( fMax );
					pLayer->set_gain( fGain );
					pLayer->set_pitch( fPitch );
					pInstrument->set_layer( pLayer, nLayer );
					nLayer++;
				}
			}

			instrumentList->add( pInstrument );
		}
		if ( instrumentList_count == 0 ) {
			WARNINGLOG( "0 instruments?" );
		}

		song->setInstrumentList( instrumentList );
	}
	else {
		ERRORLOG( "Error reading song: instrumentList node not found");
		delete song;
		return NULL;
	}





	// Pattern list
	TiXmlNode* patterns = songNode->FirstChild("patternList");

	PatternList *patternList = new PatternList();
	int pattern_count = 0;
	TiXmlNode* patternNode = 0;
	for (patternNode = patterns->FirstChild("pattern"); patternNode; patternNode = patternNode->NextSibling( "pattern" )) {
		pattern_count++;
		Pattern *pat = getPattern(patternNode, instrumentList);
		if (pat) {
			patternList->add(pat);
		}
		else {
			ERRORLOG( "Error loading pattern" );
			delete patternList;
			delete song;
			return NULL;
		}
	}
	if ( pattern_count == 0 ) {
		WARNINGLOG( "0 patterns?" );
	}
	song->setPatternList(patternList);


	// Pattern sequence
	TiXmlNode* patternSequenceNode = songNode->FirstChild("patternSequence");

	vector<PatternList*>* pPatternGroupVector = new vector<PatternList*>;

	// back-compatibility code..
	for (TiXmlNode* pPatternIDNode = patternSequenceNode->FirstChild("patternID"); pPatternIDNode; pPatternIDNode = pPatternIDNode->NextSibling("patternID")) {
		WARNINGLOG( "Using old patternSequence code for back compatibility" );
		PatternList *patternSequence = new PatternList();
		string patId = pPatternIDNode->FirstChild()->Value();

		Pattern *pat = NULL;
		for (unsigned i = 0; i < patternList->getSize(); i++) {
			Pattern *tmp = patternList->get(i);
			if (tmp) {
				if (tmp->m_sName == patId) {
					pat = tmp;
					break;
				}
			}
		}
		if ( pat == NULL ) {
			WARNINGLOG( "patternid not found in patternSequence" );
			continue;
		}
		patternSequence->add( pat );

		pPatternGroupVector->push_back( patternSequence );
	}

	for (TiXmlNode* groupNode = patternSequenceNode->FirstChild("group"); groupNode; groupNode = groupNode->NextSibling("group")) {
		PatternList *patternSequence = new PatternList();
		for (TiXmlNode* patternId = groupNode->FirstChild("patternID"); patternId; patternId = patternId->NextSibling("patternID")) {
			string patId = patternId->FirstChild()->Value();

			Pattern *pat = NULL;
			for (unsigned i = 0; i < patternList->getSize(); i++) {
				Pattern *tmp = patternList->get(i);
				if (tmp) {
					if (tmp->m_sName == patId) {
						pat = tmp;
						break;
					}
				}
			}
			if ( pat == NULL ) {
				WARNINGLOG( "patternid not found in patternSequence" );
				continue;
			}
			patternSequence->add( pat );
		}
		pPatternGroupVector->push_back( patternSequence );
	}

	song->setPatternGroupVector( pPatternGroupVector );

#ifdef LADSPA_SUPPORT
	// reset FX
	for ( int fx = 0; fx < MAX_FX; ++fx ) {
		//LadspaFX* pFX = Effects::getInstance()->getLadspaFX( fx );
		//delete pFX;
		Effects::getInstance()->setLadspaFX( NULL, fx );
	}
#endif

	// LADSPA FX
	TiXmlNode* ladspaNode = songNode->FirstChild( "ladspa" );
	if (ladspaNode) {
		int nFX = 0;
		TiXmlNode* fxNode;
		for (fxNode = ladspaNode->FirstChild("fx"); fxNode; fxNode = fxNode->NextSibling("fx")) {
			string sName = LocalFileMng::readXmlString( fxNode, "name", "" );
			string sFilename = LocalFileMng::readXmlString( fxNode, "filename", "" );
			bool bEnabled = LocalFileMng::readXmlBool( fxNode, "enabled", false );
			float fVolume = LocalFileMng::readXmlFloat( fxNode, "volume", 1.0 );

			if (sName != "no plugin" ) {
				// FIXME: il caricamento va fatto fare all'engine, solo lui sa il samplerate esatto
#ifdef LADSPA_SUPPORT
				LadspaFX* pFX = LadspaFX::load( sFilename, sName, 44100 );
				Effects::getInstance()->setLadspaFX( pFX, nFX );
				if (pFX) {
					pFX->setEnabled( bEnabled );
					pFX->setVolume( fVolume );
					TiXmlNode* inputControlNode;
					for ( inputControlNode = fxNode->FirstChild("inputControlPort"); inputControlNode; inputControlNode = inputControlNode->NextSibling("inputControlPort")) {
						string sName = LocalFileMng::readXmlString( inputControlNode, "name", "" );
						float fValue = LocalFileMng::readXmlFloat( inputControlNode, "value", 0.0 );

						for (unsigned nPort = 0; nPort < pFX->inputControlPorts.size(); nPort++) {
							LadspaControlPort *port = pFX->inputControlPorts[ nPort ];
							if ( string(port->sName) == sName) {
								port->fControlValue = fValue;
							}
						}
					}

					TiXmlNode* outputControlNode;
					for ( outputControlNode = fxNode->FirstChild("outputControlPort"); outputControlNode; outputControlNode = outputControlNode->NextSibling("outputControlPort")) {
					}
				}
#endif
			}
			nFX++;
		}
	}
	else {
		WARNINGLOG( "ladspa node not found" );
	}


	song->m_bIsModified = false;
	song->setFilename(filename);

	return song;
}



Pattern* SongReader::getPattern(TiXmlNode* pattern, InstrumentList* instrList)
{
	Pattern *pPattern = NULL;

	string sName = "";	// name
	sName = LocalFileMng::readXmlString( pattern, "name", sName );

	int nSize = -1;
	nSize = LocalFileMng::readXmlInt( pattern, "size", nSize, false, false );

	pPattern = new Pattern( sName, nSize );



	TiXmlNode* pNoteListNode = pattern->FirstChild( "noteList" );
	if ( pNoteListNode ) {
		// new code :)
		for ( TiXmlNode* noteNode = pNoteListNode->FirstChild("note"); noteNode; noteNode = noteNode->NextSibling("note") ) {

			Note* pNote = NULL;

			unsigned nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
			float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );
			float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
			float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );
			int nLength = LocalFileMng::readXmlInt( noteNode, "length", -1, true );
			float nPitch = LocalFileMng::readXmlFloat( noteNode, "pitch", 0.0, false, false );
			string sKey = LocalFileMng::readXmlString( noteNode, "key", "C0", false, false );

			string instrId = LocalFileMng::readXmlString( noteNode, "instrument", "" );

			Instrument *instrRef = NULL;
			// search instrument by ref
			for (unsigned i = 0; i < instrList->get_size(); i++) {
				Instrument *instr = instrList->get(i);
				if (instrId == instr->get_id()) {
					instrRef = instr;
					break;
				}
			}
			if ( !instrRef ) {
				ERRORLOG( "Instrument with ID: '" + instrId + "' not found. Note skipped." );
				continue;
			}
			//assert( instrRef );

			pNote = new Note( instrRef, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch, Note::stringToKey( sKey ) );
			pPattern->m_noteMap.insert( std::make_pair( pNote->get_position(), pNote ) );
		}
	}
	else {
		// Back compatibility code. Version < 0.9.4
		TiXmlNode* sequenceListNode = pattern->FirstChild("sequenceList");

		int sequence_count = 0;
		TiXmlNode* sequenceNode = 0;
		for ( sequenceNode = sequenceListNode->FirstChild("sequence"); sequenceNode; sequenceNode = sequenceNode->NextSibling("sequence")) {
			sequence_count++;

			TiXmlNode* noteListNode = sequenceNode->FirstChild( "noteList" );
			for ( TiXmlNode* noteNode = noteListNode->FirstChild("note"); noteNode; noteNode = noteNode->NextSibling("note") ) {

				Note* pNote = NULL;

				unsigned nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
				float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );
				float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
				float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );
				int nLength = LocalFileMng::readXmlInt( noteNode, "length", -1, true );
				float nPitch = LocalFileMng::readXmlFloat( noteNode, "pitch", 0.0, false, false );

				string instrId = LocalFileMng::readXmlString( noteNode, "instrument", "" );

				Instrument *instrRef = NULL;
				// search instrument by ref
				for (unsigned i = 0; i < instrList->get_size(); i++) {
					Instrument *instr = instrList->get(i);
					if (instrId == instr->get_id()) {
						instrRef = instr;
						break;
					}
				}
				assert( instrRef );

				pNote = new Note( instrRef, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch);

				//infoLog( "new note!! pos: " + toString( pNote->m_nPosition ) + "\t instr: " + instrId );
				pPattern->m_noteMap.insert( std::make_pair( pNote->get_position(), pNote ) );
			}
		}
	}

	return pPattern;
}

};




