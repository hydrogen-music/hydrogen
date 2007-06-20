/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#ifndef LADSPA_FX_H
#define LADSPA_FX_H

#ifdef LADSPA_SUPPORT

#include <QLibrary>

#include <vector>
#include <list>
#include "ladspa.h"
#include <hydrogen/Object.h>

namespace H2Core {

class LadspaFXInfo : public Object
{
	public:
		LadspaFXInfo( const std::string& sName );
		~LadspaFXInfo();

		std::string m_sFilename;	///< plugin filename
		std::string m_sID;
		std::string m_sLabel;
		std::string m_sName;
		std::string m_sMaker;
		std::string m_sCopyright;
		unsigned m_nICPorts;	///< input control port
		unsigned m_nOCPorts;	///< output control port
		unsigned m_nIAPorts;	///< input audio port
		unsigned m_nOAPorts;	///< output audio port
};



class LadspaFXGroup : public Object
{
	public:
		LadspaFXGroup( const std::string& sName );
		~LadspaFXGroup();

		const std::string& getName() {	return m_sName;	}

		void addLadspaInfo( LadspaFXInfo *pInfo);
		std::vector<LadspaFXInfo*> getLadspaInfo() {	return m_ladspaList;	}

		void addChild( LadspaFXGroup *pChild );
		std::vector<LadspaFXGroup*> getChildList() {	return m_childGroups;	}

	private:
		std::string m_sName;
		std::vector<LadspaFXInfo*> m_ladspaList;
		std::vector<LadspaFXGroup*> m_childGroups;
};



class LadspaControlPort : public Object
{
	public:
		std::string sName;
		bool isToggle;
		bool m_bIsInteger;
		LADSPA_Data fControlValue;
		LADSPA_Data fLowerBound;
		LADSPA_Data fUpperBound;

		LadspaControlPort() : Object( "LadspaControlPort" ) { }
};



class LadspaFX : public Object
{
	public:
		enum {
			MONO_FX,
			STEREO_FX,
			UNDEFINED
		};

		//unsigned m_nBufferSize;

		float* m_pBuffer_L;
		float* m_pBuffer_R;

		std::vector<LadspaControlPort*> inputControlPorts;
		std::vector<LadspaControlPort*> outputControlPorts;

		~LadspaFX();

		void connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R );
		void activate();
		void deactivate();
		void processFX( unsigned nFrames );


		const std::string& getPluginLabel() {	return m_sLabel;	}

		const std::string& getPluginName() {	return m_sName;	}
		void setPluginName( const std::string& sName ) {	m_sName = sName;	}

		const std::string& getLibraryPath() {	return m_sLibraryPath;	}

		bool isEnabled() {	return m_bEnabled;	}
		void setEnabled( bool value ) {	m_bEnabled = value;	}

		static LadspaFX* load( const std::string& sLibraryPath,  const std::string& sPluginLabel, long nSampleRate );

		int getPluginType() {	return m_pluginType;	}

		void setVolume( float fValue );
		float getVolume() {	return m_fVolume;	}


	private:
		bool m_pluginType;
		bool m_bEnabled;
		std::string m_sLabel;
		std::string m_sName;
		std::string m_sLibraryPath;

		QLibrary *m_pLibrary;

		const LADSPA_Descriptor * m_d;
		LADSPA_Handle m_handle;
		float m_fVolume;

		unsigned m_nICPorts;	///< input control port
		unsigned m_nOCPorts;	///< output control port
		unsigned m_nIAPorts;	///< input audio port
		unsigned m_nOAPorts;	///< output audio port


		LadspaFX( const std::string& sLibraryPath, const std::string& sPluginLabel );
};

};

#endif

#endif // LADSPA_SUPPORT
