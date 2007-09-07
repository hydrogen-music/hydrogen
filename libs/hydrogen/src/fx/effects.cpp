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
#include <hydrogen/fx/Effects.h>

#ifdef LADSPA_SUPPORT

#include <hydrogen/Preferences.h>
#include <hydrogen/fx/LadspaFX.h>
#include <hydrogen/audio_engine.h>

#include <QDir>
#include <QLibrary>
#include <cassert>

#ifdef LRDF_SUPPORT
#include <lrdf.h>
#endif

using namespace std;

namespace H2Core {

// static data
Effects* Effects::m_pInstance = NULL;



Effects::Effects()
 : Object( "Effects" )
 , m_pRootGroup( NULL)
{
	//INFOLOG( "INIT" );

	for ( int nFX = 0; nFX < MAX_FX; ++nFX ) {
		m_FXList[ nFX ] = NULL;
	}

	getPluginList();
}



Effects* Effects::getInstance()
{
	if ( m_pInstance == NULL ) {
		m_pInstance = new Effects();
	}
	return m_pInstance;
}




Effects::~Effects()
{
	//INFOLOG( "DESTROY" );
	delete getLadspaFXGroup();

	//INFOLOG( "destroying " + toString( m_pluginList.size() ) + " LADSPA plugins" );
	for( unsigned i = 0; i < m_pluginList.size(); i++) {
		delete m_pluginList[i];
	}
	m_pluginList.clear();

	for ( int nFX = 0; nFX < MAX_FX; ++nFX ) {
		delete m_FXList[ nFX ];
	}
}



LadspaFX* Effects::getLadspaFX( int nFX )
{
	assert( nFX < MAX_FX );
	return m_FXList[ nFX ];
}



void  Effects::setLadspaFX( LadspaFX* pFX, int nFX )
{
	assert( nFX < MAX_FX );
	//INFOLOG( "[setLadspaFX] FX: " + pFX->getPluginLabel() + ", " + toString( nFX ) );

	AudioEngine::get_instance()->lock("Effects::setLadspaFX");


	if ( m_FXList[ nFX ] ) {
		( m_FXList[ nFX ] )->deactivate();
		delete m_FXList[ nFX ];
	}

	m_FXList[ nFX ] = pFX;

	AudioEngine::get_instance()->unlock();
}



///
/// Loads only usable plugins
///
std::vector<LadspaFXInfo*> Effects::getPluginList()
{
	if ( m_pluginList.size() != 0 ) {
		//Logger::getInstance()->log( "skippinggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg" );
		return m_pluginList;
	}



	vector<string> ladspaPathVect = Preferences::getInstance()->getLadspaPath();
	INFOLOG( "PATHS: " + toString( ladspaPathVect.size() ) );
	for (vector<string>::iterator i = ladspaPathVect.begin(); i != ladspaPathVect.end(); i++) {
		string sPluginDir = *i;
		INFOLOG( "*** [getPluginList] reading directory: " + sPluginDir );

		QDir dir( QString( sPluginDir.c_str() ) );
		if (!dir.exists()) {
			INFOLOG( "Directory " + sPluginDir + " not found" );
			continue;
		}

		QFileInfoList list = dir.entryInfoList();
		for ( int i = 0; i < list.size(); ++i ) {
			string sPluginName = list.at( i ).fileName().toStdString();

			if ( (sPluginName == ".") || (sPluginName == ".." ) ) {
				continue;
			}

			// if the file ends with .so or .dll is a plugin, else...
#ifdef WIN32
			int pos = sPluginName.rfind( ".dll" );
#else
	#ifdef Q_OS_MACX
			int pos = sPluginName.rfind( ".dylib" );
	#else
			int pos = sPluginName.rfind( ".so" );
	#endif
#endif
			if ( pos == (int)std::string::npos ) {
				continue;
			}
			//warningLog( "[getPluginList] Loading: " + sPluginName  );

			string sAbsPath = string( sPluginDir ) + string( "/" ) + sPluginName;

			QLibrary lib( QString( sAbsPath.c_str() ) );
			LADSPA_Descriptor_Function desc_func = (LADSPA_Descriptor_Function)lib.resolve( "ladspa_descriptor" );
			if ( desc_func == NULL ) {
				ERRORLOG( "Error loading the library. (" + sAbsPath + ")" );
				continue;
			}
			const LADSPA_Descriptor * d;
			if ( desc_func ) {
				for ( unsigned i = 0; (d = desc_func (i)) != NULL; i++) {
					LadspaFXInfo* pFX = new LadspaFXInfo( d->Name );
					pFX->m_sFilename = sAbsPath;
					pFX->m_sLabel = d->Label;
					pFX->m_sID = toString( d->UniqueID );
					pFX->m_sMaker = d->Maker;
					pFX->m_sCopyright = d->Copyright;

					//INFOLOG( "Loading: " + pFX->m_sLabel );

					for (unsigned j = 0; j < d->PortCount; j++) {
						LADSPA_PortDescriptor pd = d->PortDescriptors[j];
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
//							string sPortName = d->PortNames[ j ];
							string sPortName = "";
							ERRORLOG( pFX->m_sLabel + "::" + sPortName + "  UNKNOWN port type" );
						}
					}
					if ( ( pFX->m_nIAPorts == 2 ) && ( pFX->m_nOAPorts == 2 ) ) {	// Stereo plugin
						m_pluginList.push_back( pFX );
					}
					else if ( ( pFX->m_nIAPorts == 1 ) && ( pFX->m_nOAPorts == 1 ) ) {	// Mono plugin
						m_pluginList.push_back( pFX );
					}
					else {	// not supported plugin
						//WARNINGLOG( "Plugin not supported: " + sPluginName  );
						delete pFX;
					}
				}
			}
			else {
				ERRORLOG( "Error loading: " + sPluginName  );
			}
		}
	}

