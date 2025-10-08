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

#include "InstrumentNameWidget.h"

#include "../HydrogenApp.h"

#include <core/Preferences/Theme.h>

using namespace H2Core;

InstrumentNameWidget::InstrumentNameWidget(QWidget* parent)
 : PixmapWidget( parent )
{
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &InstrumentNameWidget::onPreferencesChanged );

	setPixmap( "/mixerPanel/mixerline_label_background.png" );

	this->resize( InstrumentNameWidget::nWidth, InstrumentNameWidget::nHeight );
}

InstrumentNameWidget::~InstrumentNameWidget() {
}

void InstrumentNameWidget::paintEvent( QPaintEvent* ev ) {

	const auto pPref = H2Core::Preferences::get_instance();

	PixmapWidget::paintEvent( ev );

	QPainter p( this );
	
	QFont font( pPref->getFontTheme()->m_sApplicationFontFamily,
				getPointSize( pPref->getFontTheme()->m_fontSize ) );

	p.setPen( QColor(230, 230, 230) );
	p.setFont( font );
	p.rotate( -90 );
	p.drawText( -InstrumentNameWidget::nHeight + 5, 0,
				InstrumentNameWidget::nHeight - 10, InstrumentNameWidget::nWidth,
				Qt::AlignVCenter, m_sInstrName );
}

void InstrumentNameWidget::setText( const QString& sText ) {
	if ( m_sInstrName != sText ) {
		m_sInstrName = sText;
		update();
	}
}

const QString& InstrumentNameWidget::text() const {
	return m_sInstrName;
}

void InstrumentNameWidget::mousePressEvent( QMouseEvent * e ) {
	UNUSED( e );
	emit clicked();
}

void InstrumentNameWidget::mouseDoubleClickEvent( QMouseEvent * e ) {
	UNUSED( e );
	emit doubleClicked();
}

void InstrumentNameWidget::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	if ( changes & H2Core::Preferences::Changes::Font ) {
		update();
	}
}
