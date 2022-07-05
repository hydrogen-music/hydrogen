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

#include "Version.h"

#include <cassert>
#include <memory>

#include <core/Preferences/Preferences.h>
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
#include <core/Hydrogen.h>
#include <core/Sampler/Sampler.h>

#ifdef H2CORE_HAVE_OSC
#include <core/NsmClient.h>
#endif

#include <QDir>

namespace
{

}//anonymous namespace
namespace H2Core
{

Song::Song( const QString& sName, const QString& sAuthor, float fBpm, float fVolume )
	: m_bIsTimelineActivated( false )
	, m_bIsMuted( false )
	, m_resolution( 48 )
	, m_fBpm( fBpm )
	, m_sName( sName )
	, m_sAuthor( sAuthor )
	, m_fVolume( fVolume )
	, m_fMetronomeVolume( 0.5 )
	, m_sNotes( "" )
	, m_pPatternList( nullptr )
	, m_pPatternGroupSequence( nullptr )
	, m_pInstrumentList( nullptr )
	, m_pComponents( nullptr )
	, m_sFilename( "" )
	, m_loopMode( LoopMode::Disabled )
	, m_patternMode( PatternMode::Selected )
	, m_fHumanizeTimeValue( 0.0 )
	, m_fHumanizeVelocityValue( 0.0 )
	, m_fSwingFactor( 0.0 )
	, m_bIsModified( false )
	, m_mode( Mode::Pattern )
	, m_sPlaybackTrackFilename( "" )
	, m_bPlaybackTrackEnabled( false )
	, m_fPlaybackTrackVolume( 0.0 )
	, m_pVelocityAutomationPath( nullptr )
	, m_license( License( "", sAuthor ) )
	, m_actionMode( ActionMode::selectMode )
	, m_bIsPatternEditorLocked( false )
	, m_nPanLawType ( Sampler::RATIO_STRAIGHT_POLYGONAL )
	, m_fPanLawKNorm ( Sampler::K_NORM_DEFAULT )
	, m_currentDrumkitLookup( Filesystem::Lookup::stacked )
{
	INFOLOG( QString( "INIT '%1'" ).arg( sName ) );

	m_pComponents = new std::vector<DrumkitComponent*> ();
	m_pVelocityAutomationPath = new AutomationPath(0.0f, 1.5f,  1.0f);

	m_pTimeline = std::make_shared<Timeline>();
}

Song::~Song()
{
	/*
	 * Warning: it is not safe to delete a song without having a lock on the audio engine.
	 * Following the current design, the caller has to care for the lock.
	 */
	
	delete m_pPatternList;

	for (std::vector<DrumkitComponent*>::iterator it = m_pComponents->begin() ; it != m_pComponents->end(); ++it) {
		delete *it;
	}
	delete m_pComponents;

	if ( m_pPatternGroupSequence ) {
		for ( unsigned i = 0; i < m_pPatternGroupSequence->size(); ++i ) {
			PatternList* pPatternList = ( *m_pPatternGroupSequence )[i];
			pPatternList->clear();	// pulisco tutto, i pattern non vanno distrutti qua
			delete pPatternList;
		}
		delete m_pPatternGroupSequence;
	}

	delete m_pInstrumentList;

	delete m_pVelocityAutomationPath;

	INFOLOG( QString( "DESTROY '%1'" ).arg( m_sName ) );
}

void Song::setBpm( float fBpm ) {
	if ( fBpm > MAX_BPM ) {
		m_fBpm = MAX_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too high. Assigning upper bound %2 instead" )
					.arg( fBpm ).arg( MAX_BPM ) );
	} else if ( fBpm < MIN_BPM ) {
		m_fBpm = MIN_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too low. Assigning lower bound %2 instead" )
					.arg( fBpm ).arg( MIN_BPM ) );
	} else {
		m_fBpm = fBpm;
	}
}

void Song::setActionMode( Song::ActionMode actionMode ) {
	m_actionMode = actionMode;
}

long Song::lengthInTicks() const {
	long nSongLength = 0;
	int nColumns = m_pPatternGroupSequence->size();
	// Sum the lengths of all pattern columns and use the macro
	// MAX_NOTES in case some of them are of size zero.
	for ( int i = 0; i < nColumns; i++ ) {
		PatternList *pColumn = ( *m_pPatternGroupSequence )[ i ];
		if ( pColumn->size() != 0 ) {
			nSongLength += pColumn->longest_pattern_length();
		} else {
			nSongLength += MAX_NOTES;
		}
	}
    return nSongLength;
}

bool Song::isPatternActive( int nColumn, int nRow ) const {
	if ( nRow < 0 || nRow > m_pPatternList->size() ) {
		return false;
	}
	
	auto pPattern = m_pPatternList->get( nRow );
	if ( pPattern == nullptr ) {
		return false;
	}
	if ( nColumn < 0 || nColumn >= m_pPatternGroupSequence->size() ) {
		return false;
	}
	auto pColumn = ( *m_pPatternGroupSequence )[ nColumn ];
	if ( pColumn->index( pPattern ) == -1 ) {
		return false;
	}

	return true;
}
	
///Load a song from file
std::shared_ptr<Song> Song::load( const QString& sFilename, bool bSilent )
{
	QString sPath = Filesystem::absolute_path( sFilename, bSilent );
	if ( sPath.isEmpty() ) {
		return nullptr;
	}

	if ( ! bSilent ) {
		INFOLOG( "Reading " + sPath );
	}

	XMLDoc doc;
	if ( ! doc.read( sFilename ) && ! bSilent ) {
		ERRORLOG( QString( "Something went wrong while loading song [%1]" )
				  .arg( sFilename ) );
	}
				  
	XMLNode songNode = doc.firstChildElement( "song" );

	if ( songNode.isNull() ) {
		if ( ! bSilent ) {
			ERRORLOG( "Error reading song: 'song' node not found" );
		}
		return nullptr;
	}

	if ( ! bSilent ) {
		QString sSongVersion = songNode.read_string( "version", "Unknown version", false, false );
		if ( sSongVersion != QString( get_version().c_str() ) ) {
			INFOLOG( QString( "Trying to load a song [%1] created with a different version [%2] of hydrogen. Current version: %3" )
					 .arg( sFilename )
					 .arg( sSongVersion )
					 .arg( get_version().c_str() ) );
		}
	}

	auto pSong = Song::loadFrom( &songNode, bSilent );
	if ( pSong != nullptr ) {
		pSong->setFilename( sFilename );
	}

	return pSong;
}

