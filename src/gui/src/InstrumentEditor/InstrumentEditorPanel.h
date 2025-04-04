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
#include "../Widgets/PixmapWidget.h"

namespace H2Core {
	class Instrument;
}

class Button;
class ComponentsEditor;
class InstrumentEditor;

///
/// Container for the Instrument Editor (Singleton).
///
/** \ingroup docGUI*/
class InstrumentEditorPanel : public PixmapWidget,
							  private H2Core::Object<InstrumentEditorPanel>,
							  public EventListener
{
    H2_OBJECT(InstrumentEditorPanel)
	Q_OBJECT
	public:
		/** Range of the fine pitch rotary for both the overall instrument pitch
		 * as well as for the layer pitch. */
		static constexpr float nPitchFineControl = 0.5;

		explicit InstrumentEditorPanel( QWidget *pParent );
		explicit InstrumentEditorPanel(const InstrumentEditorPanel&) = delete;
		~InstrumentEditorPanel();
		InstrumentEditorPanel& operator=( const InstrumentEditorPanel& rhs ) = delete;

		ComponentsEditor* getComponentsEditor() const;
		InstrumentEditor* getInstrumentEditor() const;

		void updateEditors();

		// implements EventListener interface
		virtual void drumkitLoadedEvent() override;
		virtual void instrumentParametersChangedEvent( int ) override;
		virtual void selectedInstrumentChangedEvent() override;
		virtual void updateSongEvent( int ) override;
		// ~ implements EventListener interface

		std::shared_ptr<H2Core::Instrument> getInstrument() const;

	public slots:
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	private:
		void updateInstrument();
		ComponentsEditor*				m_pComponentsEditor;
		InstrumentEditor*				m_pInstrumentEditor;

		PixmapWidget* m_pBackground;
		Button* m_pShowInstrumentBtn;
		Button* m_pShowComponentsBtn;

		std::shared_ptr<H2Core::Instrument> m_pInstrument;
};

inline ComponentsEditor* InstrumentEditorPanel::getComponentsEditor() const {
	return m_pComponentsEditor;
}
inline InstrumentEditor* InstrumentEditorPanel::getInstrumentEditor() const {
	return m_pInstrumentEditor;
}

inline std::shared_ptr<H2Core::Instrument> InstrumentEditorPanel::getInstrument() const {
	return m_pInstrument;

}

#endif

