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

	// ADR 0030 batch 2c — note / grid edits (enum + GridPoint args).
	bool editNoteProperty( NoteProperty property, int nPatternNumber,
		int nPosition, int nOldInstrumentId, int nNewInstrumentId,
		const QString& sOldType, const QString& sNewType, float fVelocity,
		float fPan, float fLeadLag, float fProbability, int nLength, int nNewKey,
		int nOldKey, int nNewOctave, int nOldOctave ) override;
	bool toggleGridCell( const GridPoint& gridPoint ) override;

	// ADR 0030 batch 2d — out-param commands. addOrRemoveNote/handleNote are
	// dual-applied (out-param filled by the mirror via the base call);
	// setInstrumentMidiOut* use request/response when the caller needs the
	// engine's feedback-event id.
	bool addOrRemoveNote( int nPosition, int nInstrumentId, const QString& sType,
		int nPatternNumber, int nOldLength, float fOldVelocity, float fOldPan,
		float fOldLeadLag, int nOldKey, int nOldOctave, float fOldProbability,
		bool bIsDelete, bool bIsNoteOff, bool bIsMappedToDrumkit,
		Uuid* pNewNoteUUid ) override;
	bool handleNote( Midi::Note note, Midi::Channel channel, float fVelocity,
		bool bNoteOff, QStringList* pMappedInstruments ) override;
	bool setInstrumentMidiOutNote( int nInstrument, Midi::Note note,
		long* pEventId ) override;
	bool setInstrumentMidiOutChannel( int nInstrument, Midi::Channel channel,
		long* pEventId ) override;

	// ADR 0030 batch 2e — object-payload (setSong: song XML) / value-struct
	// (*Properties: strings + License + tags) commands.
	bool setSong( std::shared_ptr<Song> pSong ) override;
	bool setSongProperties( const QString& sNewPath, const int nNewVersion,
		const QString& sNewName, const QString& sNewAuthor,
		const QString& sNewNotes, const H2Core::License& newLicense,
		const QStringList& newTags ) override;
	bool setPatternProperties( const QString& sNewPatternPath,
		const int nNewVersion, const QString& sNewPatternName,
		const QString& sNewAuthor, const QString& sNewPatternInfo,
		const H2Core::License& newLicense, const QStringList& newTags,
		int nPatternIndex ) override;

	// ADR 0030 batch 2f — object-payload (XML buffer) commands + file-save.
	// setDrumkit/setPattern/replaceInstrument marshal the object as an XML-buffer
	// payload and dual-apply; addInstrument is request/response when the caller
	// needs the engine's event id. The save* commands are engine-only (no mirror
	// write to the shared file).
	bool setDrumkit( std::shared_ptr<Drumkit> pDrumkit ) override;
	bool setPattern( std::shared_ptr<Pattern> pPattern, int nPatternNumber,
		bool bReplace ) override;
	bool replaceInstrument( std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument ) override;
	bool addInstrument( std::shared_ptr<Instrument> pInstrument, int nIndex,
		long* pEventId ) override;
	bool saveSong( bool bKeepMissingSamples ) override;
	bool saveSongAs( const QString& sNewFileName,
		bool bKeepMissingSamples ) override;
	bool savePlaylist() override;
	bool savePlaylistAs( const QString& sPath ) override;

private:
	/** Control channel to the authoritative engine; not owned. */
	IpcChannel* m_pChannel;
	/** The editor-side mirror engine (read-model), used to resolve serialisation
	 * context (e.g. the current drumkit for a pattern). Not owned. */
	Hydrogen* m_pMirror;
};

}

#endif
