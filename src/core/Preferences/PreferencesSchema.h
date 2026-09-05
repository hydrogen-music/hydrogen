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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#ifndef H2C_PREFERENCES_SCHEMA_H
#define H2C_PREFERENCES_SCHEMA_H

#include <core/Preferences/Preferences.h>
#include <core/Helpers/Xml.h>

#include <optional>
#include <tuple>

namespace H2Core {

class Hydrogen;

/** \brief Single source of truth for the Preferences XML format.
 *
 * One table describes every field of #PreferencesData that is (de)serialized in
 * the user config file: where the field lives in the document, which layer and
 * owner it belongs to, and how to read and write it. All former hand-written
 * serialization routines of Preferences are thin loops over this table, so key
 * names, fallbacks, and quirks exist exactly once (ADR 0023 amendment).
 *
 * The table is ordered to reproduce the element order of the legacy
 * hand-written saveTo(), keeping saved files byte-compatible.
 *
 * Enforcement chain: static_asserts in PreferencesSchema.cpp keep the row count
 * identical to the number of members tied by #tieAllMembers and to the actual
 * table size, and a structured-binding canary over PreferencesData breaks the
 * build the moment a member is added or removed without extending the tie -
 * the tie alone would stay silent about an unmentioned member. When adding a
 * serialized member, extend the canary, #tieAllMembers, and the table in the
 * same change - the asserts then break the build if any third is missing, and
 * the round-trip unit test reports the exact row that lost data.
 *
 * Statics-only class: never instantiated; H2_OBJECT provides the class name for
 * log messages emitted by the codecs.
 *
 * \ingroup docCore docConfiguration */
class PreferencesSchema : public H2Core::Object<PreferencesSchema>
{
	H2_OBJECT( PreferencesSchema )

public:

	/** Which configuration layer a field belongs to (ADR 0022). */
	enum class Layer {
		/** Read from and written to the user-level config file by any Hydrogen
		 * instance. */
		Base,
		/** Host- or state-owned field (ADR 0022): a plugin-host instance never
		 * writes it back to the user config. */
		Override
	};

	/** Which part of the application owns a field. */
	enum class Owner {
		/** Owned by the headless core engine: part of the IPC core-properties
		 * fragment exchanged with the GUI mirror. */
		Core,
		/** Owned by the GUI: never part of the IPC core-properties fragment. */
		Gui
	};

	/** Circumstances under which the schema rows are evaluated while
	 *  reading. */
	struct ReadContext {
		/** Hydrogen instance MIDI events and shortcuts are registered against
		 *  (may be nullptr). */
		Hydrogen* pHydrogen = nullptr;
		/** Whether debug and info messages should be logged when anomalies are
		 *  encountered while reading the XML nodes. */
		bool bSilent = false;
		/** Whether the path_to_rubberband row applies its special handling (the
		 *  Preferences constructor did not find the executable in common
		 *  places). */
		bool bSearchForRubberband = false;
		/** Whether missing structural nodes are reported via WARNINGLOG (user
		 *  config files) or silently skipped (trusted IPC fragments). */
		bool bEmitStructuralWarnings = true;
		/** Pre-2.0 "fixed_mapping" flag (midi_driver): MIDI input was mapped to
		 *  the current output instrument. */
		bool bLegacyFixedMapping = false;
		/** Pre-2.0 "discard_note_after_action" flag (midi_driver): incoming
		 *  MIDI messages only triggered actions, no realtime notes. */
		bool bLegacyDiscardNoteAfterAction = false;
		/** Pre-2.0 "instrumentInputMode" flag (root): MIDI input was mapped to
		 *  the selected instrument. */
		bool bLegacyPlaySelectedInstrument = false;
		/** Whether the midiInstrumentMap row derives the pre-2.0 mapping state
		 *  from the legacy flags above when its element is missing (load of old
		 *  config files) or keeps the current value (IPC fragments). */
		bool bApplyLegacyMidiInput = false;
	};

	/** Circumstances under which the schema rows are evaluated while
	 *  writing. */
	struct WriteContext {
		/** Whether debug and info messages should be suppressed while writing
		 *  the XML nodes. */
		bool bSilent = false;
	};

	/** Description of a single serialized field. */
	struct FieldRow {
		/** Signature of the codec writing one field (or subtree) into its
		 *  parent element. */
		using WriteFn = void (*)( XMLNode& parent, const FieldRow& row,
								  const PreferencesData& data,
								  const WriteContext& context );
		/** Signature of the codec reading one field (or subtree) from its
		 *  parent element. */
		using ReadFn = void (*)( const XMLNode& parent, const FieldRow& row,
								 PreferencesData& data,
								 const ReadContext& context );

