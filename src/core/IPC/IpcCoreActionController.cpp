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

#include <core/IPC/IpcCoreActionController.h>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Event.h>
#include <core/Basics/GridPoint.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcMessage.h>
#include <core/License.h>

namespace H2Core {

IpcCoreActionController::IpcCoreActionController(
	Hydrogen* pMirror, IpcChannel* pChannel )
	: CoreActionController( pMirror )
	, m_pChannel( pChannel )
	, m_pMirror( pMirror ) {
}

// Each override forwards the command to the engine (opcode on the channel) and
// applies it to the local mirror via the base implementation (snappy UI). The
// arg order matches IpcEngineBridge::dispatchCommand.

bool IpcCoreActionController::setBpm( float fBpm ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetBpm ).arg( fBpm ) );
	}
	return CoreActionController::setBpm( fBpm );
}

bool IpcCoreActionController::setMasterVolume( float fVolumeValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetMasterVolume ).arg( fVolumeValue ) );
	}
	return CoreActionController::setMasterVolume( fVolumeValue );
}

bool IpcCoreActionController::setMasterIsMuted( bool bIsMuted ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetMasterIsMuted ).arg( bIsMuted ) );
	}
	return CoreActionController::setMasterIsMuted( bIsMuted );
}

bool IpcCoreActionController::setMetronomeIsActive( bool bIsActive ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetMetronomeIsActive ).arg( bIsActive ) );
	}
	return CoreActionController::setMetronomeIsActive( bIsActive );
}

bool IpcCoreActionController::locateToColumn( int nPatternGroup ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::LocateToColumn ).arg( nPatternGroup ) );
	}
	return CoreActionController::locateToColumn( nPatternGroup );
}

bool IpcCoreActionController::locateToTick( long nTick, bool bWithJackBroadcast ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::LocateToTick )
							  .arg( static_cast<qlonglong>( nTick ) )
							  .arg( bWithJackBroadcast ) );
	}
	return CoreActionController::locateToTick( nTick, bWithJackBroadcast );
}

bool IpcCoreActionController::selectPattern( int nPatternNumber ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SelectPattern ).arg( nPatternNumber ) );
	}
	return CoreActionController::selectPattern( nPatternNumber );
}

bool IpcCoreActionController::setStripVolume(
	int nStrip, float fVolumeValue, bool bSelectStrip ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetStripVolume )
							  .arg( nStrip ).arg( fVolumeValue ).arg( bSelectStrip ) );
	}
	return CoreActionController::setStripVolume( nStrip, fVolumeValue, bSelectStrip );
}

bool IpcCoreActionController::setStripPan(
	int nStrip, float fValue, bool bSelectStrip ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetStripPan )
							  .arg( nStrip ).arg( fValue ).arg( bSelectStrip ) );
	}
	return CoreActionController::setStripPan( nStrip, fValue, bSelectStrip );
}

bool IpcCoreActionController::activateLoopMode( bool bActivate ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::ActivateLoopMode ).arg( bActivate ) );
	}
	return CoreActionController::activateLoopMode( bActivate );
}

bool IpcCoreActionController::activateSongMode( bool bActivate ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::ActivateSongMode ).arg( bActivate ) );
	}
	return CoreActionController::activateSongMode( bActivate );
}

bool IpcCoreActionController::activateRecordMode( bool bActivate ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::ActivateRecordMode ).arg( bActivate ) );
	}
	return CoreActionController::activateRecordMode( bActivate );
}

bool IpcCoreActionController::addTempoMarker( int nPosition, float fBpm ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::AddTempoMarker )
							  .arg( nPosition ).arg( fBpm ) );
	}
	return CoreActionController::addTempoMarker( nPosition, fBpm );
}

bool IpcCoreActionController::addTag( int nPosition, const QString& sText ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::AddTag ).arg( nPosition ).arg( sText ) );
	}
	return CoreActionController::addTag( nPosition, sText );
}

bool IpcCoreActionController::newPattern( const QString& sPatternName ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::NewPattern ).arg( sPatternName ) );
	}
	return CoreActionController::newPattern( sPatternName );
}

bool IpcCoreActionController::quit() {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::Quit ) );
	}
	return CoreActionController::quit();
}

