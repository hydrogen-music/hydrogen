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

#include "../InstrumentEditor/InstrumentEditor.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../CommonStrings.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/Fader.h"
#include "../Widgets/Rotary.h"
#include "../Widgets/Button.h"
#include "../Widgets/LCDDisplay.h"
#include "../Widgets/LED.h"
#include "../Widgets/WidgetWithInput.h"

#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/MidiAction.h>

using namespace H2Core;

LadspaFXLine::LadspaFXLine( QWidget* pParent )
	: PixmapWidget( pParent )
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
	connect( m_pBypassBtn, SIGNAL( clicked() ), this, SLOT( bypassBtnClicked() ) );

	// edit button
	m_pEditBtn = new Button(
		this, QSize( 34, 14 ), Button::Type::Push, "",
		pCommonStrings->getEditButton(), false, QSize(), tr( "Edit FX parameters") );
	m_pEditBtn->setObjectName( "MixerFXEditButton" );
	m_pEditBtn->move( 86, 25 );
	connect( m_pEditBtn, SIGNAL( clicked() ), this, SLOT( editBtnClicked() ) );

	// instrument name widget
	m_pNameLCD = new LCDDisplay( this, QSize( 108, 15 ), false, false );
	m_pNameLCD->move( 11, 9 );
	m_pNameLCD->setText( "No name" );
	m_pNameLCD->setToolTip( tr( "Ladspa FX name" ) );

	// m_pRotary
	m_pRotary = new Rotary(
		this, Rotary::Type::Normal, tr( "Effect return" ), false );
	m_pRotary->setDefaultValue( m_pRotary->getMax() );
	m_pRotary->move( 124, 4 );
	m_pRotary->setIsActive( false );
	connect( m_pRotary, SIGNAL( valueChanged( WidgetWithInput* ) ),
			 this, SLOT( rotaryChanged( WidgetWithInput* ) ) );

	m_pReturnLbl = new ClickableLabel(
		this, QSize( 46, 9 ), pCommonStrings->getReturnLabel(),
		ClickableLabel::Color::Dark );
	m_pReturnLbl->move( 123, 30 );
}

LadspaFXLine::~LadspaFXLine() {
}

void LadspaFXLine::setName( const QString& sName ) {
	m_pNameLCD->setText( sName );
}

void LadspaFXLine::bypassBtnClicked() {
	emit bypassBtnClicked( this );
}
void LadspaFXLine::editBtnClicked() {
	emit editBtnClicked( this );
}

bool LadspaFXLine::isFxBypassed() const {
	return ( ( m_pBypassBtn->isChecked() && ! m_pBypassBtn->isDown() ) ||
			 ( ! m_pBypassBtn->isChecked() && m_pBypassBtn->isDown() ) );
}

void LadspaFXLine::setFxBypassed( bool bBypassed ) {
	if ( ! m_pBypassBtn->isDown() ) {
		m_pBypassBtn->setChecked( bBypassed );
	}
	m_pRotary->setIsActive( ! bBypassed );
}

void LadspaFXLine::rotaryChanged( WidgetWithInput *ref ) {
	emit volumeChanged( this );
	UNUSED( ref );
}

float LadspaFXLine::getVolume() const {
	return m_pRotary->getValue();
}

void LadspaFXLine::setVolume( float value, H2Core::Event::Trigger trigger ) {
	m_pRotary->setValue( value, false, trigger );
}
