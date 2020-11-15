/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 *
 * http://hydrogen.sourceforge.net
 *
 * CoreAudio Driver for Hydrogen
 * Copyright(c) 2005 by Jonathan Dempsey [jonathandempsey@fastmail.fm]
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
 * Rewrote CoreAudio driver, now using AUHAL (2005/03/02 Jonathan Dempsey)
 * Cleaned up the code a bit and updated (2005/11/29 Jonathan Dempsey)
 * More cleaning . . . (2005/12/31 Jonathan Dempsey)
 */

#ifndef COREAUDIO_DRIVER_H
#define COREAUDIO_DRIVER_H

#include <core/IO/AudioOutput.h>
#include <core/IO/NullDriver.h>

#if defined(H2CORE_HAVE_COREAUDIO) || _DOXYGEN_
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioComponent.h>
#endif

#include <core/Preferences.h>
#include <inttypes.h>


typedef int ( *audioProcessCallback )( uint32_t, void * );

namespace H2Core
{

#if defined(H2CORE_HAVE_COREAUDIO) || _DOXYGEN_

class CoreAudioDriver : public AudioOutput
{
	H2_OBJECT
public:

	audioProcessCallback mProcessCallback;
	UInt32 m_nBufferSize;

	AudioUnit m_outputUnit;
	AudioDeviceID m_outputDevice;

	float* m_pOut_L;
	float* m_pOut_R;

	CoreAudioDriver( audioProcessCallback processCallback );
	virtual ~CoreAudioDriver();

	int init( unsigned bufferSize );

	unsigned getSampleRate();
	unsigned getBufferSize();

	int connect();
	void disconnect();

	float* getOut_L();
	float* getOut_R();


	virtual void play();
	virtual void stop();
	virtual void locate( unsigned long nFrame );
	virtual void updateTransportInfo();
	virtual void setBpm( float fBPM );



private:
	void retrieveDefaultDevice(void);
	void retrieveBufferSize(void);
	void printStreamInfo(void);


	bool m_bIsRunning;
	unsigned m_nSampleRate;
	unsigned oSampleRate;
};

#else

class CoreAudioDriver : public NullDriver
{
	H2_OBJECT
public:
	CoreAudioDriver( audioProcessCallback processCallback ) : NullDriver ( processCallback ) {}

};

#endif // H2CORE_HAVE_COREAUDIO

}

#endif
