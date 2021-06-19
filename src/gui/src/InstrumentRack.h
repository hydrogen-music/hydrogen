/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#ifndef INSTRUMENT_RACK_H
#define INSTRUMENT_RACK_H

#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

class ToggleButton;
class SoundLibraryPanel;

class InstrumentRack : public QWidget, private H2Core::Object
{
    H2_OBJECT
	Q_OBJECT
	public:
		explicit InstrumentRack( QWidget *pParent );
		~InstrumentRack();

		SoundLibraryPanel* getSoundLibraryPanel() {	return m_pSoundLibraryPanel;	}

	public slots:
		void on_showSoundLibraryBtnClicked();
		void on_showInstrumentEditorBtnClicked();


	private:
		/// button for showing the Sound Library
		ToggleButton *m_pShowSoundLibraryBtn;

		/// button for showing the Instrument Editor
		ToggleButton *m_pShowInstrumentEditorBtn;

		SoundLibraryPanel* m_pSoundLibraryPanel;

};

#endif
