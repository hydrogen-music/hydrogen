/*
 * Hydrogen
 * Copyright(c) 2002-2004 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: LadspaFX.h,v 1.5 2005/05/24 12:56:17 comix Exp $
 *
 */
#ifndef LADSPA_FX_H
#define LADSPA_FX_H

#include "config.h"
#ifdef LADSPA_SUPPORT


#include <vector>
#include <list>
using namespace std;
#include "ladspa.h"
#include "../Object.h"


#include <qlibrary.h>

class LadspaFXInfo : public Object
{
	public:
		LadspaFXInfo();
		~LadspaFXInfo();

		string sFilename;	///< plugin filename
		string sID;
		string sLabel;
		string sName;
		string sMaker;
		string sCopyright;
		unsigned nICPorts;	///< input control port
		unsigned nOCPorts;	///< output control port
		unsigned nIAPorts;	///< input audio port
		unsigned nOAPorts;	///< output audio port
};



class LadspaFXGroup : public Object
{
	public:
		LadspaFXGroup( string sName );
		~LadspaFXGroup();

		string getName() {	return m_sName;	}

		void addLadspaInfo(LadspaFXInfo *pInfo);
		vector<LadspaFXInfo*> getLadspaInfo() {	return m_ladspaList;	}

		void addChildGroup( LadspaFXGroup *pChild );
		vector<LadspaFXGroup*> getChildGroups() {	return m_childGroups;	}

	private:
		string m_sName;
		vector<LadspaFXInfo*> m_ladspaList;
		vector<LadspaFXGroup*> m_childGroups;
};



class LadspaControlPort : public Object
{
	public:
		string sName;
		bool isToggle;
		LADSPA_Data fControlValue;
		LADSPA_Data fLowerBound;
		LADSPA_Data fUpperBound;

		LadspaControlPort() : Object( "LadspaControlPort") { }
};



class LadspaFX : public Object
{
	public:
		enum {
			MONO_FX,
			STEREO_FX,
			UNDEFINED
		};

		vector<LadspaControlPort*> inputControlPorts;
		vector<LadspaControlPort*> outputControlPorts;

		~LadspaFX();

		void connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R );
		void activate();
		void deactivate();
		void processFX( unsigned nFrames );

		static vector<LadspaFXInfo*> getPluginList();
		static LadspaFXGroup* getLadspaFXGroup();

		string getPluginLabel() {	return m_sLabel;	}

		string getPluginName() {	return m_sName;	}
		void setPluginName( string sName ) {	m_sName = sName;	}

		string getLibraryPath() {	return m_sLibraryPath;	}

		bool isEnabled() {	return m_bEnabled;	}
		void setEnabled( bool value ) {	m_bEnabled = value;	}

		static LadspaFX* load( string sLibraryPath, string sPluginLabel, long nSampleRate );

		int getPluginType() {	return m_pluginType;	}

		void setVolume( float fValue );
		float getVolume() {	return m_fVolume;	}

	private:
		bool m_pluginType;
		bool m_bEnabled;
		string m_sLabel;
		string m_sName;
		string m_sLibraryPath;
		
		//void* m_libraryModule;
		QLibrary *m_pLibrary;
		
		const LADSPA_Descriptor * m_d;
		LADSPA_Handle m_handle;
		float m_fVolume;

		unsigned m_nICPorts;	///< input control port
		unsigned m_nOCPorts;	///< output control port
		unsigned m_nIAPorts;	///< input audio port
		unsigned m_nOAPorts;	///< output audio port

		LadspaFX( string sLibraryPath, string sPluginLabel );

		static void RDFDescend( string sBase, LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList );
		static void getRDF( LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList );

};

#endif

#endif // LADSPA_SUPPORT
