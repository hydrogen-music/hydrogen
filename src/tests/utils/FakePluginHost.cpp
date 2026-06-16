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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "FakePluginHost.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/IO/PluginAudioDriver.h>
#include <core/IO/PluginMidiDriver.h>
#include <core/Preferences/Preferences.h>

#include <algorithm>
#include <cstring>

using namespace H2Core;

// Preferences for a headless, host-driven instance: the plugin audio and MIDI
// drivers, no OSC. create_instance() returns a freshly-owned object (ADR 0015).
static std::shared_ptr<Preferences> makeHostPreferences( unsigned nSampleRate,
														 unsigned nBlockSize ) {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Plugin;
	pPref->m_midiDriver = Preferences::MidiDriver::Plugin;
	pPref->m_nBufferSize = nBlockSize;
	pPref->m_nSampleRate = nSampleRate;
	pPref->setOscServerEnabled( false );
	return pPref;
}

FakePluginHost::FakePluginHost(unsigned nSampleRate, unsigned nBlockSize,
							   unsigned nBuses)
	: m_pHydrogen( nullptr )
	, m_pDriver( nullptr )
	, m_pMidiDriver( nullptr )
	, m_nSampleRate( nSampleRate )
	, m_nBlockSize( nBlockSize )
	, m_bPlaying( false )
	, m_fBpm( 120.0f )
	, m_nFramePosition( 0 )
	, m_pOutL( nullptr )
	, m_pOutR( nullptr )
	, m_nBuses( nBuses ) {

	m_pHydrogen = new Hydrogen( makeHostPreferences( nSampleRate, nBlockSize ), -1 );
	// Without a GUI the event queue drops events while in the startup state; a
	// headless instance keeps them so MIDI/transport tests can observe them.
	m_pHydrogen->setGUIState( Hydrogen::GUIState::headless );

	// The engine created a PluginAudioDriver from the preferences above; grab it
	// so we can point it at our host-owned buffers each block.
	m_pDriver = std::dynamic_pointer_cast<PluginAudioDriver>(
		m_pHydrogen->getAudioDriver() );
	if ( m_pDriver != nullptr ) {
		m_pDriver->setSampleRate( m_nSampleRate );
	}

	// The engine created a PluginMidiDriver from the preferences; grab it so we
	// can inject host MIDI events each block.
	m_pMidiDriver = std::dynamic_pointer_cast<PluginMidiDriver>(
		m_pHydrogen->getMidiDriver() );

	// The empty song is set directly in the Hydrogen constructor, which (unlike
	// setSong()) does not load the kit's samples. Load them so notes actually
	// render - a real plugin loads its song via setSong().
	if ( m_pHydrogen->getSong() != nullptr &&
		 m_pHydrogen->getSong()->getDrumkit() != nullptr ) {
		m_pHydrogen->getSong()->getDrumkit()->loadSamples(
			120, m_pHydrogen->getPreferences().get() );
	}

	allocateBuffers();
}

FakePluginHost::~FakePluginHost() {
	m_pMidiDriver.reset();
	m_pDriver.reset();
	delete m_pHydrogen;
	m_pHydrogen = nullptr;

	delete[] m_pOutL;
	delete[] m_pOutR;
	for ( auto p : m_busL ) { delete[] p; }
	for ( auto p : m_busR ) { delete[] p; }
}

void FakePluginHost::allocateBuffers() {
	delete[] m_pOutL;
	delete[] m_pOutR;
	m_pOutL = new float[ m_nBlockSize ];
	m_pOutR = new float[ m_nBlockSize ];
	std::fill( m_pOutL, m_pOutL + m_nBlockSize, 0.0f );
	std::fill( m_pOutR, m_pOutR + m_nBlockSize, 0.0f );

	for ( auto p : m_busL ) { delete[] p; }
	for ( auto p : m_busR ) { delete[] p; }
	m_busL.assign( m_nBuses, nullptr );
	m_busR.assign( m_nBuses, nullptr );
	for ( unsigned ii = 0; ii < m_nBuses; ++ii ) {
		m_busL[ ii ] = new float[ m_nBlockSize ];
		m_busR[ ii ] = new float[ m_nBlockSize ];
		std::fill( m_busL[ ii ], m_busL[ ii ] + m_nBlockSize, 0.0f );
		std::fill( m_busR[ ii ], m_busR[ ii ] + m_nBlockSize, 0.0f );
	}
}

void FakePluginHost::setSampleRate(unsigned nSampleRate) {
	m_nSampleRate = nSampleRate;
	if ( m_pDriver != nullptr ) {
		m_pDriver->setSampleRate( nSampleRate );
	}
}

void FakePluginHost::setBlockSize(unsigned nBlockSize) {
	if ( nBlockSize != m_nBlockSize ) {
		m_nBlockSize = nBlockSize;
		allocateBuffers();
	}
}

unsigned FakePluginHost::getSampleRate() const {
	return m_nSampleRate;
}

unsigned FakePluginHost::getBlockSize() const {
	return m_nBlockSize;
}

void FakePluginHost::setPlaying(bool bPlaying) {
	m_bPlaying = bPlaying;
	if ( m_pHydrogen == nullptr ) {
		return;
	}
	if ( bPlaying ) {
		m_pHydrogen->sequencerPlay();
	} else {
		m_pHydrogen->sequencerStop();
	}
}

