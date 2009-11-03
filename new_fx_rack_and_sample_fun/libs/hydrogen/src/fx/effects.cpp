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
#include <hydrogen/fx/Effects.h>

#ifdef LADSPA_SUPPORT

#include <hydrogen/Preferences.h>
#include <hydrogen/fx/LadspaFX.h>
#include <hydrogen/audio_engine.h>

#include <algorithm>
#include <QDir>
#include <QLibrary>
#include <cassert>

#ifdef LRDF_SUPPORT
#include <lrdf.h>
#endif

using namespace std;

namespace H2Core
{

// static data
Effects* Effects::__instance = NULL;



Effects::Effects()
		: Object( "Effects" )
		, m_pRootGroup( NULL )
		, m_pRecentGroup( NULL )
{
	__instance = this;

	for ( int nFX = 0; nFX < MAX_FX; ++nFX ) {
		m_FXList[ nFX ] = NULL;
	}

	getPluginList();
}



void Effects::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new Effects;
	}
}




Effects::~Effects()
{
	//INFOLOG( "DESTROY" );
	if ( m_pRootGroup != NULL ) delete m_pRootGroup;
	
	//INFOLOG( "destroying " + to_string( m_pluginList.size() ) + " LADSPA plugins" );
	for ( unsigned i = 0; i < m_pluginList.size(); i++ ) {
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
	//INFOLOG( "[setLadspaFX] FX: " + pFX->getPluginLabel() + ", " + to_string( nFX ) );

	AudioEngine::get_instance()->lock( RIGHT_HERE );


	if ( m_FXList[ nFX ] ) {
		( m_FXList[ nFX ] )->deactivate();
		delete m_FXList[ nFX ];
	}

	m_FXList[ nFX ] = pFX;
	
	if ( pFX != NULL ) {
		Preferences::get_instance()->setMostRecentFX( pFX->getPluginName() );
		updateRecentGroup();
	}


	AudioEngine::get_instance()->unlock();
}



///
/// Loads only usable plugins
///
std::vector<LadspaFXInfo*> Effects::getPluginList()
{
	if ( m_pluginList.size() != 0 ) {
		return m_pluginList;
	}

	vector<QString> ladspaPathVect = Preferences::get_instance()->getLadspaPath();
	INFOLOG( QString( "PATHS: %1" ).arg( ladspaPathVect.size() ) );
	for ( vector<QString>::iterator i = ladspaPathVect.begin(); i != ladspaPathVect.end(); i++ ) {
		QString sPluginDir = *i;
		INFOLOG( "*** [getPluginList] reading directory: " + sPluginDir );

		QDir dir( sPluginDir );
		if ( !dir.exists() ) {
			INFOLOG( "Directory " + sPluginDir + " not found" );
			continue;
		}

		QFileInfoList list = dir.entryInfoList();
		for ( int i = 0; i < list.size(); ++i ) {
			QString sPluginName = list.at( i ).fileName();

			if ( ( sPluginName == "." ) || ( sPluginName == ".." ) ) {
				continue;
			}

			// if the file ends with .so or .dll is a plugin, else...
#ifdef WIN32
			int pos = sPluginName.indexOf( ".dll" );
#else
#ifdef Q_OS_MACX
			int pos = sPluginName.indexOf( ".dylib" );
#else
			int pos = sPluginName.indexOf( ".so" );
#endif
#endif
			if ( pos == -1 ) {
				continue;
			}
			//warningLog( "[getPluginList] Loading: " + sPluginName  );

			QString sAbsPath = QString( "%1/%2" ).arg( sPluginDir ).arg( sPluginName );

			QLibrary lib( sAbsPath );
			LADSPA_Descriptor_Function desc_func = ( LADSPA_Descriptor_Function )lib.resolve( "ladspa_descriptor" );
			if ( desc_func == NULL ) {
				ERRORLOG( "Error loading the library. (" + sAbsPath + ")" );
				continue;
			}
			const LADSPA_Descriptor * d;
			if ( desc_func ) {
				for ( unsigned i = 0; ( d = desc_func ( i ) ) != NULL; i++ ) {
					LadspaFXInfo* pFX = new LadspaFXInfo( QString::fromLocal8Bit(d->Name) );
					pFX->m_sFilename = sAbsPath;
					pFX->m_sLabel = QString::fromLocal8Bit(d->Label);
					pFX->m_sID = QString::number(d->UniqueID);
					pFX->m_sMaker = QString::fromLocal8Bit(d->Maker);
					pFX->m_sCopyright = QString::fromLocal8Bit(d->Copyright);

					//INFOLOG( "Loading: " + pFX->m_sLabel );

					for ( unsigned j = 0; j < d->PortCount; j++ ) {
						LADSPA_PortDescriptor pd = d->PortDescriptors[j];
						if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
							pFX->m_nICPorts++;
						} else if ( LADSPA_IS_PORT_INPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
							pFX->m_nIAPorts++;
						} else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_CONTROL( pd ) ) {
							pFX->m_nOCPorts++;
						} else if ( LADSPA_IS_PORT_OUTPUT( pd ) && LADSPA_IS_PORT_AUDIO( pd ) ) {
							pFX->m_nOAPorts++;
						} else {
//							string sPortName = d->PortNames[ j ];
							QString sPortName;
							ERRORLOG( QString( "%1::%2 unknown port type" ).arg( pFX->m_sLabel ).arg( sPortName ) );
						}
					}
					if ( ( pFX->m_nIAPorts == 2 ) && ( pFX->m_nOAPorts == 2 ) ) {	// Stereo plugin
						m_pluginList.push_back( pFX );
					} else if ( ( pFX->m_nIAPorts == 1 ) && ( pFX->m_nOAPorts == 1 ) ) {	// Mono plugin
						m_pluginList.push_back( pFX );
					} else {	// not supported plugin
						//WARNINGLOG( "Plugin not supported: " + sPluginName  );
						delete pFX;
					}
				}
			} else {
				ERRORLOG( "Error loading: " + sPluginName  );
			}
		}
	}

	INFOLOG( QString( "Loaded %1 LADSPA plugins" ).arg( m_pluginList.size() ) );
	std::sort( m_pluginList.begin(), m_pluginList.end(), LadspaFXInfo::alphabeticOrder );
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
	
	// Adding recent FX.
	m_pRecentGroup = new LadspaFXGroup( "Recently Used" );
	m_pRootGroup->addChild( m_pRecentGroup );
	updateRecentGroup();

	LadspaFXGroup *pUncategorizedGroup = new LadspaFXGroup( "Uncategorized" );
	m_pRootGroup->addChild( pUncategorizedGroup );

	char C = 0;
	LadspaFXGroup* pGroup;
	for ( std::vector<LadspaFXInfo*>::iterator i = m_pluginList.begin(); i < m_pluginList.end(); i++ ) {
                char ch = (*i)->m_sName.toLocal8Bit().at(0);
		if ( ch != C ) {
			C = ch;
			pGroup = new LadspaFXGroup( QString( C ) );
			pUncategorizedGroup->addChild( pGroup );
		}
		pGroup->addLadspaInfo( *i );
	}


