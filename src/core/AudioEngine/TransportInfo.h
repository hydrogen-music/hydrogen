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
class TransportInfo : public H2Core::Object<TransportInfo>
{
	H2_OBJECT(TransportInfo)
public:

	/**
	 * Constructor of TransportInfo
	 */
	TransportInfo();
	/** Destructor of TransportInfo */
	~TransportInfo();

	long long getFrames() const;
	double getTick() const;
	float getTickSize() const;
	float getBpm() const;

protected:
	void setFrames( long long nNewFrames );
	void setTick( double fNewTick );
	void setBpm( float fNewBpm );
	void setTickSize( float fNewTickSize );
	/** All classes other than the AudioEngine should use
		AudioEngine::locate().
	*/

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
	 * on float precision ticks. (#m_nFrames / #m_fTickSize) Caution:
	 * when using the Timeline the ticksize does change throughout the
	 * song. This requires #m_nFrames to be recalculate with
	 * AudioEngine::updateFrames() each time the transport position is
	 * relocated (possibly crossing some tempo markers).
	 */
	long long m_nFrames;
	
	/**
	 * Smallest temporal unit used for transport navigation within
	 * Hydrogen and is calculated using AudioEngine::computeTick(),
	 * #m_nFrames, and #m_fTickSize.
	 *
	 * Note that the smallest unit for positioning a #Note is a frame
	 * due to the humanization capabilities.
	 *
	 * Float is, unfortunately, not enough. When the engine is running
	 * for a long time the high precision digits after decimal point
	 * required to keep frames and ticks in sync would be lost.
	 */
	double m_fTick;
	
	/** 
	 * Number of frames that make up one tick.
	 *
	 * The notes won't be processed frame by frame but, instead, tick
	 * by tick. Therefore, #m_fTickSize represents the minimum
	 * duration of a Note as well as the minimum distance between two
	 * of them.
	 *
	 * Calculated using AudioEngine::computeTickSize().
	 */
	float m_fTickSize;
	/** Current tempo in beats per minute.
	 *
	 * The tempo hold by the #TransportInfo (and thus the
	 * #AudioEngine) is the one currently used throughout Hydrogen. It
	 * can be set through three different mechanisms and the
	 * corresponding values are stored in different places.
	 *
	 * The most fundamental one is stored in Song::m_fBpm and can be
	 * set using the BPM widget in the PlayerControl or via MIDI and
	 * OSC commands. Writing the value to the current #Song is done by
	 * the latter commands and widget and not within the AudioEngine.
	 *
	 * It is superseded by the tempo markers as soon as the #Timeline
	 * is activated and at least one TempoMarker is set. The current
	 * speed during Timeline-based transport will not override
	 * Song::m_fBpm and is stored using the tempo markers in the
	 * .h2song file instead. (see #Timeline for details)
	 *
	 * Both Song and Timeline tempo are superseded by the BPM
	 * broadcasted by the JACK timebase master application once
	 * Hydrogen acts as timebase slave. The corresponding value
	 * depends entirely on the external application and will not be
	 * stored by Hydrogen.
	 */
	float m_fBpm;
};

inline long long TransportInfo::getFrames() const {
	return m_nFrames;
}
inline double TransportInfo::getTick() const {
	return m_fTick;
}
inline float TransportInfo::getTickSize() const {
	return m_fTickSize;
}
inline float TransportInfo::getBpm() const {
	return m_fBpm;
}
};

#endif

