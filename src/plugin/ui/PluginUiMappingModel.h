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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see https://www.gnu.org/licenses
 *
 */

#ifndef PLUGIN_UI_MAPPING_MODEL_H
#define PLUGIN_UI_MAPPING_MODEL_H

#include <memory>

#include <QString>

#include <core/Basics/Instrument.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiInstrumentMap.h>

namespace H2Core {

class CoreActionController;
class HydrogenPlugin;

/** Headless state + command model behind the Instrument Mapping tab of
 * the plugin UI (#PluginUiWindow).
 *
 * The model owns no state of its own. Instead every setter reads the
 * live #MidiInstrumentMap of the plugin's engine, applies a single
 * change to a copy of it, and installs that copy with one
 * CoreActionController command — the same read-modify-persist flow the
 * MIDI instrument map dialog uses. This keeps every widget change
 * atomic and leaves the engine in charge of validation and event
 * emission.
 *
 * Per-instrument output note/channel are not part of the map; they
 * live on the #Instrument itself and are forwarded to the matching
 * CoreActionController commands. */
class PluginUiMappingModel
{
public:
	explicit PluginUiMappingModel( HydrogenPlugin* pPlugin );
	~PluginUiMappingModel();

	PluginUiMappingModel( const PluginUiMappingModel& ) = delete;
	PluginUiMappingModel& operator=( const PluginUiMappingModel& ) = delete;

	/** The live map of the plugin's engine — the read side for
	 * populating the view. */
	std::shared_ptr<MidiInstrumentMap> liveMap() const;

	bool setInputMode( MidiInstrumentMap::Input mode );
	bool setOutputMode( MidiInstrumentMap::Output mode );
	bool setUseGlobalInputChannel( bool bUse );
	bool setGlobalInputChannel( Midi::Channel channel );
	bool setUseGlobalOutputChannel( bool bUse );
	bool setGlobalOutputChannel( Midi::Channel channel );

	/** Adds (or replaces) the custom input mapping row of @a
	 * pInstrument. */
	bool insertCustomInputMapping( std::shared_ptr<Instrument> pInstrument,
								   Midi::Note note, Midi::Channel channel );
	/** Adds (or replaces) the custom input mapping row of the instrument
	 * at @a nInstrument in the current kit. */
	bool insertCustomInputMapping( int nInstrument, Midi::Note note,
								   Midi::Channel channel );

	/** Per-instrument output — forwarded to the instrument itself.
	 * @a nInstrument is the index within the current kit. */
	bool setInstrumentMidiOutNote( int nInstrument, Midi::Note note );
	bool setInstrumentMidiOutChannel( int nInstrument, Midi::Channel channel );

	// The read side for populating the per-instrument rows. Each
	// returns the engine's current value, or the documented degenerate
	// value when the engine is unavailable.
	/** Number of instruments in the current kit (0 when the engine is
	 *  unavailable). */
	int instrumentCount() const;
	/** The instrument's name (empty when the engine is unavailable or
	 *  @a nInstrument is out of range). */
	QString instrumentName( int nInstrument ) const;
	/** The instrument's output note (Midi::NoteDefault when the engine
	 *  is unavailable or @a nInstrument is out of range). */
	Midi::Note instrumentMidiOutNote( int nInstrument ) const;
	/** The instrument's output channel (Midi::ChannelOff when the
	 *  engine is unavailable or @a nInstrument is out of range). */
	Midi::Channel instrumentMidiOutChannel( int nInstrument ) const;
	/** The instrument's effective incoming note/channel — resolved
	 * through the engine's live map (custom row, global channel, or the
	 * mode's derivation); an invalid #MidiInstrumentMap::NoteRef when
	 * the engine is unavailable or @a nInstrument is out of range. */
	MidiInstrumentMap::NoteRef inputMapping( int nInstrument ) const;

private:
	HydrogenPlugin* m_pPlugin;

	/** The instrument at @a nInstrument in the plugin engine's current
	 * kit (nullptr when the engine is unavailable or the index is out
	 * of range). */
	std::shared_ptr<Instrument> instrument( int nInstrument ) const;

	/** The engine's command controller (nullptr when the model has no
	 * plugin or the engine is unavailable). */
	std::shared_ptr<CoreActionController> controller() const;
	/** Copy of the live map for read-modify-apply (nullptr when the
	 * engine is unavailable). */
	std::shared_ptr<MidiInstrumentMap> mapCopy() const;
	/** Installs @a pMap with one command; the engine adopts it. */
	bool applyMap( std::shared_ptr<MidiInstrumentMap> pMap );
};

} // namespace H2Core

#endif
