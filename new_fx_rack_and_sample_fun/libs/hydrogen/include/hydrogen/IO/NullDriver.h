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


#ifndef NULL_AUDIO_DRIVER_H
#define NULL_AUDIO_DRIVER_H

#include <hydrogen/IO/AudioOutput.h>

#include <inttypes.h>

namespace H2Core
{

typedef int  ( *audioProcessCallback )( uint32_t, void * );

class NullDriver : public AudioOutput
{
public:
	NullDriver( audioProcessCallback processCallback );
	~NullDriver();

	int init( unsigned nBufferSize );
	int connect();
	void disconnect();
	unsigned getBufferSize();
	unsigned getSampleRate();

	float* getOut_L();
	float* getOut_R();

	virtual void play();
	virtual void stop();
	virtual void locate( unsigned long nFrame );
	virtual void updateTransportInfo();
	virtual void setBpm( float fBPM );

};

};

#endif

