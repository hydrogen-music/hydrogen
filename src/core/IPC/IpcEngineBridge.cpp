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
#include <core/IO/MidiBaseDriver.h>
#include <core/License.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiEvent.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

namespace H2Core {

bool IpcEngineBridge::dispatchCommand( const IpcMessage& msg,
									   Hydrogen* pHydrogen ) {
	if ( pHydrogen == nullptr ) {
		ERRORLOG( "Hydrogen instance invalid" );
		return false;
	}
	auto pController = pHydrogen->getCoreActionController();
	if ( pController == nullptr ) {
		ERRORLOG( "CoreActionController instance invalid" );
		return false;
	}
	const QVector<QVariant>& args = msg.getArgs();

	// Editor-forwarded commands apply with Trigger::Suppress: the editor
	// already applied the same command on its mirror (IpcCoreActionController
	// dual-apply), so an engine-origin SongIsModified echo would only
	// trigger a redundant full song re-pull in the editor. Engine-local
	// callers (OSC/MIDI/NSM on the engine) keep the Default trigger — their
	// echo is the editor's signal to re-pull engine-side changes
	// (ADR 0026 point 13).

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
			return pController->setBpm( args[0].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetMasterVolume:
		if ( args.size() >= 1 ) {
			return pController->setMasterVolume( args[0].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetMasterIsMuted:
		if ( args.size() >= 1 ) {
			return pController->setMasterIsMuted( args[0].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetMetronomeIsActive:
		if ( args.size() >= 1 ) {
			return pController->setMetronomeIsActive( args[0].toBool() );
		}
		break;
	case IpcOpcode::SetMetronomeVolume:
		if ( args.size() >= 1 ) {
			return pController->setMetronomeVolume( args[0].toFloat() );
		}
		break;
	case IpcOpcode::LocateToColumn:
		if ( args.size() >= 1 ) {
			return pController->locateToColumn( args[0].toInt() );
		}
		break;
	case IpcOpcode::LocateToTick:
		if ( args.size() >= 2 ) {
			return pController->locateToTick(
				static_cast<long>( args[0].toLongLong() ), args[1].toBool() );
		}
		break;
	case IpcOpcode::SelectPattern:
		if ( args.size() >= 1 ) {
			return pController->selectPattern( args[0].toInt() );
		}
		break;
	case IpcOpcode::SetSelectedInstrument:
		if ( args.size() >= 1 ) {
			// No CoreActionController surface for this — apply on the engine
			// directly (like Play/Stop). Default trigger: the resulting
			// SelectedInstrumentChanged event echoes the change back to the
			// editor's mirror.
			pHydrogen->setSelectedInstrumentNumber(
				args[0].toInt(), Event::Trigger::Default );
			return true;
		}
		break;
	case IpcOpcode::SetSongModified:
		if ( args.size() >= 1 ) {
			// No CoreActionController surface for this — apply on the
			// engine directly (like SetSelectedInstrument). The engine's
			// NsmClient reports the flip to the session manager when one
			// is connected. Suppress: the editor already applied the
			// flip on its mirror — an echoed SongIsModified would
			// trigger a redundant full song re-pull per edit (ADR 0026
			// point 10).
			pHydrogen->setSongModified(
				args[0].toBool(), Event::Trigger::Suppress );
			return true;
		}
		break;
	case IpcOpcode::HandleBeatCounter:
		if ( args.size() >= 1 ) {
			// No CoreActionController surface for this — apply on the
			// engine directly (like Play/Stop). Engine and editor share
			// the host clock, so the epoch count reconstructs the
			// TimePoint losslessly; 0 is the "no stamp" sentinel, which
			// handleBeatCounter() resolves to its own clock.
			return pHydrogen->handleBeatCounter( TimePoint(
				TimePoint::duration( args[0].toLongLong() ) ) );
		}
		break;
	case IpcOpcode::TapTempoAccelEvent:
		if ( args.size() >= 1 ) {
			pHydrogen->onTapTempoAccelEvent( TimePoint(
				TimePoint::duration( args[0].toLongLong() ) ) );
			return true;
		}
		break;
	case IpcOpcode::UpdateBeatCounterSettings:
		// Config snapshot from the editor: beat length, total beats,
		// drift compensation, start offset, and the Tap/TapAndPlay mode
		// (ADR 0026 point 12).
		if ( args.size() >= 5 ) {
			pHydrogen->updateBeatCounterSettings(
				args[0].toFloat(), args[1].toInt(), args[2].toInt(),
				args[3].toInt(),
				static_cast<Preferences::BeatCounter>( args[4].toInt() ) );
			return true;
		}
		break;
	case IpcOpcode::SetIsTimelineActivated:
		if ( args.size() >= 1 ) {
			pHydrogen->setIsTimelineActivated( args[0].toBool() );
			return true;
		}
		break;
	case IpcOpcode::SetPatternMode:
		if ( args.size() >= 1 ) {
			// Suppress: the editor already applied the mode flip on its
			// mirror — an engine-origin SongIsModified echo would only
			// trigger a redundant full song re-pull (ADR 0026 point 13).
			pHydrogen->setPatternMode(
				static_cast<Song::PatternMode>( args[0].toInt() ),
				Event::Trigger::Suppress );
			return true;
		}
		break;
	case IpcOpcode::LoadPlaybackTrack:
		if ( args.size() >= 1 ) {
			pHydrogen->loadPlaybackTrack( args[0].toString() );
			return true;
		}
		return false;
	case IpcOpcode::SetDrumkitModified:
		if ( args.size() >= 1 ) {
			// Suppress: the editor already applied the flip on its
			// mirror — an engine-origin SongIsModified echo would only
			// trigger a redundant full song re-pull (ADR 0026 point 13).
			pHydrogen->setDrumkitModified(
				args[0].toBool(), Event::Trigger::Suppress );
			return true;
		}
		break;
	case IpcOpcode::SetPatternModified:
		if ( args.size() >= 2 ) {
			// Suppress: like SetDrumkitModified above.
			pHydrogen->setPatternModified(
				args[0].toBool(), args[1].toInt(),
				Event::Trigger::Suppress );
			return true;
		}
		break;
	case IpcOpcode::SetPlaylistIsModified:
		if ( args.size() >= 1 ) {
			// No Suppress needed (unlike SetPatternModified above): the
			// playlist flip queues no event.
			pHydrogen->setPlaylistIsModified( args[0].toBool() );
			return true;
		}
		break;
	case IpcOpcode::SetIsPatternEditorLocked:
		if ( args.size() >= 1 ) {
			// Suppress: like SetDrumkitModified above.
			pHydrogen->setIsPatternEditorLocked(
				args[0].toBool(), Event::Trigger::Suppress );
			return true;
		}
		break;
	case IpcOpcode::SetStripVolume:
		if ( args.size() >= 3 ) {
			return pController->setStripVolume(
				args[0].toInt(), args[1].toFloat(), args[2].toBool(),
				Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetStripPan:
		if ( args.size() >= 3 ) {
			return pController->setStripPan(
				args[0].toInt(), args[1].toFloat(), args[2].toBool(),
				Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::ActivateLoopMode:
		if ( args.size() >= 1 ) {
			return pController->activateLoopMode( args[0].toBool() );
		}
		break;
	case IpcOpcode::ActivateSongMode:
		if ( args.size() >= 1 ) {
			return pController->activateSongMode( args[0].toBool() );
		}
		break;
	case IpcOpcode::ActivateRecordMode:
		if ( args.size() >= 1 ) {
			return pController->activateRecordMode( args[0].toBool() );
		}
		break;
	case IpcOpcode::AddTempoMarker:
		if ( args.size() >= 2 ) {
			return pController->addTempoMarker( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::AddTag:
		if ( args.size() >= 2 ) {
			return pController->addTag( args[0].toInt(), args[1].toString(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::AddAutomationPoint:
		if ( args.size() >= 2 ) {
			return pController->addAutomationPoint(
				args[0].toFloat(), args[1].toFloat(), Event::Trigger::Suppress
			);
		}
		break;
		case IpcOpcode::RemoveAutomationPoint:
		if ( args.size() >= 1 ) {
			return pController->removeAutomationPoint( args[0].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetSong: {
		auto pSong = Song::fromXmlBuffer(
			msg.getPayload(), Xml::Flag::Ipc, true, pHydrogen
		);
		if ( pSong == nullptr ) {
			ERRORLOG( QString( "Unable to serialize song from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->setSong( pSong, Event::Trigger::Suppress );
	}
	// args: version, name, author, notes, licenseString, copyrightHolder, tags
	case IpcOpcode::SetSongProperties:
		if ( args.size() >= 7 ) {
			return pController->setSongProperties(
				args[0].toInt(), args[1].toString(),
				args[2].toString(), args[3].toString(),
				License( args[4].toString(), args[5].toString() ),
				args[6].toStringList(), Event::Trigger::Suppress );
		}
		break;
	// args: path, version, name, author, info, licenseString, copyrightHolder,
	//       tags, patternIndex
	case IpcOpcode::SetPatternProperties:
		if ( args.size() >= 9 ) {
			return pController->setPatternProperties(
				args[0].toString(), args[1].toInt(), args[2].toString(),
				args[3].toString(), args[4].toString(),
				License( args[5].toString(), args[6].toString() ),
				args[7].toStringList(), args[8].toInt(),
				Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::RescanSoundLibrary:
		if ( pHydrogen->getSoundLibraryDatabase() != nullptr ) {
			pHydrogen->getSoundLibraryDatabase()->update();
			return true;
		}
		break;
	// args: SoundLibraryInfo::Type as int
	case IpcOpcode::UpdateSoundLibrary:
		if ( args.size() >= 1 ) {
			const auto pDatabase = pHydrogen->getSoundLibraryDatabase();
			if ( pDatabase != nullptr ) {
				// The editor already pushed SoundLibraryChanged on its
				// mirror — the engine-side re-scan stays silent (no echo).
				// Values outside the enum fall through to the generic
				// failure below.
				switch ( static_cast<SoundLibraryInfo::Type>(
							 args[0].toInt() ) ) {
				case SoundLibraryInfo::Type::Drumkit:
					pDatabase->updateDrumkits( Event::Trigger::Suppress );
					return true;
				case SoundLibraryInfo::Type::Pattern:
					pDatabase->updatePatterns( Event::Trigger::Suppress );
					return true;
				case SoundLibraryInfo::Type::Song:
					pDatabase->updateSongs( Event::Trigger::Suppress );
					return true;
				case SoundLibraryInfo::Type::Instrument:
					ERRORLOG( "There is no instrument list in the sound library database" );
					return false;
				}
			}
		}
		break;
	case IpcOpcode::SetInstrumentPitch:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentPitch( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentGain:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentGain( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentRandomPitch:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentRandomPitch( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentFilterCutoff:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentFilterCutoff( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentFilterResonance:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentFilterResonance( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentAttack:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentAttack( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentDecay:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentDecay( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentSustain:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentSustain( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentRelease:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentRelease( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentFilterActive:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentFilterActive( args[0].toInt(), args[1].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentMuteGroup:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentMuteGroup( args[0].toInt(), args[1].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentStopNotes:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentStopNotes( args[0].toInt(), args[1].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentApplyVelocity:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentApplyVelocity( args[0].toInt(), args[1].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentHihatGroup:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentHihatGroup( args[0].toInt(), args[1].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentLowerCc:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentLowerCc( args[0].toInt(), args[1].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentHigherCc:
		if ( args.size() >= 2 ) {
			return pController->setInstrumentHigherCc( args[0].toInt(), args[1].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentMidiOutNote:
		if ( args.size() >= 3 ) {
			return pController->setInstrumentMidiOutNote(
				args[0].toInt(), static_cast<Midi::Note>( args[1].toInt() ),
				args[2].value<long>(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetInstrumentMidiOutChannel:
		if ( args.size() >= 3 ) {
			return pController->setInstrumentMidiOutChannel(
				args[0].toInt(), static_cast<Midi::Channel>( args[1].toInt() ),
				args[2].value<long>(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetComponentIsMuted:
		if ( args.size() >= 3 ) {
			return pController->setComponentIsMuted( args[0].toInt(), args[1].toInt(), args[2].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetComponentIsSoloed:
		if ( args.size() >= 3 ) {
			return pController->setComponentIsSoloed( args[0].toInt(), args[1].toInt(), args[2].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetComponentGain:
		if ( args.size() >= 3 ) {
			return pController->setComponentGain( args[0].toInt(), args[1].toInt(), args[2].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetComponentSelection:
		if ( args.size() >= 3 ) {
			return pController->setComponentSelection( args[0].toInt(), args[1].toInt(), args[2].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetLayerIsMuted:
		if ( args.size() >= 4 ) {
			return pController->setLayerIsMuted( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetLayerIsSoloed:
		if ( args.size() >= 4 ) {
			return pController->setLayerIsSoloed( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetLayerGain:
		if ( args.size() >= 4 ) {
			return pController->setLayerGain( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetLayerPitchOffset:
		if ( args.size() >= 4 ) {
			return pController->setLayerPitchOffset( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetLayerStartVelocity:
		if ( args.size() >= 4 ) {
			return pController->setLayerStartVelocity( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetLayerEndVelocity:
		if ( args.size() >= 4 ) {
			return pController->setLayerEndVelocity( args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetStripIsMuted:
		if ( args.size() >= 3 ) {
			return pController->setStripIsMuted( args[0].toInt(), args[1].toBool(), args[2].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetStripIsSoloed:
		if ( args.size() >= 3 ) {
			return pController->setStripIsSoloed( args[0].toInt(), args[1].toBool(), args[2].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetStripPanSym:
		if ( args.size() >= 3 ) {
			return pController->setStripPanSym( args[0].toInt(), args[1].toFloat(), args[2].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetHumanizeTime:
		if ( args.size() >= 1 ) {
			return pController->setHumanizeTime( args[0].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetHumanizeVelocity:
		if ( args.size() >= 1 ) {
			return pController->setHumanizeVelocity( args[0].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetSwing:
		if ( args.size() >= 1 ) {
			return pController->setSwing( args[0].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetPanLaw:
		if ( args.size() >= 2 ) {
			return pController->setPanLaw( args[0].toInt(), args[1].toFloat(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetPlaybackTrackMuted:
		if ( args.size() >= 1 ) {
			return pController->setPlaybackTrackMuted( args[0].toBool(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetPlaybackTrackVolume:
		if ( args.size() >= 1 ) {
			return pController->setPlaybackTrackVolume( args[0].toFloat(), Event::Trigger::Suppress );
		}
		break;
	// ADR 0030 batch 2v — no args; the editor already applied the reset on
	// its mirror, the engine-side apply stays silent (no echo).
	case IpcOpcode::SetDefaultMidiOutNotes:
		return pController->setDefaultMidiOutNotes( Event::Trigger::Suppress );
	// ADR 0030 batch 2w — args: MidiAction::Type as int, then value,
	// instrument, component, layer, pattern, song, factor. The editor
	// already executed the action on its mirror; this is the
	// authoritative engine-side execution. Only parameters the type
	// supports are set — setting an unsupported one asserts. Types
	// outside the enum fail the manager's dispatch-map lookup
	// (graceful false).
	case IpcOpcode::HandleMidiAction:
		if ( args.size() >= 8 ) {
			auto pAction = std::make_shared<MidiAction>(
				static_cast<MidiAction::Type>( args[0].toInt() ) );
			pAction->setValue( args[1].toInt() );
			const auto requires = pAction->getRequires();
			if ( requires & MidiAction::RequiresInstrument ) {
				pAction->setInstrument( args[2].toInt() );
			}
			if ( requires & MidiAction::RequiresComponent ) {
				pAction->setComponent( args[3].toInt() );
			}
			if ( requires & MidiAction::RequiresLayer ) {
				pAction->setLayer( args[4].toInt() );
			}
			if ( requires & MidiAction::RequiresPattern ) {
				pAction->setPattern( args[5].toInt() );
			}
			if ( requires & MidiAction::RequiresSong ) {
				pAction->setSong( args[6].toInt() );
			}
			if ( requires & MidiAction::RequiresFactor ) {
				pAction->setFactor( args[7].toFloat() );
			}
			return pHydrogen->getMidiActionManager()
				->handleMidiActionSync( pAction );
		}
		break;
	// ADR 0030 batch 2x — no args; the editor already recalculated its
	// mirror's samples, the engine-side apply swaps its own in-memory
	// copies (each side locks its own audio engine).
	case IpcOpcode::RecalculateRubberband:
		return pController->recalculateRubberband();
	case IpcOpcode::PreviewInstrument:
		if ( args.size() >= 2 ) {
			return pController->previewInstrument( args[0].toInt(), args[1].toBool() );
		}
		break;
	case IpcOpcode::PreviewInstrumentSerialized: {
		auto pInstrument = Instrument::fromXmlBuffer(
			msg.getPayload(), Xml::Flag::KeepMissingSamples,
			true /* bSilent */, pHydrogen );
		if ( args.size() < 1 ) {
			break;
		}
		if ( pInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to serialize instrument from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		auto pNote = Note::fromXmlBuffer(
			args[0].toByteArray(), true /* bSilent */, pHydrogen );
		if ( pNote == nullptr ) {
			ERRORLOG( QString( "Unable to serialize note from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		// The ad-hoc instrument is not part of any kit, so the note
		// crossed without one; attach it to the reconstructed instrument.
		pNote->mapToInstrument( pInstrument );
		return pController->previewInstrument( pInstrument, pNote );
	}
	case IpcOpcode::ActivateTimeline:
		if ( args.size() >= 1 ) {
			return pController->activateTimeline( args[0].toBool() );
		}
		break;
	case IpcOpcode::ToggleTimeline:
		return pController->toggleTimeline();
	case IpcOpcode::DeleteTempoMarker:
		if ( args.size() >= 1 ) {
			return pController->deleteTempoMarker( args[0].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::DeleteTag:
		if ( args.size() >= 1 ) {
			return pController->deleteTag( args[0].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::ActivateJackTransport:
		if ( args.size() >= 1 ) {
			return pController->activateJackTransport( args[0].toBool() );
		}
		break;
	case IpcOpcode::ToggleJackTransport:
		return pController->toggleJackTransport();
	case IpcOpcode::ActivateJackTimebaseControl:
		if ( args.size() >= 1 ) {
			return pController->activateJackTimebaseControl( args[0].toBool() );
		}
		break;
	case IpcOpcode::ToggleJackTimebaseControl:
		return pController->toggleJackTimebaseControl();
	case IpcOpcode::ToggleSongMode:
		return pController->toggleSongMode();
	case IpcOpcode::ToggleLoopMode:
		return pController->toggleLoopMode();
	case IpcOpcode::MoveInstrument:
		if ( args.size() >= 2 ) {
			return pController->moveInstrument( args[0].toInt(), args[1].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::RenameComponent:
		if ( args.size() >= 3 ) {
			return pController->renameComponent( args[0].toInt(), args[1].toInt(), args[2].toString(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::ToggleNextPattern:
		if ( args.size() >= 1 ) {
			return pController->toggleNextPattern( args[0].toInt() );
		}
		break;
	case IpcOpcode::MovePattern:
		if ( args.size() >= 2 ) {
			return pController->movePattern( args[0].toInt(), args[1].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::RemovePattern:
		if ( args.size() >= 1 ) {
			return pController->removePattern( args[0].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::SetPatternSize:
		if ( args.size() >= 3 ) {
			return pController->setPatternSize( args[0].toInt(), args[1].toInt(), args[2].toInt(), Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::StartCountIn:
		return pController->startCountIn();
	case IpcOpcode::ActivatePlaylistSong:
		if ( args.size() >= 1 ) {
			return pController->activatePlaylistSong( args[0].toInt() );
		}
		break;
	case IpcOpcode::SetMidiClockInputHandling:
		if ( args.size() >= 1 ) {
			return pController->setMidiClockInputHandling( args[0].toBool() );
		}
		break;
	case IpcOpcode::SetMidiClockOutputSend:
		if ( args.size() >= 1 ) {
			return pController->setMidiClockOutputSend( args[0].toBool() );
		}
		break;
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
				args[13].toInt(), args[14].toInt(), args[15].toInt(),
				Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::RemoveNote:
		if ( args.size() >= 2 ) {
			return pController->removeNote(
				Uuid::fromQString( args[0].toString() ),
				Uuid::fromQString( args[1].toString() ),
				Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::ToggleGridCell:
		if ( args.size() >= 2 ) {
			return pController->toggleGridCell(
				GridPoint( args[0].toInt(), args[1].toInt() ),
				Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::AddOrRemoveNote:
		if ( args.size() >= 14 ) {
			return pController->addOrRemoveNote(
				args[0].toInt(), args[1].toInt(), args[2].toString(),
				args[3].toInt(), args[4].toInt(), args[5].toFloat(),
				args[6].toFloat(), args[7].toFloat(), args[8].toInt(),
				args[9].toInt(), args[10].toFloat(), args[11].toBool(),
				args[12].toBool(), args[13].toBool(), nullptr,
				Event::Trigger::Suppress );
		}
		break;
	case IpcOpcode::HandleNote:
		if ( args.size() >= 4 ) {
			return pController->handleNote(
				static_cast<Midi::Note>( args[0].toInt() ),
				static_cast<Midi::Channel>( args[1].toInt() ),
				args[2].toFloat(), args[3].toBool(), nullptr );
		}
		break;
 	case IpcOpcode::SetDrumkit: {
		auto pDrumkit = Drumkit::fromXmlBuffer(
			msg.getPayload(), "", Xml::Flag::SongKit, true, pHydrogen );
		if ( pDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to serialize drumkit from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->setDrumkit( pDrumkit, Event::Trigger::Suppress );
	}
	case IpcOpcode::SetPattern: {
		if ( args.size() < 2 || pHydrogen->getSong() == nullptr ) {
			return false;
		}
		auto pPattern = Pattern::fromXmlBuffer(
			msg.getPayload(), pHydrogen->getSong()->getDrumkit(), true,
			pHydrogen->getSoundLibraryDatabase() );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "Unable to serialize pattern from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->setPattern(
			pPattern, args[0].toInt(), args[1].toBool(),
			Event::Trigger::Suppress );
	}
	case IpcOpcode::ReplaceInstrument: {
		if ( args.size() < 1 ) {
			break;
		}
		if ( pHydrogen->getSong() == nullptr ||
			 pHydrogen->getSong()->getDrumkit() == nullptr ) {
			ERRORLOG( "Invalid current song" );
			return false;
		}
		auto pNewInstrument = Instrument::fromXmlBuffer(
			msg.getPayload(), Xml::Flag::SongKit, true, pHydrogen );
		if ( pNewInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to serialize new instrument from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		auto pOldInstrument =
			pHydrogen->getSong()->getDrumkit()->getInstruments()->find(
				static_cast<Instrument::Id>( args[0].toInt() ) );
		if ( pOldInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to serialize old instrument from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->replaceInstrument( pNewInstrument, pOldInstrument, Event::Trigger::Suppress );
	}
	case IpcOpcode::AddInstrument: {
		// Fire-and-forget path (the request/response path is in handleRequest).
		if ( args.size() < 2 ) {
			break;
		}
		auto pInstrument = Instrument::fromXmlBuffer(
			msg.getPayload(), Xml::Flag::SongKit, true, pHydrogen );
		if ( pInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to serialize instrument from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->addInstrument(
			pInstrument, args[0].toInt(), args[1].value<long>(),
			Event::Trigger::Suppress
		);
	}
	case IpcOpcode::RemoveInstrument: {
		// Fire-and-forget path (the request/response path is in handleRequest).
		if ( args.size() < 1 ) {
			break;
		}
		auto pInstrument = Instrument::fromXmlBuffer(
			msg.getPayload(), Xml::Flag::SongKit, true, pHydrogen );
		if ( pInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to serialize instrument from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->removeInstrument(
			pInstrument, args[0].value<long>(), Event::Trigger::Suppress
		);
	}
	case IpcOpcode::SaveSong:
		if ( args.size() >= 1 ) {
			return pController->saveSong( args[0].toBool() );
		}
		break;
	// args: newFileName, keepMissingSamples, pathPolicy (optional;
	//       0 = Adopt, 1 = Keep)
	case IpcOpcode::SaveSongAs:
		if ( args.size() >= 2 ) {
			return pController->saveSongAs(
				args[0].toString(), args[1].toBool(),
				args.size() >= 3
					? static_cast<CoreActionController::PathPolicy>(
						args[2].toInt() )
					: CoreActionController::PathPolicy::Adopt );
		}
		break;
	case IpcOpcode::SavePlaylist:
		return pController->savePlaylist();
	case IpcOpcode::SavePlaylistAs:
		if ( args.size() >= 1 ) {
			return pController->savePlaylistAs( args[0].toString() );
		}
		break;
	case IpcOpcode::SetPlaylist: {
		auto pPlaylist = Playlist::fromXmlBuffer( msg.getPayload() );
		if ( pPlaylist == nullptr ) {
			ERRORLOG( QString( "Unable to serialize playlist from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->setPlaylist( pPlaylist );
	}
	case IpcOpcode::SetMidiEventMap: {
		auto pMidiEventMap = MidiEventMap::fromXmlBuffer(
			msg.getPayload(), true, pHydrogen );
		if ( pMidiEventMap == nullptr ) {
			ERRORLOG( QString( "Unable to serialize MidiEventMap from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->setMidiEventMap( pMidiEventMap );
	}
	case IpcOpcode::SetMidiInstrumentMap: {
		auto pMidiInstrumentMap = MidiInstrumentMap::fromXmlBuffer(
			msg.getPayload(), true );
		if ( pMidiInstrumentMap == nullptr ) {
			ERRORLOG( QString( "Unable to serialize MidiInstrumentMap from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		return pController->setMidiInstrumentMap( pMidiInstrumentMap );
	}
	case IpcOpcode::SetLastMidiEvent:
		if ( args.size() >= 2 ) {
			return pController->setLastMidiEvent(
				static_cast<MidiEvent::Type>( args[0].toInt() ),
				static_cast<Midi::Parameter>( args[1].toInt() ) );
		}
		break;
	case IpcOpcode::AddCustomSoundLibraryDir:
		if ( args.size() >= 1 ) {
			return pController->addCustomSoundLibraryDir(
				args[0].toString() );
		}
		break;
	case IpcOpcode::RemoveCustomSoundLibraryDir:
		if ( args.size() >= 1 ) {
			return pController->removeCustomSoundLibraryDir(
				args[0].toString() );
		}
		break;
	case IpcOpcode::StopExportSession:
		// Fire-and-forget: the engine cancels the remaining plan and
		// restores its own state (ADR 0030 batch 2l).
		pController->stopExportSession();
		return true;
	case IpcOpcode::SetPreferences: {
		// The headless engine only needs the engine-core subset of
		// Preferences (audio driver, MIDI maps, metronome, etc.).
		// GUI-only fields are not relevant and not sent.
		auto pPref = pHydrogen->getPreferences();
		if ( pPref == nullptr ) {
			ERRORLOG( QString( "Unable to serialize Preferences from [%1]" )
						  .arg( msg.toQString() ) );
			return false;
		}
		pPref->applyCorePropsFromXml( msg.getPayload() );
		return pController->setPreferences( pPref );
	}
	case IpcOpcode::RecreateOscServer:
		// The engine's controller is the base class here (only the editor
		// side uses the forwarding subclass), so this cannot loop back.
		return pController->recreateOscServer();
	case IpcOpcode::AddToPlaylist:
		if ( args.size() >= 2 ) {
			return pController->addToPlaylist(
				PlaylistEntry::fromMimeText( args[0].toString() ),
				args[1].toInt() );
		}
		break;
	case IpcOpcode::RemoveFromPlaylist:
		if ( args.size() >= 2 ) {
			return pController->removeFromPlaylist(
				PlaylistEntry::fromMimeText( args[0].toString() ),
				args[1].toInt() );
		}
		break;
	case IpcOpcode::Panic:
		return pController->panic();
	case IpcOpcode::NoteOn: {
		auto pNote = Note::fromXmlBuffer(
			msg.getPayload(), true /* bSilent */, pHydrogen );
		if ( pNote == nullptr ) {
			ERRORLOG( QString( "Unable to serialize note from [%1]" )
						  .arg( msg.toQString() ) );
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
		ERRORLOG( QString( "Unknown opcode [%1 : %2]" )
				  .arg( static_cast<int>( msg.getOpcode() ) )
					.arg( H2Core::IpcOpcodeToQString(
						static_cast<quint16>( msg.getOpcode() )
					) ) );
		return false; // Hello / Event / unknown are not engine commands
	}

	WARNINGLOG( QString( "Invalid numbar of arguments [%1] in [%2]" )
					.arg( args.size() )
					.arg( H2Core::IpcOpcodeToQString(
						static_cast<quint16>( msg.getOpcode() )
					) ) );
	return false;
}

IpcMessage IpcEngineBridge::handleRequest( const IpcMessage& msg,
										   Hydrogen* pHydrogen ) {
	IpcMessage reply( IpcOpcode::Reply );
	reply.setRequestId( msg.getRequestId() );
	if ( pHydrogen == nullptr ) {
		ERRORLOG( "Hydrogen instance invalid" );
		return reply;
	}
	auto pController = pHydrogen->getCoreActionController();
	if ( pController == nullptr ) {
		ERRORLOG( "CoreActionController instance invalid" );
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
			.arg( info.jackTransportEnabled )
			// The engine's actual rate — the editor mirror must be re-rated
			// to it (frame<->tick conversion, ADR 0018/0029). Buffer size and
			// latency ride along for display only.
			.arg( info.sampleRate )
			.arg( info.bufferSize )
			.arg( info.latencyFrames );
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
	case IpcOpcode::GetSessionFolderPath:
		// Empty string: no NSM session in effect (or no OSC support).
		reply.arg( pHydrogen->getSessionFolderPath() );
		break;
	// ── Driver enumeration queries (ADR 0029): the engine owns the driver
	// stacks, so these can only be answered here. ──
	case IpcOpcode::GetMidiPorts: {
		QStringList ports;
		auto pDriver = pHydrogen->getMidiDriver();
		if ( pDriver != nullptr && args.size() >= 1 ) {
			// PortType semantics are inverted relative to Hydrogen's own
			// direction (PortType::Output feeds the Input combo); all
			// backends agree on them, so the value passes through verbatim.
			for ( const auto& ssPort : pDriver->getExternalPortList(
					  static_cast<MidiBaseDriver::PortType>(
						  args[0].toInt() ) ) ) {
				ports << ssPort;
			}
		}
		reply.arg( ports );
		break;
	}
	case IpcOpcode::GetHandledMidiInputs: {
		auto pDriver = pHydrogen->getMidiDriver();
		if ( pDriver == nullptr ) {
			reply.arg( 0 );
			break;
		}
		const auto inputs = pDriver->getHandledInputs();
		reply.arg( static_cast<int>( inputs.size() ) );
		for ( const auto& ppInput : inputs ) {
			// Engine and editor run on the same host (QLocalSocket), so the
			// clock epoch is shared and the count round-trips losslessly.
			reply.arg( static_cast<qint64>(
						   ppInput->timePoint.time_since_epoch().count() ) )
				.arg( static_cast<int>( ppInput->type ) )
				.arg( static_cast<int>( ppInput->data1 ) )
				.arg( static_cast<int>( ppInput->data2 ) )
				.arg( static_cast<int>( ppInput->channel ) );
			QVariantList actionTypes;
			for ( const auto& actionType : ppInput->actionTypes ) {
				actionTypes << static_cast<int>( actionType );
			}
			reply.arg( actionTypes ).arg( ppInput->mappedInstruments );
		}
		break;
	}
	case IpcOpcode::GetHandledMidiOutputs: {
		auto pDriver = pHydrogen->getMidiDriver();
		if ( pDriver == nullptr ) {
			reply.arg( 0 );
			break;
		}
		const auto outputs = pDriver->getHandledOutputs();
		reply.arg( static_cast<int>( outputs.size() ) );
		for ( const auto& ppOutput : outputs ) {
			reply.arg( static_cast<qint64>(
						   ppOutput->timePoint.time_since_epoch().count() ) )
				.arg( static_cast<int>( ppOutput->type ) )
				.arg( static_cast<int>( ppOutput->data1 ) )
				.arg( static_cast<int>( ppOutput->data2 ) )
				.arg( static_cast<int>( ppOutput->channel ) );
		}
		break;
	}
	case IpcOpcode::GetAudioHostAPIs:
		reply.arg( pHydrogen->getAudioHostAPIs() );
		break;
	case IpcOpcode::GetOscTemporaryPort:
		reply.arg( pHydrogen->getOscTemporaryPort() );
		break;
	case IpcOpcode::GetLastMidiEvent:
		// The pair is one logical value (the MIDI input writes both
		// per event); both ride in a single reply so the editor's
		// snapshot can not tear.
		reply.arg( static_cast<int>( pHydrogen->getLastMidiEvent() ) )
			.arg( static_cast<int>(
				pHydrogen->getLastMidiEventParameter() ) );
		break;
	case IpcOpcode::GetAudioDevices: {
		QStringList devices;
		if ( args.size() >= 2 ) {
			devices = pHydrogen->getAudioDevices(
				static_cast<Preferences::AudioDriver>( args[0].toInt() ),
				args[1].toString() );
		}
		// Always answer with a list arg (like GetMidiPorts) so the client
		// can treat a malformed request uniformly as "no devices".
		reply.arg( devices );
		break;
	}
	case IpcOpcode::ExportSong: {
		// One-shot plan (ADR 0030 batch 2l): [sampleRate, sampleDepth,
		// compressionLevel, interpolateMode, rubberbandBatch,
		// renderCount, then per render: fileName + excluded instrument
		// ids]. The engine arms the session synchronously and renders
		// on a background thread, so this handler stays fast and the
		// serve loop keeps pumping events while the plan runs.
		bool bMalformed = args.size() < 6;
		std::vector<ExportRender> renders;
		if ( ! bMalformed ) {
			const int nRenderCount = args[5].toInt();
			if ( nRenderCount < 0 ) {
				// A negative count would silently yield an empty plan.
				bMalformed = true;
			}
			int nn = 6;
			for ( int ii = 0; ii < nRenderCount; ++ii ) {
				if ( nn + 2 > args.size() ) {
					bMalformed = true;
					break;
				}
				ExportRender render;
				render.sFileName = args[nn].toString();
				++nn;
				for ( const auto& sUuid : args[nn].toStringList() ) {
					render.excludedInstruments.push_back(
						Uuid::fromQString( sUuid ) );
				}
				++nn;
				renders.push_back( std::move( render ) );
			}
		}
		if ( bMalformed ) {
			ERRORLOG( QString( "Malformed export request [%1]" )
						  .arg( msg.toQString() ) );
			reply.arg( false );
			break;
		}
		reply.arg( pController->exportSong(
			args[0].toInt(), args[1].toInt(), args[2].toDouble(),
			static_cast<Interpolation::InterpolateMode>( args[3].toInt() ),
			args[4].toBool(), renders ) );
		break;
	}
	case IpcOpcode::GetExportWritingFailed:
		reply.arg( pHydrogen->isExportWritingFailed() );
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
