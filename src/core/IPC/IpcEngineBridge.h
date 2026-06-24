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

#ifndef H2C_IPC_ENGINE_BRIDGE_H
#define H2C_IPC_ENGINE_BRIDGE_H

#include <core/Object.h>
#include <core/Basics/Event.h>
#include <core/IPC/IpcMessage.h>

namespace H2Core {

class Hydrogen;
class IpcChannel;

/**
 * Engine side of the editor↔engine bridge (ADR 0018).
 *
 * - dispatchCommand() maps an editor-issued IpcMessage (a CoreActionController
 *   opcode + typed args / XML payload) onto the live engine, so the host-side
 *   engine performs the action the editor requested.
 * - forwardEvent() sends one engine-origin EventQueue event to the editor over
 *   the channel, dropping editor-internal events (isEngineOriginEvent). A bridge
 *   thread drains the EventQueue and calls this for each event, keeping the
 *   audio thread untouched.
 *
 * \ingroup docCore
 */
class IpcEngineBridge : public H2Core::Object<IpcEngineBridge> {
	H2_OBJECT(IpcEngineBridge)
public:
	/** Apply a command message to @a pHydrogen. Returns true if handled. */
	static bool dispatchCommand( const IpcMessage& msg, Hydrogen* pHydrogen );

	/** Apply a request message and build its reply (ADR 0030 tier 3). The
	 * returned message has opcode Reply, echoes @a msg's requestId, and carries
	 * the engine-computed result (e.g. a feedback-event id) in its args. Used by
	 * the engine-side IPC consumer for opcodes the editor sends via
	 * IpcChannel::request(). */
	static IpcMessage handleRequest( const IpcMessage& msg, Hydrogen* pHydrogen );

	/** Forward one event to the editor if it is engine-origin. Returns true if
	 * it was sent, false if it was editor-internal (and so not marshalled). */
	static bool forwardEvent( IpcChannel& channel, Event::Type type,
							  int nValue, long nId );
};

};

#endif
