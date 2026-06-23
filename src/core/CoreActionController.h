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
#include <core/Midi/Midi.h>
#include <core/Object.h>

namespace H2Core {
class Drumkit;
class GridPoint;
class Hydrogen;
class Instrument;
class InstrumentComponent;
class InstrumentLayer;
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
	bool setMasterVolume( float masterVolumeValue );
	/**
	 * \param nStrip Instrument which to set the volume for.
	 * \param fVolumeValue New volume.
	 * \param bSelectStrip Whether the corresponding instrument
	 * should be selected.
	 */
	bool
	setStripVolume( int nStrip, float fVolumeValue, bool bSelectStrip );
	/**
	 * \param nStrip Instrument which to set the pan for.
	 * \param fValue New pan.
	 * \param bSelectStrip Whether the corresponding instrument
	 * should be selected.
	 */
	bool setStripPan( int nStrip, float fValue, bool bSelectStrip );
	/**
	 * \param nStrip Instrument which to set the pan for.
	 * \param fValue New pan. range in [-1;1] => symmetric respect to 0
	 * \param bSelectStrip Whether the corresponding instrument
	 * should be selected.
	 */
	bool setStripPanSym( int nStrip, float fValue, bool bSelectStrip );
	bool setInstrumentPitch( int nInstrument, float fValue );
	bool setInstrumentGain( int nInstrument, float fValue );
	bool setInstrumentRandomPitch( int nInstrument, float fValue );
	bool setInstrumentFilterCutoff( int nInstrument, float fValue );
	bool setInstrumentFilterResonance( int nInstrument, float fValue );
	bool setInstrumentAttack( int nInstrument, float fValue );
	bool setInstrumentDecay( int nInstrument, float fValue );
	bool setInstrumentSustain( int nInstrument, float fValue );
	bool setInstrumentRelease( int nInstrument, float fValue );
	bool setInstrumentFilterActive( int nInstrument, bool bActive );
	bool setInstrumentMuteGroup( int nInstrument, int nMuteGroup );
	bool setInstrumentStopNotes( int nInstrument, bool bStopNotes );
	bool setInstrumentApplyVelocity( int nInstrument, bool bApplyVelocity );
	bool setInstrumentHihatGroup( int nInstrument, int nHihatGroup );
	bool setInstrumentLowerCc( int nInstrument, int nCc );
	bool setInstrumentHigherCc( int nInstrument, int nCc );
	bool setComponentIsMuted( int nInstrument, int nComponent, bool bIsMuted );
	bool setComponentIsSoloed( int nInstrument, int nComponent, bool bIsSoloed );
	bool setComponentGain( int nInstrument, int nComponent, float fGain );
	/** \param nSelection underlying value of #H2Core::InstrumentComponent::Selection. */
	bool setComponentSelection( int nInstrument, int nComponent, int nSelection );
	bool setLayerIsMuted( int nInstrument, int nComponent, int nLayer,
						  bool bIsMuted );
	bool setLayerIsSoloed( int nInstrument, int nComponent, int nLayer,
						   bool bIsSoloed );
	bool setLayerGain( int nInstrument, int nComponent, int nLayer, float fGain );
	bool setLayerPitchOffset( int nInstrument, int nComponent, int nLayer,
							  float fPitchOffset );
	bool setLayerStartVelocity( int nInstrument, int nComponent, int nLayer,
								float fVelocity );
	bool setLayerEndVelocity( int nInstrument, int nComponent, int nLayer,
							  float fVelocity );
	bool setInstrumentMidiOutNote(
		int nInstrument,
		Midi::Note note,
		long* pEventId
	);
	bool setInstrumentMidiOutChannel(
		int nInstrument,
		Midi::Channel channel,
		long* pEventId
	);
	bool setMetronomeIsActive( bool isActive );
	bool setMasterIsMuted( bool isMuted );
	bool setHumanizeTime( float fValue );
	bool setHumanizeVelocity( float fValue );
	bool setSwing( float fValue );
	/** Sets the song-global pan law (MixerSettingsDialog). @a nPanLawType is a
	 * #H2Core::Sampler pan-law constant. */
	bool setPanLaw( int nPanLawType, float fPanLawKNorm );
	/** Auditions an instrument through the #H2Core::Sampler (the mixer-strip
	 * "play sample" / "stop sample" preview buttons). Builds a transient note
	 * for the instrument at @a nInstrument and triggers it; @a bStop sends a
	 * note-off (stop) instead of a full-velocity note-on.
	 *
	 * @return true on success. */
	bool previewInstrument( int nInstrument, bool bStop );
	/** Mutes/unmutes the song's playback-track instrument (and its sole
	 * component/layer), which is not part of the drumkit instrument list. */
	bool setPlaybackTrackMuted( bool bMuted );
	/** Sets the volume of the song's playback-track instrument. */
	bool setPlaybackTrackVolume( float fVolume );