#ifdef LRDF_SUPPORT
	LadspaFXGroup *pLRDFGroup = new LadspaFXGroup( "Categorized(LRDF)" );
	m_pRootGroup->addChild( pLRDFGroup );
	getRDF( pLRDFGroup, m_pluginList );
#endif

	return m_pRootGroup;
}

void Effects::updateRecentGroup()
{
	if ( m_pRecentGroup == NULL )
		return;  // Too early :s
	
	m_pRecentGroup->clear();
	

	QString sRecent; // The recent fx names sit in the preferences object
	foreach ( sRecent, Preferences::get_instance()->getRecentFX() ) {
		for ( std::vector<LadspaFXInfo*>::iterator i = m_pluginList.begin(); i < m_pluginList.end(); i++ ) {
			if ( sRecent == (*i)->m_sName ) {
				m_pRecentGroup->addLadspaInfo( *i );
				break;
			}
		}
	}
}

#ifdef LRDF_SUPPORT


void Effects::getRDF( LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList )
{
	lrdf_init();

	QString sDir = "/usr/share/ladspa/rdf";

	QDir dir( sDir );
	if ( !dir.exists() ) {
		WARNINGLOG( QString( "Directory %1 not found" ).arg( sDir ) );
		return;
	}

	QFileInfoList list = dir.entryInfoList();
	for ( int i = 0; i < list.size(); ++i ) {
		QString sFilename = list.at( i ).fileName();
		int pos = sFilename.indexOf( ".rdf" );
		if ( pos == -1 ) {
			continue;
		}

		QString sRDFFile = QString( "file://%1/%2" ).arg( sDir ).arg( sFilename );

		int err = lrdf_read_file( sRDFFile.toLocal8Bit() );
		if ( err ) {
			ERRORLOG( "Error parsing rdf file " + sFilename );
		}

		QString sBase = "http://ladspa.org/ontology#Plugin";
		RDFDescend( sBase, pGroup, pluginList );
	}
}



