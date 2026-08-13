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

#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/GridPoint.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>
#include <core/IO/AudioDriverInfo.h>
#include <core/License.h>
#include <core/Midi/Midi.h>
#include <core/Preferences/Preferences.h>
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
	case IpcOpcode::SetSong: {
		auto pSong = Song::fromXmlBuffer( msg.getPayload(), true, pHydrogen );
		if ( pSong == nullptr ) {
			return false;
		}
		return pController->setSong( pSong );
	}
	// args: path, version, name, author, notes, licenseString, copyrightHolder, tags
	case IpcOpcode::SetSongProperties:
		if ( args.size() >= 8 ) {
			return pController->setSongProperties(
				args[0].toString(), args[1].toInt(), args[2].toString(),
				args[3].toString(), args[4].toString(),
				License( args[5].toString(), args[6].toString() ),
				args[7].toStringList() );
		}
		return false;
	// args: path, version, name, author, info, licenseString, copyrightHolder,
	//       tags, patternIndex
	case IpcOpcode::SetPatternProperties:
		if ( args.size() >= 9 ) {
			return pController->setPatternProperties(
				args[0].toString(), args[1].toInt(), args[2].toString(),
				args[3].toString(), args[4].toString(),
				License( args[5].toString(), args[6].toString() ),
				args[7].toStringList(), args[8].toInt() );
		}
		return false;
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
	case IpcOpcode::SetInstrumentMidiOutNote:
		if ( args.size() >= 3 ) {
			return pController->setInstrumentMidiOutNote(
				args[0].toInt(), static_cast<Midi::Note>( args[1].toInt() ),
				args[2].value<long>() );
		}
		return false;
	case IpcOpcode::SetInstrumentMidiOutChannel:
		if ( args.size() >= 3 ) {
			return pController->setInstrumentMidiOutChannel(
				args[0].toInt(), static_cast<Midi::Channel>( args[1].toInt() ),
				args[2].value<long>() );
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

	case IpcOpcode::EditNoteProperty:
		if ( args.size() >= 16 ) {
			return pController->editNoteProperty(
				static_cast<NoteProperty>( args[0].toInt() ),
				args[1].toInt(), args[2].toInt(), args[3].toInt(), args[4].toInt(),
				args[5].toString(), args[6].toString(),
				args[7].toFloat(), args[8].toFloat(), args[9].toFloat(),
				args[10].toFloat(), args[11].toInt(), args[12].toInt(),
				args[13].toInt(), args[14].toInt(), args[15].toInt() );
		}
		return false;
	case IpcOpcode::ToggleGridCell:
		if ( args.size() >= 2 ) {
			return pController->toggleGridCell(
				GridPoint( args[0].toInt(), args[1].toInt() ) );
		}
		return false;
	case IpcOpcode::AddOrRemoveNote:
		if ( args.size() >= 14 ) {
			return pController->addOrRemoveNote(
				args[0].toInt(), args[1].toInt(), args[2].toString(),
				args[3].toInt(), args[4].toInt(), args[5].toFloat(),
				args[6].toFloat(), args[7].toFloat(), args[8].toInt(),
				args[9].toInt(), args[10].toFloat(), args[11].toBool(),
				args[12].toBool(), args[13].toBool(), nullptr );
		}
		return false;
	case IpcOpcode::HandleNote:
		if ( args.size() >= 4 ) {
			return pController->handleNote(
				static_cast<Midi::Note>( args[0].toInt() ),
				static_cast<Midi::Channel>( args[1].toInt() ),
				args[2].toFloat(), args[3].toBool(), nullptr );
		}
		return false;
 	case IpcOpcode::SetDrumkit: {
		auto pDrumkit = Drumkit::fromXmlBuffer( msg.getPayload(), "", true, true,
												pHydrogen );
		if ( pDrumkit == nullptr ) {
			return false;
		}
		return pController->setDrumkit( pDrumkit );
	}
	case IpcOpcode::SetPattern: {
		if ( args.size() < 2 || pHydrogen->getSong() == nullptr ) {
			return false;
		}
		auto pPattern = Pattern::fromXmlBuffer(
			msg.getPayload(), pHydrogen->getSong()->getDrumkit(), true,
			pHydrogen->getSoundLibraryDatabase() );
		if ( pPattern == nullptr ) {
			return false;
		}
		return pController->setPattern(
			pPattern, args[0].toInt(), args[1].toBool() );
	}
	case IpcOpcode::ReplaceInstrument: {
		if ( args.size() < 1 || pHydrogen->getSong() == nullptr ||
			 pHydrogen->getSong()->getDrumkit() == nullptr ) {
			return false;
		}
		auto pNewInstrument = Instrument::fromXmlBuffer( msg.getPayload(), true,
														 true, pHydrogen );
		auto pOldInstrument =
			pHydrogen->getSong()->getDrumkit()->getInstruments()->find(
				static_cast<Instrument::Id>( args[0].toInt() ) );
		if ( pNewInstrument == nullptr || pOldInstrument == nullptr ) {
			return false;
		}
		return pController->replaceInstrument( pNewInstrument, pOldInstrument );
	}
	case IpcOpcode::AddInstrument: {
		// Fire-and-forget path (the request/response path is in handleRequest).
		if ( args.size() < 2 ) {
			return false;
		}
		auto pInstrument = Instrument::fromXmlBuffer( msg.getPayload(), true, true,
													  pHydrogen );
		if ( pInstrument == nullptr ) {
			return false;
		}
		return pController->addInstrument(
			pInstrument, args[0].toInt(), args[1].value<long>()
		);
	}
	case IpcOpcode::RemoveInstrument: {
		// Fire-and-forget path (the request/response path is in handleRequest).
		if ( args.size() < 1 ) {
			return false;
		}
		auto pInstrument = Instrument::fromXmlBuffer( msg.getPayload(), true, true,
													  pHydrogen );
		if ( pInstrument == nullptr ) {
			return false;
		}
		return pController->removeInstrument(
			pInstrument, args[0].value<long>()
		);
	}
	case IpcOpcode::SaveSong:
		if ( args.size() >= 1 ) {
			return pController->saveSong( args[0].toBool() );
		}
		return false;
	case IpcOpcode::SaveSongAs:
		if ( args.size() >= 2 ) {
			return pController->saveSongAs( args[0].toString(), args[1].toBool() );
		}
		return false;
	case IpcOpcode::SavePlaylist:
		return pController->savePlaylist();
	case IpcOpcode::SavePlaylistAs:
		if ( args.size() >= 1 ) {
			return pController->savePlaylistAs( args[0].toString() );
		}
		return false;
	case IpcOpcode::SetPlaylist: {
		auto pPlaylist = Playlist::fromXmlBuffer( msg.getPayload() );
		if ( pPlaylist == nullptr ) {
			return false;
		}
		return pController->setPlaylist( pPlaylist );
	}
	case IpcOpcode::SetPreferences: {
		// The headless engine only needs the engine-core subset of
		// Preferences (audio driver, MIDI maps, metronome, etc.).
		// GUI-only fields are not relevant and not sent.
		auto pPref = pHydrogen->getPreferences();
		if ( pPref == nullptr ) {
			return false;
		}
		pPref->applyCorePropsFromXml( msg.getPayload() );
		return pController->setPreferences( pPref );
	}
	case IpcOpcode::AddToPlaylist:
		if ( args.size() >= 2 ) {
			return pController->addToPlaylist(
				PlaylistEntry::fromMimeText( args[0].toString() ),
				args[1].toInt() );
		}
		return false;
	case IpcOpcode::RemoveFromPlaylist:
		if ( args.size() >= 2 ) {
			return pController->removeFromPlaylist(
				PlaylistEntry::fromMimeText( args[0].toString() ),
				args[1].toInt() );
		}
		return false;
	case IpcOpcode::Panic:
		return pController->panic();
	case IpcOpcode::NoteOn: {
		auto pNote = Note::fromXmlBuffer(
			msg.getPayload(), true /* bSilent */, pHydrogen );
		if ( pNote == nullptr ) {
			return false;
		}
		return pController->noteOn( pNote );
	}
	case IpcOpcode::ReleasePlayingNotes: {
		auto uuid = Uuid::fromQString(
			QString( msg.getPayload() ), true /* bSilent */ );
		return pController->releasePlayingNotes( uuid );
	}
	default:
		return false; // Hello / Event / unknown are not engine commands
	}
}

