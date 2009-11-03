/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef ALSA_AUDIO_DRIVER_H
#define ALSA_AUDIO_DRIVER_H

#include <hydrogen/IO/AudioOutput.h>
#include <hydrogen/IO/NullDriver.h>

#ifdef ALSA_SUPPORT

#include <inttypes.h>
#include <alsa/asoundlib.h>

namespace H2Core
{

typedef int  ( *audioProcessCallback )( uint32_t, void * );

class AlsaAudioDriver : public AudioOutput
{
public:
	snd_pcm_t *m_pPlayback_handle;
	bool m_bIsRunning;
	unsigned long m_nBufferSize;
	float* m_pOut_L;
	float* m_pOut_R;
	int m_nXRuns;
	QString m_sAlsaAudioDevice;
	audioProcessCallback m_processCallback;

	AlsaAudioDriver( audioProcessCallback processCallback );
	~AlsaAudioDriver();

	virtual int init( unsigned nBufferSize );
	virtual int connect();
	virtual void disconnect();
	virtual unsigned getBufferSize();
	virtual unsigned getSampleRate();
	virtual float* getOut_L();
	virtual float* getOut_R();

	virtual void updateTransportInfo();
	virtual void play();
	virtual void stop();
	virtual void locate( unsigned long nFrame );
	virtual void setBpm( float fBPM );

private:

	unsigned int m_nSampleRate;
};

#else

namespace H2Core {

class AlsaAudioDriver : public NullDriver
{
public:
	AlsaAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

};

#endif // ALSA_SUPPORT

};

#endif