bool IpcCoreActionController::setInstrumentPitch( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentPitch ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentPitch( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentGain( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentGain ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentGain( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentRandomPitch( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentRandomPitch ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentRandomPitch( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentFilterCutoff( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentFilterCutoff ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentFilterCutoff( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentFilterResonance( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentFilterResonance ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentFilterResonance( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentAttack( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentAttack ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentAttack( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentDecay( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentDecay ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentDecay( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentSustain( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentSustain ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentSustain( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentRelease( int nInstrument, float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentRelease ).arg( nInstrument ).arg( fValue ) );
	}
	return CoreActionController::setInstrumentRelease( nInstrument, fValue );
}

bool IpcCoreActionController::setInstrumentFilterActive( int nInstrument, bool bActive ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentFilterActive ).arg( nInstrument ).arg( bActive ) );
	}
	return CoreActionController::setInstrumentFilterActive( nInstrument, bActive );
}

bool IpcCoreActionController::setInstrumentMuteGroup( int nInstrument, int nMuteGroup ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentMuteGroup ).arg( nInstrument ).arg( nMuteGroup ) );
	}
	return CoreActionController::setInstrumentMuteGroup( nInstrument, nMuteGroup );
}

bool IpcCoreActionController::setInstrumentStopNotes( int nInstrument, bool bStopNotes ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentStopNotes ).arg( nInstrument ).arg( bStopNotes ) );
	}
	return CoreActionController::setInstrumentStopNotes( nInstrument, bStopNotes );
}

bool IpcCoreActionController::setInstrumentApplyVelocity( int nInstrument, bool bApplyVelocity ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentApplyVelocity ).arg( nInstrument ).arg( bApplyVelocity ) );
	}
	return CoreActionController::setInstrumentApplyVelocity( nInstrument, bApplyVelocity );
}

bool IpcCoreActionController::setInstrumentHihatGroup( int nInstrument, int nHihatGroup ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentHihatGroup ).arg( nInstrument ).arg( nHihatGroup ) );
	}
	return CoreActionController::setInstrumentHihatGroup( nInstrument, nHihatGroup );
}

bool IpcCoreActionController::setInstrumentLowerCc( int nInstrument, int nCc ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentLowerCc ).arg( nInstrument ).arg( nCc ) );
	}
	return CoreActionController::setInstrumentLowerCc( nInstrument, nCc );
}

bool IpcCoreActionController::setInstrumentHigherCc( int nInstrument, int nCc ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentHigherCc ).arg( nInstrument ).arg( nCc ) );
	}
	return CoreActionController::setInstrumentHigherCc( nInstrument, nCc );
}

bool IpcCoreActionController::setComponentIsMuted( int nInstrument, int nComponent, bool bIsMuted ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetComponentIsMuted ).arg( nInstrument ).arg( nComponent ).arg( bIsMuted ) );
	}
	return CoreActionController::setComponentIsMuted( nInstrument, nComponent, bIsMuted );
}

bool IpcCoreActionController::setComponentIsSoloed( int nInstrument, int nComponent, bool bIsSoloed ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetComponentIsSoloed ).arg( nInstrument ).arg( nComponent ).arg( bIsSoloed ) );
	}
	return CoreActionController::setComponentIsSoloed( nInstrument, nComponent, bIsSoloed );
}

bool IpcCoreActionController::setComponentGain( int nInstrument, int nComponent, float fGain ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetComponentGain ).arg( nInstrument ).arg( nComponent ).arg( fGain ) );
	}
	return CoreActionController::setComponentGain( nInstrument, nComponent, fGain );
}

bool IpcCoreActionController::setComponentSelection( int nInstrument, int nComponent, int nSelection ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetComponentSelection ).arg( nInstrument ).arg( nComponent ).arg( nSelection ) );
	}
	return CoreActionController::setComponentSelection( nInstrument, nComponent, nSelection );
}

bool IpcCoreActionController::setLayerIsMuted( int nInstrument, int nComponent, int nLayer, bool bIsMuted ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetLayerIsMuted ).arg( nInstrument ).arg( nComponent ).arg( nLayer ).arg( bIsMuted ) );
	}
	return CoreActionController::setLayerIsMuted( nInstrument, nComponent, nLayer, bIsMuted );
}

bool IpcCoreActionController::setLayerIsSoloed( int nInstrument, int nComponent, int nLayer, bool bIsSoloed ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetLayerIsSoloed ).arg( nInstrument ).arg( nComponent ).arg( nLayer ).arg( bIsSoloed ) );
	}
	return CoreActionController::setLayerIsSoloed( nInstrument, nComponent, nLayer, bIsSoloed );
}