IpcMessage IpcEngineBridge::handleRequest( const IpcMessage& msg,
										   Hydrogen* pHydrogen ) {
	IpcMessage reply( IpcOpcode::Reply );
	reply.setRequestId( msg.getRequestId() );
	if ( pHydrogen == nullptr ) {
		return reply;
	}
	auto pController = pHydrogen->getCoreActionController();
	if ( pController == nullptr ) {
		return reply;
	}
	const QVector<QVariant>& args = msg.getArgs();

	switch ( msg.getOpcode() ) {
	// ── State-sync requests (ADR 0032): editor pulls authoritative state ──
	case IpcOpcode::GetSong: {
		auto pSong = pHydrogen->getSong();
		if ( pSong != nullptr ) {
			reply.setPayload( pSong->toXmlBuffer() );
		}
		break;
	}
	case IpcOpcode::GetPlaylist: {
		auto pPlaylist = pHydrogen->getPlaylist();
		if ( pPlaylist != nullptr ) {
			reply.setPayload( pPlaylist->toXmlBuffer() );
		}
		break;
	}
	case IpcOpcode::GetSelectedPattern:
		reply.arg( pHydrogen->getSelectedPatternNumber() );
		break;
	case IpcOpcode::GetSelectedInstrument:
		reply.arg( pHydrogen->getSelectedInstrumentNumber() );
		break;
	case IpcOpcode::GetRecordEnabled:
		reply.arg( pHydrogen->getRecordEnabled() );
		break;
	case IpcOpcode::GetCorePreferences: {
		auto pPref = pHydrogen->getPreferences();
		if ( pPref != nullptr ) {
			reply.setPayload( pPref->corePropsToXml() );
		}
		break;
	}
	case IpcOpcode::GetSoundLibraryInfo: {
		auto pDb = pHydrogen->getSoundLibraryDatabase();
		if ( pDb != nullptr ) {
			reply.arg( pDb->getDrumkitFolders() )
				.arg( pDb->getCustomDrumkitFolders() )
				.arg( pDb->getCustomDrumkitPaths() );
		}
		break;
	}
	case IpcOpcode::GetAudioDriverInfo: {
		const auto info = pHydrogen->getAudioDriverInfo();
		reply.arg( static_cast<int>( info.kind ) )
			.arg( info.isPresent )
			.arg( info.isRunning )
			.arg( info.connectedDevice )
			.arg( static_cast<int>( info.timebaseState ) )
			.arg( info.jackTransportEnabled );
		break;
	}
	case IpcOpcode::GetMidiDriverInfo: {
		const auto info = pHydrogen->getMidiDriverInfo();
		reply.arg( info.isPresent )
			.arg( info.isInputActive )
			.arg( info.isOutputActive );
		break;
	}
	case IpcOpcode::GetIsUnderSessionManagement:
		reply.arg( pHydrogen->isUnderSessionManagement() );
		break;
	case IpcOpcode::GetIsUnderPluginHost:
		reply.arg( pHydrogen->isUnderPluginHost() );
		break;
	default:
		break; // unknown request → empty Reply (correlated by id)
	}
	return reply;
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
