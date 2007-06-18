/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Rotary.cpp,v 1.5 2005/05/01 19:51:23 comix Exp $
 *
 */
#include "Rotary.h"
#include "gui/Skin.h"

#include <qtooltip.h>
#include <qcursor.h>

RotaryTooltip::RotaryTooltip( QPoint pos )
  : QWidget( 0, "RotaryTooltip", Qt::WStyle_Customize| Qt::WStyle_NoBorder | Qt::WStyle_StaysOnTop| Qt::WX11BypassWM )
{
	m_pDisplay = new LCDDisplay( this, LCDDigit::SMALL_BLUE, 4);
	m_pDisplay->move( 0, 0 );
	resize( m_pDisplay->size() );
	setPaletteBackgroundColor( QColor( 49, 53, 61 ) );
}


void RotaryTooltip::showTip( QPoint pos, QString sText )
{
	move( pos );
	m_pDisplay->setText( sText.ascii() );
	show();
}

RotaryTooltip::~RotaryTooltip()
{
	delete m_pDisplay;
}





///////////////////

QPixmap* Rotary::m_background_normal = NULL;
QPixmap* Rotary::m_background_center = NULL;


Rotary::Rotary( QWidget* parent, RotaryType type, QString sToolTip, bool bUseValueTip )
 : QWidget( parent , "Rotary", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "Rotary" )
 , m_bChanged( true )
 , m_type( type )
 , m_nLastFrame( -1 )
 , m_bShowValueToolTip( bUseValueTip )
{
	QToolTip::add( this, sToolTip );

	m_pValueToolTip = new RotaryTooltip( mapToGlobal( QPoint( 0, 0 ) ) );

	m_nWidgetWidth = 28;
	m_nWidgetHeight = 26;
	m_fValue = 0.0;

	if ( m_background_normal == NULL ) {
		string sBackground_path = Skin::getImagePath() + string( "/mixerPanel/rotary_images.png" );
		m_background_normal = new QPixmap();
		if ( m_background_normal->load( sBackground_path.c_str() ) == false ){
			errorLog( string("Error loading pixmap ") + sBackground_path );
		}
	}
	if ( m_background_center == NULL ) {
		string sBackground_path = Skin::getImagePath() + string( "/mixerPanel/rotary_center_images.png" );
		m_background_center = new QPixmap();
		if ( m_background_center->load( sBackground_path.c_str() ) == false ){
			errorLog( string("Error loading pixmap ") + sBackground_path );
		}
	}

	resize( m_nWidgetWidth, m_nWidgetHeight );
	m_temp.resize( m_nWidgetWidth, m_nWidgetHeight );
}



Rotary::~ Rotary()
{
	delete m_pValueToolTip;
}



void Rotary::paintEvent( QPaintEvent* ev )
{
	if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;

		int nFrame = (int)(63.0 * m_fValue);

		if ( m_nLastFrame != nFrame ) {
			m_nLastFrame = nFrame;
			int xPos = m_nWidgetWidth * nFrame;
			if ( m_type == TYPE_NORMAL ) {
				bitBlt(&m_temp, 0, 0, m_background_normal, xPos, 0, m_nWidgetWidth, m_nWidgetHeight, CopyROP);
			}
			else {
				bitBlt(&m_temp, 0, 0, m_background_center, xPos, 0, m_nWidgetWidth, m_nWidgetHeight, CopyROP);
			}
		}
	}
	bitBlt( this, 0, 0, &m_temp, 0, 0, m_nWidgetWidth, m_nWidgetHeight, CopyROP);
}



void Rotary::setValue( float fValue )
{
	if ( fValue == m_fValue ) {
		return;
	}

	if ( fValue < 0.0 ) {
		fValue = 0.0;
	}
	else if ( fValue > 1.0 ) {
		fValue = 1.0;
	}
	m_fValue = fValue;
	m_bChanged = true;

	updateRotary();
}



void Rotary::mousePressEvent(QMouseEvent *ev)
{
	setCursor( QCursor( Qt::SizeVerCursor ) );

	m_fMousePressValue = m_fValue;
	m_fMousePressY = ev->y();

	if ( m_bShowValueToolTip ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", m_fValue );
		m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), QString( tmp ) );
	}
}




void Rotary::mouseReleaseEvent( QMouseEvent *ev )
{
	setCursor( QCursor( Qt::ArrowCursor ) );
	m_pValueToolTip->hide();
}


void Rotary::wheelEvent ( QWheelEvent *ev )
{
//	infoLog("wheelEvent delta: " + toString( ev->delta() ) );
	ev->accept();

	if ( ev->delta() > 0 ) {
		setValue( m_fValue + 0.025 );
	}
	else {
		setValue( m_fValue - 0.025 );
	}
	update();
	emit valueChanged(this);
}



 void Rotary::mouseMoveEvent( QMouseEvent *ev ) {
	float y = ev->y() - m_fMousePressY;
	float fNewValue = m_fMousePressValue - ( y / 100.0 );
	setValue( fNewValue );
	update();
	emit valueChanged(this);

	if ( m_bShowValueToolTip ) {
		char tmp[20];
		sprintf( tmp, "%#.2f", m_fValue );
		m_pValueToolTip->showTip( mapToGlobal( QPoint( -38, 1 ) ), QString( tmp ) );
	}
}



void Rotary::updateRotary()
{
	if (m_bChanged && isVisible()) {
		update();
	}
}


