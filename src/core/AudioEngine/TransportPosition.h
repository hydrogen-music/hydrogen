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
#ifndef TRANSPORT_POSITION_H
#define TRANSPORT_POSITION_H

#include <memory>

#include <core/Object.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/AudioEngineTests.h>

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
class TransportPosition : public H2Core::Object<TransportPosition>
{
	H2_OBJECT(TransportPosition)
public:

	/**
	 * Constructor of TransportPosition
	 */
	TransportPosition( const QString sLabel = "" );
	/** Destructor of TransportPosition */
	~TransportPosition();

	const QString getLabel() const;
	long long getFrame() const;
	/**
	 * Retrieve a rounded version of #m_fTick.
	 *
	 * Only within the #AudioEngine ticks are handled as doubles.
	 * This is required to allow for a seamless transition between
	 * frames and ticks without any rounding error. All other parts
	 * use integer values.
	 */
	long getTick() const;
	float getTickSize() const;
	float getBpm() const;
	long getPatternStartTick() const;
	long getPatternTickPosition() const;
	int getColumn() const;
	double getTickMismatch() const;

		/**
	 * Calculates a tick equivalent to @a nFrame.
	 *
	 * The function takes all passed tempo markers into account and
	 * depends on the sample rate @a nSampleRate. It also assumes that
	 * sample rate and resolution are constant over the whole song.
	 *
	 * @param nFrame Transport position in frame which should be
	 * converted into ticks.
	 * @param nSampleRate If set to 0, the sample rate provided by the
	 * audio driver will be used.
	 */
	static double computeTickFromFrame( long long nFrame, int nSampleRate = 0 );

	/**
	 * Calculates the frame equivalent to @a fTick.
	 *
	 * The function takes all passed tempo markers into account and
	 * depends on the sample rate @a nSampleRate. It also assumes that
	 * sample rate and resolution are constant over the whole song.
	 *
	 * @param fTick Current transport position in ticks.
	 * @param fTickMismatch Since ticks are stored as doubles and there
	 * is some loss in precision, this variable is used report how
	 * much @fTick exceeds/is ahead of the resulting frame.
	 * @param nSampleRate If set to 0, the sample rate provided by the
	 * audio driver will be used.
	 *
	 * @return frame
	 */
	static long long computeFrameFromTick( double fTick, double* fTickMismatch, int nSampleRate = 0 );
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

	friend class AudioEngine;
	friend class AudioEngineTests;

private:
	/**
	 * Copying the content of one position into the other is a lot
	 * cheaper than performing computations, like
	 * #AudioEngine::updateTransportPosition(), twice.
	 */
	void set( std::shared_ptr<TransportPosition> pOther );
	void reset();
	
	void setFrame( long long nNewFrame );
	void setTick( double fNewTick );
	void setTickSize( float fNewTickSize );
	void setBpm( float fNewBpm );
	void setPatternStartTick( long nPatternStartTick );
	void setPatternTickPosition( long nPatternTickPosition );
	void setColumn( int nColumn );

	/**
	 * Converts a tick into frames under the assumption of a constant
	 * @a fTickSize since the beginning of the song (sample rate,
	 * tempo, and resolution did not change).
	 *
	 * As the assumption above usually does not hold,
	 * computeFrameFromTick() should be used instead while this
	 * function is only meant for internal use.
	 */
	static long long computeFrame( double fTick, float fTickSize );
	/**
	 * Converts a frame into ticks under the assumption of a constant
	 * @a fTickSize since the beginning of the song (sample rate,
	 * tempo, and resolution did not change).
	 *
	 * As the assumption above usually does not hold,
	 * computeTickFromFrame() should be used instead while this
	 * function is only meant for internal use.
	 */
	static double computeTick( long long nFrame, float fTickSize );

	double getDoubleTick() const;

	/** Identifier of the transport position. Used to keep different
	 * instances apart.
	 */
	const QString m_sLabel;

	/** 
	 * Current transport position in number of frames since the
	 * beginning of the song.
	 *
	 * A __frame__ is a single sample of an audio signal. Thus, with a
	 * _sample rate_ of 48000Hz, 48000 frames will be recorded in one
	 * second and, with a _buffer size_ = 1024, 1024 consecutive
	 * frames will be accumulated before they are handed over to the
	 * audio engine for processing. Internally, the transport is based
	 * on float precision ticks. (#m_nFrame / #m_fTickSize)
	 */
	long long m_nFrame;
	
	/**
	 * Smallest temporal unit used for transport navigation within
	 * Hydrogen and is calculated using AudioEngine::computeTick(),
	 * #m_nFrame, and #m_fTickSize.
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
	 * The tempo hold by the #TransportPosition (and thus the
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
	
	/**
	 * Beginning of the pattern in ticks the transport position is
	 * located in.
	 *
	 * The current transport position corresponds
	 * to #m_fTick = #m_nPatternStartTick +
	 * #m_nPatternTickPosition.
	 */
	long				m_nPatternStartTick;
	/**
	 * Ticks passed since #m_nPatternStartTick.
	 *
	 * The current transport position thus corresponds
	 * to #m_fTick = #m_nPatternStartTick +
	 * #m_nPatternTickPosition.
	 */
	long				m_nPatternTickPosition;
	/**
	 * Coarse-grained version of #m_nPatternStartTick which can be
	 * used as the index of the current PatternList/column in the
	 * #Song::m_pPatternGroupSequence.
	 *
	 * A value of -1 corresponds to "pattern list could not be found"
	 * and is used to indicate that transport reached the end of the
	 * song (with transport not looped).
	 */
	int					m_nColumn;
	
	/** Number of frames #m_nFrame is ahead/behind of
	 *	#m_nTick.
	 *
	 * This is due to the rounding error introduced when calculating
	 * the frame counterpart in double within computeFrameFromTick()
	 * and rounding it to assign it to #m_nFrame.
	**/
	double 				m_fTickMismatch;
		
};

inline const QString TransportPosition::getLabel() const {
	return m_sLabel;
}
inline long long TransportPosition::getFrame() const {
	return m_nFrame;
}
inline double TransportPosition::getDoubleTick() const {
	return m_fTick;
}
inline long TransportPosition::getTick() const {
	return static_cast<long>(std::floor( m_fTick ));
}
inline float TransportPosition::getTickSize() const {
	return m_fTickSize;
}
inline float TransportPosition::getBpm() const {
	return m_fBpm;
}
inline long TransportPosition::getPatternStartTick() const {
	return m_nPatternStartTick;
}
inline long TransportPosition::getPatternTickPosition() const {
	return m_nPatternTickPosition;
}
inline int TransportPosition::getColumn() const {
	return m_nColumn;
}
inline double TransportPosition::getTickMismatch() const {
	return m_fTickMismatch;
}
};

#endif

