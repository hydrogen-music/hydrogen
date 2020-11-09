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
#ifndef TRANSPORT_INFO_H
#define TRANSPORT_INFO_H

#include <hydrogen/object.h>

namespace H2Core
{

/**
 * Object holding most of the information about the transport state of
 * the AudioEngine, like if it is playing or stopped or its current
 * transport position and speed.
 */
class TransportInfo : public H2Core::Object
{
	H2_OBJECT
public:
	enum {
	      /** The audio engine is playing back or processing audio
		  and the transport is stopped. */
	      STOPPED,
	      /** The audio engine is playing back or processing audio
		  and the transport is running. */
	      ROLLING,
	      /** This option is not yet used in the source code. */
	      BAD
	};
	/**
	 * Current transport status of the audio engine. It can have
	 * three (known) states:
	 * - TransportInfo::STOPPED
	 * - TransportInfo::ROLLING
	 * - TransportInfo::BAD
	 */
	unsigned m_status;

	/** 
	 * Current transport position in number of frames since the
	 * beginning of the song.
	 *
	 * A __frame__ is a single sample of an audio signal. Thus,
	 * with a _sample rate_ of 48000Hz, 48000 frames will be
	 * recorded in one second and, with a _buffer size_ = 1024,
	 * 1024 consecutive frames will be accumulated before they are
	 * handed over to the audio engine for processing. Internally,
	 * a frame will be represented by a float.
	 */
	long long m_nFrames;
	/** 
	 * Number of frames that make up one tick.
	 *
	 * A tick is the most fine-grained time scale handled by the
	 * AudioEngine. The notes won't be processed frame by frame but,
	 * instead, tick by tick. Therefore, #m_fTickSize represents the
	 * minimum duration of a Note as well as the minimum distance
	 * between two of them.
	 * 
	 * It will only be used by the JackAudioDriver and is
	 * calculated by the sample rate * 60.0 / ( #m_fBPM *
	 * Song::__resolution ). The factor 60.0 will be used to convert
	 * the sample rate, which is given in second, into minutes (as
	 * the #m_fBPM).
	 */
	float m_fTickSize;
	/** Current tempo in beats per minute. */
	float m_fBPM;

	/**
	 * Constructor of TransportInfo
	 *
	 * - Sets #m_status to TransportInfo::STOPPED
	 * - Sets #m_nFrames and #m_fTickSize to 0
	 * - Sets #m_fBPM to 120
	 */
	TransportInfo();
	/** Destructor of TransportInfo */
	~TransportInfo();
	/** Displays general information about the transport state in
	    the #INFOLOG
	  *
	  * Prints out #m_status, #m_nFrames, and #m_fTickSize.
	  */
	void printInfo();
};

};

#endif

