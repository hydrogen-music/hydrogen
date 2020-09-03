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

#include <hydrogen/object.h>

namespace H2Core
{

class LadspaFX;
class LadspaFXInfo;

class Lv2FX;
class LV2FXInfo;


class H2FXInfo : public H2Core::Object
{
	H2_OBJECT
public:
	 H2FXInfo( const QString& sName );
	 ~H2FXInfo();

	QString m_sFilename;	///< plugin filename
	QString m_sID;
	QString m_sLabel;
	QString m_sName;
	QString m_sMaker;
	QString m_sCopyright;
	
	unsigned m_nICPorts;	///< input control port
	unsigned m_nOCPorts;	///< output control port
	unsigned m_nIAPorts;	///< input audio port
	unsigned m_nOAPorts;	///< output audio port
	
	static bool				alphabeticOrder( H2FXInfo* a, H2FXInfo* b );
	
	virtual LV2FXInfo*		isLV2FXInfo();
	virtual LadspaFXInfo*	isLadspaFXInfo();
};

class H2FXGroup : public H2Core::Object
{
	H2_OBJECT
public:
	H2FXGroup( const QString& sName );
	 ~H2FXGroup();

	const QString& getName() {
		return m_sName;
	}

	void addLadspaInfo( H2FXInfo *pInfo );
	std::vector<H2FXInfo*> getLadspaInfo() {
		return m_ladspaList;
	}

	void addChild( H2FXGroup *pChild );
	std::vector<H2FXGroup*> getChildList() {
		return m_childGroups;
	}

	void clear() {
		m_childGroups.clear();
		m_ladspaList.clear();
	}

	static bool alphabeticOrder( H2FXGroup*, H2FXGroup* );
	void sort();


private:
	QString m_sName;
	std::vector<H2FXInfo*>		m_ladspaList;
	std::vector<H2FXGroup*>		m_childGroups;
};

class LadspaControlPort : public H2Core::Object
{
	H2_OBJECT
public:
	QString		sName;
	bool		isToggle;
	bool		m_bIsInteger;
	float		fDefaultValue;
	float		fControlValue;
	float		fLowerBound;
	float		fUpperBound;
	int			nPortIndex;

	LadspaControlPort() : Object( "LadspaControlPort" ) { }
};


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
		
		const QString& getPluginName() {
			return m_sName;
		}
		void setPluginName( const QString& sName ) {
			m_sName = sName;
		}
			
		float* m_pBuffer_L;
		float* m_pBuffer_R;
		
		std::vector<LadspaControlPort*> inputControlPorts;
		std::vector<LadspaControlPort*> outputControlPorts;
		
	protected:
		QString m_sName;
		bool	m_pluginType;
		float	m_fVolume;
		bool	m_bEnabled;
};

};

#endif
