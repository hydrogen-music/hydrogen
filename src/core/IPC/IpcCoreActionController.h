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

#ifndef H2C_IPC_CORE_ACTION_CONTROLLER_H
#define H2C_IPC_CORE_ACTION_CONTROLLER_H

#include <core/CoreActionController.h>

namespace H2Core {

class IpcChannel;

/**
 * \ingroup docCore
 *
 * Editor-mode `CoreActionController` (ADR 0030). The GUI calls the same
 * `CoreActionController` surface as in standalone, but in editor mode
 * #IpcEngineAccess hands it this subclass: each command is **marshalled to an
 * `IpcMessage` and sent to the authoritative engine** over the #IpcChannel, and
 * is **also applied to the local mirror** (via the base implementation) so the
 * editor's read-model and UI stay responsive — the pattern already used by
 * `IpcEngineAccess::sequencerStop`.
 *
 * \note This first stage overrides the commands the #IpcEngineBridge already
 *   dispatches; the override set is extended in lockstep with the bridge /
 *   `IpcOpcode` vocabulary. Methods not yet overridden fall through to the base
 *   (mirror-only) — the pre-ADR-0030 behaviour — until they are added. The
 *   `bool`-returning commands return the local result optimistically; the engine
 *   is authoritative and reports genuine failures via events (ADR 0030).
 */
class IpcCoreActionController : public CoreActionController {
public:
	IpcCoreActionController( Hydrogen* pMirror, IpcChannel* pChannel );

	bool setBpm( float fBpm ) override;
	bool setMasterVolume( float fVolumeValue ) override;
	bool setMasterIsMuted( bool bIsMuted ) override;
	bool setMetronomeIsActive( bool bIsActive ) override;
	bool locateToColumn( int nPatternGroup ) override;
	bool locateToTick( long nTick, bool bWithJackBroadcast ) override;
	bool selectPattern( int nPatternNumber ) override;
	bool setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip ) override;
	bool setStripPan( int nStrip, float fValue, bool bSelectStrip ) override;
	bool activateLoopMode( bool bActivate ) override;
	bool activateSongMode( bool bActivate ) override;
	bool activateRecordMode( bool bActivate ) override;
	bool addTempoMarker( int nPosition, float fBpm ) override;
	bool addTag( int nPosition, const QString& sText ) override;
	bool newPattern( const QString& sPatternName ) override;
	bool quit() override;

	// ADR 0030 batch 2a — scalar parameter setters.
	bool setInstrumentPitch( int nInstrument, float fValue ) override;
	bool setInstrumentGain( int nInstrument, float fValue ) override;
	bool setInstrumentRandomPitch( int nInstrument, float fValue ) override;
	bool setInstrumentFilterCutoff( int nInstrument, float fValue ) override;
	bool setInstrumentFilterResonance( int nInstrument, float fValue ) override;
	bool setInstrumentAttack( int nInstrument, float fValue ) override;
	bool setInstrumentDecay( int nInstrument, float fValue ) override;
	bool setInstrumentSustain( int nInstrument, float fValue ) override;
	bool setInstrumentRelease( int nInstrument, float fValue ) override;
	bool setInstrumentFilterActive( int nInstrument, bool bActive ) override;
	bool setInstrumentMuteGroup( int nInstrument, int nMuteGroup ) override;
	bool setInstrumentStopNotes( int nInstrument, bool bStopNotes ) override;
	bool setInstrumentApplyVelocity( int nInstrument, bool bApplyVelocity ) override;
	bool setInstrumentHihatGroup( int nInstrument, int nHihatGroup ) override;
	bool setInstrumentLowerCc( int nInstrument, int nCc ) override;
	bool setInstrumentHigherCc( int nInstrument, int nCc ) override;
	bool setComponentIsMuted( int nInstrument, int nComponent, bool bIsMuted ) override;
	bool setComponentIsSoloed( int nInstrument, int nComponent, bool bIsSoloed ) override;
	bool setComponentGain( int nInstrument, int nComponent, float fGain ) override;
	bool setComponentSelection( int nInstrument, int nComponent, int nSelection ) override;
	bool setLayerIsMuted( int nInstrument, int nComponent, int nLayer, bool bIsMuted ) override;
	bool setLayerIsSoloed( int nInstrument, int nComponent, int nLayer, bool bIsSoloed ) override;
	bool setLayerGain( int nInstrument, int nComponent, int nLayer, float fGain ) override;
	bool setLayerPitchOffset( int nInstrument, int nComponent, int nLayer, float fPitchOffset ) override;
	bool setLayerStartVelocity( int nInstrument, int nComponent, int nLayer, float fVelocity ) override;
	bool setLayerEndVelocity( int nInstrument, int nComponent, int nLayer, float fVelocity ) override;
	bool setStripIsMuted( int nStrip, bool isMuted, bool bSelectStrip ) override;
	bool setStripIsSoloed( int nStrip, bool isSoloed, bool bSelectStrip ) override;
	bool setStripPanSym( int nStrip, float fValue, bool bSelectStrip ) override;
	bool setHumanizeTime( float fValue ) override;
	bool setHumanizeVelocity( float fValue ) override;
	bool setSwing( float fValue ) override;
	bool setPanLaw( int nPanLawType, float fPanLawKNorm ) override;
	bool setPlaybackTrackMuted( bool bMuted ) override;
	bool setPlaybackTrackVolume( float fVolume ) override;


	// ADR 0030 batch 2b — simple commands.
	bool previewInstrument( int nInstrument, bool bStop ) override;
	bool activateTimeline( bool bActivate ) override;
	bool toggleTimeline(  ) override;
	bool deleteTempoMarker( int nPosition ) override;
	bool deleteTag( int nPosition ) override;
	bool activateJackTransport( bool bActivate ) override;
	bool toggleJackTransport(  ) override;
	bool activateJackTimebaseControl( bool bActivate ) override;
	bool toggleJackTimebaseControl(  ) override;
	bool toggleSongMode(  ) override;
	bool toggleLoopMode(  ) override;
	bool moveInstrument( int nSourceIndex, int nTargetIndex ) override;
	bool renameComponent( int nInstrumentIdx, int nComponentId, const QString& sNewName ) override;
	bool toggleNextPattern( int nPatternNumber ) override;
	bool movePattern( int nSourcePattern, int nTargetPattern ) override;
	bool removePattern( int nPatternNumber ) override;
	bool setPatternSize( int nLength, int nDenominator, int nPatternNumber ) override;
	bool startCountIn(  ) override;
	bool activatePlaylistSong( int nSongNumber ) override;
	bool setMidiClockInputHandling( bool bHandle ) override;
	bool setMidiClockOutputSend( bool bHandle ) override;
	bool clearMidiInputLog(  ) override;
	bool clearMidiOutputLog(  ) override;


private:
	/** Control channel to the authoritative engine; not owned. */
	IpcChannel* m_pChannel;
};

}

#endif
