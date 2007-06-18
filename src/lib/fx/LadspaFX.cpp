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
 * $Id: LadspaFX.cpp,v 1.9 2005/07/05 15:19:19 comix Exp $
 *
 */

#include "config.h"

#ifdef LADSPA_SUPPORT


#include "LadspaFX.h"
#include "../Preferences.h"
#include <qdir.h>

//#include <dlfcn.h>
#include <cstdio>
#include <vector>
using namespace std;

#ifdef HAVE_LRDF_H
#include <lrdf.h>
#endif


#define LADSPA_IS_CONTROL_INPUT(x) (LADSPA_IS_PORT_INPUT(x) && LADSPA_IS_PORT_CONTROL(x))
#define LADSPA_IS_AUDIO_INPUT(x) (LADSPA_IS_PORT_INPUT(x) && LADSPA_IS_PORT_AUDIO(x))
#define LADSPA_IS_CONTROL_OUTPUT(x) (LADSPA_IS_PORT_OUTPUT(x) && LADSPA_IS_PORT_CONTROL(x))
#define LADSPA_IS_AUDIO_OUTPUT(x) (LADSPA_IS_PORT_OUTPUT(x) && LADSPA_IS_PORT_AUDIO(x))

LadspaFXGroup::LadspaFXGroup( string sName ) : Object("LadspaFXGroup")
{
//	infoLog( "INIT - " + sName );
	m_sName = sName;
}


LadspaFXGroup::~LadspaFXGroup()
{
//	infoLog( "DESTROY - " + m_sName );
	for( unsigned i = 0; i < m_ladspaList.size(); i++) {
		delete m_ladspaList[i];
	}
	for( unsigned i = 0; i < m_childGroups.size(); i++) {
		delete m_childGroups[i];
	}
}



void LadspaFXGroup::addLadspaInfo(LadspaFXInfo *pInfo)
{
	m_ladspaList.push_back( pInfo );
}


void LadspaFXGroup::addChildGroup( LadspaFXGroup *pChild )
{
	m_childGroups.push_back( pChild );
}



////////////////


LadspaFXInfo::LadspaFXInfo() : Object( "LadspaFXInfo" )
{
//	infoLog( "INIT" );
	sFilename = "";
	sLabel = "";
	sName = "";
	nICPorts = 0;
	nOCPorts = 0;
	nIAPorts = 0;
	nOAPorts = 0;
}


LadspaFXInfo::~LadspaFXInfo()
{
//	infoLog( "DESTROY" );
}


///////////////////


// ctor
LadspaFX::LadspaFX( string sLibraryPath, string sPluginLabel )
 : Object( "LadspaFX   " )
 , m_pLibrary( NULL )
{
	infoLog( string("INIT - ") + sLibraryPath + " - " + sPluginLabel );

	m_sLibraryPath = sLibraryPath;
	m_bEnabled = false;
	m_sLabel = sPluginLabel;
	m_handle = NULL;
	m_nICPorts = 0;
	m_nOCPorts = 0;
	m_nIAPorts = 0;
	m_nOAPorts = 0;
	m_d = NULL;
	//m_libraryModule = NULL;
	m_pluginType = UNDEFINED;
	m_fVolume = 1.0;
}


// dtor
LadspaFX::~LadspaFX()
{
	// dealloca il plugin
	infoLog( string("DESTROY - ") + m_sLibraryPath + " - " + m_sLabel );


	if (m_d) {
		if (m_d->deactivate) {
			if ( m_handle ) {
				infoLog( "deactivate" );
				m_d->deactivate( m_handle );
			}
		}

		if (m_d->cleanup) {
			if ( m_handle ) {
				infoLog( "Cleanup" );
				m_d->cleanup( m_handle );
			}
		}
	}
/*	if (m_libraryModule) {
		dlclose( m_libraryModule );
	}*/
	delete m_pLibrary;

	for (unsigned i = 0; i < inputControlPorts.size(); i++) {
		delete inputControlPorts[i];
	}
	for (unsigned i = 0; i < outputControlPorts.size(); i++) {
		delete outputControlPorts[i];
	}

}



