/*
 * Hydrogen
 * Copyright(c) 2023-2025 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
 *
 * Copyright (C) 1999-2011 by Werner Schweer and others
 * Author: Mathias Lundgren <lunar_shuttle@users.sourceforge.net>, (C) 2003
 *
 * Copyright: Mathias Lundgren (lunar_shuttle@users.sourceforge.net) (C) 2003
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

#include "ShortcutCaptureDialog.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"

ShortcutCaptureDialog::ShortcutCaptureDialog( QWidget* pParent )
	: QDialog( pParent ),
	  m_nKey( -1 ) {

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	setWindowTitle( pCommonStrings->getPreferencesShortcutCapture() );

	QHBoxLayout* pHBoxLayout = new QHBoxLayout( this );
	setLayout( pHBoxLayout );

	/*: Text displayed in the shortcut capture dialog*/
	m_pLabel = new QLabel( tr( "Waiting for keyboard input" ), this );
	m_pLabel->setAlignment( Qt::AlignCenter );
	pHBoxLayout->addWidget( m_pLabel );
}

ShortcutCaptureDialog::~ShortcutCaptureDialog() {
}

void ShortcutCaptureDialog::keyPressEvent( QKeyEvent* ev ) {
	auto modifiers = ev->modifiers();
	bool bShiftPressed = modifiers & Qt::ShiftModifier;
	bool bCtrlPressed  = modifiers & Qt::ControlModifier;
	bool bAltPressed   = modifiers & Qt::AltModifier;
	bool bMetaPressed  = modifiers & Qt::MetaModifier;

	int nTempKey = ev->key();
	nTempKey += bShiftPressed ? static_cast<int>(Qt::SHIFT) : 0;
	nTempKey += bCtrlPressed  ? static_cast<int>(Qt::CTRL)  : 0;
	nTempKey += bAltPressed   ? static_cast<int>(Qt::ALT)   : 0;
	nTempKey += bMetaPressed  ? static_cast<int>(Qt::META)  : 0;

	// Check if this is a "real" key that completes a valid shortcut
	int nLastKey = ev->key();
	if ( nLastKey < 256               ||
		 nLastKey == Qt::Key_Enter    ||
		 nLastKey == Qt::Key_Return   ||
		 ( nLastKey >= Qt::Key_F1 &&
		   nLastKey <= Qt::Key_F12 )  ||
		 nLastKey == Qt::Key_Home     ||
		 nLastKey == Qt::Key_PageUp   ||
		 nLastKey == Qt::Key_PageDown ||
		 nLastKey == Qt::Key_End      ||
		 nLastKey == Qt::Key_Insert   ||
		 nLastKey == Qt::Key_Delete   ||
		 nLastKey == Qt::Key_Up       ||
		 nLastKey == Qt::Key_Down     ||
		 nLastKey == Qt::Key_Left     ||
		 nLastKey == Qt::Key_Right ) {
		
		m_nKey = nTempKey;
		apply();
	}
	else if ( nLastKey == Qt::Key_Escape ) {
		reject();
	}
		
}

void ShortcutCaptureDialog::apply() {
	done( m_nKey );
}
