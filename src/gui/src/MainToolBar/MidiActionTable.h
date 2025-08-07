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

#ifndef MIDI_ACTION_TABLE_H
#define MIDI_ACTION_TABLE_H

#include <memory>

#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

class MidiAction;

/** \ingroup docGUI docWidgets docMIDI*/
class MidiActionTable : public QTableWidget,
						public H2Core::Object<MidiActionTable>
{
    H2_OBJECT(MidiActionTable)
	Q_OBJECT
	public:
		explicit MidiActionTable( QWidget* pParent );
		~MidiActionTable();

		void setupMidiActionTable();
		void saveMidiActionTable();

signals:
	/** Identicates a user action changing the content of the table.*/
	void changed();

private slots:
	void updateTable();
	void midiSensePressed( int );
	void sendChanged();
	
private:
	void insertNewRow( std::shared_ptr<MidiAction> pAction,
					   const QString& eventString, int eventParameter );
	void updateRow( int nRow );
	virtual void paintEvent( QPaintEvent* ev ) override;
	
		int m_nRowCount;
		int m_nCurrentMidiAutosenseRow;
		QTimer* m_pUpdateTimer;
	int m_nRowHeight;
	int m_nColumn0Width;
	int m_nMinComboWidth;
	int m_nMaxComboWidth;
	int m_nDefaultComboWidth;
	int m_nSpinBoxWidth;

};

#endif
