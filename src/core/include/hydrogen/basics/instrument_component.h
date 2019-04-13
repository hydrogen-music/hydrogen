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

#ifndef H2C_INSTRUMENTCOMPONENT_H
#define H2C_INSTRUMENTCOMPONENT_H

#include <cassert>
#include <vector>

#include <hydrogen/object.h>

namespace H2Core
{

class XMLNode;
class ADSR;
class Drumkit;
class InstrumentLayer;
class DrumkitComponent;

class InstrumentComponent : public H2Core::Object
{
		H2_OBJECT
	public:
		InstrumentComponent( int related_drumkit_componentID );
		InstrumentComponent( InstrumentComponent* other );
		~InstrumentComponent();

		void				save_to( XMLNode* node, int component_id );
		static InstrumentComponent* 	load_from( XMLNode* node, const QString& dk_path );

		InstrumentLayer*		operator[]( int ix );
		InstrumentLayer*		get_layer( int idx );
		void				set_layer( InstrumentLayer* layer, int idx );

		void				set_drumkit_componentID( int related_drumkit_componentID );
		int				get_drumkit_componentID();

		void				set_gain( float gain );
		float				get_gain() const;

		/**  @return #m_iMaxLayers.*/
		static int			getMaxLayers();
		/** @param layers Sets #m_iMaxLayers.*/
		static void			setMaxLayers( int layers );

	private:
		/** Component ID of the drumkit. It is set by
		    set_drumkit_componentID() and
		    accessed via get_drumkit_componentID(). */
		int				__related_drumkit_componentID;
		float				__gain;
		
		/** Maximum number of layers to be used in the
		 *  Instrument editor.
		 *
		 * It is set by setMaxLayers(), queried by
		 * getMaxLayers(), and inferred from
		 * Preferences::m_iMaxLayers. Default value assigned in
		 * Preferences::Preferences(): 16. */
		static int			m_iMaxLayers;
		std::vector<InstrumentLayer*>	__layers;
};

// DEFINITIONS
/** Sets the component ID #__related_drumkit_componentID
 * \param related_drumkit_componentID New value for the component ID */
inline void InstrumentComponent::set_drumkit_componentID( int related_drumkit_componentID )
{
	__related_drumkit_componentID = related_drumkit_componentID;
}
/** Returns the component ID of the drumkit.
 * \return #__related_drumkit_componentID */
inline int InstrumentComponent::get_drumkit_componentID()
{
	return __related_drumkit_componentID;
}

inline void InstrumentComponent::set_gain( float gain )
{
	__gain = gain;
}

inline float InstrumentComponent::get_gain() const
{
	return __gain;
}

inline InstrumentLayer* InstrumentComponent::operator[]( int idx )
{
	assert( idx >= 0 && idx < m_iMaxLayers );
	return __layers[ idx ];
}

inline InstrumentLayer* InstrumentComponent::get_layer( int idx )
{
	assert( idx >= 0 && idx < m_iMaxLayers );
	return __layers[ idx ];
}

};


#endif