// Static
LadspaFX* LadspaFX::load( string sLibraryPath, string sPluginLabel, long nSampleRate )
{
	LadspaFX* pFX = new LadspaFX( sLibraryPath, sPluginLabel);

	Logger::getInstance()->log( "[LadspaFX::load] INIT - " + sLibraryPath + " - " + sPluginLabel );

	pFX->m_pLibrary = new QLibrary( QString( sLibraryPath.c_str() ) );
	LADSPA_Descriptor_Function desc_func = (LADSPA_Descriptor_Function)pFX->m_pLibrary->resolve( "ladspa_descriptor" );
	if ( desc_func == NULL ) {
		pFX->errorLog( "[LadspaFX::load] Error loading the library. (" + sLibraryPath + ")" );
		delete pFX;
		return NULL;
	}
	if ( desc_func ) {
		for ( unsigned i = 0; ( pFX->m_d = desc_func( i ) ) != NULL; i++ ) {
			string sName =pFX->m_d->Name;
			string sLabel = pFX->m_d->Label;

			if (sLabel != sPluginLabel) {
				continue;
			}
			pFX->setPluginName( sName );

			for (unsigned j = 0; j < pFX->m_d->PortCount; j++) {
				LADSPA_PortDescriptor pd = pFX->m_d->PortDescriptors[j];
				if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
					pFX->m_nICPorts++;
				}
				else if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
					pFX->m_nIAPorts++;
				}
				else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
					pFX->m_nOCPorts++;
				}
				else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
					pFX->m_nOAPorts++;
				}
				else {
					pFX->errorLog( "[LadspaFX::load] Unknown port type" );
				}
			}
			break;
		}
	}
	else {
		pFX->errorLog( "[LadspaFX::load] Error in dlsym" );
		delete pFX;
		return NULL;
	}

	if ( ( pFX->m_nIAPorts == 2 ) && ( pFX->m_nOAPorts == 2 ) ) {		// Stereo plugin
		pFX->m_pluginType = STEREO_FX;
	}
	else if ( ( pFX->m_nIAPorts == 1 ) && ( pFX->m_nOAPorts == 1 ) ) {	// Mono plugin
		pFX->m_pluginType = MONO_FX;
	}
	else {
		pFX->errorLog( "[load] Wrong number of ports" );
		pFX->errorLog( "[load] in audio = " + toString(pFX->m_nIAPorts) );
		pFX->errorLog( "[load] out audio = " + toString(pFX->m_nOAPorts) );
	}

	//pFX->infoLog( "[LadspaFX::load] instantiate " + pFX->getPluginName() );
	pFX->m_handle = pFX->m_d->instantiate( pFX->m_d, nSampleRate);

	for ( unsigned nPort = 0; nPort < pFX->m_d->PortCount; nPort++) {
		LADSPA_PortDescriptor pd = pFX->m_d->PortDescriptors[ nPort ];

		if ( LADSPA_IS_CONTROL_INPUT( pd ) ) {
			string sName = pFX->m_d->PortNames[ nPort ];
			float fMin = 0.0;
			float fMax = 0.0;
			float fDefault = 0.0;
			bool isToggle = false;

			LADSPA_PortRangeHint rangeHints = pFX->m_d->PortRangeHints[ nPort ];
			if ( LADSPA_IS_HINT_BOUNDED_BELOW( rangeHints.HintDescriptor ) ) {
				fMin = ( pFX->m_d->PortRangeHints[ nPort ] ).LowerBound;
			}
			if ( LADSPA_IS_HINT_BOUNDED_ABOVE( rangeHints.HintDescriptor ) ) {
				fMax = ( pFX->m_d->PortRangeHints[ nPort ] ).UpperBound;
			}
			if ( LADSPA_IS_HINT_TOGGLED( rangeHints.HintDescriptor ) ) {
				isToggle = true;
				// temporaneo, solo per permettere l'uso di un fader normale
				fMin = 0.0;
				fMax = 1.0;
			}
			if ( LADSPA_IS_HINT_SAMPLE_RATE( rangeHints.HintDescriptor ) ) {
				pFX->warningLog( "[LadspaFX::load] samplerate hint not implemented yet" );
			}
			if ( LADSPA_IS_HINT_LOGARITHMIC( rangeHints.HintDescriptor ) ) {
				pFX->warningLog( "[LadspaFX::load] logarithmic hint not implemented yet" );
			}
			if ( LADSPA_IS_HINT_INTEGER( rangeHints.HintDescriptor ) ) {
				pFX->warningLog( "[LadspaFX::load] integer hint not implemented yet" );
			}
			if ( LADSPA_IS_HINT_HAS_DEFAULT( rangeHints.HintDescriptor ) ) {
				if ( LADSPA_IS_HINT_DEFAULT_MINIMUM( rangeHints.HintDescriptor ) ) {
					fDefault = fMin;
				}
				if ( LADSPA_IS_HINT_DEFAULT_LOW( rangeHints.HintDescriptor ) ) {
					// TODO: bisogna gestire diversamente se viene specificato di usare la scala logaritmica
					fDefault = (fMin * 0.75 + fMax * 0.25);
				}
				if ( LADSPA_IS_HINT_DEFAULT_MIDDLE( rangeHints.HintDescriptor ) ) {
					fDefault = (fMax - fMin) / 2.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_HIGH( rangeHints.HintDescriptor ) ) {
					// TODO: bisogna gestire diversamente se viene specificato di usare la scala logaritmica
					fDefault = (fMin * 0.25 + fMax * 0.75);
				}
				if ( LADSPA_IS_HINT_DEFAULT_MAXIMUM( rangeHints.HintDescriptor ) ) {
					fDefault = fMax;
				}
				if ( LADSPA_IS_HINT_DEFAULT_0( rangeHints.HintDescriptor ) ) {
					fDefault = 0.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_1( rangeHints.HintDescriptor ) ) {
					fDefault = 1.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_100( rangeHints.HintDescriptor ) ) {
					fDefault = 100.0;
				}
				if ( LADSPA_IS_HINT_DEFAULT_440( rangeHints.HintDescriptor ) ) {
					fDefault = 440.0;
				}
			}

			LadspaControlPort* pControl = new LadspaControlPort();
			pControl->sName = sName;
			pControl->fLowerBound = fMin;
			pControl->fUpperBound = fMax;
			pControl->fControlValue = fDefault;
			pControl->isToggle = isToggle;

			pFX->infoLog( "[LadspaFX::load] Input control port\t[" + sName + "]\tmin=" + toString(fMin) + ",\tmax=" + toString(fMax) + ",\tcontrolValue=" + toString(pControl->fControlValue) );

			pFX->inputControlPorts.push_back( pControl );
			pFX->m_d->connect_port( pFX->m_handle, nPort, &(pControl->fControlValue) );
		}
		else if ( LADSPA_IS_CONTROL_OUTPUT( pd ) ) {
			string sName = pFX->m_d->PortNames[ nPort ];
			float fMin = 0.0;
			float fMax = 0.0;
			float fDefault = 0.0;

			LADSPA_PortRangeHint rangeHints = pFX->m_d->PortRangeHints[ nPort ];
			if ( LADSPA_IS_HINT_BOUNDED_BELOW( rangeHints.HintDescriptor ) ) {
				fMin = ( pFX->m_d->PortRangeHints[ nPort ] ).LowerBound;
			}
			if ( LADSPA_IS_HINT_BOUNDED_ABOVE( rangeHints.HintDescriptor ) ) {
				fMax = ( pFX->m_d->PortRangeHints[ nPort ] ).UpperBound;
			}

/*			LadspaControlPort* pControl = new LadspaControlPort();
			pControl->sName = pFX->m_d->PortNames[ nPort ];
			pControl->fLowerBound = ( pFX->m_d->PortRangeHints[ nPort ] ).LowerBound;
			pControl->fUpperBound = ( pFX->m_d->PortRangeHints[ nPort ] ).UpperBound;
			pControl->fControlValue = pControl->fUpperBound / 2.0;
*/
			// always middle
			fDefault = (fMax - fMin) / 2.0;

			LadspaControlPort* pControl = new LadspaControlPort();
			pControl->sName = sName;
			pControl->fLowerBound = fMin;
			pControl->fUpperBound = fMax;
			pControl->fControlValue = fDefault;
			//pFX->infoLog( "[LadspaFX::load] Output control port\t[" + sName + "]\tmin=" + toString(fMin) + ",\tmax=" + toString(fMax) + ",\tcontrolValue=" + toString(pControl->fControlValue) );

			pFX->outputControlPorts.push_back( pControl );
			pFX->m_d->connect_port( pFX->m_handle, nPort, &(pControl->fControlValue) );
		}
		else if ( LADSPA_IS_AUDIO_INPUT( pd ) ) {
		}
		else if ( LADSPA_IS_AUDIO_OUTPUT( pd ) ) {
		}
		else {
			pFX->errorLog( "[LadspaFX::load] unknown port" );
		}
	}

	return pFX;
}



