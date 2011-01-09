
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

#ifndef INSTRUMENT_LAYER_H
#define INSTRUMENT_LAYER_H

#include <hydrogen/object.h>
#include <hydrogen/basics/sample.h>

namespace H2Core
{


class InstrumentLayer : public Object
{
    H2_OBJECT
public:
	InstrumentLayer( Sample *sample );
	~InstrumentLayer();

	void set_start_velocity( float vel ) {
		__start_velocity = vel;
	}
	float get_start_velocity() {
		return __start_velocity;
	}

	void set_end_velocity( float vel ) {
		__end_velocity = vel;
	}
	float get_end_velocity() {
		return __end_velocity;
	}

	void set_pitch( float pitch ) {
		__pitch = pitch;
	}
	float get_pitch() {
		return __pitch;
	}

	void set_gain( float gain ) {
		__gain = gain;
	}
	float get_gain() {
		return __gain;
	}

	void set_sample( Sample* sample ) {
		__sample = sample;
	}
	Sample* get_sample() {
		return __sample;
	}

private:
	float __start_velocity;		///< Start velocity
	float __end_velocity;		///< End velocity
	float __pitch;
	float __gain;
	Sample *__sample;
};

};

#endif


