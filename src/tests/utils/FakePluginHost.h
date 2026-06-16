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

#ifndef FAKE_PLUGIN_HOST_H
#define FAKE_PLUGIN_HOST_H

#include <core/Midi/Midi.h>
#include <core/Midi/MidiMessage.h>

#include <memory>
#include <vector>

namespace H2Core {
	class Hydrogen;
	class PluginAudioDriver;
	class PluginMidiDriver;
}

/**
 * Non-audio test harness for specifying engine/plugin behaviour in tests
 * before real plugins exist.
 *
 * Owns its own headless engine instance configured with the host-driven
 * PluginAudioDriver (ADR 0013): no real-time thread, host-owned buffers, the
 * host drives the process cycle synchronously. Each process() call points the
 * driver at the host's output buffers and runs audioEngine_process() directly,
 * so the engine mixes straight into them.
 *
 * Provides: output buffers, settable transport state, MIDI event list with
 * sample offsets, and configurable sample rate / block size.
 *
 * Grows through later phases (P3 host seams, P4 buses, etc).
 *
 * \ingroup docTests
 */
class FakePluginHost {
public:
    /** A host MIDI event: a message plus its sample offset within a block. */
    struct MidiEvent {
        int nSampleOffset;
        H2Core::MidiMessage msg;
    };

    /**
     * @param nSampleRate  Sample rate in Hz (default 44100)
     * @param nBlockSize   Block size in frames (default 1024)
     */
    FakePluginHost(unsigned nSampleRate = 44100, unsigned nBlockSize = 1024);
    ~FakePluginHost();

    // ── Configuration ──────────────────────────────────────────────
    void setSampleRate(unsigned nSampleRate);
    void setBlockSize(unsigned nBlockSize);
    unsigned getSampleRate() const;
    unsigned getBlockSize() const;

    // ── Transport state (settable for testing) ─────────────────────
    void setPlaying(bool bPlaying);
    bool isPlaying() const;
    void setBpm(float fBpm);
    float getBpm() const;
    void setFramePosition(long long nFrame);
    long long getFramePosition() const;

    // ── Process callback (synchronous, no threading) ───────────────
    /**
     * Call AudioEngine::audioEngine_process() directly.
     *
     * @param nFrames  Number of frames to process
     * @return         Return value from audioEngine_process()
     */
    int process(unsigned nFrames);

    // ── Output buffer access ───────────────────────────────────────
    float* getOutputL();
    float* getOutputR();

    /**
     * @return Copy of the output buffers from the most recent process() call.
     */
    const std::vector<float>& getLastOutputL() const;
    const std::vector<float>& getLastOutputR() const;

    // ── Reset ──────────────────────────────────────────────────────
    /** Zero all output buffers. */
    void reset();

    // ── MIDI events with sample offsets ────────────────────────────
    /**
     * Queue a host MIDI message for injection at a given sample offset. The
     * message is handed to the PluginMidiDriver and dispatched (in offset
     * order) at the start of the next process() block.
     */
    void addMidiMessage(int nSampleOffset, const H2Core::MidiMessage& msg);

    /** Convenience: queue a Note-On for the given key/velocity. */
    void addNoteOn(int nSampleOffset, int nKey, int nVelocity,
                   int nChannel = static_cast<int>( H2Core::Midi::ChannelDefault ));
    /** Convenience: queue a Note-Off for the given key. */
    void addNoteOff(int nSampleOffset, int nKey,
                    int nChannel = static_cast<int>( H2Core::Midi::ChannelDefault ));
    /** Convenience: queue a Control-Change message. */
    void addControlChange(int nSampleOffset, int nParameter, int nValue,
                          int nChannel = static_cast<int>( H2Core::Midi::ChannelDefault ));

    /** Events queued but not yet dispatched by a process() call. */
    const std::vector<MidiEvent>& getMidiEvents() const;

    // ── Engine access (for tests inspecting engine-side state) ─────
    H2Core::Hydrogen* getHydrogen() const;
    H2Core::PluginMidiDriver* getMidiDriver() const;

private:
    /** (Re)allocate the host output buffers to m_nBlockSize frames, zeroed. */
    void allocateBuffers();

    H2Core::Hydrogen* m_pHydrogen;
    std::shared_ptr<H2Core::PluginAudioDriver> m_pDriver;
    std::shared_ptr<H2Core::PluginMidiDriver> m_pMidiDriver;

    unsigned m_nSampleRate;
    unsigned m_nBlockSize;
    bool m_bPlaying;
    float m_fBpm;
    long long m_nFramePosition;

    float* m_pOutL;
    float* m_pOutR;
    std::vector<float> m_lastOutputL;
    std::vector<float> m_lastOutputR;

    std::vector<MidiEvent> m_midiEvents;
};

#endif
