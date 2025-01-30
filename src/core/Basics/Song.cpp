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

#include "Version.h"

#include <cassert>
#include <memory>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>

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
#include <core/Helpers/Future.h>
#include <core/Helpers/Legacy.h>
#include <core/Sampler/Sampler.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

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
	, m_resolution( nDefaultResolution )
	, m_fBpm( fBpm )
	, m_sName( sName )
	, m_sAuthor( sAuthor )
	, m_fVolume( fVolume )
	, m_fMetronomeVolume( 0.5 )
	, m_sNotes( "" )
	, m_pPatternList( nullptr )
	, m_pPatternGroupSequence( nullptr )
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
	, m_sLastLoadedDrumkitName( "" )
	, m_sLastLoadedDrumkitPath( "" )
{
	INFOLOG( QString( "INIT '%1'" ).arg( sName ) );

	m_pInstrumentList = std::make_shared<InstrumentList>();
	m_pComponents = std::make_shared<std::vector<std::shared_ptr<DrumkitComponent>>>();
	
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
	
	if ( m_pPatternGroupSequence ) {
		for ( unsigned i = 0; i < m_pPatternGroupSequence->size(); ++i ) {
			PatternList* pPatternList = ( *m_pPatternGroupSequence )[i];
			pPatternList->clear();	// pulisco tutto, i pattern non vanno distrutti qua
			delete pPatternList;
		}
		delete m_pPatternGroupSequence;
	}

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

	if ( m_pTimeline != nullptr ) {
		m_pTimeline->setDefaultBpm( m_fBpm );
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
		ERRORLOG( "Error reading song: 'song' node not found" );
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

	auto pSong = Song::loadFrom( &songNode, sFilename, bSilent );
	if ( pSong != nullptr ) {
		pSong->setFilename( sFilename );
	}

	return pSong;
}

std::shared_ptr<Song> Song::loadFrom( XMLNode* pRootNode, const QString& sFilename, bool bSilent )
{
	auto pPreferences = Preferences::get_instance();
	
	float fBpm = pRootNode->read_float( "bpm", 120, false, false, bSilent );
	float fVolume = pRootNode->read_float( "volume", 0.5, false, false, bSilent );
	QString sName( pRootNode->read_string( "name", "Untitled Song",
										   false, false, bSilent ) );
	QString sAuthor( pRootNode->read_string( "author", "Unknown Author",
											 false, false, bSilent ) );

	std::shared_ptr<Song> pSong = std::make_shared<Song>( sName, sAuthor, fBpm, fVolume );

	pSong->setIsMuted( pRootNode->read_bool( "isMuted", false, true, false,
											 bSilent ) );
	pSong->setMetronomeVolume( pRootNode->read_float( "metronomeVolume", 0.5,
													  false, false, bSilent ) );
	pSong->setNotes( pRootNode->read_string( "notes", "...", false, false, bSilent ) );
	pSong->setLicense( License( pRootNode->read_string( "license", "",
														false, false, bSilent ), sAuthor ) );
	if ( pRootNode->read_bool( "loopEnabled", false, false, false, bSilent ) ) {
		pSong->setLoopMode( Song::LoopMode::Enabled );
	} else {
		pSong->setLoopMode( Song::LoopMode::Disabled );
	}
	
	if ( pRootNode->read_bool( "patternModeMode",
							   static_cast<bool>(Song::PatternMode::Selected),
							   false, false, bSilent ) ) {
		pSong->setPatternMode( Song::PatternMode::Selected );
	} else {
		pSong->setPatternMode( Song::PatternMode::Stacked );
	}

	if ( pRootNode->read_string( "mode", "pattern", false, false, bSilent ) == "song" ) {
		pSong->setMode( Song::Mode::Song );
	} else {
		pSong->setMode( Song::Mode::Pattern );
	}

	QString sPlaybackTrack( pRootNode->read_string( "playbackTrackFilename", "",
													false, true, bSilent ) );
	if ( sPlaybackTrack.left( 2 ) == "./" ||
		 sPlaybackTrack.left( 2 ) == ".\\" ) {
		// Playback track has been made portable by manually
		// converting the absolute path stored by Hydrogen into a
		// relative one.
		QFileInfo info( sFilename );
		sPlaybackTrack = info.absoluteDir()
			.filePath( sPlaybackTrack.right( sPlaybackTrack.size() - 2 ) );
	}
	
	// Check the file of the playback track and resort to the default
	// in case the file can not be found.
	if ( ! sPlaybackTrack.isEmpty() &&
		 ! Filesystem::file_exists( sPlaybackTrack, true ) ) {
		ERRORLOG( QString( "Provided playback track file [%1] does not exist. Using empty string instead" )
				  .arg( sPlaybackTrack ) );
		sPlaybackTrack = "";
	}
	pSong->setPlaybackTrackFilename( sPlaybackTrack );
	pSong->setPlaybackTrackEnabled( pRootNode->read_bool( "playbackTrackEnabled", false,
														  false, false, bSilent ) );
	pSong->setPlaybackTrackVolume( pRootNode->read_float( "playbackTrackVolume", 0.0,
														  false, false, bSilent ) );
	
	pSong->setHumanizeTimeValue( pRootNode->read_float( "humanize_time", 0.0,
														false, false, bSilent ) );
	pSong->setHumanizeVelocityValue( pRootNode->read_float( "humanize_velocity", 0.0,
															false, false, bSilent ) );
	pSong->setSwingFactor( pRootNode->read_float( "swing_factor", 0.0, false, false, bSilent ) );
	pSong->setActionMode( static_cast<Song::ActionMode>(
		pRootNode->read_int( "action_mode",
							 static_cast<int>( Song::ActionMode::selectMode ),
							 false, false, bSilent ) ) );
	pSong->setIsPatternEditorLocked( pRootNode->read_bool( "isPatternEditorLocked",
														   false, true, false, true ) );

	bool bContainsIsTimelineActivated;
	bool bIsTimelineActivated =
		pRootNode->read_bool( "isTimelineActivated", false,
							  &bContainsIsTimelineActivated, true, false, true );
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
	QString sPanLawType( pRootNode->read_string( "pan_law_type",
												 "RATIO_STRAIGHT_POLYGONAL",
												 false, false, bSilent ) );
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

	float fPanLawKNorm = pRootNode->read_float( "pan_law_k_norm", Sampler::K_NORM_DEFAULT,
												false, false, bSilent );
	if ( fPanLawKNorm <= 0.0 ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Invalid pan law k in import song [%1] (<= 0). Set default k." )
						.arg( fPanLawKNorm ) );
		}
		fPanLawKNorm = Sampler::K_NORM_DEFAULT;
	}
	pSong->setPanLawKNorm( fPanLawKNorm );

	auto formatVersionNode = pRootNode->firstChildElement( "formatVersion" );
	if ( ! formatVersionNode.isNull() ) {
		WARNINGLOG( QString( "Song [%1] was created with a more recent version of Hydrogen than the current one!" )
					.arg( sFilename ) );
		XMLNode drumkitNode = pRootNode->firstChildElement( "drumkit_info" );
		const auto pDrumkit = Future::loadDrumkit( drumkitNode, "", bSilent );
		if ( pDrumkit != nullptr ) {
			pSong->setInstrumentList( pDrumkit->get_instruments() );
			pSong->getInstrumentList()->load_samples( fBpm );
			pSong->m_pComponents = pDrumkit->get_components();
		}
		else {
			ERRORLOG( "Unable to load drumkit contained in song" );

			pSong->setInstrumentList( std::make_shared<InstrumentList>() );
			pSong->m_pComponents =
				std::make_shared<std::vector<std::shared_ptr<DrumkitComponent>>>();
			auto pDrumkitComponent = std::make_shared<DrumkitComponent>( 0, "Main" );
			pSong->getComponents()->push_back( pDrumkitComponent );
		}
	}
	else {
		XMLNode componentListNode = pRootNode->firstChildElement( "componentList" );
		if ( ( ! componentListNode.isNull()  ) ) {
			XMLNode componentNode = componentListNode.firstChildElement( "drumkitComponent" );
			while ( ! componentNode.isNull()  ) {
				auto pDrumkitComponent = DrumkitComponent::load_from( &componentNode );
				if ( pDrumkitComponent != nullptr ) {
					pSong->getComponents()->push_back( pDrumkitComponent );
				}

				componentNode = componentNode.nextSiblingElement( "drumkitComponent" );
			}
		}
		else {
			auto pDrumkitComponent = std::make_shared<DrumkitComponent>( 0, "Main" );
			pSong->getComponents()->push_back( pDrumkitComponent );
		}

		// Instrument List
		//
		// By supplying no drumkit path the individual drumkit meta infos
		// stored in the 'instrument' nodes will be used.
		auto pInstrumentList = InstrumentList::load_from( pRootNode,
														  "", // sDrumkitPath
														  "", // sDrumkitName
														  License(), // per-instrument licenses
														  bSilent );

		if ( pInstrumentList == nullptr ) {
			return nullptr;
		}

		pInstrumentList->load_samples( fBpm );
		pSong->setInstrumentList( pInstrumentList );
	}

	QString sLastLoadedDrumkitPath =
		pRootNode->read_string( "last_loaded_drumkit", "", true, false, true );
	QString sLastLoadedDrumkitName = 
		pRootNode->read_string( "last_loaded_drumkit_name", "", true, false, true );

