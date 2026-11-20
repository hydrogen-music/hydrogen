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

#include <core/IPC/IpcEngineAccess.h>

#include <core/IO/MidiBaseDriver.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcMessage.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiActionManager.h>
#include <core/Midi/MidiEvent.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

namespace H2Core {

namespace {

// Issues a blocking request against the authoritative engine. Returns
// false on timeout; callers fall back to an empty result (the GUI treats
// "no answer" like "no data" rather than showing stale mirror state).
bool ipcRequest( IpcChannel* pChannel, const IpcMessage& request,
				 IpcMessage& reply )
{
	return pChannel != nullptr &&
		pChannel->request( request, reply, 3000 );
}

} // namespace

void IpcEngineAccess::sequencerPlay() {
	// Transport start is engine-authoritative: forward it to the real engine and
	// reflect it locally so the GUI's transport widgets update immediately.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::Play ) );
	}
	m_pMirror->sequencerPlay();
}

void IpcEngineAccess::sequencerStop() {
	// Transport stop is engine-authoritative: forward it to the real engine and
	// reflect it locally so the GUI's transport widgets update immediately.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::Stop ) );
	}
	m_pMirror->sequencerStop();
}

void IpcEngineAccess::setSelectedInstrumentNumber(
	int nInstrument, Event::Trigger trigger )
{
	// Instrument selection is engine-relevant: the headless engine's
	// MIDI-to-selected-instrument routing follows it. Forward the change and
	// apply it locally so the GUI updates immediately; the engine's echo
	// event re-applies the same value on the mirror (idempotent).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetSelectedInstrument ).arg( nInstrument ) );
	}
	m_pMirror->setSelectedInstrumentNumber( nInstrument, trigger );
}

void IpcEngineAccess::setSongModified( bool bIsModified )
{
	// NSM judges "unsaved changes" by the dirty state the engine's
	// NsmClient reports, but modifications happen here in the editor.
	// Forward the flip and apply it locally so the GUI title updates
	// immediately; the SongIsModified event stays engine-side (the
	// editor already knows — no echo needed).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetSongModified ).arg( bIsModified ) );
	}
	m_pMirror->setSongModified( bIsModified );
}

void IpcEngineAccess::setDrumkitModified( bool bIsModified )
{
	// Class C song state (ADR 0026 point 13): dual-apply — the mirror
	// reflects the flip immediately, the forwarded command applies it
	// engine-side under Suppress (no SongIsModified echo: the editor
	// initiated and already applied the flip).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetDrumkitModified ).arg( bIsModified ) );
	}
	m_pMirror->setDrumkitModified( bIsModified );
}

void IpcEngineAccess::setIsPatternEditorLocked( bool bLocked )
{
	// Class C song state (ADR 0026 point 13): dual-apply like
	// setDrumkitModified(). The PatternEditorLocked event the engine
	// pushes crosses and re-applies the same value on the mirror
	// (idempotent).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetIsPatternEditorLocked ).arg( bLocked ) );
	}
	m_pMirror->setIsPatternEditorLocked( bLocked );
}

void IpcEngineAccess::setPatternModified( bool bIsModified, int nIndex )
{
	// Class C song state (ADR 0026 point 13): dual-apply like
	// setDrumkitModified().
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetPatternModified )
			.arg( bIsModified )
			.arg( nIndex ) );
	}
	m_pMirror->setPatternModified( bIsModified, nIndex );
}

void IpcEngineAccess::setPlaylistIsModified( bool bIsModified )
{
	// Dual-apply like setPatternModified() above; the engine-side apply
	// needs no Suppress since the playlist flip queues no event.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetPlaylistIsModified ).arg( bIsModified ) );
	}
	m_pMirror->setPlaylistIsModified( bIsModified );
}

bool IpcEngineAccess::handleBeatCounter( TimePoint start ) {
	// Taps are engine-authoritative: the mirror's handler is a designed
	// no-op in editor mode (getTempoSource() == Tempo::Remote). Engine
	// and editor share the host clock, so the epoch count reconstructs
	// the TimePoint losslessly (like the handled-MIDI queries above). A
	// default-constructed TimePoint is stamped HERE, at call time: the
	// engine must not stamp at bridge-thread processing time — a tap
	// queued behind e.g. a TapAndPlay lead-in sleep would inherit that
	// delay as interval jitter (standalone stamps at call time too). The
	// engine's BeatCounter echoes carry the event count back for the
	// BpmTap display (ADR 0026 point 12).
	if ( start == TimePoint() ) {
		start = Clock::now();
	}
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::HandleBeatCounter )
			.arg( static_cast<qint64>(
				start.time_since_epoch().count() ) ) );
		return true;
	}
	return m_pMirror->handleBeatCounter( start );
}

