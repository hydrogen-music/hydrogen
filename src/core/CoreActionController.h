/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef CORE_ACTION_CONTROLLER_H
#define CORE_ACTION_CONTROLLER_H

#include <memory>
#include <QString>
#include <vector>

#include <core/Basics/DrumkitMap.h>
#include <core/Basics/Event.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiEvent.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Sampler/Interpolation.h>

namespace H2Core {
class Drumkit;
struct ExportRender;
class GridPoint;
class Hydrogen;
class Instrument;
class InstrumentComponent;
class InstrumentLayer;
class MidiEventMap;
class MidiInstrumentMap;
class Note;
class Pattern;
class Playlist;
struct PlaylistEntry;
class Preferences;
class Song;

/** Identifies which #H2Core::Note property #CoreActionController::editNoteProperty
 * sets. Values mirror the GUI's PatternEditor::Property so a cast is faithful. */
enum class NoteProperty {
	Velocity = 0,
	Pan = 1,
	LeadLag = 2,
	KeyOctave = 3,
	Probability = 4,
	Length = 5,
	Type = 6,
	InstrumentId = 7
};

/** \ingroup docCore docAutomation */
class CoreActionController : public H2Core::Object<CoreActionController> {
	H2_OBJECT( CoreActionController )

   public:
		/** @param pHydrogen Owning Hydrogen instance (ADR 0015). */
		CoreActionController( Hydrogen* pHydrogen );
	/** Virtual so editor mode can substitute #IpcCoreActionController, which
	 * marshals each command over IPC instead of mutating locally (ADR 0030). */
	virtual ~CoreActionController() = default;
	virtual bool setMasterVolume(
		float masterVolumeValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * \param nStrip Instrument which to set the volume for.
	 * \param fVolumeValue New volume.
	 * \param bSelectStrip Whether the corresponding instrument
	 * should be selected.
	 */
	virtual bool setStripVolume(
		int nStrip,
		float fVolumeValue,
		bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * \param nStrip Instrument which to set the pan for.
	 * \param fValue New pan.
	 * \param bSelectStrip Whether the corresponding instrument
	 * should be selected.
	 */
	virtual bool setStripPan(
		int nStrip,
		float fValue,
		bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * \param nStrip Instrument which to set the pan for.
	 * \param fValue New pan. range in [-1;1] => symmetric respect to 0
	 * \param bSelectStrip Whether the corresponding instrument
	 * should be selected.
	 */
	virtual bool setStripPanSym(
		int nStrip,
		float fValue,
		bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * Resets the MIDI out note of every instrument in the current
	 * drumkit to its list-slot default.
	 *
	 * Idempotent: when every instrument already carries its default
	 * note, neither the modified flag nor parameter events are touched.
	 */
	virtual bool setDefaultMidiOutNotes(
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * Recalculates the rubberband preprocessing of all samples in the
	 * current drumkit for the current tempo (no-op unless rubberband
	 * batch mode is enabled). Swaps in-memory samples; each side of the
	 * split applies it on its own copy.
	 */
	virtual bool recalculateRubberband();
	virtual bool setInstrumentPitch(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentGain(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentRandomPitch(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentFilterCutoff(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentFilterResonance(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentAttack(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentDecay(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentSustain(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentRelease(
		int nInstrument,
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentFilterActive(
		int nInstrument,
		bool bActive,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentMuteGroup(
		int nInstrument,
		int nMuteGroup,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentStopNotes(
		int nInstrument,
		bool bStopNotes,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentApplyVelocity(
		int nInstrument,
		bool bApplyVelocity,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentHihatGroup(
		int nInstrument,
		int nHihatGroup,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentLowerCc(
		int nInstrument,
		int nCc,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentHigherCc(
		int nInstrument,
		int nCc,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setComponentIsMuted(
		int nInstrument,
		int nComponent,
		bool bIsMuted,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setComponentIsSoloed(
		int nInstrument,
		int nComponent,
		bool bIsSoloed,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setComponentGain(
		int nInstrument,
		int nComponent,
		float fGain,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** \param nSelection underlying value of #H2Core::InstrumentComponent::Selection. */
	virtual bool setComponentSelection(
		int nInstrument,
		int nComponent,
		int nSelection,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setLayerIsMuted(
		int nInstrument,
		int nComponent,
		int nLayer,
		bool bIsMuted,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setLayerIsSoloed(
		int nInstrument,
		int nComponent,
		int nLayer,
		bool bIsSoloed,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setLayerGain(
		int nInstrument,
		int nComponent,
		int nLayer,
		float fGain,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setLayerPitchOffset(
		int nInstrument,
		int nComponent,
		int nLayer,
		float fPitchOffset,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setLayerStartVelocity(
		int nInstrument,
		int nComponent,
		int nLayer,
		float fVelocity,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setLayerEndVelocity(
		int nInstrument,
		int nComponent,
		int nLayer,
		float fVelocity,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentMidiOutNote(
		int nInstrument,
		Midi::Note note,
		long nEventId,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setInstrumentMidiOutChannel(
		int nInstrument,
		Midi::Channel channel,
		long nEventId,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setMetronomeIsActive( bool isActive );
	virtual bool setMetronomeVolume( float fVolume );
	virtual bool setMasterIsMuted(
		bool isMuted,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setHumanizeTime(
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setHumanizeVelocity(
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool setSwing(
		float fValue,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Sets the song-global pan law (MixerSettingsDialog). @a nPanLawType is a
	 * #H2Core::Sampler pan-law constant. */
	virtual bool setPanLaw(
		int nPanLawType,
		float fPanLawKNorm,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Auditions an instrument through the #H2Core::Sampler (the mixer-strip
	 * "play sample" / "stop sample" preview buttons). Builds a transient note
	 * for the instrument at @a nInstrument and triggers it; @a bStop sends a
	 * note-off (stop) instead of a full-velocity note-on.
	 *
	 * @return true on success. */
	virtual bool previewInstrument( int nInstrument, bool bStop );

	/** Auditions an ad-hoc instrument — one that is not part of the current
	 * song's kit (file browser, sound library, sample editor) — through the
	 * #H2Core::Sampler. @a pNote must already be bound to @a pInstrument
	 * (Sampler::previewInstrument() enforces this). In editor mode this is
	 * forwarded over IPC to the authoritative engine — the instrument and
	 * note cross as XML and the engine reloads the samples from their
	 * (shared-disk) paths; in standalone the local sampler plays it. A
	 * note's SelectedLayerInfo does not cross for ad-hoc instruments (it
	 * is resolved only against kit instruments on deserialization).
	 *
	 * @return true on success. */
	virtual bool previewInstrument( std::shared_ptr<Instrument> pInstrument,
									std::shared_ptr<Note> pNote );

	/** Triggers a note for immediate playback through the #H2Core::Sampler
	 * (ADR 0030). The note carries its own instrument pointer, velocity,
	 * key/octave, and optional SelectedLayerInfo (component/layer
	 * selection). In editor mode this is forwarded over IPC to the
	 * authoritative engine; in standalone it calls Sampler::noteOn()
	 * directly.
	 *
	 * @return true on success. */
	virtual bool noteOn( std::shared_ptr<Note> pNote );
	/** Mutes/unmutes the song's playback-track instrument (and its sole
	 * component/layer), which is not part of the drumkit instrument list. */
	virtual bool setPlaybackTrackMuted(
		bool bMuted,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Sets the volume of the song's playback-track instrument. */
	virtual bool setPlaybackTrackVolume(
		float fVolume,
		Event::Trigger trigger = Event::Trigger::Default
	);

	virtual bool setStripIsMuted(
		int nStrip,
		bool isMuted,
		bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool toggleStripIsMuted(
		int nStrip,
		Event::Trigger trigger = Event::Trigger::Default
	);

	virtual bool setStripIsSoloed(
		int nStrip,
		bool isSoloed,
		bool bSelectStrip,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool toggleStripIsSoloed(
		int nStrip,
		Event::Trigger trigger = Event::Trigger::Default
	);

	virtual bool initExternalControlInterfaces();

	/** Restarts the OSC server of this instance's engine (re-reading the
	 * OSC configuration from Preferences). Single write surface (ADR 0027):
	 * in editor mode the IpcCoreActionController override forwards the
	 * command to the authoritative engine — the mirror holds no server, so
	 * its local base call is a no-op. */
	virtual bool recreateOscServer();

	// -----------------------------------------------------------
	// Actions required for session management.

	/**
	 * Opens the #H2Core::Song specified in @a songPath.
	 *
	 * This will be done immediately and without saving
	 * the current #H2Core::Song. All unsaved changes will be lost!
	 *
	 * \param songPath Absolute path to the .h2song file to be
	 *    opened.
	 * \param sRecoverSongPath If set to a value other than "",
	 *    the corresponding path will be used to load the song and
	 *    the latter is assigned @a songPath as Song::m_sFileName
	 *    afterwards. Using this mechanism the GUI can use an
	 *    autosave backup file to load a song without the core
	 *    having to do some string magic to retrieve the original name.
	 * \return nullptr on failure
	 */
	virtual std::shared_ptr<Song>
	loadSong( const QString& sSongPath, const QString& sRecoverSongPath = "" );
	/**
	 * Sets a #H2Core::Song to be used by Hydrogen.
	 *
	 * This will be done immediately and without saving the
	 * current #H2Core::Song. All unsaved changes will be lost!
	 *
	 * \param pSong Pointer to the #H2Core::Song to set.
	 * \return true on success
	 */
	virtual bool setSong(
		std::shared_ptr<Song> pSong,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * Saves the current #H2Core::Song.
	 *
	 * @param bKeepMissingSamples Whether layers containing a missing sample
	 *   should be kept or discarded.
	 *
	 * \return true on success
	 */
	virtual bool saveSong( bool bKeepMissingSamples );
	/**
	 * Saves the current #H2Core::Song to the path provided in @a sNewFileName.
	 *
	 * The intended use of this function for session
	 * management. Therefore, the function will *not* store the
	 * provided @a sNewFileName in
	 * #H2Core::Preferences::m_lastSongFileName and Hydrogen won't
	 * resume with the corresponding song on restarting.
	 *
	 * \param sNewFileName Absolute path to the file to store the
	 *   current #H2Core::Song in.
	 * @param bKeepMissingSamples Whether layers containing a missing sample
	 *   should be kept or discarded.
	 * \return true on success
	 */
	/** How #saveSongAs treats the path it saves to. */
	enum class PathPolicy {
		/** The saved file becomes the song's backing path (regular
		 * save-as). */
		Adopt,
		/** A copy is written while the current backing path is kept
		 * (NSM export-from-session). */
		Keep
	};
	virtual bool
	saveSongAs( const QString& sNewFileName, bool bKeepMissingSamples,
				PathPolicy policy = PathPolicy::Adopt );
	/**
	 * Loads an instance of #H2Core::Preferences from the corresponding XML
	 * file. */
	virtual std::shared_ptr<Preferences> loadPreferences( const QString& sPath );
	/**
	 * Replaces the current #H2Core::Preferences singleton with the provided
	 * instance. */
	virtual bool setPreferences( std::shared_ptr<Preferences> pPreferences );
	/**
	 * Replaces the MIDI action map held by the current
	 * #H2Core::Preferences.
	 *
	 * The map is GUI-authoritative: the editor's MIDI action table
	 * funnels every change through this command so the authoritative
	 * engine applies the same map (ADR 0030). Unlike setPreferences()
	 * this is a granular install — no driver restarts, no
	 * UpdatePreferences event; the engine's MIDI dispatch reads the map
	 * live on every incoming event. */
	virtual bool setMidiEventMap( std::shared_ptr<MidiEventMap> pMidiEventMap );
	/**
	 * Replaces the MIDI instrument map held by the current
	 * #H2Core::Preferences.
	 *
	 * The map is GUI-authoritative: the editor's MIDI control dialog
	 * funnels every change through this command so the authoritative
	 * engine applies the same map (ADR 0030). Like setMidiEventMap()
	 * this is a granular install — no driver restarts, no
	 * UpdatePreferences event; the engine's MIDI I/O and Sampler read
	 * the map live. */
	virtual bool setMidiInstrumentMap(
		std::shared_ptr<MidiInstrumentMap> pMidiInstrumentMap );
	/**
	 * Sets the scalar MIDI control settings of the current
	 * #H2Core::Preferences: note-off handling, the action dispatch
	 * channel, MIDI feedback, transport message handling/sending, the
	 * feedback channel and note-off sending.
	 *
	 * Granular install — no driver restarts, no UpdatePreferences
	 * event: the engine's MIDI I/O reads all of them live
	 * (MidiInput/MidiOutput/Sampler/AudioEngine). In the editor split
	 * the MIDI control dialog funnels every change through this
	 * command so the authoritative engine applies the same settings
	 * (ADR 0030). */
	virtual bool setMidiControlSettings(
		bool bNoteOffIgnore,
		Midi::Channel actionChannel,
		bool bEnableFeedback,
		bool bTransportInputHandling,
		bool bTransportOutputSend,
		Midi::Channel feedbackChannel,
		Preferences::MidiSendNoteOff sendNoteOff );
	/**
	 * Sets whether rubberband batch recalculation is active in the
	 * current #H2Core::Preferences.
	 *
	 * Granular install — no driver restarts, no UpdatePreferences
	 * event: the engine's transport and drumkit read the flag live
	 * (rubberband recalculation on tempo changes). In the editor split
	 * the GUI toggle must cross it to the authoritative engine
	 * (ADR 0030). */
	virtual bool setRubberBandBatchMode( int nMode );
	/**
	 * Sets the punch-in/out area markers the engine's recording
	 * decision reads.
	 *
	 * The pair is installed as one command so the engine never sees a
	 * half-updated area; an out position below the in position (the
	 * ruler's unset uses `(0, -1)`) clears the area — every position
	 * records. Runtime-only state — not part of the serialized
	 * preferences (ADR 0030). */
	virtual bool setPunchArea( int nPunchInPos, int nPunchOutPos );
	/**
	 * Sets the last MIDI event registered by the engine's MIDI input
	 * — the learning channel the MIDI sense widget polls to bind
	 * incoming events.
	 *
	 * The type and parameter form one logical value (the MIDI input
	 * writes both for every event), so they are set together. The
	 * editor resets the channel before listening; in the editor split
	 * the reset must cross to the authoritative engine, whose MIDI
	 * input is the only writer afterwards (ADR 0030). */
	virtual bool setLastMidiEvent( const MidiEvent::Type& type,
								   Midi::Parameter parameter );
	/**
	 * Adds @a sDirPath to the list of custom sound library dirs held
	 * by the current #H2Core::Preferences and rescans the sound
	 * library database.
	 *
	 * The dirs are engine-consumed state: the database scans them on
	 * every update (Filesystem::listContent in Context::Custom). The
	 * editor's sound library tree funnels every add through this
	 * command so the authoritative engine's preferences and database
	 * see the dir (ADR 0030). Idempotent: an already-registered dir
	 * is a no-op returning true.
	 *
	 * \return true on success */
	virtual bool addCustomSoundLibraryDir( const QString& sDirPath );
	/**
	 * Removes @a sDirPath from the list of custom sound library dirs
	 * held by the current #H2Core::Preferences and rescans the sound
	 * library database.
	 *
	 * Counterpart to addCustomSoundLibraryDir(): the editor's sound
	 * library tree funnels every remove through this command (ADR
	 * 0030). Idempotent: a dir that is not registered is a no-op
	 * returning true.
	 *
	 * \return true on success */
	virtual bool removeCustomSoundLibraryDir( const QString& sDirPath );
	/**
	 * Exports the current song in one shot: opens an export session
	 * with the given render settings, renders one file per entry in
	 * @a renders (minus the entry's excluded instruments, ADR 0027),
	 * and restores the previous drivers, transport state, rubberband
	 * batch mode flag, and interpolation override afterwards.
	 *
	 * The render pipeline only runs in the authoritative engine — the
	 * editor mirror's process loop skips rendering by design — so in
	 * the editor split the ExportSongDialog funnels the whole plan
	 * through this command (ADR 0030). The plan runs on a background
	 * thread in the engine; this call returns once the plan is armed.
	 * An empty plan is an acknowledged no-op.
	 *
	 * \return true on success */
	virtual bool exportSong(
		int nSampleRate, int nSampleDepth, double fCompressionLevel,
		Interpolation::InterpolateMode interpolateMode,
		bool bRubberbandBatchMode,
		const std::vector<ExportRender>& renders );
	/** Ends a running export session — also safe without one: cancels
	 * the remaining plan, aborts a running render, and restores the
	 * previous audio/MIDI drivers and transport state. */
	virtual void stopExportSession();
	/**
	 * Saves the current state of the #H2Core::Preferences. */
	virtual bool savePreferences();
	/**
	 * Triggers the shutdown of Hydrogen.
	 *
	 * This will be done immediately and without saving the
	 * current #H2Core::Song. All unsaved changes will be lost!
	 *
	 * The shutdown will be triggered in both the CLI and the GUI
	 * via the #H2Core::Event::Type::Quit event.
	 *
	 * \return true on success
	 */
	virtual bool quit();

	/** Stops all playback and sends an all-notes-off to the MIDI driver.
	 *
	 * This is the "panic" action triggered by the Panic shortcut. It stops
	 * the sequencer, stops all playing notes in the sampler, and sends an
	 * all-notes-off message via the MIDI driver (ADR 0027/0030 — routed
	 * through CAC so editor mode can marshal it over IPC).
	 *
	 * \return true on success */
	virtual bool panic();

	// -----------------------------------------------------------
	// Further OSC commands

	/**
	 * (De)activates the usage of the Timeline.
	 *
	 * Note that this function will fail in the presence of both JACK audio
	 * driver and an external Timebase controller (see
	 * Hydrogen::getJackTimebaseState()).
	 *
	 * @param bActivate If true - activate or if false -
	 * deactivate.
	 *
	 * @return bool true on success
	 */
	virtual bool activateTimeline( bool bActivate );
	virtual bool toggleTimeline();
	/**
	 * Adds a tempo marker to the Timeline.
	 *
	 * @param nPosition Location of the tempo marker in bars.
	 * @param fBpm Speed associated with the tempo marker.
	 *
	 * @return bool true on success
	 */
	virtual bool addTempoMarker(
		int nPosition,
		float fBpm,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * Delete a tempo marker from the Timeline.
	 *
	 * If no Tempo marker is present at @a nPosition, the function
	 * will return true as well.
	 *
	 * @param nPosition Location of the tempo marker in bars.
	 *
	 * @return bool true on success
	 */
	virtual bool deleteTempoMarker(
		int nPosition,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * Adds a tag to the Timeline.
	 *
	 * @param nPosition Location of the tag in bars.
	 * @param sText Message associated with the tag.
	 *
	 * @return bool true on success
	 */
	virtual bool addTag(
		int nPosition,
		const QString& sText,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * Delete a tag from the Timeline.
	 *
	 * If no tag is present at @a nPosition, the function
	 * will return true as well.
	 *
	 * @param nPosition Location of the tag in bars.
	 *
	 * @return bool true on success
	 */
	virtual bool deleteTag(
		int nPosition,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * (De)activates the usage of Jack transport.
	 *
	 * Note that this function will fail if Jack is not used as
	 * audio driver.
	 *
	 * @param bActivate If true - activate or if false -
	 * deactivate.
	 *
	 * @return bool true on success
	 */
	virtual bool activateJackTransport( bool bActivate );
	virtual bool toggleJackTransport();
	/**
	 * (Un)registers Hydrogen as JACK Timebase constroller.
	 *
	 * Note that this function will fail if JACK is not used as audio
	 * driver.
	 *
	 * @param bActivate If true - activate or if false -
	 * deactivate.
	 *
	 * @return bool true on success
	 */
	virtual bool activateJackTimebaseControl( bool bActivate );
	virtual bool toggleJackTimebaseControl();

	/**
	 * Switches between Song and Pattern mode of playback.
	 *
	 * @param bActivate If true - activates Song mode or if false -
	 * activates Pattern mode.
	 *
	 * @return bool true on success
	 */
	virtual bool activateSongMode( bool bActivate );
	virtual bool toggleSongMode();
	/**
	 * (De)activates loop mode of playback.
	 *
	 * @param bActivate If true - activates loop mode.
	 *
	 * @return bool true on success
	 */
	virtual bool activateLoopMode( bool bActivate );
	virtual bool toggleLoopMode();
	virtual bool activateRecordMode( bool bActivate );
	virtual bool toggleRecordMode();

	/**
	 * Sets Drumkit @a pDrumkit as the one used in the current #Song.
	 *
	 * The loading will overwrite the #InstrumentList of the current
	 * #Song with the one found in @a pDrumkit (among other things)
	 * and also can be used to reset the parameters of the current
	 * drumkit to its default values.
	 *
	 * \param pDrumkit Full-fledged #H2Core::Drumkit to load.
	 */
	virtual bool setDrumkit(
		std::shared_ptr<Drumkit> pDrumkit,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/**
	 * Upgrades the drumkit found at absolute path @a sDrumkitDirOrXml.
	 *
	 * If @a sNewDir is missing, the drumkit will be upgraded in
	 * place and a backup file will be created in order to not
	 * overwrite the existing state.
	 */
	virtual bool upgradeDrumkit(
		const QString& sDrumkitDirOrXml,
		const QString& sNewDir = ""
	);

	/**
	 * Checks whether the provided drumkit in @a sDrumkitDirOrXml can be found,
	 * can be loaded, and does comply with the current XSD definition.
	 *
	 * @param sDrumkitDirOrXml Can be either an absolute path to a folder
	 *   containing a drumkit file (drumkit.xml), an absolute path to a
	 *   drumkit file itself, or an absolute file to a compressed
	 *   drumkit (.h2drumkit).
	 * @param bCheckLegacyVersions Whether just the current XSD
	 *   definition or also all previous versions should be checked.
	 */
	virtual bool validateDrumkit(
		const QString& sDrumkitDirOrXml,
		bool bCheckLegacyVersions = false
	);
	/**
	 * Extracts the compressed .h2drumkit file in @a sDrumkitBundledPath into @a
	 * sTargetDir.
	 *
	 * The function does not automatically load the extracted kit into
	 * the current Hydrogen session in case a custom @a sTargetDir was
	 * supplied. To do so, the name of the folder contained in the
	 * tarball is required (might differ from the name of the tarball)
	 * and it is not easily obtained.
	 *
	 * \param sDrumkitBundledPath Tar-compressed drumkit with .h2drumkit
	 * extension \param sTargetDir Folder to extract the drumkit to. If the
	 * folder is not present yet, it will be created. If left empty, the drumkit
	 * will be installed to the users drumkit data folder. \param pInstalledDir
	 * Will contain the actual name of the folder the kit was installed to. In
	 * most cases this will coincide with a folder within
	 *   @a sTargetPath named like the kit itself. But in case the system does
	 *   not support UTF-8 encoding and @a sTargetPath contains characters other
	 *   than those whitelisted in #Filesystem::removeUtf8Characters, those
	 *   might be omitted and the directory and files created using `libarchive`
	 *   might differ.
	 * \param pEncodingIssuesDetected will be set to `true` in case at least one
	 *   filepath of extracted kit had to be altered in order to not run into
	 *   UTF-8 issues.
	 */
	virtual bool extractDrumkit(
		const QString& sDrumkitBundledPath,
		const QString& sTargetDir = "",
		QString* pInstalledDir = nullptr,
		bool* pEncodingIssuesDetected = nullptr
	);

	/** Adds @a pInstrument to the current drumkit.
	 *
	 * In case @a nIndex is `-1` @a pInstrument will be appended to the
	 * instrument list.*/
	virtual bool addInstrument(
		std::shared_ptr<Instrument> pInstrument,
		int nIndex,
		long nEventId,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Removes @a pInstrument from the current drumkit and adds it to the
	 * instrument death row. This way it is guarantueed that its samples
	 * stay loaded until the last #H2Core::Note is done rendering it.
	 * Afterwards, its samples will be unloaded. */
	virtual bool removeInstrument(
		std::shared_ptr<Instrument> pInstrument,
		long nEventId,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Replaces @a pOldInstrument by @a pNewInstrument in the current
	 * drumkit without clearing notes, changing the selected instrument
	 * number, etc. */
	virtual bool replaceInstrument(
		std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool replaceDrumkitInstrument(
		std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool replacePlaybackTrackInstrument(
		std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Moves instrument @a nSourceIndex of the instrument list of the
	 * current drumkit to index @a nTargetIndex.
	 *
	 * Note that both @a nSourceIndex and @a nTargetIndex are the position
	 * within the instrument list and _not_ the ID of the instrument (which
	 * stays the same during the move action). */
	virtual bool moveInstrument(
		int nSourceIndex,
		int nTargetIndex,
		Event::Trigger trigger = Event::Trigger::Default
	);

	virtual bool renameComponent(
		int nInstrumentIdx,
		int nComponentId,
		const QString& sNewName,
		Event::Trigger trigger = Event::Trigger::Default
	);

	/** Relocates transport to the beginning of a particular
	 * column/Pattern group.
	 *
	 * @param nPatternGroup Position of the Song provided as the
	 * index of a particular pattern group (starting at zero).
	 *
	 * @return bool true on success
	 */
	virtual bool locateToColumn( int nPatternGroup );
	/** Relocates transport to a particular tick.
	 *
	 * @param nTick Destination
	 * \param bWithJackBroadcast Relocate not using the AudioEngine
	 * directly but using the JACK server.
	 *
	 * @return bool true on success
	 */
	virtual bool locateToTick( long nTick, bool bWithJackBroadcast = true );

	/** Relocates transport to an absolute frame. Unlike #locateToTick this issues
	 * no JACK broadcast and no MIDI SongPos feedback: it is the read-only
	 * "follow" relocation used by the editor mirror to track the host engine's
	 * playhead (ADR 0026/0031), never a user-initiated seek.
	 *
	 * @param nFrame Destination frame.
	 * @return bool true on success
	 */
	virtual bool relocateToFrame( long long nFrame );

	/** Creates an empty pattern and adds it to the pattern list.
	 *
	 * @param sPath Name for the created pattern.
	 *
	 * @return bool true on success
	 */
	virtual bool newPattern( const QString& sPatternName );
	/**
	 * Loads an instance of #H2Core::Pattern from the corresponding XML
	 * file. */
	virtual std::shared_ptr<Pattern> loadPattern( const QString& sPath );
	/** Opens a pattern to the current pattern list.
	 *
	 * @param pPattern pattern to be added.
	 * @param nPatternNumber Row the pattern will be added to.
	 * @param bReplace Whether the pattern at @a nPatternNumber should be
		 replaced or moved to the next higher number (including all
		 following patterns).
	 *
	 * @return bool true on success
	 */
	virtual bool setPattern(
		std::shared_ptr<Pattern> pPattern,
		int nPatternNumber,
		bool bReplace,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Selects a pattern from the current pattern list while taking into
	 * account whether the pattern editor is currently locked.
	 *
	 * @param nPatternNumber Row the pattern will be added to.
	 *
	 * @return bool true on success
	 */
	virtual bool selectPattern( int nPatternNumber );
	/** Marks a pattern to be played next once the current one finishes (stacked
	 * mode). Wraps `Hydrogen::toggleNextPattern`. */
	virtual bool toggleNextPattern( int nPatternNumber );
	/** Reorders the pattern list, moving the pattern at @a nSourcePattern to
	 * @a nTargetPattern (shifting the patterns in between). Owns the
	 * `AudioEngine` lock; updates the selection and fires `PatternChanged` so the
	 * editors refresh (ADR 0027). */
	virtual bool movePattern(
		int nSourcePattern,
		int nTargetPattern,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Removes a pattern from the pattern list.
	 *
	 * @param nPatternNumber Specifies the position/row of the pattern.
	 *
	 * @return bool true on success
	 */
	virtual bool removePattern(
		int nPatternNumber,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Deletes all notes for instrument @a pInstrument in a specified
	 * pattern.
	 *
	 * @param nInstrumentNumber target instrument
	 * @param nPatternNumber index of the target pattern in
	 *   Song::m_pPatternList in the current song. If set to -1, the
	 *   currently selected pattern will be used instead.
	 *
	 * @return bool true on success. */
	virtual bool
	clearInstrumentInPattern( int nInstrumentNumber, int nPatternNumber = -1 );
	virtual bool setPatternProperties(
		const QString& sNewPatternPath,
		const int nNewVersion,
		const QString& sNewPatternName,
		const QString& sNewAuthor,
		const QString& sNewPatternInfo,
		const H2Core::License& newLicense,
		const QStringList& newTags,
		int nPatternIndex,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Sets the length and denominator of a pattern (real-time-sensitive:
	 * holds the #H2Core::AudioEngine lock and refreshes the song size).
	 *
	 * @param nLength New pattern length in ticks.
	 * @param nDenominator New pattern denominator.
	 * @param nPatternNumber Position/row of the target pattern.
	 * @return true on success */
	virtual bool setPatternSize(
		int nLength,
		int nDenominator,
		int nPatternNumber,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Replaces the set of virtual patterns of one pattern with the ones
	 * referenced by @a virtualPatternNames (whole-set replacement, not a
	 * merge). The flattened virtual patterns are recomputed and the
	 * playing patterns refreshed. Virtual relationships are song
	 * structure, so the song — not the pattern files — is marked dirty.
	 *
	 * The command is validated atomically: an out-of-range pattern
	 * number, an unknown name, or a self-reference is refused without
	 * touching the current set.
	 *
	 * @param nPatternNumber Position/row of the target pattern.
	 * @param virtualPatternNames Names of the new virtual pattern set.
	 * @return true on success */
	virtual bool setVirtualPatterns(
		int nPatternNumber,
		const QStringList& virtualPatternNames,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Edits a single property of one note, addressed by value identity
	 * (pattern slot + position + instrument id/type + key/octave) so it is
	 * split-safe. Real-time-sensitive: owns the #H2Core::AudioEngine lock
	 * (ADR 0027). Instrument ids, key and octave are passed as their underlying
	 * integers to keep this header light.
	 *
	 * @return true if a value actually changed. */
	virtual bool editNoteProperty(
		NoteProperty property,
		int nPatternNumber,
		int nPosition,
		int nOldInstrumentId,
		int nNewInstrumentId,
		const QString& sOldType,
		const QString& sNewType,
		float fVelocity,
		float fPan,
		float fLeadLag,
		float fProbability,
		int nLength,
		int nNewKey,
		int nOldKey,
		int nNewOctave,
		int nOldOctave,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Adds or removes a single note in a pattern (the engine half of the
	 * PatternEditor add/remove undo action). The note is addressed by value
	 * (position + instrument id/type + key/octave; on delete further
	 * disambiguated by length/velocity/pan/lead-lag/probability/note-off when
	 * several match). Owns the #H2Core::AudioEngine lock (ADR 0027);
	 * selection/cursor/view stay GUI-side.
	 *
	 * @param bIsDelete remove the matching note(s) when true, else insert a new
	 *   note built from the provided properties.
	 * @param bIsMappedToDrumkit whether @a nInstrumentId resolves to a kit
	 *   instrument (else the note stays unmapped/typed).
	 * @return true on success. */
	virtual bool addOrRemoveNote(
		int nPosition,
		int nInstrumentId,
		const QString& sType,
		int nPatternNumber,
		int nOldLength,
		float fOldVelocity,
		float fOldPan,
		float fOldLeadLag,
		int nOldKey,
		int nOldOctave,
		float fOldProbability,
		bool bIsDelete,
		bool bIsNoteOff,
		bool bIsMappedToDrumkit,
		Uuid* pNewNoteUUid,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Removes a specific note @a noteUuid within pattern @n patternUuid */
	virtual bool removeNote(
		Uuid noteUuid,
		Uuid patternUuid,
		Event::Trigger trigger = Event::Trigger::Default
	);

	virtual bool setSongProperties(
		const int nNewVersion,
		const QString& sNewName,
		const QString& sNewAuthor,
		const QString& sNewNotes,
		const H2Core::License& newLicense,
		const QStringList& newTags,
		Event::Trigger trigger = Event::Trigger::Default
	);

	/** Fills or clears a specific grid cell in the SongEditor.
	 *
	 * @param gridPoint position on the #SongEditor grid.
	 *
	 * @return bool true on success
	 */
	virtual bool toggleGridCell(
		const GridPoint& gridPoint,
		Event::Trigger trigger = Event::Trigger::Default
	);

	/** Handle an incoming note event, e.g. a MIDI or OSC Note-On or
	 * Note-Off as well as virtual keyboard stroke.
	 *
	 * @param note determines which note will be triggered.
	 * @param channel specifies the channel on which a matching instrument
	 *   is searched for. `H2Core::MidiMessage::nChannelOff` result in the
	 *   note being dropped and `H2Core::MidiMessage::nChannelAll` for the
	 *   mapping to only match the @a nNote information.
	 * @param fVelocity how "hard" the note was triggered.
	 * @param bNoteOff whether note should trigger or stop sound.
	 * @param pMappedInstrument if provided, will hold the names of all
	 *   instruments the note was mapped to.
	 *
	 * @return bool true on success */
	virtual bool handleNote(
		Midi::Note note,
		Midi::Channel channel,
		float fVelocity,
		bool bNoteOff = false,
		QStringList* pMappedInstruments = nullptr
	);

	/** Tells the #H2Core::Sampler to drop all notes of a specific instrument by
	 * making their #H2Core::Adsr advance into the release state.
	 *
	 * @param instrumentUuid since this method will also be used across the IPC
	 *   engine - editor split, the particular instrument will be indicated by
	 *   its #H2Core::Uuid. */
	virtual bool releasePlayingNotes( Uuid instrumentUuid );

	/**
	 * Loads the drumkit specified in @a sDrumkitDirOrXml.
	 *
	 * Methods from within Hydrogen should _never_ call this function
	 * directly but, instead, use
	 * #SoundLibrarydatabase::getDrumkit(). It is only exposed
	 * publicly to be used within the unit tests.
	 *
	 * \param sDrumkitDirOrXml Can be either an absolute path to a folder
	 *   containing a drumkit file (drumkit.xml), an absolute path to a
	 *   drumkit file itself, or an absolute file to a compressed
	 *   drumkit (.h2drumkit).
	 * \param bIsCompressed Stores whether the drumkit was provided as
	 *   a compressed .h2drumkit file
	 * \param sDrumkitDir Stores the folder containing the drumkit
	 *   file. If a compressed drumkit was provided, this will point to
	 *   a temporary folder.
	 * \param sTemporaryFolder Root path of a temporary folder
	 *   containing the extracted drumkit in case @a sDrumkitDirOrXml
	 *   pointed to a compressed .h2drumkit file.
	 * \param pLegacyFormatEncountered will be set to `true` is any of the
	 *   XML elements requires legacy format support and left untouched
	 *   otherwise.
	 */
	virtual std::shared_ptr<Drumkit> retrieveDrumkit(
		const QString& sDrumkitDirOrXml,
		bool* bIsCompressed,
		QString* sDrumkitDir,
		QString* sTemporaryFolder,
		bool* pLegacyFormatEncountered
	);

	/**
	 * Set's song-level tempo of the #AudioEngine and stores the value
	 * in the current #Song.
	 */
	virtual bool setBpm(
		float fBpm,
		Event::Trigger trigger = Event::Trigger::Default
	);

	/** Makes the metronome count for the length of the largest pattern in
	 * the current row (song mode)/largest active pattern (pattern mode)
	 * before starting playback. */
	virtual bool startCountIn();

	/**
	 * Opens the #H2Core::Playlist specified in @a sPath.
	 *
	 * This will be done immediately and without saving
	 * the current #H2Core::Playlist. All unsaved changes will be lost!
	 *
	 * \param sPath Absolute path to the .h2playlist file to be
	 *    opened.
	 * \param sRecoverPath If set to a value other than "",
	 *    the corresponding path will be used to load the playlist and
	 *    the latter is assigned @a sPath as Playlist::m_sFileName
	 *    afterwards. Using this mechanism the GUI can use an
	 *    autosave backup file to load a playlist without the core
	 *    having to do some string magic to retrieve the original name.
	 * \return nullptr on failure
	 */
	virtual std::shared_ptr<Playlist>
	loadPlaylist( const QString& sPath, const QString& sRecoverPath = "" );
	/** Replaces the current #Playlist with @a Playlist. */
	virtual bool setPlaylist( std::shared_ptr<Playlist> pPlaylist );
	/** Saves changes of the current #Playlist to disk. */
	virtual bool savePlaylist();
	/** Saves the current #Playlist to @a sPath.*/
	virtual bool savePlaylistAs( const QString& sPath );
	/** Adds a new song/ entry to the current playlist.
	 *
	 * If @a nIndex is set to a value of -1, @a pEntry will be appended at
	 * the end of the playlist. */
	virtual bool
	addToPlaylist( std::shared_ptr<PlaylistEntry> pEntry, int nIndex = -1 );
	/** Removes a song from the current playlist.
	 *
	 * If @a nIndex is set to a value of -1, the first occurrance of @a
	 * pEntry will be deleted. */
	virtual bool removeFromPlaylist(
		std::shared_ptr<PlaylistEntry> pEntry,
		int nIndex = -1
	);
	/** Does not load the corresponding song! Only marks it active in the
	 * playlist.
	 *
	 * Song loading was split off to allow the GUI to show error dialogs in
	 * case something went wrong. */
	virtual bool activatePlaylistSong( int nSongNumber );

	/** Enable or disable tempo control using MIDI clock. */
	virtual bool setMidiClockInputHandling( bool bHandle );

	/** Enable or disable sending MIDI clock messages. */
	virtual bool setMidiClockOutputSend( bool bHandle );

	/** Clear the MIDI driver's handled-input activity log (ADR 0029 — the GUI
	 * "bin" button routes through here instead of touching the driver). */
	virtual bool clearMidiInputLog();
	/** Clear the MIDI driver's handled-output activity log (ADR 0029). */
	virtual bool clearMidiOutputLog();

	virtual bool addAutomationPoint(
		float fX,
		float fY,
		Event::Trigger trigger = Event::Trigger::Default
	);
	virtual bool removeAutomationPoint(
		float fX,
		Event::Trigger trigger = Event::Trigger::Default
	);
	/** Move the automation point at (fOldX, fOldY) to (fNewX, fNewY).
	 *
	 * Refused (returning false) when no point sits at the exact source
	 * coordinates, when its value differs from fOldY (a stale command),
	 * or when another point already occupies fNewX — moving would
	 * otherwise silently drop it (AutomationPath::move() erases before
	 * inserting and std::map::insert does not overwrite). */
	virtual bool moveAutomationPoint(
		float fOldX,
		float fOldY,
		float fNewX,
		float fNewY,
		Event::Trigger trigger = Event::Trigger::Default
	);

protected:
	/** \return The owning Hydrogen instance — in editor mode the local
	 * mirror the IPC override applies state to (batch 2t). */
	Hydrogen* getHydrogen() const {
		return m_pHydrogen;
	}

   private:
	/** Back-pointer to the owning Hydrogen instance (ADR 0015). */
	Hydrogen* m_pHydrogen;

	/** Resolve an instrument by its position in the current drumkit, logging on
	 * failure. Shared by the per-parameter instrument setters. */
	std::shared_ptr<Instrument> resolveInstrument( int nInstrument ) const;
	/** Resolve a component by index within an instrument, logging on failure. */
	std::shared_ptr<InstrumentComponent> resolveComponent(
		int nInstrument, int nComponent ) const;
	/** Resolve a layer by index within a component, logging on failure. */
	std::shared_ptr<InstrumentLayer> resolveLayer(
		int nInstrument, int nComponent, int nLayer ) const;

	bool sendMasterVolumeFeedback();
	bool sendStripVolumeFeedback( int nStrip );
	bool sendMetronomeIsActiveFeedback();
	bool sendMasterIsMutedFeedback();
	bool sendStripIsMutedFeedback( int nStrip );
	bool sendStripIsSoloedFeedback( int nStrip );
	bool sendStripPanFeedback( int nStrip );
	bool sendStripPanSymFeedback( int nStrip );

	bool handleOutgoingControlChanges(
		const std::vector<Midi::Parameter>& params,
		Midi::Parameter nValue
	);

	// -----------------------------------------------------------
	// Actions required for session management.

protected:
	/**
	 * Add @a sFileName to the list of recent songs in
	 * Preferences::m_recentFiles.
	 *
	 * The function will also take care of removing any duplicates in
	 * the list in case @a sFileName is already present.
	 *
	 * \param sFileName New song to be added on top of the list.
	 */
	void insertRecentFile( const QString& sFileName );
};

}  // namespace H2Core
#endif
