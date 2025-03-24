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

#ifndef COMPONENTS_EDITOR_H
#define COMPONENTS_EDITOR_H

#include <QtGui>
#include <QtWidgets>

#include <vector>

#include <core/Object.h>

#include "../Widgets/WidgetWithScalableFont.h"
#include "../WidgetScrollArea.h"

class ComponentView;
class InstrumentEditorPanel;

/** Editor to add, delete, and manipulate all components contained in an
 * #H2Core::Instrument.
 *
 * \ingroup docGUI*/
class ComponentsEditor :  public QWidget,
						  protected WidgetWithScalableFont<10, 12, 14>,
						  public H2Core::Object<ComponentsEditor>
{
	H2_OBJECT(ComponentsEditor)
	Q_OBJECT

	public:
		explicit ComponentsEditor( InstrumentEditorPanel* pPanel );
		~ComponentsEditor();

		void updateComponents();
		void updateEditor();

		ComponentView* getCurrentView() const;

		void deleteComponent( int nComponentIdx);
		void renameComponent( int nComponentIdx, const QString& sNewName );

	public slots:
		void addComponent();

	private:

		QWidget* m_pComponentsWidget;
		QVBoxLayout* m_pComponentsLayout;

		WidgetScrollArea* m_pScrollArea;

		std::vector<ComponentView*> m_componentViews;

		InstrumentEditorPanel* m_pInstrumentEditorPanel;

};

#endif
