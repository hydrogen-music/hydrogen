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

#ifndef MIDI_CONTROL_DIALOG_H
#define MIDI_CONTROL_DIALOG_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>

#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

/** \ingroup docGUI*/
class MidiControlDialog : public QDialog,
						  public EventListener,
						  protected WidgetWithScalableFont<5, 6, 7>,
						  public H2Core::Object<MidiControlDialog> {
		H2_OBJECT(MidiControlDialog)
		Q_OBJECT

public:
		static constexpr int nWidth = 800;
		static constexpr int nHeight = 740;
		static constexpr int nBinButtonHeight = 30;
		static constexpr int nBinButtonMargin = 7;

		explicit MidiControlDialog( QWidget* pParent );
		~MidiControlDialog();

		// EventListerer
		void midiDriverChangedEvent() override;
		void midiInputEvent() override;
		void midiOutputEvent() override;

public slots:
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

private:
		static QString timestampToQString( QTime );

		void hideEvent( QHideEvent* pEvent ) override;
		void showEvent( QShowEvent* pEvent ) override;

		void updateFont();
		void updateIcons();
		void updateInputTable();
		void updateOutputTable();

		QTabWidget* m_pTabWidget;

		QTableWidget* m_pMidiInputTable;
		QToolButton* m_pInputBinButton;
		QTableWidget* m_pMidiOutputTable;
		QToolButton* m_pOutputBinButton;
};


#endif
