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

#ifndef H2C_LOCAL_ENGINE_ACCESS_H
#define H2C_LOCAL_ENGINE_ACCESS_H

#include <core/IEngineAccess.h>
#include <core/Hydrogen.h>

namespace H2Core {

/**
 * \ingroup docCore
 *
 * #IEngineAccess backed by a local, in-process #Hydrogen instance (ADR
 * 0015/0016) — the standalone application's engine handle. Every call forwards
 * to the wrapped instance; it does not own it. (Editor mode will provide a
 * second implementation backed by IPC; the GUI is agnostic between the two.)
 */
class LocalEngineAccess : public IEngineAccess {
public:
	explicit LocalEngineAccess( Hydrogen* pHydrogen ) : m_pHydrogen( pHydrogen ) {}
	~LocalEngineAccess() override = default;

	Hydrogen* getHydrogen() const { return m_pHydrogen; }

	AudioEngine* getAudioEngine() const override {
		return m_pHydrogen->getAudioEngine(); }
	std::shared_ptr<CoreActionController> getCoreActionController() const override {
		return m_pHydrogen->getCoreActionController(); }
	EventQueue* getEventQueue() const override {
		return m_pHydrogen->getEventQueue(); }
	std::shared_ptr<MidiActionManager> getMidiActionManager() const override {
		return m_pHydrogen->getMidiActionManager(); }
	std::shared_ptr<Playlist> getPlaylist() const override {
		return m_pHydrogen->getPlaylist(); }
	std::shared_ptr<Preferences> getPreferences() const override {
		return m_pHydrogen->getPreferences(); }
	std::shared_ptr<SoundLibraryDatabase> getSoundLibraryDatabase() const override {
		return m_pHydrogen->getSoundLibraryDatabase(); }
	std::shared_ptr<Song> getSong() const override {
		return m_pHydrogen->getSong(); }

	const Hydrogen::GUIState& getGUIState() const override {
		return m_pHydrogen->getGUIState(); }
	JackDriver::Timebase getJackTimebaseState() const override {
		return m_pHydrogen->getJackTimebaseState(); }
	Song::Mode getMode() const override {
		return m_pHydrogen->getMode(); }
	std::shared_ptr<Instrument> getSelectedInstrument() const override {
		return m_pHydrogen->getSelectedInstrument(); }
	int getSelectedInstrumentNumber() const override {
		return m_pHydrogen->getSelectedInstrumentNumber(); }
	int getSelectedPatternNumber() const override {
		return m_pHydrogen->getSelectedPatternNumber(); }
	bool hasJackDriver() const override {
		return m_pHydrogen->hasJackDriver(); }
	bool hasJackTransport() const override {
		return m_pHydrogen->hasJackTransport(); }
	bool isPatternEditorLocked() const override {
		return m_pHydrogen->isPatternEditorLocked(); }
	bool isUnderSessionManagement() const override {
		return m_pHydrogen->isUnderSessionManagement(); }

	// Audio-driver value views (ADR 0029). Defined out-of-line in
	// LocalEngineAccess.cpp: they read the live driver and so need the concrete
	// driver headers, which we keep out of this widely-included header.
	AudioDriverInfo getAudioDriverInfo() const override;
	int getAudioSampleRate() const override;
	int getAudioBufferSize() const override;
	int getAudioLatencyFrames() const override;
	int getAudioXRuns() const override;
	QStringList getAudioDevices(
		Preferences::AudioDriver kind, const QString& sHostAPI ) const override;
	QStringList getAudioHostAPIs() const override;
	bool isExportWritingFailed() const override;
	MidiDriverInfo getMidiDriverInfo() const override;
	std::vector<QString> getMidiPorts(
		MidiBaseDriver::PortType portType ) const override;
	std::vector<std::shared_ptr<MidiInput::HandledInput>>
		getHandledMidiInputs() const override;
	std::vector<std::shared_ptr<MidiOutput::HandledOutput>>
		getHandledMidiOutputs() const override;

	bool handleBeatCounter( TimePoint start = TimePoint() ) override {
		return m_pHydrogen->handleBeatCounter( start ); }
	void loadPlaybackTrack( const QString& sFileName ) override {
		m_pHydrogen->loadPlaybackTrack( sFileName ); }
	void onTapTempoAccelEvent( TimePoint start = TimePoint() ) override {
		m_pHydrogen->onTapTempoAccelEvent( start ); }
	void sequencerStop() override {
		m_pHydrogen->sequencerStop(); }
	void setDrumkitModified( bool bIsModified ) override {
		m_pHydrogen->setDrumkitModified( bIsModified ); }
	void setIsTimelineActivated( bool bEnabled ) override {
		m_pHydrogen->setIsTimelineActivated( bEnabled ); }
	void setPatternMode( const Song::PatternMode& mode ) override {
		m_pHydrogen->setPatternMode( mode ); }
	void setPatternModified( bool bIsModified, int nIndex ) override {
		m_pHydrogen->setPatternModified( bIsModified, nIndex ); }
	void setSelectedInstrumentNumber(
		int nInstrument,
		Event::Trigger trigger = Event::Trigger::Default ) override {
		m_pHydrogen->setSelectedInstrumentNumber( nInstrument, trigger ); }
	void setSongModified( bool bIsModified ) override {
		m_pHydrogen->setSongModified( bIsModified ); }
	void updateBeatCounterSettings() override {
		m_pHydrogen->updateBeatCounterSettings(); }

private:
	/** Wrapped engine; not owned. */
	Hydrogen* m_pHydrogen;
};

}

#endif
