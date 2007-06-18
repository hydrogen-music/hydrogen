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
 * $Id: LCD.cpp,v 1.17 2005/05/23 11:21:01 comix Exp $
 *
 */

#include "LCD.h"

#include "../Skin.h"

QPixmap* LCDDigit::m_pSmallBlueFontSet = NULL;
QPixmap* LCDDigit::m_pSmallRedFontSet = NULL;
QPixmap* LCDDigit::m_pLargeGrayFontSet = NULL;
QPixmap* LCDDigit::m_pSmallGrayFontSet = NULL;

LCDDigit::LCDDigit( QWidget * pParent, LCDType type )
 : QWidget( pParent , "LCDDigit", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "LCDDigit" )
 , m_bChanged( true )
 , m_type( type )
{
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
		string sSmallBlueFontSet = Skin::getImagePath() + string( "/lcd/LCDSmallBlueFontSet.png" );
		m_pSmallBlueFontSet = new QPixmap();
		bool ok = m_pSmallBlueFontSet->load( sSmallBlueFontSet.c_str() );
		if( ok == false ) {
			errorLog( string("Fader: Error loading pixmap") );
		}
	}

	// Small red FontSet image
	if (m_pSmallRedFontSet == NULL ) {
		string sSmallRedFontSet = Skin::getImagePath() + string( "/lcd/LCDSmallRedFontSet.png" );
		m_pSmallRedFontSet = new QPixmap();
		bool ok = m_pSmallRedFontSet->load( sSmallRedFontSet.c_str() );
		if( ok == false ) {
			errorLog("Fader: Error loading pixmap");
		}
	}

	// Large gray FontSet image
	if (m_pLargeGrayFontSet == NULL ) {
		string sLargeGrayFontSet = Skin::getImagePath() + string( "/lcd/LCDLargeGrayFontSet.png" );
		m_pLargeGrayFontSet = new QPixmap();
		bool ok = m_pLargeGrayFontSet->load( sLargeGrayFontSet.c_str() );
		if( ok == false ) {
			errorLog("Fader: Error loading pixmap");
		}
	}

	// Small gray FontSet image
	if (m_pSmallGrayFontSet == NULL ) {
		string sSmallGrayFontSet = Skin::getImagePath() + string( "/lcd/LCDSmallGrayFontSet.png" );
		m_pSmallGrayFontSet = new QPixmap();
		bool ok = m_pSmallGrayFontSet->load( sSmallGrayFontSet.c_str() );
		if( ok == false ) {
			errorLog("Fader: Error loading pixmap");
		}
	}

	m_temp.resize( width(), height() );

	set( ' ' );
}


LCDDigit::~LCDDigit()
{
	delete m_pSmallBlueFontSet;
	m_pSmallBlueFontSet = NULL;

	delete m_pSmallRedFontSet;
	m_pSmallRedFontSet = NULL;
}


//void LCDDigit::mousePressEvent(QMouseEvent *ev)
void LCDDigit::mouseReleaseEvent(QMouseEvent* ev)
{
	emit digitClicked();
}


void LCDDigit::paintEvent(QPaintEvent *ev)
{
	if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;

		int x = m_nCol * width();
		int y = m_nRow * height();

		switch ( m_type ) {
			case SMALL_BLUE:
				bitBlt( &m_temp, 0, 0, m_pSmallBlueFontSet, x, y, width(), height(), CopyROP );
				break;

			case SMALL_RED:
				bitBlt( &m_temp, 0, 0, m_pSmallRedFontSet, x, y, width(), height(), CopyROP );
				break;

			case LARGE_GRAY:
				bitBlt( &m_temp, 0, 0, m_pLargeGrayFontSet, x, y, width(), height(), CopyROP );
				break;

			case SMALL_GRAY:
				bitBlt( &m_temp, 0, 0, m_pSmallGrayFontSet, x, y, width(), height(), CopyROP );
				break;

			default:
				errorLog( "[paint] Unhandled type" );
		}
	}
	bitBlt( this, 0, 0, &m_temp, 0, 0, width(), height(), CopyROP, true );
}


