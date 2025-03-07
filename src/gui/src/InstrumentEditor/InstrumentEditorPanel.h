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
#include "../EventListener.h"

namespace H2Core {
	class Instrument;
}
class InstrumentEditor;

///
/// Container for the Instrument Editor (Singleton).
///
/** \ingroup docGUI*/
class InstrumentEditorPanel : public QWidget,
							  private H2Core::Object<InstrumentEditorPanel>,
							  public EventListener
{
    H2_OBJECT(InstrumentEditorPanel)
	Q_OBJECT
	public:
		explicit InstrumentEditorPanel( QWidget *pParent );
		explicit InstrumentEditorPanel(const InstrumentEditorPanel&) = delete;
		~InstrumentEditorPanel();
		InstrumentEditorPanel& operator=( const InstrumentEditorPanel& rhs ) = delete;

		InstrumentEditor* getInstrumentEditor() const;

		void updateEditors();

		// implements EventListener interface
		virtual void drumkitLoadedEvent() override;
		virtual void instrumentParametersChangedEvent( int ) override;
		virtual void selectedInstrumentChangedEvent() override;
		virtual void updateSongEvent( int ) override;
		// ~ implements EventListener interface

		std::shared_ptr<H2Core::Instrument> getInstrument() const;
		int getSelectedComponent() const;
		int getSelectedLayer() const;

		void setSelectedComponent( int nComponent );
		void setSelectedLayer( int nLayer );

	public slots:
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	private:
		InstrumentEditor*				m_pInstrumentEditor;

		std::shared_ptr<H2Core::Instrument> m_pInstrument;
		int m_nSelectedLayer;
		int m_nSelectedComponent;
};

inline InstrumentEditor* InstrumentEditorPanel::getInstrumentEditor() const {
	return m_pInstrumentEditor;
}

inline std::shared_ptr<H2Core::Instrument> InstrumentEditorPanel::getInstrument() const {
	return m_pInstrument;

}
inline int InstrumentEditorPanel::getSelectedComponent() const {
	return m_nSelectedComponent;
}
inline int InstrumentEditorPanel::getSelectedLayer() const {
	return m_nSelectedLayer;
}

#endif

