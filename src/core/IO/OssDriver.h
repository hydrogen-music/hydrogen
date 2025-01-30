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

#ifndef OSS_AUDIO_DRIVER_H
#define OSS_AUDIO_DRIVER_H

#include <core/IO/AudioOutput.h>
#include <core/IO/NullDriver.h>

// check if OSS support is enabled
#if defined(H2CORE_HAVE_OSS) || _DOXYGEN_


#ifdef __NetBSD__
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <inttypes.h>

#include <core/Globals.h>

/*
#ifdef __NetBSD__
	#define AUDIO_DEVICE "/dev/audio"
#else
	#define AUDIO_DEVICE "/dev/dsp"
#endif
*/

namespace H2Core
{

///
/// OSS Audio Driver
///
/** \ingroup docCore docAudioDriver */
class OssDriver : public Object<OssDriver>, public AudioOutput
{
	H2_OBJECT(OssDriver)
public:
	OssDriver( audioProcessCallback processCallback );
	~OssDriver();

	int init( unsigned bufferSize );
	int connect();
	void disconnect();

	void write();
	unsigned getBufferSize();
	unsigned getSampleRate();
	float* getOut_L();
	float* getOut_R();

private:
	/** file descriptor, for writing to /dev/dsp */
	int fd;

	short* audioBuffer;
	float* out_L;
	float* out_R;

	audioProcessCallback processCallback;
	int log2( int n );

};

#else

namespace H2Core {

class OssDriver : public Object<OssDriver>, public NullDriver
{
	H2_OBJECT(OssDriver)
public:
	OssDriver( audioProcessCallback processCallback ) : NullDriver( processCallback ) {}

};



#endif // OSS support

};

#endif
