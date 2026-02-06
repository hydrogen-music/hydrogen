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
#include <core/Midi/MidiEvent.h>
#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

#include "Widgets/LCDSpinBox.h"

class Button;

class SpinBoxWithIcon : public QWidget {
   public:
	SpinBoxWithIcon( QWidget* pParent );
	~SpinBoxWithIcon();

	Button* m_pButton;
	LCDSpinBox* m_pSpinBox;
};

/** \ingroup docGUI docWidgets docMIDI*/
class MidiActionTable : public QTableWidget,
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

	/** Should only used in very rare occassions when the underlying data is
	 * change. E.g. when loading a different Preferences file during
	 * runtime. */
	void resetTable();

	/** Redo/undo interface.
	 *
	 * @param nRow a value of -1 makes the function append the new row.
	 * @{ */
	void insertRow(
		int nRow,
		H2Core::MidiEvent::Type eventType,
		H2Core::Midi::Parameter eventParameter,
		std::shared_ptr<MidiAction> pMidiAction
	);
	void removeRow( int nRow );
	void replaceRow(
		int nRow,
		H2Core::MidiEvent::Type eventType,
		H2Core::Midi::Parameter eventParameter,
		std::shared_ptr<MidiAction> pMidiAction
	);
	/** @} */

	void removeRegisteredEvents( std::shared_ptr<MidiAction> pMidiAction );

   private slots:
	void midiSensePressed( int );

   private:
	/** Reflects the options the user can set within one row of the table. */
	struct RowContent {
		H2Core::MidiEvent::Type eventType;
		H2Core::Midi::Parameter eventParameter;
		std::shared_ptr<MidiAction> pMidiAction;
	};

	void appendEmptyRow();

	void updateRowContent(
		int nRow,
		H2Core::MidiEvent::Type eventType,
		H2Core::Midi::Parameter eventParameter,
		std::shared_ptr<MidiAction> pMidiAction
	);
	/** We can not use updateRowContent() in paintEvent() because the former
	 * does trigger a redraw itself. */
	void updateRowVisibility( int nRow );

	/** Writes back changes in the visual representation of the row (current
	 * values of all widgets) into #m_tableRows via redo/undo routines. */
	void writeBackRow( int nRow );

	int findRowOf( QWidget* pWidget ) const;

	virtual void paintEvent( QPaintEvent* ev ) override;

	/** Instead of using #H2Core::MidiEventMap as single source of truth, we
	 * duplicates its content and using this member as a middleware between the
	 * data written to/loader from disk and the content presented in the table.
	 *
	 * This way we combine two advantages (while suffering from more
	 * complexity):
	 *
	 * - We do not have to worry about "nullptr" being stored and loaded from
	 *   disk as well as ensuring backward compatibility.
	 * - We do not have to deal with glitches and reordering in the user-facing
	 *   table. E.g. when having multiple "empty" lines not written to
	 *   #H2Core::MidiEventMap dealing with a delete operation for a row would
	 *   be a non-trivial task.
	 *
	 * Each element of the vector represents a row of the table. */
	std::vector<RowContent> m_tableRows;

	QStringList m_availableActions;
};

#endif
