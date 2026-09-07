/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Preferences/PreferencesSchema.h>

#include <core/Helpers/Filesystem.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/PreferencesKeys.h>
#include <core/Preferences/Shortcuts.h>
#include <core/Preferences/Theme.h>
#include <core/Preferences/WindowProperties.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

namespace H2Core {

// The schema table and the drivers below are defined at namespace scope: pull
// the nested table types in so the row initializers and the driver signatures
// can refer to them unqualified. PreferencesSchema is a class, so these must be
// type aliases: using-declarations cannot import class members at namespace
// scope.
using FieldRow = PreferencesSchema::FieldRow;
using Layer = PreferencesSchema::Layer;
using Owner = PreferencesSchema::Owner;
using ReadContext = PreferencesSchema::ReadContext;
using WriteContext = PreferencesSchema::WriteContext;

namespace {

	/** Writes value as the element named key into parent, dispatching on the
	 * static type: enums are stored as their underlying int, unsigned members
	 * through the same int codec the legacy writer used. */
	template <typename T>
	void writeValue( XMLNode& parent, const char* key, const T& value )
	{
		if constexpr ( std::is_enum_v<T> ) {
			parent.write_int( key, static_cast<int>( value ) );
		}
		else if constexpr ( std::is_same_v<T, bool> ) {
			parent.write_bool( key, value );
		}
		else if constexpr ( std::is_same_v<T, float> ) {
			parent.write_float( key, value );
		}
		else if constexpr ( std::is_same_v<T, QString> ) {
			parent.write_string( key, value );
		}
		else {
			static_assert(
				std::is_integral_v<T>, "Unsupported PreferencesData field type"
			);
			parent.write_int( key, static_cast<int>( value ) );
		}
	}

	/** Reads the element named key from parent, falling back to the current
	 * value for missing or empty elements. The row flags map to the
	 * inexistent_ok/empty_ok arguments of the XMLNode read helpers. */
	template <typename T>
	T readValue(
		const XMLNode& parent,
		const FieldRow& row,
		const T& current,
		const ReadContext& context
	)
	{
		if constexpr ( std::is_enum_v<T> ) {
			return static_cast<T>( parent.read_int(
				row.key, static_cast<int>( current ), row.bInexistentOk,
				row.bEmptyOk, context.bSilent
			) );
		}
		else if constexpr ( std::is_same_v<T, bool> ) {
			return parent.read_bool(
				row.key, current, row.bInexistentOk, row.bEmptyOk,
				context.bSilent
			);
		}
		else if constexpr ( std::is_same_v<T, float> ) {
			return parent.read_float(
				row.key, current, row.bInexistentOk, row.bEmptyOk,
				context.bSilent
			);
		}
		else if constexpr ( std::is_same_v<T, QString> ) {
			return parent.read_string(
				row.key, current, row.bInexistentOk, row.bEmptyOk,
				context.bSilent
			);
		}
		else {
			static_assert(
				std::is_integral_v<T>, "Unsupported PreferencesData field type"
			);
			return static_cast<T>( parent.read_int(
				row.key, static_cast<int>( current ), row.bInexistentOk,
				row.bEmptyOk, context.bSilent
			) );
		}
	}

	/** Codec pair for a plain PreferencesData member. */
	template <auto MemberPtr>
	void scalarWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		writeValue( parent, row.key, data.*MemberPtr );
	}

