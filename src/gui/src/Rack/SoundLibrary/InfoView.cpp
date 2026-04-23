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

#include "InfoView.h"

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

	auto addRow = [=]( const QString& sText, QLabel** pLabel ) {
		const int nRow = pMainLayout->rowCount();

		auto pDescription = new QLabel( this );
		pDescription->setText( sText );
		pMainLayout->addWidget( pDescription, nRow, 0 );
		*pLabel = pDescription;

		auto pText = new QLabel( this );
		pMainLayout->addWidget( pText, nRow, 2 );

		return pText;
	};

	m_pNameText = addRow( pCommonStrings->getNameDialog(), &m_pNameLabel );
	m_pAuthorText = addRow( pCommonStrings->getAuthorDialog(), &m_pAuthorLabel );
	m_pInfoText = addRow( pCommonStrings->getNotesDialog(), &m_pInfoLabel );
	m_pLicenseText =
		addRow( pCommonStrings->getLicenseDialog(), &m_pLicenseLabel );
	m_pPathText = addRow( "Path", &m_pPathLabel );
	m_pTagsText = addRow( pCommonStrings->getTagsLabel(), &m_pTagsLabel );

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
	}
	else {
		auto setText = [&]( QLabel* pLabel, const QString& sText ) {
			pLabel->setToolTip( sText );

			pLabel->setText( Skin::trimToFitWidth(
				sText.simplified(), pLabel->font(), pLabel->width(),
				QMargins( 5, 0, 0, 0 )
			) );
		};
		setText( m_pNameText, pInfo->getName() );
		setText( m_pAuthorText, pInfo->getAuthor() );
		setText( m_pInfoText, pInfo->getInfo() );
		setText(
			m_pLicenseText,
			License::LicenseTypeToQString( pInfo->getLicense().getType() )
		);
		setText( m_pPathText, pInfo->getPath() );
		setText( m_pTagsText, pInfo->getTags().join( ", " ) );
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
	DEBUGLOG( "not implemented" );
}

void InfoView::mousePressEvent( QMouseEvent* pEvent )
{
	if ( pEvent->buttons() & Qt::RightButton ) {
		auto pEv = static_cast<MouseEvent*>( pEvent );
		m_pMenu->popup( pEv->globalPosition().toPoint() );
	}
}
