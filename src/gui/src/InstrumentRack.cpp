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

#include "InstrumentRack.h"

#include "CommonStrings.h"
#include "HydrogenApp.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "Skin.h"
#include "SoundLibrary/SoundLibraryPanel.h"
#include "Widgets/Button.h"

#include <QGridLayout>

InstrumentRack::InstrumentRack( QWidget *pParent )
 : QWidget( pParent )
 , Object()
{
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setFixedWidth( InstrumentRack::nWidth );
	setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );

	auto pVBoxMainLayout = new QVBoxLayout();
	pVBoxMainLayout->setSpacing( 0 );
	pVBoxMainLayout->setContentsMargins( 0, 0, 0, 0 );

	QFont fontButtons( pFontTheme->m_sApplicationFontFamily,
					   getPointSize( pFontTheme->m_fontSize ) );

	// TAB buttons

	QWidget* pTabButtonsWidget = new QWidget( this );
	pTabButtonsWidget->setFixedSize( InstrumentRack::nWidth, 24 );
	QHBoxLayout *pTabHBox = new QHBoxLayout();
	pTabHBox->setSpacing( 0 );
	pTabHBox->setContentsMargins( 0, 0, 0, 0 );
	pTabButtonsWidget->setLayout( pTabHBox );

	const int nInstrumentBtnWidth = InstrumentRack::nWidth / 2;
	m_pShowInstrumentEditorBtn = new Button(
		pTabButtonsWidget, QSize( nInstrumentBtnWidth, 24 ), Button::Type::Toggle,
		"", pCommonStrings->getInstrumentButton(), QSize(),
		tr( "Show Instrument editor" ) );
	connect( m_pShowInstrumentEditorBtn, &QPushButton::clicked,
			 [=]() { showSoundLibrary( false ); });
	pTabHBox->addWidget( m_pShowInstrumentEditorBtn );

	m_pShowSoundLibraryBtn = new Button(
		pTabButtonsWidget,
		QSize( InstrumentRack::nWidth - nInstrumentBtnWidth, 24 ),
		Button::Type::Toggle, "", pCommonStrings->getSoundLibraryButton(),
		QSize(), tr( "Show sound library" ) );
	connect( m_pShowSoundLibraryBtn, &QPushButton::clicked,
			 [=]() { showSoundLibrary( true ); });
	pTabHBox->addWidget( m_pShowSoundLibraryBtn );

	pVBoxMainLayout->addWidget( pTabButtonsWidget );

	// Panels

	auto pPanelsWidget = new QWidget( this );
	pPanelsWidget->setMinimumWidth( InstrumentRack::nWidth );
	pPanelsWidget->setSizePolicy(
		QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding ) );
	m_pStackedPanelsLayout = new QStackedLayout();
	m_pStackedPanelsLayout->setContentsMargins( 0, 0, 0, 0 );
	pPanelsWidget->setLayout( m_pStackedPanelsLayout );
	pVBoxMainLayout->addWidget( pPanelsWidget );

	m_pInstrumentEditorPanel = new InstrumentEditorPanel( pPanelsWidget );
	m_pStackedPanelsLayout->addWidget( m_pInstrumentEditorPanel );

	m_pSoundLibraryPanel = new SoundLibraryPanel( pPanelsWidget, false );
	m_pStackedPanelsLayout->addWidget( m_pSoundLibraryPanel );

	setLayout( pVBoxMainLayout );

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &InstrumentRack::onPreferencesChanged );

	showSoundLibrary( false );
}



InstrumentRack::~InstrumentRack()
{
	INFOLOG( "DESTROY" );
}

void InstrumentRack::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		QFont fontButtons( pFontTheme->m_sApplicationFontFamily,
						   getPointSize( pFontTheme->m_fontSize ) );
		m_pShowInstrumentEditorBtn->setFont( fontButtons );
		m_pShowSoundLibraryBtn->setFont( fontButtons );
	}
}

void InstrumentRack::showSoundLibrary( bool bShow ) {
	if ( bShow ) {
		m_pShowSoundLibraryBtn->setChecked( true );
		m_pShowInstrumentEditorBtn->setChecked( false );
		m_pStackedPanelsLayout->setCurrentIndex( 1 );
	}
	else {
		m_pShowSoundLibraryBtn->setChecked( false );
		m_pShowInstrumentEditorBtn->setChecked( true );
		m_pStackedPanelsLayout->setCurrentIndex( 0 );
	}
}