void LadspaFX::connectAudioPorts( float* pIn_L, float* pIn_R, float* pOut_L, float* pOut_R )
{
	infoLog( "[connectAudioPorts]" );

	unsigned nAIConn = 0;
	unsigned nAOConn = 0;
	for ( unsigned nPort = 0; nPort < m_d->PortCount; nPort++) {
		LADSPA_PortDescriptor pd = m_d->PortDescriptors[ nPort ];
		if ( LADSPA_IS_CONTROL_INPUT( pd ) ) {
		}
		else if ( LADSPA_IS_CONTROL_OUTPUT( pd ) ) {
		}
		else if ( LADSPA_IS_AUDIO_INPUT( pd ) ) {
			if (nAIConn == 0) {
				m_d->connect_port( m_handle, nPort, pIn_L );
				//infoLog( "connect input port (L): " + string( m_d->PortNames[ nPort ] ) );
			}
			else if (nAIConn == 1) {
				m_d->connect_port( m_handle, nPort, pIn_R );
				//infoLog( "connect input port (R): " + string( m_d->PortNames[ nPort ] ) );
			}
			else {
				errorLog( "[connectAudioPorts] too many input ports.." );
			}
			nAIConn++;
		}
		else if ( LADSPA_IS_AUDIO_OUTPUT( pd ) ) {
			if (nAOConn == 0) {
				m_d->connect_port( m_handle, nPort, pOut_L );
				//infoLog( "connect output port (L): " + string( m_d->PortNames[ nPort ] ) );
			}
			else if (nAOConn == 1) {
				m_d->connect_port( m_handle, nPort, pOut_R );
				//infoLog( "connect output port (R): " + string( m_d->PortNames[ nPort ] ) );
			}
			else {
				errorLog( "[connectAudioPorts] too many output ports.." );
			}
			nAOConn++;
		}
		else {
			errorLog( "[connectAudioPorts] unknown port" );
		}
	}
}



