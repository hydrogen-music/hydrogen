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

#include <core/Hydrogen.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

using namespace H2Core;

MidiControlButton::MidiControlButton( QWidget* pParent )
	: QToolButton( pParent )
	, m_bMidiInputEnabled( false )
	, m_bMidiOutputEnabled( false )
	, m_bMidiInputActive( false )
	, m_bMidiOutputActive( false )
{
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	setCheckable( true );
	setFocusPolicy( Qt::ClickFocus );
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

	updateActivation();
	updateIcons();

	HydrogenApp::get_instance()->addEventListener( this );
}

MidiControlButton::~MidiControlButton() {
}

void MidiControlButton::flashMidiInputIcon() {
	if ( m_bMidiInputEnabled ) {
		m_pMidiInputTimer->stop();
		m_bMidiInputActive = true;
		update();
		m_pMidiInputTimer->start(
			MidiControlButton::midiActivityTimeout );
	}
}

void MidiControlButton::flashMidiOutputIcon() {
	if ( m_bMidiOutputEnabled ) {
		m_pMidiOutputTimer->stop();
		m_bMidiOutputActive = true;
		update();
		m_pMidiOutputTimer->start( MidiControlButton::midiActivityTimeout );
	}
}

void MidiControlButton::updateActivation() {
	const auto pPref = H2Core::Preferences::get_instance();

	// No MIDI driver or device -> turn off
	auto pMidiDriver = Hydrogen::get_instance()->getMidiDriver();
	if ( pMidiDriver != nullptr ) {
		m_bMidiInputEnabled = pMidiDriver->isInputActive();
		m_bMidiOutputEnabled = pMidiDriver->isOutputActive();
	}
	else {
		m_bMidiInputEnabled = false;
		m_bMidiOutputEnabled = false;
	}

	setEnabled( pMidiDriver != nullptr );
	update();
}

void MidiControlButton::updateIcons() {
	QString sIconPath( Skin::getSvgImagePath() );
	if ( Preferences::get_instance()->getInterfaceTheme()->m_iconColor ==
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

void MidiControlButton::midiDriverChangedEvent() {
	updateActivation();
}

void MidiControlButton::midiInputEvent() {
	flashMidiInputIcon();
}

void MidiControlButton::midiOutputEvent() {
	flashMidiOutputIcon();
}

void MidiControlButton::paintEvent( QPaintEvent* pEvent ) {
	QToolButton::paintEvent( pEvent );

	const auto highlightColor =
		H2Core::Preferences::get_instance()->getColorTheme()->m_highlightColor;
	const auto disabledColor =
		H2Core::Preferences::get_instance()->getColorTheme()->m_lightColor;

	QPainter painter( this );

	////////////////////////////////////////////////////////////////////////////
	// Input symbol
	QRect inputIconRect( 0, height() / 2 - MidiControlButton::nIconWidth / 2,
						 MidiControlButton::nIconWidth,
						 MidiControlButton::nIconWidth );
 	if ( m_bMidiInputActive || ! m_bMidiInputEnabled ) {
		// Change the color of the input icon.
		QPixmap inputPixmap( inputIconRect.width(), inputIconRect.height() );
		inputPixmap.fill( Qt::GlobalColor::transparent );

		QPainter inputPainter( &inputPixmap );

		m_pIconInputSvg->render( &inputPainter );

		inputPainter.setCompositionMode(
			QPainter::CompositionMode_SourceIn);
		if ( m_bMidiInputEnabled ) {
			inputPainter.fillRect( inputPixmap.rect(), highlightColor );
		} else {
			inputPainter.fillRect( inputPixmap.rect(), disabledColor );
		}
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
	if ( !isEnabled() ) {
		QPixmap midiLogoPixmap( midiLogoRect.width(), midiLogoRect.height() );
		midiLogoPixmap.fill( Qt::GlobalColor::transparent );

		QPainter midiLogoPainter( &midiLogoPixmap );

		m_pMidiLogoSvg->render( &midiLogoPainter );

		midiLogoPainter.setCompositionMode( QPainter::CompositionMode_SourceIn );
        midiLogoPainter.fillRect( midiLogoPixmap.rect(), disabledColor );
		painter.drawPixmap( midiLogoRect, midiLogoPixmap );
	}
	else {
		m_pMidiLogoSvg->render( &painter, midiLogoRect );
	}

	////////////////////////////////////////////////////////////////////////////
	// Output symbol
	QRect outputIconRect( width() - MidiControlButton::nIconWidth,
						  height() / 2 - MidiControlButton::nIconWidth / 2,
						  MidiControlButton::nIconWidth,
						  MidiControlButton::nIconWidth );
 	if ( m_bMidiOutputActive || ! m_bMidiOutputEnabled ) {
		// Change the color of the output icon.
		QPixmap outputPixmap( outputIconRect.width(), outputIconRect.height() );
		outputPixmap.fill( Qt::GlobalColor::transparent );

		QPainter outputPainter( &outputPixmap );

		m_pIconOutputSvg->render( &outputPainter );

		outputPainter.setCompositionMode(
			QPainter::CompositionMode_SourceIn);
		if ( m_bMidiOutputEnabled ) {
			outputPainter.fillRect( outputPixmap.rect(), highlightColor );
		} else {
			outputPainter.fillRect( outputPixmap.rect(), disabledColor );
		}
		painter.drawPixmap( outputIconRect, outputPixmap );
	}
	else {
		m_pIconOutputSvg->render( &painter, outputIconRect );
	}
}
