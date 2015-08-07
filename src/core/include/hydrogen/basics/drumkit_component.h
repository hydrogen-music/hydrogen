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

#ifndef H2C_DRUMKITCOMPONENT_H
#define H2C_DRUMKITCOMPONENT_H

#include <cassert>
#include <inttypes.h>

#include <hydrogen/object.h>

namespace H2Core
{

class XMLNode;
class ADSR;
class Drumkit;
class InstrumentLayer;

class DrumkitComponent : public H2Core::Object
{
		H2_OBJECT
	public:
		DrumkitComponent( const int id, const QString& name );
		DrumkitComponent( DrumkitComponent* other );
		~DrumkitComponent();

		void save_to( XMLNode* node );
		static DrumkitComponent* load_from( XMLNode* node, const QString& dk_path );

		void load_from( Drumkit* drumkit, DrumkitComponent* component, bool is_live = true );

		void set_name( const QString& name );
		const QString& get_name() const;

		void set_id( const int id );
		int get_id() const;

		void set_volume( float volume );
		float get_volume() const;

		void set_muted( bool active );
		bool is_muted() const;

		void set_soloed( bool soloed );
		bool is_soloed() const;

		void set_peak_l( float val );
		float get_peak_l() const;
		void set_peak_r( float val );
		float get_peak_r() const;

		void reset_outs( uint32_t nFrames );
		void set_outs( int nBufferPos, float valL, float valR );
		float get_out_L( int nBufferPos );
		float get_out_R( int nBufferPos );

	private:
		int __id;
		QString __name;
		float __volume;
		bool __muted;
		bool __soloed;

		float __peak_l;
		float __peak_r;

		float *__out_L;
		float *__out_R;
};

// DEFINITIONS

inline void DrumkitComponent::set_name( const QString& name )
{
	__name = name;
}

inline const QString& DrumkitComponent::get_name() const
{
	return __name;
}

inline void DrumkitComponent::set_id( const int id )
{
	__id = id;
}

inline int DrumkitComponent::get_id() const
{
	return __id;
}

inline void DrumkitComponent::set_volume( float volume )
{
	__volume = volume;
}

inline float DrumkitComponent::get_volume() const
{
	return __volume;
}

inline void DrumkitComponent::set_muted( bool muted )
{
	__muted = muted;
}

inline bool DrumkitComponent::is_muted() const
{
	return __muted;
}

inline void DrumkitComponent::set_soloed( bool soloed )
{
	__soloed = soloed;
}

inline bool DrumkitComponent::is_soloed() const
{
	return __soloed;
}

inline void DrumkitComponent::set_peak_l( float val )
{
	__peak_l = val;
}

inline float DrumkitComponent::get_peak_l() const
{
	return __peak_l;
}

inline void DrumkitComponent::set_peak_r( float val )
{
	__peak_r = val;
}

inline float DrumkitComponent::get_peak_r() const
{
	return __peak_r;
}

};


#endif