std::shared_ptr<Song> Song::loadFrom( XMLNode* pRootNode, bool bSilent )
{
	auto pPreferences = Preferences::get_instance();
	
	float fBpm = pRootNode->read_float( "bpm", 120, false, false );
	float fVolume = pRootNode->read_float( "volume", 0.5, false, false );
	QString sName( pRootNode->read_string( "name", "Untitled Song", false, false ) );
	QString sAuthor( pRootNode->read_string( "author", "Unknown Author", false, false ) );

	std::shared_ptr<Song> pSong = std::make_shared<Song>( sName, sAuthor, fBpm, fVolume );

	pSong->setMetronomeVolume( pRootNode->read_float( "metronomeVolume", 0.5, false, false ) );
	pSong->setNotes( pRootNode->read_string( "notes", "...", false, false ) );
	pSong->setLicense( License( pRootNode->read_string( "license", "", false, false ), sAuthor ) );
	if ( pRootNode->read_bool( "loopEnabled", false, false, false ) ) {
		pSong->setLoopMode( Song::LoopMode::Enabled );
	} else {
		pSong->setLoopMode( Song::LoopMode::Disabled );
	}
	
	if ( pRootNode->read_bool( "patternModeMode",
							   static_cast<bool>(Song::PatternMode::Selected),
							   false, false ) ) {
		pSong->setPatternMode( Song::PatternMode::Selected );
	} else {
		pSong->setPatternMode( Song::PatternMode::Stacked );
	}

	if ( pRootNode->read_string( "mode", "pattern", false, false ) == "song" ) {
		pSong->setMode( Song::Mode::Song );
	} else {
		pSong->setMode( Song::Mode::Pattern );
	}

	QString sPlaybackTrack( pRootNode->read_string( "playbackTrackFilename", "", false, false ) );
	// Check the file of the playback track and resort to the default
	// in case the file can not be found.
	if ( ! sPlaybackTrack.isEmpty() &&
		 ! Filesystem::file_exists( sPlaybackTrack, true ) ) {
		if ( ! bSilent ) {
			ERRORLOG( QString( "Provided playback track file [%1] does not exist. Using empty string instead" )
					  .arg( sPlaybackTrack ) );
		}
		sPlaybackTrack = "";
	}
	pSong->setPlaybackTrackFilename( sPlaybackTrack );
	pSong->setPlaybackTrackEnabled( pRootNode->read_bool( "playbackTrackEnabled", false, false, false ) );
	pSong->setPlaybackTrackVolume( pRootNode->read_float( "playbackTrackVolume", 0.0, false, false ) );
	
	pSong->setHumanizeTimeValue( pRootNode->read_float( "humanize_time", 0.0, false, false ) );
	pSong->setHumanizeVelocityValue( pRootNode->read_float( "humanize_velocity", 0.0, false, false ) );
	pSong->setSwingFactor( pRootNode->read_float( "swing_factor", 0.0, false, false ) );
	pSong->setActionMode( static_cast<Song::ActionMode>(
		pRootNode->read_int( "action_mode",
							 static_cast<int>( Song::ActionMode::selectMode ), false, false ) ) );
	pSong->setIsPatternEditorLocked( pRootNode->read_bool( "isPatternEditorLocked", false, false, false ) );

	bool bContainsIsTimelineActivated;
	bool bIsTimelineActivated =
		pRootNode->read_bool( "isTimelineActivated", false,
							  &bContainsIsTimelineActivated, false, false );
	if ( ! bContainsIsTimelineActivated ) {
		// .h2song file was created in an older version of
		// Hydrogen. Using the Timeline state in the
		// Preferences as a fallback.
		bIsTimelineActivated = pPreferences->getUseTimelineBpm();
	} else {
		pPreferences->setUseTimelineBpm( bIsTimelineActivated );
	}
	pSong->setIsTimelineActivated( bIsTimelineActivated );
	
	// pan law
	QString sPanLawType( pRootNode->read_string( "pan_law_type", "RATIO_STRAIGHT_POLYGONAL", false, false ) );
	if ( sPanLawType == "RATIO_STRAIGHT_POLYGONAL" ) {
		pSong->setPanLawType( Sampler::RATIO_STRAIGHT_POLYGONAL );
	} else if ( sPanLawType == "RATIO_CONST_POWER" ) {
		pSong->setPanLawType( Sampler::RATIO_CONST_POWER );
	} else if ( sPanLawType == "RATIO_CONST_SUM" ) {
		pSong->setPanLawType( Sampler::RATIO_CONST_SUM );
	} else if ( sPanLawType == "LINEAR_STRAIGHT_POLYGONAL" ) {
		pSong->setPanLawType( Sampler::LINEAR_STRAIGHT_POLYGONAL );
	} else if ( sPanLawType == "LINEAR_CONST_POWER" ) {
		pSong->setPanLawType( Sampler::LINEAR_CONST_POWER );
	} else if ( sPanLawType == "LINEAR_CONST_SUM" ) {
		pSong->setPanLawType( Sampler::LINEAR_CONST_SUM );
	} else if ( sPanLawType == "POLAR_STRAIGHT_POLYGONAL" ) {
		pSong->setPanLawType( Sampler::POLAR_STRAIGHT_POLYGONAL );
	} else if ( sPanLawType == "POLAR_CONST_POWER" ) {
		pSong->setPanLawType( Sampler::POLAR_CONST_POWER );
	} else if ( sPanLawType == "POLAR_CONST_SUM" ) {
		pSong->setPanLawType( Sampler::POLAR_CONST_SUM );
	} else if ( sPanLawType == "QUADRATIC_STRAIGHT_POLYGONAL" ) {
		pSong->setPanLawType( Sampler::QUADRATIC_STRAIGHT_POLYGONAL );
	} else if ( sPanLawType == "QUADRATIC_CONST_POWER" ) {
		pSong->setPanLawType( Sampler::QUADRATIC_CONST_POWER );
	} else if ( sPanLawType == "QUADRATIC_CONST_SUM" ) {
		pSong->setPanLawType( Sampler::QUADRATIC_CONST_SUM );
	} else if ( sPanLawType == "LINEAR_CONST_K_NORM" ) {
		pSong->setPanLawType( Sampler::LINEAR_CONST_K_NORM );
	} else if ( sPanLawType == "POLAR_CONST_K_NORM" ) {
		pSong->setPanLawType( Sampler::POLAR_CONST_K_NORM );
	} else if ( sPanLawType == "RATIO_CONST_K_NORM" ) {
		pSong->setPanLawType( Sampler::RATIO_CONST_K_NORM );
	} else if ( sPanLawType == "QUADRATIC_CONST_K_NORM" ) {
		pSong->setPanLawType( Sampler::QUADRATIC_CONST_K_NORM );
	} else {
		pSong->setPanLawType( Sampler::RATIO_STRAIGHT_POLYGONAL );
		if ( ! bSilent ) {
			WARNINGLOG( "Unknown pan law type in import song. Set default." );
		}
	}

	float fPanLawKNorm = pRootNode->read_float( "pan_law_k_norm", Sampler::K_NORM_DEFAULT, false, false );
	if ( fPanLawKNorm <= 0.0 ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Invalid pan law k in import song [%1] (<= 0). Set default k." )
						.arg( fPanLawKNorm ) );
		}
		fPanLawKNorm = Sampler::K_NORM_DEFAULT;
	}
	pSong->setPanLawKNorm( fPanLawKNorm );

	XMLNode componentListNode = pRootNode->firstChildElement( "componentList" );
	if ( ( ! componentListNode.isNull()  ) ) {
		XMLNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
		while ( ! componentNode.isNull()  ) {
			int id = componentNode.read_int( "id", -1, false, false );			// instrument id
			QString sName = componentNode.read_string( "name", "", false, false );		// name
			
			DrumkitComponent* pDrumkitComponent = new DrumkitComponent( id, sName );
			pDrumkitComponent->set_volume( componentNode.read_float( "volume", 1.0, false, false ) );

			pSong->getComponents()->push_back(pDrumkitComponent);

			componentNode = componentNode.nextSiblingElement( "drumkitComponent" );
		}
	} else {
		DrumkitComponent* pDrumkitComponent = new DrumkitComponent( 0, "Main" );
		pSong->getComponents()->push_back(pDrumkitComponent);
	}

	//  Instrument List
	InstrumentList* pInstrList = new InstrumentList();

	XMLNode instrumentListNode = pRootNode->firstChildElement( "instrumentList" );
	if ( ( ! instrumentListNode.isNull()  ) ) {
		// INSTRUMENT NODE
		int instrumentList_count = 0;
		XMLNode instrumentNode = instrumentListNode.firstChildElement( "instrument" );
		while ( ! instrumentNode.isNull()  ) {
			instrumentList_count++;

			int id = instrumentNode.read_int( "id", -1, false, false );			// instrument id
			if ( id == -1 ) {
				if ( ! bSilent ) {
					ERRORLOG( "Empty ID for instrument '" + sName + "'. skipping." );
				}
				instrumentNode = instrumentNode.nextSiblingElement( "instrument" );
				continue;
			}

			QString sDrumkit = instrumentNode.read_string( "drumkit", "", false, false );	// drumkit
			pSong->setCurrentDrumkitName( sDrumkit );
			int iLookup = instrumentNode.read_int( "drumkitLookup", -1, false, false );	// drumkit
			if ( iLookup == -1 ) {
				// Song was created with an older version of the
				// Hydrogen and we just have the name of the drumkit
				// and no information whether it is found at user- or
				// system-level.

				if ( ! Filesystem::drumkit_path_search( sDrumkit, Filesystem::Lookup::user, true ).isEmpty() ) {
					iLookup = 1;
					if ( ! bSilent ) {
						WARNINGLOG( QString( "Missing drumkitLookup for kit [%1]: user-level determined" )
									.arg( sDrumkit ) );
					}
				}
				else if ( ! Filesystem::drumkit_path_search( sDrumkit, Filesystem::Lookup::system, true ).isEmpty() ) {
					iLookup = 2;
					if ( ! bSilent ) {
						WARNINGLOG( QString( "Missing drumkitLookup for kit [%1]: system-level determined" )
									.arg( sDrumkit ) );
					}
				}
				else {
					iLookup = 2;
					if ( ! bSilent ) {
						ERRORLOG( QString( "Missing drumkitLookup for kit [%1]: drumkit could not be found. system-level will be set as a fallback" )
									.arg( sDrumkit ) );
					}
				}
			}	
			pSong->setCurrentDrumkitLookup( static_cast<Filesystem::Lookup>( iLookup ) );
			
			QString sName = instrumentNode.read_string( "name", "", false, false );
			int fAttack = instrumentNode.read_int( "Attack", 0, true, false );
			int fDecay = instrumentNode.read_int( "Decay", 0, true, false );
			float fSustain = instrumentNode.read_float( "Sustain", 1.0, true, false );
			int fRelease = instrumentNode.read_float( "Release", 1000.0, true, false );

			auto pInstrument = std::make_shared<Instrument>( id, sName, std::make_shared<ADSR>( fAttack, fDecay, fSustain, fRelease ) );

			pInstrument->set_volume( instrumentNode.read_float( "volume", 1.0, false, false ) );
			pInstrument->set_muted( instrumentNode.read_bool( "isMuted", false, false, false ) );
			pInstrument->set_soloed( instrumentNode.read_bool( "isSoloed", false, false, false ) );
			
			bool bFound, bFound2;
			float fPan = instrumentNode.read_float( "pan", 0.f, &bFound, true, false );
			if ( !bFound ) {
				// check if pan is expressed in the old fashion (version <= 1.1 ) with the pair (pan_L, pan_R)
				float fPanL = instrumentNode.read_float( "pan_L", 1.f, &bFound, false, false );
				float fPanR = instrumentNode.read_float( "pan_R", 1.f, &bFound2, false, false );
				if ( bFound == true && bFound2 == true ) { // found nodes pan_L and pan_R
					fPan = Sampler::getRatioPan( fPanL, fPanR );  // convert to single pan parameter
				}
			}
			pInstrument->setPan( fPan );
			
			pInstrument->set_drumkit_name( sDrumkit );
			pInstrument->set_drumkit_lookup( static_cast<Filesystem::Lookup>( iLookup ) );
			pInstrument->set_apply_velocity( instrumentNode.read_bool( "applyVelocity", true, false, false ) );
			pInstrument->set_fx_level( instrumentNode.read_float( "FX1Level", 0.0, false, false ), 0 );
			pInstrument->set_fx_level( instrumentNode.read_float( "FX2Level", 0.0, false, false ), 1 );
			pInstrument->set_fx_level( instrumentNode.read_float( "FX3Level", 0.0, false, false ), 2 );
			pInstrument->set_fx_level( instrumentNode.read_float( "FX4Level", 0.0, false, false ), 3 );
			pInstrument->set_pitch_offset( instrumentNode.read_float( "pitchOffset", 0.0f, true, false ) );
			pInstrument->set_random_pitch_factor( instrumentNode.read_float( "randomPitchFactor", 0.0f, true, false ) );
			pInstrument->set_filter_active( instrumentNode.read_bool( "filterActive", false, false, false ) );
			pInstrument->set_filter_cutoff( instrumentNode.read_float( "filterCutoff", 1.0f, false, false ) );
			pInstrument->set_filter_resonance( instrumentNode.read_float( "filterResonance", 0.0f, false, false ) );
			pInstrument->set_gain( instrumentNode.read_float( "gain", 1.0, true, false ) );

			// create a new instrument
			pInstrument->set_mute_group( instrumentNode.read_string( "muteGroup", "-1", false, false ).toInt() );
			pInstrument->set_stop_notes( instrumentNode.read_bool( "isStopNote", false, false, false ) );
			pInstrument->set_hihat_grp( instrumentNode.read_int( "isHihat", -1, false, true ) );
			pInstrument->set_lower_cc( instrumentNode.read_int( "lower_cc", 0, false, true ) );
			pInstrument->set_higher_cc( instrumentNode.read_int( "higher_cc", 127, false, true ) );

			QString sRead_sample_select_algo = instrumentNode.read_string( "sampleSelectionAlgo", "VELOCITY", false, false );
			if ( sRead_sample_select_algo.compare("VELOCITY") == 0 ) {
				pInstrument->set_sample_selection_alg( Instrument::VELOCITY );
			} else if ( sRead_sample_select_algo.compare("ROUND_ROBIN") == 0 ) {
				pInstrument->set_sample_selection_alg( Instrument::ROUND_ROBIN );
			} else if ( sRead_sample_select_algo.compare("RANDOM") == 0 ) {
				pInstrument->set_sample_selection_alg( Instrument::RANDOM );
			}
			pInstrument->set_midi_out_channel( instrumentNode.read_string( "midiOutChannel", "-1", true, false ).toInt() );
			pInstrument->set_midi_out_note( instrumentNode.read_string( "midiOutNote", "60", true, false ).toInt() );

			QString drumkitPath;
			if ( ( !sDrumkit.isEmpty() ) && ( sDrumkit != "-" ) ) {
				drumkitPath = Filesystem::drumkit_path_search( sDrumkit );
			} else if ( ! bSilent ) {
				ERRORLOG( "Missing drumkit path" );
			}

			// Get license used by drumkit.
			License drumkitLicense = Drumkit::loadLicenseFrom( drumkitPath );

			XMLNode sFilenameNode = instrumentNode.firstChildElement( "filename" );

			// back compatibility code ( song version <= 0.9.0 )
			if ( ! sFilenameNode.isNull() ) {
				if ( ! bSilent ) {
					WARNINGLOG( "Using back compatibility code. sFilename node found" );
				}
				QString sFilename = instrumentNode.read_string( "filename", "", false, false );

				if ( !QFile( sFilename ).exists() && !drumkitPath.isEmpty() ) {
					sFilename = drumkitPath + "/" + sFilename;
				}
				auto pSample = Sample::load( sFilename, drumkitLicense );
				if ( pSample == nullptr ) {
					// nel passaggio tra 0.8.2 e 0.9.0 il drumkit di default e' cambiato.
					// Se fallisce provo a caricare il corrispettivo file in formato flac
//					warningLog( "[readSong] Error loading sample: " + sFilename + " not found. Trying to load a flac..." );
					sFilename = sFilename.left( sFilename.length() - 4 );
					sFilename += ".flac";
					pSample = Sample::load( sFilename, drumkitLicense );
				}
				if ( pSample == nullptr ) {
					if ( ! bSilent ) {
						ERRORLOG( "Error loading sample: " + sFilename + " not found" );
					}
					pInstrument->set_muted( true );
					pInstrument->set_missing_samples( true );
				}
				auto pCompo = std::make_shared<InstrumentComponent>( 0 );
				auto pLayer = std::make_shared<InstrumentLayer>( pSample );
				pCompo->set_layer( pLayer, 0 );
				pInstrument->get_components()->push_back( pCompo );
			}
			//~ back compatibility code
			else {
				bool bFoundAtLeastOneComponent = false;
				XMLNode componentNode = instrumentNode.firstChildElement( "instrumentComponent" );
				while ( ! componentNode.isNull()  ) {
					bFoundAtLeastOneComponent = true;
					auto pCompo = std::make_shared<InstrumentComponent>(
						componentNode.read_int( "component_id", 0, false, false ) );
					pCompo->set_gain( componentNode.read_float( "gain", 1.0, false, false ) );

					unsigned nLayer = 0;
					XMLNode layerNode = componentNode.firstChildElement( "layer" );
					while ( ! layerNode.isNull()  ) {
						if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
							if ( ! bSilent ) {
								ERRORLOG( QString( "nLayer (%1) > m_nMaxLayers (%2)" )
										  .arg( nLayer )
										  .arg( InstrumentComponent::getMaxLayers() ) );
							}
							continue;
						}
						//bool sIsModified = false;
						QString sFilename = layerNode.read_string( "filename", "", false, false );
						bool sIsModified = layerNode.read_bool( "ismodified", false, false, false );
						Sample::Loops lo;
						lo.mode = Sample::parse_loop_mode( layerNode.read_string( "smode", "forward", false, false ) );
						lo.start_frame = layerNode.read_int( "startframe", 0, false, false );
						lo.loop_frame = layerNode.read_int( "loopframe", 0, false, false );
						lo.count = layerNode.read_int( "loops", 0, false, false );
						lo.end_frame = layerNode.read_int( "endframe", 0, false, false );
						Sample::Rubberband ro;
						ro.use = layerNode.read_int( "userubber", 0, false, false );
						ro.divider = layerNode.read_float( "rubberdivider", 0.0, false, false );
						ro.c_settings = layerNode.read_int( "rubberCsettings", 1, false, false);
						ro.pitch = layerNode.read_float( "rubberPitch", 0.0, false, false );

						float fMin = layerNode.read_float( "min", 0.0, false, false );
						float fMax = layerNode.read_float( "max", 1.0, false, false );
						float fGain = layerNode.read_float( "gain", 1.0, false, false );
						float fPitch = layerNode.read_float( "pitch", 0.0, true, false );

						if ( ! QFile( sFilename ).exists() &&
							 ! drumkitPath.isEmpty() &&
							 ! sFilename.startsWith( "/" ) ) {
							sFilename = drumkitPath + "/" + sFilename;
						}

						QString program = pPreferences->m_rubberBandCLIexecutable;
						//test the path. if test fails, disable rubberband
						if ( QFile( program ).exists() == false ) {
							ro.use = false;
						}

						std::shared_ptr<Sample> pSample;
						if ( !sIsModified ) {
							pSample = Sample::load( sFilename, drumkitLicense );
						} else {
							// FIXME, kill EnvelopePoint, create Envelope class
							EnvelopePoint pt;

							Sample::VelocityEnvelope velocity;
							XMLNode volumeNode = layerNode.firstChildElement( "volume" );
							while ( ! volumeNode.isNull()  ) {
								pt.frame = volumeNode.read_int( "volume-position", 0, false, false );
								pt.value = volumeNode.read_int( "volume-value", 0, false, false );
								velocity.push_back( pt );
								volumeNode = volumeNode.nextSiblingElement( "volume" );
								//ERRORLOG( QString("volume-posi %1").arg(volumeNode.read_int( "volume-position", 0)) );
							}

							Sample::VelocityEnvelope pan;
							XMLNode panNode = layerNode.firstChildElement( "pan" );
							while ( ! panNode.isNull()  ) {
								pt.frame = panNode.read_int( "pan-position", 0, false, false );
								pt.value = panNode.read_int( "pan-value", 0, false, false );
								pan.push_back( pt );
								panNode = panNode.nextSiblingElement( "pan" );
							}

							pSample = Sample::load( sFilename, lo, ro, velocity,
													pan, fBpm, drumkitLicense );
						}
						if ( pSample == nullptr ) {
							if ( ! bSilent ) {
								ERRORLOG( "Error loading sample: " + sFilename + " not found" );
							}
							pInstrument->set_muted( true );
							pInstrument->set_missing_samples( true );
						}
						auto pLayer = std::make_shared<InstrumentLayer>( pSample );
						pLayer->set_start_velocity( fMin );
						pLayer->set_end_velocity( fMax );
						pLayer->set_gain( fGain );
						pLayer->set_pitch( fPitch );
						pCompo->set_layer( pLayer, nLayer );
						nLayer++;

						layerNode = layerNode.nextSiblingElement( "layer" );
					}

					pInstrument->get_components()->push_back( pCompo );
					componentNode = componentNode.nextSiblingElement( "instrumentComponent" );
				}
				
				if( ! bFoundAtLeastOneComponent ) {
					auto pCompo = std::make_shared<InstrumentComponent>( 0 );
					pCompo->set_gain( componentNode.read_float( "gain", 1.0, false, false ) );

					unsigned nLayer = 0;
					XMLNode layerNode = instrumentNode.firstChildElement( "layer" );
					while (  ! layerNode.isNull()  ) {
						if ( nLayer >= InstrumentComponent::getMaxLayers() ) {
							if ( ! bSilent ) {
								ERRORLOG( QString( "nLayer (%1) > m_nMaxLayers (%2)" )
										  .arg( nLayer )
										  .arg( InstrumentComponent::getMaxLayers() ) );
							}
							continue;
						}
						QString sFilename = layerNode.read_string( "filename", "", false, false );
						bool sIsModified = layerNode.read_bool( "ismodified", false, false, false );
						Sample::Loops lo;
						lo.mode = Sample::parse_loop_mode( layerNode.read_string( "smode", "forward", false, false ) );
						lo.start_frame = layerNode.read_int( "startframe", 0, false, false );
						lo.loop_frame = layerNode.read_int( "loopframe", 0, false, false );
						lo.count = layerNode.read_int( "loops", 0, false, false );
						lo.end_frame = layerNode.read_int( "endframe", 0, false, false );
						Sample::Rubberband ro;
						ro.use = layerNode.read_int( "userubber", 0, false, false );
						ro.divider = layerNode.read_float( "rubberdivider", 0.0, false, false );
						ro.c_settings = layerNode.read_int( "rubberCsettings", 1, false, false );
						ro.pitch = layerNode.read_float( "rubberPitch", 0.0, false, false );

						float fMin = layerNode.read_float( "min", 0.0, false, false );
						float fMax = layerNode.read_float( "max", 1.0, false, false );
						float fGain = layerNode.read_float( "gain", 1.0, false, false );
						float fPitch = layerNode.read_float( "pitch", 0.0, true, false );

						if ( !QFile( sFilename ).exists() && !drumkitPath.isEmpty() ) {
							sFilename = drumkitPath + "/" + sFilename;
						}

						QString program = pPreferences->m_rubberBandCLIexecutable;
						//test the path. if test fails, disable rubberband
						if ( QFile( program ).exists() == false ) {
							ro.use = false;
						}

						std::shared_ptr<Sample> pSample = nullptr;
						if ( !sIsModified ) {
							pSample = Sample::load( sFilename );
						} else {
							EnvelopePoint pt;

							Sample::VelocityEnvelope velocity;
							XMLNode volumeNode = layerNode.firstChildElement( "volume" );
							while (  ! volumeNode.isNull()  ) {
								pt.frame = volumeNode.read_int( "volume-position", 0, false, false );
								pt.value = volumeNode.read_int( "volume-value", 0, false, false );
								velocity.push_back( pt );
								volumeNode = volumeNode.nextSiblingElement( "volume" );
								//ERRORLOG( QString("volume-posi %1").arg(volumeNode.read_int( "volume-position", 0)) );
							}

							Sample::VelocityEnvelope pan;
							XMLNode  panNode = layerNode.firstChildElement( "pan" );
							while (  ! panNode.isNull()  ) {
								pt.frame = panNode.read_int( "pan-position", 0, false, false );
								pt.value = panNode.read_int( "pan-value", 0, false, false );
								pan.push_back( pt );
								panNode = panNode.nextSiblingElement( "pan" );
							}

							pSample = Sample::load( sFilename, lo, ro, velocity, pan, fBpm );
						}
						if ( pSample == nullptr ) {
							if ( ! bSilent ) {
								ERRORLOG( "Error loading sample: " + sFilename + " not found" );
							}
							pInstrument->set_muted( true );
							pInstrument->set_missing_samples( true );
						}
						auto pLayer = std::make_shared<InstrumentLayer>( pSample );
						pLayer->set_start_velocity( fMin );
						pLayer->set_end_velocity( fMax );
						pLayer->set_gain( fGain );
						pLayer->set_pitch( fPitch );
						pCompo->set_layer( pLayer, nLayer );
						nLayer++;

						layerNode = layerNode.nextSiblingElement( "layer" );
					}
					pInstrument->get_components()->push_back( pCompo );
				}
			}
			pInstrList->add( pInstrument );
			instrumentNode = instrumentNode.nextSiblingElement( "instrument" );
		}

		if ( instrumentList_count == 0 && ! bSilent ) {
			WARNINGLOG( "0 instruments?" );
		}
		pSong->setInstrumentList( pInstrList );
	}
	else {
		if ( ! bSilent ) {
			ERRORLOG( "Error reading song: instrumentList node not found" );
		}
		delete pInstrList;
		return nullptr;
	}

	// Pattern list
	XMLNode patterns = pRootNode->firstChildElement( "patternList" );

	PatternList* pPatternList = new PatternList();
	int pattern_count = 0;

	XMLNode patternNode =  patterns.firstChildElement( "pattern" );
	while ( !patternNode.isNull()  ) {
		pattern_count++;
		Pattern* pPattern = Pattern::load_from( &patternNode, pInstrList );
		if ( pPattern != nullptr ) {
			pPatternList->add( pPattern );
		}
		else {
			if ( ! bSilent ) {
				ERRORLOG( "Error loading pattern" );
			}
			delete pPatternList;
			return nullptr;
		}
		patternNode = patternNode.nextSiblingElement( "pattern" );
	}
	if ( pattern_count == 0 && ! bSilent ) {
		WARNINGLOG( "0 patterns?" );
	}
	pSong->setPatternList( pPatternList );

	// Virtual Patterns
	XMLNode virtualPatternListNode = pRootNode->firstChildElement( "virtualPatternList" );
	XMLNode virtualPatternNode = virtualPatternListNode.firstChildElement( "pattern" );
	if ( ! virtualPatternNode.isNull() ) {

		while ( ! virtualPatternNode.isNull() ) {
			QString sName = virtualPatternNode.read_string( "name", sName, false, false );

			Pattern* pCurPattern = nullptr;
			unsigned nPatterns = pPatternList->size();
			for ( unsigned i = 0; i < nPatterns; i++ ) {
				Pattern* pPattern = pPatternList->get( i );

				if ( pPattern->get_name() == sName ) {
					pCurPattern = pPattern;
					break;
				}
			}

			if ( pCurPattern != nullptr ) {
				XMLNode virtualNode = virtualPatternNode.firstChildElement( "virtual" );
				while ( !virtualNode.isNull() ) {
					QString virtName = virtualNode.firstChild().nodeValue();

					Pattern* virtPattern = nullptr;
					for ( unsigned i = 0; i < nPatterns; i++ ) {
						Pattern* pat = pPatternList->get( i );

						if ( pat->get_name() == virtName ) {
							virtPattern = pat;
							break;
						}
					}

					if ( virtPattern != nullptr ) {
						pCurPattern->virtual_patterns_add( virtPattern );
					}
					else if ( ! bSilent ) {
						ERRORLOG( "Song had invalid virtual pattern list data (virtual)" );
					}
					virtualNode = virtualNode.nextSiblingElement( "virtual" );
				}
			} else if ( ! bSilent ) {
				ERRORLOG( "Song had invalid virtual pattern list data (name)" );
			}
			virtualPatternNode = virtualPatternNode.nextSiblingElement( "pattern" );
		}
	}

	pPatternList->flattened_virtual_patterns_compute();

	// Pattern sequence
    XMLNode patternSequenceNode = pRootNode->firstChildElement( "patternSequence" );

	std::vector<PatternList*>* pPatternGroupVector = new std::vector<PatternList*>;

	// back-compatibility code..
	XMLNode pPatternIDNode = patternSequenceNode.firstChildElement( "patternID" );
	while ( ! pPatternIDNode.isNull()  ) {
		if ( ! bSilent ) {
			WARNINGLOG( "Using old patternSequence code for back compatibility" );
		}
		PatternList* pPatternSequence = new PatternList();
		QString patId = pPatternIDNode.firstChildElement().text();

		Pattern* pPattern = nullptr;
		for ( unsigned i = 0; i < pPatternList->size(); i++ ) {
			Pattern* pTmpPattern = pPatternList->get( i );
			if ( pTmpPattern != nullptr ) {
				if ( pTmpPattern->get_name() == patId ) {
					pPattern = pTmpPattern;
					break;
				}
			}
		}
		if ( pPattern == nullptr ) {
			if ( ! bSilent ) {
				WARNINGLOG( "patternid not found in patternSequence" );
			}
			pPatternIDNode = pPatternIDNode.nextSiblingElement( "patternID" );
			
			delete pPatternSequence;
			
			continue;
		}
		pPatternSequence->add( pPattern );

		pPatternGroupVector->push_back( pPatternSequence );

		pPatternIDNode = pPatternIDNode.nextSiblingElement( "patternID" );
	}

    XMLNode groupNode = patternSequenceNode.firstChildElement( "group" );
	while ( ! groupNode.isNull() ) {
		PatternList* patternSequence = new PatternList();
		XMLNode patternId = groupNode.firstChildElement( "patternID" );
		while ( ! patternId.isNull() ) {
			QString patId = patternId.firstChild().nodeValue();

			Pattern* pPattern = nullptr;
			for ( unsigned i = 0; i < pPatternList->size(); i++ ) {
				Pattern* pTmpPattern = pPatternList->get( i );
				if ( pTmpPattern != nullptr ) {
					if ( pTmpPattern->get_name() == patId ) {
						pPattern = pTmpPattern;
						break;
					}
				}
			}
			if ( pPattern == nullptr ) {
				if ( ! bSilent ) {
					WARNINGLOG( "patternid not found in patternSequence" );
				}
				patternId = patternId.nextSiblingElement( "patternID" );
				continue;
			}
			patternSequence->add( pPattern );
			patternId = patternId.nextSiblingElement( "patternID" );
		}
		pPatternGroupVector->push_back( patternSequence );

		groupNode = groupNode.nextSiblingElement( "group" );
	}

	pSong->setPatternGroupVector( pPatternGroupVector );

