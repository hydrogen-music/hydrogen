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

#ifndef H2C_DRUMKITCOMPONENT_H
#define H2C_DRUMKITCOMPONENT_H

#include <cassert>
#include <memory>
#include <inttypes.h>
#include <core/Object.h>

namespace H2Core
{

class XMLNode;
class ADSR;
class Drumkit;
class InstrumentLayer;

/** \ingroup docCore docDataStructure */
class DrumkitComponent : public H2Core::Object<DrumkitComponent>
{
		H2_OBJECT(DrumkitComponent)
	public:
		DrumkitComponent( const int id, const QString& name );
		DrumkitComponent( std::shared_ptr<DrumkitComponent> other );
		~DrumkitComponent();

		void						save_to( XMLNode* node );
		static std::shared_ptr<DrumkitComponent>	load_from( XMLNode* node );

		void						load_from( std::shared_ptr<DrumkitComponent> component );

		void						set_name( const QString& name );
		const QString&				get_name() const;

		void						set_id( const int id );
		int							get_id() const;

		void						set_volume( float volume );
		float						get_volume() const;

		void						set_muted( bool active );
		bool						is_muted() const;

		void						set_soloed( bool soloed );
		bool						is_soloed() const;

		void						set_peak_l( float val );
		float						get_peak_l() const;
		void						set_peak_r( float val );
		float						get_peak_r() const;

		void						reset_outs( uint32_t nFrames );
		void						set_outs( int nBufferPos, float valL, float valR );
		float						get_out_L( int nBufferPos );
		float						get_out_R( int nBufferPos );
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
		int		__id;
	        /** Name of the DrumkitComponent. It is set by
		    set_name() and accessed via get_name().*/
		QString		__name;
		float		__volume;
		bool		__muted;
		bool		__soloed;

		float		__peak_l;
		float		__peak_r;

		float *		__out_L;
		float *		__out_R;
};

// DEFINITIONS
/** Sets the name of the DrumkitComponent #__name.
 * \param name New name. */
inline void DrumkitComponent::set_name( const QString& name )
{
	__name = name;
}
/** Access the name of the DrumkitComponent.
 * \return #__name */
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

inline void DrumkitComponent::set_outs( int nBufferPos, float valL, float valR )
{
	__out_L[nBufferPos] += valL;
	__out_R[nBufferPos] += valR;
}

};

#endif
