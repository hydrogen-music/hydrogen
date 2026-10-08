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

// The landing view's model (proposal 0006 TU4.1): the load/install commands
// behind the landing view's buttons, the editor button, and the MIDI config
// apply — all through the plugin's engine commands (ADR 0022's
// plugin-instance layer). Pure logic, no imgui types (TU1.3): the renderers
// only forward widget state in and results out.
//
// The MIDI config applies are runtime-only: the plugin never persists them
// back to the user config (the GUI dialog's save is deliberately not
// replicated — a host session must not clobber the user's data folder).

#ifndef H2C_PLUGIN_UI_LANDING_MODEL_H
#define H2C_PLUGIN_UI_LANDING_MODEL_H

#include <memory>
#include <QString>

#include <core/Midi/Midi.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {

class HydrogenPlugin;
class MidiInstrumentMap;

class PluginUiLandingModel {
public:
	explicit PluginUiLandingModel( HydrogenPlugin* pPlugin );
	~PluginUiLandingModel();

	PluginUiLandingModel( const PluginUiLandingModel& ) = delete;
	PluginUiLandingModel& operator=( const PluginUiLandingModel& ) = delete;

	/** Loads the .h2song at @a sPath and installs it as the engine's
	 * current song (loadSong + setSong). False without touching the
	 * current song when the path does not load. */
	bool loadSong( const QString& sPath );
	/** Loads the .h2pattern at @a sPath and installs it at
	 * @a nPatternNumber of the current song (loadPattern +
	 * setPattern). False without touching the song when the path does
	 * not load. */
	bool loadPattern( const QString& sPath, int nPatternNumber,
					  bool bReplace );
	/** Loads the drumkit folder at @a sPath and installs it into the
	 * current song (Drumkit::load + setDrumkit). False without touching
	 * the song when the path does not load. */
	bool loadDrumkit( const QString& sPath );
	/** Opens the external editor — the landing view's button owns this
	 * (UI-5 retires the hosts' direct `show` path). */
	bool openEditor( bool bLaunchProcess = true );
	/** Applies the scalar MIDI control settings to the plugin's engine
	 * (runtime-only; never persisted back to the user config). */
	bool applyMidiControlSettings(
		bool bNoteOffIgnore,
		Midi::Channel actionChannel,
		bool bEnableFeedback,
		bool bTransportInputHandling,
		bool bTransportOutputSend,
		Midi::Channel feedbackChannel,
		Preferences::MidiSendNoteOff sendNoteOff );
	/** The plugin UI exposes only three of the MIDI control settings
	 * (feedback and transport stay engine-side, settable via the
	 * editor): this apply changes the exposed three and preserves the
	 * current engine values of the other four. */
	bool applyExposedMidiControlSettings(
		bool bNoteOffIgnore,
		Midi::Channel actionChannel,
		Preferences::MidiSendNoteOff sendNoteOff );
	/** Replaces the MIDI instrument map of the plugin's engine
	 * (runtime-only; never persisted back to the user config). */
	bool applyMidiInstrumentMap( std::shared_ptr<MidiInstrumentMap> pMap );

	// The read side for populating the exposed MIDI config widgets.
	// Each returns the engine's current value, or the engine default
	// when the engine is unavailable.
	bool midiNoteOffIgnore() const;
	Midi::Channel midiActionChannel() const;
	Preferences::MidiSendNoteOff midiSendNoteOff() const;

private:
	HydrogenPlugin* m_pPlugin = nullptr;

	/** The plugin engine's preferences (nullptr when the plugin or the
	 * engine is unavailable). */
	std::shared_ptr<Preferences> preferences() const;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_LANDING_MODEL_H