	bool setStripIsMuted( int nStrip, bool isMuted, bool bSelectStrip );
	bool toggleStripIsMuted( int nStrip );

	bool
	setStripIsSoloed( int nStrip, bool isSoloed, bool bSelectStrip );
	bool toggleStripIsSoloed( int nStrip );

	bool initExternalControlInterfaces();

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
	std::shared_ptr<Song>
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
	bool setSong( std::shared_ptr<Song> pSong );
	/**
	 * Saves the current #H2Core::Song.
	 *
	 * @param bKeepMissingSamples Whether layers containing a missing sample
	 *   should be kept or discarded.
	 *
	 * \return true on success
	 */
	bool saveSong( bool bKeepMissingSamples );
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
	bool
	saveSongAs( const QString& sNewFileName, bool bKeepMissingSamples );
	/**
	 * Loads an instance of #H2Core::Preferences from the corresponding XML
	 * file. */
	std::shared_ptr<Preferences> loadPreferences( const QString& sPath );
	/**
	 * Replaces the current #H2Core::Preferences singleton with the provided
	 * instance. */
	bool setPreferences( std::shared_ptr<Preferences> pPreferences );
	/**
	 * Saves the current state of the #H2Core::Preferences. */
	bool savePreferences();
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
	bool quit();

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
	bool activateTimeline( bool bActivate );
	bool toggleTimeline();
	/**
	 * Adds a tempo marker to the Timeline.
	 *
	 * @param nPosition Location of the tempo marker in bars.
	 * @param fBpm Speed associated with the tempo marker.
	 *
	 * @return bool true on success
	 */
	bool addTempoMarker( int nPosition, float fBpm );
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
	bool deleteTempoMarker( int nPosition );
	/**
	 * Adds a tag to the Timeline.
	 *
	 * @param nPosition Location of the tag in bars.
	 * @param sText Message associated with the tag.
	 *
	 * @return bool true on success
	 */
	bool addTag( int nPosition, const QString& sText );
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
	bool deleteTag( int nPosition );
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
	bool activateJackTransport( bool bActivate );
	bool toggleJackTransport();
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
	bool activateJackTimebaseControl( bool bActivate );
	bool toggleJackTimebaseControl();

