/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: PortAudioDriver.h,v 1.7 2005/05/01 19:51:40 comix Exp $
 *
 */

#ifndef PORT_AUDIO_DRIVER_H
#define PORT_AUDIO_DRIVER_H

#include "config.h"
#include "GenericDriver.h"
#include "NullDriver.h"

#ifdef PORTAUDIO_SUPPORT

#include <inttypes.h>
#include <portaudio.h>

typedef int  (*audioProcessCallback)(uint32_t, void *);

class PortAudioDriver : public GenericDriver
{
	public:
		audioProcessCallback m_processCallback;
		float* m_pOut_L;
		float* m_pOut_R;
		unsigned m_nBufferSize;

		PortAudioDriver(audioProcessCallback processCallback);
		~PortAudioDriver();

		virtual int init(unsigned nBufferSize);
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
		virtual void setBpm(float fBPM);

	private:
		PortAudioStream *m_pStream;
		unsigned m_nSampleRate;

};

#else

class PortAudioDriver : public NullDriver
{
	public:
		PortAudioDriver(audioProcessCallback processCallback) : NullDriver( processCallback ) {}

};

#endif

#endif

