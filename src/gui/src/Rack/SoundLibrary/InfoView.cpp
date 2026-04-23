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

#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/DrumkitInfo.h>
#include <core/SoundLibrary/PatternInfo.h>
#include <core/SoundLibrary/SongInfo.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

using namespace H2Core;

InfoView::InfoView( QWidget* pParent ) : QWidget( pParent )
{
	setMinimumWidth( Rack::nWidth );
	setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred ) );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Main layout
	QVBoxLayout* pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing( 0 );
	pMainLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pMainLayout );

	m_pDetailName = new QLabel( this );
	m_pDetailName->setWordWrap( true );
	m_pDetailAuthor = new QLabel( this );
	m_pDetailAuthor->setWordWrap( true );
	m_pDetailInfo = new QLabel( this );
	m_pDetailInfo->setWordWrap( true );
	m_pDetailLicense = new QLabel( this );
	m_pDetailLicense->setWordWrap( true );
	m_pDetailPath = new QLabel( this );
	m_pDetailPath->setWordWrap( true );

	QFormLayout* pFormLayout = new QFormLayout();
	pFormLayout->addRow( pCommonStrings->getNameDialog(), m_pDetailName );
	pFormLayout->addRow( pCommonStrings->getAuthorDialog(), m_pDetailAuthor );
	pFormLayout->addRow( pCommonStrings->getNotesDialog(), m_pDetailInfo );
	pFormLayout->addRow( pCommonStrings->getLicenseDialog(), m_pDetailLicense );
	pFormLayout->addRow( "Path", m_pDetailPath );

	QWidget* pDetailContainer = new QWidget( this );
	pDetailContainer->setLayout( pFormLayout );
	pMainLayout->addWidget( pDetailContainer );

	updateVisibility();
}

InfoView::~InfoView()
{
}

void InfoView::updateContent( std::shared_ptr<H2Core::SoundLibraryInfo> pInfo )
{
	if ( pInfo == nullptr ) {
		m_pDetailName->clear();
		m_pDetailAuthor->clear();
		m_pDetailInfo->clear();
		m_pDetailLicense->clear();
		m_pDetailPath->clear();
	}
	else {
		m_pDetailName->setText( pInfo->getName() );
		m_pDetailAuthor->setText( pInfo->getAuthor() );
		m_pDetailInfo->setText( pInfo->getInfo() );
		m_pDetailLicense->setText( pInfo->getLicense().toQString( "", true ) );
		m_pDetailPath->setText( pInfo->getPath() );
	}
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
