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

#include "LadspaFXLine.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../LadspaFXProperties.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/WidgetWithInput.h"

#include <core/Basics/Event.h>
#include <core/FX/Effects.h>

using namespace H2Core;

LadspaFXLine::LadspaFXLine( QWidget* pParent, std::shared_ptr<LadspaFX> pFX )
	: PixmapWidget( pParent )
	, m_pFX( pFX )
{
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	resize( LadspaFXLine::nWidth, LadspaFXLine::nHeight );
	setMinimumSize( LadspaFXLine::nWidth, LadspaFXLine::nHeight );
	setMaximumSize( LadspaFXLine::nWidth, LadspaFXLine::nHeight );

	setPixmap( "/mixerPanel/fxline_background.png" );

	// active button
	m_pBypassBtn = new Button(
		this, QSize( 34, 14 ), Button::Type::Toggle, "",
		pCommonStrings->getBypassButton(), true, QSize(), tr( "FX bypass") );
	m_pBypassBtn->setObjectName( "MixerFXBypassButton" );
	m_pBypassBtn->move( 52, 25 );
#ifdef H2CORE_HAVE_LADSPA
	connect( m_pBypassBtn, &Button::clicked, [&]() {
		if ( m_pFX != nullptr ) {
			m_pFX->setEnabled( ! m_pBypassBtn->isChecked() );

			Hydrogen::get_instance()->setIsModified( true );
		}
	});
#endif

	// edit button
	m_pEditBtn = new Button(
		this, QSize( 34, 14 ), Button::Type::Push, "",
		pCommonStrings->getEditButton(), false, QSize(), tr( "Edit FX parameters") );
	m_pEditBtn->setObjectName( "MixerFXEditButton" );
	m_pEditBtn->move( 86, 25 );
	connect( m_pEditBtn, &Button::clicked, [&](){
#ifdef H2CORE_HAVE_LADSPA
		for ( int nnFX = 0; nnFX < MAX_FX; nnFX++ ) {
			auto pFX = Effects::get_instance()->getLadspaFX( nnFX );
			if ( pFX == m_pFX) {
				HydrogenApp::get_instance()->getLadspaFXProperties( nnFX )->hide();
				HydrogenApp::get_instance()->getLadspaFXProperties( nnFX )->show();
			}
		}
#else
		QMessageBox::critical(
			this, "Hydrogen", tr("LADSPA effects are not available in this version of Hydrogen.") );
#endif
	});

	// instrument name widget
	m_pNameLCD = new LCDDisplay( this, QSize( 108, 15 ), false, false );
	m_pNameLCD->move( 11, 9 );
	m_pNameLCD->setText( "No name" );
	m_pNameLCD->setToolTip( tr( "Ladspa FX name" ) );

	// m_pRotary
	m_pVolumeRotary = new Rotary(
		this, Rotary::Type::Normal, tr( "Effect return" ), false );
	m_pVolumeRotary->setDefaultValue( m_pVolumeRotary->getMax() );
	m_pVolumeRotary->move( 124, 4 );
	m_pVolumeRotary->setIsActive( false );
#ifdef H2CORE_HAVE_LADSPA
	connect( m_pVolumeRotary, &Rotary::valueChanged, [&]() {
		if ( m_pFX != nullptr ) {
			m_pFX->setVolume( m_pVolumeRotary->getValue() );
			HydrogenApp::get_instance()->showStatusBarMessage(
			tr( "Set volume [%1] of FX" )
					.arg( m_pVolumeRotary->getValue(), 0, 'f', 2 ),
			QString( "%1:rotaryChanged:%2" )
			.arg( class_name() ).arg( pFX->getPluginName() ) );

			Hydrogen::get_instance()->setIsModified( true );
		}
	});
#endif

	m_pReturnLbl = new ClickableLabel(
		this, QSize( 46, 9 ), pCommonStrings->getReturnLabel(),
		ClickableLabel::Color::Dark );
	m_pReturnLbl->move( 123, 30 );

	updateLine();
}

LadspaFXLine::~LadspaFXLine() {
}

void LadspaFXLine::updateLine() {
#ifdef H2CORE_HAVE_LADSPA
	if ( m_pFX == nullptr ) {
		m_pBypassBtn->setChecked( false );
		m_pBypassBtn->setIsActive( false );
		m_pNameLCD->setText( tr( "No plugin" ) );
		m_pVolumeRotary->setIsActive( false );

		return;
	}
	else {
		m_pBypassBtn->setIsActive( true );
		m_pVolumeRotary->setIsActive( true );
	}

	m_pBypassBtn->setChecked( ! m_pFX->isEnabled() );
	m_pNameLCD->setText( m_pFX->getPluginName() );

	if ( m_pFX->isEnabled() ) {
		m_pVolumeRotary->setValue( m_pFX->getVolume(), false,
								   Event::Trigger::Suppress );
	}
	else {
		m_pVolumeRotary->setIsActive( false );
	}

#else
	m_pBypassBtn->setIsActive( false );
	m_pEditBtn->setIsActive( false );
	m_pNameLCD->setIsActive( false );
	m_pVolumeRotary->setIsActive( false );
#endif
}

void LadspaFXLine::setFX( std::shared_ptr<H2Core::LadspaFX> pFX ) {
	m_pFX = pFX;
}
