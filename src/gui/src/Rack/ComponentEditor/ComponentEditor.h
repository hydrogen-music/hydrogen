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

#include "../../EventListener.h"
#include "../../Widgets/WidgetWithScalableFont.h"
#include "../../WidgetScrollArea.h"

class ComponentView;

/** Editor to add, delete, and manipulate all components contained in an
 * #H2Core::Instrument.
 *
 * \ingroup docGUI*/
class ComponentEditor : public QWidget,
						 protected WidgetWithScalableFont<10, 12, 14>,
						 public H2Core::Object<ComponentEditor>,
						 public EventListener {
	H2_OBJECT( ComponentEditor )
	Q_OBJECT

   public:
	explicit ComponentEditor( QWidget* pParent );
	~ComponentEditor();

	void updateColors();
	void updateComponents();
	void updateIcons();
	void updateEditor();
	void updateStyleSheet();

	// implements EventListener interface
	void drumkitLoadedEvent() override;
	void instrumentLayerChangedEvent( int nId ) override;
	void instrumentParametersChangedEvent( int ) override;
	void selectedInstrumentChangedEvent() override;
	void updateSongEvent( int ) override;
	// ~ implements EventListener interface

	ComponentView* getCurrentView() const;

	void setVisible( bool bVisible ) override;

   public slots:
	void addComponent();
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

   private:
	virtual void mousePressEvent( QMouseEvent* event ) override;

	void updateSize();

	QWidget* m_pComponentsWidget;
	QVBoxLayout* m_pComponentsLayout;
	QMenu* m_pPopup;

	WidgetScrollArea* m_pScrollArea;

	std::vector<ComponentView*> m_componentViews;
};

#endif
