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

#include "MetronomeButton.h"

#include "../Skin.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include <chrono>

MetronomeButton::MetronomeButton( QWidget *pParent, const QSize& size )
	: Button( pParent,
			  size,
			  Button::Type::Toggle,
			  "metronome.svg", /* icon */
			  "", /* label */
			  QSize( size.width() - 6, size.height() - 6 ), /* icon size */
			  tr( "Switch metronome on/off" ), /* tooltip */
			  false, /* colorful */
			  true /* modify on change */ )
{
	m_pTimer = new QTimer();
	connect( m_pTimer, &QTimer::timeout, [=]() {
		setCheckedBackgroundColor( m_colorBackgroundChecked );
		m_pTimer->stop();
	} );

	updateStyleSheet();
}

MetronomeButton::~MetronomeButton() {
}

void MetronomeButton::metronomeEvent( int nValue ) {
	// Only trigger LED if the metronome button was pressed or it was
	// activated via MIDI or OSC.
	//
	// Value 2 corresponds to the metronome being turned on or off an is not
	// handled in here neither.
	if ( ! H2Core::Preferences::get_instance()->m_bUseMetronome ||
		 nValue == 2 ) {
		return;
	}

	if ( nValue == 0 ) {
		setCheckedBackgroundColor( m_colorFirstBeat );
	}
	else {
		setCheckedBackgroundColor( m_colorBeat );
	}

	// Percentage [0,1] the button stays highlighted over the duration of a
	// beat. We make this one tempo-dependent so it works for both small and
	// high tempi as well.
	const float fPercentage = 0.5;
	const auto fBpm = H2Core::Hydrogen::get_instance()->getAudioEngine()->
		getTransportPosition()->getBpm();
	const std::chrono::milliseconds duration{ static_cast<int>(
		std::round( 60 * 1000 / fBpm * fPercentage))};
	m_pTimer->start( duration );
}

void MetronomeButton::updateStyleSheet() {
	const auto pPref = H2Core::Preferences::get_instance();
	m_colorBackgroundChecked = pPref->getTheme().m_color.m_accentColor;
	m_colorFirstBeat = pPref->getTheme().m_color.m_highlightColor;

	const int nScaling = 155;
	if ( Skin::moreBlackThanWhite( m_colorBackgroundChecked ) ) {
		m_colorBeat = m_colorBackgroundChecked.darker( nScaling );
	} else {
		m_colorBeat = m_colorBackgroundChecked.lighter( nScaling );
	}
}
