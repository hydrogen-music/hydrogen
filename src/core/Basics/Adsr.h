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

		/**
		 * __attack setter
		 * \param value the new value
		 */
		void set_attack( unsigned int value );
		/** __attack accessor */
		unsigned int get_attack();
		/**
		 * __decay setter
		 * \param value the new value
		 */
		void set_decay( unsigned int value );
		/** __decay accessor */
		unsigned int get_decay();
		/**
		 * __sustain setter
		 * \param value the new value
		 */
		void set_sustain( float value );
		/** __sustain accessor */
		float get_sustain();
		/**
		 * __release setter
		 * \param value the new value
		 */
		void set_release( unsigned int value );
		/** __release accessor */
		unsigned int get_release();

		/**
		 * sets state to ATTACK
		 */
		void attack();
		/**
		 * compute the value and return it
		 * \param step the increment to be added to __ticks
		 */
		float get_value( float step );
		/**
		 * sets state to RELEASE,
		 * returns 0 if the state is IDLE,
		 * __value if the state is RELEASE,
		 * set state to RELEASE, save __release_value and return it.
		 * */
		float release();

		/**
		 * Compute and apply successive ADSR values to stereo buffers.
		 * \param pLeft left-channel audio buffer
		 * \param pRight right-channel audio buffer
		 * \param nFrames number of frames of audio
		 * \param nReleaseFrame frame number of the release point
		 * \param fStep the increment to be added to __ticks
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
		QString toQString( const QString& sPrefix, bool bShort = true ) const override;
	private:
		unsigned int __attack;		///< Attack tick count
		unsigned int __decay;		///< Decay tick count
		float __sustain;			///< Sustain level
		unsigned int __release;		///< Release tick count
		/** possible states */
		enum ADSRState {
			ATTACK=0,
			DECAY,
			SUSTAIN,
			RELEASE,
			IDLE
		};
		ADSRState __state;      ///< current state
		float __ticks;          ///< current tick count
		float __value;          ///< current value
		float __release_value;  ///< value when the release state was entered

		double m_fQ;				///< exponential decay state

		void normalise();
};

// DEFINITIONS

inline void ADSR::set_attack( unsigned int value )
{
	__attack = value;
}

inline unsigned int ADSR::get_attack()
{
	return __attack;
}

inline void ADSR::set_decay( unsigned int value )
{
	__decay = value;
}

inline unsigned int ADSR::get_decay()
{
	return __decay;
}

inline void ADSR::set_sustain( float value )
{
	__sustain = value;
}

inline float ADSR::get_sustain()
{
	return __sustain;
}

inline void ADSR::set_release( unsigned int value )
{
	__release = value;
}

inline unsigned int ADSR::get_release()
{
	return __release;
}

};

#endif // H2C_ADRS_H

/* vim: set softtabstop=4 noexpandtab: */