	template <auto MemberPtr>
	void scalarRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		data.*MemberPtr = readValue( parent, row, data.*MemberPtr, context );
	}

	/** Codec pair for a member of one of the Theme sub-objects. SubMemberPtr
	 * selects the sub-object within Theme (m_pInterface or m_pFont),
	 * MemberPtr the serialized member within it. */
	template <auto SubMemberPtr, auto MemberPtr>
	void themeScalarWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		writeValue(
			parent, row.key,
			( *( ( *data.m_pTheme ).*SubMemberPtr ) ).*MemberPtr
		);
	}

	template <auto SubMemberPtr, auto MemberPtr>
	void themeScalarRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		auto& member = ( *( ( *data.m_pTheme ).*SubMemberPtr ) ).*MemberPtr;
		member = readValue( parent, row, member, context );
	}

	/** Codec pair for the WindowProperties members: a missing element keeps the
	 * current value (legacy behavior, no warning). */
	template <auto MemberPtr>
	void windowPropsWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		XMLNode node = parent.createNode( row.key );
		( data.*MemberPtr ).saveTo( node );
	}

	template <auto MemberPtr>
	void windowPropsRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const XMLNode node = parent.firstChildElement( row.key );
		if ( !node.isNull() ) {
			data.*MemberPtr = WindowProperties::loadFrom(
				node, data.*MemberPtr, context.bSilent
			);
		}
	}

	// ---- specialized codecs
	// ------------------------------------------------------
	//
	// One codec pair per field whose legacy (de)serialization is not a plain
	// scalar read/write: string-encoded enums, pre-2.0 channel encodings,
	// containers, and opaque subtrees. They reproduce the legacy behavior
	// verbatim - including its quirks - so the table-driven files stay
	// byte-compatible with the hand-written ones.

	/** path_to_rubberband: the writer only persists an executable path (the
	 * sentinel otherwise); the reader applies the configured path only while
	 * the constructor is still searching for the executable. */
	void rubberbandWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		QString sExecutable( data.m_sRubberBandCLIexecutable );
		if ( !Filesystem::fileExecutable( sExecutable, true /* silent */ ) ) {
			sExecutable = "Path to Rubberband-CLI";
		}
		parent.write_string( row.key, sExecutable );
	}

	void rubberbandRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		if ( !context.bSearchForRubberband ) {
			return;
		}
		// In case the Rubberband CLI executable was not found yet, we check
		// the additional path provided in the config (the Preferences
		// constructor already checked the common places).
		const QString sRubberbandPath = parent.read_string(
			row.key, "", row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		if ( !sRubberbandPath.isEmpty() && QFile( sRubberbandPath ).exists() ) {
			data.m_sRubberBandCLIexecutable = sRubberbandPath;
		}
		else {
			data.m_sRubberBandCLIexecutable = "Path to Rubberband-CLI";
		}
	}

	/** recentUsedSongs: only the five most recent songs are persisted. The
	 * reader replaces the list (load() starts from defaults anyway). */
	void recentSongsWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		XMLNode node = parent.createNode( row.key );
		unsigned nSongs = 5;
		if ( data.m_recentFiles.size() < 5 ) {
			nSongs = data.m_recentFiles.size();
		}
		for ( unsigned ii = 0; ii < nSongs; ii++ ) {
			node.write_string( "song", data.m_recentFiles[ii] );
		}
	}

	void recentSongsRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const XMLNode node = parent.firstChildElement( row.key );
		if ( node.isNull() ) {
			if ( context.bEmitStructuralWarnings ) {
				___WARNINGLOG( "<recentUsedSongs> node not found" );
			}
			return;
		}
		data.m_recentFiles.clear();
		QDomElement songElement = node.firstChildElement( "song" );
		while ( !songElement.isNull() && !songElement.text().isEmpty() ) {
			data.m_recentFiles.push_back( songElement.text() );
			songElement = songElement.nextSiblingElement( "song" );
		}
	}

	/** onlineRepos: entries are merged into the constructor defaults instead
	 * of replacing them (Gui-owned: never part of the IPC fragment). */
	void onlineReposWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		XMLNode node = parent.createNode( row.key );
		for ( const auto& sRepo : data.m_onlineRepos ) {
			node.write_string( "repo", sRepo );
		}
	}

	void onlineReposRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const XMLNode node = parent.firstChildElement( row.key );
		if ( node.isNull() ) {
			return;
		}
		QDomElement repoElement = node.firstChildElement( "repo" );
		while ( !repoElement.isNull() && !repoElement.text().isEmpty() ) {
			if ( !data.m_onlineRepos.contains( repoElement.text() ) ) {
				data.m_onlineRepos.push_back( repoElement.text() );
			}
			repoElement = repoElement.nextSiblingElement( "repo" );
		}
	}

	/** audio_driver: an unparsable name falls back to Auto (warned unless
	 * the fragment is trusted IPC input). */
	void audioDriverWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		parent.write_string(
			row.key, Preferences::audioDriverToQString( data.m_audioDriver )
		);
	}

	void audioDriverRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const QString sAudioDriver = parent.read_string(
			row.key, Preferences::audioDriverToQString( data.m_audioDriver ),
			row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		data.m_audioDriver = Preferences::parseAudioDriver( sAudioDriver );
		if ( data.m_audioDriver == PreferencesData::AudioDriver::None ) {
			if ( context.bEmitStructuralWarnings ) {
				___WARNINGLOG( QString( "Parsing of audio driver [%1] failed. "
										"Falling back to 'Auto'" )
								   .arg( sAudioDriver ) );
			}
			data.m_audioDriver = PreferencesData::AudioDriver::Auto;
		}
	}

	/** jack_transport_mode: two-state int member stored as pre-2.0 string
	 * constants. Unrecognized values keep the current setting (no warning,
	 * as in the legacy reader). */
	void jackTransportModeWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		QString sMode;
		if ( data.m_nJackTransportMode == PreferencesData::NO_JACK_TRANSPORT ) {
			sMode = "NO_JACK_TRANSPORT";
		}
		else if ( data.m_nJackTransportMode ==
				  PreferencesData::USE_JACK_TRANSPORT ) {
			sMode = "USE_JACK_TRANSPORT";
		}
		parent.write_string( row.key, sMode );
	}

	void jackTransportModeRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const QString sMode = parent.read_string(
			row.key, "", row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		if ( sMode == "NO_JACK_TRANSPORT" ) {
			data.m_nJackTransportMode = PreferencesData::NO_JACK_TRANSPORT;
		}
		else if ( sMode == "USE_JACK_TRANSPORT" ) {
			data.m_nJackTransportMode = PreferencesData::USE_JACK_TRANSPORT;
		}
	}

	/** jack_transport_mode_master: same string encoding for the timebase
	 * mode; we stick to the old strings for compatibility with old versions
	 * still in use. */
	void jackTimebaseModeWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		QString tmMode;
		if ( data.m_bJackTimebaseMode ==
			 PreferencesData::NO_JACK_TIMEBASE_CONTROL ) {
			tmMode = "NO_JACK_TIME_MASTER";
		}
		else if ( data.m_bJackTimebaseMode ==
				  PreferencesData::USE_JACK_TIMEBASE_CONTROL ) {
			tmMode = "USE_JACK_TIME_MASTER";
		}
		parent.write_string( row.key, tmMode );
	}

	void jackTimebaseModeRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		// The current value is only overwritten when the element is present
		// and well formatted.
		const QString sJackMasterMode = parent.read_string(
			row.key, "", row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		if ( sJackMasterMode == "NO_JACK_TIME_MASTER" ) {
			data.m_bJackTimebaseMode =
				PreferencesData::NO_JACK_TIMEBASE_CONTROL;
		}
		else if ( sJackMasterMode == "USE_JACK_TIME_MASTER" ) {
			data.m_bJackTimebaseMode =
				PreferencesData::USE_JACK_TIMEBASE_CONTROL;
		}
		else if ( !sJackMasterMode.isEmpty() &&
				  context.bEmitStructuralWarnings ) {
			___WARNINGLOG(
				QString( "Unable to parse <jack_transport_mode_master>: "
						 "[%1]" )
					.arg( sJackMasterMode )
			);
		}
	}

	/** jack_track_output_mode: -255 is the "element absent" sentinel of the
	 * legacy reader. */
	void jackTrackOutputModeWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		int nJackTrackOutputMode = 0;
		if ( data.m_JackTrackOutputMode ==
			 PreferencesData::JackTrackOutputMode::postFader ) {
			nJackTrackOutputMode = 0;
		}
		else if ( data.m_JackTrackOutputMode ==
				  PreferencesData::JackTrackOutputMode::preFader ) {
			nJackTrackOutputMode = 1;
		}
		parent.write_int( row.key, nJackTrackOutputMode );
	}

	void jackTrackOutputModeRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const int nJackTrackOutputMode = parent.read_int(
			row.key, -255, row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		if ( nJackTrackOutputMode == 0 ) {
			data.m_JackTrackOutputMode =
				PreferencesData::JackTrackOutputMode::postFader;
		}
		else if ( nJackTrackOutputMode == 1 ) {
			data.m_JackTrackOutputMode =
				PreferencesData::JackTrackOutputMode::preFader;
		}
		else if ( nJackTrackOutputMode != -255 &&
				  context.bEmitStructuralWarnings ) {
			___WARNINGLOG( QString( "Unable to parse <jack_track_output_mode>: "
									"[%1]" )
							   .arg( nJackTrackOutputMode ) );
		}
	}

	/** driverName: unlike the audio driver there is no fallback - an
	 * unparsable name maps to MidiDriver::None. */
	void midiDriverWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		parent.write_string(
			row.key, Preferences::midiDriverToQString( data.m_midiDriver )
		);
	}

	void midiDriverRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const QString sMidiDriver = parent.read_string(
			row.key, Preferences::midiDriverToQString( data.m_midiDriver ),
			row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		data.m_midiDriver = Preferences::parseMidiDriver( sMidiDriver );
	}

	/** channel_filter: pre-2.0 zero-based channel encoding (-1 = all,
	 * -2 = off) around the 1-based Midi::Channel values. */
	void channelFilterWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		// In versions prior to 2.0 there was an inconsistent scheme for storing
		// MIDI channels. In this variable `-1` did indicate to use "All"
		// channels while the same value set in the MIDI output channel within
		// the instruments of a drumkit meant "Off" or none. Valid channel
		// values were zero-based Starting from 2.0 we unified those ranges
		// allowing this variable, too, to represent both "All" and "Off". In
		// addition, we now use 1 based channel values in accordance with the
		// MIDI standard. But, for backward compatibility, we still write
		// zero-based values to file and use the old one and not those defined
		// in Midi.h.
		int nChannelFilter = static_cast<int>( data.m_midiActionChannel ) - 1;
		if ( data.m_midiActionChannel == Midi::ChannelAll ) {
			// Old value indicating to use all channels.
			nChannelFilter = -1;
		}
		else if ( data.m_midiActionChannel == Midi::ChannelOff ) {
			// Helper value indicating to use no channel (since -1 was already
			// taken). Please note that this value is only handled properly
			// starting with Hydrogen 1.2.7 (where it selects the "All" option
			// too, since the overall MIDI input channel can not be turned off
			// prior to 2.0).
			nChannelFilter = -2;
		}
		parent.write_int( row.key, nChannelFilter );
	}

	void channelFilterRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		// In versions prior to 2.0 there was an inconsistent scheme for storing
		// MIDI channels. In this variable `-1` did indicate to use "All"
		// channels while the same value set in the MIDI output channel within
		// the instruments of a drumkit meant "Off" or none. Valid channel
		// values were zero-based Starting from 2.0 we unified those ranges
		// allowing this variable, too, to represent both "All" and "Off". In
		// addition, we now use 1 based channel values in accordance with the
		// MIDI standard. But, for backward compatibility, we still write
		// zero-based values to file and use the old one and not those defined
		// in Midi.h.
		const int nMidiActionChannel = parent.read_int(
			row.key, /* previous value used to indicate 'all' */ -1,
			row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		if ( nMidiActionChannel == -1 ) {
			data.m_midiActionChannel = Midi::ChannelAll;
		}
		else if ( nMidiActionChannel == -2 ) {
			data.m_midiActionChannel = Midi::ChannelOff;
		}
		else {
			data.m_midiActionChannel =
				Midi::channelFromIntClamp( nMidiActionChannel + 1 );
		}
	}

	/** midi_feedback_channel: zero-based file encoding; the -1 shift maps
	 * ChannelAll/ChannelOff onto -2/-1, which channelFromInt() restores. */
	void midiFeedbackChannelWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
        // The file-based representation of the MIDI channel is zero-based (for
		// historical reasons) while we start with 1 within the application
		// (since version 2.0).
		parent.write_int(
			row.key, static_cast<int>( data.m_midiFeedbackChannel ) - 1
		);
	}

	void midiFeedbackChannelRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		data.m_midiFeedbackChannel = Midi::channelFromInt(
			parent.read_int(
				row.key, static_cast<int>( data.m_midiFeedbackChannel ) - 1,
				row.bInexistentOk, row.bEmptyOk, context.bSilent
			) +
			1
		);
	}

	/** QTStyle: the Plastique style was removed in Qt 5.10. */
	void qtStyleWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		parent.write_string( row.key, data.m_pTheme->m_pInterface->m_sQTStyle );
	}

	void qtStyleRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		QString sQTStyle = parent.read_string(
			row.key, data.m_pTheme->m_pInterface->m_sQTStyle, row.bInexistentOk,
			row.bEmptyOk, context.bSilent
		);
		if ( sQTStyle == "Plastique" ) {
			sQTStyle = "Fusion";
		}
		data.m_pTheme->m_pInterface->m_sQTStyle = sQTStyle;
	}

	/** exportDialogFormat: stored as the file suffix of the audio format. */
	void exportFormatWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		parent.write_string(
			row.key, Filesystem::AudioFormatToSuffix( data.m_exportFormat )
		);
	}

	void exportFormatRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		data.m_exportFormat =
			Filesystem::AudioFormatFromSuffix( parent.read_string(
				row.key, Filesystem::AudioFormatToSuffix( data.m_exportFormat ),
				row.bInexistentOk, row.bEmptyOk, context.bSilent
			) );
	}

	/** bc: pre-2.0 string constants selecting the tempo input widget. */
	void bpmTapWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		QString sBeatCounterOn( "BC_OFF" );
		if ( data.m_bpmTap == PreferencesData::BpmTap::BeatCounter ) {
			sBeatCounterOn = "BC_ON";
		}
		parent.write_string( row.key, sBeatCounterOn );
	}

	void bpmTapRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const QString sUseBeatCounter = parent.read_string(
			row.key, "", row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		if ( sUseBeatCounter == "BC_OFF" ) {
			data.m_bpmTap = PreferencesData::BpmTap::TapTempo;
		}
		else if ( sUseBeatCounter == "BC_ON" ) {
			data.m_bpmTap = PreferencesData::BpmTap::BeatCounter;
		}
		else if ( !sUseBeatCounter.isEmpty() &&
				  context.bEmitStructuralWarnings ) {
			___WARNINGLOG(
				QString( "Unable to parse <bc>: [%1]" ).arg( sUseBeatCounter )
			);
		}
	}

	/** setplay: whether the beat counter also starts playback. */
	void beatCounterWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		QString setPlay( "SET_PLAY_OFF" );
		if ( data.m_beatCounter == PreferencesData::BeatCounter::TapAndPlay ) {
			setPlay = "SET_PLAY_ON";
		}
		parent.write_string( row.key, setPlay );
	}

	void beatCounterRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const QString sBeatCounterSetPlay = parent.read_string(
			row.key, "", row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		if ( sBeatCounterSetPlay == "SET_PLAY_OFF" ) {
			data.m_beatCounter = PreferencesData::BeatCounter::Tap;
		}
		else if ( sBeatCounterSetPlay == "SET_PLAY_ON" ) {
			data.m_beatCounter = PreferencesData::BeatCounter::TapAndPlay;
		}
		else if ( !sBeatCounterSetPlay.isEmpty() &&
				  context.bEmitStructuralWarnings ) {
			___WARNINGLOG( QString( "Unable to parse <setplay>: [%1]" )
							   .arg( sBeatCounterSetPlay ) );
		}
	}

	/** colorTheme: ColorTheme::saveTo() creates the <colorTheme> element
	 * itself; loadFrom() is a factory returning a fresh object. */
	void colorThemeWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		data.m_pTheme->m_pColor->saveTo( parent );
	}

	void colorThemeRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const XMLNode node = parent.firstChildElement( row.key );
		if ( node.isNull() ) {
			if ( context.bEmitStructuralWarnings ) {
				___WARNINGLOG( "<colorTheme> node not found" );
			}
			return;
		}
		data.m_pTheme->m_pColor = ColorTheme::loadFrom( node, context.bSilent );
	}

	/** SongEditor_pattern_color_: 50 formatted keys, one per pattern color
	 * slot. */
	void patternColorsWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		const auto& patternColors =
			data.m_pTheme->m_pInterface->m_patternColors;
		for ( int ii = 0; ii < InterfaceTheme::nMaxPatternColors; ii++ ) {
			parent.write_color(
				QString( row.key ) + QString::number( ii ), patternColors[ii]
			);
		}
	}

	void patternColorsRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		auto& patternColors = data.m_pTheme->m_pInterface->m_patternColors;
		for ( int ii = 0; ii < InterfaceTheme::nMaxPatternColors; ii++ ) {
			patternColors[ii] = parent.read_color(
				QString( row.key ) + QString::number( ii ), patternColors[ii],
				row.bInexistentOk, row.bEmptyOk, context.bSilent
			);
		}
	}

	/** SongEditor_visible_pattern_colors: the legacy reader clamps against
	 * the hardcoded range 0..50. */
	void visiblePatternColorsWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		parent.write_int(
			row.key, data.m_pTheme->m_pInterface->m_nVisiblePatternColors
		);
	}

	void visiblePatternColorsRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		data.m_pTheme->m_pInterface->m_nVisiblePatternColors = std::clamp(
			parent.read_int(
				row.key, data.m_pTheme->m_pInterface->m_nVisiblePatternColors,
				row.bInexistentOk, row.bEmptyOk, context.bSilent
			),
			0, 50
		);
	}

	/** iconColor: every value other than White is folded back to Black. */
	void iconColorWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		parent.write_int(
			row.key,
			static_cast<int>( data.m_pTheme->m_pInterface->m_iconColor )
		);
	}

	void iconColorRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const int nIconColor = parent.read_int(
			row.key,
			static_cast<int>( data.m_pTheme->m_pInterface->m_iconColor ),
			row.bInexistentOk, row.bEmptyOk, context.bSilent
		);
		data.m_pTheme->m_pInterface->m_iconColor =
			nIconColor == static_cast<int>( InterfaceTheme::IconColor::White )
				? InterfaceTheme::IconColor::White
				: InterfaceTheme::IconColor::Black;
	}

	/** customSoundLibraryDirs: empty entries are not written; the reader
	 * replaces the list (load() starts from defaults anyway). */
	void customDirsWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		XMLNode node = parent.createNode( row.key );
		for ( const auto& sDir : data.m_customSoundLibraryDirs ) {
			if ( !sDir.isEmpty() ) {
				node.write_string( "dir", sDir );
			}
		}
	}

	void customDirsRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const XMLNode node = parent.firstChildElement( row.key );
		if ( node.isNull() ) {
			return;
		}
		data.m_customSoundLibraryDirs.clear();
		QDomElement customDirNode = node.firstChildElement( "dir" );
		while ( !customDirNode.isNull() && !customDirNode.text().isEmpty() ) {
			data.m_customSoundLibraryDirs << customDirNode.text();
			customDirNode = customDirNode.nextSiblingElement( "dir" );
		}
	}

	/** midiEventMap: opaque subtree - MidiEventMap::saveTo() creates the
	 * <midiEventMap> element itself and loadFrom() validates its internals. */
	void midiEventMapWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		data.m_pMidiEventMap->saveTo( parent, context.bSilent );
	}

	void midiEventMapRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const XMLNode node = parent.firstChildElement( row.key );
		if ( node.isNull() ) {
			if ( context.bEmitStructuralWarnings ) {
				___WARNINGLOG( "<midiEventMap> node not found" );
			}
			return;
		}
		data.m_pMidiEventMap =
			MidiEventMap::loadFrom( node, context.bSilent, context.pHydrogen );
	}

	/** midiInstrumentMap: opaque subtree. When the element is missing, the
	 * load() of pre-2.0 config files derives the mapping state from the
	 * legacy flags in the ReadContext. */
	void midiInstrumentMapWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		data.m_pMidiInstrumentMap->saveTo( parent );
	}

	void midiInstrumentMapRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		const XMLNode node = parent.firstChildElement( row.key );
		if ( !node.isNull() ) {
			data.m_pMidiInstrumentMap =
				MidiInstrumentMap::loadFrom( node, context.bSilent );
			return;
		}
		if ( !context.bApplyLegacyMidiInput ) {
			// Trusted IPC fragments keep the current mapping.
			return;
		}
		// Backward compatibility: derive the mapping state from the pre-2.0
		// flags read by the load() prologue.
		if ( context.bLegacyFixedMapping ) {
			data.m_pMidiInstrumentMap->setInput(
				MidiInstrumentMap::Input::AsOutput
			);
		}
		else if ( context.bLegacyPlaySelectedInstrument ) {
			data.m_pMidiInstrumentMap->setInput(
				MidiInstrumentMap::Input::SelectedInstrument
			);
		}
		else if ( context.bLegacyDiscardNoteAfterAction ) {
			// Incoming MIDI messages were not mapped to realtime notes.
			data.m_pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::None
			);
		}
		else {
			data.m_pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::Order
			);
		}
		// Prior to version 2.0 a single numerical value in the preferences
		// indicated which channel (or all of them) was used for MIDI input.
		// Since 2.0 this value only affects MIDI actions, but it is used here
		// to set up a global input channel for note mapping as well, to
		// provide as much backward compatibility as possible.
		data.m_pMidiInstrumentMap->setUseGlobalInputChannel( true );
		data.m_pMidiInstrumentMap->setGlobalInputChannel(
			data.m_midiActionChannel
		);
	}

	/** shortcuts: opaque subtree - Shortcuts::saveTo() creates the
	 * <shortcuts> element itself. */
	void shortcutsWrite(
		XMLNode& parent,
		const FieldRow& row,
		const PreferencesData& data,
		const WriteContext& context
	)
	{
		data.m_pShortcuts->saveTo( parent );
	}

	void shortcutsRead(
		const XMLNode& parent,
		const FieldRow& row,
		PreferencesData& data,
		const ReadContext& context
	)
	{
		// Shortcuts::loadFrom() handles a missing <shortcuts> element
		// internally (falling back to the defaults).
		data.m_pShortcuts =
			Shortcuts::loadFrom( parent, context.pHydrogen, context.bSilent );
	}

}  // namespace

