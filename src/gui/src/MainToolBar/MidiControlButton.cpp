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

#include <QSvgRenderer>

#include "MainToolBar.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

MidiControlButton::MidiControlButton( QWidget* pParent )
	: QToolButton( pParent )
	, m_bMidiInputActive( false )
	, m_bMidiOutputActive( false )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setObjectName( "MidiControlButton" );

	setContentsMargins( MidiControlButton::nIconWidth, 0,
						MidiControlButton::nIconWidth, 0 );

	setText( "MIDI" );

	m_pIconInput = new QSvgRenderer(
		Skin::getSvgImagePath() + "/icons/black/mixer.svg", this );
	m_pIconOutput = new QSvgRenderer(
		Skin::getSvgImagePath() + "/icons/black/component-editor.svg", this );

	m_pMidiInputTimer = new QTimer( this );
	connect( m_pMidiInputTimer, &QTimer::timeout, [=]() {
		m_pMidiInputTimer->stop();
		m_bMidiInputActive = false;
		update();
	} );
	m_pMidiOutputTimer = new QTimer( this );
	connect( m_pMidiOutputTimer, &QTimer::timeout, [=]() {
		m_pMidiOutputTimer->stop();
		m_bMidiOutputActive = false;
		update();
	} );

	HydrogenApp::get_instance()->addEventListener( this );
}

MidiControlButton::~MidiControlButton() {
}

void MidiControlButton::flashMidiInputIcon() {
	m_pMidiInputTimer->stop();
	m_bMidiInputActive = true;
	update();
	m_pMidiInputTimer->start(
		MidiControlButton::midiActivityTimeout );
}

void MidiControlButton::flashMidiOutputIcon() {
	m_pMidiOutputTimer->stop();
	m_bMidiOutputActive = true;
	update();
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
	flashMidiInputIcon();
	flashMidiOutputIcon();
}

void MidiControlButton::paintEvent( QPaintEvent* pEvent ) {
	QToolButton::paintEvent( pEvent );

	const auto highlightColor =
		H2Core::Preferences::get_instance()->getTheme().m_color.m_highlightColor;

	QPainter painter( this );

	QRect inputIconRect( 0, MainToolBar::nMargin, MidiControlButton::nIconWidth,
						 height() - 2 * MainToolBar::nMargin );
 	if ( m_bMidiInputActive ) {
		// Change the color of the input icon.
		QPixmap inputPixmap( inputIconRect.width(), inputIconRect.height() );
		inputPixmap.fill( Qt::GlobalColor::transparent );

		QPainter inputPainter( &inputPixmap );

		m_pIconInput->render( &inputPainter );

		inputPainter.setCompositionMode(
			QPainter::CompositionMode_SourceIn);
		inputPainter.fillRect( inputPixmap.rect(), highlightColor );
		painter.drawPixmap( inputIconRect, inputPixmap );
	}
	else {
		m_pIconInput->render( &painter, inputIconRect );
	}

	QRect outputIconRect(
		width() - MidiControlButton::nIconWidth, MainToolBar::nMargin,
		MidiControlButton::nIconWidth, height() - 2 * MainToolBar::nMargin );
 	if ( m_bMidiOutputActive ) {
		// Change the color of the output icon.
		QPixmap outputPixmap( outputIconRect.width(), outputIconRect.height() );
		outputPixmap.fill( Qt::GlobalColor::transparent );

		QPainter outputPainter( &outputPixmap );

		m_pIconOutput->render( &outputPainter );

		outputPainter.setCompositionMode(
			QPainter::CompositionMode_SourceIn);
		outputPainter.fillRect( outputPixmap.rect(), highlightColor );
		painter.drawPixmap( outputIconRect, outputPixmap );
	}
	else {
		m_pIconOutput->render( &painter, outputIconRect );
	}
}