bool IpcCoreActionController::setLayerGain( int nInstrument, int nComponent, int nLayer, float fGain ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetLayerGain ).arg( nInstrument ).arg( nComponent ).arg( nLayer ).arg( fGain ) );
	}
	return CoreActionController::setLayerGain( nInstrument, nComponent, nLayer, fGain );
}

bool IpcCoreActionController::setLayerPitchOffset( int nInstrument, int nComponent, int nLayer, float fPitchOffset ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetLayerPitchOffset ).arg( nInstrument ).arg( nComponent ).arg( nLayer ).arg( fPitchOffset ) );
	}
	return CoreActionController::setLayerPitchOffset( nInstrument, nComponent, nLayer, fPitchOffset );
}

bool IpcCoreActionController::setLayerStartVelocity( int nInstrument, int nComponent, int nLayer, float fVelocity ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetLayerStartVelocity ).arg( nInstrument ).arg( nComponent ).arg( nLayer ).arg( fVelocity ) );
	}
	return CoreActionController::setLayerStartVelocity( nInstrument, nComponent, nLayer, fVelocity );
}

bool IpcCoreActionController::setLayerEndVelocity( int nInstrument, int nComponent, int nLayer, float fVelocity ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetLayerEndVelocity ).arg( nInstrument ).arg( nComponent ).arg( nLayer ).arg( fVelocity ) );
	}
	return CoreActionController::setLayerEndVelocity( nInstrument, nComponent, nLayer, fVelocity );
}

bool IpcCoreActionController::setStripIsMuted( int nStrip, bool isMuted, bool bSelectStrip ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetStripIsMuted ).arg( nStrip ).arg( isMuted ).arg( bSelectStrip ) );
	}
	return CoreActionController::setStripIsMuted( nStrip, isMuted, bSelectStrip );
}

bool IpcCoreActionController::setStripIsSoloed( int nStrip, bool isSoloed, bool bSelectStrip ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetStripIsSoloed ).arg( nStrip ).arg( isSoloed ).arg( bSelectStrip ) );
	}
	return CoreActionController::setStripIsSoloed( nStrip, isSoloed, bSelectStrip );
}

bool IpcCoreActionController::setStripPanSym( int nStrip, float fValue, bool bSelectStrip ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetStripPanSym ).arg( nStrip ).arg( fValue ).arg( bSelectStrip ) );
	}
	return CoreActionController::setStripPanSym( nStrip, fValue, bSelectStrip );
}

bool IpcCoreActionController::setHumanizeTime( float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetHumanizeTime ).arg( fValue ) );
	}
	return CoreActionController::setHumanizeTime( fValue );
}

bool IpcCoreActionController::setHumanizeVelocity( float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetHumanizeVelocity ).arg( fValue ) );
	}
	return CoreActionController::setHumanizeVelocity( fValue );
}

bool IpcCoreActionController::setSwing( float fValue ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetSwing ).arg( fValue ) );
	}
	return CoreActionController::setSwing( fValue );
}

bool IpcCoreActionController::setPanLaw( int nPanLawType, float fPanLawKNorm ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetPanLaw ).arg( nPanLawType ).arg( fPanLawKNorm ) );
	}
	return CoreActionController::setPanLaw( nPanLawType, fPanLawKNorm );
}

bool IpcCoreActionController::setPlaybackTrackMuted( bool bMuted ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetPlaybackTrackMuted ).arg( bMuted ) );
	}
	return CoreActionController::setPlaybackTrackMuted( bMuted );
}

bool IpcCoreActionController::setPlaybackTrackVolume( float fVolume ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetPlaybackTrackVolume ).arg( fVolume ) );
	}
	return CoreActionController::setPlaybackTrackVolume( fVolume );
}

bool IpcCoreActionController::previewInstrument( int nInstrument, bool bStop ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::PreviewInstrument ).arg( nInstrument ).arg( bStop ) );
	}
	return CoreActionController::previewInstrument( nInstrument, bStop );
}

bool IpcCoreActionController::activateTimeline( bool bActivate ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ActivateTimeline ).arg( bActivate ) );
	}
	return CoreActionController::activateTimeline( bActivate );
}

