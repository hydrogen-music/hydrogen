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
 * along with this program. If not, see 
https://www.gnu.org/licenses
 *
 */

#include "Footer.h"

#include "StatusMessageDisplay.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"

#include <core/Hydrogen.h>

using namespace H2Core;

Footer::Footer( QWidget* pParent) : QWidget( pParent )
								  , m_nXRuns( 0 )
								  , m_bCpuLoadWarning( false )
{
	const auto pPref = Preferences::get_instance();
	const auto pSong = Hydrogen::get_instance()->getSong();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setFixedHeight( Footer::nHeight );
	setFocusPolicy( Qt::ClickFocus );
	setObjectName( "Footer" );

	auto pOverallLayout = new QHBoxLayout( this );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	pOverallLayout->setSpacing( 0 );
	setLayout( pOverallLayout );

	auto pMainFooter = new QWidget( this );
	pMainFooter->setObjectName( "MainFooter" );
	pOverallLayout->addWidget( pMainFooter );

	auto pMainFooterLayout = new QHBoxLayout( pMainFooter );
	pMainFooterLayout->setContentsMargins( 2, 2, 2, 2 );
	pMainFooterLayout->setSpacing( 2 );
	pMainFooterLayout->setAlignment( Qt::AlignLeft );
	pMainFooter->setLayout( pMainFooterLayout );

	////////////////////////////////////////////////////////////////////////////
	m_pStatusMessageDisplay = new StatusMessageDisplay( pMainFooter, QSize() );
	m_pStatusMessageDisplay->setFixedHeight( Footer::nHeight - 2 );
	m_pStatusMessageDisplay->setSizePolicy(
		QSizePolicy::Expanding, QSizePolicy::Fixed );
	pMainFooterLayout->addWidget( m_pStatusMessageDisplay );

	////////////////////////////////////////////////////////////////////////////
	m_pCpuGroup = new QWidget( pMainFooter );
	m_pCpuGroup->setFixedWidth( 102 );
	m_pCpuGroup->setObjectName( "GroupBox" );
	pMainFooterLayout->addWidget( m_pCpuGroup );
	auto pCpuGroupLayout = new QVBoxLayout( m_pCpuGroup );
	pCpuGroupLayout->setContentsMargins( 2, 2, 2, 1 );
	m_pCpuLabel = new QLabel( m_pCpuGroup );
	m_pCpuLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	m_pCpuLabel->setObjectName( "FooterCpusLabel" );
	m_pCpuLabel->setContentsMargins( 5, 0, 5, 0 );
	pCpuGroupLayout->addWidget( m_pCpuLabel );

	////////////////////////////////////////////////////////////////////////////
	m_pXRunGroup = new QWidget( pMainFooter );
	m_pXRunGroup->setObjectName( "GroupBox" );
	pMainFooterLayout->addWidget( m_pXRunGroup );
	auto pXRunGroupLayout = new QHBoxLayout( m_pXRunGroup );
	pXRunGroupLayout->setContentsMargins( 2, 2, 2, 1 );

	m_pXRunLabel = new QLabel( "XRuns: 9933", m_pXRunGroup );
	m_pXRunLabel->setObjectName( "FooterXRunsLabel" );
	m_pXRunLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	m_pXRunLabel->setContentsMargins( 5, 0, 5, 0 );
	pXRunGroupLayout->addWidget( m_pXRunLabel );

	////////////////////////////////////////////////////////////////////////////

	auto pCpuTimer = new QTimer( this );
	connect( pCpuTimer, SIGNAL( timeout() ), this, SLOT( updateCpuLoad() ) );
	pCpuTimer->start( Footer::cpuTimeout );

	////////////////////////////////////////////////////////////////////////////

	updateStyleSheet();
	updateCpuLoad();

	HydrogenApp::get_instance()->addEventListener( this );
}

Footer::~Footer() {
}

void Footer::driverChangedEvent() {
	m_nXRuns = 0;
}

void Footer::XRunEvent() {
	++m_nXRuns;
}

void Footer::showStatusBarMessage( const QString& sMessage,
										  const QString& sCaller ) {
	if ( H2Core::Hydrogen::get_instance()->getGUIState() ==
		 H2Core::Hydrogen::GUIState::ready ) {
		m_pStatusMessageDisplay->showMessage( sMessage, sCaller );
	}
}

void Footer::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
	}
}

void Footer::updateCpuLoad() {
	auto pAudioEngine = H2Core::Hydrogen::get_instance()->getAudioEngine();
	int nPercentage = 0;
	if ( pAudioEngine->getMaxProcessTime() != 0.0 ) {
		nPercentage = std::clamp(
			static_cast<int>(
				std::round( pAudioEngine->getProcessTime() /
							pAudioEngine->getMaxProcessTime() * 100 ) ), 0, 100 );
	}

	if ( nPercentage >= Footer::nCpuLoadWarningThreshold &&
		 ! m_bCpuLoadWarning ) {
		m_bCpuLoadWarning = true;
		updateStyleSheet();
	}
	else if ( nPercentage < Footer::nCpuLoadWarningThreshold &&
		 m_bCpuLoadWarning ) {
		m_bCpuLoadWarning = false;
		updateStyleSheet();
	}

	m_pCpuLabel->setText( QString( "CPU: %1%" ).arg( nPercentage ) );
}

void Footer::updateStyleSheet() {

	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QColor colorText = colorTheme.m_windowTextColor;
	const QColor colorFooter =
		colorTheme.m_windowColor.lighter( 134 );
	const QColor colorFooterLighter = colorFooter.lighter( 130 );
	const QColor colorRed = colorTheme.m_buttonRedColor;

	setStyleSheet( QString( "\
QWidget#MainFooter {\
     background-color: %1; \
     color: %2; \
     border: 1px solid #000;\
}")
				   .arg( colorFooter.name() ).arg( colorText.name() ) );

	const QString sGroupStyleSheet = QString( "\
QWidget#GroupBox {\
    background-color: %1;\
    color: %2;\
    border: 1px solid #000;\
    border-radius: 2px;\
}" )
		.arg( colorFooterLighter.name() ).arg( colorText.name() );

	m_pCpuGroup->setStyleSheet( sGroupStyleSheet );
	m_pXRunGroup->setStyleSheet( sGroupStyleSheet );

	if ( m_bCpuLoadWarning ) {
		m_pCpuLabel->setStyleSheet(
			QString( "color: %1;" ).arg( colorRed.name() ) );
	}
	else {
		m_pCpuLabel->setStyleSheet(
			QString( "color: %1;" ).arg( colorText.name() ) );
	}
}
