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
#ifndef H2_FX_H
#define H2_FX_H

#include "hydrogen/config.h"
#if defined(H2CORE_HAVE_LILV) || _DOXYGEN_

#include <hydrogen/object.h>

namespace H2Core
{

class Lv2FX;
class LadspaFX;

class H2FX : public H2Core::Object
{
	public:
	
		enum {
			MONO_FX,
			STEREO_FX,
			UNDEFINED
		};
	
		H2FX( const char* class_name )
			: Object( class_name )
			, m_pBuffer_L( nullptr )
			, m_pBuffer_R( nullptr )
			, m_pluginType( UNDEFINED )
			, m_bEnabled( true )
			, m_fVolume( 1.0f ){}
		
		virtual ~H2FX() { }
		
		virtual void connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R ) = 0;		
		virtual void activate() = 0;
		virtual void deactivate()= 0;
		virtual void processFX( unsigned nFrames ) = 0;
		
		virtual Lv2FX* isLv2FX() { return nullptr; }
		virtual LadspaFX* isLadspaFX() { return nullptr; }
		
		virtual const QString& getPluginName() = 0;
		
		int getPluginType() {
			return m_pluginType;
		}
		
		void setVolume( float fValue )
		{
			if ( fValue > 2.0 ) {
				fValue = 2.0;
			} else if ( fValue < 0.0 ) {
				fValue = 0.0;
			}
			m_fVolume = fValue;
		}
		
		float getVolume() {
			return m_fVolume;
		}
		
		bool isEnabled() {
			return m_bEnabled;
		}
		void setEnabled( bool value ) {
			m_bEnabled = value;
		}
			
		float* m_pBuffer_L;
		float* m_pBuffer_R;
		
	protected:
		bool	m_pluginType;
		float	m_fVolume;
		bool	m_bEnabled;
};

};

#endif

#endif // H2CORE_HAVE_LILV
