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

#ifndef H2C_ADSR_H
#define H2C_ADSR_H

#include <core/Object.h>

#include <memory>

namespace H2Core
{

/**
 * Attack Decay Sustain Release envelope.
 */
/** \ingroup docCore docDataStructure */
class ADSR : public Object<ADSR>
{
		H2_OBJECT(ADSR)
	public:

		/**
		 * constructor
		 * \param attack phase duration in frames
		 * \param decay phase duration in frames
		 * \param sustain level
		 * \param release phase duration in frames
		 */
		ADSR ( unsigned int attack = 0, unsigned int decay = 0, float sustain = 1.0, unsigned int release = 1000 );

		/** copy constructor */
		ADSR( const std::shared_ptr<ADSR> other );

		/** destructor */
		~ADSR();

		void setAttack( unsigned int value );
		unsigned int getAttack() const;
		void setDecay( unsigned int value );
		unsigned int getDecay() const;
		void setSustain( float value );
		float getSustain() const;
		void setRelease( unsigned int value );
		unsigned int getRelease() const;


		/**
		 * Sets #m_state to #State::Attack
		 */
		void attack();
		/**
		 * Sets #m_state to #State::Release and return the current
		 * #m_fReleaseValue.
		 *
		 * State setting is only applied if the ADSR is not in #State::Idle.
		 * */
		float release();

		/**
		 * Compute and apply successive ADSR values to stereo buffers.
		 *
		 * In case the ADSR still hold its default values, the
		 * provided buffers aren't altered.
		 *
		 * Conceptionally it multiplies the first #m_nAttack frames of
		 * a sample with a rising attack exponential, the successive
		 * #m_nDecay frames with a falling decay exponential, and
		 * remaining frames with #m_fSustain. Which of these steps are
		 * covered in a run of applyADSR() depends on #m_state.
		 *
		 * If no note length was specified by the user in the GUI,
		 * #m_fSustain will be applied till the end of the
		 * corresponding sample and ADSR application won't enter
		 * release phase which would apply a falling exponential for
		 * #m_nRelease frames and zero all following frames.
		 *
		 * \param pLeft left-channel audio buffer
		 * \param pRight right-channel audio buffer
		 * \param nFinalBufferPos Up to which frame @a pLeft and @a
		 * pRight will be processed.
		 * \param nReleaseFrame Frame number indicating the end of the
		 * note or sample at which ADSR processing will enter the
		 * release phase.
		 * \param fStep the increment to be added to m_fFramesInState.
		 */

		bool applyADSR( float *pLeft, float *pRight, int nFinalBufferPos, int nReleaseFrame, float fStep );

		/** possible states */
		enum class State {
			Attack = 0,
			Decay,
			Sustain,
			Release,
			Idle
		};
	static QString StateToQString( const State& state );

		const State& getState() const;

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
	private:
	
		unsigned int m_nAttack;		///< Attack phase duration in frames
		unsigned int m_nDecay;		///< Decay phase duration in frames
		float m_fSustain;			///< Sustain level
		unsigned int m_nRelease;		///< Release phase duration in frames
		State m_state;      ///< current state
	/**
	 * Tracks the number of frames passed in the current #m_state.
	 *
	 * It is used to determine whether ADSR processing should proceed
	 * into the next state and required if multiple process cycles of
	 * the #H2Core::Sampler render the same state.
	 *
	 * Note that it is given in float instead of integer (the basic
	 * unit for frames in the #H2Core::AudioEngine) in order to
	 * account for processing while resampling the original audio.
	 */
		float m_fFramesInState;
		float m_fValue;          ///< current value
		float m_fReleaseValue;  ///< value when the release state was entered

		double m_fQ;				///< exponential decay state

		void normalise();
};

// DEFINITIONS

inline void ADSR::setAttack( unsigned int value )
{
	m_nAttack = value;
}

inline unsigned int ADSR::getAttack() const
{
	return m_nAttack;
}

inline void ADSR::setDecay( unsigned int value )
{
	m_nDecay = value;
}

inline unsigned int ADSR::getDecay() const
{
	return m_nDecay;
}

inline void ADSR::setSustain( float value )
{
	m_fSustain = value;
}

inline float ADSR::getSustain() const
{
	return m_fSustain;
}

inline void ADSR::setRelease( unsigned int value )
{
	m_nRelease = value;
}

inline unsigned int ADSR::getRelease() const
{
	return m_nRelease;
}
inline const ADSR::State& ADSR::getState() const {
	return m_state;
}

};

#endif // H2C_ADRS_H

/* vim: set softtabstop=4 noexpandtab: */
