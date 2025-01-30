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
#include "Rotary.h"
#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"

#include <cmath>

#include <QFile>
#include <QSvgRenderer>

#include <core/Globals.h>

Rotary::Rotary( QWidget* parent, Type type, QString sBaseTooltip, bool bUseIntSteps, float fMin, float fMax, bool bModifyOnChange )
	: WidgetWithInput( parent,
					   bUseIntSteps,
					   sBaseTooltip,
					   1, //nScrollSpeed,
					   5, // nScrollSpeedFast,
					   fMin,
					   fMax,
					   bModifyOnChange )
	, m_type( type ) {

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &Rotary::onPreferencesChanged );

	installEventFilter( HydrogenApp::get_instance()->getMainForm() );

	if ( type == Type::Small ) {
		m_nWidgetWidth = 18;
		m_nWidgetHeight = 18;
	} else {
		m_nWidgetWidth = 44;
		m_nWidgetHeight = 26;
	}

	if ( bUseIntSteps ) {
		m_fDefaultValue = static_cast<int>( type == Type::Center ? ( m_fMin + ( m_fMax - m_fMin ) / 2.0 ) : m_fMin );
	}
	else {
		m_fDefaultValue = ( type == Type::Center ? ( m_fMin + ( m_fMax - m_fMin ) / 2.0 ) : m_fMin );
	}

	m_fValue = m_fDefaultValue;
	updateTooltip();

	// Since the load function does not report success, we will check
	// for the existence of the knob image separately.
	QString sKnobPath( Skin::getSvgImagePath() + "/rotary.svg" ); 
	QFile knobFile( sKnobPath );
	if ( knobFile.exists() ) {
		m_knob = new QSvgRenderer( sKnobPath, this );
	} else {
		m_knob = nullptr;
		ERRORLOG( QString( "Unable to load knob image [%1]" ).arg( sKnobPath ) );
	}

	QString sBackgroundPath( Skin::getSvgImagePath() + "/rotary_background.svg" ); 
	QFile backgroundFile( sBackgroundPath );
	if ( backgroundFile.exists() ) {
		m_background = new QSvgRenderer( sBackgroundPath, this );
	} else {
		m_background = nullptr;
		ERRORLOG( QString( "Unable to load background image [%1]" ).arg( sBackgroundPath ) );
	}
	
	resize( m_nWidgetWidth, m_nWidgetHeight );
}

Rotary::~ Rotary() {
}

