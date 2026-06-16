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

#ifndef PLUGIN_AUDIO_DRIVER_H
#define PLUGIN_AUDIO_DRIVER_H

#include <core/IO/AudioDriver.h>

#include <inttypes.h>

#include <vector>

namespace H2Core
{

/**
 * Passive audio driver used when Hydrogen runs as a plugin (ADR 0013).
 *
 * Unlike the real-time drivers (JACK, ALSA, …) this driver owns no thread and
 * allocates no buffers of its own. The host owns the audio buffers and the
 * process cycle: each block the host points the driver at its output buffers
 * via setHostBuffers() and then drives audioEngine_process() itself. The engine
 * mixes straight into the host buffers through getOut_L()/getOut_R().
 *
 * init()/connect()/disconnect() are no-ops: there is nothing to open, the host
 * is always "connected". Sample rate and block size are dictated by the host
 * and can change between blocks (setSampleRate(), setHostBuffers()).
 *
 * \ingroup docCore docAudioDriver
 */
class PluginAudioDriver : public Object<PluginAudioDriver>, public AudioDriver
{
	H2_OBJECT(PluginAudioDriver)
public:
	PluginAudioDriver( Hydrogen* pHydrogen, audioProcessCallback processCallback );
	~PluginAudioDriver();

	int init( unsigned nBufferSize ) override;
	int connect() override;
	void disconnect() override;
	unsigned getBufferSize() override;
	unsigned getSampleRate() override;

	float* getOut_L() override;
	float* getOut_R() override;

	/**
	 * Point the driver at the host-owned output buffers for the next block.
	 * The host must keep them valid for the duration of the process cycle.
	 * @param pOut_L    Left output buffer (host owned, at least nFrames long)
	 * @param pOut_R    Right output buffer (host owned, at least nFrames long)
	 * @param nFrames   Number of frames the host will request this block
	 */
	void setHostBuffers( float* pOut_L, float* pOut_R, unsigned nFrames );

	/** Update the sample rate the host runs at (may change between blocks). */
	void setSampleRate( unsigned nSampleRate );

	/** Transport state the host supplies alongside each process block (ADR
	 * 0013). The engine follows this instead of free-running / JACK timebase. */
	struct HostTransport {
		/** Whether the host transport is rolling this block. */
		bool bRolling = false;
		/** Host tempo in BPM. <= 0 means "host provides no tempo this block";
		 * the engine then keeps its current tempo. */
		double fBpm = 0.0;
		/** Host transport frame at the start of this block. A discontinuity
		 * versus the engine's playhead is treated as a relocate / host loop. */
		long long nFrame = 0;
		/** Set once the host has provided transport at least once. Until then
		 * the engine does not follow (it free-runs from its own state). */
		bool bValid = false;
	};

	/** Called by the host before each process block to publish its transport. */
	void setHostTransport( bool bRolling, double fBpm, long long nFrame );
	const HostTransport& getHostTransport() const;

	/**
	 * Publish the host's per-bus output buffers for this block (ADR 0019). Each
	 * bus is a stereo pair; the sampler mixes the instrument assigned to a bus
	 * into it (pre-fader). The master out (getOut_L/R) still carries the full
	 * post-fader sum. The two vectors must be the same length.
	 */
	void setBusBuffers( const std::vector<float*>& busOut_L,
						const std::vector<float*>& busOut_R );
	/** Number of output buses the host currently provides. */
	int getBusCount() const;
	/** Left/right buffer for bus @a nBus, or nullptr if out of range. */
	float* getBusBuffer_L( int nBus ) const;
	float* getBusBuffer_R( int nBus ) const;
	/** Zero every bus buffer for the first @a nFrames frames. */
	void clearBusBuffers( unsigned nFrames );

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:
	audioProcessCallback m_processCallback;
	unsigned m_nBufferSize;
	unsigned m_nSampleRate;
	float* m_pOut_L;
	float* m_pOut_R;
	HostTransport m_hostTransport;
	std::vector<float*> m_busOut_L;
	std::vector<float*> m_busOut_R;
};

};

#endif