// The schema table. Order is load-bearing: it reproduces the element
// order of the legacy hand-written saveTo(), keeping saved files
// byte-compatible (the writeRows() parent cache relies on it).
const PreferencesSchema::FieldRow PreferencesSchema::kSchemaRows[] = {
	// ---- root ----
	{ { nullptr, nullptr, nullptr },
	  "preferredLanguage",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sPreferredLanguage>,
	  &scalarRead<&PreferencesData::m_sPreferredLanguage> },
	{ { nullptr, nullptr, nullptr },
	  "maxBars",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nMaxBars>,
	  &scalarRead<&PreferencesData::m_nMaxBars> },
	{ { nullptr, nullptr, nullptr },
	  "defaultUILayout",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<&Theme::m_pInterface, &InterfaceTheme::m_layout>,
	  &themeScalarRead<&Theme::m_pInterface, &InterfaceTheme::m_layout> },
	{ { nullptr, nullptr, nullptr },
	  "uiScalingPolicy",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_uiScalingPolicy>,
	  &themeScalarRead<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_uiScalingPolicy> },
	{ { nullptr, nullptr, nullptr },
	  "lastOpenTab",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nLastOpenTab>,
	  &scalarRead<&PreferencesData::m_nLastOpenTab> },
	{ { nullptr, nullptr, nullptr },
	  "useTheRubberbandBpmChangeEvent",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bUseTheRubberbandBpmChangeEvent>,
	  &scalarRead<&PreferencesData::m_bUseTheRubberbandBpmChangeEvent> },
	{ { nullptr, nullptr, nullptr },
	  "useRelativeFilenamesForPlaylists",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bUseRelativeFileNamesForPlaylists>,
	  &scalarRead<&PreferencesData::m_bUseRelativeFileNamesForPlaylists> },
	{ { nullptr, nullptr, nullptr },
	  "hideKeyboardCursorWhenUnused",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bHideKeyboardCursor>,
	  &scalarRead<&PreferencesData::m_bHideKeyboardCursor> },
	{ { nullptr, nullptr, nullptr },
	  "showDevelWarning",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowDevelWarning>,
	  &scalarRead<&PreferencesData::m_bShowDevelWarning> },
	{ { nullptr, nullptr, nullptr },
	  "showNoteOverwriteWarning",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowNoteOverwriteWarning>,
	  &scalarRead<&PreferencesData::m_bShowNoteOverwriteWarning> },
	{ { nullptr, nullptr, nullptr },
	  "hearNewNotes",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bHearNewNotes>,
	  &scalarRead<&PreferencesData::m_bHearNewNotes> },
	{ { nullptr, nullptr, nullptr },
	  "quantizeEvents",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bQuantizeEvents>,
	  &scalarRead<&PreferencesData::m_bQuantizeEvents> },
	{ { nullptr, nullptr, nullptr },
	  "path_to_rubberband",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &rubberbandWrite,
	  &rubberbandRead },
	{ { nullptr, nullptr, nullptr },
	  PreferencesKeys::RecentUsedSongs,
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &recentSongsWrite,
	  &recentSongsRead },
	{ { nullptr, nullptr, nullptr },
	  "onlineRepos",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &onlineReposWrite,
	  &onlineReposRead },
	// ---- audio_engine ----
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  PreferencesKeys::AudioDriver,
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &audioDriverWrite,
	  &audioDriverRead },
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  "use_metronome",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bUseMetronome>,
	  &scalarRead<&PreferencesData::m_bUseMetronome> },
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  "metronome_volume",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_fMetronomeVolume>,
	  &scalarRead<&PreferencesData::m_fMetronomeVolume> },
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  "maxNotes",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nMaxNotes>,
	  &scalarRead<&PreferencesData::m_nMaxNotes> },
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  "interpolateMode",
	  Layer::Base,
	  Owner::Core,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_interpolateMode>,
	  &scalarRead<&PreferencesData::m_interpolateMode> },
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  PreferencesKeys::BufferSize,
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nBufferSize>,
	  &scalarRead<&PreferencesData::m_nBufferSize> },
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  PreferencesKeys::SampleRate,
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nSampleRate>,
	  &scalarRead<&PreferencesData::m_nSampleRate> },
	{ { PreferencesKeys::AudioEngine, nullptr, nullptr },
	  "countIn",
	  Layer::Base,
	  Owner::Core,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bCountIn>,
	  &scalarRead<&PreferencesData::m_bCountIn> },
	// ---- audio_engine/oss_driver ----
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::OssDriver, nullptr },
	  "ossDevice",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sOSSDevice>,
	  &scalarRead<&PreferencesData::m_sOSSDevice> },
	// ---- audio_engine/portaudio_driver ----
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::PortAudioDriver, nullptr
	  },
	  "portAudioDevice",
	  Layer::Override,
	  Owner::Core,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sPortAudioDevice>,
	  &scalarRead<&PreferencesData::m_sPortAudioDevice> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::PortAudioDriver, nullptr
	  },
	  "portAudioHostAPI",
	  Layer::Override,
	  Owner::Core,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sPortAudioHostAPI>,
	  &scalarRead<&PreferencesData::m_sPortAudioHostAPI> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::PortAudioDriver, nullptr
	  },
	  "latencyTarget",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nLatencyTarget>,
	  &scalarRead<&PreferencesData::m_nLatencyTarget> },
	// ---- audio_engine/coreaudio_driver ----
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::CoreAudioDriver, nullptr
	  },
	  "coreAudioDevice",
	  Layer::Override,
	  Owner::Core,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sCoreAudioDevice>,
	  &scalarRead<&PreferencesData::m_sCoreAudioDevice> },
	// ---- audio_engine/jack_driver ----
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_port_name_1",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sJackPortName1>,
	  &scalarRead<&PreferencesData::m_sJackPortName1> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_port_name_2",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sJackPortName2>,
	  &scalarRead<&PreferencesData::m_sJackPortName2> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_transport_mode",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &jackTransportModeWrite,
	  &jackTransportModeRead },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_timebase_enabled",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bJackTimebaseEnabled>,
	  &scalarRead<&PreferencesData::m_bJackTimebaseEnabled> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_transport_mode_master",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &jackTimebaseModeWrite,
	  &jackTimebaseModeRead },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_connect_defaults",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bJackConnectDefaults>,
	  &scalarRead<&PreferencesData::m_bJackConnectDefaults> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_track_output_mode",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &jackTrackOutputModeWrite,
	  &jackTrackOutputModeRead },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_track_outs",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bJackTrackOuts>,
	  &scalarRead<&PreferencesData::m_bJackTrackOuts> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::JackDriver, nullptr },
	  "jack_enforce_instrument_name",
	  Layer::Override,
	  Owner::Core,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bJackEnforceInstrumentName>,
	  &scalarRead<&PreferencesData::m_bJackEnforceInstrumentName> },
	// ---- audio_engine/alsa_audio_driver ----
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::AlsaAudioDriver, nullptr
	  },
	  "alsa_audio_device",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sAlsaAudioDevice>,
	  &scalarRead<&PreferencesData::m_sAlsaAudioDevice> },
	// ---- audio_engine/midi_driver ----
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  PreferencesKeys::MidiDriverName,
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &midiDriverWrite,
	  &midiDriverRead },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  PreferencesKeys::MidiPortName,
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sMidiPortName>,
	  &scalarRead<&PreferencesData::m_sMidiPortName> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  PreferencesKeys::MidiOutputPortName,
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sMidiOutputPortName>,
	  &scalarRead<&PreferencesData::m_sMidiOutputPortName> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "channel_filter",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &channelFilterWrite,
	  &channelFilterRead },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "ignore_note_off",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bMidiNoteOffIgnore>,
	  &scalarRead<&PreferencesData::m_bMidiNoteOffIgnore> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "enable_midi_feedback",
	  Layer::Base,
	  Owner::Core,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bEnableMidiFeedback>,
	  &scalarRead<&PreferencesData::m_bEnableMidiFeedback> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "midi_feedback_channel",
	  Layer::Base,
	  Owner::Core,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &midiFeedbackChannelWrite,
	  &midiFeedbackChannelRead },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "midi_clock_input_handling",
	  Layer::Base,
	  Owner::Core,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bMidiClockInputHandling>,
	  &scalarRead<&PreferencesData::m_bMidiClockInputHandling> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "midi_transport_input_handling",
	  Layer::Base,
	  Owner::Core,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bMidiTransportInputHandling>,
	  &scalarRead<&PreferencesData::m_bMidiTransportInputHandling> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "midi_clock_output_send",
	  Layer::Base,
	  Owner::Core,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bMidiClockOutputSend>,
	  &scalarRead<&PreferencesData::m_bMidiClockOutputSend> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "midi_transport_output_send",
	  Layer::Base,
	  Owner::Core,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bMidiTransportOutputSend>,
	  &scalarRead<&PreferencesData::m_bMidiTransportOutputSend> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::MidiDriver, nullptr },
	  "midi_send_note_off",
	  Layer::Base,
	  Owner::Core,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_midiSendNoteOff>,
	  &scalarRead<&PreferencesData::m_midiSendNoteOff> },
	// ---- audio_engine/osc_configuration ----
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::OscConfiguration, nullptr
	  },
	  "oscServerPort",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nOscServerPort>,
	  &scalarRead<&PreferencesData::m_nOscServerPort> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::OscConfiguration, nullptr
	  },
	  "oscEnabled",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bOscServerEnabled>,
	  &scalarRead<&PreferencesData::m_bOscServerEnabled> },
	{ { PreferencesKeys::AudioEngine, PreferencesKeys::OscConfiguration, nullptr
	  },
	  "oscFeedbackEnabled",
	  Layer::Override,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bOscFeedbackEnabled>,
	  &scalarRead<&PreferencesData::m_bOscFeedbackEnabled> },
	// ---- gui ----
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "QTStyle",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &qtStyleWrite,
	  &qtStyleRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "application_font_family",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<&Theme::m_pFont, &FontTheme::m_sApplicationFontFamily>,
	  &themeScalarRead<&Theme::m_pFont, &FontTheme::m_sApplicationFontFamily> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "level2_font_family",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<&Theme::m_pFont, &FontTheme::m_sLevel2FontFamily>,
	  &themeScalarRead<&Theme::m_pFont, &FontTheme::m_sLevel2FontFamily> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "level3_font_family",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<&Theme::m_pFont, &FontTheme::m_sLevel3FontFamily>,
	  &themeScalarRead<&Theme::m_pFont, &FontTheme::m_sLevel3FontFamily> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "font_size",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<&Theme::m_pFont, &FontTheme::m_fontSize>,
	  &themeScalarRead<&Theme::m_pFont, &FontTheme::m_fontSize> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "mixer_falloff_speed",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_fMixerFalloffSpeed>,
	  &themeScalarRead<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_fMixerFalloffSpeed> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "patternEditorGridResolution",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nPatternEditorGridResolution>,
	  &scalarRead<&PreferencesData::m_nPatternEditorGridResolution> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "patternEditorGridHeight",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nPatternEditorGridHeight>,
	  &scalarRead<&PreferencesData::m_nPatternEditorGridHeight> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "patternEditorGridWidth",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nPatternEditorGridWidth>,
	  &scalarRead<&PreferencesData::m_nPatternEditorGridWidth> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "patternEditorUsingTriplets",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bPatternEditorUsingTriplets>,
	  &scalarRead<&PreferencesData::m_bPatternEditorUsingTriplets> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "patternEditorAlwaysShowTypeLabels",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bPatternEditorAlwaysShowTypeLabels>,
	  &scalarRead<&PreferencesData::m_bPatternEditorAlwaysShowTypeLabels> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "songEditorGridHeight",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nSongEditorGridHeight>,
	  &scalarRead<&PreferencesData::m_nSongEditorGridHeight> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "songEditorGridWidth",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nSongEditorGridWidth>,
	  &scalarRead<&PreferencesData::m_nSongEditorGridWidth> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "showInstrumentPeaks",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowInstrumentPeaks>,
	  &scalarRead<&PreferencesData::m_bShowInstrumentPeaks> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "showAutomationArea",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowAutomationArea>,
	  &scalarRead<&PreferencesData::m_bShowAutomationArea> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "showPlaybackTrack",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowPlaybackTrack>,
	  &scalarRead<&PreferencesData::m_bShowPlaybackTrack> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "mainForm_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_mainFormProperties>,
	  &windowPropsRead<&PreferencesData::m_mainFormProperties> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "mixer_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_mixerProperties>,
	  &windowPropsRead<&PreferencesData::m_mixerProperties> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "patternEditor_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_patternEditorProperties>,
	  &windowPropsRead<&PreferencesData::m_patternEditorProperties> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "songEditor_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_songEditorProperties>,
	  &windowPropsRead<&PreferencesData::m_songEditorProperties> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "instrumentRack_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_rackProperties>,
	  &windowPropsRead<&PreferencesData::m_rackProperties> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "audioEngineInfo_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_audioEngineInfoProperties>,
	  &windowPropsRead<&PreferencesData::m_audioEngineInfoProperties> },
	// In order to be backward compatible we still call the XML node
	// "playlistDialog". For some time we had playlistEditor and playlistDialog
	// coexisting.
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "playlistDialog_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_playlistEditorProperties>,
	  &windowPropsRead<&PreferencesData::m_playlistEditorProperties> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "director_properties",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &windowPropsWrite<&PreferencesData::m_directorProperties>,
	  &windowPropsRead<&PreferencesData::m_directorProperties> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastExportPatternAsDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastExportPatternAsDirectory>,
	  &scalarRead<&PreferencesData::m_sLastExportPatternAsDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastExportSongDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastExportSongDirectory>,
	  &scalarRead<&PreferencesData::m_sLastExportSongDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastSaveSongAsDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastSaveSongAsDirectory>,
	  &scalarRead<&PreferencesData::m_sLastSaveSongAsDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastOpenSongDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastOpenSongDirectory>,
	  &scalarRead<&PreferencesData::m_sLastOpenSongDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastOpenPatternDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastOpenPatternDirectory>,
	  &scalarRead<&PreferencesData::m_sLastOpenPatternDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastExportLilypondDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastExportLilypondDirectory>,
	  &scalarRead<&PreferencesData::m_sLastExportLilypondDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastExportMidiDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastExportMidiDirectory>,
	  &scalarRead<&PreferencesData::m_sLastExportMidiDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastImportDrumkitDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastImportDrumkitDirectory>,
	  &scalarRead<&PreferencesData::m_sLastImportDrumkitDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastExportDrumkitDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastExportDrumkitDirectory>,
	  &scalarRead<&PreferencesData::m_sLastExportDrumkitDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastSaveDrumkitAsDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastSaveDrumkitAsDirectory>,
	  &scalarRead<&PreferencesData::m_sLastSaveDrumkitAsDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastOpenLayerDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastOpenLayerDirectory>,
	  &scalarRead<&PreferencesData::m_sLastOpenLayerDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastOpenPlaybackTrackDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastOpenPlaybackTrackDirectory>,
	  &scalarRead<&PreferencesData::m_sLastOpenPlaybackTrackDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastAddSongToPlaylistDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastAddSongToPlaylistDirectory>,
	  &scalarRead<&PreferencesData::m_sLastAddSongToPlaylistDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastPlaylistDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastPlaylistDirectory>,
	  &scalarRead<&PreferencesData::m_sLastPlaylistDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastPlaylistScriptDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastPlaylistScriptDirectory>,
	  &scalarRead<&PreferencesData::m_sLastPlaylistScriptDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastImportThemeDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastImportThemeDirectory>,
	  &scalarRead<&PreferencesData::m_sLastImportThemeDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "lastExportThemeDirectory",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastExportThemeDirectory>,
	  &scalarRead<&PreferencesData::m_sLastExportThemeDirectory> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "exportDialogMode",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nExportModeIdx>,
	  &scalarRead<&PreferencesData::m_nExportModeIdx> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "exportDialogFormat",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &exportFormatWrite,
	  &exportFormatRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "exportDialogCompressionLevel",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_fExportCompressionLevel>,
	  &scalarRead<&PreferencesData::m_fExportCompressionLevel> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "exportDialogSampleRate",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nExportSampleRateIdx>,
	  &scalarRead<&PreferencesData::m_nExportSampleRateIdx> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "exportDialogSampleDepth",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nExportSampleDepthIdx>,
	  &scalarRead<&PreferencesData::m_nExportSampleDepthIdx> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "showExportSongLicenseWarning",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowExportSongLicenseWarning>,
	  &scalarRead<&PreferencesData::m_bShowExportSongLicenseWarning> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "showExportDrumkitLicenseWarning",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowExportDrumkitLicenseWarning>,
	  &scalarRead<&PreferencesData::m_bShowExportDrumkitLicenseWarning> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "showExportDrumkitCopyleftWarning",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowExportDrumkitCopyleftWarning>,
	  &scalarRead<&PreferencesData::m_bShowExportDrumkitCopyleftWarning> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "showExportDrumkitAttributionWarning",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bShowExportDrumkitAttributionWarning>,
	  &scalarRead<&PreferencesData::m_bShowExportDrumkitAttributionWarning> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "followPlayhead",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bFollowPlayhead>,
	  &scalarRead<&PreferencesData::m_bFollowPlayhead> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "midiExportDialogMode",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nMidiExportMode>,
	  &scalarRead<&PreferencesData::m_nMidiExportMode> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "midiExportDialogUseHumanization",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bMidiExportUseHumanization>,
	  &scalarRead<&PreferencesData::m_bMidiExportUseHumanization> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryShowName",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bSoundLibraryShowName>,
	  &scalarRead<&PreferencesData::m_bSoundLibraryShowName> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryShowAuthor",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bSoundLibraryShowAuthor>,
	  &scalarRead<&PreferencesData::m_bSoundLibraryShowAuthor> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryShowInfo",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bSoundLibraryShowInfo>,
	  &scalarRead<&PreferencesData::m_bSoundLibraryShowInfo> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryShowLicense",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bSoundLibraryShowLicense>,
	  &scalarRead<&PreferencesData::m_bSoundLibraryShowLicense> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryShowPath",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bSoundLibraryShowPath>,
	  &scalarRead<&PreferencesData::m_bSoundLibraryShowPath> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryShowTags",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bSoundLibraryShowTags>,
	  &scalarRead<&PreferencesData::m_bSoundLibraryShowTags> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryShowVersion",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bSoundLibraryShowVersion>,
	  &scalarRead<&PreferencesData::m_bSoundLibraryShowVersion> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "soundLibraryLastTab",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nSoundLibraryLastTab>,
	  &scalarRead<&PreferencesData::m_nSoundLibraryLastTab> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "rackLastTab",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nRackLastTab>,
	  &scalarRead<&PreferencesData::m_nRackLastTab> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "bc",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &bpmTapWrite,
	  &bpmTapRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "setplay",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &beatCounterWrite,
	  &beatCounterRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "countoffset",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nBeatCounterDriftCompensation>,
	  &scalarRead<&PreferencesData::m_nBeatCounterDriftCompensation> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "playoffset",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nBeatCounterStartOffset>,
	  &scalarRead<&PreferencesData::m_nBeatCounterStartOffset> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "playSamplesOnClicking",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_bPlaySamplesOnClicking>,
	  &scalarRead<&PreferencesData::m_bPlaySamplesOnClicking> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "autosavesPerHour",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_nAutosavesPerHour>,
	  &scalarRead<&PreferencesData::m_nAutosavesPerHour> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "colorTheme",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::OpaqueSubtree,
	  &colorThemeWrite,
	  &colorThemeRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "SongEditor_ColoringMethod",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_coloringMethod>,
	  &themeScalarRead<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_coloringMethod> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "SongEditor_pattern_color_",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Prefix,
	  &patternColorsWrite,
	  &patternColorsRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "SongEditor_visible_pattern_colors",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &visiblePatternColorsWrite,
	  &visiblePatternColorsRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "iconColor",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &iconColorWrite,
	  &iconColorRead },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "indicateNotePlayback",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_bIndicateNotePlayback>,
	  &themeScalarRead<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_bIndicateNotePlayback> },
	{ { PreferencesKeys::Gui, nullptr, nullptr },
	  "indicateEffectiveNoteLength",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  false,
	  FieldRow::KeyMatch::Exact,
	  &themeScalarWrite<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_bIndicateEffectiveNoteLength>,
	  &themeScalarRead<
		  &Theme::m_pInterface,
		  &InterfaceTheme::m_bIndicateEffectiveNoteLength> },
	// ---- files ----
	{ { PreferencesKeys::Files, nullptr, nullptr },
	  PreferencesKeys::LastSongFilename,
	  Layer::Override,
	  Owner::Core,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastSongPath>,
	  &scalarRead<&PreferencesData::m_sLastSongPath> },
	{ { PreferencesKeys::Files, nullptr, nullptr },
	  PreferencesKeys::LastPlaylistFilename,
	  Layer::Override,
	  Owner::Core,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sLastPlaylistPath>,
	  &scalarRead<&PreferencesData::m_sLastPlaylistPath> },
	{ { PreferencesKeys::Files, nullptr, nullptr },
	  "defaulteditor",
	  Layer::Base,
	  Owner::Gui,
	  false,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &scalarWrite<&PreferencesData::m_sDefaultEditor>,
	  &scalarRead<&PreferencesData::m_sDefaultEditor> },
	{ { PreferencesKeys::Files, nullptr, nullptr },
	  "customSoundLibraryDirs",
	  Layer::Base,
	  Owner::Core,
	  true,
	  true,
	  FieldRow::KeyMatch::Exact,
	  &customDirsWrite,
	  &customDirsRead },
	// ---- root subtrees ----
	{ { nullptr, nullptr, nullptr },
	  "midiEventMap",
	  Layer::Base,
	  Owner::Core,
	  false,
	  false,
	  FieldRow::KeyMatch::OpaqueSubtree,
	  &midiEventMapWrite,
	  &midiEventMapRead },
	{ { nullptr, nullptr, nullptr },
	  "midiInstrumentMap",
	  Layer::Base,
	  Owner::Core,
	  true,
	  true,
	  FieldRow::KeyMatch::OpaqueSubtree,
	  &midiInstrumentMapWrite,
	  &midiInstrumentMapRead },
	{ { nullptr, nullptr, nullptr },
	  "shortcuts",
	  Layer::Base,
	  Owner::Gui,
	  true,
	  true,
	  FieldRow::KeyMatch::OpaqueSubtree,
	  &shortcutsWrite,
	  &shortcutsRead },
};

