/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "InfoView.h"

#include <QTextDocumentFragment>

#include "../../CommonStrings.h"
#include "../../Compatibility/MouseEvent.h"
#include "../../HydrogenApp.h"
#include "../Rack.h"
#include "../../Skin.h"

#include <core/License.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/DrumkitInfo.h>
#include <core/SoundLibrary/PatternInfo.h>
#include <core/SoundLibrary/SongInfo.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

using namespace H2Core;

InfoView::InfoView( QWidget* pParent ) : QWidget( pParent )
{
	setMinimumWidth( Rack::nWidth );
	setMinimumHeight( 30 );
	setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred ) );

	const auto pPref = Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	auto pContainer = new QWidget( this );
	pContainer->setObjectName( "InfoViewContainer" );
	auto pOverallLayout = new QVBoxLayout();
	pOverallLayout->setSpacing( 0 );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	pOverallLayout->addWidget( pContainer );
	setLayout( pOverallLayout );

	auto pMainLayout = new QGridLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setContentsMargins( 3, 3, 3, 0 );
	pMainLayout->setColumnStretch( 0, 0 );
	pMainLayout->setColumnStretch( 1, 0 );
	pMainLayout->setColumnStretch( 2, 1 );
	pContainer->setLayout( pMainLayout );

	m_pMenu = new QMenu();

	auto addRow = [=]( const QString& sText, QLabel** pLabel, QLabel** pText ) {
		const int nRow = pMainLayout->rowCount();

		auto pDescription = new QLabel( this );
		pDescription->setText( sText );
		pMainLayout->addWidget( pDescription, nRow, 0 );
		*pLabel = pDescription;

		auto pContent = new QLabel( this );
		pMainLayout->addWidget( pContent, nRow, 2 );
		*pText = pContent;

		// Allow the user the show and hide the row via the popup menu.
		auto pAction = new QAction( sText, this );
		pAction->setCheckable( true );
		m_pMenu->addAction( pAction );

	return pAction;
	};

	auto pNameAction =
		addRow( pCommonStrings->getNameDialog(), &m_pNameLabel, &m_pNameText );
	pNameAction->setChecked( pPref->getSoundLibraryShowName() );
	connect( pNameAction, &QAction::toggled, this, [&]( bool bChecked ) {
		Preferences::get_instance()->setSoundLibraryShowName( bChecked );
		updateVisibility();
	} );
	auto pAuthorAction = addRow(
		pCommonStrings->getAuthorDialog(), &m_pAuthorLabel, &m_pAuthorText
	);
	pAuthorAction->setChecked( pPref->getSoundLibraryShowAuthor() );
	connect( pAuthorAction, &QAction::toggled, this, [&]( bool bChecked ) {
		Preferences::get_instance()->setSoundLibraryShowAuthor( bChecked );
		updateVisibility();
	} );
	auto pInfoAction =
		addRow( pCommonStrings->getNotesDialog(), &m_pInfoLabel, &m_pInfoText );
	pInfoAction->setChecked( pPref->getSoundLibraryShowInfo() );
	connect( pInfoAction, &QAction::toggled, this, [&]( bool bChecked ) {
		Preferences::get_instance()->setSoundLibraryShowInfo( bChecked );
		updateVisibility();
	} );
	auto pLicenseAction = addRow(
		pCommonStrings->getLicenseDialog(), &m_pLicenseLabel, &m_pLicenseText
	);
	pLicenseAction->setChecked( pPref->getSoundLibraryShowLicense() );
	connect( pLicenseAction, &QAction::toggled, this, [&]( bool bChecked ) {
		Preferences::get_instance()->setSoundLibraryShowLicense( bChecked );
		updateVisibility();
	} );
	auto pPathAction = addRow( "Path", &m_pPathLabel, &m_pPathText );
	pPathAction->setChecked( pPref->getSoundLibraryShowPath() );
	connect( pPathAction, &QAction::toggled, this, [&]( bool bChecked ) {
		Preferences::get_instance()->setSoundLibraryShowPath( bChecked );
		updateVisibility();
	} );
	auto pTagsAction =
		addRow( pCommonStrings->getTagsLabel(), &m_pTagsLabel, &m_pTagsText );
	pTagsAction->setChecked( pPref->getSoundLibraryShowTags() );
	connect( pTagsAction, &QAction::toggled, this, [&]( bool bChecked ) {
		Preferences::get_instance()->setSoundLibraryShowTags( bChecked );
		updateVisibility();
	} );
	auto pVersionAction = addRow(
		pCommonStrings->getVersionDialog(), &m_pVersionLabel, &m_pVersionText
	);
	pVersionAction->setChecked( pPref->getSoundLibraryShowVersion() );
	connect( pVersionAction, &QAction::toggled, this, [&]( bool bChecked ) {
		Preferences::get_instance()->setSoundLibraryShowVersion( bChecked );
		updateVisibility();
	} );

	auto pSeparator = new QFrame( this );
	pSeparator->setFixedWidth( 1 );
	pSeparator->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	pMainLayout->addWidget( pSeparator, 0, 1, pMainLayout->rowCount(), 1 );

	updateStyleSheet();
	updateVisibility();
}

