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
#ifndef LADSPA_FX_H
#define LADSPA_FX_H

#include "hydrogen/config.h"
#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_

#include <QLibrary>

#include <vector>
#include <list>
#include "ladspa.h"
#include <hydrogen/object.h>
#include "H2FX.h"


namespace H2Core
{

class LadspaFXInfo;
class LV2FXInfo;


class LadspaFXInfo : public H2Core::H2FXInfo
{
	H2_OBJECT
public:
	LadspaFXInfo( const QString& sName );
	~LadspaFXInfo();
	
	LV2FXInfo*		isLV2FXInfo() override;
	LadspaFXInfo*	isLadspaFXInfo() override;
};

class LadspaFX : public H2Core::H2FX
{
	H2_OBJECT
public:

	virtual ~LadspaFX();
	
	virtual LadspaFX* isLadspaFX();

	virtual void connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R ) override;
	virtual void activate() override;
	virtual void deactivate() override;
	virtual void processFX( unsigned nFrames ) override;


	const QString& getPluginLabel() {
		return m_sLabel;
	}

	const QString& getLibraryPath() {
		return m_sLibraryPath;
	}

	static LadspaFX* load( const QString& sLibraryPath, const QString& sPluginLabel, long nSampleRate );



private:
	bool m_bActivated;	// Guard against plugins that can't be deactivated before being activated (
	QString m_sLabel;
	QString m_sLibraryPath;

	QLibrary *m_pLibrary;

	const LADSPA_Descriptor * m_d;
	LADSPA_Handle m_handle;

	unsigned m_nICPorts;	///< input control port
	unsigned m_nOCPorts;	///< output control port
	unsigned m_nIAPorts;	///< input audio port
	unsigned m_nOAPorts;	///< output audio port


	LadspaFX( const QString& sLibraryPath, const QString& sPluginLabel );
};

};

#endif

#endif // H2CORE_HAVE_LADSPA