#ifdef H2CORE_HAVE_LADSPA
	// reset FX
	for ( int fx = 0; fx < MAX_FX; ++fx ) {
		//LadspaFX* pFX = Effects::get_instance()->getLadspaFX( fx );
		//delete pFX;
		Effects::get_instance()->setLadspaFX( nullptr, fx );
	}
#endif

	// LADSPA FX
	XMLNode ladspaNode = pRootNode->firstChildElement( "ladspa" );
	if ( ! ladspaNode.isNull() ) {
		int nFX = 0;
		XMLNode fxNode = ladspaNode.firstChildElement( "fx" );
		while ( ! fxNode.isNull() ) {
			QString sName = fxNode.read_string( "name", "", false, false );

			if ( sName != "no plugin" ) {
				// FIXME: il caricamento va fatto fare all'engine, solo lui sa il samplerate esatto
#ifdef H2CORE_HAVE_LADSPA
				LadspaFX* pFX = LadspaFX::load( fxNode.read_string( "filename", "", false, false ),
												sName, 44100 );
				Effects::get_instance()->setLadspaFX( pFX, nFX );
				if ( pFX != nullptr ) {
					pFX->setEnabled( fxNode.read_bool( "enabled", false, false, false ) );
					pFX->setVolume( fxNode.read_float( "volume", 1.0, false, false ) );
					XMLNode inputControlNode = fxNode.firstChildElement( "inputControlPort" );
					while ( ! inputControlNode.isNull() ) {
						QString sName = inputControlNode.read_string( "name", "", false, false );
						float fValue = inputControlNode.read_float( "value", 0.0, false, false );

						for ( unsigned nPort = 0; nPort < pFX->inputControlPorts.size(); nPort++ ) {
							LadspaControlPort* port = pFX->inputControlPorts[ nPort ];
							if ( QString( port->sName ) == sName ) {
								port->fControlValue = fValue;
							}
						}
						inputControlNode = inputControlNode.nextSiblingElement( "inputControlPort" );
					}
				}
#endif
			}
			nFX++;
			fxNode = fxNode.nextSiblingElement( "fx" );
		}
	} else if ( ! bSilent ) {
		WARNINGLOG( "'ladspa' node not found" );
	}

	std::shared_ptr<Timeline> pTimeline = std::make_shared<Timeline>();
	XMLNode bpmTimeLineNode = pRootNode->firstChildElement( "BPMTimeLine" );
	if ( ! bpmTimeLineNode.isNull() ) {
		XMLNode newBPMNode = bpmTimeLineNode.firstChildElement( "newBPM" );
		while ( ! newBPMNode.isNull() ) {
			pTimeline->addTempoMarker( newBPMNode.read_int( "BAR", 0, false, false ),
									   newBPMNode.read_float( "BPM", 120.0, false, false ) );
			newBPMNode = newBPMNode.nextSiblingElement( "newBPM" );
		}
	} else if ( ! bSilent ) {
		WARNINGLOG( "'BPMTimeLine' node not found" );
	}

	XMLNode timeLineTagNode = pRootNode->firstChildElement( "timeLineTag" );
	if ( ! timeLineTagNode.isNull() ) {
		XMLNode newTAGNode = timeLineTagNode.firstChildElement( "newTAG" );
		while ( ! newTAGNode.isNull() ) {
			pTimeline->addTag( newTAGNode.read_int( "BAR", 0, false, false ),
							   newTAGNode.read_string( "TAG", "", false, false ) );
			newTAGNode = newTAGNode.nextSiblingElement( "newTAG" );
		}
	} else if ( ! bSilent ) {
		WARNINGLOG( "TagTimeLine node not found" );
	}
	pSong->setTimeline( pTimeline );

	// Automation Paths
	XMLNode automationPathsNode = pRootNode->firstChildElement( "automationPaths" );
	if ( ! automationPathsNode.isNull() ) {
		AutomationPathSerializer pathSerializer;

		XMLNode pathNode = automationPathsNode.firstChildElement( "path" );
		while ( ! pathNode.isNull()) {
			QString sAdjust = pathNode.read_attribute( "adjust", "velocity", false, false );

			// Select automation path to be read based on "adjust" attribute
			AutomationPath *pPath = nullptr;
			if ( sAdjust == "velocity" ) {
				pPath = pSong->getVelocityAutomationPath();
			}

			if ( pPath ) {
				pathSerializer.read_automation_path( pathNode, *pPath );
			}

			pathNode = pathNode.nextSiblingElement( "path" );
		}
	}

	return pSong;
}

