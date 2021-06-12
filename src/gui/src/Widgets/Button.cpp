/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "Button.h"

#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../CommonStrings.h"
#include "MidiSenseWidget.h"

#include <qglobal.h>	// for QT_VERSION

#include <core/Globals.h>
#include <core/Preferences.h>
#include <core/Hydrogen.h>

const char* Button::__class_name = "Button";

Button::Button( QWidget *pParent, QSize size, Type type, const QString& sIcon, const QString& sText, bool bUseRedBackground, QSize iconSize, QString sBaseTooltip )
	: QPushButton( pParent )
	, Object( __class_name )
	, m_size( size )
	, m_bEntered( false )
	, m_iconSize( iconSize )
	, m_sBaseTooltip( sBaseTooltip )
	, m_sRegisteredMidiEvent( "" )
	, m_nRegisteredMidiParameter( 0 )
{
	m_lastUsedFontSize = H2Core::Preferences::get_instance()->getFontSize();
	m_sLastUsedFontFamily = H2Core::Preferences::get_instance()->getLevel3FontFamily();
	
	setAttribute( Qt::WA_OpaquePaintEvent );
	setFocusPolicy( Qt::NoFocus );
	
	adjustSize();
	setFixedSize( size );

	if ( ! sIcon.isEmpty() ) {
		setIcon( QIcon( Skin::getSvgImagePath() + "/icons/" + sIcon ) );
		setIconSize( iconSize );
	} else {
		setText( sText );
	}

	if ( bUseRedBackground ) {
		setStyleSheet( "QPushButton {"
					   "color: #0a0a0a;"
					   "background-color: #9fa3af;"
					   "}"
					   "QPushButton:checked {"
					   "background-color: #ff6767;"
					   "}"
					   );
	} else {
		setStyleSheet( "QPushButton {"
					   "background-color: #9fa3af;"
					   "color: #0a0a0a;"
					   "}"
					   "QPushButton:checked {"
					   "background-color: #61a7fb;"
					   "}"
					   );
	}
	
	if ( type == Type::Toggle ) {
		setCheckable( true );
	} else {
		setCheckable( false );
	}

	updateFont();
	
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &Button::onPreferencesChanged );
	resize( size );
}

Button::~Button() {
}

void Button::setBaseToolTip( const QString& sNewTip ) {
	m_sBaseTooltip = sNewTip;
	updateTooltip();
}

void Button::mousePressEvent(QMouseEvent*ev) {
	
	/*
	*  Shift + Left-Click activate the midi learn widget
	*/
	
	if ( ev->button() == Qt::LeftButton && ( ev->modifiers() & Qt::ShiftModifier ) ){
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();

		// Store the registered MIDI event and parameter in order to
		// show them in the tooltip. Looking them up in the MidiMap
		// using the Action associated to the Widget might not yield a
		// unique result since the Action can be registered from the
		// PreferencesDialog as well.
		m_sRegisteredMidiEvent = H2Core::Hydrogen::get_instance()->lastMidiEvent;
		m_nRegisteredMidiParameter = H2Core::Hydrogen::get_instance()->lastMidiEventParameter;
		
		updateTooltip();
		return;
	}

	QPushButton::mousePressEvent( ev );
}

void Button::enterEvent( QEvent *ev )
{
	QPushButton::enterEvent( ev );
	m_bEntered = true;
}

void Button::leaveEvent( QEvent *ev )
{
	QPushButton::leaveEvent( ev );
	m_bEntered = false;
}

void Button::updateTooltip() {

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	QString sTip = QString("%1" ).arg( m_sBaseTooltip );

	// Add the associated MIDI action.
	if ( m_action != nullptr ) {
		sTip.append( QString( "\n%1: %2 " ).arg( pCommonStrings->getMidiTooltipHeading() )
					 .arg( m_action->getType() ) );
		if ( ! m_sRegisteredMidiEvent.isEmpty() ) {
			sTip.append( QString( "%1 [%2 : %3]" ).arg( pCommonStrings->getMidiTooltipBound() )
						 .arg( m_sRegisteredMidiEvent ).arg( m_nRegisteredMidiParameter ) );
		} else {
			sTip.append( QString( "%1" ).arg( pCommonStrings->getMidiTooltipUnbound() ) );
		}
	}
			
	setToolTip( sTip );
}

void Button::updateFont() {
	
	float fScalingFactor = 1.0;
    switch ( m_lastUsedFontSize ) {
    case H2Core::Preferences::FontSize::Small:
		fScalingFactor = 1.5;
		break;
    case H2Core::Preferences::FontSize::Normal:
		fScalingFactor = 1.0;
		break;
    case H2Core::Preferences::FontSize::Large:
		fScalingFactor = 0.75;
		break;
	}

	int nMargin, nPixelSize;
	if ( m_size.width() <= 11 || m_size.height() <= 11 ) {
		nMargin = 1;
	} else {
		nMargin = 5;
	}
	
	if ( m_size.width() >= m_size.height() ) {
		nPixelSize = m_size.height() - std::round( fScalingFactor * nMargin );
	} else {
		nPixelSize = m_size.width() - std::round( fScalingFactor * nMargin );
	}

	QFont font( m_sLastUsedFontFamily );
	font.setPixelSize( nPixelSize );
	setFont( font );
}

void Button::paintEvent( QPaintEvent* ev )
{
	QPushButton::paintEvent( ev );
	
	QPainter painter( this );
	if ( m_bEntered || hasFocus() ) {
		QPainter painter(this);
	
		QColor colorHighlightActive = QColor( 97, 167, 251);

		// If the mouse is placed on the widget but the user hasn't
		// clicked it yet, the highlight will be done more transparent to
		// indicate that keyboard inputs are not accepted yet.
		if ( ! hasFocus() ) {
			colorHighlightActive.setAlpha( 150 );
		}
	
		painter.fillRect( 0, m_size.height() - 2, m_size.width(), 2, colorHighlightActive );
	}
}

void Button::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( m_sLastUsedFontFamily != pPref->getLevel3FontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = pPref->getFontSize();
		m_sLastUsedFontFamily = pPref->getLevel3FontFamily();
		updateFont();
	}
}
