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

#ifndef ADSR_H
#define ADSR_H

#include <hydrogen/Object.h>

namespace H2Core
{

///
/// ADSR envelope.
///
class ADSR : private Object
{
public:
	float __attack;		///< Attack time (in samples)
	float __decay;		///< Decay time (in samples)
	float __sustain;	///< Sustain level
	float __release;	///< Release time (in samples)

	ADSR(
	    float attack = 0.0,
	    float decay = 0.0,
	    float sustain = 1.0,
	    float release = 1000
	);

	ADSR( const ADSR& orig );

	~ADSR();

	float get_value( float step );
	float release();

private:
	enum ADSRState {
		ATTACK,
		DECAY,
		SUSTAIN,
		RELEASE,
		IDLE
	};

	ADSRState __state;
	float __ticks;
	float __value;
	float __release_value;
};

};


#endif
