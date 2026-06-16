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

#ifndef PLUGIN_MIDI_DRIVER_H
#define PLUGIN_MIDI_DRIVER_H

#include <core/IO/MidiBaseDriver.h>
#include <core/Midi/MidiMessage.h>

#include <vector>

namespace H2Core {

/**
 * Host-driven MIDI driver used when Hydrogen runs as a plugin (ADR 0013).
 *
 * Real-time MIDI drivers receive events on their own thread and feed them into
 * the base class's async input worker. A plugin instead receives MIDI as part
 * of each process block, each event carrying a sample offset within the block.
 * To keep rendering deterministic the host enqueues those events with
 * enqueueHostEvent() and the engine, at the start of the block, calls
 * dispatchHostEvents() — which runs them synchronously, in sample-offset order,
 * on the process thread so they take effect within the same block.
 *
 * \ingroup docCore docMIDI
 */
class PluginMidiDriver : public Object<PluginMidiDriver>, public MidiBaseDriver {
	H2_OBJECT( PluginMidiDriver )
public:
	PluginMidiDriver( Hydrogen* pHydrogen );
	virtual ~PluginMidiDriver();

	// MidiBaseDriver interface
	virtual void open() override;
	virtual void close() override;
	virtual std::vector<QString> getExternalPortList( const PortType& portType )
		override;

	// MidiInput / MidiOutput interface
	virtual bool isInputActive() const override;
	virtual bool isOutputActive() const override;

	/**
	 * Queue a host MIDI event for the current block.
	 * @param msg           The MIDI message.
	 * @param nSampleOffset Offset within the block, in frames (used to order
	 *                      events; sub-block scheduling lands with the
	 *                      host-transport follower, T3.3).
	 */
	void enqueueHostEvent( const MidiMessage& msg, int nSampleOffset = 0 );

	/** Process all queued host events synchronously, in sample-offset order,
	 * then clear the queue. Called at the start of the process block. */
	void dispatchHostEvents();

	/** Drop any queued-but-undispatched host events. */
	void clearHostEvents();

	size_t getHostEventCount() const;

	virtual QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

private:
	struct HostEvent {
		int nSampleOffset;
		MidiMessage msg;
	};

	std::vector<HostEvent> m_hostEvents;
	bool m_bActive;
};

};

#endif
