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

#include <core/Midi/Midi.h>
#include <core/Midi/MidiAction.h>
#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

#include "../EventListener.h"
#include "Widgets/LCDSpinBox.h"

class Button;

namespace H2Core {
class MidiEvent;
}

class SpinBoxWithIcon : public QWidget {
   public:
	SpinBoxWithIcon( QWidget* pParent );
	~SpinBoxWithIcon();

	Button* m_pButton;
	LCDSpinBox* m_pSpinBox;
};

/** \ingroup docGUI docWidgets docMIDI*/
class MidiActionTable : public QTableWidget,
						public EventListener,
						public H2Core::Object<MidiActionTable> {
	H2_OBJECT( MidiActionTable )
	Q_OBJECT

   public:
	static constexpr int nRowHeight = 29;
	static constexpr int nColumnButtonWidth = 25;
	static constexpr int nMinComboWidth = 100;
	static constexpr int nMaxComboWidth = 1460;
	static constexpr int nDefaultComboWidth = 146;
	static constexpr int nSpinBoxWidth = 75;

	explicit MidiActionTable( QWidget* pParent );
	~MidiActionTable();

	/** Recreates the table based on the #H2Core::MidiEventMap stored within
	 * #H2Core::Preferences. Local changes will be discarded. */
	void updateTable();

	void appendNewRow();

	// EventListerer
	void midiMapChangedEvent() override;

   private slots:
	void midiSensePressed( int );

   private:
	/** Update the row based on its widgets. This focusses on the combo box
	 * holding the types for both #H2Core::MidiAction and #H2Core::MidiEvent.
	 * Different types require different parameter spin boxes. */
	void updateRow( int nRow );

	/** Update the row based on external data provided via #a pEvent. */
	void updateRow( std::shared_ptr<H2Core::MidiEvent> pEvent, int nRow );

	/** Replaces the #H2Core::MidiEvent associated with @n nRow in
	 * #H2Core::MidiEventMap with the values hold by the row's widgets. */
	void saveRow( int nRow );

	void updateActionParameters(
		MidiAction::Type type,
		SpinBoxWithIcon* pSpinBox1,
		SpinBoxWithIcon* pSpinBox2,
		SpinBoxWithIcon* pSpinBox3
	);

	virtual void paintEvent( QPaintEvent* ev ) override;

	int m_nCurrentMidiAutosenseRow;

	/** Maps a row to the corresponding #H2Core::MidiEvent. */
	std::map<int, std::shared_ptr<H2Core::MidiEvent>> m_cachedEventMap;

	QStringList m_availableActions;
};

#endif