		/** Parent element names from the document root, nullptr-terminated. A
		 *  nullptr first entry addresses the root element itself. */
		const char* path[ 3 ];
		/** Element name of the field: a leaf for scalar rows or the subtree
		 *  root for composite rows. */
		const char* key;
		/** Configuration layer the field belongs to (ADR 0022). */
		Layer layer;
		/** Part of the application owning the field. */
		Owner owner;
		/** Passed as inexistent_ok to the XMLNode read helpers. */
		bool bInexistentOk;
		/** Passed as empty_ok to the XMLNode read helpers. */
		bool bEmptyOk;
		/** How checkForUnknownElements() matches the element named key:
		 *  exactly, as a name prefix (formatted container keys like the pattern
		 *  colors), or not at all (opaque subtree whose internals are validated
		 *  by its own loader). */
		enum class KeyMatch {
			Exact,
			Prefix,
			OpaqueSubtree
		};
		/** Matching mode of #key for the unknown-element check. */
		KeyMatch keyMatch;
		/** Writes the field into parent, creating the element named key. */
		WriteFn write;
		/** Reads the element named key from parent into data. */
		ReadFn read;
	};

	/** The schema table. Definition (and row count) in
	 *  PreferencesSchema.cpp. */
	static const FieldRow kSchemaRows[];
	/** Number of rows in #kSchemaRows. A static_assert in PreferencesSchema.cpp
	 *  keeps this identical to the number of members tied by #tieAllMembers. */
	static const int kSchemaRowCount;

	/** Ties every member of PreferencesData — the Theme sub-objects decomposed
	 *  into their serialized members — into a single tuple, one element per
	 *  schema row, in #kSchemaRows order.
	 *
	 * The order is load-bearing for diagnostics only: element \c i corresponds
	 * to row \c i, which the round-trip unit test uses to report the exact row
	 * that lost data. The count, in contrast, is enforced at compile time.
	 *
	 * \param data Requires data.m_pTheme and its sub-objects to be non-null
	 * (guaranteed by the PreferencesData defaults). */
	static auto tieAllMembers( PreferencesData& data ) {
		return std::tie(
			// ── root ──
			data.m_sPreferredLanguage, data.m_nMaxBars,
			data.m_pTheme->m_pInterface->m_layout,
			data.m_pTheme->m_pInterface->m_uiScalingPolicy,
			data.m_nLastOpenTab, data.m_bUseTheRubberbandBpmChangeEvent,
			data.m_bUseRelativeFileNamesForPlaylists,
			data.m_bHideKeyboardCursor, data.m_bShowDevelWarning,
			data.m_bShowNoteOverwriteWarning, data.m_bHearNewNotes,
			data.m_bQuantizeEvents, data.m_sRubberBandCLIexecutable,
			data.m_recentFiles, data.m_onlineRepos,
			// ── audio_engine ──
			data.m_audioDriver, data.m_bUseMetronome,
			data.m_fMetronomeVolume, data.m_nMaxNotes,
			data.m_interpolateMode, data.m_nBufferSize,
			data.m_nSampleRate, data.m_bCountIn,
			// ── audio_engine/oss_driver ──
			data.m_sOSSDevice,
			// ── audio_engine/portaudio_driver ──
			data.m_sPortAudioDevice, data.m_sPortAudioHostAPI,
			data.m_nLatencyTarget,
			// ── audio_engine/coreaudio_driver ──
			data.m_sCoreAudioDevice,
			// ── audio_engine/jack_driver ──
			data.m_sJackPortName1, data.m_sJackPortName2,
			data.m_nJackTransportMode, data.m_bJackTimebaseEnabled,
			data.m_bJackTimebaseMode, data.m_bJackConnectDefaults,
			data.m_JackTrackOutputMode, data.m_bJackTrackOuts,
			data.m_bJackEnforceInstrumentName,
			// ── audio_engine/alsa_audio_driver ──
			data.m_sAlsaAudioDevice,
			// ── audio_engine/midi_driver ──
			data.m_midiDriver, data.m_sMidiPortName,
			data.m_sMidiOutputPortName, data.m_midiActionChannel,
			data.m_bMidiNoteOffIgnore, data.m_bEnableMidiFeedback,
			data.m_midiFeedbackChannel, data.m_bMidiClockInputHandling,
			data.m_bMidiTransportInputHandling,
			data.m_bMidiClockOutputSend,
			data.m_bMidiTransportOutputSend, data.m_midiSendNoteOff,
			// ── audio_engine/osc_configuration ──
			data.m_nOscServerPort, data.m_bOscServerEnabled,
			data.m_bOscFeedbackEnabled,
			// ── gui ──
			data.m_pTheme->m_pInterface->m_sQTStyle,
			data.m_pTheme->m_pFont->m_sApplicationFontFamily,
			data.m_pTheme->m_pFont->m_sLevel2FontFamily,
			data.m_pTheme->m_pFont->m_sLevel3FontFamily,
			data.m_pTheme->m_pFont->m_fontSize,
			data.m_pTheme->m_pInterface->m_fMixerFalloffSpeed,
			data.m_nPatternEditorGridResolution,
			data.m_nPatternEditorGridHeight,
			data.m_nPatternEditorGridWidth,
			data.m_bPatternEditorUsingTriplets,
			data.m_bPatternEditorAlwaysShowTypeLabels,
			data.m_nSongEditorGridHeight, data.m_nSongEditorGridWidth,
			data.m_bShowInstrumentPeaks, data.m_bShowAutomationArea,
			data.m_bShowPlaybackTrack,
			data.m_mainFormProperties, data.m_mixerProperties,
			data.m_patternEditorProperties, data.m_songEditorProperties,
			data.m_rackProperties, data.m_audioEngineInfoProperties,
			data.m_playlistEditorProperties, data.m_directorProperties,
			data.m_sLastExportPatternAsDirectory,
			data.m_sLastExportSongDirectory,
			data.m_sLastSaveSongAsDirectory,
			data.m_sLastOpenSongDirectory,
			data.m_sLastOpenPatternDirectory,
			data.m_sLastExportLilypondDirectory,
			data.m_sLastExportMidiDirectory,
			data.m_sLastImportDrumkitDirectory,
			data.m_sLastExportDrumkitDirectory,
			data.m_sLastSaveDrumkitAsDirectory,
			data.m_sLastOpenLayerDirectory,
			data.m_sLastOpenPlaybackTrackDirectory,
			data.m_sLastAddSongToPlaylistDirectory,
			data.m_sLastPlaylistDirectory,
			data.m_sLastPlaylistScriptDirectory,
			data.m_sLastImportThemeDirectory,
			data.m_sLastExportThemeDirectory,
			data.m_nExportModeIdx, data.m_exportFormat,
			data.m_fExportCompressionLevel, data.m_nExportSampleRateIdx,
			data.m_nExportSampleDepthIdx,
			data.m_bShowExportSongLicenseWarning,
			data.m_bShowExportDrumkitLicenseWarning,
			data.m_bShowExportDrumkitCopyleftWarning,
			data.m_bShowExportDrumkitAttributionWarning,
			data.m_bFollowPlayhead, data.m_nMidiExportMode,
			data.m_bMidiExportUseHumanization,
			data.m_bSoundLibraryShowName, data.m_bSoundLibraryShowAuthor,
			data.m_bSoundLibraryShowInfo, data.m_bSoundLibraryShowLicense,
			data.m_bSoundLibraryShowPath, data.m_bSoundLibraryShowTags,
			data.m_bSoundLibraryShowVersion, data.m_nSoundLibraryLastTab,
			data.m_nRackLastTab, data.m_bpmTap, data.m_beatCounter,
			data.m_nBeatCounterDriftCompensation,
			data.m_nBeatCounterStartOffset,
			data.m_bPlaySamplesOnClicking, data.m_nAutosavesPerHour,
			data.m_pTheme->m_pColor,
			data.m_pTheme->m_pInterface->m_coloringMethod,
			data.m_pTheme->m_pInterface->m_patternColors,
			data.m_pTheme->m_pInterface->m_nVisiblePatternColors,
			data.m_pTheme->m_pInterface->m_iconColor,
			data.m_pTheme->m_pInterface->m_bIndicateNotePlayback,
			data.m_pTheme->m_pInterface->m_bIndicateEffectiveNoteLength,
			// ── files ──
			data.m_sLastSongPath, data.m_sLastPlaylistPath,
			data.m_sDefaultEditor, data.m_customSoundLibraryDirs,
			// ── root subtrees ──
			data.m_pMidiEventMap, data.m_pMidiInstrumentMap,
			data.m_pShortcuts );
	}