bool IpcCoreActionController::toggleTimeline(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ToggleTimeline ) );
	}
	return CoreActionController::toggleTimeline(  );
}

bool IpcCoreActionController::deleteTempoMarker( int nPosition ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::DeleteTempoMarker ).arg( nPosition ) );
	}
	return CoreActionController::deleteTempoMarker( nPosition );
}

bool IpcCoreActionController::deleteTag( int nPosition ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::DeleteTag ).arg( nPosition ) );
	}
	return CoreActionController::deleteTag( nPosition );
}

bool IpcCoreActionController::activateJackTransport( bool bActivate ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ActivateJackTransport ).arg( bActivate ) );
	}
	return CoreActionController::activateJackTransport( bActivate );
}

bool IpcCoreActionController::toggleJackTransport(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ToggleJackTransport ) );
	}
	return CoreActionController::toggleJackTransport(  );
}

bool IpcCoreActionController::activateJackTimebaseControl( bool bActivate ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ActivateJackTimebaseControl ).arg( bActivate ) );
	}
	return CoreActionController::activateJackTimebaseControl( bActivate );
}

bool IpcCoreActionController::toggleJackTimebaseControl(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ToggleJackTimebaseControl ) );
	}
	return CoreActionController::toggleJackTimebaseControl(  );
}

bool IpcCoreActionController::toggleSongMode(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ToggleSongMode ) );
	}
	return CoreActionController::toggleSongMode(  );
}

bool IpcCoreActionController::toggleLoopMode(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ToggleLoopMode ) );
	}
	return CoreActionController::toggleLoopMode(  );
}

bool IpcCoreActionController::moveInstrument( int nSourceIndex, int nTargetIndex ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::MoveInstrument ).arg( nSourceIndex ).arg( nTargetIndex ) );
	}
	return CoreActionController::moveInstrument( nSourceIndex, nTargetIndex );
}

bool IpcCoreActionController::renameComponent( int nInstrumentIdx, int nComponentId, const QString& sNewName ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::RenameComponent ).arg( nInstrumentIdx ).arg( nComponentId ).arg( sNewName ) );
	}
	return CoreActionController::renameComponent( nInstrumentIdx, nComponentId, sNewName );
}

bool IpcCoreActionController::toggleNextPattern( int nPatternNumber ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ToggleNextPattern ).arg( nPatternNumber ) );
	}
	return CoreActionController::toggleNextPattern( nPatternNumber );
}

bool IpcCoreActionController::movePattern( int nSourcePattern, int nTargetPattern ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::MovePattern ).arg( nSourcePattern ).arg( nTargetPattern ) );
	}
	return CoreActionController::movePattern( nSourcePattern, nTargetPattern );
}

bool IpcCoreActionController::removePattern( int nPatternNumber ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::RemovePattern ).arg( nPatternNumber ) );
	}
	return CoreActionController::removePattern( nPatternNumber );
}

bool IpcCoreActionController::setPatternSize( int nLength, int nDenominator, int nPatternNumber ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetPatternSize ).arg( nLength ).arg( nDenominator ).arg( nPatternNumber ) );
	}
	return CoreActionController::setPatternSize( nLength, nDenominator, nPatternNumber );
}

bool IpcCoreActionController::startCountIn(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::StartCountIn ) );
	}
	return CoreActionController::startCountIn(  );
}

bool IpcCoreActionController::activatePlaylistSong( int nSongNumber ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ActivatePlaylistSong ).arg( nSongNumber ) );
	}
	return CoreActionController::activatePlaylistSong( nSongNumber );
}

bool IpcCoreActionController::setMidiClockInputHandling( bool bHandle ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetMidiClockInputHandling ).arg( bHandle ) );
	}
	return CoreActionController::setMidiClockInputHandling( bHandle );
}

bool IpcCoreActionController::setMidiClockOutputSend( bool bHandle ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetMidiClockOutputSend ).arg( bHandle ) );
	}
	return CoreActionController::setMidiClockOutputSend( bHandle );
}

bool IpcCoreActionController::clearMidiInputLog(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ClearMidiInputLog ) );
	}
	return CoreActionController::clearMidiInputLog(  );
}

bool IpcCoreActionController::clearMidiOutputLog(  ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ClearMidiOutputLog ) );
	}
	return CoreActionController::clearMidiOutputLog(  );
}

