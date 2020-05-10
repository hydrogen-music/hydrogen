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
#ifndef LV2_FX_H
#define LV2_FX_H

#include "hydrogen/config.h"
#if defined(H2CORE_HAVE_LILV) || _DOXYGEN_

#include <vector>
#include <list>
#include "ladspa.h"
#include <hydrogen/object.h>
#include <lilv-0/lilv/lilv.h>

namespace H2Core
{





class Lv2FX : public H2Core::Object
{
	H2_OBJECT
public:
	enum {
		MONO_FX,
		STEREO_FX,
		UNDEFINED
	};

	//unsigned m_nBufferSize;

	float* m_pBuffer_L;
	float* m_pBuffer_R;

	//std::vector<LadspaControlPort*> inputControlPorts;
	//std::vector<LadspaControlPort*> outputControlPorts;

	~Lv2FX();

	void connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R );
	void activate();
	void deactivate();
	void processFX( unsigned nFrames );


	const QString& getPluginLabel() {
		return m_sURI;
	}

	const QString& getPluginName() {
		return m_sName;
	}
	void setPluginName( const QString& sName ) {
		m_sName = sName;
	}

	bool isEnabled() {
		return m_bEnabled;
	}
	void setEnabled( bool value ) {
		m_bEnabled = value;
	}

	static Lv2FX* load(const QString& sPluginURI, long nSampleRate );

	int getPluginType() {
		return m_pluginType;
	}

	void setVolume( float fValue );
	float getVolume() {
		return m_fVolume;
	}


private:
	bool m_pluginType;
	bool m_bEnabled;
	bool m_bActivated;	// Guard against plugins that can't be deactivated before being activated (
	QString m_sURI;
	QString m_sName;

	LilvInstance * m_pLilvInstance;
	LADSPA_Handle m_handle;
	float m_fVolume;

	unsigned m_nICPorts;	///< input control port
	unsigned m_nOCPorts;	///< output control port
	unsigned m_nIAPorts;	///< input audio port
	unsigned m_nOAPorts;	///< output audio port

	const LilvPort*		m_pAudioInput;
	const LilvPort*		m_pAudioOutput;
	
	std::vector<float>	m_fDefaultValues;
	
	Lv2FX( LilvWorld* pWorld, const LilvPlugin* plugin, long nSampleRate );
};

};

#endif

#endif // H2CORE_HAVE_LADSPA
