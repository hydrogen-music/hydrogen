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

#include "LCDCombo.h"

#include "../Skin.h"
#include "LCD.h"
#include "Button.h"

#include <hydrogen/globals.h>

const char* LCDCombo::__class_name = "LCDCombo";

LCDCombo::LCDCombo( QWidget *pParent, int digits )
	: QWidget(pParent)
	, Object( __class_name )
{
	INFOLOG( "INIT" );

	display = new LCDDisplay( this, LCDDigit::SMALL_BLUE, digits, false );
	button = new Button( this,
	                     "/patternEditor/btn_dropdown_on.png",
	                     "/patternEditor/btn_dropdown_off.png",
	                     "/patternEditor/btn_dropdown_over.png",
	                     QSize(13, 13)
	                   );
	pop = new QMenu( this );
	size = digits;
	active = -1;

	button->move( ( digits * 8 ) + 5, 1 );
	setFixedSize( ( digits * 8 ) + 17, display->height() );

	connect( button, SIGNAL( clicked( Button* ) ), this, SLOT( onClick( Button* ) ) );
	connect( pop, SIGNAL( triggered(QAction*) ), this, SLOT( changeText(QAction*) ) );
}

LCDCombo::~LCDCombo()
{
}

void LCDCombo::changeText( QAction* pAction )
{
	select( actions.indexOf(pAction) );
}

void LCDCombo::onClick( Button* )
{
	pop->popup( display->mapToGlobal( QPoint( 1, display->height() + 2 ) ) );
}

bool LCDCombo::addItem( const QString &text )
{
	//INFOLOG( "add item" );
	if ( text.size() <= size ) {
		actions.append( pop->addAction( text ) );
		return true;
	} else {
		WARNINGLOG(QString( "'%1' is > %2").arg( text ).arg( size ) );
		return false;
	}
}

void LCDCombo::addSeparator()
{
	actions.append( pop->addSeparator() );
}

void LCDCombo::mousePressEvent( QMouseEvent *ev )
{
	UNUSED( ev );
	pop->popup( display->mapToGlobal( QPoint( 1, display->height() + 2 ) ) );
}

void LCDCombo::wheelEvent( QWheelEvent * ev )
{
	ev->ignore();
	const int n = actions.size();
	const int d = ( ev->delta() > 0 ) ? -1: 1;
	int next = ( n + active + d ) % n;
	if ( actions.at( next )->isSeparator() ) {
		next = ( n + next + d ) % n;
	}
	
	select( next );
}

int LCDCombo::selected()
{
	return active;
}

bool LCDCombo::select( int idx )
{
	return select(idx, true);
}

bool LCDCombo::select( int idx, bool emitValueChanged )
{
	if (active == idx) {
		return false;
	}

	if (idx < 0 || idx >= actions.size()) {
		WARNINGLOG(QString("out of index %1 >= %2").arg(idx).arg(actions.size()));
		return false;
	}

	active = idx;
	display->setText( actions.at( idx )->text() );
	if ( emitValueChanged ) {
		emit valueChanged( idx );
	}
	
	return true;
}
