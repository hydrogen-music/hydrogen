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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef H2_SOFTWARE_DRIVER_H
#define H2_SOFTWARE_DRIVER_H

#include <core/Helpers/Time.h>
#include <core/IO/AudioDriver.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace H2Core {

/** Internally-clocked software audio driver (ADR 0031).
 *
 * Drives `audioEngine_process()` from a timer thread at `bufferSize/sampleRate`,
 * decoupling the engine **clock** from any real audio hardware. It always clocks
 * the engine; only its audio **output** is optional (#m_bProducesAudio):
 *  - `producesAudio == true`  — a self-clocked driver with scratch output buffers
 *    (the former `FakeAudioDriver`: profiling, unit tests).
 *  - `producesAudio == false` — a **headless** driver: the engine still advances
 *    transport, schedules notes and sends/handles MIDI, but no audio is delivered
 *    to a real sink (the audio-device-failure fallback, standalone MIDI-only mode,
 *    and the out-of-process editor mirror). The former inert `StubAudioDriver`.
 *
 * Even when headless it returns valid scratch buffers from #getOut_L()/#getOut_R()
 * so the render path stays uniform (render-to-scratch; a true silent-render that
 * skips the sample math is a later optimisation, ADR 0031).
 *
 * \ingroup docCore docAudioDriver docMIDI */
class SoftwareDriver : Object<SoftwareDriver>, public AudioDriver
{
	H2_OBJECT(SoftwareDriver)
public:
	/** @param bProducesAudio whether the rendered audio is meant for a real
	 * output sink (true) or discarded — headless (false). */
	SoftwareDriver( Hydrogen* pHydrogen, audioProcessCallback processCallback,
					bool bProducesAudio = true );
	~SoftwareDriver();

	virtual int init( unsigned nBufferSize ) override;
	virtual int connect() override;
	virtual void disconnect() override;
	virtual unsigned getBufferSize() override {
		return m_nBufferSize;
	}
	virtual unsigned getSampleRate() override;

	virtual float* getOut_L() override;
	virtual float* getOut_R() override;

	/** Whether this driver feeds a real audio output sink. False for the headless
	 * contexts above; surfaced as `AudioDriverInfo::isRunning` (ADR 0029). */
	bool getProducesAudio() const {
		return m_bProducesAudio;
	}

	/** AudioDriver capability hook (ADR 0031): headless ⇒ the Sampler takes the
	 * silent-render fast path (no sample interpolation / mixing). */
	bool producesAudio() const override {
		return m_bProducesAudio;
	}

	/** Toggle audio output on this running driver. The flag is read by the Sampler
	 * inside the (locked) process cycle, so call this with the audio engine locked
	 * (or stopped) to avoid racing it. */
	void setProducesAudio( bool bProducesAudio ) {
		m_bProducesAudio = bProducesAudio;
	}

	/** Stop the clock thread without tearing down the buffers. Used by unit tests
	 * that drive the process cycle synchronously (former
	 * `FakeAudioDriver::deactivate`). */
	void deactivate();

	const std::chrono::duration<float>& getProcessInterval() const;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	static void processCallback( void* pInstance );

	std::atomic<bool> m_bActive;
	std::shared_ptr< std::thread > m_pCallbackHandler;

	/** Time that needs to elapse between two runs of the process callback in
	 * order to clock the engine at the configured rate. */
	std::chrono::duration<float> m_processInterval;
	/** Point in time the last run of the process callback did finish. */
	TimePoint m_lastRun;

	audioProcessCallback m_processCallback;
	unsigned m_nBufferSize;
	unsigned m_nSampleRate;
	bool m_bProducesAudio;
	float* m_pOut_L;
	float* m_pOut_R;
};

inline const std::chrono::duration<float>& SoftwareDriver::getProcessInterval() const {
	return m_processInterval;
}

};

#endif