const int PreferencesSchema::kSchemaRowCount = 135;

// Compile-time enforcement of the single source of truth: the loop bound
// above must match the actual table size, and every member tied by
// tieAllMembers() must have a row. Adding a serialized member without a
// row - or letting the count drift from the table - breaks the build.
static_assert(
	sizeof( PreferencesSchema::kSchemaRows ) /
			sizeof( PreferencesSchema::kSchemaRows[0] ) ==
		PreferencesSchema::kSchemaRowCount,
	"kSchemaRowCount drifted from the actual number of kSchemaRows entries" );
static_assert(
	std::tuple_size<decltype( PreferencesSchema::tieAllMembers(
		std::declval<PreferencesData&>() ) )>::value ==
		PreferencesSchema::kSchemaRowCount,
	"Every serialized PreferencesData member needs a schema row: add one to "
	"kSchemaRows when adding a member" );

// ── Structured-binding canary ──
//
// The assert above pins the row count to the hand-maintained
// tieAllMembers() tuple - but a member added to PreferencesData without a
// tie entry would compile silently and go unserialized. A decomposition
// declaration, in contrast, must name every direct member of the class
// exactly, so any member addition or removal breaks the build right here.
// When that happens: extend the binding below, extend tieAllMembers()
// with the new member, and add a schema row - the count-sum assert at the
// end then fails until all three are consistent.
//
// m_pTheme is named here but tied through its sub-objects (15 entries),
// not as itself. The theme sub-classes (InterfaceTheme, FontTheme,
// ColorTheme) cannot get canaries of their own: they derive from
// Object<T>, whose Base carries non-static data members, and a C++17
// decomposition requires all members to belong to a single class. Adding
// a member to them therefore still relies on extending the tie manually -
// their counts live only in the sum assert below.
//
// The canary is never called; only its declaration is checked.