/// Save a song to file
bool Song::save( const QString& sFilename, bool bSilent )
{
	QFileInfo fi( sFilename );
	if ( ( Filesystem::file_exists( sFilename, true ) &&
		   ! Filesystem::file_writable( sFilename, true ) ) ||
		 ( ! Filesystem::file_exists( sFilename, true ) &&
		   ! Filesystem::dir_writable( fi.dir().absolutePath(), true ) ) ) {
		// In case a read-only file is loaded by Hydrogen. Beware:
		// .isWritable() will return false if the song does not exist.
		if ( ! bSilent ) {
			ERRORLOG( QString( "Unable to save song to [%1]. Path is not writable!" )
					  .arg( sFilename ) );
		}
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "Saving song to [%1]" ).arg( sFilename ) );
	}

	XMLDoc doc;
	
	// In order to comply with the GPL license we have to add a
	// license notice to the file.
	if ( getLicense().getType() == License::GPL ) {
		doc.appendChild( doc.createComment( License::getGPLLicenseNotice( getAuthor() ) ) );
	}
	
	XMLNode rootNode = doc.set_root( "song" );

	writeTo( &rootNode, bSilent );
	
	setFilename( sFilename );
	setIsModified( false );

	if ( ! doc.write( sFilename ) ) {
		if ( ! bSilent ) {
			ERRORLOG( QString( "Error writing song to [%1]" ).arg( sFilename ) );
		}
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG("Save was successful.");
	}

	return true;
}

