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


#include <cassert>
#include <memory>

#include <core/Basics/Song.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/AutomationPathSerializer.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Note.h>
#include <core/Basics/AutomationPath.h>
#include <core/EventQueue.h>
#include <core/FX/Effects.h>
#include <core/Globals.h>
#include <core/Helpers/Legacy.h>
#include <core/Hydrogen.h>
#ifdef H2CORE_HAVE_OSC
  #include <core/NsmClient.h>
#endif
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Sampler.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Timeline.h>
#include <core/Version.h>


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
	, m_nVersion( 0 )
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
	, m_sLastLoadedDrumkitPath( "" )
	, m_pDrumkit( std::make_shared<Drumkit>() )
	, m_pTimeline( std::make_shared<Timeline>() )
{
	if ( m_sName.isEmpty() ){
		m_sName = Filesystem::untitled_song_name();
	}
	INFOLOG( QString( "INIT '%1'" ).arg( m_sName ) );

	m_pVelocityAutomationPath = new AutomationPath(0.0f, 1.5f,  1.0f);
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

void Song::setDrumkit( std::shared_ptr<Drumkit> pDrumkit ) {
	m_pDrumkit = pDrumkit;
	m_pDrumkit->setContext( Drumkit::Context::Song );

	m_sLastLoadedDrumkitPath = pDrumkit->getPath();
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

void Song::setActionMode( const Song::ActionMode& actionMode ) {
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

	auto pSong = Song::loadFrom( songNode, sFilename, bSilent );
	if ( pSong != nullptr ) {
		pSong->setFilename( sFilename );
	}

	return pSong;
}

std::shared_ptr<Song> Song::loadFrom( const XMLNode& rootNode, const QString& sFilename, bool bSilent )
{
	auto pPreferences = Preferences::get_instance();

	float fBpm = rootNode.read_float( "bpm", 120, false, false, bSilent );
	float fVolume = rootNode.read_float( "volume", 0.5, false, false, bSilent );
	const QString sName( rootNode.read_string( "name", "Untitled Song",
											   false, false, bSilent ) );
	const QString sAuthor( rootNode.read_string( "author", "Unknown Author",
												 false, false, bSilent ) );

	std::shared_ptr<Song> pSong = std::make_shared<Song>( sName, sAuthor, fBpm, fVolume );

	pSong->m_nVersion = rootNode.read_int(
		"userVersion", pSong->m_nVersion, true, false, bSilent );

	pSong->setIsMuted( rootNode.read_bool( "isMuted", false, true, false,
											 bSilent ) );
	pSong->setMetronomeVolume( rootNode.read_float( "metronomeVolume", 0.5,
													  false, false, bSilent ) );
	pSong->setNotes( rootNode.read_string( "notes", "...", false, false, bSilent ) );
	const License license =
		License( rootNode.read_string( "license", "",
									   false, false, bSilent ), sAuthor );
	pSong->setLicense( license );
	if ( rootNode.read_bool( "loopEnabled", false, false, false, bSilent ) ) {
		pSong->setLoopMode( Song::LoopMode::Enabled );
	} else {
		pSong->setLoopMode( Song::LoopMode::Disabled );
	}

	if ( rootNode.read_bool( "patternModeMode",
							   static_cast<bool>(Song::PatternMode::Selected),
							   false, false, bSilent ) ) {
		pSong->setPatternMode( Song::PatternMode::Selected );
	} else {
		pSong->setPatternMode( Song::PatternMode::Stacked );
	}

	if ( rootNode.read_string( "mode", "pattern", false, false, bSilent ) == "song" ) {
		pSong->setMode( Song::Mode::Song );
	} else {
		pSong->setMode( Song::Mode::Pattern );
	}

	const auto sSongPath = Filesystem::absolute_path( sFilename );

	QString sPlaybackTrack( rootNode.read_string( "playbackTrackFilename", "",
													false, true, bSilent ) );
	QFileInfo playbackTrackInfo( sPlaybackTrack );
	if ( ! sPlaybackTrack.isEmpty() && playbackTrackInfo.isRelative() ) {
		// Playback track has been made portable by manually
		// converting the absolute path stored by Hydrogen into a
		// relative one.
		QFileInfo songPathInfo( sSongPath );
		sPlaybackTrack = songPathInfo.absoluteDir()
			.absoluteFilePath( sPlaybackTrack );
	}

	// Check the file of the playback track and resort to the default
	// in case the file can not be found.
	if ( ! sPlaybackTrack.isEmpty() &&
		 ! Filesystem::file_exists( sPlaybackTrack, true ) ) {
		ERRORLOG( QString( "Provided playback track file [%1] does not exist. Using empty string instead" )
				  .arg( sPlaybackTrack ) )
		sPlaybackTrack = "";
	}
	pSong->setPlaybackTrackFilename( sPlaybackTrack );
	pSong->setPlaybackTrackEnabled( rootNode.read_bool( "playbackTrackEnabled", false,
														  false, false, bSilent ) );
	pSong->setPlaybackTrackVolume( rootNode.read_float( "playbackTrackVolume", 0.0,
														  false, false, bSilent ) );

	pSong->setHumanizeTimeValue( rootNode.read_float( "humanize_time", 0.0,
														false, false, bSilent ) );
	pSong->setHumanizeVelocityValue( rootNode.read_float( "humanize_velocity", 0.0,
															false, false, bSilent ) );
	pSong->setSwingFactor( rootNode.read_float( "swing_factor", 0.0, false, false, bSilent ) );
	pSong->setActionMode( static_cast<Song::ActionMode>(
		rootNode.read_int( "action_mode",
							 static_cast<int>( Song::ActionMode::selectMode ),
							 false, false, bSilent ) ) );
	pSong->setIsPatternEditorLocked( rootNode.read_bool( "isPatternEditorLocked",
														   false, true, false, true ) );

	pSong->setIsTimelineActivated( rootNode.read_bool(
									   "isTimelineActivated",
									   pSong->getIsTimelineActivated(),
									   true, false, true ) );

	// pan law
	QString sPanLawType( rootNode.read_string( "pan_law_type",
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

	float fPanLawKNorm = rootNode.read_float( "pan_law_k_norm", Sampler::K_NORM_DEFAULT,
												false, false, bSilent );
	if ( fPanLawKNorm <= 0.0 ) {
		if ( ! bSilent ) {
			WARNINGLOG( QString( "Invalid pan law k in import song [%1] (<= 0). Set default k." )
						.arg( fPanLawKNorm ) );
		}
		fPanLawKNorm = Sampler::K_NORM_DEFAULT;
	}
	pSong->setPanLawKNorm( fPanLawKNorm );

	std::shared_ptr<Drumkit> pDrumkit;
	XMLNode drumkitNode = rootNode.firstChildElement( "drumkit_info");
	if ( ! drumkitNode.isNull() ) {
		// Current format (>= 1.3.0) storing a proper Drumkit
		pDrumkit = Drumkit::loadFrom( drumkitNode, "", sSongPath, true, bSilent );
	}
	else {
		// Older format (< 1.3.0) storing only selected elements
		pDrumkit = Legacy::loadEmbeddedSongDrumkit( rootNode, sSongPath, bSilent );
	}

	if ( pDrumkit == nullptr ) {
		ERRORLOG( "Unable to load drumkit. Falling back to default kit." );
		// Load default kit
		pDrumkit = std::make_shared<Drumkit>();
	}
	pSong->setDrumkit( pDrumkit );
	pSong->setLastLoadedDrumkitPath(
		rootNode.read_string( "lastLoadedDrumkitPath", "", true, true,
								bSilent ) );

	// Pattern list
	auto pPatternList = PatternList::load_from(
		rootNode, pDrumkit->getExportName(), bSilent );
	if ( pPatternList != nullptr ) {
		pPatternList->mapTo( pDrumkit, nullptr );
	}
	pSong->setPatternList( pPatternList );

	// Virtual Patterns
	pSong->loadVirtualPatternsFrom( rootNode, bSilent );

	// Pattern sequence
	pSong->loadPatternGroupVectorFrom( rootNode, bSilent );

#ifdef H2CORE_HAVE_LADSPA
	// reset FX
	for ( int fx = 0; fx < MAX_FX; ++fx ) {
		//LadspaFX* pFX = Effects::get_instance()->getLadspaFX( fx );
		//delete pFX;
		Effects::get_instance()->setLadspaFX( nullptr, fx );
	}
#endif

	// LADSPA FX
	XMLNode ladspaNode = rootNode.firstChildElement( "ladspa" );
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
	XMLNode bpmTimeLineNode = rootNode.firstChildElement( "BPMTimeLine" );
	if ( ! bpmTimeLineNode.isNull() ) {
		XMLNode newBPMNode = bpmTimeLineNode.firstChildElement( "newBPM" );

		// Use more efficient version to add TempoMarkers that will only
		// sort and update them once.
		std::vector<std::shared_ptr<Timeline::TempoMarker>> tempoMarkers;
		while ( ! newBPMNode.isNull() ) {
			tempoMarkers.push_back( std::make_shared<Timeline::TempoMarker>(
				newBPMNode.read_int( "BAR", 0, false, false, bSilent ),
				newBPMNode.read_float( "BPM", 120.0, false, false, bSilent ) ) );
			newBPMNode = newBPMNode.nextSiblingElement( "newBPM" );
		}

		if ( tempoMarkers.size() > 0 ) {
			pTimeline->addTempoMarkers( tempoMarkers );
		}
	} else if ( ! bSilent ) {
		WARNINGLOG( "'BPMTimeLine' node not found" );
	}

	XMLNode timeLineTagNode = rootNode.firstChildElement( "timeLineTag" );
	if ( ! timeLineTagNode.isNull() ) {
		XMLNode newTAGNode = timeLineTagNode.firstChildElement( "newTAG" );
		// Use more efficient version to add TempoMarkers that will only sort
		// once.
		std::vector<std::shared_ptr<Timeline::Tag>> tags;

		while ( ! newTAGNode.isNull() ) {
			tags.push_back( std::make_shared<Timeline::Tag>(
				newTAGNode.read_int( "BAR", 0, false, false, bSilent ),
				newTAGNode.read_string( "TAG", "", false, false, bSilent ) ) );
			newTAGNode = newTAGNode.nextSiblingElement( "newTAG" );
		}

		if ( tags.size() > 0 ) {
			pTimeline->addTags( tags );
		}
	} else if ( ! bSilent ) {
		WARNINGLOG( "TagTimeLine node not found" );
	}
	pSong->setTimeline( pTimeline );

	// Automation Paths
	XMLNode automationPathsNode = rootNode.firstChildElement( "automationPaths" );
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

	saveTo( rootNode, bSilent );

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

void Song::loadVirtualPatternsFrom( const XMLNode& node, bool bSilent ) {

	XMLNode virtualPatternListNode = node.firstChildElement( "virtualPatternList" );
	if ( virtualPatternListNode.isNull() ) {
		ERRORLOG( "'virtualPatternList' node not found. Aborting." );
		return;
	}

	XMLNode virtualPatternNode = virtualPatternListNode.firstChildElement( "pattern" );
	while ( ! virtualPatternNode.isNull() ) {
		QString sName = virtualPatternNode.read_string( "name", sName, false, false, bSilent );

		std::shared_ptr<Pattern> pCurPattern = nullptr;
		for ( const auto& pPattern : *m_pPatternList ) {
			if ( pPattern->getName() == sName ) {
				pCurPattern = pPattern;
				break;
			}
		}

		if ( pCurPattern != nullptr ) {
			XMLNode virtualNode = virtualPatternNode.firstChildElement( "virtual" );
			while ( !virtualNode.isNull() ) {
				QString sVirtualPatternName = virtualNode.firstChild().nodeValue();

				std::shared_ptr<Pattern> pVirtualPattern = nullptr;
				for ( const auto& pPattern : *m_pPatternList ) {
					if ( pPattern != nullptr &&
						 pPattern->getName() == sVirtualPatternName ) {
						pVirtualPattern = pPattern;
						break;
					}
				}

				if ( pVirtualPattern != nullptr ) {
					pCurPattern->virtualPatternsAdd( pVirtualPattern );
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

void Song::loadPatternGroupVectorFrom( const XMLNode& node, bool bSilent ) {
    XMLNode patternSequenceNode = node.firstChildElement( "patternSequence" );
	if ( patternSequenceNode.isNull() ) {
		if ( ! bSilent ) {
			ERRORLOG( "'patternSequence' node not found. Aborting." );
		}
		return;
	}

	if ( ! patternSequenceNode.firstChildElement( "patternID" ).isNull() ) {
		// back-compatibility code..
		m_pPatternGroupSequence = Legacy::loadPatternGroupVector( patternSequenceNode,
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

				std::shared_ptr<Pattern> pPattern = nullptr;
				for ( const auto& ppPat : *m_pPatternList ) {
					if ( ppPat != nullptr ) {
						if ( ppPat->getName() == sPatternName ) {
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

void Song::saveVirtualPatternsTo( XMLNode& node, bool bSilent ) const {
	if ( m_pPatternList == nullptr ) {
		return;
	}

	XMLNode virtualPatternListNode = node.createNode( "virtualPatternList" );
	for ( const auto& pPattern : *m_pPatternList ) {
		if ( ! pPattern->getVirtualPatterns()->empty() ) {
			XMLNode patternNode = virtualPatternListNode.createNode( "pattern" );
			patternNode.write_string( "name", pPattern->getName() );

			for ( const auto& pVirtualPattern : *( pPattern->getVirtualPatterns() ) ) {
				patternNode.write_string( "virtual", pVirtualPattern->getName() );
			}
		}
	}
}

void Song::savePatternGroupVectorTo( XMLNode& node, bool bSilent ) const {
	if ( m_pPatternGroupSequence == nullptr ) {
		return;
	}

	XMLNode patternSequenceNode = node.createNode( "patternSequence" );
	for ( const auto& pPatternList : *m_pPatternGroupSequence ) {
		if ( pPatternList != nullptr ) {
			XMLNode groupNode = patternSequenceNode.createNode( "group" );

			for ( const auto& pPattern : *pPatternList ) {
				if ( pPattern != nullptr ) {
					groupNode.write_string( "patternID", pPattern->getName() );
				}
			}
		}
	}
}

void Song::saveTo( XMLNode& rootNode, bool bSilent ) const {
	rootNode.write_string( "version", QString( get_version().c_str() ) );
	rootNode.write_int( "formatVersion", nCurrentFormatVersion );
	rootNode.write_float( "bpm", m_fBpm );
	rootNode.write_float( "volume", m_fVolume );
	rootNode.write_bool( "isMuted", m_bIsMuted );
	rootNode.write_float( "metronomeVolume", m_fMetronomeVolume );
	rootNode.write_int( "userVersion", m_nVersion );
	rootNode.write_string( "name", m_sName );
	rootNode.write_string( "author", m_sAuthor );
	rootNode.write_string( "notes", m_sNotes );
	rootNode.write_string( "license", m_license.getLicenseString() );
	rootNode.write_bool( "loopEnabled", isLoopEnabled() );

	bool bPatternMode = static_cast<bool>(Song::PatternMode::Selected);
	if ( m_patternMode == Song::PatternMode::Stacked ) {
		bPatternMode = static_cast<bool>(Song::PatternMode::Stacked);
	}
	rootNode.write_bool( "patternModeMode", bPatternMode );

	rootNode.write_string( "playbackTrackFilename", m_sPlaybackTrackFilename );
	rootNode.write_bool( "playbackTrackEnabled", m_bPlaybackTrackEnabled );
	rootNode.write_float( "playbackTrackVolume", m_fPlaybackTrackVolume );
	rootNode.write_int( "action_mode", static_cast<int>( m_actionMode ) );
	rootNode.write_bool( "isPatternEditorLocked",
						   m_bIsPatternEditorLocked );
	rootNode.write_bool( "isTimelineActivated", m_bIsTimelineActivated );

	if ( m_mode == Song::Mode::Song ) {
		rootNode.write_string( "mode", QString( "song" ) );
	} else {
		rootNode.write_string( "mode", QString( "pattern" ) );
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
	rootNode.write_string( "pan_law_type", sPanLawType );
	rootNode.write_float( "pan_law_k_norm", m_fPanLawKNorm );

	rootNode.write_float( "humanize_time", m_fHumanizeTimeValue );
	rootNode.write_float( "humanize_velocity", m_fHumanizeVelocityValue );
	rootNode.write_float( "swing_factor", m_fSwingFactor );

	// "drumkit_info" instead of "drumkit" seem unintuitive but is dictated by a
	// ancient design desicion and we will stick to it.
	auto drumkitNode = rootNode.createNode( "drumkit_info" );
	m_pDrumkit->saveTo( drumkitNode,
						true, // Enable per-instrument sample loading
						bSilent );

	rootNode.write_string( "lastLoadedDrumkitPath", m_sLastLoadedDrumkitPath );

	if ( m_pPatternList != nullptr ) {
		m_pPatternList->save_to( rootNode );
	}
	saveVirtualPatternsTo( rootNode, bSilent );
	savePatternGroupVectorTo( rootNode, bSilent );

	XMLNode ladspaFxNode = rootNode.createNode( "ladspa" );
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
	XMLNode bpmTimeLineNode = rootNode.createNode( "BPMTimeLine" );
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
	XMLNode timeLineTagNode = rootNode.createNode( "timeLineTag" );
	if ( tagVector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(tagVector.size()); t++){
			XMLNode newTAGNode = timeLineTagNode.createNode( "newTAG" );
			newTAGNode.write_int( "BAR", tagVector[t]->nColumn );
			newTAGNode.write_string( "TAG", tagVector[t]->sTag );
		}
	}

	// Automation Paths
	XMLNode automationPathsNode = rootNode.createNode( "automationPaths" );
	AutomationPath *pPath = getVelocityAutomationPath();
	if ( pPath != nullptr ) {
		XMLNode pathNode = automationPathsNode.createNode( "path" );
		pathNode.write_attribute("adjust", "velocity");

		AutomationPathSerializer serializer;
		serializer.write_automation_path(pathNode, *pPath);
	}
}

std::shared_ptr<Song> Song::getEmptySong( std::shared_ptr<SoundLibraryDatabase> pDB )
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

	PatternList*	pPatternList = new PatternList();
	PatternList*    patternSequence = new PatternList();

	for ( int nn = 0; nn < 10; ++nn ) {
		auto pEmptyPattern = std::make_shared<Pattern>();

		pEmptyPattern->setName( QString( "Pattern %1" ).arg( nn + 1 ) );
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

	pSong->setFilename( Filesystem::empty_path( Filesystem::Type::Song ) );

	std::shared_ptr<SoundLibraryDatabase> pSoundLibraryDatabase;

	if ( pDB != nullptr ) {
		// During startup
		pSoundLibraryDatabase = pDB;
	} else {
		pSoundLibraryDatabase =
			Hydrogen::get_instance()->getSoundLibraryDatabase();
	}

	const QString sDefaultDrumkitPath = Filesystem::drumkit_default_kit();
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

	if ( pDrumkit == nullptr ) {
		// In case we fail to load the default kit, we use an empty one.
		pDrumkit = Drumkit::getEmptyDrumkit();
	}
	else {
		pDrumkit = std::make_shared<Drumkit>(pDrumkit);
	}

	pSong->setDrumkit( pDrumkit );

	pSong->setIsModified( false );

	return pSong;

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

#ifdef H2CORE_HAVE_OSC
		if ( Hydrogen::get_instance()->isUnderSessionManagement() ) {
			// If Hydrogen is under session management (NSM), tell the
			// NSM server that the Song was modified.
			NsmClient::get_instance()->sendDirtyState( bIsModified );
		}
#endif
	}

}

bool Song::hasMissingSamples() const
{
	auto pInstrumentList = getDrumkit()->getInstruments();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		if ( pInstrumentList->get( i )->has_missing_samples() ) {
			return true;
		}
	}
	return false;
}

void Song::clearMissingSamples() {
	auto pInstrumentList = getDrumkit()->getInstruments();
	for ( int i = 0; i < pInstrumentList->size(); i++ ) {
		pInstrumentList->get( i )->set_missing_samples( false );
	}
}

void Song::loadTempPatternList( const QString& sFilename )
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

	loadVirtualPatternsFrom( root, false );
	loadPatternGroupVectorFrom( root, false );
}

bool Song::saveTempPatternList( const QString& sFilename ) const
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "sequence" );

	saveVirtualPatternsTo( root, false );
	savePatternGroupVectorTo( root, false );

	return doc.write( sFilename );
}

void Song::setPanLawKNorm( float fKNorm ) {
	if ( fKNorm >= 0. ) {
		m_fPanLawKNorm = fKNorm;
	} else {
		WARNINGLOG("negative kNorm. Set default" );
		m_fPanLawKNorm = Sampler::K_NORM_DEFAULT;
	}
}

std::vector<std::shared_ptr<Note>> Song::getAllNotes() const {

	std::vector< std::shared_ptr<Note> > notes;
	std::set< std::shared_ptr<Pattern> > encounteredPattern;

	auto addNotes = [&notes]( std::shared_ptr<Note> pNote,
							  int nColumnStartTick ) {
		if ( pNote != nullptr ) {
			// Use the copy constructor to not mess
			// with the song itself.
			auto pNoteCopied = std::make_shared<Note>( pNote );

			// The position property of the note specifies its position within
			// the pattern. All we need to do is to add the pattern start tick.
			pNoteCopied->setPosition( pNoteCopied->getPosition() +
									  nColumnStartTick );
			notes.push_back( pNoteCopied );
		}
	};

	long nColumnStartTick = 0;
	for ( const auto& ppColumn : *m_pPatternGroupSequence ) {
		if ( ppColumn == nullptr || ppColumn->size() == 0 ) {
			// An empty column with no patterns selected (but not the
			// end of the song).
			nColumnStartTick += MAX_NOTES;
			continue;
		}

		for ( const auto& ppPattern : *ppColumn ) {
			if ( ppPattern == nullptr ) {
				continue;
			}

			// Add all notes of this pattern.
			for ( const auto& [ _, ppNote ] : *ppPattern->getNotes() ) {
				addNotes( ppNote, nColumnStartTick );
			}
			encounteredPattern.insert( ppPattern );

			// Add notes of contained virtual patterns as well.
			if ( ppPattern->isVirtual() ) {
				for ( const auto& ppVirtualPattern :
						  *ppPattern->getFlattenedVirtualPatterns() ) {
					// A pattern could be part of several patterns as well as
					// activated directly. We only need to take it into account
					// once.
					if ( encounteredPattern.find( ppVirtualPattern ) !=
						 encounteredPattern.end() ) {
						continue;
					}
					encounteredPattern.insert( ppVirtualPattern );

					for ( const auto& [ _, ppNote ] : *ppVirtualPattern->getNotes() ) {
						addNotes( ppNote, nColumnStartTick );
					}
				}
			}
		}

		nColumnStartTick += ppColumn->longest_pattern_length();
	}

	std::sort( notes.begin(), notes.end(), Note::compareAscending );

	return notes;
}

QString Song::ModeToQString( const Mode& mode ) {
	switch( mode ) {
	case Mode::Pattern:
		return "Pattern";
	case Mode::Song:
		return "Song";
	case Mode::None:
		return "None";
	default:
		return QString( "Unknown mode [%1]" )
			.arg( static_cast<int>(mode) );
	}
}

QString Song::ActionModeToQString( const ActionMode& actionMode ) {
	switch( actionMode ) {
	case ActionMode::selectMode:
		return "selectMode";
	case ActionMode::drawMode:
		return "drawMode";
	case ActionMode::None:
		return "None";
	default:
		return QString( "Unknown actionMode [%1]" )
			.arg( static_cast<int>(actionMode) );
	}
}

QString Song::LoopModeToQString( const LoopMode& loopMode ) {
	switch( loopMode ) {
	case LoopMode::Disabled:
		return "Disabled";
	case LoopMode::Enabled:
		return "Enabled";
	case LoopMode::Finishing:
		return "Finishing";
	default:
		return QString( "Unknown loopMode [%1]" )
			.arg( static_cast<int>(loopMode) );
	}
}

QString Song::PatternModeToQString( const PatternMode& patternMode ) {
	switch( patternMode ) {
	case PatternMode::Stacked:
		return "Stacked";
	case PatternMode::Selected:
		return "Selected";
	case PatternMode::None:
		return "None";
	default:
		return QString( "Unknown patternMode [%1]" )
			.arg( static_cast<int>(patternMode) );
	}
}

QString Song::PlaybackTrackToQString( const PlaybackTrack& playbackTrack ) {
	switch( playbackTrack ) {
	case PlaybackTrack::Unavailable:
		return "Unavailable";
	case PlaybackTrack::Muted:
		return "Muted";
	case PlaybackTrack::Enabled:
		return "Enabled";
	case PlaybackTrack::None:
		return "None";
	default:
		return QString( "Unknown playbackTrack [%1]" )
			.arg( static_cast<int>(playbackTrack) );
	}
}

QString Song::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Song]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_bIsTimelineActivated: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsTimelineActivated ) )
			.append( QString( "%1%2m_bIsMuted: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsMuted ) )
			.append( QString( "%1%2m_resolution: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_resolution ) )
			.append( QString( "%1%2m_fBpm: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fBpm ) )
			.append( QString( "%1%2m_nVersion: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nVersion ) )
			.append( QString( "%1%2m_sName: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sName ) )
			.append( QString( "%1%2m_sAuthor: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sAuthor ) )
			.append( QString( "%1%2m_fVolume: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fVolume ) )
			.append( QString( "%1%2m_fMetronomeVolume: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fMetronomeVolume ) )
			.append( QString( "%1%2m_sNotes: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sNotes ) )
			.append( QString( "%1" ).arg( m_pPatternList->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_pPatternGroupSequence:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& pp : *m_pPatternGroupSequence ) {
			if ( pp != nullptr ) {
				sOutput.append( QString( "%1" ).arg( pp->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		if ( m_pDrumkit != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pDrumkit->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "%1%2m_pDrumkit: nullptr\n" ).arg( sPrefix )
							.arg( s ) );
		}
		sOutput.append( QString( "%1%2m_sFilename: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sFilename ) )
			.append( QString( "%1%2m_loopMode: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( LoopModeToQString( m_loopMode ) ) )
			.append( QString( "%1%2m_patternMode: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( PatternModeToQString( m_patternMode ) ) )
			.append( QString( "%1%2m_fHumanizeTimeValue: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fHumanizeTimeValue ) )
			.append( QString( "%1%2m_fHumanizeVelocityValue: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fHumanizeVelocityValue ) )
			.append( QString( "%1%2m_fSwingFactor: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fSwingFactor ) )
			.append( QString( "%1%2m_bIsModified: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsModified ) )
			.append( QString( "%1%2m_latestRoundRobins\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& mm : m_latestRoundRobins ) {
			sOutput.append( QString( "%1%2%3 : %4\n" ).arg( sPrefix ).arg( s )
					 .arg( mm.first ).arg( mm.second ) );
		}
		sOutput.append( QString( "%1%2m_mode: %3\n" ).arg( sPrefix ).arg( s )
						.arg( ModeToQString( m_mode ) ) )
			.append( QString( "%1%2m_sPlaybackTrackFilename: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sPlaybackTrackFilename ) )
			.append( QString( "%1%2m_bPlaybackTrackEnabled: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bPlaybackTrackEnabled ) )
			.append( QString( "%1%2m_fPlaybackTrackVolume: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPlaybackTrackVolume ) )
			.append( QString( "%1" ).arg( m_pVelocityAutomationPath->toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_license: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_license.toQString( sPrefix + s, bShort ) ) )
			.append( QString( "%1%2m_actionMode: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( ActionModeToQString( m_actionMode ) ) )
			.append( QString( "%1%2m_bIsPatternEditorLocked: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_bIsPatternEditorLocked ) )
			.append( QString( "%1%2m_nPanLawType: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_nPanLawType ) )
			.append( QString( "%1%2m_fPanLawKNorm: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_fPanLawKNorm ) )
			.append( QString( "%1%2m_pTimeline:\n" ).arg( sPrefix ).arg( s ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1" ).arg( m_pTimeline->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
			sOutput.append( QString( "%1%2m_sLastLoadedDrumkitPath: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sLastLoadedDrumkitPath ) );
	} else {

		sOutput = QString( "[Song]" )
			.append( QString( ", m_bIsTimelineActivated: %1" ).arg( m_bIsTimelineActivated ) )
			.append( QString( ", m_bIsMuted: %1" ).arg( m_bIsMuted ) )
			.append( QString( ", m_resolution: %1" ).arg( m_resolution ) )
			.append( QString( ", m_fBpm: %1" ).arg( m_fBpm ) )
			.append( QString( ", m_nVersion: %1" ).arg( m_nVersion ) )
			.append( QString( ", m_sName: %1" ).arg( m_sName ) )
			.append( QString( ", m_sAuthor: %1" ).arg( m_sAuthor ) )
			.append( QString( ", m_fVolume: %1" ).arg( m_fVolume ) )
			.append( QString( ", m_fMetronomeVolume: %1" ).arg( m_fMetronomeVolume ) )
			.append( QString( ", m_sNotes: %1" ).arg( m_sNotes ) )
			.append( QString( "%1" ).arg( m_pPatternList->toQString( sPrefix + s, bShort ) ) )
			.append( QString( ", m_pPatternGroupSequence:" ) );
		for ( const auto& pp : *m_pPatternGroupSequence ) {
			if ( pp != nullptr ) {
				sOutput.append( QString( "%1" ).arg( pp->toQString( sPrefix + s + s, bShort ) ) );
			}
		}
		if ( m_pDrumkit != nullptr ) {
			sOutput.append( QString( "%1" )
							.arg( m_pDrumkit->toQString( sPrefix + s, bShort ) ) );
		} else {
			sOutput.append( ", m_pDrumkit: nullptr" );
		}
		sOutput.append( QString( ", m_sFilename: %1" ).arg( m_sFilename ) )
			.append( QString( ", m_loopMode: %1" )
					 .arg( LoopModeToQString( m_loopMode ) ) )
			.append( QString( ", m_patternMode: %1" )
					 .arg( PatternModeToQString( m_patternMode ) ) )
			.append( QString( ", m_fHumanizeTimeValue: %1" ).arg( m_fHumanizeTimeValue ) )
			.append( QString( ", m_fHumanizeVelocityValue: %1" ).arg( m_fHumanizeVelocityValue ) )
			.append( QString( ", m_fSwingFactor: %1" ).arg( m_fSwingFactor ) )
			.append( QString( ", m_bIsModified: %1" ).arg( m_bIsModified ) )
			.append( QString( ", m_latestRoundRobins" ) );
		for ( const auto& mm : m_latestRoundRobins ) {
			sOutput.append( QString( ", %1 : %4" ).arg( mm.first ).arg( mm.second ) );
		}
		sOutput.append( QString( ", m_mode: %1" )
						.arg( ModeToQString( m_mode ) ) )
			.append( QString( ", m_sPlaybackTrackFilename: %1" ).arg( m_sPlaybackTrackFilename ) )
			.append( QString( ", m_bPlaybackTrackEnabled: %1" ).arg( m_bPlaybackTrackEnabled ) )
			.append( QString( ", m_fPlaybackTrackVolume: %1" ).arg( m_fPlaybackTrackVolume ) )
			.append( QString( ", m_pVelocityAutomationPath: %1" ).arg( m_pVelocityAutomationPath->toQString( sPrefix ) ) )
			.append( QString( ", m_license: %1" ).arg( m_license.toQString( sPrefix, bShort ) ) )
			.append( QString( ", m_actionMode: %1" ).
					 arg( ActionModeToQString( m_actionMode ) ) )
			.append( QString( ", m_bIsPatternEditorLocked: %1" )
					 .arg( m_bIsPatternEditorLocked ) )
			.append( QString( ", m_nPanLawType: %1" ).arg( m_nPanLawType ) )
			.append( QString( ", m_fPanLawKNorm: %1" ).arg( m_fPanLawKNorm ) )
			.append( QString( ", m_pTimeline: " ) );
		if ( m_pTimeline != nullptr ) {
			sOutput.append( QString( "%1\n" ).arg( m_pTimeline->toQString( sPrefix, bShort ) ) );
		} else {
			sOutput.append( QString( "nullptr\n" ) );
		}
			sOutput.append( QString( ", m_sLastLoadedDrumkitPath: %1" )
							.arg( m_sLastLoadedDrumkitPath ) );
	}

	return sOutput;
}
};