inline void preferencesDataCanary( PreferencesData& data ) {
	auto& [
		m_sPreferredLanguage, m_nMaxBars, m_nLastOpenTab,
		m_bUseTheRubberbandBpmChangeEvent,
		m_bUseRelativeFileNamesForPlaylists, m_bHideKeyboardCursor,
		m_bShowDevelWarning, m_bShowNoteOverwriteWarning,
		m_bHearNewNotes, m_bQuantizeEvents, m_sRubberBandCLIexecutable,
		m_recentFiles, m_onlineRepos, m_audioDriver, m_bUseMetronome,
		m_fMetronomeVolume, m_nMaxNotes, m_interpolateMode, m_nBufferSize,
		m_nSampleRate, m_bCountIn, m_sOSSDevice, m_sPortAudioDevice,
		m_sPortAudioHostAPI, m_nLatencyTarget, m_sCoreAudioDevice,
		m_sJackPortName1, m_sJackPortName2, m_nJackTransportMode,
		m_bJackTimebaseEnabled, m_bJackTimebaseMode, m_bJackConnectDefaults,
		m_JackTrackOutputMode, m_bJackTrackOuts,
		m_bJackEnforceInstrumentName, m_sAlsaAudioDevice, m_midiDriver,
		m_sMidiPortName, m_sMidiOutputPortName, m_midiActionChannel,
		m_bMidiNoteOffIgnore, m_bEnableMidiFeedback, m_midiFeedbackChannel,
		m_bMidiClockInputHandling, m_bMidiTransportInputHandling,
		m_bMidiClockOutputSend, m_bMidiTransportOutputSend, m_midiSendNoteOff,
		m_nOscServerPort, m_bOscServerEnabled, m_bOscFeedbackEnabled,
		m_nPatternEditorGridResolution, m_nPatternEditorGridHeight,
		m_nPatternEditorGridWidth, m_bPatternEditorUsingTriplets,
		m_bPatternEditorAlwaysShowTypeLabels, m_nSongEditorGridHeight,
		m_nSongEditorGridWidth, m_bShowInstrumentPeaks, m_bShowAutomationArea,
		m_bShowPlaybackTrack, m_mainFormProperties, m_mixerProperties,
		m_patternEditorProperties, m_songEditorProperties, m_rackProperties,
		m_audioEngineInfoProperties, m_playlistEditorProperties,
		m_directorProperties, m_sLastExportPatternAsDirectory,
		m_sLastExportSongDirectory, m_sLastSaveSongAsDirectory,
		m_sLastOpenSongDirectory, m_sLastOpenPatternDirectory,
		m_sLastExportLilypondDirectory, m_sLastExportMidiDirectory,
		m_sLastImportDrumkitDirectory, m_sLastExportDrumkitDirectory,
		m_sLastSaveDrumkitAsDirectory, m_sLastOpenLayerDirectory,
		m_sLastOpenPlaybackTrackDirectory,
		m_sLastAddSongToPlaylistDirectory, m_sLastPlaylistDirectory,
		m_sLastPlaylistScriptDirectory, m_sLastImportThemeDirectory,
		m_sLastExportThemeDirectory, m_nExportModeIdx, m_exportFormat,
		m_fExportCompressionLevel, m_nExportSampleRateIdx,
		m_nExportSampleDepthIdx, m_bShowExportSongLicenseWarning,
		m_bShowExportDrumkitLicenseWarning,
		m_bShowExportDrumkitCopyleftWarning,
		m_bShowExportDrumkitAttributionWarning, m_bFollowPlayhead,
		m_nMidiExportMode, m_bMidiExportUseHumanization,
		m_bSoundLibraryShowName, m_bSoundLibraryShowAuthor,
		m_bSoundLibraryShowInfo, m_bSoundLibraryShowLicense,
		m_bSoundLibraryShowPath, m_bSoundLibraryShowTags,
		m_bSoundLibraryShowVersion, m_nSoundLibraryLastTab, m_nRackLastTab,
		m_bpmTap, m_beatCounter, m_nBeatCounterDriftCompensation,
		m_nBeatCounterStartOffset, m_bPlaySamplesOnClicking,
		m_nAutosavesPerHour, m_sLastSongPath, m_sLastPlaylistPath,
		m_sDefaultEditor, m_customSoundLibraryDirs, m_pMidiEventMap,
		m_pMidiInstrumentMap, m_pShortcuts, m_pTheme
	] = data;
}

