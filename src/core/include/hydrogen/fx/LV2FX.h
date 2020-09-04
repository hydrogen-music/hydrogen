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
#include <hydrogen/object.h>
#include <lilv-0/lilv/lilv.h>
#include "H2FX.h"

namespace H2Core
{

class LadspaFXInfo;

class LV2FXInfo : public H2Core::H2FXInfo
{
	H2_OBJECT
public:
	LV2FXInfo( const QString& sName );
	~LV2FXInfo();
	
	LV2FXInfo*		isLV2FXInfo() override;
	LadspaFXInfo*	isLadspaFXInfo() override;
};

class Lv2FX : public H2Core::H2FX
{
	H2_OBJECT
public:

	virtual ~Lv2FX();

	virtual void connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R ) override;
	virtual void activate() override;
	virtual void deactivate() override;
	virtual void processFX( unsigned nFrames ) override;
	
	virtual Lv2FX* isLv2FX() override;

	const QString& getPluginLabel() {
		return m_sURI;
	}
	
	void setPluginLabel(const QString& sLabel) {
		m_sURI = sLabel;
	}

	static Lv2FX* load(const QString& sPluginURI, long nSampleRate );


private:
	bool m_bActivated;	// Guard against plugins that can't be deactivated before being activated (
	QString m_sURI;

	LilvInstance * m_pLilvInstance;

	unsigned m_nICPorts;	///< input control port
	unsigned m_nOCPorts;	///< output control port
	unsigned m_nIAPorts;	///< input audio port
	unsigned m_nOAPorts;	///< output audio port

	int	m_nAudioInput1Idx;
	int	m_nAudioInput2Idx;
	int	m_nAudioOutput1Idx;
	int	m_nAudioOutput2Idx;
	
	std::vector<std::pair<uint,float>>	m_fDefaultValues;
	
	const LilvPlugin*	m_pPlugin;
	LilvWorld*			m_pWorld;
	
	Lv2FX( LilvWorld* pWorld, const LilvPlugin* plugin, long nSampleRate );
};

};

#endif

#endif // H2CORE_HAVE_LILV