InfoView::~InfoView()
{
}

void InfoView::updateContent( std::shared_ptr<H2Core::SoundLibraryInfo> pInfo )
{
	m_pInfo = pInfo;

	if ( pInfo == nullptr ) {
		m_pNameText->clear();
		m_pNameText->setToolTip( "" );
		m_pAuthorText->clear();
		m_pAuthorText->setToolTip( "" );
		m_pInfoText->clear();
		m_pInfoText->setToolTip( "" );
		m_pLicenseText->clear();
		m_pLicenseText->setToolTip( "" );
		m_pPathText->clear();
		m_pPathText->setToolTip( "" );
		m_pTagsText->clear();
		m_pTagsText->setToolTip( "" );
		m_pVersionText->clear();
		m_pVersionText->setToolTip( "" );
	}
	else {
		auto setText = [&]( QLabel* pLabel, const QString& sText ) {
			// Some drumkits feature a HTML-based description.
			const QString sTextCleaned =
				QTextDocumentFragment::fromHtml( sText ).toPlainText();
			pLabel->setToolTip( sTextCleaned );

			pLabel->setText( Skin::trimTextToFitWidth(
				sTextCleaned.simplified(), pLabel->font(), pLabel->width(),
				QMargins( 10, 0, 0, 0 )
			) );
		};
		setText( m_pNameText, pInfo->getName() );
		setText( m_pAuthorText, pInfo->getAuthor() );
		setText( m_pInfoText, pInfo->getInfo() );
		setText(
			m_pLicenseText,
			License::LicenseTypeToQString( pInfo->getLicense().getType() )
		);
		m_pPathText->setToolTip( pInfo->getPath() );
		m_pPathText->setText( Skin::trimPathToFitWidth(
			pInfo->getPath().simplified(), m_pPathText->font(),
			m_pPathText->width(), QMargins( 10, 0, 0, 0 )
		) );
		setText( m_pTagsText, pInfo->getTags().join( ", " ) );
		setText( m_pVersionText, QString::number( pInfo->getVersion() ) );
	}
}

void InfoView::updateStyleSheet()
{
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	const auto borderColor = pColorTheme->m_windowColor.darker( 140 );
	const auto separatorColor = pColorTheme->m_windowColor;

	const auto backgroundColor = pColorTheme->m_baseColor;
	const QColor textColor = Skin::moreBlackThanWhite( backgroundColor )
							   ? Qt::white
							   : Qt::black;
	setStyleSheet( QString( "        \
QWidget#InfoViewContainer {			 \
    background-color: %1;			 \
    border-left: 1px solid %3;		 \
    border-right: 1px solid %3;		 \
    border-bottom: 1px solid %3;	 \
}									 \
QFrame {		 \
    background-color: %4;			 \
}									 \
QLabel {						     \
    background-color: %1;			 \
    color: %2;						 \
    border-bottom: 1px solid %4;	 \
    padding: 5px;					 \
}									 \
" )
					   .arg( backgroundColor.name() )
					   .arg( textColor.name() )
					   .arg( borderColor.name() )
					   .arg( separatorColor.name() ) );
}

void InfoView::updateVisibility()
{
	const auto pPref = Preferences::get_instance();

	m_pNameLabel->setVisible( pPref->getSoundLibraryShowName() );
	m_pNameText->setVisible( pPref->getSoundLibraryShowName() );
	m_pAuthorLabel->setVisible( pPref->getSoundLibraryShowAuthor() );
	m_pAuthorText->setVisible( pPref->getSoundLibraryShowAuthor() );
	m_pInfoLabel->setVisible( pPref->getSoundLibraryShowInfo() );
	m_pInfoText->setVisible( pPref->getSoundLibraryShowInfo() );
	m_pLicenseLabel->setVisible( pPref->getSoundLibraryShowLicense() );
	m_pLicenseText->setVisible( pPref->getSoundLibraryShowLicense() );
	m_pPathLabel->setVisible( pPref->getSoundLibraryShowPath() );
	m_pPathText->setVisible( pPref->getSoundLibraryShowPath() );
	m_pTagsLabel->setVisible( pPref->getSoundLibraryShowTags() );
	m_pTagsText->setVisible( pPref->getSoundLibraryShowTags() );
	m_pVersionLabel->setVisible( pPref->getSoundLibraryShowVersion() );
	m_pVersionText->setVisible( pPref->getSoundLibraryShowVersion() );

	// In case a row was initial hidden, its string cutting based on the
	// widget's width did not work as expected since the visible widget will
	// have a different width. That's why we play it save and regenerate all
	// strings.
	updateContent( m_pInfo );
}

void InfoView::mousePressEvent( QMouseEvent* pEvent )
{
	if ( pEvent->buttons() & Qt::RightButton ) {
		auto pEv = static_cast<MouseEvent*>( pEvent );
		m_pMenu->popup( pEv->globalPosition().toPoint() );
	}
}
