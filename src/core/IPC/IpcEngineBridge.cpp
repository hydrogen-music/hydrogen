/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/IPC/IpcEngineBridge.h>

#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core {

bool IpcEngineBridge::dispatchCommand( const IpcMessage& msg,
									   Hydrogen* pHydrogen ) {
	if ( pHydrogen == nullptr ) {
		return false;
	}
	auto pController = pHydrogen->getCoreActionController();
	if ( pController == nullptr ) {
		return false;
	}
	const QVector<QVariant>& args = msg.getArgs();

	switch ( msg.getOpcode() ) {
	case IpcOpcode::Play:
		pHydrogen->sequencerPlay();
		return true;
	case IpcOpcode::Stop:
		pHydrogen->sequencerStop();
		return true;
	case IpcOpcode::Quit:
		return pController->quit();
	case IpcOpcode::SetBpm:
		if ( args.size() >= 1 ) {
			return pController->setBpm( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::SetMasterVolume:
		if ( args.size() >= 1 ) {
			return pController->setMasterVolume( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::SetMasterIsMuted:
		if ( args.size() >= 1 ) {
			return pController->setMasterIsMuted( args[0].toBool() );
		}
		return false;
	case IpcOpcode::SetMetronomeIsActive:
		if ( args.size() >= 1 ) {
			return pController->setMetronomeIsActive( args[0].toBool() );
		}
		return false;
	case IpcOpcode::LocateToColumn:
		if ( args.size() >= 1 ) {
			return pController->locateToColumn( args[0].toInt() );
		}
		return false;
	case IpcOpcode::LocateToTick:
		if ( args.size() >= 2 ) {
			return pController->locateToTick(
				static_cast<long>( args[0].toLongLong() ), args[1].toBool() );
		}
		return false;
	case IpcOpcode::SelectPattern:
		if ( args.size() >= 1 ) {
			return pController->selectPattern( args[0].toInt() );
		}
		return false;
	case IpcOpcode::SetStripVolume:
		if ( args.size() >= 3 ) {
			return pController->setStripVolume(
				args[0].toInt(), args[1].toFloat(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::SetStripPan:
		if ( args.size() >= 3 ) {
			return pController->setStripPan(
				args[0].toInt(), args[1].toFloat(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::ActivateLoopMode:
		if ( args.size() >= 1 ) {
			return pController->activateLoopMode( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ActivateSongMode:
		if ( args.size() >= 1 ) {
			return pController->activateSongMode( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ActivateRecordMode:
		if ( args.size() >= 1 ) {
			return pController->activateRecordMode( args[0].toBool() );
		}
		return false;
	case IpcOpcode::AddTempoMarker:
		if ( args.size() >= 2 ) {
			return pController->addTempoMarker( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::AddTag:
		if ( args.size() >= 2 ) {
			return pController->addTag( args[0].toInt(), args[1].toString() );
		}
		return false;
	case IpcOpcode::NewPattern:
		if ( args.size() >= 1 ) {
			return pController->newPattern( args[0].toString() );
		}
		return false;
	case IpcOpcode::SetSong: {
		auto pSong = Song::fromXmlBuffer( msg.getPayload(), "<ipc>", true,
										  pHydrogen );
		if ( pSong == nullptr ) {
			return false;
		}
		return pController->setSong( pSong );
	}
	case IpcOpcode::RescanSoundLibrary:
		if ( pHydrogen->getSoundLibraryDatabase() != nullptr ) {
			pHydrogen->getSoundLibraryDatabase()->update();
			return true;
		}
		return false;
	case IpcOpcode::SetInstrumentPitch:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentPitch( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentGain:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentGain( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentRandomPitch:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentRandomPitch( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentFilterCutoff:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentFilterCutoff( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentFilterResonance:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentFilterResonance( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentAttack:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentAttack( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentDecay:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentDecay( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentSustain:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentSustain( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentRelease:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentRelease( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetInstrumentFilterActive:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentFilterActive( args[0].toInt(), args[1].toBool() );
		}
		return false;
	case IpcOpcode::SetInstrumentMuteGroup:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentMuteGroup( args[0].toInt(), args[1].toInt() );
		}
		return false;
	case IpcOpcode::SetInstrumentStopNotes:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentStopNotes( args[0].toInt(), args[1].toBool() );
		}
		return false;
	case IpcOpcode::SetInstrumentApplyVelocity:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentApplyVelocity( args[0].toInt(), args[1].toBool() );
		}
		return false;
	case IpcOpcode::SetInstrumentHihatGroup:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentHihatGroup( args[0].toInt(), args[1].toInt() );
		}
		return false;
	case IpcOpcode::SetInstrumentLowerCc:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentLowerCc( args[0].toInt(), args[1].toInt() );
		}
		return false;
	case IpcOpcode::SetInstrumentHigherCc:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentHigherCc( args[0].toInt(), args[1].toInt() );
		}
		return false;
	case IpcOpcode::SetComponentIsMuted:
		if ( args.size() >= 3 ) {
			return pController->setComponentIsMuted( args[0].toInt(), args[1].toInt(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::SetComponentIsSoloed:
		if ( args.size() >= 3 ) {
			return pController->setComponentIsSoloed( args[0].toInt(), args[1].toInt(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::SetComponentGain:
		if ( args.size() >= 3 ) {
			return pController->setComponentGain( args[0].toInt(), args[1].toInt(), args[2].toFloat() );
		}
		return false;
	case IpcOpcode::SetComponentSelection:
		if ( args.size() >= 3 ) {
			return pController->setComponentSelection( args[0].toInt(), args[1].toInt(), args[2].toInt() );
		}
		return false;
	case IpcOpcode::SetLayerIsMuted:
		if ( args.size() >= 4 ) {
			return pController->setLayerIsMuted( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toBool() );
		}
		return false;
	case IpcOpcode::SetLayerIsSoloed:
		if ( args.size() >= 4 ) {
			return pController->setLayerIsSoloed( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toBool() );
		}
		return false;
	case IpcOpcode::SetLayerGain:
		if ( args.size() >= 4 ) {
			return pController->setLayerGain( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat() );
		}
		return false;
	case IpcOpcode::SetLayerPitchOffset:
		if ( args.size() >= 4 ) {
			return pController->setLayerPitchOffset( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat() );
		}
		return false;
	case IpcOpcode::SetLayerStartVelocity:
		if ( args.size() >= 4 ) {
			return pController->setLayerStartVelocity( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat() );
		}
		return false;
	case IpcOpcode::SetLayerEndVelocity:
		if ( args.size() >= 4 ) {
			return pController->setLayerEndVelocity( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat() );
		}
		return false;
	case IpcOpcode::SetStripIsMuted:
		if ( args.size() >= 3 ) {
			return pController->setStripIsMuted( args[0].toInt(), args[1].toBool(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::SetStripIsSoloed:
		if ( args.size() >= 3 ) {
			return pController->setStripIsSoloed( args[0].toInt(), args[1].toBool(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::SetStripPanSym:
		if ( args.size() >= 3 ) {
			return pController->setStripPanSym( args[0].toInt(), args[1].toFloat(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::SetHumanizeTime:
		if ( args.size() >= 1 ) {
			return pController->setHumanizeTime( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::SetHumanizeVelocity:
		if ( args.size() >= 1 ) {
			return pController->setHumanizeVelocity( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::SetSwing:
		if ( args.size() >= 1 ) {
			return pController->setSwing( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::SetPanLaw:
		if ( args.size() >= 2 ) {
			return pController->setPanLaw( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::SetPlaybackTrackMuted:
		if ( args.size() >= 1 ) {
			return pController->setPlaybackTrackMuted( args[0].toBool() );
		}
		return false;
	case IpcOpcode::SetPlaybackTrackVolume:
		if ( args.size() >= 1 ) {
			return pController->setPlaybackTrackVolume( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::PreviewInstrument:
		if ( args.size() >= 2 ) {
			return pController->previewInstrument( args[0].toInt(), args[1].toBool() );
		}
		return false;
	case IpcOpcode::ActivateTimeline:
		if ( args.size() >= 1 ) {
			return pController->activateTimeline( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ToggleTimeline:
		return pController->toggleTimeline();
	case IpcOpcode::DeleteTempoMarker:
		if ( args.size() >= 1 ) {
			return pController->deleteTempoMarker( args[0].toInt() );
		}
		return false;
	case IpcOpcode::DeleteTag:
		if ( args.size() >= 1 ) {
			return pController->deleteTag( args[0].toInt() );
		}
		return false;
	case IpcOpcode::ActivateJackTransport:
		if ( args.size() >= 1 ) {
			return pController->activateJackTransport( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ToggleJackTransport:
		return pController->toggleJackTransport();
	case IpcOpcode::ActivateJackTimebaseControl:
		if ( args.size() >= 1 ) {
			return pController->activateJackTimebaseControl( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ToggleJackTimebaseControl:
		return pController->toggleJackTimebaseControl();
	case IpcOpcode::ToggleSongMode:
		return pController->toggleSongMode();
	case IpcOpcode::ToggleLoopMode:
		return pController->toggleLoopMode();
	case IpcOpcode::MoveInstrument:
		if ( args.size() >= 2 ) {
			return pController->moveInstrument( args[0].toInt(), args[1].toInt() );
		}
		return false;
	case IpcOpcode::RenameComponent:
		if ( args.size() >= 3 ) {
			return pController->renameComponent( args[0].toInt(), args[1].toInt(), args[2].toString() );
		}
		return false;
	case IpcOpcode::ToggleNextPattern:
		if ( args.size() >= 1 ) {
			return pController->toggleNextPattern( args[0].toInt() );
		}
		return false;
	case IpcOpcode::MovePattern:
		if ( args.size() >= 2 ) {
			return pController->movePattern( args[0].toInt(), args[1].toInt() );
		}
		return false;
	case IpcOpcode::RemovePattern:
		if ( args.size() >= 1 ) {
			return pController->removePattern( args[0].toInt() );
		}
		return false;
	case IpcOpcode::SetPatternSize:
		if ( args.size() >= 3 ) {
			return pController->setPatternSize( args[0].toInt(), args[1].toInt(), args[2].toInt() );
		}
		return false;
	case IpcOpcode::StartCountIn:
		return pController->startCountIn();
	case IpcOpcode::ActivatePlaylistSong:
		if ( args.size() >= 1 ) {
			return pController->activatePlaylistSong( args[0].toInt() );
		}
		return false;
	case IpcOpcode::SetMidiClockInputHandling:
		if ( args.size() >= 1 ) {
			return pController->setMidiClockInputHandling( args[0].toBool() );
		}
		return false;
	case IpcOpcode::SetMidiClockOutputSend:
		if ( args.size() >= 1 ) {
			return pController->setMidiClockOutputSend( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ClearMidiInputLog:
		return pController->clearMidiInputLog();
	case IpcOpcode::ClearMidiOutputLog:
		return pController->clearMidiOutputLog();

	default:
		return false; // Hello / Event / unknown are not engine commands
	}
}

bool IpcEngineBridge::forwardEvent( IpcChannel& channel, Event::Type type,
									int nValue, long nId ) {
	if ( ! isEngineOriginEvent( type ) ) {
		return false; // editor-internal: stays in the editor process
	}
	channel.send( IpcMessage::fromEvent( type, nValue, nId ) );
	return true;
}

};