bool IpcEngineAccess::handleMidiAction(
	const std::shared_ptr<MidiAction> pAction ) {
	// MIDI action (ADR 0030 batch 2w): dual-apply — the mirror's manager
	// executes the action immediately (its event pushes refresh the GUI),
	// and the command crosses so the authoritative engine's manager
	// executes it on the real song, drumkit, and transport. Engine-side
	// event echoes re-fan-out on the editor as harmless redundant
	// refreshes. Editor-local actions (undo/redo — the engine holds no
	// command stack) never cross.
	if ( pAction == nullptr ) {
		return false;
	}
	if ( m_pChannel != nullptr &&
		 ! MidiActionManager::isEditorLocal( pAction->getType() ) ) {
		// Only parameters the action's type actually supports cross the
		// wire — reading an unsupported one asserts (MidiAction models
		// its parameter space strictly). The positional slots stay
		// fixed; unsupported ones cross as 0 and are never read by the
		// engine-side handler.
		const auto requires = pAction->getRequires();
		int nInstrument = 0;
		int nComponent = 0;
		int nLayer = 0;
		int nPattern = 0;
		int nSong = 0;
		float fFactor = 0.0;
		if ( requires & MidiAction::RequiresInstrument ) {
			nInstrument = pAction->getInstrument();
		}
		if ( requires & MidiAction::RequiresComponent ) {
			nComponent = pAction->getComponent();
		}
		if ( requires & MidiAction::RequiresLayer ) {
			nLayer = pAction->getLayer();
		}
		if ( requires & MidiAction::RequiresPattern ) {
			nPattern = pAction->getPattern();
		}
		if ( requires & MidiAction::RequiresSong ) {
			nSong = pAction->getSong();
		}
		if ( requires & MidiAction::RequiresFactor ) {
			fFactor = pAction->getFactor();
		}
		m_pChannel->send( IpcMessage( IpcOpcode::HandleMidiAction )
			.arg( static_cast<int>( pAction->getType() ) )
			.arg( pAction->getValue() )
			.arg( nInstrument )
			.arg( nComponent )
			.arg( nLayer )
			.arg( nPattern )
			.arg( nSong )
			.arg( fFactor ) );
	}
	return m_pMirror->getMidiActionManager()
		->handleMidiActionSync( pAction );
}

void IpcEngineAccess::onTapTempoAccelEvent( TimePoint start ) {
	// Like handleBeatCounter(): the mirror's handler is a designed no-op
	// in editor mode, so the tap crosses with its absolute timestamp —
	// stamped at call time, before queueing can add jitter (ADR 0026
	// point 12).
	if ( start == TimePoint() ) {
		start = Clock::now();
	}
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::TapTempoAccelEvent )
			.arg( static_cast<qint64>(
				start.time_since_epoch().count() ) ) );
		return;
	}
	m_pMirror->onTapTempoAccelEvent( start );
}

void IpcEngineAccess::updateBeatCounterSettings() {
	// Beat-counter config is editor-owned: the BpmTap buttons write the
	// mirror's Hydrogen members and the mode actions and preferences
	// dialog write the mirror's preferences — none of which sync to the
	// engine on their own. Cross the whole configuration as a snapshot
	// of the mirror's current state; the engine's TapAndPlay completion
	// branch reads the mode from its (otherwise stale) preferences copy
	// (ADR 0026 point 12).
	if ( m_pChannel != nullptr ) {
		const auto pPreferences = m_pMirror->getPreferences();
		m_pChannel->send( IpcMessage( IpcOpcode::UpdateBeatCounterSettings )
			.arg( m_pMirror->getBeatCounterBeatLength() )
			.arg( m_pMirror->getBeatCounterTotalBeats() )
			.arg( pPreferences->m_nBeatCounterDriftCompensation )
			.arg( pPreferences->m_nBeatCounterStartOffset )
			.arg( static_cast<int>( pPreferences->m_beatCounter ) ) );
		return;
	}
	m_pMirror->updateBeatCounterSettings();
}

void IpcEngineAccess::setIsTimelineActivated( bool bEnabled ) {
	// Song state the GUI reads on the mirror: forward the command and
	// apply it locally so the timeline widgets update immediately (ADR
	// 0026 point 12).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetIsTimelineActivated )
			.arg( bEnabled ) );
	}
	m_pMirror->setIsTimelineActivated( bEnabled );
}

void IpcEngineAccess::setPatternMode( const Song::PatternMode& mode ) {
	// Engine-authoritative song state: forward the command and apply it
	// locally for immediate reflection. The engine-side apply flips the
	// dirty flag with the Default trigger — the engine-origin
	// SongIsModified echo then triggers the editor's full song re-pull
	// (ADR 0026 point 10/12).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetPatternMode )
			.arg( static_cast<int>( mode ) ) );
	}
	m_pMirror->setPatternMode( mode );
}

