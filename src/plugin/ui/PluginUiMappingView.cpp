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

// The MIDI mapping view's renderer (proposal 0006 TU4.2): the exposed MIDI
// configuration foldable section (moved from the landing view — feedback and
// transport stay engine-side, settable via the editor) and the note mapping as
// two foldable sections — "Input" (input mode, global input channel, the
// per-instrument input rows) and "Output" (output mode, global output channel,
// the per-instrument output rows). The map widgets apply their change with one
// model command (apply-on-change — the model's atomic-command design); nothing
// is held locally, so nothing can drift — except the MIDI config widgets, whose
// values are held locally and applied on the section's Apply button. Thin draw
// calls only (TU1.3): every decision is the model's.

#include "PluginUiMappingView.h"

#include <imgui.h>

#include <core/CommonStrings.h>
#include <plugin/HydrogenPlugin.h>

namespace H2Core {

PluginUiMappingView::PluginUiMappingView( HydrogenPlugin* pPlugin )
	: m_model( pPlugin ), m_pCommonStrings( pPlugin->commonStrings() )
{
	refreshMidiConfigWidgets();
}

void PluginUiMappingView::refreshMidiConfigWidgets()
{
	m_bNoteOffIgnore = m_model.midiNoteOffIgnore();
	m_nActionChannel = static_cast<int>( m_model.midiActionChannel() );
	m_nSendNoteOff = static_cast<int>( m_model.midiSendNoteOff() );
}

void PluginUiMappingView::draw()
{
	// The exposed MIDI configuration — drawn before the engine guard so
	// the section renders (with the engine defaults) even without an
	// engine, exactly as it did on the landing view.
	if ( ImGui::CollapsingHeader(
			 m_pCommonStrings->getMidiConfigurationHeader().toUtf8()
					.constData(),
			 ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::Checkbox(
			m_pCommonStrings->getIgnoreNoteOffLabel().toUtf8().constData(),
			&m_bNoteOffIgnore );

		// The action channel dropdown — the GUI spinbox's range and
		// texts (all channels, Off, 1..16), the same +1 index shift as
		// the channel dropdowns below. The QByteArrays must outlive
		// the Combo call — held locals, not chained temporaries.
		QByteArray sActionChannelEntries[ 18 ];
		const char* sActionChannelEntryPtrs[ 18 ];
		for ( int n = 0; n < 18; ++n ) {
			sActionChannelEntries[ n ] =
				m_pCommonStrings->getChannelDisplayName(
					Midi::channelFromInt( n - 1 ) ).toUtf8();
			sActionChannelEntryPtrs[ n ] =
				sActionChannelEntries[ n ].constData();
		}
		int nActionChannel = m_nActionChannel + 1;
		ImGui::SetNextItemWidth( 72.0f );
		if ( ImGui::Combo(
				 m_pCommonStrings->getActionChannelLabel().toUtf8()
					 .constData(),
				 &nActionChannel, sActionChannelEntryPtrs, 18 ) ) {
			m_nActionChannel = nActionChannel - 1;
		}

		// The combo entries must outlive the Combo call — held locals,
		// not chained temporaries.
		const auto sAlways = m_pCommonStrings->getOptionAlways().toUtf8();
		const auto sOnCustomNoteLengths =
			m_pCommonStrings->getOptionOnCustomNoteLengths().toUtf8();
		const auto sNever = m_pCommonStrings->getOptionNever().toUtf8();
		const char* sSendNoteOffEntries[ 3 ] = {
			sAlways.constData(), sOnCustomNoteLengths.constData(),
			sNever.constData()
		};
		ImGui::Combo( m_pCommonStrings->getSendNoteOffLabel().toUtf8()
							.constData(),
					 &m_nSendNoteOff, sSendNoteOffEntries, 3 );

		if ( ImGui::Button( m_pCommonStrings->getApplyButton().toUtf8()
								.constData() ) ) {
			if ( m_model.applyExposedMidiControlSettings(
					 m_bNoteOffIgnore,
					 Midi::channelFromInt( m_nActionChannel ),
					 static_cast<Preferences::MidiSendNoteOff>(
						 m_nSendNoteOff ) ) ) {
				// The engine may have normalized a value — resync the
				// widgets, but never per frame (snap-back).
				refreshMidiConfigWidgets();
			}
		}
	}

	auto pMap = m_model.liveMap();
	if ( pMap == nullptr ) {
		// No engine yet — nothing to map.
		return;
	}

	// The channel dropdowns' entries — All / Off / 1..16, one per
	// selectable channel: index 0 is "All" (-1), 1 is "Off" (0), 2..17
	// are the channels, the same shift every channel combo below
	// applies. The QByteArrays must outlive the combo calls, so they
	// are held next to the pointers into them.
	QByteArray sChannelEntries[ 18 ];
	const char* sChannelEntryPtrs[ 18 ];
	for ( int n = 0; n < 18; ++n ) {
		sChannelEntries[ n ] = m_pCommonStrings->getChannelDisplayName(
			Midi::channelFromInt( n - 1 ) ).toUtf8();
		sChannelEntryPtrs[ n ] = sChannelEntries[ n ].constData();
	}

	const int nInstruments = m_model.instrumentCount();

	ImGui::Dummy( ImVec2( 0.0f, PluginUiMappingView::nSpacing ) );

	// The sections reuse the existing "Input"/"Output" strings as
	// headers and the "Note mapping" string as the mode combos' label
	// (the "##" suffixes keep the twice-used label's imgui IDs apart) —
	// no new translatable strings.
	if ( ImGui::CollapsingHeader(
			 m_pCommonStrings->getInputHeader().toUtf8().constData(),
			 ImGuiTreeNodeFlags_DefaultOpen ) ) {
		// The map-wide input setting, applied on change. The combo
		// entries must outlive the Combo call — held locals, not
		// chained temporaries.
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
		if ( ImGui::Combo(
				 ( m_pCommonStrings->getNoteMappingHeader() + "##input" )
					 .toUtf8()
					 .constData(),
				 &nInputMode, sInputModes, 5
			 ) ) {
			m_model.setInputMode(
				static_cast<MidiInstrumentMap::Input>( nInputMode )
			);
		}

		// The global input channel — a dropdown with the GUI's spinbox
		// texts (all channels, Off, 1..16). The "##" suffixes keep the
		// twice-used labels' imgui IDs apart from the output section's
		// twins below.
		const auto sUseGlobalInput = ( m_pCommonStrings->
										   getUseGlobalChannelLabel() +
									   "##input" ).toUtf8();
		bool bUseGlobalInput = pMap->getUseGlobalInputChannel();
		if ( ImGui::Checkbox( sUseGlobalInput.constData(),
							  &bUseGlobalInput ) ) {
			m_model.setUseGlobalInputChannel( bUseGlobalInput );
		}
		const auto sGlobalInputChannel = ( m_pCommonStrings->
												getGlobalChannelLabel() +
											"##input" ).toUtf8();
		int nGlobalInputChannel =
			static_cast<int>( pMap->getGlobalInputChannel() ) + 1;
		ImGui::SetNextItemWidth( 60.0f );
		if ( ImGui::Combo( sGlobalInputChannel.constData(),
						   &nGlobalInputChannel, sChannelEntryPtrs, 18 ) ) {
			m_model.setGlobalInputChannel(
				Midi::channelFromInt( nGlobalInputChannel - 1 ) );
		}

		// The per-instrument input rows: the effective incoming
		// note/channel, editable only in Custom mode (the only mode
		// with per-instrument rows). No standalone "Input" label — the
		// section header carries it.
		ImGui::PushID( "input" );
		if ( ImGui::BeginTable( "##table", 3,
								ImGuiTableFlags_Borders |
									ImGuiTableFlags_RowBg ) ) {
			ImGui::TableSetupColumn( m_pCommonStrings->getInstrumentButton()
										 .toUtf8().constData() );
			ImGui::TableSetupColumn( m_pCommonStrings->getMidiOutNoteLabel()
										 .toUtf8().constData() );
			ImGui::TableSetupColumn(
				m_pCommonStrings->getMidiOutChannelLabel().toUtf8()
					.constData() );
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
					// Unmapped in this mode (e.g. Selected Instrument):
					// the GUI disables the widgets — the read-only row
					// reads "Off" for both fields.
					const auto sOff =
						m_pCommonStrings->getStatusOff().toUtf8();
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
					const auto sChannel =
						m_pCommonStrings->getChannelDisplayName(
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
	}

	ImGui::Dummy( ImVec2( 0.0f, PluginUiMappingView::nSpacing ) );

	if ( ImGui::CollapsingHeader(
			 m_pCommonStrings->getOutputHeader().toUtf8().constData(),
			 ImGuiTreeNodeFlags_DefaultOpen ) ) {
		// The map-wide output setting, applied on change. The combo
		// entries must outlive the Combo call — held locals, not
		// chained temporaries.
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
		if ( ImGui::Combo(
				 ( m_pCommonStrings->getNoteMappingHeader() + "##output" )
					 .toUtf8()
					 .constData(),
				 &nOutputMode, sOutputModes, 3
			 ) ) {
			m_model.setOutputMode(
				static_cast<MidiInstrumentMap::Output>( nOutputMode )
			);
		}

		// The global output channel — the input section's twin above;
		// the "##" suffixes keep the twice-used labels' imgui IDs
		// apart.
		const auto sUseGlobalOutput = ( m_pCommonStrings->
											getUseGlobalChannelLabel() +
										"##output" ).toUtf8();
		bool bUseGlobalOutput = pMap->getUseGlobalOutputChannel();
		if ( ImGui::Checkbox( sUseGlobalOutput.constData(),
							  &bUseGlobalOutput ) ) {
			m_model.setUseGlobalOutputChannel( bUseGlobalOutput );
		}
		const auto sGlobalOutputChannel = ( m_pCommonStrings->
												  getGlobalChannelLabel() +
											  "##output" ).toUtf8();
		int nGlobalOutputChannel =
			static_cast<int>( pMap->getGlobalOutputChannel() ) + 1;
		ImGui::SetNextItemWidth( 60.0f );
		if ( ImGui::Combo( sGlobalOutputChannel.constData(),
						   &nGlobalOutputChannel, sChannelEntryPtrs, 18 ) ) {
			m_model.setGlobalOutputChannel(
				Midi::channelFromInt( nGlobalOutputChannel - 1 ) );
		}

		// The per-instrument output rows: the instrument's outgoing
		// note/channel. No standalone "Output" label — the section
		// header carries it.
		ImGui::PushID( "output" );
		if ( ImGui::BeginTable( "##table", 3,
								ImGuiTableFlags_Borders |
									ImGuiTableFlags_RowBg ) ) {
			ImGui::TableSetupColumn( m_pCommonStrings->getInstrumentButton()
										 .toUtf8().constData() );
			ImGui::TableSetupColumn( m_pCommonStrings->getMidiOutNoteLabel()
										 .toUtf8().constData() );
			ImGui::TableSetupColumn(
				m_pCommonStrings->getMidiOutChannelLabel().toUtf8()
					.constData() );
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
}

} // namespace H2Core
