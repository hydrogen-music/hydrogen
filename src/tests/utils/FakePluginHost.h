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

#include <core/Basics/Note.h>
#include <core/Midi/Midi.h>

#include <vector>

/**
 * Non-audio test harness for specifying engine/plugin behaviour in tests
 * before real plugins exist.
 *
 * Modelled on FakeAudioDriver but without threading or timing simulation.
 * Drives one engine instance's process callback directly and synchronously.
 * Provides: output buffers, settable transport state, MIDI event list with
 * sample offsets, and configurable sample rate / block size.
 *
 * Grows through later phases (P3 host seams, P4 buses, etc).
 *
 * \ingroup docTests
 */
class FakePluginHost {
public:
    /** MIDI event queued for later injection into the engine. */
    struct MidiEvent {
        int nSampleOffset;
        std::shared_ptr<H2Core::Note> pNote;
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
     * Queue a MIDI note for injection at a given sample offset.
     * (Phase 3 will wire these into the engine's MIDI input queue.)
     */
    void addMidiEvent(int nSampleOffset, std::shared_ptr<H2Core::Note> pNote);
    const std::vector<MidiEvent>& getMidiEvents() const;

private:
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
