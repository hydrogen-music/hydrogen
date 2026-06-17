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

#ifndef H2C_IPC_MESSAGE_H
#define H2C_IPC_MESSAGE_H

#include <core/Object.h>
#include <core/Basics/Event.h>

#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QVariant>
#include <QtCore/QVector>

namespace H2Core {

/** Protocol version negotiated in the `hello` handshake (ADR 0018). A mismatch
 * fails gracefully (editor falls back to events-only / refuses to attach). */
constexpr quint16 IPC_PROTOCOL_VERSION = 1;

/** Pinned QDataStream version so the wire format is stable for a given build
 * (both ends are the same Hydrogen binary). */
constexpr int IPC_DATASTREAM_VERSION = QDataStream::Qt_5_15;

/**
 * Message opcodes (ADR 0018): the `hello` handshake, forwarded engine events,
 * the sound-library rescan command (ADR 0016), and the CoreActionController
 * command vocabulary (editor → engine). Each CoreActionController method maps to
 * one opcode; scalar arguments ride in IpcMessage::m_args, large structured
 * payloads (Song/Drumkit/state XML) in IpcMessage::m_payload.
 */
enum class IpcOpcode : quint16 {
	Hello = 0,
	Event,                  ///< engine → editor: (type, value, id)
	RescanSoundLibrary,     ///< editor → engine (ADR 0016)

	// ── CoreActionController commands (editor → engine) ──
	Play,
	Stop,
	Quit,
	SetBpm,                 ///< args: [float]
	SetMasterVolume,        ///< args: [float]
	SetMasterIsMuted,       ///< args: [bool]
	SetMetronomeIsActive,   ///< args: [bool]
	LocateToColumn,         ///< args: [int]
	LocateToTick,           ///< args: [qlonglong, bool]
	SelectPattern,          ///< args: [int]
	SetStripVolume,         ///< args: [int, float, bool]
	SetStripPan,            ///< args: [int, float, bool]
	ActivateLoopMode,       ///< args: [bool]
	ActivateSongMode,       ///< args: [bool]
	ActivateRecordMode,     ///< args: [bool]
	AddTempoMarker,         ///< args: [int, float]
	AddTag,                 ///< args: [int, QString]
	NewPattern,             ///< args: [QString]
	SetSong,                ///< payload: song XML
	SetDrumkit,             ///< payload: drumkit XML
	LoadState,              ///< payload: .h2project / song-only state

	OpcodeCount
};

/**
 * One framed IPC message. The wire encoding (ADR 0018) is a length-prefixed
 * frame: `[u32 bodyLength][u16 opcode][QVariantList args][QByteArray payload]`,
 * serialized with a pinned QDataStream version.
 *
 * \ingroup docCore
 */
class IpcMessage {
public:
	IpcMessage() = default;
	explicit IpcMessage( IpcOpcode opcode ) : m_opcode( opcode ) {}

	IpcOpcode getOpcode() const { return m_opcode; }
	void setOpcode( IpcOpcode opcode ) { m_opcode = opcode; }

	const QVector<QVariant>& getArgs() const { return m_args; }
	IpcMessage& arg( const QVariant& value ) { m_args.append( value ); return *this; }

	const QByteArray& getPayload() const { return m_payload; }
	void setPayload( const QByteArray& payload ) { m_payload = payload; }

	/** Encode to a complete length-prefixed wire frame. */
	QByteArray encode() const;

	/** Decode a single complete frame *body* (opcode + args + payload), i.e. the
	 * bytes after the u32 length prefix. Returns false on a malformed body. */
	static bool decodeBody( const QByteArray& body, IpcMessage& out );

	// ── Event convenience (ADR 0018: events are type + value + id) ──
	static IpcMessage fromEvent( Event::Type type, int nValue, long nId );
	/** Extract the (type, value, id) of an Event opcode message. */
	bool toEventFields( Event::Type& type, int& nValue, long& nId ) const;

	/** Hello handshake message carrying the protocol version. */
	static IpcMessage hello( quint16 nProtocolVersion = IPC_PROTOCOL_VERSION );
	/** Protocol version from a Hello message (0 if not a Hello). */
	quint16 helloProtocolVersion() const;

private:
	IpcOpcode m_opcode = IpcOpcode::Hello;
	QVector<QVariant> m_args;
	QByteArray m_payload;
};

/**
 * Incremental frame reader for a byte stream (e.g. QLocalSocket), which delivers
 * data in arbitrary chunks. Append received bytes and pull complete messages.
 */
class IpcFrameReader {
public:
	void append( const QByteArray& bytes ) { m_buffer.append( bytes ); }
	/** Pull the next complete message, if a full frame is buffered. */
	bool next( IpcMessage& out );
	int bufferedBytes() const { return m_buffer.size(); }

private:
	QByteArray m_buffer;
};

/**
 * Event classification (ADR 0016/0018): only *engine-origin* events are
 * marshalled engine → editor by the IPC bridge. *Editor-internal* events (which
 * originate and are consumed inside the editor process - e.g. OnlineImporter's
 * progress) must NOT cross IPC. Returns true for engine-origin events.
 */
bool isEngineOriginEvent( Event::Type type );

};

#endif