bool IpcCoreActionController::editNoteProperty(
	NoteProperty property, int nPatternNumber, int nPosition,
	int nOldInstrumentId, int nNewInstrumentId, const QString& sOldType,
	const QString& sNewType, float fVelocity, float fPan, float fLeadLag,
	float fProbability, int nLength, int nNewKey, int nOldKey, int nNewOctave,
	int nOldOctave ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::EditNoteProperty )
							  .arg( static_cast<int>( property ) )
							  .arg( nPatternNumber ).arg( nPosition )
							  .arg( nOldInstrumentId ).arg( nNewInstrumentId )
							  .arg( sOldType ).arg( sNewType )
							  .arg( fVelocity ).arg( fPan ).arg( fLeadLag )
							  .arg( fProbability ).arg( nLength ).arg( nNewKey )
							  .arg( nOldKey ).arg( nNewOctave ).arg( nOldOctave ) );
	}
	return CoreActionController::editNoteProperty(
		property, nPatternNumber, nPosition, nOldInstrumentId, nNewInstrumentId,
		sOldType, sNewType, fVelocity, fPan, fLeadLag, fProbability, nLength,
		nNewKey, nOldKey, nNewOctave, nOldOctave );
}

bool IpcCoreActionController::toggleGridCell( const GridPoint& gridPoint ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::ToggleGridCell )
							  .arg( gridPoint.getColumn() )
							  .arg( gridPoint.getRow() ) );
	}
	return CoreActionController::toggleGridCell( gridPoint );
}

// ── ADR 0030 batch 2d — out-param commands ──

bool IpcCoreActionController::addOrRemoveNote(
	int nPosition, int nInstrumentId, const QString& sType, int nPatternNumber,
	int nOldLength, float fOldVelocity, float fOldPan, float fOldLeadLag,
	int nOldKey, int nOldOctave, float fOldProbability, bool bIsDelete,
	bool bIsNoteOff, bool bIsMappedToDrumkit, Uuid* pNewNoteUUid ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::AddOrRemoveNote )
							  .arg( nPosition ).arg( nInstrumentId ).arg( sType )
							  .arg( nPatternNumber ).arg( nOldLength )
							  .arg( fOldVelocity ).arg( fOldPan ).arg( fOldLeadLag )
							  .arg( nOldKey ).arg( nOldOctave ).arg( fOldProbability )
							  .arg( bIsDelete ).arg( bIsNoteOff )
							  .arg( bIsMappedToDrumkit ) );
	}
	// Dual-apply: the mirror creates its own note and fills pNewNoteUUid with the
	// mirror-side id the GUI's local undo tracks (ADR 0030; engine identifies the
	// note by value).
	return CoreActionController::addOrRemoveNote(
		nPosition, nInstrumentId, sType, nPatternNumber, nOldLength, fOldVelocity,
		fOldPan, fOldLeadLag, nOldKey, nOldOctave, fOldProbability, bIsDelete,
		bIsNoteOff, bIsMappedToDrumkit, pNewNoteUUid );
}

bool IpcCoreActionController::handleNote(
	Midi::Note note, Midi::Channel channel, float fVelocity, bool bNoteOff,
	QStringList* pMappedInstruments ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::HandleNote )
							  .arg( static_cast<int>( note ) )
							  .arg( static_cast<int>( channel ) )
							  .arg( fVelocity ).arg( bNoteOff ) );
	}
	// Dual-apply so the editor's own mapped-instruments list is filled locally;
	// the audible note is produced engine-side from the opcode.
	return CoreActionController::handleNote(
		note, channel, fVelocity, bNoteOff, pMappedInstruments );
}

bool IpcCoreActionController::setInstrumentMidiOutNote(
	int nInstrument, Midi::Note note, long* pEventId ) {
	if ( pEventId != nullptr && m_pChannel != nullptr ) {
		// The caller needs the engine's feedback-event id (to ignore the echo);
		// round-trip for the authoritative value (ADR 0030 tier 3).
		IpcMessage reply;
		if ( m_pChannel->request(
				 IpcMessage( IpcOpcode::SetInstrumentMidiOutNote )
					 .arg( nInstrument ).arg( static_cast<int>( note ) ),
				 reply ) && ! reply.getArgs().isEmpty() ) {
			*pEventId = static_cast<long>( reply.getArgs()[0].toLongLong() );
		}
		else {
			*pEventId = Event::nInvalidId;
		}
		// Keep the mirror's instrument property in sync for display.
		long nIgnored = Event::nInvalidId;
		return CoreActionController::setInstrumentMidiOutNote(
			nInstrument, note, &nIgnored );
	}
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentMidiOutNote )
							  .arg( nInstrument ).arg( static_cast<int>( note ) ) );
	}
	return CoreActionController::setInstrumentMidiOutNote(
		nInstrument, note, pEventId );
}