void IpcEngineAccess::loadPlaybackTrack( const QString& sFileName ) {
	// The playback track is engine-audible: forward the command and load
	// a local copy so the GUI's waveform shows immediately (ADR 0026
	// point 12).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::LoadPlaybackTrack )
			.arg( sFileName ) );
	}
	m_pMirror->loadPlaybackTrack( sFileName );
}

void IpcEngineAccess::rescanSoundLibrary() {
	// Full library rescan (ADR 0016): dual-apply — the mirror's database
	// rebuilds immediately (its SoundLibraryChanged push refreshes the
	// GUI), and the command crosses so the authoritative engine's
	// database rebuilds too. Unlike the per-type rescan the engine-side
	// update() pushes SoundLibraryChanged unconditionally; its echo
	// re-fans-out on the editor as a harmless redundant refresh.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::RescanSoundLibrary ) );
	}
	const auto pDatabase = m_pMirror->getSoundLibraryDatabase();
	if ( pDatabase != nullptr ) {
		pDatabase->update();
	}
}

void IpcEngineAccess::updateSoundLibrary( SoundLibraryInfo::Type type,
										  Event::Trigger trigger ) {
	// Per-type library rescan (ADR 0016): dual-apply — the mirror's
	// database re-scans the type immediately (its push refreshes the
	// GUI), and the command crosses so the authoritative engine's
	// database re-scans it too. The engine-side apply runs under
	// Suppress: the editor initiated the rescan and already pushed the
	// event on its mirror, so no SoundLibraryChanged echo crosses.
	if ( m_pChannel != nullptr && type != SoundLibraryInfo::Type::Instrument ) {
		m_pChannel->send( IpcMessage( IpcOpcode::UpdateSoundLibrary )
			.arg( static_cast<int>( type ) ) );
	}
	const auto pDatabase = m_pMirror->getSoundLibraryDatabase();
	if ( pDatabase == nullptr ) {
		return;
	}
	switch ( type ) {
	case SoundLibraryInfo::Type::Drumkit:
		pDatabase->updateDrumkits( trigger );
		break;
	case SoundLibraryInfo::Type::Pattern:
		pDatabase->updatePatterns( trigger );
		break;
	case SoundLibraryInfo::Type::Song:
		pDatabase->updateSongs( trigger );
		break;
	case SoundLibraryInfo::Type::Instrument:
		// The database tracks no instrument list — nothing to re-scan.
		ERRORLOG( "There is no instrument list in the sound library database" );
		break;
	}
}

QStringList IpcEngineAccess::getAudioDevices(
		Preferences::AudioDriver kind, const QString& sHostAPI
	) const {
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetAudioDevices )
						   .arg( static_cast<int>( kind ) )
						   .arg( sHostAPI ),
					   reply ) ) {
		WARNINGLOG( QString( "Engine did not answer the audio device query "
							 "for driver [%1], host API [%2]" )
						.arg( Preferences::audioDriverToQString( kind ) )
						.arg( sHostAPI ) );
		return QStringList();
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return QStringList();
	}
	return args[0].toStringList();
}

QStringList IpcEngineAccess::getAudioHostAPIs() const {
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel, IpcMessage( IpcOpcode::GetAudioHostAPIs ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the host API query" );
		return QStringList();
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return QStringList();
	}
	return args[0].toStringList();
}

int IpcEngineAccess::getOscTemporaryPort() const {
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetOscTemporaryPort ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the OSC temporary port query" );
		return -1;
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return -1;
	}
	return args[0].toInt();
}

LastMidiEvent IpcEngineAccess::getLastMidiEvent() const {
	// The mirror owns no MIDI driver (AudioEngine forces
	// MidiDriver::None in editor mode), so the learning channel can
	// only be served by the authoritative engine. One query returns
	// the (type, parameter) pair atomically: the engine's MIDI input
	// writes both members for each event, and two separate queries
	// could tear across consecutive events.
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetLastMidiEvent ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the last MIDI event query" );
		return LastMidiEvent();
	}
	const auto& args = reply.getArgs();
	if ( args.size() < 2 ) {
		WARNINGLOG( QString( "Malformed last MIDI event reply: args [%1]" )
						.arg( args.size() ) );
		return LastMidiEvent();
	}
	return { static_cast<MidiEvent::Type>( args[0].toInt() ),
			 static_cast<Midi::Parameter>( args[1].toInt() ) };
}

QString IpcEngineAccess::getSessionFolderPath() const {
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetSessionFolderPath ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the session folder query" );
		return QString();
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return QString();
	}
	return args[0].toString();
}

