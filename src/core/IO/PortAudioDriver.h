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

#ifndef PORT_AUDIO_DRIVER_H
#define PORT_AUDIO_DRIVER_H

#include <core/IO/AudioOutput.h>
#include <core/IO/NullDriver.h>

#include <unistd.h>

#if defined(H2CORE_HAVE_PORTAUDIO) || _DOXYGEN_

#include <inttypes.h>
#include <portaudio.h>

namespace H2Core
{

/** \ingroup docCore docAudioDriver */
class PortAudioDriver : public Object<PortAudioDriver>, public AudioOutput
{
	H2_OBJECT(PortAudioDriver)
public:
	audioProcessCallback m_processCallback;
	float* m_pOut_L;
	float* m_pOut_R;

	PortAudioDriver( audioProcessCallback processCallback );
	virtual ~PortAudioDriver();

	virtual int init( unsigned nBufferSize ) override;
	virtual int connect() override;
	virtual void disconnect() override;
	virtual unsigned getBufferSize() override;
	virtual int getLatency() override;
	virtual unsigned getSampleRate() override;
	virtual float* getOut_L() override;
	virtual float* getOut_R() override;

	static QStringList getDevices();
	static QStringList getDevices( QString HostAPI );
	static QStringList getHostAPIs();

private:
	PaStream *m_pStream;
	unsigned m_nSampleRate;
	QString m_sDevice;

	static bool m_bInitialised;

};

};

#else

namespace H2Core
{

class PortAudioDriver : public NullDriver
{
	H2_OBJECT(PortAudioDriver)
public:
	PortAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}
};

};

#endif


#endif

