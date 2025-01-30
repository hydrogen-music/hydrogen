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
#ifndef TRANSPORT_POSITION_H
#define TRANSPORT_POSITION_H

#include <memory>

#include <core/Object.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/AudioEngineTests.h>
#include <core/IO/JackAudioDriver.h>

namespace H2Core
{

class PatternList;

/**
 * Object holding most of the information about the transport state of
 * the AudioEngine.
 *
 * Due to the original design of Hydrogen the fundamental variable to
 * determine the transport position is a tick. Whenever a tempo or
 * song size change is encountered, the tick and frame information are
 * shifted or rescaled. To nevertheless ensure consistency, the amount
 * of compensation required to retrieve the original position is
 * stored in a number of dedicated offset variables.
 */
class TransportPosition : public H2Core::Object<TransportPosition>
{
	H2_OBJECT(TransportPosition)
public:

	TransportPosition( const QString sLabel = "" );
	TransportPosition( std::shared_ptr<TransportPosition> pOther );
	~TransportPosition();

	const QString getLabel() const;
	long long getFrame() const;
	/**
	 * Retrieve a rounded version of #m_fTick.
	 *
	 * Only within the #AudioEngine ticks are handled as doubles.
	 * This is required to allow for a seamless transition between
	 * frames and ticks without any rounding error. All other parts
	 * use integer values (due to historical reasons).
	 */
	long getTick() const;
	float getTickSize() const;
	float getBpm() const;
	long getPatternStartTick() const;
	long getPatternTickPosition() const;
	int getColumn() const;
	double getTickMismatch() const;
	long long getFrameOffsetTempo() const;
	double getTickOffsetQueuing() const;
	double getTickOffsetSongSize() const;
	const PatternList* getPlayingPatterns() const;
	const PatternList* getNextPatterns() const;
	int getPatternSize() const;
	long long getLastLeadLagFactor() const;
	int getBar() const;
	int getBeat() const;

	/**
	 * Calculates tick equivalent of @a nFrame.
	 *
	 * In case the #Timeline is activated, the function takes all
	 * passed tempo markers into account in order to determine the
	 * number of ticks passed when letting the #AudioEngine roll for
	 * @a nFrame frames.
	 *
	 * It depends on the sample rate @a nSampleRate and assumes that
	 * it as well as the resolution to be constant over the whole
	 * song.
	 *
	 * @param nFrame Transport position in frame which should be
	 * converted into ticks.
	 * @param nSampleRate If set to 0, the sample rate provided by the
	 * audio driver will be used.
	 */
	static double computeTickFromFrame( long long nFrame, int nSampleRate = 0 );

	/**
	 * Calculates frame equivalent of @a fTick.
	 *
	 * In case the #Timeline is activated, the function takes all
	 * passed tempo markers into account in order to determine the
	 * number of frames passed when letting the #AudioEngine roll for
	 * @a fTick ticks.
	 *
	 * It depends on the sample rate @a nSampleRate and assumes that
	 * it as well as the resolution to be constant over the whole
	 * song.
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

	friend bool operator==( std::shared_ptr<TransportPosition> lhs,
							 std::shared_ptr<TransportPosition> rhs );
	friend bool operator!=( std::shared_ptr<TransportPosition> lhs,
							 std::shared_ptr<TransportPosition> rhs );
	
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
	friend class JackAudioDriver;

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
	void setFrameOffsetTempo( long long nFrameOffset );
	void setTickOffsetQueuing( double nTickOffset );
	void setTickOffsetSongSize( double fTickOffset );
	void setPlayingPatterns( PatternList* pPatternList );
	void setNextPatterns( PatternList* pPatternList );
	void setPatternSize( int nPatternSize );
	void setLastLeadLagFactor( long long nValue );
	void setBar( int nBar );
	void setBeat( int nBeat );
	
	PatternList* getPlayingPatterns();
	PatternList* getNextPatterns();

	/**
	 * Converts ticks into frames under the assumption of a constant
	 * @a fTickSize (sample rate, tempo, and resolution did not
	 * change).
	 *
	 * As the assumption above does not hold once a tempo marker is
	 * introduced, computeFrameFromTick() should be used instead while
	 * this function is only meant for internal use.
	 */
	static long long computeFrame( double fTick, float fTickSize );
	/**
	 * Converts frames into ticks under the assumption of a constant
	 * @a fTickSize (sample rate, tempo, and resolution did not
	 * change).
	 *
	 * As the assumption above does not hold once a tempo marker is
	 * introduced, computeFrameFromTick() should be used instead while
	 * this function is only meant for internal use.
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
	 * on ticks. (#m_nFrame / #m_fTickSize)
	 */
	long long m_nFrame;
	
