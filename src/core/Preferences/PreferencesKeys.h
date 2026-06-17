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

#ifndef H2C_PREFERENCES_KEYS_H
#define H2C_PREFERENCES_KEYS_H

namespace H2Core {

/**
 * Single source of truth for the XML element names of the Preferences config
 * file. Used both by Preferences (save/load) and by PluginConfig (the layered
 * config / merge paths, ADR 0022/0023), so the two cannot drift: renaming a key
 * here updates the writer/reader and the override-layer paths together.
 *
 * Only the keys that are referenced from more than one place (the document
 * structure and the host/state-owned "override" fields) are centralized here;
 * the many purely-local leaves stay as literals at their single Preferences
 * save/load site.
 */
namespace PreferencesKeys {

// ── Document structure ──
constexpr const char* Root = "hydrogen_preferences";
constexpr const char* AudioEngine = "audio_engine";
constexpr const char* Files = "files";
constexpr const char* RecentUsedSongs = "recentUsedSongs";

// ── Audio/MIDI driver sub-nodes (host-owned I/O) ──
constexpr const char* OssDriver = "oss_driver";
constexpr const char* PortAudioDriver = "portaudio_driver";
constexpr const char* CoreAudioDriver = "coreaudio_driver";
constexpr const char* AlsaAudioDriver = "alsa_audio_driver";
constexpr const char* JackDriver = "jack_driver";
constexpr const char* OscConfiguration = "osc_configuration";
constexpr const char* MidiDriver = "midi_driver";

// ── Override-layer leaf fields (host/state-owned) ──
constexpr const char* AudioDriver = "audio_driver";
constexpr const char* BufferSize = "buffer_size";
constexpr const char* SampleRate = "samplerate";
constexpr const char* MidiDriverName = "driverName";
constexpr const char* MidiPortName = "port_name";
constexpr const char* MidiOutputPortName = "output_port_name";
constexpr const char* LastSongFilename = "lastSongFilename";
constexpr const char* LastPlaylistFilename = "lastPlaylistFilename";

};

};

#endif
