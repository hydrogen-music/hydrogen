/*
 * Hydrogen
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "PanelGroupBox.h"
#include "../Skin.h"

#include "../HydrogenApp.h"
#include <core/Globals.h>
#include <core/Preferences/Preferences.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>

PanelGroupBox::PanelGroupBox( QWidget *pParent )
	: QWidget( pParent )
	, m_borderColor( Qt::black )
{
	setFocusPolicy( Qt::ClickFocus );

	auto pOverallLayout = new QHBoxLayout( this );
	pOverallLayout->setContentsMargins( 0, 0, 0, 0 );
	pOverallLayout->setSpacing( 0 );
	setLayout( pOverallLayout );

	auto pPanelGroupBox = new QWidget( this );
	pPanelGroupBox->setObjectName( "PanelGroupBox" );
	pOverallLayout->addWidget( pPanelGroupBox );

	m_pLayout = new QHBoxLayout( pPanelGroupBox );
	m_pLayout->setContentsMargins(
		PanelGroupBox::nMarginHorizontal, PanelGroupBox::nMarginVertical,
		PanelGroupBox::nMarginHorizontal, PanelGroupBox::nMarginVertical );
	m_pLayout->setSpacing( PanelGroupBox::nSpacing );
	pPanelGroupBox->setLayout( m_pLayout );

	updateStyleSheet();
}

PanelGroupBox::~PanelGroupBox() {
}

void PanelGroupBox::addWidget( QWidget* pWidget ) {
	if ( pWidget != nullptr ) {
		m_pLayout->addWidget( pWidget );
	}
}

void PanelGroupBox::updateStyleSheet() {
	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QColor colorBackground = colorTheme.m_widgetColor;

	setStyleSheet( QString( "\
#PanelGroupBox {\
     background-color: %1; \
     border: %2px solid %3;\
}")
				   .arg( colorBackground.name() )
				   .arg( PanelGroupBox::nBorder )
				   .arg( m_borderColor.name() ) );
}
