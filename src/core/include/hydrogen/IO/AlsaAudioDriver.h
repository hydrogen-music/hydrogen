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

#if defined(H2CORE_HAVE_ALSA) || _DOXYGEN_

#include <inttypes.h>
#include <alsa/asoundlib.h>

namespace H2Core
{

typedef int  ( *audioProcessCallback )( uint32_t, void * );

/** \ingroup docCore docAudioDriver */
class AlsaAudioDriver : public AudioOutput
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
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
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;

	unsigned int m_nSampleRate;
};

#else

namespace H2Core {

/** \ingroup docCore docAudioDriver */
class AlsaAudioDriver : public NullDriver
{
public:
	/** \return #m_sClassName*/
	static const char* className() { return m_sClassName; }
	AlsaAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}
private:
	/** Contains the name of the class.
	 *
	 * This variable allows from more informative log messages
	 * with the name of the class the message is generated in
	 * being displayed as well. Queried using className().*/
	static const char* m_sClassName;

};

#endif // H2CORE_HAVE_ALSA

};

#endif