void LadspaFX::processFX( unsigned nFrames )
{
//	infoLog( "[LadspaFX::applyFX()]" );
	m_d->run( m_handle, nFrames );
}

void LadspaFX::activate()
{
	if ( m_d->activate ) {
		infoLog( "[activate] " + getPluginName() );
		m_d->activate( m_handle );
	}
}


void LadspaFX::deactivate()
{
	if ( m_d->deactivate ) {
		infoLog( "[deactivate] " + getPluginName());
		m_d->deactivate( m_handle );
	}
}


void LadspaFX::setVolume( float fValue )
{
	if (fValue > 2.0) {
		fValue = 2.0;
	}
	else if (fValue < 0.0 ){
		fValue = 0.0;
	}
	m_fVolume = fValue;
}

///
/// carica solo i plugin validi
///
vector<LadspaFXInfo*> LadspaFX::getPluginList()
{
	vector<LadspaFXInfo*> pluginList;
	vector<string> ladspaPathVect = Preferences::getInstance()->getLadspaPath();
	for (vector<string>::iterator i = ladspaPathVect.begin(); i != ladspaPathVect.end(); i++) {
		string sPluginDir = *i;

		QDir dir( QString( sPluginDir.c_str() ) );
		if (!dir.exists()) {
			//Logger::getInstance()->log( "[LadspaFX::getPluginList] directory " + sPluginDir + " not found" );
			return pluginList;
		}

		Logger::getInstance()->log( "[LadspaFX::getPluginList] reading directory: " + sPluginDir );
		const QFileInfoList *pList = dir.entryInfoList();
		QFileInfoListIterator it( *pList );
		QFileInfo *pFileInfo;

		while ( (pFileInfo = it.current()) != 0 ) {
			string sPluginName = pFileInfo->fileName().latin1();
			if ( (sPluginName == ".") || (sPluginName == ".." ) ) {
				++it;
				continue;
			}

			// if the file ends with .so is a plugin, else...
			int pos = sPluginName.rfind( ".so" );
			if ( pos == (int)std::string::npos ) {
				++it;
				continue;
			}

			string sAbsPath = string( sPluginDir ) + string("/") + sPluginName;

			QLibrary lib( QString( sAbsPath.c_str() ) );
			LADSPA_Descriptor_Function desc_func = (LADSPA_Descriptor_Function)lib.resolve( "ladspa_descriptor" );
			if ( desc_func == NULL ) {
				Logger::getInstance()->log( "[LadspaFX::getPluginList] Error loading the library. (" + sAbsPath + ")" );
			}

			const LADSPA_Descriptor * d;
			if ( desc_func ) {
				for ( unsigned i = 0; (d = desc_func (i)) != NULL; i++) {
					LadspaFXInfo* pFX = new LadspaFXInfo();
					pFX->sFilename = sAbsPath;
					pFX->sLabel = d->Label;
					pFX->sName = d->Name;
					pFX->sID = toString( d->UniqueID );
					pFX->sMaker = d->Maker;
					pFX->sCopyright = d->Copyright;

					for (unsigned j = 0; j < d->PortCount; j++) {
						LADSPA_PortDescriptor pd = d->PortDescriptors[j];
						if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
							pFX->nICPorts++;
						}
						else if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
							pFX->nIAPorts++;
						}
						else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
							pFX->nOCPorts++;
						}
						else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
							pFX->nOAPorts++;
						}
						else {
//							string sPortName = d->PortNames[ j ];
							string sPortName = "";
							Logger::getInstance()->log( "[LadspaFX::getPluginList] " + pFX->sLabel + "::" + sPortName + "  UNKNOWN port type" );
						}
					}
					if ( ( pFX->nIAPorts == 2 ) && ( pFX->nOAPorts == 2 ) ) {	// Stereo plugin
						pluginList.push_back( pFX );
					}
					else if ( ( pFX->nIAPorts == 1 ) && ( pFX->nOAPorts == 1 ) ) {	// Mono plugin
						pluginList.push_back( pFX );
					}
					else {	// not supported plugin
						delete pFX;
					}
				}
			}
