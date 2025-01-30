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

#ifndef INSTRUMENT_EDITOR_PANEL_H
#define INSTRUMENT_EDITOR_PANEL_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include "InstrumentEditor.h"
#include "../EventListener.h"

///
/// Container for the Instrument Editor (Singleton).
///
/** \ingroup docGUI*/
class InstrumentEditorPanel : public QWidget, private H2Core::Object<InstrumentEditorPanel>, public EventListener
{
    H2_OBJECT(InstrumentEditorPanel)
	Q_OBJECT
	public:
		static InstrumentEditorPanel* get_instance();
		~InstrumentEditorPanel();
	
		explicit InstrumentEditorPanel(const InstrumentEditorPanel&) = delete;
		InstrumentEditorPanel& operator=( const InstrumentEditorPanel& rhs ) = delete;

		InstrumentEditor* getInstrumentEditor() const;

		void selectLayer( int nLayer );
		
		int getSelectedLayer() {
			return m_nLayer;
		}

		void updateWaveDisplay();

	private:
		static InstrumentEditorPanel*	m_pInstance;
		InstrumentEditor*				m_pInstrumentEditor;
		int								m_nLayer;

		explicit InstrumentEditorPanel( QWidget *pParent );

};

inline 	InstrumentEditor* InstrumentEditorPanel::getInstrumentEditor() const {
	return m_pInstrumentEditor;
}
#endif