void Song::writeTo( XMLNode* pRootNode, bool bSilent ) {
	pRootNode->write_string( "version", QString( get_version().c_str() ) );
	pRootNode->write_string( "bpm", QString("%1").arg( getBpm() ) );
	pRootNode->write_string( "volume", QString("%1").arg( getVolume() ) );
	pRootNode->write_string( "metronomeVolume", QString("%1").arg( getMetronomeVolume() ) );
	pRootNode->write_string( "name", getName() );
	pRootNode->write_string( "author", getAuthor() );
	pRootNode->write_string( "notes", getNotes() );
	pRootNode->write_string( "license", getLicense().getLicenseString() );
	pRootNode->write_bool( "loopEnabled", isLoopEnabled() );

	bool bPatternMode = static_cast<bool>(Song::PatternMode::Selected);
	if ( getPatternMode() == Song::PatternMode::Stacked ) {
		bPatternMode = static_cast<bool>(Song::PatternMode::Stacked);
	}
	pRootNode->write_bool( "patternModeMode", bPatternMode );
	
	pRootNode->write_string( "playbackTrackFilename", QString("%1").arg( getPlaybackTrackFilename() ) );
	pRootNode->write_bool( "playbackTrackEnabled", getPlaybackTrackEnabled() );
	pRootNode->write_string( "playbackTrackVolume", QString("%1").arg( getPlaybackTrackVolume() ) );
	pRootNode->write_string( "action_mode",
							 QString::number( static_cast<int>( getActionMode() ) ) );
	pRootNode->write_bool( "isPatternEditorLocked",
						   getIsPatternEditorLocked() );
	pRootNode->write_bool( "isTimelineActivated", getIsTimelineActivated() );
	
	if ( getMode() == Song::Mode::Song ) {
		pRootNode->write_string( "mode", QString( "song" ) );
	} else {
		pRootNode->write_string( "mode", QString( "pattern" ) );
	}

	Sampler* pSampler = Hydrogen::get_instance()->getAudioEngine()->getSampler();
	
	QString sPanLawType; // prepare the pan law string to write
	int nPanLawType = getPanLawType();
	if ( nPanLawType == Sampler::RATIO_STRAIGHT_POLYGONAL ) {
		sPanLawType = "RATIO_STRAIGHT_POLYGONAL";
	} else if ( nPanLawType == Sampler::RATIO_CONST_POWER ) {
		sPanLawType = "RATIO_CONST_POWER";
	} else if ( nPanLawType == Sampler::RATIO_CONST_SUM ) {
		sPanLawType = "RATIO_CONST_SUM";
	} else if ( nPanLawType == Sampler::LINEAR_STRAIGHT_POLYGONAL ) {
		sPanLawType = "LINEAR_STRAIGHT_POLYGONAL";
	} else if ( nPanLawType == Sampler::LINEAR_CONST_POWER ) {
		sPanLawType = "LINEAR_CONST_POWER";
	} else if ( nPanLawType == Sampler::LINEAR_CONST_SUM ) {
		sPanLawType = "LINEAR_CONST_SUM";
	} else if ( nPanLawType == Sampler::POLAR_STRAIGHT_POLYGONAL ) {
		sPanLawType = "POLAR_STRAIGHT_POLYGONAL";
	} else if ( nPanLawType == Sampler::POLAR_CONST_POWER ) {
		sPanLawType = "POLAR_CONST_POWER";
	} else if ( nPanLawType == Sampler::POLAR_CONST_SUM ) {
		sPanLawType = "POLAR_CONST_SUM";
	} else if ( nPanLawType == Sampler::QUADRATIC_STRAIGHT_POLYGONAL ) {
		sPanLawType = "QUADRATIC_STRAIGHT_POLYGONAL";
	} else if ( nPanLawType == Sampler::QUADRATIC_CONST_POWER ) {
		sPanLawType = "QUADRATIC_CONST_POWER";
	} else if ( nPanLawType == Sampler::QUADRATIC_CONST_SUM ) {
		sPanLawType = "QUADRATIC_CONST_SUM";
	} else if ( nPanLawType == Sampler::LINEAR_CONST_K_NORM ) {
		sPanLawType = "LINEAR_CONST_K_NORM";
	} else if ( nPanLawType == Sampler::POLAR_CONST_K_NORM ) {
		sPanLawType = "POLAR_CONST_K_NORM";
	} else if ( nPanLawType == Sampler::RATIO_CONST_K_NORM ) {
		sPanLawType = "RATIO_CONST_K_NORM";
	} else if ( nPanLawType == Sampler::QUADRATIC_CONST_K_NORM ) {
		sPanLawType = "QUADRATIC_CONST_K_NORM";
	} else {
		if ( ! bSilent ) {
			WARNINGLOG( "Unknown pan law in saving song. Saved default type." );
		}
		sPanLawType = "RATIO_STRAIGHT_POLYGONAL";
	}
	// write the pan law string in file
	pRootNode->write_string( "pan_law_type", sPanLawType );
	pRootNode->write_string( "pan_law_k_norm", QString("%1").arg( getPanLawKNorm() ) );

	pRootNode->write_string( "humanize_time", QString("%1").arg( getHumanizeTimeValue() ) );
	pRootNode->write_string( "humanize_velocity", QString("%1").arg( getHumanizeVelocityValue() ) );
	pRootNode->write_string( "swing_factor", QString("%1").arg( getSwingFactor() ) );

	// component List
	XMLNode componentListNode = pRootNode->createNode( "componentList" );
	for ( const auto& pComponent : *m_pComponents ) {
		XMLNode componentNode = componentListNode.createNode( "drumkitComponent" );

		componentNode.write_string( "id", QString("%1").arg( pComponent->get_id() ) );
		componentNode.write_string( "name", pComponent->get_name() );
		componentNode.write_string( "volume", QString("%1").arg( pComponent->get_volume() ) );
	}

	// instrument list
	XMLNode instrumentListNode = pRootNode->createNode( "instrumentList" );
	unsigned nInstrument = getInstrumentList()->size();

	// INSTRUMENT NODE
	for ( unsigned i = 0; i < nInstrument; i++ ) {
		auto  pInstr = getInstrumentList()->get( i );
		assert( pInstr );

		XMLNode instrumentNode = instrumentListNode.createNode( "instrument" );

		instrumentNode.write_string( "id", QString("%1").arg( pInstr->get_id() ) );
		instrumentNode.write_string( "name", pInstr->get_name() );
		instrumentNode.write_string( "drumkit", pInstr->get_drumkit_name() );
		instrumentNode.write_string( "drumkitLookup", QString::number(static_cast<int>( pInstr->get_drumkit_lookup() )) );
		instrumentNode.write_string( "volume", QString("%1").arg( pInstr->get_volume() ) );
		instrumentNode.write_bool( "isMuted", pInstr->is_muted() );
		instrumentNode.write_bool( "isSoloed", pInstr->is_soloed() );
		instrumentNode.write_string( "pan", QString("%1").arg( pInstr->getPan() ) );
		instrumentNode.write_string( "gain", QString("%1").arg( pInstr->get_gain() ) );
		instrumentNode.write_bool( "applyVelocity", pInstr->get_apply_velocity() );

		instrumentNode.write_bool( "filterActive", pInstr->is_filter_active() );
		instrumentNode.write_string( "filterCutoff", QString("%1").arg( pInstr->get_filter_cutoff() ) );
		instrumentNode.write_string( "filterResonance", QString("%1").arg( pInstr->get_filter_resonance() ) );

		instrumentNode.write_string( "FX1Level", QString("%1").arg( pInstr->get_fx_level( 0 ) ) );
		instrumentNode.write_string( "FX2Level", QString("%1").arg( pInstr->get_fx_level( 1 ) ) );
		instrumentNode.write_string( "FX3Level", QString("%1").arg( pInstr->get_fx_level( 2 ) ) );
		instrumentNode.write_string( "FX4Level", QString("%1").arg( pInstr->get_fx_level( 3 ) ) );

		assert( pInstr->get_adsr() );
		instrumentNode.write_string( "Attack", QString("%1").arg( pInstr->get_adsr()->get_attack() ) );
		instrumentNode.write_string( "Decay", QString("%1").arg( pInstr->get_adsr()->get_decay() ) );
		instrumentNode.write_string( "Sustain", QString("%1").arg( pInstr->get_adsr()->get_sustain() ) );
		instrumentNode.write_string( "Release", QString("%1").arg( pInstr->get_adsr()->get_release() ) );
		instrumentNode.write_string( "pitchOffset", QString("%1").arg( pInstr->get_pitch_offset() ) );
		instrumentNode.write_string( "randomPitchFactor", QString("%1").arg( pInstr->get_random_pitch_factor() ) );

		instrumentNode.write_string( "muteGroup", QString("%1").arg( pInstr->get_mute_group() ) );
		instrumentNode.write_bool( "isStopNote", pInstr->is_stop_notes() );
		switch ( pInstr->sample_selection_alg() ) {
			case Instrument::VELOCITY:
				instrumentNode.write_string( "sampleSelectionAlgo", "VELOCITY" );
				break;
			case Instrument::RANDOM:
				instrumentNode.write_string( "sampleSelectionAlgo", "RANDOM" );
				break;
			case Instrument::ROUND_ROBIN:
				instrumentNode.write_string( "sampleSelectionAlgo", "ROUND_ROBIN" );
				break;
		}

		instrumentNode.write_string( "midiOutChannel", QString("%1").arg( pInstr->get_midi_out_channel() ) );
		instrumentNode.write_string( "midiOutNote", QString("%1").arg( pInstr->get_midi_out_note() ) );
		instrumentNode.write_string( "isHihat", QString("%1").arg( pInstr->get_hihat_grp() ) );
		instrumentNode.write_string( "lower_cc", QString("%1").arg( pInstr->get_lower_cc() ) );
		instrumentNode.write_string( "higher_cc", QString("%1").arg( pInstr->get_higher_cc() ) );

		for ( const auto& pComponent : *pInstr->get_components() ) {

			XMLNode componentNode = instrumentNode.createNode( "instrumentComponent" );

			componentNode.write_string( "component_id", QString("%1").arg( pComponent->get_drumkit_componentID() ) );
			componentNode.write_string( "gain", QString("%1").arg( pComponent->get_gain() ) );

			for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
				auto pLayer = pComponent->get_layer( nLayer );
				if ( pLayer == nullptr ) {
					continue;
				}
				
				auto pSample = pLayer->get_sample();
				if ( pSample == nullptr ) {
					continue;
				}

				bool sIsModified = pSample->get_is_modified();
				Sample::Loops lo = pSample->get_loops();
				Sample::Rubberband ro = pSample->get_rubberband();
				QString sMode = pSample->get_loop_mode_string();

				XMLNode layerNode = componentNode.createNode( "layer" );
				layerNode.write_string( "filename", Filesystem::prepare_sample_path( pSample->get_filepath() ) );
				layerNode.write_bool( "ismodified", sIsModified);
				layerNode.write_string( "smode", pSample->get_loop_mode_string() );
				layerNode.write_string( "startframe", QString("%1").arg( lo.start_frame ) );
				layerNode.write_string( "loopframe", QString("%1").arg( lo.loop_frame ) );
				layerNode.write_string( "loops", QString("%1").arg( lo.count ) );
				layerNode.write_string( "endframe", QString("%1").arg( lo.end_frame ) );
				layerNode.write_string( "userubber", QString("%1").arg( ro.use ) );
				layerNode.write_string( "rubberdivider", QString("%1").arg( ro.divider ) );
				layerNode.write_string( "rubberCsettings", QString("%1").arg( ro.c_settings ) );
				layerNode.write_string( "rubberPitch", QString("%1").arg( ro.pitch ) );
				layerNode.write_string( "min", QString("%1").arg( pLayer->get_start_velocity() ) );
				layerNode.write_string( "max", QString("%1").arg( pLayer->get_end_velocity() ) );
				layerNode.write_string( "gain", QString("%1").arg( pLayer->get_gain() ) );
				layerNode.write_string( "pitch", QString("%1").arg( pLayer->get_pitch() ) );

				Sample::VelocityEnvelope* velocity = pSample->get_velocity_envelope();
				for (int y = 0; y < velocity->size(); y++){
					XMLNode volumeNode = layerNode.createNode( "volume" );
					volumeNode.write_string( "volume-position", QString("%1").arg( velocity->at(y).frame ) );
					volumeNode.write_string( "volume-value", QString("%1").arg( velocity->at(y).value ) );
				}

				Sample::PanEnvelope* pan = pSample->get_pan_envelope();
				for (int y = 0; y < pan->size(); y++){
					XMLNode panNode = layerNode.createNode( "pan" );
					panNode.write_string( "pan-position", QString("%1").arg( pan->at(y).frame ) );
					panNode.write_string( "pan-value", QString("%1").arg( pan->at(y).value ) );
				}
			}
		}
	}

	// pattern list
	XMLNode patternListNode = pRootNode->createNode( "patternList" );

	for ( const auto& pPattern : *m_pPatternList ) {
		// pattern
		XMLNode patternNode = patternListNode.createNode( "pattern" );
		patternNode.write_string( "name", pPattern->get_name() );
		patternNode.write_string( "category", pPattern->get_category() );
		patternNode.write_string( "size", QString("%1").arg( pPattern->get_length() ) );
		patternNode.write_string( "denominator", QString("%1").arg( pPattern->get_denominator() ) );
		patternNode.write_string( "info", pPattern->get_info() );

		XMLNode noteListNode = patternNode.createNode( "noteList" );
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );

			XMLNode noteNode = noteListNode.createNode( "note" );
			noteNode.write_string( "position", QString("%1").arg( pNote->get_position() ) );
			noteNode.write_string( "leadlag", QString("%1").arg( pNote->get_lead_lag() ) );
			noteNode.write_string( "velocity", QString("%1").arg( pNote->get_velocity() ) );
			noteNode.write_string( "pan", QString("%1").arg( pNote->getPan() ) );
			noteNode.write_string( "pitch", QString("%1").arg( pNote->get_pitch() ) );
			noteNode.write_string( "probability", QString("%1").arg( pNote->get_probability() ) );

			noteNode.write_string( "key", pNote->key_to_string() );

			noteNode.write_string( "length", QString("%1").arg( pNote->get_length() ) );
			noteNode.write_string( "instrument", QString("%1").arg( pNote->get_instrument()->get_id() ) );

			QString noteoff = "false";
			if ( pNote->get_note_off() ) {
				noteoff = "true";
			}
			noteNode.write_string( "note_off", noteoff );
		}
	}

	XMLNode virtualPatternListNode = pRootNode->createNode( "virtualPatternList" );
	for ( const auto& pPattern : *m_pPatternList ) {
		// pattern
		if ( ! pPattern->get_virtual_patterns()->empty() ) {
			XMLNode patternNode = virtualPatternListNode.createNode( "pattern" );
			patternNode.write_string( "name", pPattern->get_name() );

			for ( const auto& pVirtualPattern : *( pPattern->get_virtual_patterns() ) ) {
				patternNode.write_string( "virtual", pVirtualPattern->get_name() );
			}
		}
	}

	// pattern sequence
	XMLNode patternSequenceNode = pRootNode->createNode( "patternSequence" );

	unsigned nPatternGroups = getPatternGroupVector()->size();
	for ( unsigned i = 0; i < nPatternGroups; i++ ) {
		XMLNode groupNode = patternSequenceNode.createNode( "group" );

		for ( const auto& pPattern : *(m_pPatternGroupSequence->at( i )) ) {
			groupNode.write_string( "patternID", pPattern->get_name() );
		}
	}

	// LADSPA FX
	XMLNode ladspaFxNode = pRootNode->createNode( "ladspa" );

	for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
		XMLNode fxNode = ladspaFxNode.createNode( "fx" );

