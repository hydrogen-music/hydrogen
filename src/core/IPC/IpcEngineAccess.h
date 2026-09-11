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

#ifndef H2C_IPC_ENGINE_ACCESS_H
#define H2C_IPC_ENGINE_ACCESS_H

#include <core/Hydrogen.h>
#include <core/IEngineAccess.h>
#include <core/IPC/IpcCoreActionController.h>
#include <core/Object.h>

namespace H2Core {

class IpcChannel;

/**
 * \ingroup docCore
 *
 * #IEngineAccess for editor mode (ADR 0016): the GUI runs in a separate process
 * from the authoritative engine (which lives either in a plugin host or in its
 * own process as `h2player`). Reads are served from a local *headless*
 * #Hydrogen mirror — kept in sync by #EditorStateMirror from the inbound IPC
 * stream — so the GUI dereferences live local Song / Preferences / Playlist /
 * SoundLibraryDatabase objects exactly as in standalone. Transport commands are
 * forwarded over the #IpcChannel to the real engine; editor-local view state
 * (dirty flags, view modes) is applied directly to the mirror, while selection
 * and record state is applied to the mirror *and* forwarded when the engine
 * has a stake in it (see setSelectedInstrumentNumber()).
 *
 * Neither the mirror nor the channel is owned by this object.
 *
 * \note The bulk of GUI commands flow through getCoreActionController(); routing
 *   those through IPC requires an IPC-backed CoreActionController and is the next
 *   editor-mode sub-step. This class establishes the read mirror + the direct
 *   command surface (#IEngineAccess) over IPC.
 */
class IpcEngineAccess : public IEngineAccess,
						public H2Core::Object<IpcEngineAccess> {
	H2_OBJECT( IpcEngineAccess )
   public:
	IpcEngineAccess( Hydrogen* pMirror, IpcChannel* pChannel )
		: m_pMirror( pMirror ), m_pChannel( pChannel )
		, m_pController(
			std::make_shared<IpcCoreActionController>( pMirror, pChannel ) ) {}
	~IpcEngineAccess() override = default;

	Hydrogen* getMirror() const { return m_pMirror; }

	// --- reads: served from the local mirror ---
	AudioEngine* getAudioEngine() const override {
		return m_pMirror->getAudioEngine(); }
	std::shared_ptr<CoreActionController> getCoreActionController() const override {
		return m_pController; }
	EventQueue* getEventQueue() const override {
		return m_pMirror->getEventQueue(); }
	std::shared_ptr<MidiActionManager> getMidiActionManager() const override {
		return m_pMirror->getMidiActionManager(); }
	std::shared_ptr<Playlist> getPlaylist() const override {
		return m_pMirror->getPlaylist(); }
	std::shared_ptr<Preferences> getPreferences() const override {
		return m_pMirror->getPreferences(); }
	std::shared_ptr<SoundLibraryDatabase> getSoundLibraryDatabase() const override {
		return m_pMirror->getSoundLibraryDatabase(); }
	std::shared_ptr<Song> getSong() const override {
		return m_pMirror->getSong(); }

	const H2Core::ProcessMode& getProcessMode() const override {
		return m_pMirror->getProcessMode(); }
	bool isFullyOperational() const override {
		return m_pMirror->isFullyOperational(); }
	JackDriver::Timebase getJackTimebaseState() const override {
		return m_pMirror->getJackTimebaseState(); }
	Song::Mode getMode() const override {
		return m_pMirror->getMode(); }
	std::shared_ptr<Instrument> getSelectedInstrument() const override {
		return m_pMirror->getSelectedInstrument(); }
	/** Selection and record state is applied to the mirror by
	 * #EditorStateMirror from the forwarded event payloads (and refreshed by
	 * the periodic syncViaIpc pull), so all read flavors are served locally
	 * and agree with each other without a blocking IPC query. */
	int getSelectedInstrumentNumber() const override {
		return m_pMirror->getSelectedInstrumentNumber(); }
	int getSelectedPatternNumber() const override {
		return m_pMirror->getSelectedPatternNumber(); }
	bool hasJackDriver() const override {
		return m_pMirror->hasJackDriver(); }
	bool hasJackTransport() const override {
		return m_pMirror->hasJackTransport(); }
	bool isPatternEditorLocked() const override {
		return m_pMirror->isPatternEditorLocked(); }
	bool isUnderSessionManagement() const override {
		return m_pMirror->isUnderSessionManagement(); }
	QString getSessionFolderPath() const override;
	bool getRecordEnabled() const override {
		return m_pMirror->getRecordEnabled(); }

	// --- audio driver (ADR 0029) ---
	//
	// The headless engine owns audio I/O; in editor mode driver state crosses
	// as config (override layer), query, and event. The mirror's
	// AudioDriverInfo is populated via IPC (syncViaIpc + event-driven re-fetch
	// on AudioDriverChanged / JackTimebaseStateChanged) and cached in the
	// mirror Hydrogen; we serve reads from that cache.
	AudioDriverInfo getAudioDriverInfo() const override {
		return m_pMirror->getCachedAudioDriverInfo(); }
	/** The engine's actual rate (from the cached info). The mirror's clock
	 * driver is re-rated to it by Hydrogen::setCachedAudioDriverInfo(), so
	 * the mirror's own frame<->tick conversion agrees (ADR 0018/0029). */
	int getAudioSampleRate() const override {
		return m_pMirror->getCachedAudioDriverInfo().sampleRate; }
	/** For drivers with variable buffer size, like #CoreAudioDriver, the cached
	 * variable obtained using this call can be considered stale. But since the
	 * buffer size could change with every processing cycle and it is only used
	 * within #AudioEngineInfoForm within the GUI, we tolerate it. The engine's
	 * value is display-only — the mirror's own buffer size deliberately stays
	 * local (it just paces the mirror's clock loop). */
	int getAudioBufferSize() const override {
		return m_pMirror->getCachedAudioDriverInfo().bufferSize; }
	int getAudioLatencyFrames() const override {
		return m_pMirror->getCachedAudioDriverInfo().latencyFrames; }
	QStringList getAudioDevices(
		Preferences::AudioDriver kind, const QString& sHostAPI
	) const override;
	/** Host APIs and devices are enumerated by the engine's driver stack —
	 * the mirror owns no drivers — so these are blocking queries (ADR
	 * 0029). */
	QStringList getAudioHostAPIs() const override;
	/** Audio export using #DiskWriterDriver is only done in the mirror engine
	 * and not in the authoritative engine. */
	bool isExportWritingFailed() const override;
	// --- MIDI driver (ADR 0029) ---
	//
	// The headless engine owns MIDI I/O; in editor mode driver state crosses as
	// config (override layer), query, and event. The mirror's MidiDriverInfo is
	// populated via IPC (syncViaIpc + event-driven re-fetch on
	// MidiDriverChanged and cached in the mirror Hydrogen; we serve reads from
	// that cache.
	MidiDriverInfo getMidiDriverInfo() const override {
			return m_pMirror->getCachedMidiDriverInfo(); }
	// The mirror owns no MIDI driver (AudioEngine forces MidiDriver::None in
	// editor mode), so the entire MIDI read surface is served by blocking
	// queries against the authoritative engine (ADR 0029). A query is
	// FIFO-ordered after in-flight commands (e.g. log clears) and always
	// reflects the engine's current state.
	std::vector<QString> getMidiPorts(
		MidiBaseDriver::PortType portType ) const override;
	std::vector<std::shared_ptr<MidiInput::HandledInput>>
		getHandledMidiInputs() const override;
	std::vector<std::shared_ptr<MidiOutput::HandledOutput>>
		getHandledMidiOutputs() const override;
	int getOscTemporaryPort() const override;

	// --- commands: transport forwarded over IPC, view state applied locally ---
	bool handleBeatCounter( TimePoint start = TimePoint() ) override {
		return m_pMirror->handleBeatCounter( start ); }
	void loadPlaybackTrack( const QString& sFileName ) override {
		m_pMirror->loadPlaybackTrack( sFileName ); }
	void onTapTempoAccelEvent( TimePoint start = TimePoint() ) override {
		m_pMirror->onTapTempoAccelEvent( start ); }
	void sequencerPlay() override;
	void sequencerStop() override;
	void setDrumkitModified( bool bIsModified ) override {
		m_pMirror->setDrumkitModified( bIsModified ); }
	void setIsTimelineActivated( bool bEnabled ) override {
		m_pMirror->setIsTimelineActivated( bEnabled ); }
	void setPatternMode( const Song::PatternMode& mode ) override {
		m_pMirror->setPatternMode( mode ); }
	void setPatternModified( bool bIsModified, int nIndex ) override {
		m_pMirror->setPatternModified( bIsModified, nIndex ); }
	/** Instrument selection is engine-relevant — the headless engine's
	 * MIDI-to-selected-instrument routing follows it — so the change is
	 * forwarded over IPC and applied to the mirror. The engine's echo event
	 * re-applies the same value on the mirror (idempotent). */
	void setSelectedInstrumentNumber(
		int nInstrument,
		Event::Trigger trigger = Event::Trigger::Default ) override;
	void setSongModified( bool bIsModified ) override;
	void updateBeatCounterSettings() override {
		m_pMirror->updateBeatCounterSettings(); }

private:
	/** Editor-side headless engine serving reads; not owned. */
	Hydrogen* m_pMirror;
	/** Control channel to the authoritative engine; not owned. */
	IpcChannel* m_pChannel;
	/** Editor-mode command surface: forwards commands over IPC (ADR 0030). Owned
	 * here; returned (as the base type) from getCoreActionController(). */
	std::shared_ptr<CoreActionController> m_pController;
};
}

#endif