bool IpcCoreActionController::setInstrumentMidiOutChannel(
	int nInstrument, Midi::Channel channel, long* pEventId ) {
	if ( pEventId != nullptr && m_pChannel != nullptr ) {
		IpcMessage reply;
		if ( m_pChannel->request(
				 IpcMessage( IpcOpcode::SetInstrumentMidiOutChannel )
					 .arg( nInstrument ).arg( static_cast<int>( channel ) ),
				 reply ) && ! reply.getArgs().isEmpty() ) {
			*pEventId = static_cast<long>( reply.getArgs()[0].toLongLong() );
		}
		else {
			*pEventId = Event::nInvalidId;
		}
		long nIgnored = Event::nInvalidId;
		return CoreActionController::setInstrumentMidiOutChannel(
			nInstrument, channel, &nIgnored );
	}
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetInstrumentMidiOutChannel )
							  .arg( nInstrument )
							  .arg( static_cast<int>( channel ) ) );
	}
	return CoreActionController::setInstrumentMidiOutChannel(
		nInstrument, channel, pEventId );
}

// ── ADR 0030 batch 2e — object-payload / value-struct commands ──

bool IpcCoreActionController::setSong( std::shared_ptr<Song> pSong ) {
	if ( m_pChannel != nullptr && pSong != nullptr ) {
		// Whole-song bulk load: the song XML is the payload (ADR 0027/0030 —
		// reserved for genuine bulk loads). The engine reconstructs it.
		IpcMessage msg( IpcOpcode::SetSong );
		msg.setPayload( pSong->toXmlBuffer() );
		m_pChannel->send( msg );
	}
	// Dual-apply: the mirror takes the live object directly (no round-trip).
	return CoreActionController::setSong( pSong );
}

bool IpcCoreActionController::setSongProperties(
	const QString& sNewPath, const int nNewVersion, const QString& sNewName,
	const QString& sNewAuthor, const QString& sNewNotes,
	const H2Core::License& newLicense, const QStringList& newTags ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetSongProperties )
							  .arg( sNewPath ).arg( nNewVersion ).arg( sNewName )
							  .arg( sNewAuthor ).arg( sNewNotes )
							  .arg( newLicense.getLicenseString() )
							  .arg( newLicense.getCopyrightHolder() )
							  .arg( newTags ) );
	}
	return CoreActionController::setSongProperties(
		sNewPath, nNewVersion, sNewName, sNewAuthor, sNewNotes, newLicense,
		newTags );
}

bool IpcCoreActionController::setPatternProperties(
	const QString& sNewPatternPath, const int nNewVersion,
	const QString& sNewPatternName, const QString& sNewAuthor,
	const QString& sNewPatternInfo, const H2Core::License& newLicense,
	const QStringList& newTags, int nPatternIndex ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SetPatternProperties )
							  .arg( sNewPatternPath ).arg( nNewVersion )
							  .arg( sNewPatternName ).arg( sNewAuthor )
							  .arg( sNewPatternInfo )
							  .arg( newLicense.getLicenseString() )
							  .arg( newLicense.getCopyrightHolder() )
							  .arg( newTags ).arg( nPatternIndex ) );
	}
	return CoreActionController::setPatternProperties(
		sNewPatternPath, nNewVersion, sNewPatternName, sNewAuthor,
		sNewPatternInfo, newLicense, newTags, nPatternIndex );
}

// ── ADR 0030 batch 2f — object-payload (XML buffer) + file-save commands ──

bool IpcCoreActionController::setDrumkit( std::shared_ptr<Drumkit> pDrumkit ) {
	if ( m_pChannel != nullptr && pDrumkit != nullptr ) {
		// The drumkit XML is the payload; the engine reconstructs it and reloads
		// samples from their (shared-disk) paths (ADR 0030).
		IpcMessage msg( IpcOpcode::SetDrumkit );
		msg.setPayload( pDrumkit->toXmlBuffer( true ) );
		m_pChannel->send( msg );
	}
	// Dual-apply: the mirror takes the live object directly (no round-trip).
	return CoreActionController::setDrumkit( pDrumkit );
}

