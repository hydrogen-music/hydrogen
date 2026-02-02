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
#ifndef H2_AUDIO_DRIVER_H
#define H2_AUDIO_DRIVER_H


#include <core/config.h>
#include <core/Object.h>

namespace H2Core
{

typedef int  ( *audioProcessCallback )( uint32_t, void * );

/** Base class for all our audio drivers.
 *
 * \ingroup docCore docAudioDriver */
class AudioDriver : public H2Core::Object<AudioDriver>
{
	H2_OBJECT(AudioDriver)
public:
	AudioDriver() = default;
	virtual ~AudioDriver() { }

	virtual int init( unsigned nBufferSize ) = 0;
	virtual int connect() = 0;
	virtual void disconnect() = 0;
	virtual unsigned getBufferSize() = 0;
	virtual unsigned getSampleRate() = 0;

	/** Approximate audio latency (in frames)
	 * A reasonable approximation is the buffer time on most audio systems.
	 * For systems with variable buffer sizes, this isn't very useful though
	 */
	virtual int getLatency()
	{
		return getBufferSize();
	}
	/** Get the number of XRuns that occurred since the audio driver
	 *	has started.
	 */
	virtual int getXRuns() const { return 0; }
	virtual float* getOut_L() = 0;
	virtual float* getOut_R() = 0;

	static QStringList getDevices() { return QStringList(); }
};

};

#endif

