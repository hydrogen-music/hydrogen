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

#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcMessage.h>

namespace H2Core {

IpcCoreActionController::IpcCoreActionController(
	Hydrogen* pMirror, IpcChannel* pChannel )
	: CoreActionController( pMirror )
	, m_pChannel( pChannel ) {
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



}
