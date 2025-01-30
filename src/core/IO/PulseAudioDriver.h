/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#ifndef H2_PULSE_AUDIO_DRIVER_H
#define H2_PULSE_AUDIO_DRIVER_H


#include <core/IO/AudioOutput.h>

#if defined(H2CORE_HAVE_PULSEAUDIO) || _DOXYGEN_

#include <pthread.h>
#include <inttypes.h>
#include <pulse/pulseaudio.h>

namespace H2Core
{


///
/// PulseAudio driver.
///
/** \ingroup docCore docAudioDriver */
class PulseAudioDriver : public Object<PulseAudioDriver>, public AudioOutput
{
	H2_OBJECT(PulseAudioDriver)
public:
	PulseAudioDriver(audioProcessCallback processCallback);
	~PulseAudioDriver();

	virtual int init( unsigned nBufferSize ) override;
	virtual int connect() override;
	virtual void disconnect() override;
	virtual unsigned getBufferSize() override;
	virtual unsigned getSampleRate() override;
	virtual float* getOut_L() override;
	virtual float* getOut_R() override;

private:
	pthread_t				m_thread;
	pthread_mutex_t			m_mutex;
	pthread_cond_t			m_cond;
	/** File descriptors used to write data to (m_pipe[1]) and read
		data from (m_pipe[0]) the pipe.*/
	int						m_pipe[2];
	audioProcessCallback	m_callback;
	pa_mainloop*			m_main_loop;
	pa_context*				m_ctx;
	pa_stream*				m_stream;
	bool					m_connected;
	int						m_ready;
	unsigned				m_sample_rate;
	unsigned				m_buffer_size;
	float*					m_outL;
	float*					m_outR;

	static void* s_thread_body(void*);
	int thread_body();

	static void ctx_state_callback(pa_context* ctx, void* udata);
	static void stream_state_callback(pa_stream* stream, void* udata);
	static void stream_write_callback(pa_stream* stream, size_t bytes, void* udata);
	static void pipe_callback(pa_mainloop_api*, pa_io_event*, int fd,
					pa_io_event_flags_t events, void *udata);
};

} //namespace H2Core

#else

#include <core/IO/NullDriver.h>

namespace H2Core {
/** \ingroup docCore docAudioDriver */
	class PulseAudioDriver : public NullDriver
	{
		H2_OBJECT(PulseAudioDriver)
	public:
		PulseAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

	};
}

#endif //H2CORE_HAVE_PULSEAUDIO


#endif //H2_PULSE_AUDIO_DRIVER_H


