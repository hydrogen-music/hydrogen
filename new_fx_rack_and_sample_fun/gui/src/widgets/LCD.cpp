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

#include "LCD.h"

#include "../Skin.h"

#include <QtGui>

#include <hydrogen/globals.h>

QPixmap* LCDDigit::m_pSmallBlueFontSet = NULL;
QPixmap* LCDDigit::m_pSmallRedFontSet = NULL;
QPixmap* LCDDigit::m_pLargeGrayFontSet = NULL;
QPixmap* LCDDigit::m_pSmallGrayFontSet = NULL;

LCDDigit::LCDDigit( QWidget * pParent, LCDType type )
 : QWidget( pParent )
 , Object( "LCDDigit" )
 , m_type( type )
{
	setAttribute(Qt::WA_NoBackground);

	switch ( m_type ) {
		case SMALL_BLUE:
		case SMALL_RED:
			resize( 8, 11 );
			break;

		case LARGE_GRAY:
			resize( 14, 16 );
			break;

		case SMALL_GRAY:
		resize( 12, 11 );
		break;
}

	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	// Small blue FontSet image
	if (m_pSmallBlueFontSet == NULL ) {
		QString sSmallBlueFontSet = Skin::getImagePath() + "/lcd/LCDSmallBlueFontSet.png";
		m_pSmallBlueFontSet = new QPixmap();
		bool ok = m_pSmallBlueFontSet->load( sSmallBlueFontSet );
		if( ok == false ) {
			ERRORLOG( "Error loading pixmap");
		}
	}

	// Small red FontSet image
	if (m_pSmallRedFontSet == NULL ) {
		QString sSmallRedFontSet = Skin::getImagePath() + "/lcd/LCDSmallRedFontSet.png";
		m_pSmallRedFontSet = new QPixmap();
		bool ok = m_pSmallRedFontSet->load( sSmallRedFontSet );
		if( ok == false ) {
			ERRORLOG( "Error loading pixmap" );
		}
	}

	// Large gray FontSet image
	if (m_pLargeGrayFontSet == NULL ) {
		QString sLargeGrayFontSet = Skin::getImagePath() + "/lcd/LCDLargeGrayFontSet.png";
		m_pLargeGrayFontSet = new QPixmap();
		bool ok = m_pLargeGrayFontSet->load( sLargeGrayFontSet );
		if( ok == false ) {
			ERRORLOG( "Error loading pixmap" );
		}
	}

	// Small gray FontSet image
	if (m_pSmallGrayFontSet == NULL ) {
		QString sSmallGrayFontSet = Skin::getImagePath() + "/lcd/LCDSmallGrayFontSet.png";
		m_pSmallGrayFontSet = new QPixmap();
		bool ok = m_pSmallGrayFontSet->load( sSmallGrayFontSet );
		if( ok == false ) {
			ERRORLOG( "Error loading pixmap" );
		}
	}

	set( ' ' );
}


LCDDigit::~LCDDigit()
{
//	delete m_pSmallBlueFontSet;
//	m_pSmallBlueFontSet = NULL;

//	delete m_pSmallRedFontSet;
//	m_pSmallRedFontSet = NULL;
}


//void LCDDigit::mousePressEvent(QMouseEvent *ev)
void LCDDigit::mouseReleaseEvent(QMouseEvent* ev)
{
	UNUSED( ev );
	emit digitClicked();
}


void LCDDigit::paintEvent(QPaintEvent *ev)
{
	UNUSED( ev );

	int x = m_nCol * width();
	int y = m_nRow * height();

	QPainter painter(this);

	switch ( m_type ) {
		case SMALL_BLUE:
			painter.drawPixmap( rect(), *m_pSmallBlueFontSet, QRect( x, y, width(), height() ) );
			break;

		case SMALL_RED:
			painter.drawPixmap( rect(), *m_pSmallRedFontSet, QRect( x, y, width(), height() ) );
			break;

		case LARGE_GRAY:
			painter.drawPixmap( rect(), *m_pLargeGrayFontSet, QRect( x, y, width(), height() ) );
			break;

		case SMALL_GRAY:
			painter.drawPixmap( rect(), *m_pSmallGrayFontSet, QRect( x, y, width(), height() ) );
			break;

		default:
			ERRORLOG( "[paint] Unhandled type" );
			painter.setPen(Qt::blue);
			painter.drawText(rect(), Qt::AlignCenter, "!");
	}
}


void LCDDigit::set( char ch )
{
	int MAXCOL = 66;
	const char keymap[] = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ',
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ',
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', ' ',
			'-', ':', '/', '\\', ',', ';', '.', ' ', ' ', ' ', '#'
	};
	for ( int n = 0; n < 77; n++ ) { //73
		if ( keymap[ n ] == ch ) {
			m_nCol = n % MAXCOL;
			m_nRow = n / MAXCOL;
			break;
		}
	}

	update();
}


void LCDDigit::setSmallRed()
{
	if ( m_type != SMALL_RED ) {
		m_type = SMALL_RED;
		update();
	}
}


void LCDDigit::setSmallBlue()
{
	if ( m_type != SMALL_BLUE ) {
		m_type = SMALL_BLUE;
		update();
	}
}



// ::::::::::::::::::