	/** Serializes all rows — or only those owned by ownerFilter when given —
	 *  into the document rooted at rootNode, creating parent elements on
	 *  demand. */
	static void writeRows( XMLNode& rootNode, const PreferencesData& data,
						   std::optional<Owner> ownerFilter,
						   const WriteContext& context );

	/** Merges this instance's own changes into the document rooted at rootNode
	 *  (the re-read shared config): every row that is both eligible under
	 *  ownership and changed against baselineRoot replaces its elements in
	 *  rootNode — removed first, then re-written, so container rows replace
	 *  wholesale. A null baselineRoot writes every eligible row (full snapshot
	 *  / self-heal, ADR 0023). baselineRoot is normalized in place
	 *  (whitespace-only text nodes dropped) so a parsed document compares equal
	 *  to the compact writer output. Returns the number of rows written. */
	static int persistRows( XMLNode& rootNode, XMLNode& baselineRoot,
							const PreferencesData& data,
							Preferences::FieldOwnership ownership,
							const WriteContext& context );

	/** Applies all rows — or only those owned by ownerFilter when given — from
	 *  the document rooted at rootNode onto data. Rows whose parent path is
	 *  missing are skipped (reported via WARNINGLOG depending on
	 *  ReadContext). */
	static void readRows( const XMLNode& rootNode, PreferencesData& data,
						  std::optional<Owner> ownerFilter,
						  const ReadContext& context );

	/** ERRORLOGs every element in the document that no schema row covers, so
	 *  config drift (typos, stale or foreign elements) surfaces instead of
	 *  being silently dropped (ADR 0023 amendment). Subtrees owned by composite
	 *  rows are skipped: their internals are validated by their own loaders. */
	static void checkForUnknownElements( const XMLNode& rootNode );
};

}; // namespace H2Core

#endif // H2C_PREFERENCES_SCHEMA_H