//			if (module) {
//				dlclose( module );
//			}
			++it;
		}
	}
	return pluginList;
}



LadspaFXGroup* LadspaFX::getLadspaFXGroup()
{
	Logger::getInstance()->log( "[LadspaFX::getLadspaFXGroup]" );
	LadspaFXGroup *pRootGroup = new LadspaFXGroup( "Root" );

	LadspaFXGroup *pUncategorizedGroup = new LadspaFXGroup( "Uncategorized" );
	pRootGroup->addChildGroup( pUncategorizedGroup );

	map<LadspaFXInfo*, string> fxGroupMap;

	vector<LadspaFXInfo*> list = LadspaFX::getPluginList();

	// build alphabetical list
	for (unsigned i = 0; i < list.size(); i++) {
		LadspaFXInfo *pInfo = list[ i ];
		char ch = pInfo->sName[0];

		fxGroupMap[ pInfo ] = ch;
	}

	for (map<LadspaFXInfo*, string>::iterator it = fxGroupMap.begin(); it != fxGroupMap.end(); it++) {
		string sGroup = it->second;
		LadspaFXInfo *pInfo = it->first;

		LadspaFXGroup *pGroup = NULL;
		for (unsigned i = 0; i < pUncategorizedGroup->getChildGroups().size(); i++) {
			LadspaFXGroup *pChild = (pUncategorizedGroup->getChildGroups())[i];
			if (pChild->getName() == sGroup) {
				pGroup = pChild;
				break;
			}
		}
		if (!pGroup) {
			pGroup = new LadspaFXGroup( sGroup );
			pUncategorizedGroup->addChildGroup( pGroup );
		}
		pGroup->addLadspaInfo( pInfo );
	}


	#ifdef HAVE_LRDF_H
	LadspaFXGroup *pLRDFGroup = new LadspaFXGroup( "Categorized(LRDF)" );
	pRootGroup->addChildGroup( pLRDFGroup );
	getRDF(pLRDFGroup, list);
	#endif

	return pRootGroup;
}





