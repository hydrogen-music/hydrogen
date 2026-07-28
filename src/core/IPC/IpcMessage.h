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

#include <core/Basics/Event.h>
#include <core/Object.h>

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
	Reply,                  ///< response to a request, correlated by requestId (ADR 0030)
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

	// ── ADR 0030 batch 2a: scalar Instrument/Component/Layer/strip/song setters ──
	SetInstrumentPitch,
	SetInstrumentGain,
	SetInstrumentRandomPitch,
	SetInstrumentFilterCutoff,
	SetInstrumentFilterResonance,
	SetInstrumentAttack,
	SetInstrumentDecay,
	SetInstrumentSustain,
	SetInstrumentRelease,
	SetInstrumentFilterActive,
	SetInstrumentMuteGroup,
	SetInstrumentStopNotes,
	SetInstrumentApplyVelocity,
	SetInstrumentHihatGroup,
	SetInstrumentLowerCc,
	SetInstrumentHigherCc,
	SetComponentIsMuted,
	SetComponentIsSoloed,
	SetComponentGain,
	SetComponentSelection,
	SetLayerIsMuted,
	SetLayerIsSoloed,
	SetLayerGain,
	SetLayerPitchOffset,
	SetLayerStartVelocity,
	SetLayerEndVelocity,
	SetStripIsMuted,
	SetStripIsSoloed,
	SetStripPanSym,
	SetHumanizeTime,
	SetHumanizeVelocity,
	SetSwing,
	SetPanLaw,
	SetPlaybackTrackMuted,
	SetPlaybackTrackVolume,

	// ── ADR 0030 batch 2b: simple transport/pattern/instrument/jack/midi commands ──
	PreviewInstrument,
	ActivateTimeline,
	ToggleTimeline,
	DeleteTempoMarker,
	DeleteTag,
	ActivateJackTransport,
	ToggleJackTransport,
	ActivateJackTimebaseControl,
	ToggleJackTimebaseControl,
	ToggleSongMode,
	ToggleLoopMode,
	MoveInstrument,
	RenameComponent,
	ToggleNextPattern,
	MovePattern,
	RemovePattern,
	SetPatternSize,
	StartCountIn,
	ActivatePlaylistSong,
	SetMidiClockInputHandling,
	SetMidiClockOutputSend,
	ClearMidiInputLog,
	ClearMidiOutputLog,

	// ── ADR 0030 batch 2c: note/grid edits (many-arg, enum + GridPoint) ──
	EditNoteProperty,
	ToggleGridCell,

	// ── ADR 0030 batch 2d: out-param commands ──
	// AddOrRemoveNote / HandleNote are dual-applied (out-param filled mirror-side).
	// SetInstrumentMidiOut* are request/response when the caller needs the
	// engine-assigned feedback-event id (else plain commands).
	AddOrRemoveNote,
	HandleNote,
	SetInstrumentMidiOutNote,
	SetInstrumentMidiOutChannel,

	// ── ADR 0030 batch 2e: object-payload / value-struct commands ──
	// (SetSong/SetDrumkit opcodes already exist above; SetSong carries the song
	// XML payload. The *Properties commands marshal their value-struct args —
	// strings/ints + a License [as string + holder] + a tags QStringList.)
	SetSongProperties,
	SetPatternProperties,

	// ── ADR 0030 batch 2f: object-payload (XML buffer) + file-save commands ──
	// SetDrumkit (above) / SetPattern / ReplaceInstrument carry the object XML in
	// the payload; AddInstrument is request/response when the caller needs the
	// engine-assigned event id (else a plain command). The Save* commands run
	// engine-side only (the engine owns the authoritative song/playlist and the
	// write to shared disk) — the editor mirror must NOT also write.
	SetPattern,             ///< args: [int nPatternNumber, bool bReplace]; payload: pattern XML
	ReplaceInstrument,      ///< args: [int nOldInstrumentId]; payload: new instrument XML
	AddInstrument,          ///< args: [int nIndex]; payload: instrument XML; reply: [qlonglong eventId]
	SaveSong,               ///< args: [bool bKeepMissingSamples]
	SaveSongAs,             ///< args: [QString sNewFileName, bool bKeepMissingSamples]
	SavePlaylist,           ///< (no args)
	SavePlaylistAs,         ///< args: [QString sPath]

	// ── ADR 0030 batch 2g: playlist commands ──
	// SetPlaylist carries the playlist XML in the payload; Add/RemoveFromPlaylist
	// marshal a single PlaylistEntry as its mime text (PlaylistEntry::toMimeText).
	SetPlaylist,            ///< payload: playlist XML
	AddToPlaylist,          ///< args: [QString entryMimeText, int nIndex]
	RemoveFromPlaylist,     ///< args: [QString entryMimeText, int nIndex]

	OpcodeCount
};
/**
 * Returns the human-readable name of an IPC opcode given its numeric value
 * (the raw \c quint16 that travels on the wire). Out-of-range values and the
 * \c OpcodeCount sentinel yield "Unknown IPC opcode".
 */
QString IpcOpcodeToQString( quint16 nOpcode );

/**
 * One framed IPC message. The wire encoding (ADR 0018) is a length-prefixed
 * frame: `[u32 bodyLength][u16 opcode][QVariantList args][QByteArray payload]`,
 * serialized with a pinned QDataStream version.
 *
 * \ingroup docCore
 */
class IpcMessage : public H2Core::Object<IpcMessage> {
	H2_OBJECT( IpcMessage )
public:
	IpcMessage() = default;
	explicit IpcMessage( IpcOpcode opcode ) : m_opcode( opcode ) {}

	IpcOpcode getOpcode() const { return m_opcode; }
	void setOpcode( IpcOpcode opcode ) { m_opcode = opcode; }

	const QVector<QVariant>& getArgs() const { return m_args; }
	IpcMessage& arg( const QVariant& value ) { m_args.append( value ); return *this; }

	/** Correlation id for request/response (ADR 0030 tier 3). 0 = a plain
	 * fire-and-forget command / event / a non-correlated message; a reply echoes
	 * the request's id. Set by IpcChannel::request(). */
	quint32 getRequestId() const { return m_requestId; }
	void setRequestId( quint32 nId ) { m_requestId = nId; }

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

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	IpcOpcode m_opcode = IpcOpcode::Hello;
	quint32 m_requestId = 0;
	QVector<QVariant> m_args;
	QByteArray m_payload;
};

/**
 * Incremental frame reader for a byte stream (e.g. QLocalSocket), which delivers
 * data in arbitrary chunks. Append received bytes and pull complete messages.
 */
class IpcFrameReader : public H2Core::Object<IpcFrameReader> {
	H2_OBJECT( IpcFrameReader )
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