	/**
	 * Current transport position in number of ticks since the
	 * beginning of the song.
	 *
	 * A tick is the smallest temporal unit used for transport,
	 * navigation, and audio rendering within Hydrogen. (Note that the
	 * smallest unit for positioning a #Note is a frame due to the
	 * humanization capabilities.)
	 *
	 * Although the precision of this variable is double, only a
	 * version of it rounded to integer is used outside of the
	 * #AudioEngine.
	 *
	 * Float is, unfortunately, not enough. When the engine is running
	 * for a long time the high precision digits after decimal point
	 * required to keep frames and ticks in sync would be lost.
	 */
	double m_fTick;
	
	/** 
	 * Number of frames that make up one tick.
	 *
	 * Calculated using AudioEngine::computeTickSize().
	 */
	float m_fTickSize;
	/** Current tempo in beats per minute.
	 *
	 * It can be set through three different mechanisms and the
	 * corresponding values are stored in different places.
	 *
	 * 1. The most fundamental one is stored in #Song::m_fBpm and can
	 * be set using the BPM widget in the #PlayerControl or via MIDI
	 * and OSC commands. Writing the value to the current #Song is
	 * done by the latter commands and widget and not within the
	 * #AudioEngine.
	 *
	 * 2. It is superseded by the tempo markers as soon as the
	 * #Timeline is activated and at least one TempoMarker is set. The
	 * current speed during Timeline-based transport will not override
	 * #Song::m_fBpm and is stored using the tempo markers in the
	 * .h2song file instead. (see #Timeline for details)
	 *
	 * 3. Both #Song and #Timeline tempo are superseded by the BPM
	 * broadcasted by the JACK Timebase controller application once
	 * Hydrogen acts as Timebase listener. The corresponding value
	 * depends entirely on the external application and will not be
	 * stored by Hydrogen.
	 */
	float m_fBpm;
	
	/**
	 * Dicstance in ticks between the beginning of the song and the
	 * beginning of the current column (#m_nColumn).
	 *
	 * The current transport position corresponds
	 * to #m_fTick = (roughly) #m_nPatternStartTick +
	 * #m_nPatternTickPosition.
	 */
	long				m_nPatternStartTick;
	/**
	 * Ticks passed since #m_nPatternStartTick.
	 *
	 * The current transport position thus corresponds
	 * to #m_fTick = (roughly) #m_nPatternStartTick +
	 * #m_nPatternTickPosition.
	 */
	long				m_nPatternTickPosition;
	/**
	 * Specifies the column transport is located in and can be used as
	 * the index of the current PatternList/column in the
	 * #Song::m_pPatternGroupSequence.
	 *
	 * A value of -1 corresponds to "pattern list could not be found"
	 * and is used to indicate that transport reached the end of the
	 * song (with transport not looped).
	 */
	int					m_nColumn;
	
	/** Number of ticks #m_nFrame is ahead/behind of
	 *	#m_fTick.
	 *
	 * This is due to the rounding error introduced when calculating
	 * the frame counterpart #m_nFrame of #m_fTick using
	 * computeFrameFromTick().
	 * #m_nFrame.
	**/
	double 				m_fTickMismatch;

	/**
	 * Frame offset introduced when changing the tempo of the song, switching to
	 * Timeline, adding or remove an Tempo Marker while Timeline is active, or
	 * encountering an external tempo change by a JACK Timebase controller.
	 *
	 * Each tempo or Tempo Marker change does alter #m_fTickSize and results in
	 * #m_nFrame and #m_fTick to not be consistent anymore. We will handle this
	 * by compensating the difference in frames (#m_fTick is kept constant
	 * during a tempo change while #m_nFrame gets rescaled) using
	 * #m_nFrameOffsetTempo as an additive offset internally.
	 *
	 * When locating transport or stopping playback both #m_nFrame and
	 * #m_fTick become synced again and #m_nFrameOffsetTempo gets
	 * resetted.
	 *
	 * Note this is not the frame equivalent of #m_fTickOffsetQueuing.
	 */
	long long 			m_nFrameOffsetTempo;
	/**
	 * Tick offset introduced when changing the tempo of the song.
	 *
	 * In case the #Timeline is deactivate each tempo change does
	 * alter #m_fTickSize and results in the start of the new tick
	 * interval covered for note enqueuing in
	 * AudioEngine::updateNoteQueue() to not be consistent with the
	 * previous interval end anymore. Holes or overlaps could lead to
	 * note misses or double enqueuings. We will handle this by
	 * compensating the difference in ticks using
	 * #m_fTickOffsetQueuing as an additive offset internally.
	 *
	 * When locating transport or stopping playback both the tick
	 * interval and #m_fTickOffsetQueuing get resetted.
	 *
	 * Note this is not the tick equivalent of #m_nFrameOffsetTempo.
	 */
	double 				m_fTickOffsetQueuing;

