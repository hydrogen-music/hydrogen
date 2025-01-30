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

#ifndef MIDI_TABLE_H
#define MIDI_TABLE_H

#include <memory>

#include <core/Object.h>
#include <core/MidiAction.h>

#include <QtGui>
#include <QtWidgets>

/** \ingroup docGUI docWidgets docMIDI*/
class MidiTable :  public QTableWidget,  public H2Core::Object<MidiTable>
{
    H2_OBJECT(MidiTable)
	Q_OBJECT
	public:
		explicit MidiTable( QWidget* pParent );
		~MidiTable();

		void setupMidiTable();
		void saveMidiTable();

signals:
	/** Identicates a user action changing the content of the table.*/
	void changed();

private slots:
	void updateTable();
	void midiSensePressed( int );
	void sendChanged();
	
private:
	void insertNewRow( std::shared_ptr<Action> pAction, QString eventString, int eventParameter );
	void updateRow( int nRow );
	virtual void paintEvent( QPaintEvent* ev ) override;
	
		int m_nRowCount;
		int m_nCurrentMidiAutosenseRow;
		QTimer* m_pUpdateTimer;
	int m_nRowHeight;
	int m_nColumn0Width;
	int m_nColumn1Width;
	int m_nColumn2Width;
	int m_nColumn3Width;
	int m_nColumn4Width;
	int m_nColumn5Width;
	int m_nColumn6Width;

};

#endif
