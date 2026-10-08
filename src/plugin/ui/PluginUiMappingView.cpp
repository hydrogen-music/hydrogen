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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

// The MIDI mapping view's renderer (proposal 0006 TU4.2): the map-wide
// settings and the per-instrument rows. Every widget applies its change
// with one model command (apply-on-change — the model's atomic-command
// design); nothing is held locally, so nothing can drift. Thin draw
// calls only (TU1.3): every decision is the model's.

#include "PluginUiMappingView.h"

#include <imgui.h>

#include "PluginUiStrings.h"

namespace H2Core {

PluginUiMappingView::PluginUiMappingView( HydrogenPlugin* pPlugin )
	: m_model( pPlugin )
{
}

void PluginUiMappingView::draw()
{
	auto pMap = m_model.liveMap();
	if ( pMap == nullptr ) {
		// No engine yet — nothing to map.
		return;
	}

	ImGui::TextUnformatted( PluginUiStrings::noteMappingHeader().toUtf8()
								.constData() );

	// Map-wide settings, applied on change. The combo entries must
	// outlive the Combo calls — held locals, not chained temporaries.
	const auto sInputNone = MidiInstrumentMap::InputToQString(
		MidiInstrumentMap::Input::None ).toUtf8();
	const auto sInputAsOutput = MidiInstrumentMap::InputToQString(
		MidiInstrumentMap::Input::AsOutput ).toUtf8();
	const auto sInputCustom = MidiInstrumentMap::InputToQString(
		MidiInstrumentMap::Input::Custom ).toUtf8();
	const auto sInputSelected = MidiInstrumentMap::InputToQString(
		MidiInstrumentMap::Input::SelectedInstrument ).toUtf8();
	const auto sInputOrder = MidiInstrumentMap::InputToQString(
		MidiInstrumentMap::Input::Order ).toUtf8();
	const char* sInputModes[ 5 ] = {
		sInputNone.constData(),   sInputAsOutput.constData(),
		sInputCustom.constData(), sInputSelected.constData(),
		sInputOrder.constData()
	};
	int nInputMode = static_cast<int>( pMap->getInput() );
	if ( ImGui::Combo( PluginUiStrings::inputHeader().toUtf8()
						   .constData(),
					   &nInputMode, sInputModes, 5 ) ) {
		m_model.setInputMode(
			static_cast<MidiInstrumentMap::Input>( nInputMode ) );
	}

	const auto sOutputNone = MidiInstrumentMap::OutputToQString(
		MidiInstrumentMap::Output::None ).toUtf8();
	const auto sOutputOffset = MidiInstrumentMap::OutputToQString(
		MidiInstrumentMap::Output::Offset ).toUtf8();
	const auto sOutputConstant = MidiInstrumentMap::OutputToQString(
		MidiInstrumentMap::Output::Constant ).toUtf8();
	const char* sOutputModes[ 3 ] = {
		sOutputNone.constData(), sOutputOffset.constData(),
		sOutputConstant.constData()
	};
	int nOutputMode = static_cast<int>( pMap->getOutput() );
	if ( ImGui::Combo( PluginUiStrings::outputHeader().toUtf8()
						   .constData(),
					   &nOutputMode, sOutputModes, 3 ) ) {
		m_model.setOutputMode(
			static_cast<MidiInstrumentMap::Output>( nOutputMode ) );
	}

	// The channel dropdowns' entries — all / Off / 1..16, one per
	// selectable channel: index 0 is "all" (-1), 1 is "Off" (0), 2..17
	// are the channels, the same shift every channel combo below
	// applies. The QByteArrays must outlive the combo calls, so they
	// are held next to the pointers into them.
	QByteArray sChannelEntries[ 18 ];
	const char* sChannelEntryPtrs[ 18 ];
	for ( int n = 0; n < 18; ++n ) {
		sChannelEntries[ n ] = PluginUiStrings::channelDisplayName(
			Midi::channelFromInt( n - 1 ) ).toUtf8();
		sChannelEntryPtrs[ n ] = sChannelEntries[ n ].constData();
	}

	// Global channels — dropdowns with the GUI's spinbox texts (all
	// channels, Off, 1..16). The "##" suffixes keep the twice-used
	// labels' imgui IDs apart.
	const auto sUseGlobalInput = ( PluginUiStrings::
									   useGlobalChannelLabel() +
								   "##input" ).toUtf8();
	bool bUseGlobalInput = pMap->getUseGlobalInputChannel();
	if ( ImGui::Checkbox( sUseGlobalInput.constData(),
						  &bUseGlobalInput ) ) {
		m_model.setUseGlobalInputChannel( bUseGlobalInput );
	}
	ImGui::SameLine();
	const auto sGlobalInputChannel = ( PluginUiStrings::
											globalChannelLabel() +
										"##input" ).toUtf8();
	int nGlobalInputChannel =
		static_cast<int>( pMap->getGlobalInputChannel() ) + 1;
	ImGui::SetNextItemWidth( 60.0f );
	if ( ImGui::Combo( sGlobalInputChannel.constData(),
					   &nGlobalInputChannel, sChannelEntryPtrs, 18 ) ) {
		m_model.setGlobalInputChannel(
			Midi::channelFromInt( nGlobalInputChannel - 1 ) );
	}

	const auto sUseGlobalOutput = ( PluginUiStrings::
										useGlobalChannelLabel() +
									"##output" ).toUtf8();
	bool bUseGlobalOutput = pMap->getUseGlobalOutputChannel();
	if ( ImGui::Checkbox( sUseGlobalOutput.constData(),
						  &bUseGlobalOutput ) ) {
		m_model.setUseGlobalOutputChannel( bUseGlobalOutput );
	}
	ImGui::SameLine();
	const auto sGlobalOutputChannel = ( PluginUiStrings::
											globalChannelLabel() +
										"##output" ).toUtf8();
	int nGlobalOutputChannel =
		static_cast<int>( pMap->getGlobalOutputChannel() ) + 1;
	ImGui::SetNextItemWidth( 60.0f );
	if ( ImGui::Combo( sGlobalOutputChannel.constData(),
					   &nGlobalOutputChannel, sChannelEntryPtrs, 18 ) ) {
		m_model.setGlobalOutputChannel(
			Midi::channelFromInt( nGlobalOutputChannel - 1 ) );
	}

	const int nInstruments = m_model.instrumentCount();

	// The per-instrument input rows: the effective incoming
	// note/channel, editable only in Custom mode (the only mode with
	// per-instrument rows).
	ImGui::TextUnformatted( PluginUiStrings::inputHeader().toUtf8()
								.constData() );
	ImGui::PushID( "input" );
	if ( ImGui::BeginTable( "##table", 3,
							ImGuiTableFlags_Borders |
								ImGuiTableFlags_RowBg ) ) {
		ImGui::TableSetupColumn( PluginUiStrings::instrumentColumn()
									.toUtf8().constData() );
		ImGui::TableSetupColumn( PluginUiStrings::noteColumn()
									.toUtf8().constData() );
		ImGui::TableSetupColumn( PluginUiStrings::channelColumn()
									.toUtf8().constData() );
		ImGui::TableHeadersRow();

		const bool bCustom = pMap->getInput() ==
			MidiInstrumentMap::Input::Custom;
		for ( int n = 0; n < nInstruments; ++n ) {
			ImGui::TableNextRow();
			ImGui::PushID( n );

			ImGui::TableNextColumn();
			const auto sName =
				m_model.instrumentName( n ).left( 24 ).toUtf8();
			ImGui::TextUnformatted( sName.constData(),
									sName.constData() + sName.size() );

			const auto mapping = m_model.inputMapping( n );
		ImGui::TableNextColumn();
		int nNote = static_cast<int>( mapping.note );
		if ( bCustom ) {
			ImGui::SetNextItemWidth( 52.0f );
			if ( ImGui::InputInt( "##note", &nNote, 0, 0 ) ) {
				m_model.insertCustomInputMapping(
					n, Midi::noteFromIntClamp( nNote ),
					mapping.channel );
			}
		}
		else if ( mapping.note == Midi::NoteInvalid ) {
			// Unmapped in this mode (e.g. Selected Instrument): the
			// GUI disables the widgets — the read-only row reads
			// "Off" for both fields.
			const auto sOff = PluginUiStrings::optionOff().toUtf8();
			ImGui::TextUnformatted( sOff.constData(),
									sOff.constData() + sOff.size() );
		}
		else {
			ImGui::Text( "%d", nNote );
		}
		ImGui::TableNextColumn();
		if ( bCustom ) {
			int nChannel = static_cast<int>( mapping.channel ) + 1;
			ImGui::SetNextItemWidth( 52.0f );
			if ( ImGui::Combo( "##channel", &nChannel,
							   sChannelEntryPtrs, 18 ) ) {
				m_model.insertCustomInputMapping(
					n, mapping.note,
					Midi::channelFromInt( nChannel - 1 ) );
			}
		}
		else {
			const auto sChannel = PluginUiStrings::channelDisplayName(
				mapping.channel ).toUtf8();
			ImGui::TextUnformatted( sChannel.constData(),
									sChannel.constData() +
										sChannel.size() );
		}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	ImGui::PopID();

	// The per-instrument output rows: the instrument's outgoing
	// note/channel.
	ImGui::TextUnformatted( PluginUiStrings::outputHeader().toUtf8()
								.constData() );
	ImGui::PushID( "output" );
	if ( ImGui::BeginTable( "##table", 3,
							ImGuiTableFlags_Borders |
								ImGuiTableFlags_RowBg ) ) {
		ImGui::TableSetupColumn( PluginUiStrings::instrumentColumn()
									.toUtf8().constData() );
		ImGui::TableSetupColumn( PluginUiStrings::noteColumn()
									.toUtf8().constData() );
		ImGui::TableSetupColumn( PluginUiStrings::channelColumn()
									.toUtf8().constData() );
		ImGui::TableHeadersRow();

		for ( int n = 0; n < nInstruments; ++n ) {
			ImGui::TableNextRow();
			ImGui::PushID( n );

			ImGui::TableNextColumn();
			const auto sName =
				m_model.instrumentName( n ).left( 24 ).toUtf8();
			ImGui::TextUnformatted( sName.constData(),
									sName.constData() + sName.size() );

			ImGui::TableNextColumn();
			int nNote = static_cast<int>(
				m_model.instrumentMidiOutNote( n ) );
			ImGui::SetNextItemWidth( 52.0f );
			if ( ImGui::InputInt( "##note", &nNote, 0, 0 ) ) {
				m_model.setInstrumentMidiOutNote(
					n, Midi::noteFromIntClamp( nNote ) );
			}
		ImGui::TableNextColumn();
		int nChannel = static_cast<int>(
			m_model.instrumentMidiOutChannel( n ) ) + 1;
		ImGui::SetNextItemWidth( 52.0f );
		if ( ImGui::Combo( "##channel", &nChannel,
						   sChannelEntryPtrs, 18 ) ) {
			m_model.setInstrumentMidiOutChannel(
				n, Midi::channelFromInt( nChannel - 1 ) );
		}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	ImGui::PopID();
}

} // namespace H2Core
