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

	bool setBpm( float fBpm, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setMasterVolume( float fVolumeValue, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setMasterIsMuted( bool bIsMuted, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setMetronomeIsActive( bool bIsActive ) override;
	bool locateToColumn( int nPatternGroup ) override;
	bool locateToTick( long nTick, bool bWithJackBroadcast ) override;
	bool selectPattern( int nPatternNumber ) override;
	bool setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setStripPan( int nStrip, float fValue, bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool activateLoopMode( bool bActivate ) override;
	bool activateSongMode( bool bActivate ) override;
	bool activateRecordMode( bool bActivate ) override;
	bool addTempoMarker( int nPosition, float fBpm,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool addTag( int nPosition, const QString& sText,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool addAutomationPoint( float fX, float fY,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool removeAutomationPoint( float fX,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool quit() override;
	bool panic() override;

	// ADR 0030 batch 2a — scalar parameter setters.
	bool setInstrumentPitch( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentGain( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentRandomPitch( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentFilterCutoff( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentFilterResonance( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentAttack( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentDecay( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentSustain( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentRelease( int nInstrument, float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentFilterActive( int nInstrument, bool bActive,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentMuteGroup( int nInstrument, int nMuteGroup,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentStopNotes( int nInstrument, bool bStopNotes,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentApplyVelocity( int nInstrument, bool bApplyVelocity,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentHihatGroup( int nInstrument, int nHihatGroup,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentLowerCc( int nInstrument, int nCc,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentHigherCc( int nInstrument, int nCc,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentMidiOutNote( int nInstrument, Midi::Note note,
		long nEventId, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setInstrumentMidiOutChannel( int nInstrument, Midi::Channel channel,
		long nEventId, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setComponentIsMuted( int nInstrument, int nComponent, bool bIsMuted,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setComponentIsSoloed( int nInstrument, int nComponent, bool bIsSoloed,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setComponentGain( int nInstrument, int nComponent, float fGain,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setComponentSelection( int nInstrument, int nComponent, int nSelection,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setLayerIsMuted( int nInstrument, int nComponent, int nLayer, bool bIsMuted,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setLayerIsSoloed( int nInstrument, int nComponent, int nLayer, bool bIsSoloed,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setLayerGain( int nInstrument, int nComponent, int nLayer, float fGain,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setLayerPitchOffset( int nInstrument, int nComponent, int nLayer, float fPitchOffset,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setLayerStartVelocity( int nInstrument, int nComponent, int nLayer, float fVelocity,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setLayerEndVelocity( int nInstrument, int nComponent, int nLayer, float fVelocity,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setStripIsMuted( int nStrip, bool isMuted, bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setStripIsSoloed( int nStrip, bool isSoloed, bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setStripPanSym( int nStrip, float fValue, bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setHumanizeTime( float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setHumanizeVelocity( float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setSwing( float fValue,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setPanLaw( int nPanLawType, float fPanLawKNorm,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setPlaybackTrackMuted( bool bMuted,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setPlaybackTrackVolume( float fVolume,
		Event::Trigger trigger = Event::Trigger::Default ) override;


	// ADR 0030 batch 2b — simple commands.
	bool previewInstrument( int nInstrument, bool bStop ) override;

	// Ad-hoc instrument preview (file browser, sound library, sample
	// editor): the instrument and its note ride as XML buffers — the
	// mirror can not render audio; the engine reloads the samples from
	// their (shared-disk) paths (ADR 0026 point 11).
	bool previewInstrument( std::shared_ptr<Instrument> pInstrument,
							std::shared_ptr<Note> pNote ) override;

	// Note preview: the note rides as an XML-buffer payload and is forwarded
	// to the authoritative engine's Sampler (ADR 0030). No dual-apply — the
	// mirror has no real sampler.
	bool noteOn( std::shared_ptr<Note> pNote ) override;
	bool releasePlayingNotes( Uuid instrumentUuid ) override;
	bool activateTimeline( bool bActivate ) override;
	bool toggleTimeline(  ) override;
	bool deleteTempoMarker( int nPosition,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool deleteTag( int nPosition,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool activateJackTransport( bool bActivate ) override;
	bool toggleJackTransport(  ) override;
	bool activateJackTimebaseControl( bool bActivate ) override;
	bool toggleJackTimebaseControl(  ) override;
	bool toggleSongMode(  ) override;
	bool toggleLoopMode(  ) override;
	bool moveInstrument( int nSourceIndex, int nTargetIndex,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool renameComponent( int nInstrumentIdx, int nComponentId, const QString& sNewName,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool toggleNextPattern( int nPatternNumber ) override;
	bool movePattern( int nSourcePattern, int nTargetPattern,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool removePattern( int nPatternNumber,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setPatternSize( int nLength, int nDenominator, int nPatternNumber,
		Event::Trigger trigger = Event::Trigger::Default ) override;
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
		int nOldKey, int nNewOctave, int nOldOctave,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool removeNote( Uuid noteUuid, Uuid patternUuid,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool toggleGridCell( const GridPoint& gridPoint,
		Event::Trigger trigger = Event::Trigger::Default ) override;

	// ADR 0030 batch 2d — out-param commands. addOrRemoveNote/handleNote are
	// dual-applied (out-param filled by the mirror via the base call);
	bool addOrRemoveNote( int nPosition, int nInstrumentId, const QString& sType,
		int nPatternNumber, int nOldLength, float fOldVelocity, float fOldPan,
		float fOldLeadLag, int nOldKey, int nOldOctave, float fOldProbability,
		bool bIsDelete, bool bIsNoteOff, bool bIsMappedToDrumkit,
		Uuid* pNewNoteUUid, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool handleNote( Midi::Note note, Midi::Channel channel, float fVelocity,
		bool bNoteOff, QStringList* pMappedInstruments ) override;

	// ADR 0030 batch 2e — object-payload (setSong: song XML) / value-struct
	// (*Properties: strings + License + tags) commands.
	bool setSong( std::shared_ptr<Song> pSong,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setPreferences( std::shared_ptr<Preferences> pPreferences ) override;
	/** Forwards the OSC-server restart to the authoritative engine (the
	 * mirror holds no server; its local base call is a no-op). */
	bool recreateOscServer() override;
	bool setSongProperties( const QString& sNewPath, const int nNewVersion,
		const QString& sNewName, const QString& sNewAuthor,
		const QString& sNewNotes, const H2Core::License& newLicense,
		const QStringList& newTags,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setPatternProperties( const QString& sNewPatternPath,
		const int nNewVersion, const QString& sNewPatternName,
		const QString& sNewAuthor, const QString& sNewPatternInfo,
		const H2Core::License& newLicense, const QStringList& newTags,
		int nPatternIndex, Event::Trigger trigger = Event::Trigger::Default ) override;

	// ADR 0030 batch 2f — object-payload (XML buffer) commands + file-save.
	// setDrumkit/setPattern/replaceInstrument marshal the object as an XML-buffer
	// payload and dual-apply; addInstrument is request/response when the caller
	// needs the engine's event id. The save* commands are engine-only (no mirror
	// write to the shared file).
	bool setDrumkit( std::shared_ptr<Drumkit> pDrumkit,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool setPattern( std::shared_ptr<Pattern> pPattern, int nPatternNumber,
		bool bReplace, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool replaceInstrument( std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	bool addInstrument( std::shared_ptr<Instrument> pInstrument, int nIndex,
		long nEventId, Event::Trigger trigger = Event::Trigger::Default ) override;
	bool removeInstrument(
		std::shared_ptr<Instrument> pInstrument,
		long nEventId,
		Event::Trigger trigger = Event::Trigger::Default
	) override;
	bool saveSong( bool bKeepMissingSamples ) override;
	bool saveSongAs( const QString& sNewFileName,
		bool bKeepMissingSamples ) override;
	bool savePlaylist() override;
	bool savePlaylistAs( const QString& sPath ) override;

	// ADR 0030 batch 2g — playlist commands. setPlaylist marshals the playlist
	// XML as payload; add/removeFromPlaylist marshal a single entry as mime text.
	bool setPlaylist( std::shared_ptr<Playlist> pPlaylist ) override;
	bool addToPlaylist( std::shared_ptr<PlaylistEntry> pEntry,
		int nIndex ) override;
	bool removeFromPlaylist( std::shared_ptr<PlaylistEntry> pEntry,
		int nIndex ) override;

	// ADR 0030 batch 2h — MIDI action map. The editor's MIDI action table
	// is the sole writer of the map; the override marshals it as an XML
	// payload so the authoritative engine's MIDI dispatch applies the same
	// bindings (the base call installs the very object on the mirror).
	bool setMidiEventMap( std::shared_ptr<MidiEventMap> pMidiEventMap ) override;

	// ADR 0030 batch 2i — MIDI instrument map. Same whole-map payload
	// shape as batch 2h; the base call installs the very object on the
	// mirror.
	bool setMidiInstrumentMap(
		std::shared_ptr<MidiInstrumentMap> pMidiInstrumentMap ) override;

	// ADR 0030 batch 2j — MIDI-learn channel. The reset crosses so the
	// authoritative engine's MIDI input starts from a clean slate; the
	// base call keeps the mirror's copy coherent.
	bool setLastMidiEvent( const MidiEvent::Type& type,
						   Midi::Parameter parameter ) override;

	// ADR 0030 batch 2k — custom sound library dirs. The dir crosses
	// so the authoritative engine's preferences and sound library
	// database pick it up; the base call keeps the mirror's copy and
	// database coherent.
	bool addCustomSoundLibraryDir( const QString& sDirPath ) override;
	bool removeCustomSoundLibraryDir( const QString& sDirPath ) override;

	// ADR 0030 batch 2l — song export. The render pipeline only runs
	// in the authoritative engine (the mirror's process loop skips
	// rendering by design), so the whole plan crosses as one
	// request/response and the mirror must not also run it. The stop
	// is a fire-and-forget command: the engine restores its own
	// state, and the dialog keeps its editor-local preferences
	// restore.
	bool exportSong( int nSampleRate, int nSampleDepth,
					 double fCompressionLevel,
					 Interpolation::InterpolateMode interpolateMode,
					 bool bRubberbandBatchMode,
					 const std::vector<ExportRender>& renders ) override;
	void stopExportSession() override;

private:
	/** Control channel to the authoritative engine; not owned. */
	IpcChannel* m_pChannel;
	/** The editor-side mirror engine (read-model), used to resolve serialisation
	 * context (e.g. the current drumkit for a pattern). Not owned. */
	Hydrogen* m_pMirror;
};

}

#endif