// 121 canary-named members - m_pTheme itself (tied through 15 sub-object
// entries: 10 InterfaceTheme + 4 FontTheme + the opaque ColorTheme row)
// + the 120 directly tied members = the 135 schema rows.
static_assert(
	121 - 1 + 10 + 4 + 1 == PreferencesSchema::kSchemaRowCount,
	"The schema row count drifted from the tied members. When adding a "
	"member, extend the canary binding above, tieAllMembers(), and "
	"kSchemaRows together" );

void PreferencesSchema::writeRows(
	XMLNode& rootNode,
	const PreferencesData& data,
	std::optional<Owner> ownerFilter,
	const WriteContext& context
)
{
	std::map<QString, XMLNode> parentCache;
	for ( int ii = 0; ii < kSchemaRowCount; ++ii ) {
		const FieldRow& row = kSchemaRows[ii];
		if ( ownerFilter.has_value() && row.owner != *ownerFilter ) {
			continue;
		}

		XMLNode parent( rootNode );
		QString sPath;
		for ( int jj = 0; jj < 3 && row.path[jj] != nullptr; ++jj ) {
			sPath += QString( row.path[jj] ) + "/";
			auto it = parentCache.find( sPath );
			if ( it == parentCache.end() ) {
				// Rows are in document order, so parents are created in
				// the same order the legacy hand-written writer emitted
				// them.
				it = parentCache
						 .emplace( sPath, parent.createNode( row.path[jj] ) )
						 .first;
			}
			parent = it->second;
		}
		row.write( parent, row, data, context );
	}
}