void Rotary::paintEvent( QPaintEvent* ev )
{

	auto pPref = H2Core::Preferences::get_instance();

	ev->accept();
	QPainter painter( this );

	painter.setRenderHint( QPainter::Antialiasing, true );

	QColor colorHighlightActive;
	QColor colorArc;
	QColor colorArcCenterSet;
	QColor colorArcCenterUnset;
	if ( m_bIsActive ) {
		colorHighlightActive = pPref->getColorTheme()->m_highlightColor;
		colorArc = Qt::red;
		colorArcCenterSet = Qt::green;
		colorArcCenterUnset = Qt::gray;
	} else {
		colorHighlightActive = pPref->getColorTheme()->m_lightColor;
		colorArc = Qt::darkGray;
		colorArcCenterSet = Qt::darkGray;
		colorArcCenterUnset = Qt::lightGray;
	}
	QColor colorHandle = Qt::black;
	QColor colorFont = Qt::white;

	// If the mouse is placed on the widget but the user hasn't
	// clicked it yet, the highlight will be done more transparent to
	// indicate that keyboard inputs are not accepted yet.
	if ( ! hasFocus() ) {
		colorHighlightActive.setAlpha( 150 );
	}
	
	QRect rectBackground( 0, 0, m_nWidgetWidth, m_nWidgetHeight );
	// Contains both the painted arc and the actual SVG image of the
	// knob.
	QRect rectRotary, rectArc;
	float fArcLineWidth, fBlackMargin;
	if ( m_type != Type::Small ) {
		// Center the rotary while using the full height of the widget.
		fArcLineWidth = 2.0;
		rectArc = QRect( ( m_nWidgetWidth - m_nWidgetHeight + fArcLineWidth ) * 0.5,
						 fArcLineWidth / 2,
						 m_nWidgetHeight - fArcLineWidth,
						 m_nWidgetHeight - fArcLineWidth );
		rectRotary = QRect( rectArc.x() + fArcLineWidth / 2,
							rectArc.y() + fArcLineWidth / 2,
							rectArc.width() - fArcLineWidth,
							rectArc.height() - fArcLineWidth );
	}
	
	if ( m_background != nullptr && m_type != Type::Small ) {
		m_background->render( &painter, rectBackground );
	}
			
	if ( m_bEntered || hasFocus()  ) {
		painter.fillRect( 0, m_nWidgetHeight - 2, m_nWidgetWidth, 2, colorHighlightActive );
	}

	if ( m_knob != nullptr ) {
		if ( m_type == Type::Small ) {
			m_knob->render( &painter, rectBackground );
		} else {
			m_knob->render( &painter, "layer2", rectRotary );
		}
	}

	if ( m_bIsActive ) {
		if ( m_type != Type::Small ) {
			if ( m_type == Type::Normal ) {
				int nStartAngle = 210 * 16; // given in 1/16 of a degree
				int nSpanAngle  = static_cast<int>( -239 * 16 * ( m_fValue - m_fMin ) / ( m_fMax - m_fMin ) );

				painter.setPen( QPen( colorArc, fArcLineWidth ) );
				painter.drawArc( rectArc, nStartAngle, nSpanAngle );
			} else {
				// Type::Center

				// There will be a special indication of the
				// center. Either as a gray dot or a bigger green one if
				// the value is smaller than +/-1% of the range around 0.
				if ( ( m_fValue - 0.5 * ( m_fMax + m_fMin ) ) == 0 ) {
				
					painter.setPen( QPen( colorArcCenterSet, fArcLineWidth * 1.25 ) );
					painter.drawArc( rectArc, 91 * 16, -3 * 16 );
				
				} else {
				
					painter.setPen( QPen( colorArcCenterUnset, fArcLineWidth * 1.25 ) );
					painter.drawArc( rectArc, 91 * 16, -3 * 16 );

					int nStartAngle = -18 * 16;
					int nSpanAngle  = static_cast<int>( -200* 16 * ( m_fValue - 0.5 * ( m_fMax + m_fMin ) ) / ( m_fMax - m_fMin ) );
					if ( m_fValue - 0.5 * ( m_fMax + m_fMin ) < 0 ) {
						nStartAngle *= -1;
						nStartAngle -= 2 * 16;
					}
					nStartAngle += 90 * 16;
				
					painter.setPen( QPen( colorArc, fArcLineWidth ) );
					painter.drawArc( rectArc, nStartAngle, nSpanAngle ); 
				}
			}
		}
	}
	
	float fCurrentAngle;
	float fStartAngle;
	if ( m_type == Type::Center ) {
		fStartAngle = -90 * M_PI / 180;
		fCurrentAngle = fStartAngle + 255 * M_PI / 180 * ( m_fValue - 0.5 * ( m_fMax + m_fMin ) ) / (  m_fMax - m_fMin );
	} else {
		fStartAngle = -90 * M_PI / 180;
		fCurrentAngle = fStartAngle + 255 * M_PI / 180 * ( m_fValue - m_fMin - 0.5 * ( m_fMax - m_fMin ) ) / ( m_fMax - m_fMin );
	}
	
	///////////////////////
	// Indicating the current position using a rotated line instead of
	// a dot
	///////////////////////
	//
	// float fLength, fWidth, fBaseX, fBaseY;
	// if ( m_type == Type::Small ) {
	// 	fBaseX = 9.0;
	// 	fBaseY = 9.0;
	// 	fLength = 4;
	// 	fWidth = 2;
	// } else {
	// 	fBaseX = 22.0;
	// 	fBaseY = 14.0;
	// 	fLength = 6;
	// 	fWidth = 3;
	// }
	//
	// QPointF p1( fBaseX + std::cos( fCurrentAngle + M_PI / 2 ) * fWidth / 2,
	// 			fBaseY + std::sin( fCurrentAngle + M_PI / 2 ) * fWidth / 2 );
	// QPointF p2( p1.x() + std::cos( fCurrentAngle ) * fLength,
	// 			p1.y() + std::sin( fCurrentAngle ) * fLength );
	// QPointF p3( p2.x() - std::cos( fCurrentAngle + M_PI / 2 ) * fWidth / 2,
	// 			p2.y() - std::sin( fCurrentAngle + M_PI / 2 ) * fWidth / 2 );
	// QPointF p4( p3.x() - std::cos( fCurrentAngle ) * fLength,
	// 			p3.y() - std::sin( fCurrentAngle ) * fLength );
	// QPainterPath path;
	// path.moveTo( p1 );
	// path.lineTo( p2 );
	// path.lineTo( p3 );
	// path.lineTo( p4 );
	//
	// path.setFillRule( Qt::WindingFill );
	// QPen pen( colorHandle );
	// pen.setJoinStyle( Qt::RoundJoin );
	// pen.setWidth( 1.7 );
	// painter.setPen( pen );
	// painter.setBrush( QBrush( colorHandle ) );
	// painter.drawPath( path );
	///////////////////////

	if ( m_bIsActive ) {
		float fDistance, fRadius, fBaseX, fBaseY;
		if ( m_type == Type::Small ) {
			fBaseX = static_cast<float>( m_nWidgetWidth ) / 2.0;
			fBaseY = static_cast<float>( m_nWidgetHeight ) / 2.0;
			fDistance = 3;
			fRadius = 1;
		} else {
			fBaseX = rectRotary.x() + rectRotary.width()/2;
			fBaseY = rectRotary.y() + rectRotary.height()/2;
			fDistance = 4;
			fRadius = 1.5;
		}

		QPointF p1( fBaseX + std::cos( fCurrentAngle ) * fDistance,
					fBaseY + std::sin( fCurrentAngle ) * fDistance );
		painter.setPen( QPen( colorHandle, 1 ) );
		painter.setBrush( QBrush( colorHandle ) );
		painter.drawEllipse( p1, fRadius, fRadius );
	}

	if ( m_type != Type::Small ) {
		QRectF leftTextRec( 2, 16, 7, 7 );
		QRectF rightTextRec( 34, 16, 9, 7 );

		QFont font( H2Core::Preferences::get_instance()->getApplicationFontFamily() );
		painter.setPen( QPen( colorFont, 3 ) );
		if ( std::fmod( m_fMin, 1 ) == 0 && std::fabs( m_fMin ) < 10 ) {
			font.setPixelSize( 7 );
			painter.setFont( font );
			painter.drawText( leftTextRec, Qt::AlignCenter, QString::number( m_fMin ) );
		} else {
			font.setPixelSize( 9 );
			painter.setFont( font );
			painter.drawText( leftTextRec, Qt::AlignCenter, "-" );
		}
		if ( std::fmod( m_fMax, 1 ) == 0 && std::fabs( m_fMax ) < 10 ) {
			font.setPixelSize( 7 );
			painter.setFont( font );
			painter.drawText( rightTextRec, Qt::AlignCenter, QString::number( m_fMax ) );
		} else {
			font.setPixelSize( 9 );
			painter.setFont( font );
			painter.drawText( rightTextRec, Qt::AlignCenter, "+" );
		}
	}
}

void Rotary::onPreferencesChanged( H2Core::Preferences::Changes changes ) {	
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		update();
	}
}
