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
 * along with this program. If not, see 
https://www.gnu.org/licenses
 *
 */


#include "MidiLearnableToolButton.h"
#include "MidiSenseWidget.h"

MidiLearnableToolButton::MidiLearnableToolButton( QWidget* pParent ) :
	QToolButton( pParent ) {
}

MidiLearnableToolButton::~MidiLearnableToolButton() {
}

void MidiLearnableToolButton::mousePressEvent(QMouseEvent*ev) {
	/*
	*  Shift + Left-Click activate the midi learn widget
	*/
	if ( ev->button() == Qt::LeftButton && ( ev->modifiers() & Qt::ShiftModifier ) ){
		MidiSenseWidget midiSense( this, true, this->getAction() );
		midiSense.exec();
		return;
	}

	QToolButton::mousePressEvent( ev );
}