#ifdef H2CORE_HAVE_LADSPA
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX != nullptr ) {
			fxNode.write_string( "name", pFX->getPluginLabel() );
			fxNode.write_string( "filename", pFX->getLibraryPath() );
			fxNode.write_bool( "enabled", pFX->isEnabled() );
			fxNode.write_string( "volume", QString("%1").arg( pFX->getVolume() ) );
			
			for ( unsigned nControl = 0; nControl < pFX->inputControlPorts.size(); nControl++ ) {
				LadspaControlPort *pControlPort = pFX->inputControlPorts[ nControl ];
				XMLNode controlPortNode = fxNode.createNode( "inputControlPort" );
				controlPortNode.write_string( "name", pControlPort->sName );
				controlPortNode.write_string( "value", QString("%1").arg( pControlPort->fControlValue ) );
			}
			for ( unsigned nControl = 0; nControl < pFX->outputControlPorts.size(); nControl++ ) {
				LadspaControlPort *pControlPort = pFX->inputControlPorts[ nControl ];
				XMLNode controlPortNode = fxNode.createNode( "outputControlPort" );
				controlPortNode.write_string( "name", pControlPort->sName );
				controlPortNode.write_string( "value", QString("%1").arg( pControlPort->fControlValue ) );
			}
		}
#else
		if ( false ) {
		}
#endif
		else {
			fxNode.write_string( "name", QString( "no plugin" ) );
			fxNode.write_string( "filename", QString( "-" ) );
			fxNode.write_bool( "enabled", false );
			fxNode.write_string( "volume", "0.0" );
		}
	}

	//bpm time line
	auto pTimeline = Hydrogen::get_instance()->getTimeline();

	XMLNode bpmTimeLineNode = pRootNode->createNode( "BPMTimeLine" );

	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	
	if ( tempoMarkerVector.size() >= 1 ){
		for ( int tt = 0; tt < static_cast<int>(tempoMarkerVector.size()); tt++){
			if ( tt == 0 && pTimeline->isFirstTempoMarkerSpecial() ) {
				continue;
			}
			XMLNode newBPMNode = bpmTimeLineNode.createNode( "newBPM" );
			newBPMNode.write_string( "BAR",QString("%1").arg( tempoMarkerVector[tt]->nColumn ));
			newBPMNode.write_string( "BPM", QString("%1").arg( tempoMarkerVector[tt]->fBpm  ) );
		}
	}

	//time line tag
	XMLNode timeLineTagNode = pRootNode->createNode( "timeLineTag" );

	auto tagVector = pTimeline->getAllTags();
	
	if ( tagVector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(tagVector.size()); t++){
			XMLNode newTAGNode = timeLineTagNode.createNode( "newTAG" );
			newTAGNode.write_string( "BAR",QString("%1").arg( tagVector[t]->nColumn ));
			newTAGNode.write_string( "TAG", QString("%1").arg( tagVector[t]->sTag ) );
		}
	}

	// Automation Paths
	XMLNode automationPathsNode = pRootNode->createNode( "automationPaths" );
	AutomationPath *pPath = getVelocityAutomationPath();
	if ( pPath != nullptr ) {
		XMLNode pathNode = automationPathsNode.createNode( "path" );
		pathNode.write_attribute("adjust", "velocity");

		AutomationPathSerializer serializer;
		serializer.write_automation_path(pathNode, *pPath);
	}
}

std::shared_ptr<Song> Song::getEmptySong()
{
	std::shared_ptr<Song> pSong =
		std::make_shared<Song>( Filesystem::untitled_song_name(), "hydrogen",
								120, 0.5 );

	pSong->setMetronomeVolume( 0.5 );
	pSong->setNotes( "..." );
	pSong->setLicense( License() );
	pSong->setLoopMode( Song::LoopMode::Disabled );
	pSong->setMode( Song::Mode::Pattern );
	pSong->setHumanizeTimeValue( 0.0 );
	pSong->setHumanizeVelocityValue( 0.0 );
	pSong->setSwingFactor( 0.0 );

	InstrumentList* pInstrList = new InstrumentList();
	auto pNewInstr = std::make_shared<Instrument>( EMPTY_INSTR_ID, "New instrument" );
	pInstrList->add( pNewInstr );
	pSong->setInstrumentList( pInstrList );

	PatternList*	pPatternList = new PatternList();
	PatternList*    patternSequence = new PatternList();

	for ( int nn = 0; nn < 10; ++nn ) {
		Pattern*		pEmptyPattern = new Pattern();
	
		pEmptyPattern->set_name( QString( "Pattern %1" ).arg( nn + 1 ) );
		pEmptyPattern->set_category( QString( "not_categorized" ) );
		pPatternList->add( pEmptyPattern );

		if ( nn == 0 ) {
			// Only the first pattern will be activated in the
			// SongEditor.
			patternSequence->add( pEmptyPattern );
		}
	}
	pSong->setPatternList( pPatternList );
	
	std::vector<PatternList*>* pPatternGroupVector = new std::vector<PatternList*>;
	pPatternGroupVector->push_back( patternSequence );
	pSong->setPatternGroupVector( pPatternGroupVector );

	pSong->setFilename( Filesystem::empty_song_path() );

	auto pDrumkit = H2Core::Drumkit::load_by_name( Filesystem::drumkit_default_kit(), true );
	if ( pDrumkit == nullptr ) {
		ERRORLOG( QString( "Unabled to load default Drumkit [%1]" )
				  .arg( Filesystem::drumkit_default_kit() ) );
	} else {
		pSong->loadDrumkit( pDrumkit, true );
		delete pDrumkit;
	}
	
	pSong->setIsModified( false );

	return pSong;

}

DrumkitComponent* Song::getComponent( int nID ) const
{
	for (std::vector<DrumkitComponent*>::iterator it = m_pComponents->begin() ; it != m_pComponents->end(); ++it) {
		if( (*it)->get_id() == nID ) {
			return *it;
		}
	}

	return nullptr;
}


void Song::setSwingFactor( float factor )
{
	if ( factor < 0.0 ) {
		factor = 0.0;
	} else if ( factor > 1.0 ) {
		factor = 1.0;
	}

	m_fSwingFactor = factor;
}

void Song::setIsModified( bool bIsModified )
{
	bool Notify = false;

	if( m_bIsModified != bIsModified ) {
		Notify = true;
	}

	m_bIsModified = bIsModified;

	if( Notify ) {
		EventQueue::get_instance()->push_event( EVENT_SONG_MODIFIED, -1 );

		if ( Hydrogen::get_instance()->isUnderSessionManagement() ) {
			// If Hydrogen is under session management (NSM), tell the
			// NSM server that the Song was modified.
#ifdef H2CORE_HAVE_OSC
			NsmClient::get_instance()->sendDirtyState( bIsModified );
#endif
		}
	}
	
}

bool Song::hasMissingSamples() const
{
	InstrumentList *pInstrumentList = getInstrumentList();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		if ( pInstrumentList->get( i )->has_missing_samples() ) {
			return true;
		}
	}
	return false;
}

void Song::clearMissingSamples() {
	InstrumentList *pInstrumentList = getInstrumentList();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		pInstrumentList->get( i )->set_missing_samples( false );
	}
}