LCDDisplay::LCDDisplay( QWidget * pParent, LCDDigit::LCDType type, int nDigits, bool leftAlign )
 : QWidget( pParent )
 , m_sMsg( "" )
 , m_bLeftAlign( leftAlign )
{

	for ( int n = 0; n < nDigits; n++ ) {
		LCDDigit *pDigit = new LCDDigit( this, type );
		if ( ( type == LCDDigit::LARGE_GRAY ) || ( type == LCDDigit::SMALL_GRAY ) ) {
			pDigit->move( pDigit->width() * n, 0 );
		}
		else {
			pDigit->move( pDigit->width() * n + 2, 2 );
		}
		connect( pDigit, SIGNAL( digitClicked() ), this, SLOT( digitClicked() ) );
		m_pDisplay.push_back( pDigit );
	}

	if ( ( type == LCDDigit::LARGE_GRAY ) || ( type == LCDDigit::SMALL_GRAY ) ) {
		int w = m_pDisplay[ 0 ]->width() * nDigits;
		int h = m_pDisplay[ 0 ]->height();

		resize( w, h );
	}
	else {
		int w = m_pDisplay[ 0 ]->width() * nDigits + 4;
		int h = m_pDisplay[ 0 ]->height() + 4;

		resize( w, h );
	}
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	setText( "    ");

	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );

}




LCDDisplay::~LCDDisplay() {
//	for ( uint i = 0; i < m_pDisplay.size(); i++ ) {
//		delete m_pDisplay[ i ];
//	}
}


void LCDDisplay::setText( const QString& sMsg )
{
	if ( sMsg == m_sMsg ) {
		return;
	}

	m_sMsg = sMsg;
	int nLen = sMsg.length();



	if ( m_bLeftAlign ) {
		for ( int i = 0; i < (int)m_pDisplay.size(); ++i ) {
			if ( i < nLen ) {
                            m_pDisplay[ i ]->set( sMsg.toLocal8Bit().at(i) );
			}
			else {
				m_pDisplay[ i ]->set( ' ' );
			}
		}
	}
	else {
		// right aligned
		int nPadding = 0;
		if ( nLen < (int)m_pDisplay.size() ) {
			nPadding = m_pDisplay.size() - nLen;
		}
		else {
			nLen = m_pDisplay.size();
		}

		for ( int i = 0; i < nPadding; i++) {
			m_pDisplay[ i ]->set( ' ' );
		}

		for ( int i = 0; i < nLen; i++ ) {
			m_pDisplay[ i + nPadding ]->set( sMsg.toLocal8Bit().at(i) );
		}
	}
}



void LCDDisplay::setSmallRed()
{
	for ( uint i = 0; i < m_pDisplay.size(); i++) {
		m_pDisplay[ i ]->setSmallRed();
	}
}

void LCDDisplay::setSmallBlue()
{
	for ( uint i = 0; i < m_pDisplay.size(); i++) {
		m_pDisplay[ i ]->setSmallBlue();
	}
}

void LCDDisplay::digitClicked()
{
	emit displayClicked( this );
}

// :::::::::::::::::::



// used in PlayerControl
LCDSpinBox::LCDSpinBox( QWidget *pParent, int nDigits, LCDSpinBoxType type, int nMin, int nMax )
 : QWidget( pParent )
 , Object( "LCDSpinBox" )
 , m_type( type )
 , m_fValue( 0 )
 , m_nMinValue( nMin )
 , m_nMaxValue( nMax )
{
	m_pDisplay = new LCDDisplay( this, LCDDigit::LARGE_GRAY, nDigits );
	connect( m_pDisplay, SIGNAL( displayClicked(LCDDisplay*) ), this, SLOT( displayClicked(LCDDisplay*) ) );

	resize( m_pDisplay->width(), m_pDisplay->height() );
	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	setValue( 0 );
}



LCDSpinBox::~LCDSpinBox()
{
	delete m_pDisplay;
//	delete m_pUpBtn;
//	delete m_pDownBtn;
}


void LCDSpinBox::upBtnClicked()
{
	switch( m_type ) {
		case INTEGER:
			if ( m_nMaxValue != -1 && m_fValue < m_nMaxValue ) {
				setValue( m_fValue + 1);
			}
			break;
		case FLOAT:
			if ( m_nMaxValue != -1 && m_fValue < (float)m_nMaxValue ) {
				setValue( m_fValue + 1.0);
			}
			break;
	}

	emit changed(this);
}

void LCDSpinBox::downBtnClicked()
{
	switch( m_type ) {
		case INTEGER:
			if ( m_nMinValue != -1 && m_fValue > m_nMinValue ) {
				setValue( m_fValue -1);
			}
			break;
		case FLOAT:
			if ( m_nMinValue != -1 && m_fValue > m_nMinValue ) {
				setValue( m_fValue - 1.0);
			}
			break;
	}
	emit changed(this);
}


void LCDSpinBox::setValue( float nValue )
{
	switch ( m_type ) {
		case INTEGER:
			if ( nValue != m_fValue ) {
				m_fValue = (int)nValue;
				m_pDisplay->setText( QString( "%1" ).arg( m_fValue ) );
			}
			break;

		case FLOAT:
			if ( nValue != m_fValue ) {
				m_fValue = nValue;
				QString floatString;
 				floatString.setNum( m_fValue , 'f' , 6 );
				m_pDisplay->setText( floatString );
			}
			break;
	}
}

void LCDSpinBox::displayClicked( LCDDisplay *pRef )
{
	UNUSED( pRef );
	emit spinboxClicked();
}


void LCDSpinBox::wheelEvent ( QWheelEvent *ev )
{
	ev->accept();

	if ( ev->delta() > 0 ) {
		switch( m_type ) {
			case INTEGER:
				setValue( m_fValue + 1);
				break;
			case FLOAT:
				setValue( m_fValue + 1.0);
				break;
		}

		emit changed(this);
	}
	else {
		switch( m_type ) {
			case INTEGER:
				setValue( m_fValue -1);
				break;
			case FLOAT:
				setValue( m_fValue - 1.0);
				break;
		}
		emit changed(this);
	}
}
