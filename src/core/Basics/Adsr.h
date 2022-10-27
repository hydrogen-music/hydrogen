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
		 * \param attack tick duration
		 * \param decay tick duration
		 * \param sustain level
		 * \param release tick duration
		 */
		ADSR ( unsigned int attack = 0, unsigned int decay = 0, float sustain = 1.0, unsigned int release = 1000 );

		/** copy constructor */
		ADSR( const std::shared_ptr<ADSR> other );

		/** destructor */
		~ADSR();

		void setAttack( unsigned int value );
		unsigned int getAttack();
		void setDecay( unsigned int value );
		unsigned int getDecay();
		void setSustain( float value );
		float getSustain();
		void setRelease( unsigned int value );
		unsigned int getRelease();

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
		 * \param pLeft left-channel audio buffer
		 * \param pRight right-channel audio buffer
		 * \param nFrames number of frames of audio
		 * \param nReleaseFrame frame number of the release point
		 * \param fStep the increment to be added to m_fTicks
		 */

		bool applyADSR( float *pLeft, float *pRight, int nFrames, int nReleaseFrame, float fStep );

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
		/** possible states */
		enum class State {
			Attack = 0,
			Decay,
			Sustain,
			Release,
			Idle
		};
	static QString StateToQString( State state );
	
		unsigned int m_nAttack;		///< Attack tick count
		unsigned int m_nDecay;		///< Decay tick count
		float m_fSustain;			///< Sustain level
		unsigned int m_nRelease;		///< Release tick count
		State m_state;      ///< current state
		float m_fTicks;          ///< current tick count
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

inline unsigned int ADSR::getAttack()
{
	return m_nAttack;
}

inline void ADSR::setDecay( unsigned int value )
{
	m_nDecay = value;
}

inline unsigned int ADSR::getDecay()
{
	return m_nDecay;
}

inline void ADSR::setSustain( float value )
{
	m_fSustain = value;
}

inline float ADSR::getSustain()
{
	return m_fSustain;
}

inline void ADSR::setRelease( unsigned int value )
{
	m_nRelease = value;
}

inline unsigned int ADSR::getRelease()
{
	return m_nRelease;
}

};

#endif // H2C_ADRS_H

/* vim: set softtabstop=4 noexpandtab: */
