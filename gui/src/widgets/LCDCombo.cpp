/*
 * Hydrogen
 * Copyright(c) 2002-2006 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <string>
#include <iostream>

#include "../Skin.h"
#include "LCD.h"
#include "Button.h"

#include <QWidget>
#include <QMenu>
#include <QPixmap>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <QPainter>

#include <hydrogen/Globals.h>


LCDCombo::LCDCombo(QWidget *pParent, int digits)
 : QWidget(pParent)
 , Object( "LCDCombo")
{
	INFOLOG( "INIT" );
	
	QStringList items;
	display = new LCDDisplay( this, LCDDigit::SMALL_BLUE, digits, false);
	button = new Button( this,
			"/patternEditor/btn_dropdown_on.png",
			"/patternEditor/btn_dropdown_off.png",
			"/patternEditor/btn_dropdown_over.png",
			QSize(13, 13)
	);
	pop = new QMenu( this );
	size = digits;

	button->move( ( digits * 8 ) + 5 , 1 );
	setFixedSize( ( digits * 8 ) + 17, display->height() );

	connect( button, SIGNAL( clicked( Button* ) ), this, SLOT( onClick( Button* ) ) );

	update();
	
	connect( pop, SIGNAL( triggered(QAction*) ), this, SLOT( changeText(QAction*) ) );
	//_WARNINGLOG("items:"+items[0]);
}



LCDCombo::~LCDCombo()
{
}



void LCDCombo::changeText(QAction* pAction)
{
	//_WARNINGLOG("triggered");
	display->setText(pAction->text());
	emit valueChanged( pAction->text() );
}



void LCDCombo::onClick(Button*)
{
	pop->popup( display->mapToGlobal( QPoint( 1, display->height() + 2 ) ) );
}



int LCDCombo::length()
{
	return size;
}



void LCDCombo::update()
{
	//INFOLOG ( "update: "+toString(items.size()) );
	pop->clear();

	for( int i = 0; i < items.size(); i++ ) {
		if(items.at(i)!=QString("--sep--")){
			pop->addAction( items.at(i) );
		}else{
			pop->addSeparator();
		}
	}

}



int LCDCombo::count()
{
	return items.size(); 
}



bool LCDCombo::addItem(const QString &text )
{
	//INFOLOG( "add item" );
	
	if ( text.size() <= size ){
		items.append( text );
		return true;
	}else{
		return false;
	}
}



void LCDCombo::addSeparator()
{
	items.append( QString("--sep--") );
}



inline void LCDCombo::insertItem( int index, const QString &text )
{
	if(text.size()<=length()){
		items.insert(index,text);
		update();
	}
}



void LCDCombo::mousePressEvent(QMouseEvent *ev)
{
	UNUSED( ev );
	pop->popup( display->mapToGlobal( QPoint( 1, display->height() + 2 ) ) );
}



void LCDCombo::set_text( const QString &text)
{
	if (display->getText() == text) {
		return;
	}
	INFOLOG(text.toStdString());
	display->setText( text );
	emit valueChanged( text );
}


