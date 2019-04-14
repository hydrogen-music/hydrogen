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
#ifndef H2_AUDIO_DRIVER_H
#define H2_AUDIO_DRIVER_H


#include "hydrogen/config.h"
#include <hydrogen/object.h>
#include <hydrogen/IO/TransportInfo.h>

namespace H2Core
{

///
/// Base abstract class for audio driver classes.
///
class AudioDriver : public H2Core::Object
{
public:
	/** Local instance of the TransportInfo. */
	TransportInfo m_transport;

	AudioDriver( const char* class_name )
			: Object( class_name )
			, __track_out_enabled( false ) { }

	virtual ~AudioDriver() { }

	virtual int init( unsigned nBufferSize ) = 0;
	virtual int connect() = 0;
	virtual void disconnect() = 0;
	virtual unsigned getBufferSize() = 0;
	virtual unsigned getSampleRate() = 0;
	virtual float* getOut_L() = 0;
	virtual float* getOut_R() = 0;

	virtual void updateTransportInfo() = 0;
	virtual void play() = 0;
	virtual void stop() = 0;
	virtual void locate( unsigned long nFrame ) = 0;
	virtual void setBpm( float fBPM ) = 0;

	/** Accesses the per-track output port settings of a driver.
	 * \return __track_out_enabled */
	bool has_track_outs() {
		return __track_out_enabled;
	}

protected:
	/**
	 * Whether the JackAudioDriver is ordered to create per-track
	 * audio output ports (true). The order comes from the
	 * variable Preferences::m_bJackTrackOuts. It will be used by
	 * the Sampler and Hydrogen itself to probe the behavior of
	 * the JackAudioDriver.
	 *
	 * In all other drivers this variable isn't used. It gets
	 * initialized to false.
	 */
	bool __track_out_enabled;

};

};

#endif

