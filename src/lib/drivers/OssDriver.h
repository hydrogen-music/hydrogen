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
 * $Id: OssDriver.h,v 1.6 2005/05/01 19:51:40 comix Exp $
 *
 */


#ifndef OSS_AUDIO_DRIVER_H
#define OSS_AUDIO_DRIVER_H

#include "config.h"
#include "GenericDriver.h"
#include "NullDriver.h"

// check if OSS support is enabled
#ifdef OSS_SUPPORT


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

#include "../Globals.h"

/*
#ifdef __NetBSD__
	#define AUDIO_DEVICE "/dev/audio"
#else
	#define AUDIO_DEVICE "/dev/dsp"
#endif
*/

typedef int  (*audioProcessCallback)(uint32_t, void *);

//----------------------------------------------------------------------------
/**
 * OSS Audio Driver
 */
class OssDriver : public GenericDriver {
	public:
		/** Constructor */
		OssDriver(audioProcessCallback processCallback);

		/** Destructor */
		~OssDriver();

		int init(unsigned bufferSize);
		int connect();
		void disconnect();

		void write();
		unsigned getBufferSize();
		unsigned getSampleRate();
		float* getOut_L();
		float* getOut_R();

		virtual void play();
		virtual void stop();
		virtual void locate( unsigned long nFrame );
		virtual void updateTransportInfo();
		virtual void setBpm(float fBPM);

	private:
		/** file descriptor, for writing to /dev/dsp */
		int fd;

		short* audioBuffer;
		float* out_L;
		float* out_R;

		audioProcessCallback processCallback;
		int log2(int n);

};

#else

class OssDriver : public NullDriver
{
	public:
		OssDriver(audioProcessCallback processCallback) : NullDriver( processCallback ) {}

};

#endif // OSS support

#endif