#ifdef H2CORE_HAVE_APPIMAGE
	sLastLoadedDrumkitPath =
		Filesystem::rerouteDrumkitPath( sLastLoadedDrumkitPath );
#endif
	
	if ( sLastLoadedDrumkitPath.isEmpty() ) {
		// Prior to version 1.2.0 the last loaded drumkit was read
		// from the last instrument loaded and was not written to disk
		// explicitly. This caused problems the moment the user put an
		// instrument from a different drumkit at the end of the
		// instrument list. To nevertheless retrieve the last loaded
		// drumkit we will use a heuristic by taking the majority vote
		// among the loaded instruments.
		std::map<QString,int> loadedDrumkits;
		for ( const auto& pInstrument : *pSong->getInstrumentList() ) {
			if ( loadedDrumkits.find( pInstrument->get_drumkit_path() ) !=
				 loadedDrumkits.end() ) {
				loadedDrumkits[ pInstrument->get_drumkit_path() ] += 1;
			}
			else {
				loadedDrumkits[ pInstrument->get_drumkit_path() ] = 1;
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
	pSong->setLastLoadedDrumkitPath( sLastLoadedDrumkitPath );

	// Attempt to access the last loaded drumkit to load it into the
	// SoundLibraryDatabase in case it was a custom one (e.g. loaded
	// via OSC or from a different system data folder due to a
	// different install prefix).
	auto pSoundLibraryDatabase = Hydrogen::get_instance()->getSoundLibraryDatabase();
	auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sLastLoadedDrumkitPath, true );

	if ( sLastLoadedDrumkitName.isEmpty() ) {
		// The initial song is loaded after Hydrogen was
		// bootstrap. So, the SoundLibrary should be present and
		// filled with all available drumkits.
		if ( pSoundLibraryDatabase != nullptr && pDrumkit != nullptr ) {
			sLastLoadedDrumkitName = pDrumkit->get_name();
		}
	}
	pSong->setLastLoadedDrumkitName( sLastLoadedDrumkitName );

	// Pattern list
	pSong->setPatternList( PatternList::load_from( pRootNode,
												   pSong->getInstrumentList(),
												   bSilent ) );

	// Virtual Patterns
	pSong->loadVirtualPatternsFrom( pRootNode, bSilent );

	// Pattern sequence
	pSong->loadPatternGroupVectorFrom( pRootNode, bSilent );

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
			QString sName = fxNode.read_string( "name", "", false, false, bSilent );

			if ( sName != "no plugin" ) {
				// FIXME: il caricamento va fatto fare all'engine, solo lui sa il samplerate esatto
#ifdef H2CORE_HAVE_LADSPA
				LadspaFX* pFX = LadspaFX::load( fxNode.read_string( "filename", "", false, false, bSilent ),
												sName, 44100 );
				Effects::get_instance()->setLadspaFX( pFX, nFX );
				if ( pFX != nullptr ) {
					pFX->setEnabled( fxNode.read_bool( "enabled", false, false, false, bSilent ) );
					pFX->setVolume( fxNode.read_float( "volume", 1.0, false, false, bSilent ) );
					XMLNode inputControlNode = fxNode.firstChildElement( "inputControlPort" );
					while ( ! inputControlNode.isNull() ) {
						QString sName = inputControlNode.read_string( "name", "", false, false, bSilent );
						float fValue = inputControlNode.read_float( "value", 0.0, false, false, bSilent );

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
			pTimeline->addTempoMarker( newBPMNode.read_int( "BAR", 0, false, false, bSilent ),
									   newBPMNode.read_float( "BPM", 120.0, false, false, bSilent ) );
			newBPMNode = newBPMNode.nextSiblingElement( "newBPM" );
		}
	} else if ( ! bSilent ) {
		WARNINGLOG( "'BPMTimeLine' node not found" );
	}

	XMLNode timeLineTagNode = pRootNode->firstChildElement( "timeLineTag" );
	if ( ! timeLineTagNode.isNull() ) {
		XMLNode newTAGNode = timeLineTagNode.firstChildElement( "newTAG" );
		while ( ! newTAGNode.isNull() ) {
			pTimeline->addTag( newTAGNode.read_int( "BAR", 0, false, false, bSilent ),
							   newTAGNode.read_string( "TAG", "", false, false, bSilent ) );
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
			QString sAdjust = pathNode.read_attribute( "adjust", "velocity", false, false, bSilent );

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
		ERRORLOG( QString( "Unable to save song to [%1]. Path is not writable!" )
				  .arg( sFilename ) );
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG( QString( "Saving song to [%1]" ).arg( sFilename ) );
	}

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( "song" );
	
	// In order to comply with the GPL license we have to add a
	// license notice to the file.
	if ( getLicense().getType() == License::GPL ) {
		doc.appendChild( doc.createComment( License::getGPLLicenseNotice( getAuthor() ) ) );
	}

	writeTo( &rootNode, bSilent );
	
	setFilename( sFilename );
	setIsModified( false );

	if ( ! doc.write( sFilename ) ) {
		ERRORLOG( QString( "Error writing song to [%1]" ).arg( sFilename ) );
		return false;
	}

	if ( ! bSilent ) {
		INFOLOG("Save was successful.");
	}

	return true;
}

void Song::loadVirtualPatternsFrom( XMLNode* pNode, bool bSilent ) {

	XMLNode virtualPatternListNode = pNode->firstChildElement( "virtualPatternList" );
	if ( virtualPatternListNode.isNull() ) {
		ERRORLOG( "'virtualPatternList' node not found. Aborting." );
		return;
	}

	XMLNode virtualPatternNode = virtualPatternListNode.firstChildElement( "pattern" );
	while ( ! virtualPatternNode.isNull() ) {
		QString sName = virtualPatternNode.read_string( "name", sName, false, false, bSilent );

		Pattern* pCurPattern = nullptr;
		for ( const auto& pPattern : *m_pPatternList ) {
			if ( pPattern->get_name() == sName ) {
				pCurPattern = pPattern;
				break;
			}
		}

		if ( pCurPattern != nullptr ) {
			XMLNode virtualNode = virtualPatternNode.firstChildElement( "virtual" );
			while ( !virtualNode.isNull() ) {
				QString sVirtualPatternName = virtualNode.firstChild().nodeValue();

				Pattern* pVirtualPattern = nullptr;
				for ( const auto& pPattern : *m_pPatternList ) {
					if ( pPattern != nullptr &&
						 pPattern->get_name() == sVirtualPatternName ) {
						pVirtualPattern = pPattern;
						break;
					}
				}

				if ( pVirtualPattern != nullptr ) {
					pCurPattern->virtual_patterns_add( pVirtualPattern );
				}
				else if ( ! bSilent ) {
					ERRORLOG( "Song had invalid virtual pattern list data (virtual)" );
				}
				virtualNode = virtualNode.nextSiblingElement( "virtual" );
			}
		}
		else if ( ! bSilent ) {
			ERRORLOG( "Song had invalid virtual pattern list data (name)" );
		}
		virtualPatternNode = virtualPatternNode.nextSiblingElement( "pattern" );
	}

	m_pPatternList->flattened_virtual_patterns_compute();
}

void Song::loadPatternGroupVectorFrom( XMLNode* pNode, bool bSilent ) {
    XMLNode patternSequenceNode = pNode->firstChildElement( "patternSequence" );
	if ( patternSequenceNode.isNull() ) {
		if ( ! bSilent ) {
			ERRORLOG( "'patternSequence' node not found. Aborting." );
		}
		return;
	}

	if ( ! patternSequenceNode.firstChildElement( "patternID" ).isNull() ) {
		// back-compatibility code..
		m_pPatternGroupSequence = Legacy::loadPatternGroupVector( &patternSequenceNode,
																  m_pPatternList,
																  bSilent );
	}
	else {
		// current format
		if ( m_pPatternGroupSequence == nullptr ) {
			m_pPatternGroupSequence = new std::vector<PatternList*>;
		} else {
			m_pPatternGroupSequence->clear();
		}
		
		XMLNode groupNode = patternSequenceNode.firstChildElement( "group" );
		while ( ! groupNode.isNull() ) {
			PatternList* patternSequence = new PatternList();
			XMLNode patternIdNode = groupNode.firstChildElement( "patternID" );
			while ( ! patternIdNode.isNull() ) {
				QString sPatternName = patternIdNode.firstChild().nodeValue();

				Pattern* pPattern = nullptr;
				for ( const auto& ppPat : *m_pPatternList ) {
					if ( ppPat != nullptr ) {
						if ( ppPat->get_name() == sPatternName ) {
							pPattern = ppPat;
							break;
						}
					}
				}
				
				if ( pPattern != nullptr ) {
					patternSequence->add( pPattern );
				} else if ( ! bSilent ) {
					WARNINGLOG( "patternid not found in patternSequence" );
				}

				patternIdNode = patternIdNode.nextSiblingElement( "patternID" );
			}
			m_pPatternGroupSequence->push_back( patternSequence );

			groupNode = groupNode.nextSiblingElement( "group" );
		}
	}
}

void Song::writeVirtualPatternsTo( XMLNode* pNode, bool bSilent ) {
	XMLNode virtualPatternListNode = pNode->createNode( "virtualPatternList" );
	for ( const auto& pPattern : *m_pPatternList ) {
		if ( ! pPattern->get_virtual_patterns()->empty() ) {
			XMLNode patternNode = virtualPatternListNode.createNode( "pattern" );
			patternNode.write_string( "name", pPattern->get_name() );

			for ( const auto& pVirtualPattern : *( pPattern->get_virtual_patterns() ) ) {
				patternNode.write_string( "virtual", pVirtualPattern->get_name() );
			}
		}
	}
}

void Song::writePatternGroupVectorTo( XMLNode* pNode, bool bSilent ) {
	XMLNode patternSequenceNode = pNode->createNode( "patternSequence" );
	for ( const auto& pPatternList : *m_pPatternGroupSequence ) {
		if ( pPatternList != nullptr ) {
			XMLNode groupNode = patternSequenceNode.createNode( "group" );

			for ( const auto& pPattern : *pPatternList ) {
				if ( pPattern != nullptr ) {
					groupNode.write_string( "patternID", pPattern->get_name() );
				}
			}
		}
	}
}
	
void Song::writeTo( XMLNode* pRootNode, bool bSilent ) {
	pRootNode->write_string( "version", QString( get_version().c_str() ) );
	pRootNode->write_float( "bpm", m_fBpm );
	pRootNode->write_float( "volume", m_fVolume );
	pRootNode->write_bool( "isMuted", m_bIsMuted );
	pRootNode->write_float( "metronomeVolume", m_fMetronomeVolume );
	pRootNode->write_string( "name", m_sName );
	pRootNode->write_string( "author", m_sAuthor );
	pRootNode->write_string( "notes", m_sNotes );
	pRootNode->write_string( "license", m_license.getLicenseString() );
	pRootNode->write_bool( "loopEnabled", isLoopEnabled() );

	bool bPatternMode = static_cast<bool>(Song::PatternMode::Selected);
	if ( m_patternMode == Song::PatternMode::Stacked ) {
		bPatternMode = static_cast<bool>(Song::PatternMode::Stacked);
	}
	pRootNode->write_bool( "patternModeMode", bPatternMode );
	
	pRootNode->write_string( "playbackTrackFilename", m_sPlaybackTrackFilename );
	pRootNode->write_bool( "playbackTrackEnabled", m_bPlaybackTrackEnabled );
	pRootNode->write_float( "playbackTrackVolume", m_fPlaybackTrackVolume );
	pRootNode->write_int( "action_mode", static_cast<int>( m_actionMode ) );
	pRootNode->write_bool( "isPatternEditorLocked",
						   m_bIsPatternEditorLocked );
	pRootNode->write_bool( "isTimelineActivated", m_bIsTimelineActivated );
	
	if ( m_mode == Song::Mode::Song ) {
		pRootNode->write_string( "mode", QString( "song" ) );
	} else {
		pRootNode->write_string( "mode", QString( "pattern" ) );
	}

	QString sPanLawType; // prepare the pan law string to write
	if ( m_nPanLawType == Sampler::RATIO_STRAIGHT_POLYGONAL ) {
		sPanLawType = "RATIO_STRAIGHT_POLYGONAL";
	} else if ( m_nPanLawType == Sampler::RATIO_CONST_POWER ) {
		sPanLawType = "RATIO_CONST_POWER";
	} else if ( m_nPanLawType == Sampler::RATIO_CONST_SUM ) {
		sPanLawType = "RATIO_CONST_SUM";
	} else if ( m_nPanLawType == Sampler::LINEAR_STRAIGHT_POLYGONAL ) {
		sPanLawType = "LINEAR_STRAIGHT_POLYGONAL";
	} else if ( m_nPanLawType == Sampler::LINEAR_CONST_POWER ) {
		sPanLawType = "LINEAR_CONST_POWER";
	} else if ( m_nPanLawType == Sampler::LINEAR_CONST_SUM ) {
		sPanLawType = "LINEAR_CONST_SUM";
	} else if ( m_nPanLawType == Sampler::POLAR_STRAIGHT_POLYGONAL ) {
		sPanLawType = "POLAR_STRAIGHT_POLYGONAL";
	} else if ( m_nPanLawType == Sampler::POLAR_CONST_POWER ) {
		sPanLawType = "POLAR_CONST_POWER";
	} else if ( m_nPanLawType == Sampler::POLAR_CONST_SUM ) {
		sPanLawType = "POLAR_CONST_SUM";
	} else if ( m_nPanLawType == Sampler::QUADRATIC_STRAIGHT_POLYGONAL ) {
		sPanLawType = "QUADRATIC_STRAIGHT_POLYGONAL";
	} else if ( m_nPanLawType == Sampler::QUADRATIC_CONST_POWER ) {
		sPanLawType = "QUADRATIC_CONST_POWER";
	} else if ( m_nPanLawType == Sampler::QUADRATIC_CONST_SUM ) {
		sPanLawType = "QUADRATIC_CONST_SUM";
	} else if ( m_nPanLawType == Sampler::LINEAR_CONST_K_NORM ) {
		sPanLawType = "LINEAR_CONST_K_NORM";
	} else if ( m_nPanLawType == Sampler::POLAR_CONST_K_NORM ) {
		sPanLawType = "POLAR_CONST_K_NORM";
	} else if ( m_nPanLawType == Sampler::RATIO_CONST_K_NORM ) {
		sPanLawType = "RATIO_CONST_K_NORM";
	} else if ( m_nPanLawType == Sampler::QUADRATIC_CONST_K_NORM ) {
		sPanLawType = "QUADRATIC_CONST_K_NORM";
	} else {
		if ( ! bSilent ) {
			WARNINGLOG( "Unknown pan law in saving song. Saved default type." );
		}
		sPanLawType = "RATIO_STRAIGHT_POLYGONAL";
	}
	// write the pan law string in file
	pRootNode->write_string( "pan_law_type", sPanLawType );
	pRootNode->write_float( "pan_law_k_norm", m_fPanLawKNorm );

	pRootNode->write_float( "humanize_time", m_fHumanizeTimeValue );
	pRootNode->write_float( "humanize_velocity", m_fHumanizeVelocityValue );
	pRootNode->write_float( "swing_factor", m_fSwingFactor );

	pRootNode->write_string( "last_loaded_drumkit", m_sLastLoadedDrumkitPath );
	pRootNode->write_string( "last_loaded_drumkit_name", m_sLastLoadedDrumkitName );

	XMLNode componentListNode = pRootNode->createNode( "componentList" );
	for ( const auto& pComponent : *m_pComponents ) {
		if ( pComponent != nullptr ) {
			pComponent->save_to( &componentListNode );
		}
	}

	m_pInstrumentList->save_to( pRootNode, -1, true, true );
	
	m_pPatternList->save_to( pRootNode, nullptr );

	writeVirtualPatternsTo( pRootNode, bSilent );
	
	writePatternGroupVectorTo( pRootNode, bSilent );

	XMLNode ladspaFxNode = pRootNode->createNode( "ladspa" );
	for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
		XMLNode fxNode = ladspaFxNode.createNode( "fx" );

#ifdef H2CORE_HAVE_LADSPA
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX != nullptr ) {
			fxNode.write_string( "name", pFX->getPluginLabel() );
			fxNode.write_string( "filename", pFX->getLibraryPath() );
			fxNode.write_bool( "enabled", pFX->isEnabled() );
			fxNode.write_float( "volume", pFX->getVolume() );
			
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
			fxNode.write_float( "volume", 0.0 );
		}
	}

	//bpm time line
	auto pTimeline = Hydrogen::get_instance()->getTimeline();

	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	XMLNode bpmTimeLineNode = pRootNode->createNode( "BPMTimeLine" );
	if ( tempoMarkerVector.size() >= 1 ){
		for ( int tt = 0; tt < static_cast<int>(tempoMarkerVector.size()); tt++){
			if ( tt == 0 && pTimeline->isFirstTempoMarkerSpecial() ) {
				continue;
			}
			XMLNode newBPMNode = bpmTimeLineNode.createNode( "newBPM" );
			newBPMNode.write_int( "BAR", tempoMarkerVector[tt]->nColumn );
			newBPMNode.write_float( "BPM", tempoMarkerVector[tt]->fBpm );
		}
	}

	//time line tag
	auto tagVector = pTimeline->getAllTags();
	XMLNode timeLineTagNode = pRootNode->createNode( "timeLineTag" );
	if ( tagVector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(tagVector.size()); t++){
			XMLNode newTAGNode = timeLineTagNode.createNode( "newTAG" );
			newTAGNode.write_int( "BAR", tagVector[t]->nColumn );
			newTAGNode.write_string( "TAG", tagVector[t]->sTag );
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

	auto pInstrList = std::make_shared<InstrumentList>();
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

	auto pSoundLibraryDatabase = Hydrogen::get_instance()->getSoundLibraryDatabase();

	QString sDefaultDrumkitPath = Filesystem::drumkit_default_kit();
	auto pDrumkit = pSoundLibraryDatabase->getDrumkit( sDefaultDrumkitPath );
	if ( pDrumkit == nullptr ) {
		for ( const auto& pEntry : pSoundLibraryDatabase->getDrumkitDatabase() ) {
			if ( pEntry.second != nullptr ) {
				WARNINGLOG( QString( "Unable to retrieve default drumkit [%1]. Using kit [%2] instead." )
							.arg( sDefaultDrumkitPath )
							.arg( pEntry.first ) );
				pDrumkit = pEntry.second;
				break;
			}
		}
	}

	if ( pDrumkit != nullptr ) {
		pSong->setDrumkit( pDrumkit, true );
	}
	else {
		ERRORLOG( "Unable to load drumkit" );
	}
	
	pSong->setIsModified( false );

	return pSong;

}

std::shared_ptr<DrumkitComponent> Song::getComponent( int nID ) const
{
	for ( auto pComponent : *m_pComponents ) {
		if ( pComponent->get_id() == nID ) {
			return pComponent;
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
	auto pInstrumentList = getInstrumentList();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		if ( pInstrumentList->get( i )->has_missing_samples() ) {
			return true;
		}
	}
	return false;
}

void Song::clearMissingSamples() {
	auto pInstrumentList = getInstrumentList();
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

	loadVirtualPatternsFrom( &root, false );
	loadPatternGroupVectorFrom( &root, false );
}

bool Song::writeTempPatternList( const QString& sFilename )
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "sequence" );

	writeVirtualPatternsTo( &root, false );
	writePatternGroupVectorTo( &root, false );

	return doc.write( sFilename );
}

QString Song::copyInstrumentLineToString( int nSelectedInstrument )
{
	auto pInstrument = getInstrumentList()->get( nSelectedInstrument );
	if ( pInstrument == nullptr ) {
		assert( pInstrument );
		ERRORLOG( QString( "Unable to retrieve instrument [%1]" )
				  .arg( nSelectedInstrument ) );
		return QString();
	}

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( "instrument_line" );
	rootNode.write_string( "author", getAuthor() );
	rootNode.write_string( "license", getLicense().getLicenseString() );

	m_pPatternList->save_to( &rootNode, pInstrument );

	// Serialize document
	return doc.toString();
}

bool Song::pasteInstrumentLineFromString( const QString& sSerialized, int nSelectedInstrument, std::list<Pattern *>& patterns )
{
	XMLDoc doc;
	if ( ! doc.setContent( sSerialized ) ) {
		return false;
	}

	// Get current instrument
	auto pInstr = getInstrumentList()->get( nSelectedInstrument );
	assert( pInstr );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to find instrument [%1]" )
				  .arg( nSelectedInstrument ) );
		return false;
	}

	// Get pattern list
	PatternList *pList = getPatternList();
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
		QString patternName( patternNode.read_string( "name", "", false, false ) );

		// Check if pattern name specified
		if ( patternName.length() > 0 || bIsNoteSelection ) {
			// Try to find pattern by name
			Pattern* pat = pList->find(patternName);

			// If OK - check if need to add this pattern to result
			// If there is only one pattern, we always add it to list
			// If there is no selected pattern, we add all existing patterns to list (match by name)
			// Otherwise we add only existing selected pattern to list (match by name)
			if ( is_single || pat != nullptr ) {

				pat = new Pattern( patternName,
								   patternNode.read_string( "info", "", true, false ),
								   patternNode.read_string( "category", "unknown", true, false ),
								   patternNode.read_int( "size", -1, true, false ),
								   patternNode.read_int( "denominator", 4, true, false ) );

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
				patterns.push_back(pat);
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

void Song::setDrumkit( std::shared_ptr<Drumkit> pDrumkit, bool bConditional ) {
	auto pHydrogen = Hydrogen::get_instance();

	assert ( pDrumkit );
	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Invalid drumkit supplied" );
		return;
	}

	m_sLastLoadedDrumkitName = pDrumkit->get_name();
	m_sLastLoadedDrumkitPath = pDrumkit->get_path();

	// Load DrumkitComponents 
	auto pDrumkitCompoList = pDrumkit->get_components();

	auto pNewComponents = std::make_shared<std::vector<std::shared_ptr<DrumkitComponent>>>();
	
	for ( const auto& pSrcComponent : *pDrumkitCompoList ) {
		auto pNewComponent = std::make_shared<DrumkitComponent>( pSrcComponent->get_id(),
																 pSrcComponent->get_name() );
		pNewComponent->load_from( pSrcComponent );

		pNewComponents->push_back( pNewComponent );
	}
	m_pComponents = pNewComponents;

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
	auto pDrumkitInstrList = pDrumkit->get_instruments();

	if ( pDrumkitInstrList == m_pInstrumentList ) {
		// This occurs when saving a Drumkit based on the instrument
		// list of the current song using a different name. It stores
		// just a pointer to the instrument and component list of the
		// current song and will be set afterwards.
		return;
	}
	
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

	// Load samples of all instruments.
	m_pInstrumentList->load_samples(
		pHydrogen->getAudioEngine()->getTransportPosition()->getBpm() );

	// Remap instruments in pattern list to ensure component indices for SelectedLayerInfo's are up to date
	// for the current kit.
	for ( auto &pPattern : *m_pPatternList ) {
		for ( auto &note : *pPattern->get_notes() ) {
			note.second->map_instrument( m_pInstrumentList );
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
				INFOLOG("Keeping instrument #" + QString::number( nInstrumentNumber ) );
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
		INFOLOG("clear last instrument to empty instrument 1 instead delete the last instrument");
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
		}
		else {
			for ( const auto& ppattern : *pColumn ) {
				if ( ppattern != nullptr ) {
					FOREACH_NOTE_CST_IT_BEGIN_LENGTH( ppattern->get_notes(), it, ppattern ) {
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

QString Song::getLastLoadedDrumkitPath() const
{
	return Filesystem::ensure_session_compatibility( m_sLastLoadedDrumkitPath );
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
		sOutput.append( QString( "%1%2m_sLastLoadedDrumkitName: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sLastLoadedDrumkitName ) )
			.append( QString( "%1%2m_sLastLoadedDrumkitPath: %3\n" ).arg( sPrefix ).arg( s ).arg( m_sLastLoadedDrumkitPath ) );;
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
		sOutput.append( QString( ", m_sLastLoadedDrumkitName: %1" ).arg( m_sLastLoadedDrumkitName ) )
			.append( QString( ", m_sLastLoadedDrumkitPath: %1" ).arg( m_sLastLoadedDrumkitPath ) );
			
	}
	
	return sOutput;
}
};