bool IpcCoreActionController::setPattern(
	std::shared_ptr<Pattern> pPattern, int nPatternNumber, bool bReplace ) {
	if ( m_pChannel != nullptr && pPattern != nullptr ) {
		// Serialise the pattern against the current drumkit so the engine can
		// resolve instrument ids/types on the far side.
		std::shared_ptr<Drumkit> pDrumkit = nullptr;
		if ( m_pMirror != nullptr && m_pMirror->getSong() != nullptr ) {
			pDrumkit = m_pMirror->getSong()->getDrumkit();
		}
		IpcMessage msg( IpcOpcode::SetPattern );
		msg.arg( nPatternNumber ).arg( bReplace );
		msg.setPayload( pPattern->toXmlBuffer( pDrumkit ) );
		m_pChannel->send( msg );
	}
	return CoreActionController::setPattern( pPattern, nPatternNumber, bReplace );
}

bool IpcCoreActionController::replaceInstrument(
	std::shared_ptr<Instrument> pNewInstrument,
	std::shared_ptr<Instrument> pOldInstrument ) {
	if ( m_pChannel != nullptr && pNewInstrument != nullptr &&
		 pOldInstrument != nullptr ) {
		// The new instrument rides as payload; the old one is identified by id so
		// the engine can locate its own copy in the authoritative drumkit.
		IpcMessage msg( IpcOpcode::ReplaceInstrument );
		msg.arg( static_cast<int>( pOldInstrument->getId() ) );
		msg.setPayload( pNewInstrument->toXmlBuffer( true ) );
		m_pChannel->send( msg );
	}
	return CoreActionController::replaceInstrument(
		pNewInstrument, pOldInstrument );
}

bool IpcCoreActionController::addInstrument(
	std::shared_ptr<Instrument> pInstrument, int nIndex, long* pEventId ) {
	if ( pEventId != nullptr && m_pChannel != nullptr && pInstrument != nullptr ) {
		// The caller needs the engine's feedback-event id; round-trip for the
		// authoritative value (ADR 0030 tier 3).
		IpcMessage req( IpcOpcode::AddInstrument );
		req.arg( nIndex );
		req.setPayload( pInstrument->toXmlBuffer( true ) );
		IpcMessage reply;
		if ( m_pChannel->request( req, reply ) && ! reply.getArgs().isEmpty() ) {
			*pEventId = static_cast<long>( reply.getArgs()[0].toLongLong() );
		}
		else {
			*pEventId = Event::nInvalidId;
		}
		// Keep the mirror in sync for display (its own event id is irrelevant).
		long nIgnored = Event::nInvalidId;
		return CoreActionController::addInstrument(
			pInstrument, nIndex, &nIgnored );
	}
	if ( m_pChannel != nullptr && pInstrument != nullptr ) {
		IpcMessage msg( IpcOpcode::AddInstrument );
		msg.arg( nIndex );
		msg.setPayload( pInstrument->toXmlBuffer( true ) );
		m_pChannel->send( msg );
	}
	return CoreActionController::addInstrument( pInstrument, nIndex, pEventId );
}

bool IpcCoreActionController::saveSong( bool bKeepMissingSamples ) {
	// Engine-only: the authoritative engine writes the shared file. The editor
	// mirror must NOT also write it (double-write / race on the same path).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SaveSong ).arg( bKeepMissingSamples ) );
		return true;
	}
	return CoreActionController::saveSong( bKeepMissingSamples );
}

bool IpcCoreActionController::saveSongAs(
	const QString& sNewFileName, bool bKeepMissingSamples ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SaveSongAs )
							  .arg( sNewFileName ).arg( bKeepMissingSamples ) );
		return true;
	}
	return CoreActionController::saveSongAs( sNewFileName, bKeepMissingSamples );
}

bool IpcCoreActionController::savePlaylist() {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SavePlaylist ) );
		return true;
	}
	return CoreActionController::savePlaylist();
}

bool IpcCoreActionController::savePlaylistAs( const QString& sPath ) {
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::SavePlaylistAs ).arg( sPath ) );
		return true;
	}
	return CoreActionController::savePlaylistAs( sPath );
}

}