	INFOLOG( "Loaded " + toString( m_pluginList.size() ) + "  LADSPA plugins" );

	return m_pluginList;
}




LadspaFXGroup* Effects::getLadspaFXGroup()
{
	INFOLOG( "[getLadspaFXGroup]" );

//	LadspaFX::getPluginList();	// load the list

	if ( m_pRootGroup  ) {
		return m_pRootGroup;
	}

	m_pRootGroup = new LadspaFXGroup( "Root" );

	LadspaFXGroup *pUncategorizedGroup = new LadspaFXGroup( "Uncategorized" );
	m_pRootGroup->addChild( pUncategorizedGroup );

	map<LadspaFXInfo*, string> fxGroupMap;

	// build alphabetical list
	for (unsigned i = 0; i < m_pluginList.size(); i++) {
		LadspaFXInfo *pInfo = m_pluginList[ i ];
		char ch = pInfo->m_sName[0];
		fxGroupMap[ pInfo ] = ch;
	}

	for (map<LadspaFXInfo*, string>::iterator it = fxGroupMap.begin(); it != fxGroupMap.end(); it++) {
		string sGroup = it->second;
		LadspaFXInfo *pInfo = it->first;

		LadspaFXGroup *pGroup = NULL;
		for (unsigned i = 0; i < pUncategorizedGroup->getChildList().size(); i++) {
			LadspaFXGroup *pChild = ( pUncategorizedGroup->getChildList() )[ i ];
			if (pChild->getName() == sGroup) {
				pGroup = pChild;
				break;
			}
		}
		if (!pGroup) {
			pGroup = new LadspaFXGroup( sGroup );
			pUncategorizedGroup->addChild( pGroup );
		}
		pGroup->addLadspaInfo( pInfo );
	}


	#ifdef LRDF_SUPPORT
	LadspaFXGroup *pLRDFGroup = new LadspaFXGroup( "Categorized(LRDF)" );
	m_pRootGroup->addChild( pLRDFGroup );
	getRDF(pLRDFGroup, m_pluginList);
	#endif

	return m_pRootGroup;
}


#ifdef LRDF_SUPPORT


void Effects::getRDF(LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList)
{
	lrdf_init();

	string sDir = "/usr/share/ladspa/rdf";

	QDir dir( QString( sDir.c_str() ) );
	if (!dir.exists()) {
		WARNINGLOG( "Directory " + sDir + " not found" );
		return;
	}

	QFileInfoList list = dir.entryInfoList();
	for ( int i = 0; i < list.size(); ++i ){
		string sFilename = list.at( i ).fileName().toStdString();
		int pos = sFilename.find(".rdf");
		if (pos == -1) {
			continue;
		}

		string sRDFFile = string("file://") + sDir + string("/") + sFilename;

		int err = lrdf_read_file( sRDFFile.c_str() );
		if (err) {
			ERRORLOG( "Error parsing rdf file " + sFilename );
		}

		string sBase = "http://ladspa.org/ontology#Plugin";
		RDFDescend( sBase, pGroup, pluginList );
	}
}



// funzione ricorsiva
void Effects::RDFDescend( const std::string& sBase, LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList )
{
//	cout << "LadspaFX::RDFDescend " << sBase << endl;

	lrdf_uris* uris = lrdf_get_subclasses( sBase.c_str() );
	if (uris) {
		for (int i = 0; i < (int)uris->count; i++) {
			string sGroup = lrdf_get_label( uris->items[ i ] );

			LadspaFXGroup *pNewGroup = NULL;
			// verifico se esiste gia una categoria con lo stesso nome
			vector<LadspaFXGroup*> childGroups = pGroup-> getChildList();
			for (unsigned nGroup = 0; nGroup < childGroups.size(); nGroup++) {
				LadspaFXGroup *pOldGroup = childGroups[nGroup];
				if (pOldGroup->getName() == sGroup) {
					pNewGroup = pOldGroup;
					break;
				}
			}
			if (pNewGroup == NULL) {	// il gruppo non esiste, lo creo
				pNewGroup = new LadspaFXGroup( sGroup );
				pGroup->addChild( pNewGroup );
			}
			RDFDescend( uris->items[i], pNewGroup, pluginList );
		}
		lrdf_free_uris (uris);
	}

	uris = lrdf_get_instances( sBase.c_str() );
	if (uris) {
		for (int i = 0; i < (int)uris->count; i++) {
			int uid = lrdf_get_uid (uris->items[i]);

			// verifico che il plugin non sia gia nella lista
			bool bExists = false;
			vector<LadspaFXInfo*> fxVect = pGroup->getLadspaInfo();
			for (unsigned nFX = 0; nFX < fxVect.size(); nFX++) {
				LadspaFXInfo *pFX = fxVect[nFX];
				if (pFX->m_sID == toString(uid)) {
					bExists = true;
					continue;
				}
			}

			if ( bExists == false ) {
				// find the ladspaFXInfo
				for (unsigned i = 0; i < pluginList.size(); i++) {
					LadspaFXInfo *pInfo = pluginList[i];
					if (pInfo->m_sID == toString(uid) ) {
						pGroup->addLadspaInfo( pInfo );	// copy the LadspaFXInfo
					}
				}
			}
		}
		lrdf_free_uris (uris);
	}
}


#endif // LRDF_SUPPORT

};

#endif // LADSPA SUPPORT