void Song::readTempPatternList( const QString& sFilename )
{
	XMLDoc doc;
	if( !doc.read( sFilename ) ) {
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
			QString patternName = virtualNode.read_attribute( "pattern", nullptr, true, false );
			XMLNode patternNode = virtualNode.firstChildElement( "pattern" );
			Pattern* pPattern = nullptr;
			while ( !patternName.isEmpty() && !patternNode.isNull() ) {
				QString virtualName = patternNode.read_text( false );
				if ( !virtualName.isEmpty() ) {
					Pattern* pVirtualPattern = nullptr;
					for ( unsigned i = 0; i < getPatternList()->size(); i++ ) {
						Pattern* pat = getPatternList()->get( i );
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

	getPatternList()->flattened_virtual_patterns_compute();
	getPatternGroupVector()->clear();

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
					for ( unsigned i = 0; i < getPatternList()->size(); i++ ) {
						Pattern* pat = getPatternList()->get( i );
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
			getPatternGroupVector()->push_back( patternSequence );
			groupNode = groupNode.nextSiblingElement( "group" );
		}
	} else {
		WARNINGLOG( "no sequence node not found" );
	}
}

bool Song::writeTempPatternList( const QString& sFilename )
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "sequence" );

	XMLNode virtualPatternListNode = root.createNode( "virtuals" );
	for ( unsigned i = 0; i < getPatternList()->size(); i++ ) {
		Pattern *pPattern = getPatternList()->get( i );
		if ( !pPattern->get_virtual_patterns()->empty() ) {
			XMLNode node = virtualPatternListNode.createNode( "virtual" );
			node.write_attribute( "pattern", pPattern->get_name() );
			for ( Pattern::virtual_patterns_it_t virtIter = pPattern->get_virtual_patterns()->begin(); virtIter != pPattern->get_virtual_patterns()->end(); ++virtIter ) {
				node.write_string( "pattern", (*virtIter)->get_name() );
			}
		}
	}

	XMLNode patternSequenceNode = root.createNode( "groups" );
	for ( unsigned i = 0; i < getPatternGroupVector()->size(); i++ ) {
		XMLNode node = patternSequenceNode.createNode( "group" );
		PatternList *pList = ( *getPatternGroupVector() )[i];
		for ( unsigned j = 0; j < pList->size(); j++ ) {
			Pattern *pPattern = pList->get( j );
			node.write_string( "pattern", pPattern->get_name() );
		}
	}

	return doc.write( sFilename );
}

QString Song::copyInstrumentLineToString( int nSelectedPattern, int nSelectedInstrument )
{
	auto pInstr = getInstrumentList()->get( nSelectedInstrument );
	assert( pInstr );

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( "instrument_line" );
	rootNode.write_string( "author", getAuthor() );
	rootNode.write_string( "license", getLicense().getLicenseString() );

	XMLNode patternListNode = rootNode.createNode( "patternList" );

	unsigned nPatterns = getPatternList()->size();
	for ( unsigned i = 0; i < nPatterns; i++ )
	{
		if (( nSelectedPattern >= 0 ) && ( nSelectedPattern != i ) ) {
			continue;
		}

		// Export pattern
		Pattern *pPattern = getPatternList()->get( i );
		if ( pPattern == nullptr ) {
			continue;
		}

		XMLNode patternNode = patternListNode.createNode( "pattern" );
		patternNode.write_string( "pattern_name", pPattern->get_name() );

		QString category;
		if ( pPattern->get_category().isEmpty() ) {
			category = "No category";
		} else {
			category = pPattern->get_category();
		}

		patternNode.write_string( "info", pPattern->get_info() );
		patternNode.write_string( "category", category  );
		patternNode.write_string( "size", QString("%1").arg( pPattern->get_length() ) );
		patternNode.write_string( "denominator", QString("%1").arg( pPattern->get_denominator() ) );
		XMLNode noteListNode = patternNode.createNode( "noteList" );
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it)
		{
			Note *pNote = it->second;
			if ( pNote == nullptr ) {
				ERRORLOG( QString( "Unable to handle note [%1] of pattern [%2]"  )
						  .arg( it->first ).arg( pPattern->get_name() ) );
				continue;
			}

			// Export only specified instrument
			if (pNote->get_instrument() == pInstr)
			{
				XMLNode noteNode = noteListNode.createNode( "note" );
				pNote->save_to( &noteNode );
			}
		}
	}

	// Serialize document & return
	return doc.toString();
}

bool Song::pasteInstrumentLineFromString( const QString& sSerialized, int nSelectedPattern, int nSelectedInstrument, std::list<Pattern *>& pPatterns )
{
	XMLDoc doc;
	if ( ! doc.setContent( sSerialized ) ) {
		return false;
	}

	// Get current instrument
	auto pInstr = getInstrumentList()->get( nSelectedInstrument );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to find instrument [%1]" )
				  .arg( nSelectedInstrument ) );
		return false;
	}

	// Get pattern list
	PatternList *pList = getPatternList();
	Pattern *pSelected = ( nSelectedPattern >= 0 ) ? pList->get( nSelectedPattern ) : nullptr;
	XMLNode patternNode;
	bool bIsNoteSelection = false;
	bool is_single = true;

	// Check if document has correct structure
	XMLNode rootNode = doc.firstChildElement( "instrument_line" );	// root element
	if ( ! rootNode.isNull() ) {
		// Find pattern list
		XMLNode patternList = rootNode.firstChildElement( "patternList" );
		if ( patternList.isNull() ) {
			return false;
		}

		// Parse each pattern if needed
		patternNode = patternList.firstChildElement( "pattern" );
		if ( ! patternNode.isNull() ) {
			is_single = (( XMLNode )patternNode.nextSiblingElement( "pattern" )).isNull();
		}
	}
	else {
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

	while ( ! patternNode.isNull() )
	{
		QString patternName( patternNode.read_string( "pattern_name", "", false, false ) );

		// Check if pattern name specified
		if ( patternName.length() > 0 || bIsNoteSelection ) {
			// Try to find pattern by name
			Pattern* pat = pList->find(patternName);

			// If OK - check if need to add this pattern to result
			// If there is only one pattern, we always add it to list
			// If there is no selected pattern, we add all existing patterns to list (match by name)
			// Otherwise we add only existing selected pattern to list (match by name)
			if ( is_single ||
				 ( pat != nullptr &&
				   ( nSelectedPattern < 0 || pat == pSelected ) ) ) {
				
				// Change name of pattern to selected pattern
				if ( pSelected != nullptr ) {
					patternName = pSelected->get_name();
				}

				pat = new Pattern( patternName,
								   patternNode.read_string( "info", "", true, false ),
								   patternNode.read_string( "category", "", true, false ),
								   patternNode.read_int( "size", -1, true, false ) );

				// Parse pattern data
				XMLNode pNoteListNode = patternNode.firstChildElement( "noteList" );
				if ( ! pNoteListNode.isNull() )
				{
					// Parse note-by-note
					XMLNode noteNode = pNoteListNode.firstChildElement( "note" );
					while ( ! noteNode.isNull() )
					{
						XMLNode instrument = noteNode.firstChildElement( "instrument" );
						XMLNode instrumentText = instrument.firstChild();

						instrumentText.setNodeValue( QString::number( pInstr->get_id() ) );
						Note *pNote = Note::load_from( &noteNode, getInstrumentList() );

						pat->insert_note( pNote ); // Add note to created pattern

						noteNode = noteNode.nextSiblingElement( "note" );
					}
				}

				// Add loaded pattern to apply-list
				pPatterns.push_back(pat);
			}
		}

		patternNode = patternNode.nextSiblingElement( "pattern" );
	}

	return true;
}


void Song::setPanLawKNorm( float fKNorm ) {
	if ( fKNorm >= 0. ) {
		m_fPanLawKNorm = fKNorm;
	} else {
		WARNINGLOG("negative kNorm. Set default" );
		m_fPanLawKNorm = Sampler::K_NORM_DEFAULT;
	}
}

void Song::loadDrumkit( Drumkit *pDrumkit, bool bConditional ) {
	assert ( pDrumkit );
	auto pAudioEngine = Hydrogen::get_instance()->getAudioEngine();

	m_sCurrentDrumkitName = pDrumkit->get_name();
	if ( pDrumkit->isUserDrumkit() ) {
		m_currentDrumkitLookup = Filesystem::Lookup::user;
	} else {
		m_currentDrumkitLookup = Filesystem::Lookup::system;
	}


	// Load DrumkitComponents 
	std::vector<DrumkitComponent*>* pDrumkitCompoList = pDrumkit->get_components();
	
	for( auto &pComponent : *m_pComponents ){
		delete pComponent;
	}
	m_pComponents->clear();
	
	for (std::vector<DrumkitComponent*>::iterator it = pDrumkitCompoList->begin() ; it != pDrumkitCompoList->end(); ++it) {
		DrumkitComponent* pSrcComponent = *it;
		DrumkitComponent* pNewComponent = new DrumkitComponent( pSrcComponent->get_id(), pSrcComponent->get_name() );
		pNewComponent->load_from( pSrcComponent );

		m_pComponents->push_back( pNewComponent );
	}

	//////
	// Load InstrumentList
	/*
	 * If the old drumkit is bigger then the new drumkit,
	 * delete all instruments with a bigger pos then
	 * pDrumkitInstrList->size(). Otherwise the instruments
	 * from our old instrumentlist with
	 * pos > pDrumkitInstrList->size() stay in the
	 * new instrumentlist
	 */
	InstrumentList *pDrumkitInstrList = pDrumkit->get_instruments();
	
	int nInstrumentDiff = m_pInstrumentList->size() - pDrumkitInstrList->size();
	int nMaxID = -1;
	
	std::shared_ptr<Instrument> pInstr, pNewInstr;
	for ( int nnInstr = 0; nnInstr < pDrumkitInstrList->size(); ++nnInstr ) {
		if ( nnInstr < m_pInstrumentList->size() ) {
			// Instrument exists already
			pInstr = m_pInstrumentList->get( nnInstr );
			assert( pInstr );
		} else {
			pInstr = std::make_shared<Instrument>();
			m_pInstrumentList->add( pInstr );
		}

		pNewInstr = pDrumkitInstrList->get( nnInstr );
		assert( pNewInstr );
		INFOLOG( QString( "Loading instrument (%1 of %2) [%3]" )
				 .arg( nnInstr + 1 )
				 .arg( pDrumkitInstrList->size() )
				 .arg( pNewInstr->get_name() ) );

		// Preserve instrument IDs. Where the new drumkit has more
		// instruments than the song does, new instruments need new
		// ids.
		int nID = pInstr->get_id();
		if ( nID == EMPTY_INSTR_ID ) {
			nID = nMaxID + 1;
		}
		nMaxID = std::max( nID, nMaxID );

		pInstr->load_from( pDrumkit, pNewInstr );
		pInstr->set_id( nID );
	}

	// Discard redundant instruments (in case the last drumkit had
	// more instruments than the new one).
	if ( nInstrumentDiff >= 0 ) {
		for ( int i = 0; i < nInstrumentDiff ; i++ ){
			removeInstrument( m_pInstrumentList->size() - 1,
							  bConditional );
		}
	}
}

