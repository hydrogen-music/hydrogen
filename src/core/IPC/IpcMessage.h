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
#include <core/EventQueue.h>
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
	MidiNoteRecorded,       ///< engine → editor: recorded MIDI note
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
	SetMetronomeVolume,     ///< args: [float]
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
	AddAutomationPoint,     ///< args: [float, float]
	RemoveAutomationPoint,  ///< args: [float]
	SetSong,                ///< payload: song XML
	SetDrumkit,             ///< payload: drumkit XML
	LoadState,              ///< payload: .h2project / song-only state
	SetPreferences,         ///< payload: core-preferences XML (editor→engine)

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
	SetInstrumentMidiOutNote,
	SetInstrumentMidiOutChannel,
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
	RemoveNote,     ///< args: [Uuid, Uuid]
	ToggleGridCell,

	// ── ADR 0030 batch 2d: out-param commands ──
	// AddOrRemoveNote / HandleNote are dual-applied (out-param filled mirror-side).
	AddOrRemoveNote,
	HandleNote,

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
	AddInstrument,          ///< args: [int nIndex, long nEventId]; payload: instrument XML;
	RemoveInstrument,       ///< args: [long nEventId]; payload: instrument XML;
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

	// ── ADR 0030 batch 2h: MIDI action map ──
	// The editor's MIDI action table is the sole writer of the map; the
	// whole map travels as XML so the authoritative engine's MIDI dispatch
	// sees the same bindings.
	SetMidiEventMap,        ///< payload: MIDI event map XML (editor→engine)

	// ── ADR 0030 batch 2i: MIDI instrument map ──
	// Same whole-map payload shape as batch 2h, for the instrument
	// note/channel mapping edited in the MIDI control dialog.
	SetMidiInstrumentMap,   ///< payload: MIDI instrument map XML (editor→engine)

	// ── State-sync requests (editor → engine, ADR 0032) ──
	// Pull-based full-state sync: the editor sends these on connect/reconnect
	// to mirror the headless engine's authoritative state. Each is a
	// request/response opcode (IpcChannel::request()); the reply carries the
	// result in args or payload.
	GetSong,                ///< reply: payload = song XML
	GetPlaylist,            ///< reply: payload = playlist XML (empty if none)
	GetSelectedPattern,     ///< reply: args = [int]
	GetSelectedInstrument,  ///< reply: args = [int]
	GetRecordEnabled,       ///< reply: args = [bool]
	GetCorePreferences,     ///< reply: payload = core-preferences XML
	GetSoundLibraryInfo,    ///< reply: args = [QStringList drumkitFolders,
	                        ///<         QStringList customDrumkitFolders,
	                        ///<         QStringList customDrumkitPaths]
	GetAudioDriverInfo,    ///< reply: args = [int kind, bool isPresent,
	                        ///<         bool isRunning, QString connectedDevice,
	                        ///<         int timebaseState,
	                        ///<         bool jackTransportEnabled,
	                        ///<         int sampleRate, int bufferSize,
	                        ///<         int latencyFrames]
	GetMidiDriverInfo,    ///< reply: args = [bool isPresent, bool isInputActive,
						  ///<                bool isOutputActive]
	GetIsUnderSessionManagement, ///< reply: args = [bool]
	GetIsUnderPluginHost,       ///< reply: args = [bool]

	// ── Panic (stop all playback + send all-notes-off) ──
	Panic,

	// ── Note preview (trigger a note for immediate playback) ──
	// The note rides as an XML-buffer payload (Note::toXmlBuffer with
	// Xml::Flag::Ipc set). The engine deserializes it, resolves the instrument
	// from its drumkit, and calls Sampler::noteOn().
	NoteOn,

	// ── Stop rendering notes currently playing in the Sampler ──
	// All note corresponding to a particular instrument in the Sampler will be
	// released. The instrument is identified by its Uuid which is serialized
	// and transmitted to the authoritative engine.
	ReleasePlayingNotes,

	// ── Selection sync (editor → engine, ADR 0018) ──
	// The engine's MIDI-to-selected-instrument routing follows the selection,
	// so editor-side changes are forwarded as a plain command (fire-and-forget
	// with local apply; the engine's echo event confirms on the mirror).
	// Appended at the enum tail on purpose: opcodes cross the wire as raw
	// u16, so inserting mid-enum would renumber every later opcode and silently
	// break old peers that passed the (unchanged) protocol-version handshake.
	SetSelectedInstrument,  ///< args: [int nInstrument]

	// ── Driver enumeration queries (editor → engine, ADR 0029) ──
	// The engine owns the audio/MIDI driver stacks, so port and device
	// enumeration can only be served there — the mirror owns no drivers
	// (AudioEngine forces MidiDriver::None in editor mode). These are
	// blocking queries rather than mirror reads or event feeds: the GUI
	// re-queries on dialog open / driver events, and a query gets FIFO
	// ordering after in-flight commands (e.g. log clears) plus attach-time
	// freshness for free. Appended at the enum tail for the same
	// wire-compatibility reason as SetSelectedInstrument above.
	GetMidiPorts,           ///< args: [int portType]; reply: args = [QStringList]
	GetHandledMidiInputs,   ///< reply: args = [int count, then per entry:
	                        ///< qint64 timeEpoch, int type, int data1,
	                        ///< int data2, int channel,
	                        ///< QVariantList actionTypes (ints),
	                        ///< QStringList mappedInstruments]
	GetHandledMidiOutputs,  ///< reply: args = [int count, then per entry:
	                        ///< qint64 timeEpoch, int type, int data1,
	                        ///< int data2, int channel]
	GetAudioHostAPIs,       ///< reply: args = [QStringList]
	GetAudioDevices,        ///< args: [int kind, QString hostAPI]; reply:
	                        ///< args = [QStringList]

	// ── OSC server: restart command + fallback-port query ──
	// The engine owns the OSC server in the editor split (ADR 0026
	// amendment): the dialog-facing restart is forwarded as a command, and
	// the fallback port — the one a remote control surface must dial — is
	// served as a blocking query like the driver enumerations above.
	// Appended at the enum tail for the same wire-compatibility reason.
	RecreateOscServer,      ///< command: restart the engine's OSC server
	                        ///< with the current OSC configuration
	                        ///< (editor→engine, ADR 0030)
	GetOscTemporaryPort,    ///< reply: args = [int temporaryPort]; -1: no
	                        ///< fallback port in effect

	// ── NSM session state: dirty-state command + session-folder query ──
	// Only the engine's NsmClient talks to the session manager (ADR
	// 0016/0026). Modifications happen in the editor, so the dirty-state
	// flip is forwarded as a command (the engine's client reports it to
	// the session manager); the session folder — needed by the editor's
	// drumkit-export path — is served as a blocking query like the OSC
	// port above. Appended at the enum tail for the same
	// wire-compatibility reason.
	SetSongModified,        ///< command: args = [bool isModified]
	                        ///< (editor→engine, ADR 0030)
	GetSessionFolderPath,   ///< reply: args = [QString folder]; empty:
	                        ///< no NSM session in effect
	// The file browser, sound library, and sample editor audition ad-hoc
	// instruments that are not part of the current song's kit — the
	// number-based PreviewInstrument command can not address them, and the
	// mirror can not render audio anyway. The instrument crosses as an XML
	// payload (samples referenced by shared-disk paths, reloaded by the
	// engine), the note as an XML arg. Appended at the enum tail for the
	// same wire-compatibility reason.
	PreviewInstrumentSerialized, ///< command: payload = instrument XML;
	                             ///< args = [note XML] (editor→engine)

	// ── Engine-access commands without a CoreActionController surface ──
	// Beat counter/tap tempo, timeline activation, pattern mode, and the
	// playback track are Hydrogen-level commands the editor issues via
	// IEngineAccess. Taps are engine-authoritative — the mirror's
	// handlers are designed no-ops in editor mode (getTempoSource() ==
	// Tempo::Remote) — so they cross as fire-and-forget commands with
	// absolute timestamps; engine and editor share the host clock, so
	// the epoch count reconstructs the TimePoint losslessly (ADR 0026
	// point 12). updateBeatCounterSettings crosses as a config snapshot
	// of the editor's current state: the engine's preferences copy goes
	// stale between syncs and its TapAndPlay completion branch reads the
	// mode from it. Appended at the enum tail for the same
	// wire-compatibility reason.
	HandleBeatCounter,        ///< command: args = [qint64 timeSinceEpochNs]
	                          ///< (editor→engine)
	TapTempoAccelEvent,       ///< command: args = [qint64 timeSinceEpochNs]
	                          ///< (editor→engine)
	UpdateBeatCounterSettings, ///< command: args = [float beatLength,
	                          ///< int totalBeats, int driftCompensation,
	                          ///< int startOffset, int beatCounterMode]
	                          ///< (editor→engine)
	SetIsTimelineActivated,   ///< command: args = [bool enabled]
	                          ///< (editor→engine)
	SetPatternMode,           ///< command: args = [int patternMode]
	                          ///< (editor→engine)
	LoadPlaybackTrack,        ///< command: args = [QString fileName]
	                          ///< (editor→engine)

	// ── Class C song-state commands (editor → engine, ADR 0026 point 13) ──
	// The drumkit/pattern modified flags and the pattern-editor lock are
	// song state the GUI writes through its engine-access handle:
	// dual-apply (mirror + forward), with the engine-side apply under
	// Suppress so no SongIsModified echo crosses. Appended at the enum
	// tail for the same wire-compatibility reason.
	SetDrumkitModified,       ///< command: args = [bool isModified]
	                          ///< (editor→engine)
	SetPatternModified,       ///< command: args = [bool isModified,
	                          ///< int patternIndex] (editor→engine)
	SetPlaylistIsModified,    ///< command: args = [bool isModified]
	                          ///< (editor→engine)
	SetIsPatternEditorLocked, ///< command: args = [bool isLocked]
	                          ///< (editor→engine)

	// ── MIDI-learn channel (editor → engine, ADR 0030 batch 2j) ──
	// The engine's MIDI input is the only writer of the last-event
	// pair; the editor resets it before opening the sense dialog
	// (command) and polls it while listening (blocking query — the
	// mirror owns no MIDI input). The query returns the pair in one
	// reply so the snapshot can not tear across two separate queries.
	// Appended at the enum tail for the same wire-compatibility
	// reason.
	SetLastMidiEvent,        ///< command: args = [int type, int parameter]
	                          ///< (editor→engine)
	GetLastMidiEvent,        ///< reply: args = [int type, int parameter]

	// ── Custom sound library dirs (editor → engine, ADR 0030 batch 2k) ──
	// The dirs are engine-consumed state: the database scans them on
	// every update. Add/remove cross as commands so the authoritative
	// engine's preferences AND its database rescan see them — a plain
	// SetPreferences sync would miss the rescan. The engine's
	// SoundLibraryChanged echo refreshes the editor's sound library
	// idempotently. Appended at the enum tail for the same
	// wire-compatibility reason.
	AddCustomSoundLibraryDir,   ///< command: args = [QString dirPath]
	                            ///< (editor→engine)
	RemoveCustomSoundLibraryDir, ///< command: args = [QString dirPath]
	                            ///< (editor→engine)

	// ── Song export (editor → engine, ADR 0030 batch 2l) ──
	// The render pipeline only runs in the authoritative engine, so
	// the whole export plan crosses as one request: the engine arms
	// the session synchronously and renders on a background thread (a
	// synchronous render would starve the serve loop and deadlock the
	// reply). The stop is a fire-and-forget command; the writer's
	// failure state is a query.
	ExportSong,               ///< request: args = [int sampleRate,
	                          ///< int sampleDepth, double compressionLevel,
	                          ///< int interpolateMode, bool rubberbandBatch,
	                          ///< int renderCount, per render: QString fileName
	                          ///< + QStringList excluded instrument ids]
	                          ///< (editor→engine); reply: args = [bool success]
	StopExportSession,        ///< command (editor→engine)
	GetExportWritingFailed,   ///< query; reply: args = [bool failed]

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

	// ── Recorded MIDI note convenience (ADR 0030 batch 2n) ──
	/** Marshal an EventQueue::AddMidiNoteVector entry the engine's
	 * Hydrogen::addRealtimeNote() queued, so the editor's mirror can
	 * re-queue it for HydrogenApp::onEventQueueTimer (which integrates it
	 * as an undoable pattern edit). */
	static IpcMessage fromMidiNote(
		const EventQueue::AddMidiNoteVector& noteAction );
	/** Extract the AddMidiNoteVector fields of a MidiNoteRecorded
	 * message. */
	bool toMidiNoteFields( EventQueue::AddMidiNoteVector& noteAction ) const;

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
