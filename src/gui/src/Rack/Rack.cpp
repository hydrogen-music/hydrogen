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

#include "Rack.h"

#include "HydrogenApp.h"

#include "InstrumentEditor/ComponentsEditor.h"
#include "Rack/InstrumentEditor.h"
#include "Skin.h"
#include "SoundLibrary/SoundLibraryPanel.h"

#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include <QGridLayout>

using namespace H2Core;

Rack::Rack( QWidget* pParent )
	: QTabWidget( pParent ), Object()
{
	setFixedWidth( Rack::nWidth );
	setFocusPolicy( Qt::NoFocus );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );

	m_pInstrumentEditor = new InstrumentEditor( this );
	m_pComponentsEditor = new ComponentsEditor( this );
	m_pSoundLibraryPanel = new SoundLibraryPanel( this, false );

	connect(
		HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this,
		&Rack::onPreferencesChanged
	);

	updateIcons();
	updateStyleSheet();
}

Rack::~Rack()
{
}

void Rack::onPreferencesChanged(
	const H2Core::Preferences::Changes& changes
)
{
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
	}
	if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateIcons();
	}
}

void Rack::showInstrument()
{
	setTabEnabled( 0, true );
}

void Rack::showComponents()
{
	setTabEnabled( 1, true );
}

void Rack::showSoundLibrary()
{
	setTabEnabled( 2, true );
}

void Rack::updateStyleSheet()
{
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();
	const QColor colorTabBar = pColorTheme->m_baseColor;
	const QColor colorTabBarText = pColorTheme->m_widgetTextColor;

	setStyleSheet( QString( "\
QTabBar { \
     background-color: %1;\
     color: %2; \
}" )
					   .arg( colorTabBar.name() )
					   .arg( colorTabBarText.name() ) );
}

void Rack::updateIcons()
{
	for ( int ii = count(); ii >= 0; --ii ) {
		removeTab( ii );
	}

	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	}
	else {
		sIconPath.append( "/icons/black/" );
	}

	/*: Descriptive text in the tab to view the general instrument parameters at
	 * the bottom-right part of Hydrogen. Designed to hold 5 characters. Be sure
	 * to check the corresponding tab bar! */
	addTab(
		m_pInstrumentEditor, QIcon( sIconPath + "drum.svg" ), tr( "Param." )
	);
	/*: Descriptive text in the tab to view the instrument components at
	 * the bottom-right part of Hydrogen. Designed to hold 5 characters. Be sure
	 * to check the corresponding tab bar! */
	addTab(
		m_pComponentsEditor, QIcon( sIconPath + "drum.svg" ), tr( "Comp." )
	);
	/*: Descriptive text in the tab to view the sound library at the
	 * bottom-right part of Hydrogen. Designed to hold 5 characters. Be sure to
	 * check the corresponding tab bar! */
	addTab(
		m_pSoundLibraryPanel, QIcon( sIconPath + "folder.svg" ), tr( "Library" )
	);
}