void Song::removeInstrument( int nInstrumentNumber, bool bConditional ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pInstr = m_pInstrumentList->get( nInstrumentNumber );
	if ( pInstr == nullptr ) {
		// Error log is already printed by get().
		return;
	}

	if ( bConditional ) {
		// If a note was assigned to this instrument in any pattern,
		// the instrument will be kept instead of discarded.
		for ( const auto& pPattern : *m_pPatternList ) {
			if ( pPattern->references( pInstr ) ) {
				DEBUGLOG("Keeping instrument #" + QString::number( nInstrumentNumber ) );
				return;
			}
		}
	} else {
		for ( const auto& pPattern : *m_pPatternList ) {
			pPattern->purge_instrument( pInstr, false );
		}
	}

	// In case there is just this one instrument left, reset it
	// instead of removing it.
	if ( m_pInstrumentList->size() == 1 ){
		pInstr->set_name( (QString( "Instrument 1" )) );
		for ( auto& pCompo : *pInstr->get_components() ) {
			// remove all layers
			for ( int nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
				pCompo->set_layer( nullptr, nLayer );
			}
		}
		DEBUGLOG("clear last instrument to empty instrument 1 instead delete the last instrument");
		return;
	}
	
	// delete the instrument from the instruments list
	m_pInstrumentList->del( nInstrumentNumber );

	// At this point the instrument has been removed from both the
	// instrument list and every pattern in the song.  Hence there's no way
	// (NOTE) to play on that instrument, and once all notes have stopped
	// playing it will be save to delete.
	// the ugly name is just for debugging...
	QString xxx_name = QString( "XXX_%1" ).arg( pInstr->get_name() );
	pInstr->set_name( xxx_name );
	pHydrogen->addInstrumentToDeathRow( pInstr );
}

std::vector<std::shared_ptr<Note>> Song::getAllNotes() const {

	std::vector<std::shared_ptr<Note>> notes;

	long nColumnStartTick = 0;
	for ( int ii = 0; ii < m_pPatternGroupSequence->size(); ++ii ) {

		auto pColumn = (*m_pPatternGroupSequence)[ ii ];
		
		if ( pColumn->size() == 0 ) {
			// An empty column with no patterns selected (but not the
			// end of the song).
			nColumnStartTick += MAX_NOTES;
			continue;
		} else {

			pColumn->longest_pattern_length();
			for ( const auto& ppattern : *pColumn ) {
				if ( ppattern != nullptr ) {
					FOREACH_NOTE_CST_IT_BEGIN_END( ppattern->get_notes(), it ) {
						if ( it->second != nullptr ) {
							// Use the copy constructor to not mess
							// with the song itself.
							std::shared_ptr<Note> pNote =
								std::make_shared<Note>( it->second );

							// The position property of the note
							// specifies its position within the
							// pattern. All we need to do is to add
							// the pattern start tick.
							pNote->set_position( pNote->get_position() +
												 nColumnStartTick );
							notes.push_back( pNote );
						}
					}
				}
			}

			nColumnStartTick += pColumn->longest_pattern_length();
		}
	}
	
	return notes;
}

int Song::findExistingComponent( const QString& sComponentName ) const {
	for ( const auto& ppComponent : *m_pComponents ) {
		if ( ppComponent->get_name().compare( sComponentName ) == 0 ){
			return ppComponent->get_id();
		}
	}
	return -1;
}

int Song::findFreeComponentID( int nStartingID ) const {

	bool bFreeID = true;
	
	for ( const auto& ppComponent : *m_pComponents ) {
		if ( ppComponent->get_id() == nStartingID ) {
			bFreeID = false;
			break;
		}
	}

	if ( bFreeID ) {
		return nStartingID;
	}
	else {
		return findFreeComponentID( nStartingID + 1 );
	}
}

QString Song::makeComponentNameUnique( const QString& sName ) const {
	for ( const auto& ppComponent : *m_pComponents ) {
		if ( ppComponent->get_name().compare( sName ) == 0 ){
			return makeComponentNameUnique( sName + "_new" );
		}
	}
	return sName;
}
 
QString Song::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Song]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_bIsTimelineActivated: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bIsTimelineActivated ) )
			.append( QString( "%1%2m_bIsMuted: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bIsMuted ) )
			.append( QString( "%1%2m_resolution: %3\n" ).arg( sPrefix ).arg( s ).arg( m_resolution ) )
			.append( QString( "%1%2m_fBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fBpm ) )
			.append( QString( "%1%2m_sName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sName ) )
			.append( QString( "%1%2m_sAuthor: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sAuthor ) )
			.append( QString( "%1%2m_fVolume: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fVolume ) )
			.append( QString( "%1%2m_fMetronomeVolume: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fMetronomeVolume ) )
			.append( QString( "%1%2m_sNotes: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sNotes ) )
			.append( QString( "%1" ).arg( m_pPatternList->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_pPatternGroupSequence:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto pp : *m_pPatternGroupSequence ) {
			if ( pp != nullptr ) {
				sOutput.append( QString( "%1" ).arg( pp->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		sOutput.append( QString( "%1" ).arg( m_pInstrumentList->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_pComponents:\n" ).arg( sPrefix ).arg( s ) );
		for ( auto cc : *m_pComponents ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( "%1" ).arg( cc->toQString( sPrefix + s + s ) ) );
			}
		}
		sOutput.append( QString( "%1%2m_sFilename: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sFilename ) )
			.append( QString( "%1%2m_loopMode: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>(m_loopMode) ) )
			.append( QString( "%1%2m_fHumanizeTimeValue: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fHumanizeTimeValue ) )
			.append( QString( "%1%2m_fHumanizeVelocityValue: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fHumanizeVelocityValue ) )
			.append( QString( "%1%2m_fSwingFactor: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fSwingFactor ) )
			.append( QString( "%1%2m_bIsModified: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bIsModified ) )
			.append( QString( "%1%2m_latestRoundRobins\n" ).arg( sPrefix ).arg( s ) );
		for ( auto mm : m_latestRoundRobins ) {
			sOutput.append( QString( "%1%2%3 : %4\n" ).arg( sPrefix ).arg( s ).arg( mm.first ).arg( mm.second ) );
		}
		sOutput.append( QString( "%1%2m_songMode: %3\n" ).arg( sPrefix ).arg( s )
						.arg( static_cast<int>(m_mode )) )
			.append( QString( "%1%2m_sPlaybackTrackFilename: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sPlaybackTrackFilename ) )
			.append( QString( "%1%2m_bPlaybackTrackEnabled: %3\n" ).arg( sPrefix ).arg( s ).arg( m_bPlaybackTrackEnabled ) )
			.append( QString( "%1%2m_fPlaybackTrackVolume: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fPlaybackTrackVolume ) )
			.append( QString( "%1" ).arg( m_pVelocityAutomationPath->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_license: %3\n" ).arg( sPrefix ).arg( s ).arg( m_license.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_actionMode: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( static_cast<int>(m_actionMode) ) )
			.append( QString( "%1%2m_nPanLawType: %3\n" ).arg( sPrefix ).arg( s ).arg( m_nPanLawType ) )
			.append( QString( "%1%2m_fPanLawKNorm: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fPanLawKNorm ) )
			.append( QString( "%1%2m_pTimeline:\n" ).arg( sPrefix ).arg( s ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pTimeline->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
		sOutput.append( QString( "%1%2m_sCurrentDrumkitName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sCurrentDrumkitName ) )
			.append( QString( "%1%2m_currentDrumkitLookup: %3\n" ).arg( sPrefix ).arg( s ).arg( static_cast<int>(m_currentDrumkitLookup) ) );
	} else {
		
		sOutput = QString( "[Song]" )
			.append( QString( ", m_bIsTimelineActivated: %1" ).arg( m_bIsTimelineActivated ) )
			.append( QString( ", m_bIsMuted: %1" ).arg( m_bIsMuted ) )
			.append( QString( ", m_resolution: %1" ).arg( m_resolution ) )
			.append( QString( ", m_fBpm: %1" ).arg( m_fBpm ) )
			.append( QString( ", m_sName: %1" ).arg( m_sName ) )
			.append( QString( ", m_sAuthor: %1" ).arg( m_sAuthor ) )
			.append( QString( ", m_fVolume: %1" ).arg( m_fVolume ) )
			.append( QString( ", m_fMetronomeVolume: %1" ).arg( m_fMetronomeVolume ) )
			.append( QString( ", m_sNotes: %1" ).arg( m_sNotes ) )
			.append( QString( "%1" ).arg( m_pPatternList->toQString( sPrefix + s, bShort ) ) )
			.append( QString( ", m_pPatternGroupSequence:" ) );
		for ( auto pp : *m_pPatternGroupSequence ) {
			if ( pp != nullptr ) {
				sOutput.append( QString( "%1" ).arg( pp->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		sOutput.append( QString( "%1" ).arg( m_pInstrumentList->toQString( sPrefix + s, bShort ) ) )
			.append( QString( ", m_pComponents: [" ) );
		for ( auto cc : *m_pComponents ) {
			if ( cc != nullptr ) {
				sOutput.append( QString( "%1" ).arg( cc->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		sOutput.append( QString( "], m_sFilename: %1" ).arg( m_sFilename ) )
			.append( QString( ", m_loopMode: %1" ).arg( static_cast<int>(m_loopMode) ) )
			.append( QString( ", m_fHumanizeTimeValue: %1" ).arg( m_fHumanizeTimeValue ) )
			.append( QString( ", m_fHumanizeVelocityValue: %1" ).arg( m_fHumanizeVelocityValue ) )
			.append( QString( ", m_fSwingFactor: %1" ).arg( m_fSwingFactor ) )
			.append( QString( ", m_bIsModified: %1" ).arg( m_bIsModified ) )
			.append( QString( ", m_latestRoundRobins" ) );
		for ( auto mm : m_latestRoundRobins ) {
			sOutput.append( QString( ", %1 : %4" ).arg( mm.first ).arg( mm.second ) );
		}
		sOutput.append( QString( ", m_mode: %1" )
						.arg( static_cast<int>(m_mode) ) )
			.append( QString( ", m_sPlaybackTrackFilename: %1" ).arg( m_sPlaybackTrackFilename ) )
			.append( QString( ", m_bPlaybackTrackEnabled: %1" ).arg( m_bPlaybackTrackEnabled ) )
			.append( QString( ", m_fPlaybackTrackVolume: %1" ).arg( m_fPlaybackTrackVolume ) )
			.append( QString( ", m_pVelocityAutomationPath: %1" ).arg( m_pVelocityAutomationPath->toQString( sPrefix ) ) )
			.append( QString( ", m_license: %1" ).arg( m_license.toQString( sPrefix, bShort ) ) )
			.append( QString( ", m_actionMode: %1" ).arg( static_cast<int>(m_actionMode) ) )
			.append( QString( ", m_nPanLawType: %1" ).arg( m_nPanLawType ) )
			.append( QString( ", m_fPanLawKNorm: %1" ).arg( m_fPanLawKNorm ) )
			.append( QString( ", m_pTimeline: " ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pTimeline->toQString( sPrefix, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr" ) );
		}
		sOutput.append( QString( ", m_sCurrentDrumkitName: %1" ).arg( m_sCurrentDrumkitName ) )
			.append( QString( ", m_currentDrumkitLookup: %1" ).arg( static_cast<int>(m_currentDrumkitLookup) ) );
			
	}
	
	return sOutput;
}
};
