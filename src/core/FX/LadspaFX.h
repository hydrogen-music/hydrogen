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
#ifndef LADSPA_FX_H
#define LADSPA_FX_H

#include <core/config.h>
#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_

#include <QLibrary>

#include <vector>
#include <list>
#include "ladspa.h"
#include <core/Object.h>

namespace H2Core
{

/** \ingroup docCore docAudioEngine */
class LadspaFXInfo : public H2Core::Object<LadspaFXInfo>
{
	H2_OBJECT(LadspaFXInfo)
public:
	LadspaFXInfo( const QString& sName );
	~LadspaFXInfo();

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
	static bool alphabeticOrder( LadspaFXInfo* a, LadspaFXInfo* b );
};



/** \ingroup docCore docAudioEngine */
class LadspaFXGroup : public H2Core::Object<LadspaFXGroup>
{
	H2_OBJECT(LadspaFXGroup)
public:
	LadspaFXGroup( const QString& sName );
	~LadspaFXGroup();

	const QString& getName() const {
		return m_sName;
	}

	void addLadspaInfo( LadspaFXInfo *pInfo );
	std::vector<LadspaFXInfo*> getLadspaInfo() const {
		return m_ladspaList;
	}

	void addChild( LadspaFXGroup *pChild );
	std::vector<LadspaFXGroup*> getChildList() const {
		return m_childGroups;
	}

	void clear();

	static bool alphabeticOrder( LadspaFXGroup*, LadspaFXGroup* );
	void sort();


private:
	QString m_sName;
	std::vector<LadspaFXInfo*> m_ladspaList;
	std::vector<LadspaFXGroup*> m_childGroups;
};



/** \ingroup docCore docAudioEngine */
class LadspaControlPort : public H2Core::Object<LadspaControlPort>
{
	H2_OBJECT(LadspaControlPort)
public:
	QString sName;
	bool isToggle = false;
	bool m_bIsInteger = false;
	LADSPA_Data fDefaultValue = 0.0;
	LADSPA_Data fControlValue = 0.0;
	LADSPA_Data fLowerBound = 0.0;
	LADSPA_Data fUpperBound = 0.0;

	LadspaControlPort() : Object( ) { }
};



/** \ingroup docCore docAudioEngine */
class LadspaFX : public H2Core::Object<LadspaFX>
{
	H2_OBJECT(LadspaFX)
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


	const QString& getPluginLabel() const {
		return m_sLabel;
	}

	const QString& getPluginName() const {
		return m_sName;
	}
	void setPluginName( const QString& sName );

	const QString& getLibraryPath() const {
		return m_sLibraryPath;
	}

	bool isEnabled() const {
		return m_bEnabled;
	}
	void setEnabled( bool bEnabled );

	static LadspaFX* load( const QString& sLibraryPath, const QString& sPluginLabel, long nSampleRate );

	int getPluginType() const {
		return m_pluginType;
	}

	float getVolume() const {
		return m_fVolume;
	}
	void setVolume( float fVolume );


private:
	bool m_pluginType;
	bool m_bEnabled;
	bool m_bActivated;	// Guard against plugins that can't be deactivated before being activated (
	QString m_sLabel;
	QString m_sName;
	QString m_sLibraryPath;

	QLibrary *m_pLibrary;

	const LADSPA_Descriptor * m_d;
	LADSPA_Handle m_handle;
	float m_fVolume;

	unsigned m_nICPorts;	///< input control port
	unsigned m_nOCPorts;	///< output control port
	unsigned m_nIAPorts;	///< input audio port
	unsigned m_nOAPorts;	///< output audio port


	LadspaFX( const QString& sLibraryPath, const QString& sPluginLabel );
};

};

#endif

#endif // H2CORE_HAVE_LADSPA
