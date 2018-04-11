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
#include <hydrogen/basics/instrument_layer.h>

namespace H2Core
{

class XMLNode;
class ADSR;
class Drumkit;
class DrumkitComponent;

class InstrumentComponent : public H2Core::Object
{
		H2_OBJECT
	public:
		//InstrumentComponent( const int id, const QString& name );
		InstrumentComponent( int related_drumkit_componentID );
		InstrumentComponent( InstrumentComponent* other );
		~InstrumentComponent();

		void save_to( XMLNode* node, int component_id );
		static InstrumentComponent* load_from( XMLNode* node, const QString& dk_path );

		InstrumentLayer* operator[]( int idx );
		InstrumentLayer* get_layer( int idx );
		void set_layer( InstrumentLayer* layer, int idx );

		void set_drumkit_componentID( int related_drumkit_componentID );
		int get_drumkit_componentID();

		//void set_name( const QString& name );
		//const QString& get_name() const;

		//void set_id( const int id );
		//int get_id() const;

		void set_gain( float gain );
		float get_gain() const;

		//void set_volume( float volume );
		//float get_volume() const;

		static int getMaxLayers( );
		static void setMaxLayers( int layers );

	private:
		int __related_drumkit_componentID;
		//QString __name;
		float __gain;
		//float __volume;
		static int maxLayers;
		std::vector<InstrumentLayer*> __layers;
};

// DEFINITIONS

inline void InstrumentComponent::set_drumkit_componentID( int related_drumkit_componentID )
{
	__related_drumkit_componentID = related_drumkit_componentID;
}

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
	assert( idx >= 0 && idx < maxLayers );
	return __layers[ idx ];
}

inline InstrumentLayer* InstrumentComponent::get_layer( int idx )
{
	assert( idx >= 0 && idx < maxLayers );
	return __layers[ idx ];
}

inline void InstrumentComponent::set_layer( InstrumentLayer* layer, int idx )
{
	assert( idx >= 0 && idx < maxLayers );
	if ( __layers[ idx ] ) {
		delete __layers[ idx ];
	}
	__layers[ idx ] = layer;
}

};


#endif
