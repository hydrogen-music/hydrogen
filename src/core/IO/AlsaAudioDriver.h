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
#ifndef ALSA_AUDIO_DRIVER_H
#define ALSA_AUDIO_DRIVER_H

#include <core/IO/AudioOutput.h>
#include <core/IO/NullDriver.h>

#if defined(H2CORE_HAVE_ALSA) || _DOXYGEN_

#include <inttypes.h>
#include <alsa/asoundlib.h>

namespace H2Core
{

/** \ingroup docCore docAudioDriver */
class AlsaAudioDriver : public Object<AlsaAudioDriver>, public AudioOutput
{
	H2_OBJECT(AlsaAudioDriver)
public:
	snd_pcm_t *m_pPlayback_handle;
	bool m_bIsRunning;
	unsigned long m_nBufferSize;
	float* m_pOut_L;
	float* m_pOut_R;
	QString m_sAlsaAudioDevice;
	audioProcessCallback m_processCallback;
	int m_nXRuns;

	AlsaAudioDriver( audioProcessCallback processCallback );
	~AlsaAudioDriver();

	virtual int init( unsigned nBufferSize ) override;
	virtual int connect() override;
	virtual void disconnect() override;
	virtual unsigned getBufferSize() override;
	virtual unsigned getSampleRate() override;
	virtual float* getOut_L() override;
	virtual float* getOut_R() override;
	static QStringList getDevices();

	virtual int getXRuns() const override { return m_nXRuns; }

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
private:

	unsigned int m_nSampleRate;
};

#else

namespace H2Core {

/** \ingroup docCore docAudioDriver */
class AlsaAudioDriver : public NullDriver
{
	H2_OBJECT(AlsaAudioDriver)
public:
	AlsaAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

};

#endif // H2CORE_HAVE_ALSA

};

#endif