bool FakePluginHost::isPlaying() const {
	return m_bPlaying;
}

void FakePluginHost::setBpm(float fBpm) {
	// In plugin mode the host owns tempo: it is published on the driver each
	// block (see process()) and the engine follows it (ADR 0013). We do not
	// poke setNextBpm() - that path is inert under a plugin host.
	m_fBpm = fBpm;
}

float FakePluginHost::getBpm() const {
	return m_fBpm;
}

void FakePluginHost::setFramePosition(long long nFrame) {
	// The new host frame is published on the driver at the next process() call;
	// the engine's transport follower relocates to it (ADR 0013, T3.3).
	m_nFramePosition = nFrame;
}

long long FakePluginHost::getFramePosition() const {
	return m_nFramePosition;
}

int FakePluginHost::process(unsigned nFrames) {
	// A real plugin host hands the engine fresh output buffers every block. We
	// must never ask the engine to render more frames than our buffers hold.
	if ( nFrames > m_nBlockSize ) {
		nFrames = m_nBlockSize;
	}

	// Point the driver at our host-owned buffers for this block; the engine
	// mixes straight into them via getOut_L()/getOut_R().
	if ( m_pDriver != nullptr ) {
		m_pDriver->setHostBuffers( m_pOutL, m_pOutR, nFrames );
		// Publish the host transport for this block so the engine follows it.
		m_pDriver->setHostTransport( m_bPlaying, m_fBpm, m_nFramePosition );
		// Publish the host's output bus buffers (engine zeroes + mixes them).
		m_pDriver->setBusBuffers( m_busL, m_busR );
	}

	// Inject queued host MIDI events synchronously before rendering so they take
	// effect within this block (in sample-offset order).
	if ( m_pMidiDriver != nullptr ) {
		m_pMidiDriver->dispatchHostEvents();
	}
	m_midiEvents.clear();

	int nResult =
		H2Core::AudioEngine::audioEngine_process( nFrames, m_pHydrogen );

	// The host clock advances only while rolling, exactly like a real host.
	if ( m_bPlaying ) {
		m_nFramePosition += static_cast<long long>( nFrames );
	}

	// Snapshot the rendered output (full block; frames beyond nFrames retain
	// whatever they held, which the caller can ignore).
	m_lastOutputL.assign( m_pOutL, m_pOutL + m_nBlockSize );
	m_lastOutputR.assign( m_pOutR, m_pOutR + m_nBlockSize );

	return nResult;
}

float* FakePluginHost::getOutputL() {
	return m_pOutL;
}

float* FakePluginHost::getOutputR() {
	return m_pOutR;
}

unsigned FakePluginHost::getBusCount() const {
	return m_nBuses;
}

float* FakePluginHost::getBusOutputL(unsigned nBus) {
	return nBus < m_busL.size() ? m_busL[ nBus ] : nullptr;
}

float* FakePluginHost::getBusOutputR(unsigned nBus) {
	return nBus < m_busR.size() ? m_busR[ nBus ] : nullptr;
}

const std::vector<float>& FakePluginHost::getLastOutputL() const {
	return m_lastOutputL;
}

const std::vector<float>& FakePluginHost::getLastOutputR() const {
	return m_lastOutputR;
}

void FakePluginHost::reset() {
	std::fill( m_pOutL, m_pOutL + m_nBlockSize, 0.0f );
	std::fill( m_pOutR, m_pOutR + m_nBlockSize, 0.0f );
}

void FakePluginHost::addMidiMessage(int nSampleOffset, const H2Core::MidiMessage& msg) {
	m_midiEvents.push_back( { nSampleOffset, msg } );
	if ( m_pMidiDriver != nullptr ) {
		m_pMidiDriver->enqueueHostEvent( msg, nSampleOffset );
	}
}

void FakePluginHost::addNoteOn(int nSampleOffset, int nKey, int nVelocity,
							   int nChannel) {
	addMidiMessage( nSampleOffset,
					MidiMessage( MidiMessage::Type::NoteOn,
								 Midi::parameterFromIntClamp( nKey ),
								 Midi::parameterFromIntClamp( nVelocity ),
								 static_cast<Midi::Channel>( nChannel ) ) );
}

void FakePluginHost::addNoteOff(int nSampleOffset, int nKey, int nChannel) {
	addMidiMessage( nSampleOffset,
					MidiMessage( MidiMessage::Type::NoteOff,
								 Midi::parameterFromIntClamp( nKey ),
								 Midi::ParameterMinimum,
								 static_cast<Midi::Channel>( nChannel ) ) );
}

void FakePluginHost::addControlChange(int nSampleOffset, int nParameter,
									  int nValue, int nChannel) {
	addMidiMessage( nSampleOffset,
					MidiMessage( MidiMessage::Type::ControlChange,
								 Midi::parameterFromIntClamp( nParameter ),
								 Midi::parameterFromIntClamp( nValue ),
								 static_cast<Midi::Channel>( nChannel ) ) );
}

const std::vector<FakePluginHost::MidiEvent>& FakePluginHost::getMidiEvents() const {
	return m_midiEvents;
}

H2Core::Hydrogen* FakePluginHost::getHydrogen() const {
	return m_pHydrogen;
}

H2Core::PluginMidiDriver* FakePluginHost::getMidiDriver() const {
	return m_pMidiDriver.get();
}
