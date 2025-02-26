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
#include <core/FX/Effects.h>
#include <core/AudioEngine/AudioEngine.h>

#if defined(H2CORE_HAVE_LADSPA) || _DOXYGEN_

#include <core/Preferences/Preferences.h>
#include <core/FX/LadspaFX.h>
#include <core/Hydrogen.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>

#include <algorithm>
#include <QDir>
#include <QLibrary>
#include <cassert>

#ifdef H2CORE_HAVE_LRDF
#include <lrdf.h>
#endif

namespace H2Core
{

// static data
Effects* Effects::__instance = nullptr;

Effects::Effects()
		: m_pRootGroup( nullptr )
		, m_pRecentGroup( nullptr )
{
	__instance = this;

	m_FXs.resize( MAX_FX );

	getPluginList();
}


void Effects::create_instance()
{
	if ( __instance == nullptr ) {
		__instance = new Effects;
	}
}

Effects::~Effects() {
}


std::shared_ptr<LadspaFX> Effects::getLadspaFX( int nFX ) const {
	if ( nFX < 0 || nFX >= m_FXs.size() ) {
		return nullptr;
	}

	return m_FXs[ nFX ];
}

void Effects::setLadspaFX( std::shared_ptr<LadspaFX> pFX, int nFX ) {
	if ( nFX < 0 || nFX >= m_FXs.size() ) {
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();

	pHydrogen->getAudioEngine()->lock( RIGHT_HERE );


	if ( m_FXs[ nFX ] != nullptr ) {
		m_FXs[ nFX ]->deactivate();
	}

	m_FXs[ nFX ] = pFX;

	if ( pFX != nullptr ) {
		Preferences::get_instance()->setMostRecentFX( pFX->getPluginName() );
		updateRecentGroup();
	}


	pHydrogen->getAudioEngine()->unlock();

	if ( pHydrogen->getSong() != nullptr ) {
		pHydrogen->setIsModified( true );
	}
}

///
/// Loads only usable plugins
///
std::vector< std::shared_ptr<LadspaFXInfo> > Effects::getPluginList()
{
	if ( m_pluginList.size() != 0 ) {
		return m_pluginList;
	}

	foreach ( const QString& sPluginDir, Filesystem::ladspa_paths() ) {
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

			QString sAbsPath = QString( "%1/%2" ).arg( sPluginDir ).arg( sPluginName );

			QLibrary lib( sAbsPath );
			LADSPA_Descriptor_Function desc_func = ( LADSPA_Descriptor_Function )lib.resolve( "ladspa_descriptor" );
			if ( desc_func == nullptr ) {
				ERRORLOG( "Error loading the library. (" + sAbsPath + ")" );
				continue;
			}
			const LADSPA_Descriptor * d;
			if ( desc_func ) {
				for ( unsigned i = 0; ( d = desc_func ( i ) ) != nullptr; i++ ) {
					auto pFX = std::make_shared<LadspaFXInfo>(
						QString::fromLocal8Bit(d->Name) );
					pFX->m_sFilename = sAbsPath;
					pFX->m_sLabel = QString::fromLocal8Bit(d->Label);
					pFX->m_sID = QString::number(d->UniqueID);
					pFX->m_sMaker = QString::fromLocal8Bit(d->Maker);
					pFX->m_sCopyright = QString::fromLocal8Bit(d->Copyright);

					for ( unsigned j = 0; j < d->PortCount; j++ ) {
						LADSPA_PortDescriptor pd = d->PortDescriptors[j];
						if ( LADSPA_IS_PORT_INPUT( pd ) &&
							 LADSPA_IS_PORT_CONTROL( pd ) ) {
							pFX->m_nICPorts++;
						}
						else if ( LADSPA_IS_PORT_INPUT( pd ) &&
									LADSPA_IS_PORT_AUDIO( pd ) ) {
							pFX->m_nIAPorts++;
						}
						else if ( LADSPA_IS_PORT_OUTPUT( pd ) &&
									LADSPA_IS_PORT_CONTROL( pd ) ) {
							pFX->m_nOCPorts++;
						}
						else if ( LADSPA_IS_PORT_OUTPUT( pd ) &&
									LADSPA_IS_PORT_AUDIO( pd ) ) {
							pFX->m_nOAPorts++;
						}
						else {
							QString sPortName;
							ERRORLOG( QString( "%1::%2 unknown port type" )
									  .arg( pFX->m_sLabel ).arg( sPortName ) );
						}
					}
					if ( ( pFX->m_nIAPorts == 2 ) && ( pFX->m_nOAPorts == 2 ) ) {	// Stereo plugin
						m_pluginList.push_back( pFX );
					}
					else if ( ( pFX->m_nIAPorts == 1 ) && ( pFX->m_nOAPorts == 1 ) ) {	// Mono plugin
						m_pluginList.push_back( pFX );
					}
					// Otherwise the plugin is not supported.
				}
			} else {
				ERRORLOG( "Error loading: " + sPluginName  );
			}
		}
	}

	INFOLOG( QString( "Loaded %1 LADSPA plugins" ).arg( m_pluginList.size() ) );
	std::sort( m_pluginList.begin(), m_pluginList.end(),
			   LadspaFXInfo::alphabeticOrder );
	return m_pluginList;
}

std::shared_ptr<LadspaFXGroup> Effects::getLadspaFXGroup()
{
	INFOLOG( "[getLadspaFXGroup]" );

	if ( m_pRootGroup ) {
		return m_pRootGroup;
	}

	m_pRootGroup = std::make_shared<LadspaFXGroup>( "Root" );

	// Adding recent FX.
	m_pRecentGroup = std::make_shared<LadspaFXGroup>( "Recently Used" );
	m_pRootGroup->addChild( m_pRecentGroup );
	updateRecentGroup();

	auto pUncategorizedGroup = std::make_shared<LadspaFXGroup>( "Uncategorized" );
	m_pRootGroup->addChild( pUncategorizedGroup );

	char C = 0;
	std::shared_ptr<LadspaFXGroup> pGroup = nullptr;
	for ( std::vector< std::shared_ptr<LadspaFXInfo> >::iterator it =
			  m_pluginList.begin();
		  it < m_pluginList.end(); it++ ) {
		char ch = (*it)->m_sName.toLocal8Bit().at(0);
		if ( ch != C ) {
			C = ch;
			pGroup = std::make_shared<LadspaFXGroup>( QString( C ) );
			pUncategorizedGroup->addChild( pGroup );
		}

		if ( pGroup != nullptr ) {
			pGroup->addLadspaInfo( *it );
		}
	}


#ifdef H2CORE_HAVE_LRDF
	auto pLRDFGroup = std::make_shared<LadspaFXGroup>( "Categorized(LRDF)" );
	m_pRootGroup->addChild( pLRDFGroup );
	getRDF( pLRDFGroup, m_pluginList );
#endif

	return m_pRootGroup;
}

void Effects::updateRecentGroup()
{
	if ( m_pRecentGroup == nullptr ) {
		return;  // Too early :s
	}

	m_pRecentGroup->clear();


	QString sRecent; // The recent fx names sit in the preferences object
	foreach ( sRecent, Preferences::get_instance()->getRecentFX() ) {
		for ( std::vector< std::shared_ptr<LadspaFXInfo> >::iterator it =
				  m_pluginList.begin();
			  it < m_pluginList.end(); it++ ) {
			if ( sRecent == (*it)->m_sName ) {
				m_pRecentGroup->addLadspaInfo( *it );
				break;
			}
		}
	}
	Hydrogen::get_instance()->setIsModified( true );
}

#ifdef H2CORE_HAVE_LRDF


void Effects::getRDF( std::shared_ptr<LadspaFXGroup> pGroup,
					  std::vector< std::shared_ptr<LadspaFXInfo> > pluginList )
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
void Effects::RDFDescend( const QString& sBase,
						  std::shared_ptr<LadspaFXGroup> pGroup,
						  std::vector< std::shared_ptr<LadspaFXInfo> > pluginList )
{
	//cout << "LadspaFX::RDFDescend " << sBase.toLocal8Bit().constData() << endl;

	lrdf_uris* uris = lrdf_get_subclasses( sBase.toLocal8Bit() );
	if ( uris ) {
		for ( int i = 0; i < ( int )uris->count; i++ ) {
			QString sGroup = QString::fromLocal8Bit(lrdf_get_label( uris->items[ i ] ));

			std::shared_ptr<LadspaFXGroup> pNewGroup = nullptr;
			// verifico se esiste gia una categoria con lo stesso nome
			auto childGroups = pGroup->getChildList();
			for ( const auto& ppOldGroup : childGroups ) {
				if ( ppOldGroup->getName() == sGroup ) {
					pNewGroup = ppOldGroup;
					break;
				}
			}
			if ( pNewGroup == nullptr ) {	// il gruppo non esiste, lo creo
				pNewGroup = std::make_shared<LadspaFXGroup>( sGroup );
				pGroup->addChild( pNewGroup );
			}
			RDFDescend( QString::fromLocal8Bit(uris->items[i]), pNewGroup,
						pluginList );
		}
		lrdf_free_uris ( uris );
	}

	uris = lrdf_get_instances( sBase.toLocal8Bit() );
	if ( uris ) {
		for ( int i = 0; i < ( int )uris->count; i++ ) {
			int uid = lrdf_get_uid ( uris->items[i] );

			// verifico che il plugin non sia gia nella lista
			bool bExists = false;
			auto fxVect = pGroup->getLadspaInfo();
			for ( const auto& ppFX : fxVect ) {
				if ( ppFX != nullptr && ppFX->m_sID.toInt() == uid ) {
					bExists = true;
					continue;
				}
			}

			if ( bExists == false ) {
				// find the ladspaFXInfo
				for ( const auto& ppInfo : pluginList ) {
					if ( ppInfo->m_sID.toInt() == uid  ) {
						pGroup->addLadspaInfo( ppInfo );	// copy the LadspaFXInfo
					}
				}
			}
		}
		lrdf_free_uris( uris );
	}
	pGroup->sort();
}


#endif // H2CORE_HAVE_LRDF

};

#endif // H2CORE_HAVE_LADSPA
