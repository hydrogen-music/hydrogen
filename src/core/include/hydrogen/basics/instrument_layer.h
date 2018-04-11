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

#ifndef H2C_INSTRUMENT_LAYER_H
#define H2C_INSTRUMENT_LAYER_H

#include <hydrogen/object.h>
#include <hydrogen/basics/sample.h>

namespace H2Core
{

class XMLNode;

/**
 * InstrumentLayer is part of an instrument
 * <br>each layer has it's own :
 * <br><b>gain</b> which is the ration between the input sample and the output signal,
 * <br><b>pitch</b> wich allows you to play the sample at a faster or lower frequency,
 * <br><b>start velocity</b> and <b>end velocity</b> which allows you to chose between a layer or another within an instrument
 * by changing the velocity of the played note. so the only layer of an instrument should start at 0.0 and end at 1.0.
*/
class InstrumentLayer : public H2Core::Object
{
		H2_OBJECT
	public:
		/** constructor
		 * \param sample the sample to use
		 * */
		InstrumentLayer( Sample* sample );
		/** copy constructor, will be initialized with an empty sample
		 * \param other the instrument layer to copy from
		 */
		InstrumentLayer( InstrumentLayer* other );
		/** copy constructor
		 * \param other the instrument layer to copy from
		 * \param sample the sample to use
		 */
		InstrumentLayer( InstrumentLayer* other, Sample* sample );
		/** destructor */
		~InstrumentLayer();

		/** set the gain of the layer */
		void set_gain( float gain );
		/** get the gain of the layer */
		float get_gain() const;
		/** set the pitch of the layer */
		void set_pitch( float pitch );
		/** get the pitch of the layer */
		float get_pitch() const;

		/** set the start ivelocity of the layer */
		void set_start_velocity( float start );
		/** get the start velocity of the layer */
		float get_start_velocity() const;
		/** set the end velocity of the layer */
		void set_end_velocity( float end );
		/** get the end velocity of the layer */
		float get_end_velocity() const;
		/** set the sample of the layer */
		void set_sample( Sample* sample );
		/** get the sample of the layer */
		Sample* get_sample() const;

		/**
		 * load the sample data
		 */
		void load_sample();
		/*
		 * unload sample and replace it with an empty one
		 */
		void unload_sample();

		/**
		 * save the intrument layer within the given XMLNode
		 * \param node the XMLNode to feed
		 */
		void save_to( XMLNode* node );
		/**
		 * load an instrument layer from an XMLNode
		 * \param node the XMLDode to read from
		 * \param dk_path the directory holding the drumkit data
		 * \return a new InstrumentLayer instance
		 */
		static InstrumentLayer* load_from( XMLNode* node, const QString& dk_path );

	private:
		float __gain;               ///< ratio between the input sample and the output signal, 1.0 by default
		float __pitch;              ///< the frequency of the sample, 0.0 by default which means output pitch is the same as input pitch
		float __start_velocity;     ///< the start velocity of the sample, 0.0 by default
		float __end_velocity;       ///< the end velocity of the sample, 1.0 by default
		Sample* __sample;           ///< the underlaying sample
};

// DEFINITIONS

inline void InstrumentLayer::set_gain( float gain )
{
	__gain = gain;
}

inline float InstrumentLayer::get_gain() const
{
	return __gain;
}

inline void InstrumentLayer::set_pitch( float pitch )
{
	__pitch = pitch;
}

inline float InstrumentLayer::get_pitch() const
{
	return __pitch;
}

inline void InstrumentLayer::set_start_velocity( float start )
{
	__start_velocity = start;
}

inline float InstrumentLayer::get_start_velocity() const
{
	return __start_velocity;
}

inline void InstrumentLayer::set_end_velocity( float end )
{
	__end_velocity = end;
}

inline float InstrumentLayer::get_end_velocity() const
{
	return __end_velocity;
}

inline void InstrumentLayer::set_sample( Sample* sample )
{
	if ( __sample ) {
		delete __sample;
	}
	__sample = sample;
}

inline Sample* InstrumentLayer::get_sample() const
{
	return __sample;
}

};

#endif // H2C_INSTRUMENT_LAYER_H

/* vim: set softtabstop=4 noexpandtab: */
