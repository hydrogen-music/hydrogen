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

#include <core/IO/PulseAudioDriver.h>

#if defined(H2CORE_HAVE_PULSEAUDIO) || _DOXYGEN_

#include <fcntl.h>
#include <core/Preferences/Preferences.h>


namespace H2Core
{

PulseAudioDriver::PulseAudioDriver(audioProcessCallback processCallback)
	:	AudioDriver(),
		m_callback(processCallback),
		m_main_loop(nullptr),
		m_ctx(nullptr),
		m_stream(nullptr),
		m_connected(false),
		m_outL(nullptr),
		m_outR(nullptr)
{
	pthread_mutex_init(&m_mutex, nullptr);
	pthread_cond_init(&m_cond, nullptr);
}


PulseAudioDriver::~PulseAudioDriver()
{
	pthread_cond_destroy(&m_cond);
	pthread_mutex_destroy(&m_mutex);
	delete []m_outL;
	delete []m_outR;
}


int PulseAudioDriver::init( unsigned nBufferSize )
{
	delete []m_outL;
	delete []m_outR;
	m_buffer_size = nBufferSize;
	m_sample_rate = Preferences::get_instance()->m_nSampleRate;
	m_outL = new float[m_buffer_size];
	m_outR = new float[m_buffer_size];
	return 0;
}


int PulseAudioDriver::connect()
{
	if (m_connected) {
		ERRORLOG( "already connected" );
		return 1;
	}

	if (pipe(m_pipe)) {
		ERRORLOG( "unable to open pipe." );
		return 1;
	}

	fcntl(m_pipe[0], F_SETFL, fcntl(m_pipe[0], F_GETFL) | O_NONBLOCK);

	m_ready = 0;
	if (pthread_create(&m_thread, nullptr, s_thread_body, this))
	{
		close(m_pipe[0]);
		close(m_pipe[1]);
		ERRORLOG( "unable to start thread." );
		return 1;
	}

	pthread_mutex_lock(&m_mutex);
	while (!m_ready) {
		pthread_cond_wait(&m_cond, &m_mutex);
	}
	pthread_mutex_unlock(&m_mutex);

	if (m_ready < 0)
	{
		pthread_join(m_thread, nullptr);
		close(m_pipe[0]);
		close(m_pipe[1]);
		ERRORLOG( QString( "unable to run driver. Main loop returned %1" ).arg( m_ready ) );
		return 1;
	}

	m_connected = true;
	return 0;
}


void PulseAudioDriver::disconnect()
{
	if (m_connected)
	{
		int junk = 0;
		while (write(m_pipe[1], &junk, 1) != 1) {
			;
		}
		pthread_join(m_thread, nullptr);
		close(m_pipe[0]);
		close(m_pipe[1]);
	}
}


unsigned PulseAudioDriver::getBufferSize()
{
	return m_buffer_size;
}


unsigned PulseAudioDriver::getSampleRate()
{
	return m_sample_rate;
}


float* PulseAudioDriver::getOut_L()
{
	return m_outL;
}


float* PulseAudioDriver::getOut_R()
{
	return m_outR;
}

void* PulseAudioDriver::s_thread_body(void* arg)
{
	PulseAudioDriver* self = (PulseAudioDriver*)arg;
	int r = self->thread_body();
	if (r)
	{
		pthread_mutex_lock(&self->m_mutex);
		self->m_ready = -r;
		pthread_cond_signal(&self->m_cond);
		pthread_mutex_unlock(&self->m_mutex);
	}

	return nullptr;
}


int PulseAudioDriver::thread_body()
{
	m_main_loop = pa_mainloop_new();
	pa_mainloop_api* api = pa_mainloop_get_api(m_main_loop);
	pa_io_event* ioev = api->io_new(api, m_pipe[0], PA_IO_EVENT_INPUT,
			pipe_callback, this);
	m_ctx = pa_context_new(api, "Hydrogen");
	pa_context_set_state_callback(m_ctx, ctx_state_callback, this);
	pa_context_connect(m_ctx, nullptr, pa_context_flags_t(0), nullptr);

	int retval;
	pa_mainloop_run(m_main_loop, &retval);

	if (m_stream)
	{
		pa_stream_set_state_callback(m_stream, nullptr, nullptr);
		pa_stream_set_write_callback(m_stream, nullptr, nullptr);
		pa_stream_unref(m_stream);
		m_stream = nullptr;
	}

	api->io_free(ioev);
	pa_context_unref(m_ctx);
	pa_mainloop_free(m_main_loop);

	return retval;
}


void PulseAudioDriver::ctx_state_callback(pa_context* ctx, void* udata)
{
	PulseAudioDriver* self = (PulseAudioDriver*)udata;
	pa_context_state s = pa_context_get_state(ctx);

	if (s == PA_CONTEXT_READY) {
		pa_sample_spec spec;
		spec.format = PA_SAMPLE_S16LE;
		spec.rate = self->m_sample_rate;
		spec.channels = 2;
		self->m_stream = pa_stream_new(ctx, "Hydrogen", &spec, nullptr);
		pa_stream_set_state_callback(self->m_stream, stream_state_callback, self);
		pa_stream_set_write_callback(self->m_stream, stream_write_callback, self);
		pa_buffer_attr bufattr;
		bufattr.fragsize = (uint32_t)-1;
		bufattr.maxlength = self->m_buffer_size * 4;
		bufattr.minreq = 0;
		bufattr.prebuf = (uint32_t)-1;
		bufattr.tlength = self->m_buffer_size * 4;
		pa_stream_connect_playback(self->m_stream, nullptr, &bufattr, pa_stream_flags_t(0), nullptr, nullptr);
	}
	else if (s == PA_CONTEXT_FAILED) {
		pa_mainloop_quit(self->m_main_loop, 1);
	}
}


void PulseAudioDriver::stream_state_callback(pa_stream* stream, void* udata)
{
	PulseAudioDriver* self = (PulseAudioDriver*)udata;
	pa_stream_state s = pa_stream_get_state(stream);

	if ( s == PA_STREAM_FAILED ) {
		pa_mainloop_quit(self->m_main_loop, 1);
	} else if ( s == PA_STREAM_READY ) {
		pthread_mutex_lock(&self->m_mutex);
		self->m_ready = 1;
		pthread_cond_signal(&self->m_cond);
		pthread_mutex_unlock(&self->m_mutex);
	}
}

#define FLOAT_TO_SHORT(x) short(round((std::min(std::max((x), -1.0f), 1.0f)) * 32767.0f))

void PulseAudioDriver::stream_write_callback(pa_stream* stream, size_t bytes, void* udata)
{
	PulseAudioDriver* self = (PulseAudioDriver*)udata;

	void* vdata;
	pa_stream_begin_write(stream, &vdata, &bytes);
	if (!vdata) return;

	short* out = (short*)vdata;

	unsigned num_samples = bytes / 4;

	while (num_samples)
	{
		int n = std::min(self->m_buffer_size, num_samples);
		self->m_callback(n, nullptr);
		for (int i = 0; i < n; ++i)
		{
			*out++ = FLOAT_TO_SHORT(self->m_outL[i]);
			*out++ = FLOAT_TO_SHORT(self->m_outR[i]);
		}

		num_samples -= n;
	}

	pa_stream_write(stream, vdata, (bytes / 4) * 4, nullptr, 0, PA_SEEK_RELATIVE);
}


void PulseAudioDriver::pipe_callback(pa_mainloop_api*, pa_io_event*, int fd,
							pa_io_event_flags_t events, void *udata)
{
	if (!(events & PA_IO_EVENT_INPUT)) {
		return;
	}
	
	char buf[16];
	int bytes = read(fd, buf, 16);
	if ( bytes > 0 ) {
		PulseAudioDriver* self = (PulseAudioDriver*)udata;
		pa_mainloop_quit(self->m_main_loop, 0);
	}
}

QString PulseAudioDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[PulseAudioDriver]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_connected: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_connected ) )
			.append( QString( "%1%2m_ready: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_ready ) )
			.append( QString( "%1%2m_sample_rate: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_sample_rate ) )
			.append( QString( "%1%2m_buffer_size: %3\n" ).arg( sPrefix ).arg( s )
					 .arg( m_buffer_size ) );
	} else {
		sOutput = QString( "[PulseAudioDriver]" )
			.append( QString( " m_connected: %1" ).arg( m_connected ) )
			.append( QString( ", m_ready: %1" ).arg( m_ready ) )
			.append( QString( ", m_sample_rate: %1" ).arg( m_sample_rate ) )
			.append( QString( ", m_buffer_size: %1" ).arg( m_buffer_size ) );
	}

	return sOutput;
}

} //namespace H2Core

#endif //H2CORE_HAVE_PULSEAUDIO

