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

#ifndef H2C_IPC_ENGINE_SESSION_H
#define H2C_IPC_ENGINE_SESSION_H

#include <core/Object.h>

#include <QtCore/QString>

#include <atomic>
#include <future>
#include <memory>

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

namespace H2Core {

class Hydrogen;
class IpcChannel;
class IpcMessage;
class EngineTelemetryShm;
struct EngineTelemetrySnapshot;

/**
 * \ingroup docCore
 *
 * Engine-process side of the editor↔engine split (ADR 0016/0018/0030) — the
 * counterpart to #EditorSession. It runs the **serve loop** on a dedicated bridge
 * thread (off the audio thread, ADR 0018): it listens on the control endpoint,
 * accepts the editor, and then for the life of the connection:
 *  - answers the `hello` handshake and sends the current song as the initial
 *    state snapshot,
 *  - drains the engine's #EventQueue and forwards engine-origin events to the
 *    editor (#IpcEngineBridge::forwardEvent),
 *  - dispatches inbound commands (#IpcEngineBridge::dispatchCommand) and answers
 *    request/response frames (#IpcEngineBridge::handleRequest).
 *
 * The engine keeps running if the editor disconnects: the loop simply returns to
 * accepting, so a respawned editor can re-attach. Does NOT own the #Hydrogen
 * engine. The #IpcServer and the accepted #IpcChannel live entirely on the bridge
 * thread (a QLocalSocket is thread-affine — its QSocketNotifier may only be pumped
 * on the Qt-managed thread that owns it), so nothing is shared across threads but
 * the engine itself (the EventQueue is single-consumer here).
 *
 * \note A QCoreApplication must exist in the process (Qt's local-socket classes
 *   require it). In a real plugin host the plugin bootstrap provides one.
 */
class EngineSession : public H2Core::Object<EngineSession> {
	H2_OBJECT( EngineSession )
public:
	/** Listen on @a sEndpoint and start serving @a pEngine on a bridge thread.
	 * Blocks up to @a nListenTimeoutMs for the listen to bind. Returns nullptr if
	 * @a pEngine is null, @a sEndpoint is empty, or the listen fails. */
	static std::unique_ptr<EngineSession> start(
		Hydrogen* pEngine, const QString& sEndpoint,
		int nListenTimeoutMs = 3000 );

	~EngineSession();

	const QString& getEndpoint() const { return m_sEndpoint; }
	bool isRunning() const { return m_bRunning.load(); }

	/** Stop the serve loop and join the bridge thread (idempotent). */
	void stop();

	/** Build a transport-only telemetry snapshot (frame / bpm / playing / tick)
	 * from @a pEngine. Peaks and full BBT are left at 0 here — they are the
	 * separate ADR 0018 metering concern; this carries what the editor mirror
	 * needs to follow the headless engine's playhead (ADR 0031). Static so it
	 * is unit-testable without a running serve loop. */
	static EngineTelemetrySnapshot buildTransportSnapshot( Hydrogen* pEngine );

private:
	EngineSession( Hydrogen* pEngine, const QString& sEndpoint );

	/** Bridge-thread entry point. Sets @a pListenResult to the listen outcome,
	 * then runs the accept + serve loop until stop(). */
	void serve( std::shared_ptr<std::promise<bool>> pListenResult );
	/** Apply one inbound frame: handshake reply / request reply / command. */
	void handleMessage( IpcChannel* pConn, const IpcMessage& msg );
	/** Forward all pending engine-origin events to the editor. */
	void forwardEvents( IpcChannel* pConn );
	/** Drain (and discard) the EventQueue while no editor is attached, so it does
	 * not overflow. */
	void discardEvents();
	/** Publish the engine's current transport into the telemetry block, if any. */
	void publishTelemetry();

	/** Authoritative engine being served; not owned. */
	Hydrogen* m_pEngine;
	QString m_sEndpoint;
	std::atomic<bool> m_bRunning;
	/** Bridge thread owning the IpcServer + accepted channel; owned. */
	QThread* m_pThread;
	/** Telemetry block written for an attached editor (ADR 0018/0031); created on
	 * the bridge thread, keyed off the endpoint. Owned. */
	std::unique_ptr<EngineTelemetryShm> m_pTelemetry;
	/** Poll/accept granularity; also bounds stop() latency. */
	int m_nPollTimeoutMs = 50;
};

}

#endif
