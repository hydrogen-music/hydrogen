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

#ifndef H2C_IENGINE_ACCESS_H
#define H2C_IENGINE_ACCESS_H

#include <memory>
#include <vector>

#include <QStringList>

#include <core/Basics/Event.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Helpers/Time.h>
#include <core/IO/JackDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/IO/MidiDriverInfo.h>

namespace H2Core {

class AudioDriver;
class AudioEngine;
class CoreActionController;
class EventQueue;
class Instrument;
class MidiActionManager;
class MidiBaseDriver;
class Playlist;
class Preferences;
class SoundLibraryDatabase;

/**
 * \ingroup docCore
 *
 * The surface through which the GUI reads engine state and issues commands
 * (ADR 0015/0016). The GUI holds an `IEngineAccess*` instead of poking a
 * concrete #Hydrogen, so the same GUI can be backed either by a local engine
 * (#LocalEngineAccess, standalone) or — in editor mode — by an IPC proxy to an
 * engine living in the plugin host process. This interface mirrors exactly the
 * #Hydrogen surface the GUI fans out from; commands flow through
 * #CoreActionController, events through #EventQueue, and live state through the
 * getters below.
 */
class IEngineAccess {
public:
	virtual ~IEngineAccess() = default;

	// --- owned services / handles the GUI fans out from ---
	virtual std::shared_ptr<AudioDriver> getAudioDriver() const = 0;
	virtual AudioEngine* getAudioEngine() const = 0;
	virtual std::shared_ptr<CoreActionController> getCoreActionController() const = 0;
	virtual EventQueue* getEventQueue() const = 0;
	virtual std::shared_ptr<MidiActionManager> getMidiActionManager() const = 0;
	// NB: the MIDI driver is NOT exposed as an object (ADR 0029). The GUI reads
	// it through the value accessors below (getMidiDriverInfo() et al.).
	virtual std::shared_ptr<Playlist> getPlaylist() const = 0;
	virtual std::shared_ptr<Preferences> getPreferences() const = 0;
	virtual std::shared_ptr<SoundLibraryDatabase> getSoundLibraryDatabase() const = 0;
	virtual std::shared_ptr<Song> getSong() const = 0;

	// --- live state reads ---
	virtual const Hydrogen::GUIState& getGUIState() const = 0;
	virtual JackDriver::Timebase getJackTimebaseState() const = 0;
	virtual Song::Mode getMode() const = 0;
	virtual std::shared_ptr<Instrument> getSelectedInstrument() const = 0;
	virtual int getSelectedInstrumentNumber() const = 0;
	virtual int getSelectedPatternNumber() const = 0;
	virtual bool hasJackDriver() const = 0;
	virtual bool hasJackTransport() const = 0;
	virtual bool isPatternEditorLocked() const = 0;
	virtual bool isUnderSessionManagement() const = 0;

	// --- MIDI driver: value views, never the driver object (ADR 0029) ---
	/** Presence / input+output activity of the active MIDI driver — replaces
	 * GUI-side `getMidiDriver()` null + isInputActive/isOutputActive probes. */
	virtual MidiDriverInfo getMidiDriverInfo() const = 0;
	/** External system MIDI ports of the given @a portType, empty when no driver
	 * is present (query — ADR 0029). */
	virtual std::vector<QString> getMidiPorts(
		MidiBaseDriver::PortType portType ) const = 0;
	/** Snapshot of the driver's recently handled input events (the MIDI-activity
	 * monitor), empty when no driver is present. */
	virtual std::vector<std::shared_ptr<MidiInput::HandledInput>>
		getHandledMidiInputs() const = 0;
	/** Snapshot of the driver's recently handled output events, empty when no
	 * driver is present. */
	virtual std::vector<std::shared_ptr<MidiOutput::HandledOutput>>
		getHandledMidiOutputs() const = 0;

	// --- commands / mutations ---
	virtual bool handleBeatCounter( TimePoint start = TimePoint() ) = 0;
	virtual void loadPlaybackTrack( const QString& sFileName ) = 0;
	virtual void onTapTempoAccelEvent( TimePoint start = TimePoint() ) = 0;
	virtual void sequencerStop() = 0;
	virtual void setDrumkitModified( bool bIsModified ) = 0;
	virtual void setIsTimelineActivated( bool bEnabled ) = 0;
	virtual void setPatternMode( const Song::PatternMode& mode ) = 0;
	virtual void setPatternModified( bool bIsModified, int nIndex ) = 0;
	virtual void setSelectedInstrumentNumber(
		int nInstrument, Event::Trigger trigger = Event::Trigger::Default ) = 0;
	virtual void setSongModified( bool bIsModified ) = 0;
	virtual void updateBeatCounterSettings() = 0;
};

}

#endif
