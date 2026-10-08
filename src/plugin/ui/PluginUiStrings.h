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

// All translatable strings of the embedded plugin UI (proposal 0006 TU4.0).
//
// The plugin UI must not depend on the Qt-Widgets GUI application, so it
// cannot reuse its CommonStrings. This class is the plugin UI's own
// translation context instead: every user-facing string goes through
// PluginUiStrings::tr() from day one, and the extraction joins the existing
// data/i18n catalogs when the wiring lands (UI-6, TU6.1).

#ifndef H2C_PLUGIN_UI_STRINGS_H
#define H2C_PLUGIN_UI_STRINGS_H

#include <QString>
#include <QtCore/QCoreApplication>

#include <core/Midi/Midi.h>

namespace H2Core {

class PluginUiStrings {
	Q_DECLARE_TR_FUNCTIONS( PluginUiStrings )

public:
	// The three views' tabs.
	static QString generalTab() { return tr( "General" ); }
	static QString mappingTab() { return tr( "Mapping" ); }
	static QString mixerTab() { return tr( "Mixer" ); }

	// Song tab: load buttons, browser titles, MIDI configuration.
	static QString loadSongButton() { return tr( "Load song..." ); }
	static QString loadPatternButton() { return tr( "Load pattern..." ); }
	static QString loadDrumkitButton() { return tr( "Load drumkit..." ); }
	static QString openEditorButton() { return tr( "Open editor" ); }
	static QString songFileDialogTitle() {
		return tr( "Choose a song file" );
	}
	static QString patternFileDialogTitle() {
		return tr( "Choose a pattern file" );
	}
	static QString drumkitFileDialogTitle() {
		return tr( "Choose a drumkit folder" );
	}
	static QString midiConfigurationHeader() {
		return tr( "MIDI configuration" );
	}
	static QString ignoreNoteOffLabel() { return tr( "Ignore note-off" ); }
	static QString actionChannelLabel() { return tr( "Action channel" ); }
	static QString sendNoteOffLabel() {
		return tr( "Send Note-Off messages" );
	}
	static QString applyButton() { return tr( "Apply" ); }
	// Pattern-load target row.
	static QString patternNumberLabel() {
		return tr( "Pattern number" );
	}
	static QString replaceLabel() { return tr( "Replace" ); }
	// Send Note-Off combo entries (mirroring the GUI dialog's texts).
	static QString optionAlways() { return tr( "Always" ); }
	static QString optionOnCustomNoteLengths() {
		return tr( "On custom note lengths" );
	}
	static QString optionNever() { return tr( "Never" ); }
	// Channel dropdown entries (the GUI's channel convention: -1 all
	// channels, 0 off, 1..16 one channel).
	static QString optionAll() { return tr( "all" ); }
	static QString optionOff() { return tr( "Off" ); }
	/** The display name of @a channel — "all" for #Midi::ChannelAll,
	 * "Off" for #Midi::ChannelOff and for the invalid -2 of unmapped
	 * rows (the GUI disables those widgets instead), the number itself
	 * for 1..16 — the convention of the channel dropdowns and the
	 * read-only mapping rows (the GUI's channel spinboxes show the
	 * same). */
	static QString channelDisplayName( Midi::Channel channel ) {
		if ( channel == Midi::ChannelAll ) {
			return optionAll();
		}
		if ( channel == Midi::ChannelOff || channel == Midi::ChannelInvalid ) {
			return optionOff();
		}
		return QString::number( static_cast<int>( channel ) );
	}

	// Mapping tab: sections and table columns.
	static QString noteMappingHeader() { return tr( "Note Mapping" ); }
	static QString globalChannelLabel() { return tr( "Global Channel" ); }
	static QString useGlobalChannelLabel() {
		return tr( "Use Global Channel" );
	}
	static QString inputHeader() { return tr( "Input" ); }
	static QString outputHeader() { return tr( "Output" ); }
	static QString instrumentColumn() { return tr( "Instrument" ); }
	static QString channelColumn() { return tr( "Channel" ); }
	static QString noteColumn() { return tr( "Note" ); }

	// Mixer tab: master strip, strip controls, buses.
	static QString masterLabel() { return tr( "Master" ); }
	static QString muteButton() { return tr( "Mute" ); }
	static QString soloButton() { return tr( "Solo" ); }
	static QString volumeLabel() { return tr( "Volume" ); }
	static QString panLabel() { return tr( "Pan" ); }
	static QString busLabel() { return tr( "Bus" ); }
	static QString resetButton() { return tr( "Reset" ); }
	static QString masterOnlyLabel() { return tr( "Master only" ); }
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_STRINGS_H