// funzione ricorsiva
void Effects::RDFDescend( const QString& sBase, LadspaFXGroup *pGroup, vector<LadspaFXInfo*> pluginList )
{
	//cout << "LadspaFX::RDFDescend " << sBase.toLocal8Bit().constData() << endl;

	lrdf_uris* uris = lrdf_get_subclasses( sBase.toLocal8Bit() );
	if ( uris ) {
		for ( int i = 0; i < ( int )uris->count; i++ ) {
			QString sGroup = QString::fromLocal8Bit(lrdf_get_label( uris->items[ i ] ));

			LadspaFXGroup *pNewGroup = NULL;
			// verifico se esiste gia una categoria con lo stesso nome
			vector<LadspaFXGroup*> childGroups = pGroup-> getChildList();
			for ( unsigned nGroup = 0; nGroup < childGroups.size(); nGroup++ ) {
				LadspaFXGroup *pOldGroup = childGroups[nGroup];
				if ( pOldGroup->getName() == sGroup ) {
					pNewGroup = pOldGroup;
					break;
				}
			}
			if ( pNewGroup == NULL ) {	// il gruppo non esiste, lo creo
				pNewGroup = new LadspaFXGroup( sGroup );
				pGroup->addChild( pNewGroup );
			}
			RDFDescend( QString::fromLocal8Bit(uris->items[i]), pNewGroup, pluginList );
		}
		lrdf_free_uris ( uris );
	}

	uris = lrdf_get_instances( sBase.toLocal8Bit() );
	if ( uris ) {
		for ( int i = 0; i < ( int )uris->count; i++ ) {
			int uid = lrdf_get_uid ( uris->items[i] );

			// verifico che il plugin non sia gia nella lista
			bool bExists = false;
			vector<LadspaFXInfo*> fxVect = pGroup->getLadspaInfo();
			for ( unsigned nFX = 0; nFX < fxVect.size(); nFX++ ) {
				LadspaFXInfo *pFX = fxVect[nFX];
				if ( pFX->m_sID.toInt() == uid ) {
					bExists = true;
					continue;
				}
			}

			if ( bExists == false ) {
				// find the ladspaFXInfo
				for ( unsigned i = 0; i < pluginList.size(); i++ ) {
					LadspaFXInfo *pInfo = pluginList[i];
					
					if ( pInfo->m_sID.toInt() == uid  ) {
						pGroup->addLadspaInfo( pInfo );	// copy the LadspaFXInfo
					} 
				}
			}
		}
		lrdf_free_uris ( uris );
	}
	pGroup->sort();
}


#endif // LRDF_SUPPORT

};

#endif // LADSPA SUPPORT
