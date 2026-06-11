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
#include <core/Hydrogen.h>

#include <algorithm>
#include <cstring>

FakePluginHost::FakePluginHost(unsigned nSampleRate, unsigned nBlockSize)
	: m_nSampleRate( nSampleRate )
	, m_nBlockSize( nBlockSize )
	, m_bPlaying( false )
	, m_fBpm( 120.0f )
	, m_nFramePosition( 0 )
	, m_pOutL( nullptr )
	, m_pOutR( nullptr ) {
	m_pOutL = new float[ m_nBlockSize ];
	m_pOutR = new float[ m_nBlockSize ];
	std::fill( m_pOutL, m_pOutL + m_nBlockSize, 0.0f );
	std::fill( m_pOutR, m_pOutR + m_nBlockSize, 0.0f );
}

FakePluginHost::~FakePluginHost() {
	delete[] m_pOutL;
	delete[] m_pOutR;
}

void FakePluginHost::setSampleRate(unsigned nSampleRate) {
	m_nSampleRate = nSampleRate;
}

void FakePluginHost::setBlockSize(unsigned nBlockSize) {
	// Reallocate buffers if size changed
	if ( nBlockSize != m_nBlockSize ) {
		delete[] m_pOutL;
		delete[] m_pOutR;
		m_nBlockSize = nBlockSize;
		m_pOutL = new float[ m_nBlockSize ];
		m_pOutR = new float[ m_nBlockSize ];
		std::fill( m_pOutL, m_pOutL + m_nBlockSize, 0.0f );
		std::fill( m_pOutR, m_pOutR + m_nBlockSize, 0.0f );
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
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen == nullptr ) {
		return;
	}
	if ( bPlaying ) {
		pHydrogen->sequencerPlay();
	} else {
		pHydrogen->sequencerStop();
	}
}

bool FakePluginHost::isPlaying() const {
	return m_bPlaying;
}

void FakePluginHost::setBpm(float fBpm) {
	m_fBpm = fBpm;
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	if ( pHydrogen != nullptr ) {
		pHydrogen->getAudioEngine()->setNextBpm( fBpm );
	}
}

float FakePluginHost::getBpm() const {
	return m_fBpm;
}

void FakePluginHost::setFramePosition(long long nFrame) {
	m_nFramePosition = nFrame;
	// Actual injection into AudioEngine's realtime frame will be wired in Phase 3
	// (plugin host seams). For now we track position locally.
}

long long FakePluginHost::getFramePosition() const {
	return m_nFramePosition;
}

int FakePluginHost::process(unsigned nFrames) {
	// Save current output before processing
	m_lastOutputL.assign( m_pOutL, m_pOutL + m_nBlockSize );
	m_lastOutputR.assign( m_pOutR, m_pOutR + m_nBlockSize );

	int nResult = H2Core::AudioEngine::audioEngine_process( nFrames, nullptr );

	m_nFramePosition += static_cast<long long>( nFrames );

	// Copy latest output after processing
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

void FakePluginHost::addMidiEvent(int nSampleOffset, std::shared_ptr<H2Core::Note> pNote) {
	m_midiEvents.push_back( { nSampleOffset, std::move(pNote) } );
}

const std::vector<FakePluginHost::MidiEvent>& FakePluginHost::getMidiEvents() const {
	return m_midiEvents;
}