#ifdef HAVE_LRDF_H


void LadspaFX::getRDF(LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList)
{
	lrdf_init();

	string sDir = "/usr/share/ladspa/rdf";

	QDir dir( QString( sDir.c_str() ) );
	if (!dir.exists()) {
		Logger::getInstance()->log( "[LadspaFX::getRDF] directory " + sDir + " not found" );
		return;
	}

	const QFileInfoList *pList = dir.entryInfoList();
	QFileInfoListIterator it( *pList );
	QFileInfo *pFileInfo;
	while ( (pFileInfo = it.current()) != 0 ) {
		string sFilename = pFileInfo->fileName().latin1();
		int pos = sFilename.find(".rdf");
		if (pos == -1) {
			++it;
			continue;
		}

		string sRDFFile = string("file://") + sDir + string("/") + sFilename;

		int err = lrdf_read_file( sRDFFile.c_str() );
		if (err) {
			Logger::getInstance()->log( "[LadspaFX::getRDF] error parsing rdf file " + sFilename );
		}

		string sBase = "http://ladspa.org/ontology#Plugin";
		RDFDescend( sBase, pGroup, pluginList );
		++it;
	}
}



// funzione ricorsiva
void LadspaFX::RDFDescend( string sBase, LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList )
{
//	cout << "LadspaFX::RDFDescend " << sBase << endl;

	lrdf_uris* uris = lrdf_get_subclasses( sBase.c_str() );
	if (uris) {
		for (int i = 0; i < uris->count; i++) {
			string sGroup = lrdf_get_label( uris->items[ i ] );

			LadspaFXGroup *pNewGroup = NULL;
			// verifico se esiste gia una categoria con lo stesso nome
			vector<LadspaFXGroup*> childGroups = pGroup-> getChildGroups();
			for (unsigned nGroup = 0; nGroup < childGroups.size(); nGroup++) {
				LadspaFXGroup *pOldGroup = childGroups[nGroup];
				if (pOldGroup->getName() == sGroup) {
					pNewGroup = pOldGroup;
					break;
				}
			}
			if (pNewGroup == NULL) {	// il gruppo non esiste, lo creo
				pNewGroup = new LadspaFXGroup( sGroup );
				pGroup->addChildGroup( pNewGroup );
			}
			RDFDescend( uris->items[i], pNewGroup, pluginList );
		}
		lrdf_free_uris (uris);
	}

	uris = lrdf_get_instances( sBase.c_str() );
	if (uris) {
		for (int i = 0; i < uris->count; i++) {
			int uid = lrdf_get_uid (uris->items[i]);

			// verifico che il plugin non sia gia nella lista
			bool bExists = false;
			vector<LadspaFXInfo*> fxVect = pGroup->getLadspaInfo();
			for (unsigned nFX = 0; nFX < fxVect.size(); nFX++) {
				LadspaFXInfo *pFX = fxVect[nFX];
				if (pFX->sID == toString(uid)) {
					bExists = true;
					continue;
				}
			}

			if ( bExists == false ) {
				// find the ladspaFXInfo
				for (unsigned i = 0; i < pluginList.size(); i++) {
					LadspaFXInfo *pInfo = pluginList[i];
					if (pInfo->sID == toString(uid) ) {
						pGroup->addLadspaInfo( new LadspaFXInfo(*pInfo) );	// copy the LadspaFXInfo
					}
				}
			}
		}
		lrdf_free_uris (uris);
	}
}


#endif // HAVE_LRDF_H

#endif // LADSPA_SUPPORT