	/**
	 * Switches between Song and Pattern mode of playback.
	 *
	 * @param bActivate If true - activates Song mode or if false -
	 * activates Pattern mode.
	 *
	 * @return bool true on success
	 */
	bool activateSongMode( bool bActivate );
	bool toggleSongMode();
	/**
	 * (De)activates loop mode of playback.
	 *
	 * @param bActivate If true - activates loop mode.
	 *
	 * @return bool true on success
	 */
	bool activateLoopMode( bool bActivate );
	bool toggleLoopMode();
	bool activateRecordMode( bool bActivate );
	bool toggleRecordMode();

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
	bool setDrumkit( std::shared_ptr<Drumkit> pDrumkit );
	/**
	 * Upgrades the drumkit found at absolute path @a sDrumkitDirOrXml.
	 *
	 * If @a sNewDir is missing, the drumkit will be upgraded in
	 * place and a backup file will be created in order to not
	 * overwrite the existing state.
	 */
	bool upgradeDrumkit(
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
	bool validateDrumkit(
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
	bool extractDrumkit(
		const QString& sDrumkitBundledPath,
		const QString& sTargetDir = "",
		QString* pInstalledDir = nullptr,
		bool* pEncodingIssuesDetected = nullptr
	);

	/** Adds @a pInstrument to the current drumkit.
	 *
	 * In case @a nIndex is `-1` @a pInstrument will be appended to the
	 * instrument list.*/
	bool addInstrument(
		std::shared_ptr<Instrument> pInstrument,
		int nIndex,
		long* pEventId
	);
	/** Removes @a pInstrument from the current drumkit and adds it to the
	 * instrument death row. This way it is guarantueed that its samples
	 * stay loaded until the last #H2Core::Note is done rendering it.
	 * Afterwards, its samples will be unloaded. */
	bool
	removeInstrument( std::shared_ptr<Instrument> pInstrument, long* pEventId );
	/** Replaces @a pOldInstrument by @a pNewInstrument in the current
	 * drumkit without clearing notes, changing the selected instrument
	 * number, etc. */
	bool replaceInstrument(
		std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument
	);
	bool replaceDrumkitInstrument(
		std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument
	);
	bool replacePlaybackTrackInstrument(
		std::shared_ptr<Instrument> pNewInstrument,
		std::shared_ptr<Instrument> pOldInstrument
	);
	/** Moves instrument @a nSourceIndex of the instrument list of the
	 * current drumkit to index @a nTargetIndex.
	 *
	 * Note that both @a nSourceIndex and @a nTargetIndex are the position
	 * within the instrument list and _not_ the ID of the instrument (which
	 * stays the same during the move action). */
	bool moveInstrument( int nSourceIndex, int nTargetIndex );

	bool renameComponent(
		int nInstrumentIdx,
		int nComponentId,
		const QString& sNewName
	);

	/** Relocates transport to the beginning of a particular
	 * column/Pattern group.
	 *
	 * @param nPatternGroup Position of the Song provided as the
	 * index of a particular pattern group (starting at zero).
	 *
	 * @return bool true on success
	 */
	bool locateToColumn( int nPatternGroup );
	/** Relocates transport to a particular tick.
	 *
	 * @param nTick Destination
	 * \param bWithJackBroadcast Relocate not using the AudioEngine
	 * directly but using the JACK server.
	 *
	 * @return bool true on success
	 */
	bool locateToTick( long nTick, bool bWithJackBroadcast = true );

	/** Creates an empty pattern and adds it to the pattern list.
	 *
	 * @param sPath Name for the created pattern.
	 *
	 * @return bool true on success
	 */
	bool newPattern( const QString& sPatternName );
	/**
	 * Loads an instance of #H2Core::Pattern from the corresponding XML
	 * file. */
	std::shared_ptr<Pattern> loadPattern( const QString& sPath );
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
	bool setPattern(
		std::shared_ptr<Pattern> pPattern,
		int nPatternNumber,
		bool bReplace
	);
	/** Selects a pattern from the current pattern list while taking into
	 * account whether the pattern editor is currently locked.
	 *
	 * @param nPatternNumber Row the pattern will be added to.
	 *
	 * @return bool true on success
	 */
	bool selectPattern( int nPatternNumber );
	/** Removes a pattern from the pattern list.
	 *
	 * @param nPatternNumber Specifies the position/row of the pattern.
	 *
	 * @return bool true on success
	 */
	bool removePattern( int nPatternNumber );
	/** Deletes all notes for instrument @a pInstrument in a specified
	 * pattern.
	 *
	 * @param nInstrumentNumber target instrument
	 * @param nPatternNumber index of the target pattern in
	 *   Song::m_pPatternList in the current song. If set to -1, the
	 *   currently selected pattern will be used instead.
	 *
	 * @return bool true on success. */
	bool
	clearInstrumentInPattern( int nInstrumentNumber, int nPatternNumber = -1 );
	bool setPatternProperties(
		const QString& sNewPatternPath,
		const int nNewVersion,
		const QString& sNewPatternName,
		const QString& sNewAuthor,
		const QString& sNewPatternInfo,
		const H2Core::License& newLicense,
		const QStringList& newTags,
		int nPatternIndex
	);
	/** Sets the length and denominator of a pattern (real-time-sensitive:
	 * holds the #H2Core::AudioEngine lock and refreshes the song size).
	 *
	 * @param nLength New pattern length in ticks.
	 * @param nDenominator New pattern denominator.
	 * @param nPatternNumber Position/row of the target pattern.
	 * @return true on success */
	bool setPatternSize( int nLength, int nDenominator, int nPatternNumber );
	/** Edits a single property of one note, addressed by value identity
	 * (pattern slot + position + instrument id/type + key/octave) so it is
	 * split-safe. Real-time-sensitive: owns the #H2Core::AudioEngine lock
	 * (ADR 0027). Instrument ids, key and octave are passed as their underlying
	 * integers to keep this header light.
	 *
	 * @return true if a value actually changed. */
	bool editNoteProperty(
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
		int nOldOctave
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
	bool addOrRemoveNote(
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
		Uuid* pNewNoteUUid
	);
	/** Removes a specific note @a noteUuid within pattern @n patternUuid */
	bool removeNote( Uuid noteUuid, Uuid patternUuid );

	bool setSongProperties(
		const QString& sNewPath,
		const int nNewVersion,
		const QString& sNewName,
		const QString& sNewAuthor,
		const QString& sNewNotes,
		const H2Core::License& newLicense,
		const QStringList& newTags
	);

	/** Fills or clears a specific grid cell in the SongEditor.
	 *
	 * @param gridPoint position on the #SongEditor grid.
	 *
	 * @return bool true on success
	 */
	bool toggleGridCell( const GridPoint& gridPoint );

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
	bool handleNote(
		Midi::Note note,
		Midi::Channel channel,
		float fVelocity,
		bool bNoteOff = false,
		QStringList* pMappedInstruments = nullptr
	);

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
	std::shared_ptr<Drumkit> retrieveDrumkit(
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
	bool setBpm( float fBpm );

	/** Makes the metronome count for the length of the largest pattern in
	 * the current row (song mode)/largest active pattern (pattern mode)
	 * before starting playback. */
	bool startCountIn();

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
	std::shared_ptr<Playlist>
	loadPlaylist( const QString& sPath, const QString& sRecoverPath = "" );
	/** Replaces the current #Playlist with @a Playlist. */
	bool setPlaylist( std::shared_ptr<Playlist> pPlaylist );
	/** Saves changes of the current #Playlist to disk. */
	bool savePlaylist();
	/** Saves the current #Playlist to @a sPath.*/
	bool savePlaylistAs( const QString& sPath );
	/** Adds a new song/ entry to the current playlist.
	 *
	 * If @a nIndex is set to a value of -1, @a pEntry will be appended at
	 * the end of the playlist. */
	bool
	addToPlaylist( std::shared_ptr<PlaylistEntry> pEntry, int nIndex = -1 );
	/** Removes a song from the current playlist.
	 *
	 * If @a nIndex is set to a value of -1, the first occurrance of @a
	 * pEntry will be deleted. */
	bool removeFromPlaylist(
		std::shared_ptr<PlaylistEntry> pEntry,
		int nIndex = -1
	);
	/** Does not load the corresponding song! Only marks it active in the
	 * playlist.
	 *
	 * Song loading was split off to allow the GUI to show error dialogs in
	 * case something went wrong. */
	bool activatePlaylistSong( int nSongNumber );

	/** Enable or disable tempo control using MIDI clock. */
	bool setMidiClockInputHandling( bool bHandle );

	/** Enable or disable sending MIDI clock messages. */
	bool setMidiClockOutputSend( bool bHandle );

	/** Clear the MIDI driver's handled-input activity log (ADR 0029 — the GUI
	 * "bin" button routes through here instead of touching the driver). */
	bool clearMidiInputLog();
	/** Clear the MIDI driver's handled-output activity log (ADR 0029). */
	bool clearMidiOutputLog();

	bool addAutomationPoint( float fX, float fY );
	bool removeAutomationPoint( float fX );

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