void LCDDigit::set( char ch )
{
	int MAXCOL = 66;
	const char keymap[] = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ',
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', ' ',
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ' ', ' ',
			'-', ':', '/', '\\', ',', ';', '.'
	};
	for ( int n = 0; n < 73; n++ ) {
		if ( keymap[ n ] == ch ) {
			m_nCol = n % MAXCOL;
			m_nRow = n / MAXCOL;
			break;
		}
	}

	m_bChanged = true;
	update();
}


void LCDDigit::setSmallRed()
{
	m_type = SMALL_RED;
	m_bChanged = true;
	update();
}


void LCDDigit::setSmallBlue()
{
	m_type = SMALL_BLUE;
	m_bChanged = true;
	update();
}



// ::::::::::::::::::




LCDDisplay::LCDDisplay( QWidget * pParent, LCDDigit::LCDType type, int nDigits )
 : QWidget( pParent, "LCDDisplay" )
 , m_sMsg( "" )
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
	
	// default background color..
	setPaletteBackgroundColor( QColor( 49, 53, 61 ) );
}




LCDDisplay::~LCDDisplay() {
	for ( uint i = 0; i < m_pDisplay.size(); i++ ) {
		delete m_pDisplay[ i ];
	}
}


void LCDDisplay::setText( string sMsg )
{
	m_sMsg = sMsg;
	// right aligned

	int nPadding = 0;
	int nLen = sMsg.length();
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
		m_pDisplay[ i + nPadding ]->set( sMsg[ i ] );
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
 : QWidget( pParent, "LCDSpinBox" )
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

//	string sUpBtn_on = Skin::getImagePath() + string( "/lcd/LCDSpinBox_up_on.png" );
//	string sUpBtn_off = Skin::getImagePath() + string( "/lcd/LCDSpinBox_up_off.png" );
//	string sUpBtn_over = Skin::getImagePath() + string( "/lcd/LCDSpinBox_up_over.png" );
//	string sUpBtn_off = sUpBtn_on;
//	string sUpBtn_over = sUpBtn_on;
//	string sDownBtn_on = Skin::getImagePath() + string( "/lcd/LCDSpinBox_down_on.png" );
//	string sDownBtn_off = Skin::getImagePath() + string( "/lcd/LCDSpinBox_down_off.png" );
//	string sDownBtn_over = Skin::getImagePath() + string( "/lcd/LCDSpinBox_down_over.png" );
//	string sDownBtn_on = sUpBtn_on;
//	string sDownBtn_off = sUpBtn_on;
//	string sDownBtn_over = sUpBtn_on;
//	m_pUpBtn = new Button( this, QSize( 10, 10 ), sUpBtn_on, sUpBtn_off, sUpBtn_over );
//	m_pUpBtn->move( m_pDisplay->width(), 0 );
//	connect( m_pUpBtn, SIGNAL( clicked(Button*) ), this, SLOT( upBtnClicked(Button*) ) );
//	m_pDownBtn = new Button( this, QSize( 10, 10 ), sDownBtn_on, sDownBtn_off, sDownBtn_over );
//	m_pDownBtn->move( m_pDisplay->width(),  10);
//	connect( m_pDownBtn, SIGNAL( clicked(Button*) ), this, SLOT( downBtnClicked(Button*) ) );

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
				m_pDisplay->setText( toString( m_fValue ) );
			}
			break;

		case FLOAT:
			if ( nValue != m_fValue ) {
				m_fValue = nValue;
				char tmp[20];
				sprintf( tmp, "%#.2f", m_fValue );
				m_pDisplay->setText( toString( tmp ) );
			}
			break;
	}
}

void LCDSpinBox::displayClicked( LCDDisplay *pRef )
{
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

