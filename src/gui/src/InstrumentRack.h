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


#ifndef INSTRUMENT_RACK_H
#define INSTRUMENT_RACK_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtGui>
#include <QtWidgets>

#include "InstrumentEditor/ComponentView.h"
#include "Skin.h"
#include "Widgets/WidgetWithScalableFont.h"

class ComponentsEditor;
class InstrumentEditor;
class SoundLibraryPanel;

/** \ingroup docGUI*/
class InstrumentRack : public QTabWidget,
					   protected WidgetWithScalableFont<5, 6, 7>,
					   private H2Core::Object<InstrumentRack>
{
    H2_OBJECT(InstrumentRack)
	Q_OBJECT

public:
		/** 450 - InstrumentEditor layer view +
		 * 24 - tab button */
		static constexpr int m_nMinimumHeight = 450;
		static constexpr int nWidth = ComponentView::nWidth +
			Skin::nScrollBarWidth;

		explicit InstrumentRack( QWidget *pParent );
		~InstrumentRack();

		ComponentsEditor* getComponentsEditor() const;
		InstrumentEditor* getInstrumentEditor() const;
		SoundLibraryPanel* getSoundLibraryPanel() const;

public slots:
	/** Used by the #Shotlist*/
	void showInstrument();
	void showComponents();
	void showSoundLibrary();
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

   private:
	void updateStyleSheet();
	void updateIcons();

	ComponentsEditor* m_pComponentsEditor;
	InstrumentEditor* m_pInstrumentEditor;

	SoundLibraryPanel* m_pSoundLibraryPanel;
};

inline ComponentsEditor* InstrumentRack::getComponentsEditor() const {
	return m_pComponentsEditor;
}
inline InstrumentEditor* InstrumentRack::getInstrumentEditor() const {
	return m_pInstrumentEditor;
}
inline SoundLibraryPanel* InstrumentRack::getSoundLibraryPanel() const {
	return m_pSoundLibraryPanel;
}

#endif