std::vector<QString> IpcEngineAccess::getMidiPorts(
		MidiBaseDriver::PortType portType ) const {
	std::vector<QString> ports;
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetMidiPorts )
						   .arg( static_cast<int>( portType ) ),
					   reply ) ) {
		WARNINGLOG( QString( "Engine did not answer the MIDI port query for "
							 "port type [%1]" )
						.arg( MidiBaseDriver::portTypeToQString( portType ) ) );
		return ports;
	}
	const auto& args = reply.getArgs();
	if ( ! args.isEmpty() ) {
		for ( const auto& ssPort : args[0].toStringList() ) {
			ports.push_back( ssPort );
		}
	}
	return ports;
}

std::vector<std::shared_ptr<MidiInput::HandledInput>>
IpcEngineAccess::getHandledMidiInputs() const {
	std::vector<std::shared_ptr<MidiInput::HandledInput>> inputs;
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetHandledMidiInputs ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the handled MIDI input query" );
		return inputs;
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return inputs;
	}
	bool bOk = false;
	const int nCount = args[0].toInt( &bOk );
	// 1 count arg + 7 args per entry.
	if ( ! bOk || nCount < 0 || args.size() < 1 + nCount * 7 ) {
		WARNINGLOG( QString( "Malformed handled MIDI input reply: count "
							 "[%1], args [%2]" )
						.arg( args[0].toString() )
						.arg( args.size() ) );
		return inputs;
	}
	for ( int ii = 0; ii < nCount; ++ii ) {
		const int nBase = 1 + ii * 7;
		auto pHandled = std::make_shared<MidiInput::HandledInput>();
		// Engine and editor share the host clock, so the epoch count
		// reconstructs the original TimePoint losslessly.
		pHandled->timePoint = TimePoint(
			TimePoint::duration( args[ nBase ].toLongLong() ) );
		pHandled->type = static_cast<MidiMessage::Type>(
			args[ nBase + 1 ].toInt() );
		pHandled->data1 = static_cast<Midi::Parameter>(
			args[ nBase + 2 ].toInt() );
		pHandled->data2 = static_cast<Midi::Parameter>(
			args[ nBase + 3 ].toInt() );
		pHandled->channel = static_cast<Midi::Channel>(
			args[ nBase + 4 ].toInt() );
		for ( const auto& actionType : args[ nBase + 5 ].toList() ) {
			pHandled->actionTypes.push_back(
				static_cast<MidiAction::Type>( actionType.toInt() ) );
		}
		pHandled->mappedInstruments = args[ nBase + 6 ].toStringList();
		inputs.push_back( std::move( pHandled ) );
	}
	return inputs;
}

std::vector<std::shared_ptr<MidiOutput::HandledOutput>>
IpcEngineAccess::getHandledMidiOutputs() const {
	std::vector<std::shared_ptr<MidiOutput::HandledOutput>> outputs;
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetHandledMidiOutputs ),
					   reply ) ) {
		WARNINGLOG( "Engine did not answer the handled MIDI output query" );
		return outputs;
	}
	const auto& args = reply.getArgs();
	if ( args.isEmpty() ) {
		return outputs;
	}
	bool bOk = false;
	const int nCount = args[0].toInt( &bOk );
	// 1 count arg + 5 args per entry.
	if ( ! bOk || nCount < 0 || args.size() < 1 + nCount * 5 ) {
		WARNINGLOG( QString( "Malformed handled MIDI output reply: count "
							 "[%1], args [%2]" )
						.arg( args[0].toString() )
						.arg( args.size() ) );
		return outputs;
	}
	for ( int ii = 0; ii < nCount; ++ii ) {
		const int nBase = 1 + ii * 5;
		auto pHandled = std::make_shared<MidiOutput::HandledOutput>();
		pHandled->timePoint = TimePoint(
			TimePoint::duration( args[ nBase ].toLongLong() ) );
		pHandled->type = static_cast<MidiMessage::Type>(
			args[ nBase + 1 ].toInt() );
		pHandled->data1 = static_cast<Midi::Parameter>(
			args[ nBase + 2 ].toInt() );
		pHandled->data2 = static_cast<Midi::Parameter>(
			args[ nBase + 3 ].toInt() );
		pHandled->channel = static_cast<Midi::Channel>(
			args[ nBase + 4 ].toInt() );
		outputs.push_back( std::move( pHandled ) );
	}
	return outputs;
}

bool IpcEngineAccess::isExportWritingFailed() const
{
	// The mirror owns no disk writer — the engine's writer state
	// crosses as a query (ADR 0030 batch 2l). On timeout we report
	// "not failed": the -1 progress event is the authoritative failure
	// signal, this query only disambiguates a completed 100.
	IpcMessage reply;
	if ( ! ipcRequest( m_pChannel,
					   IpcMessage( IpcOpcode::GetExportWritingFailed ),
					   reply ) ) {
		return false;
	}
	return reply.getArgs().size() >= 1 && reply.getArgs()[0].toBool();
}
};
