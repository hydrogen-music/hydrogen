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

#ifndef SOUND_LIBRARY_OPEN_DIALOG_H
#define SOUND_LIBRARY_OPEN_DIALOG_H

#include <QtGui>
#if QT_VERSION >= 0x050000
	#include <QtWidgets>
#endif

#include "SoundLibraryDatastructures.h"

class SoundLibraryPanel;

class SoundLibraryOpenDialog : public QDialog, public H2Core::Object
{
	Q_OBJECT
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
		SoundLibraryOpenDialog( QWidget* pParent );
		~SoundLibraryOpenDialog();

	private slots:
		void on_soundLib_item_changed( bool bDrumkitSelected );

		void on_cancel_btn_clicked();
		void on_open_btn_clicked();

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
		SoundLibraryPanel*	m_pSoundLibraryPanel;
		QPushButton*		m_pOkBtn;
		QPushButton*		m_pCancelBtn;
};

#endif
