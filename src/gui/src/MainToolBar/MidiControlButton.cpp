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

using namespace H2Core;

MidiControlButton::MidiControlButton( QWidget* pParent )
	: QToolButton( pParent )
	, m_bMidiInputActive( false )
	, m_bMidiOutputActive( false )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setObjectName( "MidiControlButton" );

	setFixedWidth( MidiControlButton::nIconWidth * 2 +
				   MainToolBar::nSpacing * 2 +
				   MidiControlButton::nLogoWidth );

	m_pIconInputSvg = new QSvgRenderer( this );
	m_pMidiLogoSvg = new QSvgRenderer( this );
	m_pIconOutputSvg = new QSvgRenderer( this );

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

	updateIcons();

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

void MidiControlButton::updateIcons() {
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getTheme().m_interface.m_iconColor ==
		 InterfaceTheme::IconColor::White ) {
		sIconPath.append( "/icons/white/" );
	} else {
		sIconPath.append( "/icons/black/" );
	}

	m_pIconInputSvg->load( sIconPath + "midi-input.svg" );
	m_pMidiLogoSvg->load( sIconPath + "midi-logo.svg" );
	m_pIconOutputSvg->load( sIconPath + "midi-output.svg" );
	update();
}

void MidiControlButton::driverChangedEvent() {
	updateActivation();
}

void MidiControlButton::midiInputEvent() {
	flashMidiInputIcon();
	flashMidiOutputIcon();
}

void MidiControlButton::paintEvent( QPaintEvent* pEvent ) {
	QToolButton::paintEvent( pEvent );

	const auto highlightColor =
		H2Core::Preferences::get_instance()->getTheme().m_color.m_highlightColor;

	QPainter painter( this );

	////////////////////////////////////////////////////////////////////////////
	// Input symbol
	QRect inputIconRect( 0, height() / 2 - MidiControlButton::nIconWidth / 2,
						 MidiControlButton::nIconWidth,
						 MidiControlButton::nIconWidth );
 	if ( m_bMidiInputActive ) {
		// Change the color of the input icon.
		QPixmap inputPixmap( inputIconRect.width(), inputIconRect.height() );
		inputPixmap.fill( Qt::GlobalColor::transparent );

		QPainter inputPainter( &inputPixmap );

		m_pIconInputSvg->render( &inputPainter );

		inputPainter.setCompositionMode(
			QPainter::CompositionMode_SourceIn);
		inputPainter.fillRect( inputPixmap.rect(), highlightColor );
		painter.drawPixmap( inputIconRect, inputPixmap );
	}
	else {
		m_pIconInputSvg->render( &painter, inputIconRect );
	}

	////////////////////////////////////////////////////////////////////////////
	// MIDI logo
	QRect midiLogoRect(
		MidiControlButton::nIconWidth + MainToolBar::nSpacing,
		MainToolBar::nMargin + 1,
		MidiControlButton::nLogoWidth, height() - 2 * MainToolBar::nMargin - 2 );
	m_pMidiLogoSvg->render( &painter, midiLogoRect );

	////////////////////////////////////////////////////////////////////////////
	// Output symbol
	QRect outputIconRect( width() - MidiControlButton::nIconWidth,
						  height() / 2 - MidiControlButton::nIconWidth / 2,
						  MidiControlButton::nIconWidth,
						  MidiControlButton::nIconWidth );
 	if ( m_bMidiOutputActive ) {
		// Change the color of the output icon.
		QPixmap outputPixmap( outputIconRect.width(), outputIconRect.height() );
		outputPixmap.fill( Qt::GlobalColor::transparent );

		QPainter outputPainter( &outputPixmap );

		m_pIconOutputSvg->render( &outputPainter );

		outputPainter.setCompositionMode(
			QPainter::CompositionMode_SourceIn);
		outputPainter.fillRect( outputPixmap.rect(), highlightColor );
		painter.drawPixmap( outputIconRect, outputPixmap );
	}
	else {
		m_pIconOutputSvg->render( &painter, outputIconRect );
	}
}
