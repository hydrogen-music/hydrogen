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
#ifndef TRANSPORT_INFO_H
#define TRANSPORT_INFO_H

#include <core/Object.h>

namespace H2Core
{

/**
 * Object holding most of the information about the transport state of
 * the AudioEngine, like if it is playing or stopped or its current
 * transport position and speed.
 *
 * Due to the original design of Hydrogen the fundamental variable to
 * determine the transport position is a tick. Whenever a tempo change
 * is encounter, the tick size (in frames per tick) is rescaled. To
 * nevertheless ensure compatibility with frame-based audio systems,
 * like JACK, this class will also keep track of the frame count
 * during BPM changes, relocations etc. This variable is dubbed
 * "externalFrames" to indicate that it's not used within Hydrogen but
 * to sync it with other apps.
 */
class TransportInfo : public H2Core::Object
{
public:

	/**
	 * Constructor of TransportInfo
	 */
	TransportInfo( const char* __class_name );
	/** Destructor of TransportInfo */
	~TransportInfo();

	long long getFrames() const;
	float getTickSize() const;
	float getBpm() const;

	/** Returns transport position in frames that would be used if
		Hydrogen would be frame- instead of tick-based.*/
	long long getExternalFrames() const;
	
	void setBpm( float fNewBpm );

	// TODO: make this protected
	void setTickSize( float fNewTickSize );
	// TODO: make this protected
	void setFrames( long long nNewFrames );

protected:
	/** All classes other than the AudioEngine should use
		AudioEngine::locate().
	*/
	void setExternalFrames( long long nNewExternalFrames );

private:

	/** 
	 * Current transport position in number of frames since the
	 * beginning of the song.
	 *
	 * A __frame__ is a single sample of an audio signal. Thus, with a
	 * _sample rate_ of 48000Hz, 48000 frames will be recorded in one
	 * second and, with a _buffer size_ = 1024, 1024 consecutive
	 * frames will be accumulated before they are handed over to the
	 * audio engine for processing. Internally, the transport is based
	 * on float precision ticks. (#m_nFrames / #m_fTickSize).
	 */
	long long m_nFrames;

	/**
	 * Current transport position if Hydrogen would have a frame-based
	 * transport system.
	 *
	 * This is required to ensure compatibility with frame-based audio
	 * drivers, like JACK. But it is not used within Hydrogen itself.
	 */
	long long m_nExternalFrames;
	
	/** 
	 * Number of frames that make up one tick.
	 *
	 * A tick is the most fine-grained time scale handled by the
	 * AudioEngine. The notes won't be processed frame by frame but,
	 * instead, tick by tick. Therefore, #m_fTickSize represents the
	 * minimum duration of a Note as well as the minimum distance
	 * between two of them.
	 */
	float m_fTickSize;
	/** Current tempo in beats per minute. */
	float m_fBpm;
};

inline long long TransportInfo::getFrames() const {
	return m_nFrames;
}
inline long long TransportInfo::getExternalFrames() const {
	return m_nExternalFrames;
}
inline float TransportInfo::getTickSize() const {
	return m_fTickSize;
}
inline float TransportInfo::getBpm() const {
	return m_fBpm;
}
};

#endif

