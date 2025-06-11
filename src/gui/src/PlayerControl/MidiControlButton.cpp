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


#include "MidiControlButton.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Widgets/LED.h"

#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

MidiControlButton::MidiControlButton( QWidget* pParent ) : Button( pParent ) {

	setObjectName( "MidiControlButton" );

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	auto pHBoxLayout = new QHBoxLayout();
	pHBoxLayout->setSpacing( 0 );
	pHBoxLayout->setContentsMargins( 0, 0, 0, 0 );
	setLayout( pHBoxLayout );

	// Midi Activity widget
	m_pMidiInputLED = new LED(
		this, QSize( MidiControlButton::nLEDWidth,
					 MidiControlButton::nLEDHeight ) );
	m_pMidiInputLED->setObjectName( "MidiInputLED" );
	m_pMidiInputLED->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pHBoxLayout->addWidget( m_pMidiInputLED );

	m_pLabel = new QLabel( pCommonStrings->getMidiLabel() );
	m_pLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
	pHBoxLayout->addWidget( m_pLabel );

	m_pMidiOutputLED = new LED(
		this, QSize( MidiControlButton::nLEDWidth,
					 MidiControlButton::nLEDHeight ) );
	m_pMidiOutputLED->setObjectName( "MidiOutputLED" );
	m_pMidiOutputLED->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pHBoxLayout->addWidget( m_pMidiOutputLED );

	m_pMidiInputTimer = new QTimer( this );
	connect( m_pMidiInputTimer, SIGNAL( timeout() ),
			 this, SLOT( deactivateMidiInputLED() ) );
	m_pMidiOutputTimer = new QTimer( this );
	connect( m_pMidiOutputTimer, SIGNAL( timeout() ),
			 this, SLOT( deactivateMidiOutputLED() ) );
}

MidiControlButton::~MidiControlButton() {
}

void MidiControlButton::flashMidiInputLED() {
	m_pMidiInputTimer->stop();
	m_pMidiInputLED->setActivated( true );
	m_pMidiInputTimer->start(
		MidiControlButton::midiActivityTimeout );
}

void MidiControlButton::flashMidiOutputLED() {
	m_pMidiOutputTimer->stop();
	m_pMidiOutputLED->setActivated( true );
	m_pMidiOutputTimer->start( MidiControlButton::midiActivityTimeout );
}

void MidiControlButton::updateActivation() {
	const auto pPref = H2Core::Preferences::get_instance();

	// No MIDI driver or device -> turn off
	// m_pMidiInputLED->setIsActive( true );
	// m_pMidiOutputLED->setIsActive( true );
}

void MidiControlButton::driverChangedEvent() {
	updateActivation();
}

void MidiControlButton::midiActivityEvent() {
	flashMidiInputLED();
	flashMidiOutputLED();
}

void MidiControlButton::deactivateMidiInputLED() {
	m_pMidiInputTimer->stop();
	m_pMidiInputLED->setActivated( false );
}

void MidiControlButton::deactivateMidiOutputLED() {
	m_pMidiOutputTimer->stop();
	m_pMidiOutputLED->setActivated( false );
}

void MidiControlButton::paintEvent( QPaintEvent* pEvent ) {
	DEBUGLOG( "" );
	Button::paintEvent( pEvent );
}