namespace {

// Whether ownership permits this instance to write the row (ADR 0022/0023).
bool rowEligible( const FieldRow& row, Preferences::FieldOwnership ownership )
{
	switch ( ownership ) {
	case Preferences::FieldOwnership::All:
		return true;
	case Preferences::FieldOwnership::BaseLayer:
		return row.layer == Layer::Base;
	case Preferences::FieldOwnership::GuiOwned:
		return row.layer == Layer::Base && row.owner == Owner::Gui;
	}
	return false;
}

// Whether a child element of a parent belongs to the row: an exact (or
// opaque-subtree) name match, or a formatted-container name prefix.
bool rowMatchesKey( const QDomElement& element, const FieldRow& row )
{
	if ( row.keyMatch == FieldRow::KeyMatch::Prefix ) {
		return QString( element.tagName() ).startsWith( row.key );
	}
	return element.tagName() == row.key;
}

// Compact serialized form of the row's element(s) under parent, concatenated in
// document order. Empty when the row is absent. Serves as the diff footprint on
// both the current and the baseline side.
QString serializeRowElements( const XMLNode& parent, const FieldRow& row )
{
	QString sFootprint;
	QDomElement element = parent.firstChildElement();
	while ( ! element.isNull() ) {
		if ( rowMatchesKey( element, row ) ) {
			QString sElement;
			QTextStream stream( &sElement );
#ifdef H2CORE_HAVE_QT6
			stream.setEncoding( QStringConverter::Utf8 );
#else
			stream.setCodec( "UTF-8" );
#endif
			element.save( stream, 0 );
			sFootprint += sElement;
		}
		element = element.nextSiblingElement();
	}
	return sFootprint;
}

// Drop whitespace-only text nodes (pretty-printing artifacts of the indented
// file format) so a parsed baseline compares equal to the compact writer
// output.
void stripWhitespaceTextNodes( QDomNode node )
{
	QDomNode child = node.firstChild();
	while ( ! child.isNull() ) {
		QDomNode next = child.nextSibling();
		if ( child.isText() && child.toText().data().trimmed().isEmpty() ) {
			node.removeChild( child );
		}
		else {
			stripWhitespaceTextNodes( child );
		}
		child = next;
	}
}

// Remove the row's element(s) from parent so the subsequent write replaces the
// row wholesale — container rows never merge with on-disk entries.
void removeRowElements( XMLNode& parent, const FieldRow& row )
{
	QDomElement element = parent.firstChildElement();
	while ( ! element.isNull() ) {
		QDomElement next = element.nextSiblingElement();
		if ( rowMatchesKey( element, row ) ) {
			parent.removeChild( element );
		}
		element = next;
	}
}

// The row's footprint as the passed instance state would write it. The single
// definition shared by the merge diff (persistRows) and the write-through
// pending check (currentFootprint), so the two can never drift apart.
QString rowFootprint( const FieldRow& row, const PreferencesData& data,
					  const WriteContext& context )
{
	XMLDoc doc;
	XMLNode parent( doc.set_root( "footprint" ) );
	for ( int jj = 0; jj < 3 && row.path[ jj ] != nullptr; ++jj ) {
		parent = parent.createNode( row.path[ jj ] );
	}
	row.write( parent, row, data, context );
	// Normalize like the baseline side.
	stripWhitespaceTextNodes( parent );
	return serializeRowElements( parent, row );
}

// The row's footprint as a freshly default-constructed instance would write it
// — the comparison base for rows the loaded baseline lacks, so unchanged
// defaults are not written.
QString defaultFootprint( const FieldRow& row, const PreferencesData& defaults,
						  const WriteContext& context )
{
	return rowFootprint( row, defaults, context );
}

// Whether every element matched by the row is bare - present on disk but
// carrying no data (no children once whitespace-only text is stripped).
bool rowElementsAreBare( const XMLNode& parent, const FieldRow& row )
{
	QDomElement element = parent.firstChildElement();
	while ( ! element.isNull() ) {
		if ( rowMatchesKey( element, row ) && ! element.firstChild().isNull() ) {
			return false;
		}
		element = element.nextSiblingElement();
	}
	return true;
}

} // namespace

QString PreferencesSchema::currentFootprint(
	const PreferencesData& data,
	Preferences::FieldOwnership ownership,
	const WriteContext& context
)
{
	// Ownership-eligible rows only: rows outside the instance's mask are
	// not its business to write through, so they must not register as
	// pending either (ADR 0023).
	QString sFootprint;
	for ( int ii = 0; ii < kSchemaRowCount; ++ii ) {
		const FieldRow& row = kSchemaRows[ ii ];
		if ( ! rowEligible( row, ownership ) ) {
			continue;
		}
		QString sLabel;
		for ( int jj = 0; jj < 3 && row.path[ jj ] != nullptr; ++jj ) {
			sLabel += QString( row.path[ jj ] ) + '/';
		}
		sLabel += QString( row.key );
		sFootprint += sLabel + '=' + rowFootprint( row, data, context ) + '\n';
	}
	return sFootprint;
}

