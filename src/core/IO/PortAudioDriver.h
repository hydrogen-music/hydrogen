/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

class PortAudioDriver : public AudioOutput
{
	H2_OBJECT
public:
	audioProcessCallback m_processCallback;
	float* m_pOut_L;
	float* m_pOut_R;
	unsigned m_nBufferSize;

	PortAudioDriver( audioProcessCallback processCallback );
	virtual ~PortAudioDriver();

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
	PaStream *m_pStream;
	unsigned m_nSampleRate;

};

};

#else

namespace H2Core
{

class PortAudioDriver : public NullDriver
{
	H2_OBJECT
public:
	PortAudioDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

};

};

#endif


#endif

