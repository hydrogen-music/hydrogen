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



namespace H2Core
{

class LadspaFXInfo;
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

class LadspaFXInfo : public H2Core::H2FXInfo
{
	H2_OBJECT
public:
	LadspaFXInfo( const QString& sName );
	~LadspaFXInfo();
	
	LV2FXInfo*		isLV2FXInfo() override;
	LadspaFXInfo*	isLadspaFXInfo() override;
};


class LV2FXInfo : public H2Core::H2FXInfo
{
	H2_OBJECT
public:
	LV2FXInfo( const QString& sName );
	~LV2FXInfo();
	
	LV2FXInfo*		isLV2FXInfo() override;
	LadspaFXInfo*	isLadspaFXInfo() override;
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

	void addLadspaH2Info( H2FXInfo *pInfo );
	
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
	std::vector<H2FXInfo*>	m_ladspaList;
	std::vector<H2FXInfo*>		m_groupContent;
	std::vector<H2FXGroup*>		m_childGroups;
};



class LadspaControlPort : public H2Core::Object
{
	H2_OBJECT
public:
	QString sName;
	bool isToggle;
	bool m_bIsInteger;
	LADSPA_Data fDefaultValue;
	LADSPA_Data fControlValue;
	LADSPA_Data fLowerBound;
	LADSPA_Data fUpperBound;

	LadspaControlPort() : Object( "LadspaControlPort" ) { }
};



class LadspaFX : public H2Core::Object
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

	std::vector<LadspaControlPort*> inputControlPorts;
	std::vector<LadspaControlPort*> outputControlPorts;

	~LadspaFX();

	void connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R );
	void activate();
	void deactivate();
	void processFX( unsigned nFrames );


	const QString& getPluginLabel() {
		return m_sLabel;
	}

	const QString& getPluginName() {
		return m_sName;
	}
	void setPluginName( const QString& sName ) {
		m_sName = sName;
	}

	const QString& getLibraryPath() {
		return m_sLibraryPath;
	}

	bool isEnabled() {
		return m_bEnabled;
	}
	void setEnabled( bool value ) {
		m_bEnabled = value;
	}

	static LadspaFX* load( const QString& sLibraryPath, const QString& sPluginLabel, long nSampleRate );

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