int PreferencesSchema::persistRows(
	XMLNode& rootNode,
	XMLNode& baselineRoot,
	const PreferencesData& data,
	Preferences::FieldOwnership ownership,
	const WriteContext& context
)
{
	if ( ! baselineRoot.isNull() ) {
		stripWhitespaceTextNodes( baselineRoot );
	}

	// Constructed lazily, at most once per call: only legacy baselines
	// lacking rows need the default footprints.
	std::unique_ptr<PreferencesData> pDefaults;
	std::map<int, QString> defaultFootprints;

	std::map<QString, XMLNode> parentCache;
	std::map<QString, XMLNode> baselineParentCache;
	int nWritten = 0;

	for ( int ii = 0; ii < kSchemaRowCount; ++ii ) {
		const FieldRow& row = kSchemaRows[ ii ];
		if ( ! rowEligible( row, ownership ) ) {
			continue;
		}

		// Current footprint: the row as this instance would write it.
		const QString sCurrent = rowFootprint( row, data, context );

		XMLNode baselineParent;
		if ( ! baselineRoot.isNull() ) {
			baselineParent = baselineRoot;
			QString sPath;
			for ( int jj = 0; jj < 3 && row.path[ jj ] != nullptr; ++jj ) {
				sPath += QString( row.path[ jj ] ) + "/";
				auto it = baselineParentCache.find( sPath );
				if ( it == baselineParentCache.end() ) {
					it = baselineParentCache
							 .emplace( sPath, baselineParent.firstChildElement(
												  row.path[ jj ] ) )
							 .first;
				}
				baselineParent = it->second;
				if ( baselineParent.isNull() ) {
					break;
				}
			}
		}
		// Baseline footprint: the row as it was loaded. A missing parent, a
		// missing row or (unless empty is a legitimate value for the row) a
		// bare row all loaded as defaults, so they compare against the
		// default footprint instead - unchanged defaults must not be written
		// over concurrent edits. Empty containers whose struct defaults are
		// also empty (midiEventMap, recentUsedSongs) serialize bare and
		// compare equal to their bare default footprint; if such a default
		// ever gains entries, legitimately-empty containers would need
		// special handling here.
		QString sBaseline;
		bool bLoadedAsDefault = baselineParent.isNull();
		if ( ! bLoadedAsDefault ) {
			sBaseline = serializeRowElements( baselineParent, row );
			bLoadedAsDefault = sBaseline.isEmpty() ||
				( ! row.bEmptyOk && rowElementsAreBare( baselineParent, row ) );
		}
		if ( bLoadedAsDefault ) {
			auto it = defaultFootprints.find( ii );
			if ( it == defaultFootprints.end() ) {
				if ( pDefaults == nullptr ) {
					pDefaults = std::make_unique<PreferencesData>();
				}
				it = defaultFootprints
						 .emplace( ii, defaultFootprint( row, *pDefaults,
														 context ) )
						 .first;
			}
			sBaseline = it->second;
		}

		if ( sCurrent == sBaseline ) {
			continue;
		}

		// Target parent on the merge document: find-or-create. Created
		// parents append at the end — loads are order-insensitive.
		XMLNode parent( rootNode );
		QString sPath;
		for ( int jj = 0; jj < 3 && row.path[ jj ] != nullptr; ++jj ) {
			sPath += QString( row.path[ jj ] ) + "/";
			auto it = parentCache.find( sPath );
			if ( it == parentCache.end() ) {
				XMLNode existing = parent.firstChildElement( row.path[ jj ] );
				it = parentCache
						 .emplace( sPath, existing.isNull()
											  ? parent.createNode(
													row.path[ jj ] )
											  : existing )
						 .first;
			}
			parent = it->second;
		}

		removeRowElements( parent, row );
		row.write( parent, row, data, context );
		++nWritten;
	}

	return nWritten;
}

void PreferencesSchema::readRows(
	const XMLNode& rootNode,
	PreferencesData& data,
	std::optional<Owner> ownerFilter,
	const ReadContext& context
)
{
	std::set<QString> warnedPaths;
	for ( int ii = 0; ii < kSchemaRowCount; ++ii ) {
		const FieldRow& row = kSchemaRows[ii];
		if ( ownerFilter.has_value() && row.owner != *ownerFilter ) {
			continue;
		}

		XMLNode parent( rootNode );
		bool bPathPresent = true;
		QString sPath;
		for ( int jj = 0; jj < 3 && row.path[jj] != nullptr; ++jj ) {
			sPath += QString( row.path[jj] ) + "/";
			const QDomElement child = parent.firstChildElement( row.path[jj] );
			if ( child.isNull() ) {
				bPathPresent = false;
				if ( context.bEmitStructuralWarnings &&
					 warnedPaths.insert( sPath ).second ) {
					WARNINGLOG(
						QString( "Config element <%1> not found - the fields "
								 "below it keep their current values" )
							.arg( row.path[jj] )
					);
				}
				break;
			}
			parent = XMLNode( child );
		}
		if ( !bPathPresent ) {
			continue;
		}
		row.read( parent, row, data, context );
	}
}

void PreferencesSchema::checkForUnknownElements( const XMLNode& rootNode )
{
	// Elements written by the save() prologue and consumed by load()
	// itself - not covered by any row.
	static const char* sRootAllowlist[] = {
		"formatVersion", "version", "instrumentInputMode"
	};
	// Pre-2.0 flags read by the load() prologue for the
	// midiInstrumentMap derivation.
	static const char* sMidiDriverAllowlist[] = {
		"fixed_mapping", "discard_note_after_action"
	};

	// Elements not covered by schema rows at the given parent position.
	const auto allowlistFor = []( const QString& sParent1,
								  const QString& sParent2
							  ) -> std::pair<const char* const*, int> {
		if ( sParent1.isEmpty() && sParent2.isEmpty() ) {
			return { sRootAllowlist, 3 };
		}
		if ( sParent1 == PreferencesKeys::AudioEngine &&
			 sParent2 == PreferencesKeys::MidiDriver ) {
			return { sMidiDriverAllowlist, 2 };
		}
		return { nullptr, 0 };
	};

	auto checkLevel = [&]( auto&& self, const XMLNode& node,
						   const QString& sParent1,
						   const QString& sParent2 ) -> void {
		const auto [pAllowlist, nAllowlistSize] =
			allowlistFor( sParent1, sParent2 );
		std::set<QString> sExact;
		std::vector<QString> sPrefixes;
		std::set<QString> sOpaque;
		std::set<QString> sContainers;
		for ( int ii = 0; ii < nAllowlistSize; ii++ ) {
			sExact.insert( pAllowlist[ii] );
		}
		for ( int ii = 0; ii < kSchemaRowCount; ii++ ) {
			const FieldRow& row = kSchemaRows[ii];
			const QString sRowParent1 =
				row.path[0] != nullptr ? QString( row.path[0] ) : QString();
			const QString sRowParent2 =
				row.path[1] != nullptr ? QString( row.path[1] ) : QString();
			if ( sRowParent1 == sParent1 && sRowParent2 == sParent2 ) {
				// The row lives directly in this element.
				switch ( row.keyMatch ) {
					case FieldRow::KeyMatch::OpaqueSubtree:
						sOpaque.insert( row.key );
						break;
					case FieldRow::KeyMatch::Prefix:
						sPrefixes.push_back( row.key );
						break;
					case FieldRow::KeyMatch::Exact:
						sExact.insert( row.key );
						break;
				}
			}
			else if ( sRowParent1 == sParent1 && sParent2.isEmpty() &&
					  !sRowParent2.isEmpty() ) {
				// The row lives one level deeper: its parent element is a
				// container here.
				sContainers.insert( sRowParent2 );
			}
			else if ( sParent1.isEmpty() && sParent2.isEmpty() &&
					  !sRowParent1.isEmpty() ) {
				// The row lives in a root-level container (audio_engine,
				// gui, files).
				sContainers.insert( sRowParent1 );
			}
		}

		for ( QDomElement child = node.firstChildElement(); !child.isNull();
			  child = child.nextSiblingElement() ) {
			const QString sName = child.tagName();
			if ( sOpaque.count( sName ) > 0 || sExact.count( sName ) > 0 ) {
				continue;
			}
			bool bPrefixMatched = false;
			for ( const auto& sPrefix : sPrefixes ) {
				if ( sName.startsWith( sPrefix ) ) {
					bPrefixMatched = true;
					break;
				}
			}
			if ( bPrefixMatched ) {
				continue;
			}
			if ( sContainers.count( sName ) > 0 ) {
				if ( sParent1.isEmpty() ) {
					self( self, XMLNode( child ), sName, QString() );
				}
				else {
					self( self, XMLNode( child ), sParent1, sName );
				}
				continue;
			}
			ERRORLOG( QString( "Unknown element <%1> in the preferences file - "
							   "please report this bug" )
						  .arg( sName ) );
		}
	};

	checkLevel( checkLevel, rootNode, QString(), QString() );
}

};	// namespace H2Core