	/**
	 * Tick offset introduced when changing the size of the song.
	 *
	 * When altering the size of the song, e.g. by enlarging a pattern
	 * prior to the one currently playing, both #m_nFrame and #m_fTick
	 * become invalid. We will handle this by compensating the
	 * difference between the old and new tick position using
	 * #m_fTickOffsetSongSize as an additive offset internally. In
	 * addition, #m_nFrameOffsetTempo and #m_fTickOffsetQueuing will
	 * be used to compensate for the change in the current frame
	 * position and tick interval end.
	 *
	 * When locating transport or stopping playback both #m_nFrame and
	 * #m_fTick become synced again and #m_fTickOffsetSongSize gets
	 * resetted.
	 */
	double 				m_fTickOffsetSongSize;

	/**
	 * Patterns used to toggle the ones in #m_pPlayingPatterns in
	 * Song::PatternMode::Stacked.
	 *
	 * If a #Pattern is already playing and added to #m_pNextPatterns,
	 * it will the removed from #m_pPlayingPatterns next time
	 * transport is looped to the beginning and vice versa.
	 *
	 * See AudioEngine::updatePlayingPatterns() for details.
	 */
	PatternList*		m_pNextPatterns;
	
	/**
	 * Contains all Patterns currently played back.
	 *
	 * If transport is in #H2Core::Song::Mode::Song, it corresponds
	 * to the patterns present in column #m_nColumn.
	 *
	 * Due to performance reasons no virtual patterns will be checked
	 * and expanded in this list. Instead, all contained patterns have
	 * to be added explicitly.
	 *
	 * See AudioEngine::updatePlayingPatterns() for details.
	 */
	PatternList*		m_pPlayingPatterns;
	
	/**
	 * Maximum size of all patterns in #m_pPlayingPatterns.
	 *
	 * If #m_pPlayingPatterns is empty, #H2Core::MAX_NOTES will be
	 * used as fallback.
	 */
	int 				m_nPatternSize;

	/**
	 * #AudioEngine::getLeadLagInFrames() calculated for the previous
     * transport position.
	 *
	 * It is required to ensure a smooth update of the queuing
	 * position in AudioEngine::updateNoteQueue() without any holes or
	 * overlaps in the covered ticks (while using the #Timeline in
	 * #Song::Mode::Song).
	 */
	long long m_nLastLeadLagFactor;

	/**
	 * Last beat (column + 1) passed.
	 *
	 * Note that this variable starts at 1 not at 0.
	 */
	int m_nBar;
	/**
	 * Last bar passed since #m_nBar. A bar is composed of 48 ticks.
	 *
	 * Note that this variable starts at 1 not at 0.
	 */
	int m_nBeat;
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
inline long long TransportPosition::getFrameOffsetTempo() const {
	return m_nFrameOffsetTempo;
}
inline void TransportPosition::setFrameOffsetTempo( long long nFrameOffset ) {
	m_nFrameOffsetTempo = nFrameOffset;
}
inline double TransportPosition::getTickOffsetQueuing() const {
	return m_fTickOffsetQueuing;
}
inline void TransportPosition::setTickOffsetQueuing( double fTickOffset ) {
	m_fTickOffsetQueuing = fTickOffset;
}
inline double TransportPosition::getTickOffsetSongSize() const {
	return m_fTickOffsetSongSize;
}
inline void TransportPosition::setTickOffsetSongSize( double fTickOffset ) {
	m_fTickOffsetSongSize = fTickOffset;
}
inline const PatternList* TransportPosition::getPlayingPatterns() const {
	return m_pPlayingPatterns;
}
inline PatternList* TransportPosition::getPlayingPatterns() {
	return m_pPlayingPatterns;
}
inline const PatternList* TransportPosition::getNextPatterns() const {
	return m_pNextPatterns;
}
inline PatternList* TransportPosition::getNextPatterns() {
	return m_pNextPatterns;
}
inline int TransportPosition::getPatternSize() const {
	return m_nPatternSize;
}
inline long long TransportPosition::getLastLeadLagFactor() const {
	return m_nLastLeadLagFactor;
}
inline void TransportPosition::setLastLeadLagFactor( long long nValue ) {
	m_nLastLeadLagFactor = nValue;
}
inline int TransportPosition::getBar() const {
	return m_nBar;
}
inline int TransportPosition::getBeat() const {
	return m_nBeat;
}
};

#endif

