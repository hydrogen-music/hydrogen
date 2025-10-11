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

#include "MidiActionManager.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core {

MidiActionManager::MidiActionManager() : m_nTickIntervalIndex( 0 )
									   , m_bMidiClockReady( false )
									   , m_bPendingStart( false )
									   , m_bWorkerShutdown( false )
{
	m_tickIntervals.resize( MidiActionManager::nMidiClockIntervals );
	for ( int ii = 0; ii < m_tickIntervals.size(); ++ii ) {
		m_tickIntervals[ ii ] = 0;
	}

	m_nLastBpmChangeCCParameter = -1;
	/*
		the m_actionMap holds all Action identifiers which hydrogen is able to interpret.
		it holds pointer to member function
	*/
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BeatCounter,
					   std::make_pair( &MidiActionManager::beatcounter, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmCcRelative,
					   std::make_pair( &MidiActionManager::bpmCcRelative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmDecr,
					   std::make_pair( &MidiActionManager::bpmDecrease, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmFineCcRelative,
					   std::make_pair( &MidiActionManager::bpmFineCcRelative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::BpmIncr,
					   std::make_pair( &MidiActionManager::bpmIncrease, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::ClearPattern,
					   std::make_pair( &MidiActionManager::clearPattern, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::ClearSelectedInstrument,
					   std::make_pair( &MidiActionManager::clearSelectedInstrument, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::CountIn,
					   std::make_pair( &MidiActionManager::countIn, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::CountInPauseToggle,
					   std::make_pair( &MidiActionManager::countInPauseToggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::CountInStopToggle,
					   std::make_pair( &MidiActionManager::countInStopToggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::EffectLevelAbsolute,
					   std::make_pair( &MidiActionManager::effectLevelAbsolute, 2 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::EffectLevelRelative,
					   std::make_pair( &MidiActionManager::effectLevelRelative, 2 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::FilterCutoffLevelAbsolute,
					   std::make_pair( &MidiActionManager::filterCutoffLevelAbsolute, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::GainLevelAbsolute,
					   std::make_pair( &MidiActionManager::gainLevelAbsolute, 3 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::InstrumentPitch,
						std::make_pair( &MidiActionManager::instrumentPitch, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::LoadNextDrumkit,
					   std::make_pair( &MidiActionManager::loadNextDrumkit, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::LoadPrevDrumkit,
					   std::make_pair( &MidiActionManager::loadPrevDrumkit, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::MasterVolumeRelative,
					   std::make_pair( &MidiActionManager::masterVolumeRelative, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::MasterVolumeAbsolute,
					   std::make_pair( &MidiActionManager::masterVolumeAbsolute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Mute,
					   std::make_pair( &MidiActionManager::mute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::MuteToggle,
					   std::make_pair( &MidiActionManager::muteToggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::NextBar,
					   std::make_pair( &MidiActionManager::nextBar, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PanAbsolute,
					   std::make_pair( &MidiActionManager::panAbsolute, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PanAbsoluteSym,
					   std::make_pair( &MidiActionManager::panAbsoluteSym, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PanRelative,
					   std::make_pair( &MidiActionManager::panRelative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Pause,
					   std::make_pair( &MidiActionManager::pause, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PitchLevelAbsolute,
					   std::make_pair( &MidiActionManager::pitchLevelAbsolute, 3 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Play,
					   std::make_pair( &MidiActionManager::play, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlaylistNextSong,
					   std::make_pair( &MidiActionManager::playlistNextSong, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlaylistPrevSong,
					   std::make_pair( &MidiActionManager::playlistPreviousSong, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlaylistSong,
					   std::make_pair( &MidiActionManager::playlistSong, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlayPauseToggle,
					   std::make_pair( &MidiActionManager::playPauseToggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PlayStopToggle,
					   std::make_pair( &MidiActionManager::playStopToggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::PreviousBar,
					   std::make_pair( &MidiActionManager::previousBar, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordExit,
					   std::make_pair( &MidiActionManager::recordExit, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordReady,
					   std::make_pair( &MidiActionManager::recordReady, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordStrobe,
					   std::make_pair( &MidiActionManager::recordStrobe, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RecordStrobeToggle,
					   std::make_pair( &MidiActionManager::recordStrobeToggle, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::RedoAction,
					   std::make_pair( &MidiActionManager::redoAction, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectAndPlayPattern,
					   std::make_pair( &MidiActionManager::selectAndPlayPattern, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectInstrument,
					   std::make_pair( &MidiActionManager::selectInstrument, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectNextPattern,
					   std::make_pair( &MidiActionManager::selectNextPattern, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectNextPatternCcAbsolute,
					   std::make_pair( &MidiActionManager::selectNextPatternCcAbsolute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectNextPatternRelative,
					   std::make_pair( &MidiActionManager::selectNextPatternRelative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectOnlyNextPattern,
					   std::make_pair( &MidiActionManager::selectOnlyNextPattern, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::SelectOnlyNextPatternCcAbsolute,
					   std::make_pair( &MidiActionManager::selectOnlyNextPatternCcAbsolute, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Stop,
					   std::make_pair( &MidiActionManager::stop, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripMuteToggle,
					   std::make_pair( &MidiActionManager::stripMuteToggle, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripSoloToggle,
					   std::make_pair( &MidiActionManager::stripSoloToggle, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripVolumeAbsolute,
					   std::make_pair( &MidiActionManager::stripVolumeAbsolute, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::StripVolumeRelative,
					   std::make_pair( &MidiActionManager::stripVolumeRelative, 1 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::TapTempo,
					   std::make_pair( &MidiActionManager::tapTempo, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::TimingClockTick,
					   std::make_pair( &MidiActionManager::timingClockTick, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::ToggleMetronome,
					   std::make_pair( &MidiActionManager::toggleMetronome, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::UndoAction,
					   std::make_pair( &MidiActionManager::undoAction, 0 ) ));
	m_midiActionMap.insert(
		std::make_pair( MidiAction::Type::Unmute,
					   std::make_pair( &MidiActionManager::unmute, 0 ) ));

	for ( const auto& ppAction : m_midiActionMap ) {
		auto ret = m_midiActions.insert( ppAction.first );
		if ( ! ret.second ) {
			ERRORLOG( QString( "Unable to insert [%1]" )
					  .arg( MidiAction::typeToQString( ppAction.first ) ) );
		}
	}

	m_pWorkerThread = std::make_shared< std::thread >(
		MidiActionManager::workerThread, ( void* )this );
}

MidiActionManager::~MidiActionManager() {
	m_bWorkerShutdown = true;
	if ( m_pWorkerThread != nullptr ) {
		{
			std::scoped_lock lock{ m_workerThreadMutex };
			m_workerThreadCV.notify_all();
		}
		m_pWorkerThread->join();
		m_pWorkerThread = nullptr;
	}
}

bool MidiActionManager::play( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Ready ||
		 pHydrogen->getAudioEngine()->getState() == AudioEngine::State::CountIn ) {
		pHydrogen->sequencerPlay();
	}
	return true;
}

bool MidiActionManager::pause( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->sequencerStop();
	return true;
}

bool MidiActionManager::stop( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->sequencerStop();
	return CoreActionController::locateToColumn( 0 );
}

bool MidiActionManager::playPauseToggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		return false;
	}

	switch ( pHydrogen->getAudioEngine()->getState() ) {
	case AudioEngine::State::Ready:
	case AudioEngine::State::CountIn:
		pHydrogen->sequencerPlay();
		break;

	case AudioEngine::State::Playing:
		pHydrogen->sequencerStop();
		break;

	default:
		ERRORLOG( "AudioEngine not ready yet." );
		return false;
	}

	return true;
}

bool MidiActionManager::playStopToggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen->getSong() == nullptr ) {
		return false;
	}
	
	switch ( pHydrogen->getAudioEngine()->getState() ) {
	case AudioEngine::State::Ready:
	case AudioEngine::State::CountIn:
		pHydrogen->sequencerPlay();
		break;

	case AudioEngine::State::Playing:
		pHydrogen->sequencerStop();
		CoreActionController::locateToColumn( 0 );
		break;

	default:
		ERRORLOG( "AudioEngine not ready yet." );
		return false;
	}

	return true;
}

//mutes the master, not a single strip
bool MidiActionManager::mute( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return CoreActionController::setMasterIsMuted( true );
}

bool MidiActionManager::unmute( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return CoreActionController::setMasterIsMuted( false );
}

bool MidiActionManager::muteToggle( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return CoreActionController::setMasterIsMuted( !pHydrogen->getSong()->getIsMuted() );
}

bool MidiActionManager::stripMuteToggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	return CoreActionController::setStripIsMuted(
		nLine, !pInstr->isMuted(), false );
}

bool MidiActionManager::stripSoloToggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	return CoreActionController::setStripIsSoloed(
		nLine, !pInstr->isSoloed(), false );
}

bool MidiActionManager::beatcounter( std::shared_ptr<MidiAction> pAction ) {
	if ( pAction == nullptr ) {
		return false;
	}

	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	return pHydrogen->handleBeatCounter( pAction->getTimePoint() );
}

bool MidiActionManager::tapTempo( std::shared_ptr<MidiAction> pAction ) {
	if ( pAction == nullptr ) {
		return false;
	}

	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	pHydrogen->onTapTempoAccelEvent( pAction->getTimePoint() );
	return true;
}

bool MidiActionManager::timingClockTick( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	if ( pAction == nullptr ) {
		return false;
	}

	const auto tick = QTime::currentTime();
	// In seconds
	const double fInterval = std::chrono::duration_cast<std::chrono::microseconds>(
		pAction->getTimePoint() - m_lastTick ).count() / 1000.0 / 1000.0;
	m_lastTick = pAction->getTimePoint();

	if ( fInterval >= 60.0 * 2 / static_cast<float>(MIN_BPM) / 24.0 ) {
		// Waiting time was too long. We start all over again.
		m_bMidiClockReady = false;
		m_nTickIntervalIndex = 0;

		// We return early since we do not want this large interval to be part
		// of the average.
		return true;
	}

	m_tickIntervals[
		std::clamp( m_nTickIntervalIndex, 0,
					static_cast<int>(m_tickIntervals.size()) - 1 ) ] = fInterval;

	++m_nTickIntervalIndex;
	if ( m_nTickIntervalIndex >= MidiActionManager::nMidiClockIntervals ) {
		m_nTickIntervalIndex = 0;

		// We got at least 10 messages. Let's start averaging.
		if ( ! m_bMidiClockReady ) {
			m_bMidiClockReady = true;
		}
	}

	if ( m_bMidiClockReady ) {
		double fAverageInterval = 0;
		for ( const auto& nnInterval : m_tickIntervals ) {
			fAverageInterval += static_cast<double>(nnInterval);
		}
		const float fBpm = static_cast<float>(
			60.0 * static_cast<double>(m_tickIntervals.size()) /
			fAverageInterval / 24.0 );

		pAudioEngine->lock( RIGHT_HERE );
		pAudioEngine->setNextBpm( fBpm );
		pAudioEngine->unlock();
	}

	if ( m_bPendingStart ) {
		if ( pHydrogen->getAudioEngine()->getState() ==
			 AudioEngine::State::Ready ) {
			pHydrogen->sequencerPlay();
		}

		m_bPendingStart = false;
	}

	return true;
}

void MidiActionManager::resetTimingClockTicks() {
	m_bMidiClockReady = false;
	m_nTickIntervalIndex = 0;
	m_lastTick = TimePoint();
}

bool MidiActionManager::selectNextPattern( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return nextPatternSelection( pAction->getParameter1().toInt(&ok,10) );
}


bool MidiActionManager::selectNextPatternRelative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	bool ok;
	return nextPatternSelection( pHydrogen->getSelectedPatternNumber() +
								 pAction->getParameter1().toInt(&ok,10) );
}

bool MidiActionManager::selectNextPatternCcAbsolute( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return nextPatternSelection( pAction->getValue().toInt(&ok,10) );
}

bool MidiActionManager::nextPatternSelection( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
    
	if ( nPatternNumber > pSong->getPatternList()->size() - 1 ||
		nPatternNumber < 0 ) {
		ERRORLOG( QString( "Provided value [%1] out of bound [0,%2]" ).arg( nPatternNumber )
				  .arg( pSong->getPatternList()->size() - 1 ) );
		return false;
	}
	
	if ( pHydrogen->getPatternMode() == Song::PatternMode::Selected ) {
		pHydrogen->setSelectedPatternNumber(
			nPatternNumber, true, Event::Trigger::Default );
	}
	else if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
		pHydrogen->toggleNextPattern( nPatternNumber );
	}
	
	return true;
}

bool MidiActionManager::selectOnlyNextPattern( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return onlyNextPatternSelection( pAction->getParameter1().toInt(&ok,10) );
}

bool MidiActionManager::selectOnlyNextPatternCcAbsolute( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	return onlyNextPatternSelection( pAction->getValue().toInt(&ok,10) );
}

bool MidiActionManager::onlyNextPatternSelection( int nPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( nPatternNumber > pSong->getPatternList()->size() -1 ||
		nPatternNumber < 0 ) {
		if ( pHydrogen->getPatternMode() == Song::PatternMode::Selected ) {
			ERRORLOG( QString( "Provided pattern number [%1] out of bound [0,%2]." )
					  .arg( nPatternNumber )
					  .arg( pSong->getPatternList()->size() - 1 ) );
			return false;
		}
		else {
			INFOLOG( QString( "Provided pattern number [%1] out of bound [0,%2]. All patterns will be deselected." )
					 .arg( nPatternNumber )
					 .arg( pSong->getPatternList()->size() - 1 ) );
		}
	}
	
	if ( pHydrogen->getPatternMode() == Song::PatternMode::Selected ) {
		return nextPatternSelection( nPatternNumber );
	}
	
	return pHydrogen->flushAndAddNextPattern( nPatternNumber );
}

bool MidiActionManager::selectAndPlayPattern( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( ! selectNextPattern( pAction ) ) {
		return false;
	}

	if ( pHydrogen->getAudioEngine()->getState() == AudioEngine::State::Ready ||
		 pHydrogen->getAudioEngine()->getState() == AudioEngine::State::CountIn ) {
		pHydrogen->sequencerPlay();
	}

	return true;
}

bool MidiActionManager::selectInstrument( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int  nInstrumentNumber = pAction->getValue().toInt(&ok,10) ;

	if ( pSong->getDrumkit()->getInstruments()->size() < nInstrumentNumber ) {
		nInstrumentNumber = pSong->getDrumkit()->getInstruments()->size() -1;
	} else if ( nInstrumentNumber < 0 ) {
		nInstrumentNumber = 0;
	}
	
	pHydrogen->setSelectedInstrumentNumber( nInstrumentNumber );
	return true;
}

bool MidiActionManager::effectLevelAbsolute( std::shared_ptr<MidiAction> pAction) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int fx_param = pAction->getValue().toInt(&ok,10);
	int fx_id = pAction->getParameter2().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	float fValue = 0;
	if ( fx_param != 0 ) {
		fValue = static_cast<float>(fx_param) / 127.0;
	}

	return CoreActionController::setStripEffectLevel( nLine, fx_id, fValue, true );
}

bool MidiActionManager::effectLevelRelative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int fx_param = pAction->getValue().toInt(&ok,10);
	int fx_id = pAction->getParameter2().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	float fValue = 0;
	if ( fx_param != 0 ) {
		if ( fx_param == 1 && pInstr->getFxLevel( fx_id ) <= 0.95 ) {
			fValue = pInstr->getFxLevel( fx_id ) + 0.05;
		}
		else if ( pInstr->getFxLevel( fx_id ) >= 0.05 ) {
			fValue = pInstr->getFxLevel( fx_id ) - 0.05;
		}
	}

	return CoreActionController::setStripEffectLevel( nLine, fx_id, fValue, true );
}

//sets the volume of a master output to a given level (percentage)
bool MidiActionManager::masterVolumeAbsolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nVolume = pAction->getValue().toInt(&ok,10);

	if ( nVolume != 0 ) {
		pSong->setVolume( 1.5* ( (float) (nVolume / 127.0 ) ));
	} else {
		pSong->setVolume( 0 );
	}

	return true;
}

//increments/decrements the volume of the whole song
bool MidiActionManager::masterVolumeRelative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nVolume = pAction->getValue().toInt(&ok,10);

	if ( nVolume != 0 ) {
		if ( nVolume == 1 && pSong->getVolume() < 1.5 ) {
			pSong->setVolume( pSong->getVolume() + 0.05 );
		} else if ( pSong->getVolume() >= 0.0 ) {
			pSong->setVolume( pSong->getVolume() - 0.05 );
		}
	} else {
		pSong->setVolume( 0 );
	}

	return true;
}

//sets the volume of a mixer strip to a given level (percentage)
bool MidiActionManager::stripVolumeAbsolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int nVolume = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	if ( nVolume != 0 ) {
		pInstr->setVolume( 1.5* ( (float) (nVolume / 127.0 ) ));
	} else {
		pInstr->setVolume( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

//increments/decrements the volume of one mixer strip
bool MidiActionManager::stripVolumeRelative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();

	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int nVolume = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	
	if ( pInstr == nullptr) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	if( nVolume != 0 ) {
		if ( nVolume == 1 && pInstr->getVolume() < 1.5 ) {
			pInstr->setVolume( pInstr->getVolume() + 0.1 );
		}
		else if( pInstr->getVolume() >= 0.0 ){
			pInstr->setVolume( pInstr->getVolume() - 0.1 );
		}
	}
	else {
		pInstr->setVolume( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

// sets the absolute panning of a given mixer channel
bool MidiActionManager::panAbsolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	pInstr->setPanWithRangeFrom0To1( (float) pan_param / 127.f );
	
	pHydrogen->setSelectedInstrumentNumber(nLine);

	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

// sets the absolute panning of a given mixer channel
bool MidiActionManager::panAbsoluteSym( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}

	pInstr->setPan( (float) pan_param / 127.f );
	
	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}


// changes the panning of a given mixer channel
// this is useful if the panning is set by a rotary control knob
bool MidiActionManager::panRelative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pan_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	float fPan = pInstr->getPan();

	if ( pan_param == 1 && fPan < PAN_MAX ) {
		pInstr->setPan( fPan + 0.1 );
	}
	else if ( pan_param != 1 && fPan > PAN_MIN ) {
		pInstr->setPan( fPan - 0.1 );
	}

	pHydrogen->setSelectedInstrumentNumber(nLine);
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

bool MidiActionManager::gainLevelAbsolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int gain_param = pAction->getValue().toInt(&ok,10);
	int component_id = pAction->getParameter2().toInt(&ok,10);
	int layer_id = pAction->getParameter3().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();
	
	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	auto pComponent =  pInstr->getComponent( component_id );
	if( pComponent == nullptr) {
		ERRORLOG( QString( "Unable to retrieve component (Par. 2) [%1]" ).arg( component_id ) );
		return false;
	}
	
	auto pLayer = pComponent->getLayer( layer_id );
	if( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve layer (Par. 3) [%1]" ).arg( layer_id ) );
		return false;
	}
	
	if ( gain_param != 0 ) {
		pLayer->setGain( 5.0* ( (float) (gain_param / 127.0 ) ) );
	} else {
		pLayer->setGain( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );
	
	return true;
}

bool MidiActionManager::pitchLevelAbsolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int pitch_param = pAction->getValue().toInt(&ok,10);
	int component_id = pAction->getParameter2().toInt(&ok,10);
	int layer_id = pAction->getParameter3().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	auto pComponent =  pInstr->getComponent( component_id );
	if( pComponent == nullptr) {
		ERRORLOG( QString( "Unable to retrieve component (Par. 2) [%1]" ).arg( component_id ) );
		return false;
	}
	
	auto pLayer = pComponent->getLayer( layer_id );
	if( pLayer == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve layer (Par. 3) [%1]" ).arg( layer_id ) );
		return false;
	}
	
	if ( pitch_param != 0 ) {
		pLayer->setPitch(
			( Instrument::fPitchMax - Instrument::fPitchMin ) *
			( (float) (pitch_param / 127.0 ) ) + Instrument::fPitchMin );
	} else {
		pLayer->setPitch( Instrument::fPitchMin );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );

	return true;
}

bool MidiActionManager::instrumentPitch( std::shared_ptr<MidiAction> pAction ) {

	bool ok;
	float fPitch;
	const int nInstrument = pAction->getParameter1().toInt(&ok,10);
	const int nPitchMidi = pAction->getValue().toInt(&ok,10);
	if ( nPitchMidi != 0 ) {
		fPitch = ( Instrument::fPitchMax - Instrument::fPitchMin ) *
			( (float) (nPitchMidi / 127.0 ) ) + Instrument::fPitchMin;
	} else {
		fPitch = Instrument::fPitchMin;
	}

	return CoreActionController::setInstrumentPitch( nInstrument, fPitch );
}

bool MidiActionManager::filterCutoffLevelAbsolute( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	
	// Preventive measure to avoid bad things.
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	bool ok;
	int nLine = pAction->getParameter1().toInt(&ok,10);
	int filter_cutoff_param = pAction->getValue().toInt(&ok,10);

	auto pInstrList = pSong->getDrumkit()->getInstruments();

	auto pInstr = pInstrList->get( nLine );
	if( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument (Par. 1) [%1]" ).arg( nLine ) );
		return false;
	}
	
	pInstr->setFilterActive( true );
	if( filter_cutoff_param != 0 ) {
		pInstr->setFilterCutoff( ( (float) (filter_cutoff_param / 127.0 ) ) );
	} else {
		pInstr->setFilterCutoff( 0 );
	}
	
	pHydrogen->setSelectedInstrumentNumber( nLine );
	EventQueue::get_instance()->pushEvent( Event::Type::InstrumentParametersChanged, nLine );
	
	return true;
}


/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpmCcRelative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	if ( pHydrogen->getTempoSource() != Hydrogen::Tempo::Song ) {
		EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );
		return true;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	//this MidiAction should be triggered only by CC commands

	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	if ( m_nLastBpmChangeCCParameter >= cc_param &&
		 fBpm - mult > MIN_BPM ) {
		CoreActionController::setBpm( fBpm - 1*mult );
	}

	if ( m_nLastBpmChangeCCParameter < cc_param
		 && fBpm + mult < MAX_BPM ) {
		CoreActionController::setBpm( fBpm + 1*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

/*
 * increments/decrements the BPM
 * this is useful if the bpm is set by a rotary control knob
 */
bool MidiActionManager::bpmFineCcRelative( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	if ( pHydrogen->getTempoSource() != Hydrogen::Tempo::Song ) {
		EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );
		return true;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	//this MidiAction should be triggered only by CC commands
	bool ok;
	int mult = pAction->getParameter1().toInt(&ok,10);
	//this value should be 1 to decrement and something other then 1 to increment the bpm
	int cc_param = pAction->getValue().toInt(&ok,10);

	if( m_nLastBpmChangeCCParameter == -1) {
		m_nLastBpmChangeCCParameter = cc_param;
	}

	if ( m_nLastBpmChangeCCParameter >= cc_param &&
		 fBpm - mult > MIN_BPM ) {
		CoreActionController::setBpm( fBpm - 0.01*mult );
	}
	if ( m_nLastBpmChangeCCParameter < cc_param
		 && fBpm + mult < MAX_BPM ) {
		CoreActionController::setBpm( fBpm + 0.01*mult );
	}

	m_nLastBpmChangeCCParameter = cc_param;
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

bool MidiActionManager::bpmIncrease( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	if ( pHydrogen->getTempoSource() != Hydrogen::Tempo::Song ) {
		EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );
		return true;
	}
	
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	const float fMult = pAction->getParameter1().toFloat();

	CoreActionController::setBpm( fBpm + 1 * fMult );
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

bool MidiActionManager::bpmDecrease( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	if ( pHydrogen->getTempoSource() != Hydrogen::Tempo::Song ) {
		EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );
		return true;
	}
	
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const float fBpm = pAudioEngine->getTransportPosition()->getBpm();

	const float fMult = pAction->getParameter1().toFloat();

	CoreActionController::setBpm( fBpm - 1 * fMult );
	
	EventQueue::get_instance()->pushEvent( Event::Type::TempoChanged, -1 );

	return true;
}

bool MidiActionManager::nextBar( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	auto pAudioEngine = pHydrogen->getAudioEngine();

	if ( pSong->getMode() == Song::Mode::Pattern ) {
		// Restart transport at the beginning of the pattern
		CoreActionController::locateToColumn( 0 );
		if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
			pAudioEngine->lock( RIGHT_HERE );
			pAudioEngine->updatePlayingPatterns( Event::Trigger::Default );
			pAudioEngine->unlock();
		}
	}
	else {
		const int nTotalColumns = pSong->getPatternGroupVector()->size();
		int nNewColumn = 1 +
			std::max( 0, pAudioEngine->getTransportPosition()->getColumn() );
		if ( nNewColumn >= nTotalColumns ) {
			if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
				// Transport exceeds length of the song and is wrapped
				// to the beginning again.
				CoreActionController::locateToColumn( 0 );
			}
			else {
				// With loop mode disabled this command won't have any
				// effect in the last column.
			}
		}
		else {
			CoreActionController::locateToColumn( nNewColumn );
		}
	}
	
	return true;
}


bool MidiActionManager::previousBar( std::shared_ptr<MidiAction>  ) {
	auto pHydrogen = Hydrogen::get_instance();

	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	if ( pSong->getMode() == Song::Mode::Pattern ) {
		// Restart transport at the beginning of the pattern. Does not
		// trigger activation of pending stacked patterns.
		CoreActionController::locateToColumn( 0 );
	}
	else {
		int nNewColumn = pHydrogen->getAudioEngine()->
			getTransportPosition()->getColumn() - 1;

		if ( nNewColumn < 0 ) {
			if ( pSong->getLoopMode() == Song::LoopMode::Enabled ) {
				// In case the song is looped, assume periodic
				// boundary conditions and move to the last column.
				CoreActionController::locateToColumn(
					pSong->getPatternGroupVector()->size() - 1 );
			}
			else {
				CoreActionController::locateToColumn( 0 );
			}
		}
		else {
			CoreActionController::locateToColumn( nNewColumn );
		}
	}
	return true;
}

bool MidiActionManager::setSongFromPlaylist( int nSongNumber ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pPlaylist = pHydrogen->getPlaylist();
	if ( pPlaylist == nullptr ) {
		ERRORLOG( "Invalid current playlist" );
		return false;
	}

	if ( nSongNumber >= 0 && nSongNumber <= pPlaylist->size() - 1 ) {
		if ( pPlaylist->getActiveSongNumber() != nSongNumber ) {
			auto pSong = CoreActionController::loadSong(
				pPlaylist->getSongFileNameByNumber( nSongNumber ) );

			if ( pSong == nullptr ||
				 ! CoreActionController::setSong( pSong ) ) {
				ERRORLOG( QString( "Unable to set song [%1] of playlist" )
						  .arg( nSongNumber ) );
				return false;
			}
			CoreActionController::activatePlaylistSong( nSongNumber );
		}
	} else {
		// Preventive measure to avoid bad things.
		if ( pHydrogen->getPlaylist()->size() == 0 ) {
			___ERRORLOG( QString( "No songs added to the current playlist yet" ) );
		}
		else {
			___ERRORLOG( QString( "Provided song number [%1] out of bound [0,%2]" )
						 .arg( nSongNumber )
						 .arg( pHydrogen->getPlaylist()->size() - 1 ) );
		}
		return false;
	}
	return true;
}

bool MidiActionManager::playlistSong( std::shared_ptr<MidiAction> pAction ) {
	bool ok;
	int songnumber = pAction->getParameter1().toInt(&ok,10);
	return setSongFromPlaylist( songnumber );
}

bool MidiActionManager::playlistNextSong( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	int songnumber = pHydrogen->getPlaylist()->getActiveSongNumber();
	return setSongFromPlaylist( ++songnumber );
}

bool MidiActionManager::playlistPreviousSong( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	int songnumber = pHydrogen->getPlaylist()->getActiveSongNumber();
	return setSongFromPlaylist( --songnumber );
}

bool MidiActionManager::recordReady( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}
	
	if ( pHydrogen->getAudioEngine()->getState() != AudioEngine::State::Playing ) {
		return CoreActionController::toggleRecordMode();
	}
	return true;
}

bool MidiActionManager::recordStrobeToggle( std::shared_ptr<MidiAction>  ) {
	return CoreActionController::toggleRecordMode();
}

bool MidiActionManager::recordStrobe( std::shared_ptr<MidiAction>  ) {
	return CoreActionController::activateRecordMode( true );
}

bool MidiActionManager::recordExit( std::shared_ptr<MidiAction> ) {
	return CoreActionController::activateRecordMode( false );
}

bool MidiActionManager::toggleMetronome( std::shared_ptr<MidiAction> ) {
	auto pHydrogen = Hydrogen::get_instance();

	// Preventive measure to avoid bad things.
	if ( pHydrogen->getSong() == nullptr ) {
		ERRORLOG( "No song set yet" );
		return false;
	}

	// Use the wrapper in CAC over a plain setting of the parameter in
	// order to send MIDI feedback
	CoreActionController::setMetronomeIsActive(
		! Preferences::get_instance()->m_bUseMetronome );
	
	return true;
}

bool MidiActionManager::undoAction( std::shared_ptr<MidiAction> ) {
	EventQueue::get_instance()->pushEvent( Event::Type::UndoRedo, 0);// 0 = undo
	return true;
}

bool MidiActionManager::redoAction( std::shared_ptr<MidiAction> ) {
	EventQueue::get_instance()->pushEvent( Event::Type::UndoRedo, 1);// 1 = redo
	return true;
}

bool MidiActionManager::loadNextDrumkit( std::shared_ptr<MidiAction> ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	// Pass copy to allow kit in the SoundLibraryDatabase to stay in a pristine
	// shape.
	return CoreActionController::setDrumkit(
		std::make_shared<Drumkit>(
			pHydrogen->getSoundLibraryDatabase()->getNextDrumkit() ) );
}

bool MidiActionManager::loadPrevDrumkit( std::shared_ptr<MidiAction> ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	// Pass copy to allow kit in the SoundLibraryDatabase to stay in a pristine
	// shape.
	return CoreActionController::setDrumkit(
		std::make_shared<Drumkit>(
			pHydrogen->getSoundLibraryDatabase()->getPreviousDrumkit() ) );
}

int MidiActionManager::getParameterNumber( const MidiAction::Type& type ) const {
	auto foundActionPair = m_midiActionMap.find( type );
	if ( foundActionPair != m_midiActionMap.end() ) {
		return foundActionPair->second.second;
	}
	else {
		ERRORLOG( QString( "MIDI MidiAction type [%1] couldn't be found" )
				  .arg( MidiAction::typeToQString( type ) ) );
	}
		
	return -1;
}

bool MidiActionManager::clearSelectedInstrument( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	const int nInstr = pHydrogen->getSelectedInstrumentNumber();
	if ( nInstr == -1 ) {
		WARNINGLOG( "No instrument selected" );
		return false;
	}

	return CoreActionController::clearInstrumentInPattern( nInstr );
}

bool MidiActionManager::clearPattern( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();

	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "no song set" );
		return false;
	}

	int nPattern = pHydrogen->getSelectedPatternNumber();

	auto pPattern = pSong->getPatternList()->get( nPattern );
	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Couldn't find pattern [%1]" ).arg( nPattern ) );
		return false;
	}

	pPattern->clear( true );

	EventQueue::get_instance()->pushEvent( Event::Type::PatternModified, 0 );

	return true;
}

bool MidiActionManager::countIn( std::shared_ptr<MidiAction> ) {
	return CoreActionController::startCountIn();
}

bool MidiActionManager::countInPauseToggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	switch ( pAudioEngine->getState() ) {
	case AudioEngine::State::Ready:
		CoreActionController::startCountIn();
		break;

	case AudioEngine::State::CountIn:
		pAudioEngine->stop();
		break;

	case AudioEngine::State::Playing:
		pHydrogen->sequencerStop();
		break;

	default:
		ERRORLOG( "AudioEngine not ready yet." );
		return false;
	}

	return true;
}

bool MidiActionManager::countInStopToggle( std::shared_ptr<MidiAction> pAction ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	switch ( pAudioEngine->getState() ) {
	case AudioEngine::State::Ready:
		CoreActionController::startCountIn();
		break;

	case AudioEngine::State::CountIn:
		pAudioEngine->stop();
		break;

	case AudioEngine::State::Playing:
		CoreActionController::locateToColumn( 0 );
		pHydrogen->sequencerStop();
		break;

	default:
		ERRORLOG( "AudioEngine not ready yet." );
		return false;
	}

	return true;
}

bool MidiActionManager::handleMidiActionsAsync( const std::vector<std::shared_ptr<MidiAction>>& midiActions ) {

	bool bResult = false;
	
	for ( const auto& ppMidiAction : midiActions ) {
		if ( ppMidiAction != nullptr ) {
			if ( handleMidiActionAsync( ppMidiAction ) ) {
				bResult = true;
			}
		}
	}

	return bResult;
}

bool MidiActionManager::handleMidiActionAsync( const std::shared_ptr<MidiAction> pAction ) {

	auto pHydrogen = Hydrogen::get_instance();
	if ( pAction == nullptr || m_pWorkerThread == nullptr ) {
		return false;
	}

	//pAction->timePoint = Clock::now();

	// Check whether there is an actual action associated with the MidiAction.
	if ( m_midiActionMap.find( pAction->getType() ) == m_midiActionMap.end() ) {
		return false;
	}

    std::scoped_lock lock{ m_workerThreadMutex };

	m_actionQueue.push_back( pAction );
	if ( m_actionQueue.size() > MidiActionManager::nActionQueueMaxSize ) {
		QString sWarning( "Message queue is full. Dropping first message" );
		const auto pAction = m_actionQueue.front();
		if ( pAction != nullptr ) {
			sWarning.append( QString( " %1" ).arg( pAction->toQString() ) );
		}
		m_actionQueue.pop_front();
		WARNINGLOG( sWarning );
	}

	m_workerThreadCV.notify_all();

	return true;
}

bool MidiActionManager::handleMidiActionSync( const std::shared_ptr<MidiAction> pAction ) {

	auto pHydrogen = Hydrogen::get_instance();
	/*
		return false if Midiaction is null
		(for example if no MidiAction exists for an event)
	*/
	if ( pAction == nullptr ) {
		return false;
	}

	auto foundActionPair = m_midiActionMap.find( pAction->getType() );
	if( foundActionPair != m_midiActionMap.end() ) {
		action_f Midiaction = foundActionPair->second.first;
		return (this->*Midiaction)(pAction);
	}
	else {
		ERRORLOG( QString( "MIDI MidiAction type [%1] couldn't be found" )
				  .arg( MidiAction::typeToQString( pAction->getType() ) ) );
	}

	return false;
}

int MidiActionManager::getActionQueueSize() {
	std::scoped_lock lock{ m_workerThreadMutex };
	return m_actionQueue.size();
}

void MidiActionManager::workerThread( void* pInstance ) {
	auto pMidiActionManager = static_cast<MidiActionManager*>( pInstance );
	if ( pMidiActionManager == nullptr ) {
		ERRORLOG( "Invalid instance provided. Shutting down." );
		return;
	}

	while ( ! pMidiActionManager->m_bWorkerShutdown ) {
		std::unique_lock lock{ pMidiActionManager->m_workerThreadMutex };
		pMidiActionManager->m_workerThreadCV.wait(
			lock, [&]{ return pMidiActionManager->m_actionQueue.size() > 0 ||
					pMidiActionManager->m_bWorkerShutdown; } );

		if ( pMidiActionManager->m_bWorkerShutdown ) {
			return;
		}

		while ( pMidiActionManager->m_actionQueue.size() > 0 ) {
			const auto pAction = pMidiActionManager->m_actionQueue.front();
			pMidiActionManager->m_actionQueue.pop_front();
			if ( pAction == nullptr ) {
				continue;
			}

			auto foundActionPair = pMidiActionManager->m_midiActionMap.find(
				pAction->getType() );
			if ( foundActionPair != pMidiActionManager->m_midiActionMap.end() ) {
				auto midiAction = foundActionPair->second.first;
				(pMidiActionManager->*midiAction)( pAction );
			}
			else {
				ERRORLOG( QString( "MIDI MidiAction type [%1] couldn't be found" )
						  .arg( MidiAction::typeToQString(
									pAction->getType() ) ) );
			}
		}
	}
}

};
