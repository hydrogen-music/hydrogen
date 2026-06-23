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

#include <core/IEngineAccess.h>
#include <core/Hydrogen.h>

namespace H2Core {

class IpcChannel;

/**
 * \ingroup docCore
 *
 * #IEngineAccess for editor mode (ADR 0016): the GUI runs in a separate process
 * from the authoritative engine (which lives in the plugin host). Reads are
 * served from a local *headless* #Hydrogen mirror — kept in sync by
 * #EditorStateMirror from the inbound IPC stream — so the GUI dereferences live
 * local Song / Preferences / Playlist / SoundLibraryDatabase objects exactly as
 * in standalone. Transport commands are forwarded over the #IpcChannel to the
 * real engine; editor-local view state (selection, dirty flags, view modes) is
 * applied directly to the mirror.
 *
 * Neither the mirror nor the channel is owned by this object.
 *
 * \note The bulk of GUI commands flow through getCoreActionController(); routing
 *   those through IPC requires an IPC-backed CoreActionController and is the next
 *   editor-mode sub-step. This class establishes the read mirror + the direct
 *   command surface (#IEngineAccess) over IPC.
 */
class IpcEngineAccess : public IEngineAccess {
public:
	IpcEngineAccess( Hydrogen* pMirror, IpcChannel* pChannel )
		: m_pMirror( pMirror ), m_pChannel( pChannel ) {}
	~IpcEngineAccess() override = default;

	Hydrogen* getMirror() const { return m_pMirror; }

	// --- reads: served from the local mirror ---
	std::shared_ptr<AudioDriver> getAudioDriver() const override {
		return m_pMirror->getAudioDriver(); }
	AudioEngine* getAudioEngine() const override {
		return m_pMirror->getAudioEngine(); }
	std::shared_ptr<CoreActionController> getCoreActionController() const override {
		return m_pMirror->getCoreActionController(); }
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

	const Hydrogen::GUIState& getGUIState() const override {
		return m_pMirror->getGUIState(); }
	JackDriver::Timebase getJackTimebaseState() const override {
		return m_pMirror->getJackTimebaseState(); }
	Song::Mode getMode() const override {
		return m_pMirror->getMode(); }
	std::shared_ptr<Instrument> getSelectedInstrument() const override {
		return m_pMirror->getSelectedInstrument(); }
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

	// MIDI driver (ADR 0029): same deferred query/event plumbing as audio.
	MidiDriverInfo getMidiDriverInfo() const override { return MidiDriverInfo(); }
	std::vector<QString> getMidiPorts(
		MidiBaseDriver::PortType /*portType*/ ) const override {
		return std::vector<QString>(); }
	std::vector<std::shared_ptr<MidiInput::HandledInput>>
		getHandledMidiInputs() const override {
		return std::vector<std::shared_ptr<MidiInput::HandledInput>>(); }
	std::vector<std::shared_ptr<MidiOutput::HandledOutput>>
		getHandledMidiOutputs() const override {
		return std::vector<std::shared_ptr<MidiOutput::HandledOutput>>(); }

	// --- commands: transport forwarded over IPC, view state applied locally ---
	bool handleBeatCounter( TimePoint start = TimePoint() ) override {
		return m_pMirror->handleBeatCounter( start ); }
	void loadPlaybackTrack( const QString& sFileName ) override {
		m_pMirror->loadPlaybackTrack( sFileName ); }
	void onTapTempoAccelEvent( TimePoint start = TimePoint() ) override {
		m_pMirror->onTapTempoAccelEvent( start ); }
	void sequencerStop() override;
	void setDrumkitModified( bool bIsModified ) override {
		m_pMirror->setDrumkitModified( bIsModified ); }
	void setIsTimelineActivated( bool bEnabled ) override {
		m_pMirror->setIsTimelineActivated( bEnabled ); }
	void setPatternMode( const Song::PatternMode& mode ) override {
		m_pMirror->setPatternMode( mode ); }
	void setPatternModified( bool bIsModified, int nIndex ) override {
		m_pMirror->setPatternModified( bIsModified, nIndex ); }
	void setSelectedInstrumentNumber(
		int nInstrument,
		Event::Trigger trigger = Event::Trigger::Default ) override {
		m_pMirror->setSelectedInstrumentNumber( nInstrument, trigger ); }
	void setSongModified( bool bIsModified ) override {
		m_pMirror->setSongModified( bIsModified ); }
	void updateBeatCounterSettings() override {
		m_pMirror->updateBeatCounterSettings(); }

private:
	/** Editor-side headless engine serving reads; not owned. */
	Hydrogen* m_pMirror;
	/** Control channel to the authoritative engine; not owned. */
	IpcChannel* m_pChannel;
};

}

#endif
