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
#ifndef LAYER_PREVIEW_H
#define LAYER_PREVIEW_H

#include <QtGui>
#include <QtWidgets>
#include <memory>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Instrument.h>
#include "../EventListener.h"
#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
class InstrumentLayer;
}

/** \ingroup docGUI*/
class LayerPreview :  public QWidget, protected WidgetWithScalableFont<5, 6, 7>,  public H2Core::Object<LayerPreview>, public EventListener
{
    H2_OBJECT(LayerPreview)
	Q_OBJECT

	public:
		explicit LayerPreview(QWidget* pParent);
		~LayerPreview();

		void updateAll();

		void paintEvent(QPaintEvent *ev) override;
		virtual void mousePressEvent(QMouseEvent *ev) override;
		virtual void mouseReleaseEvent(QMouseEvent *ev) override;
		virtual void mouseMoveEvent ( QMouseEvent *ev ) override;

		void set_selected_component( int SelectedComponent );
	void setSelectedLayer( int nSelectedLayer );

public slots:
		void onPreferencesChanged( H2Core::Preferences::Changes changes );
	
	private:
		static const int		m_nLayerHeight = 10;
		QPixmap					m_speakerPixmap;
		std::shared_ptr<H2Core::Instrument>	m_pInstrument;
		int						m_nSelectedLayer;
		int						m_nSelectedComponent;
		bool					m_bMouseGrab;
		bool					m_bGrabLeft;

		/**
		 * convert a raw velocity value (0.0 to 1.0)
		 * into a MIDI velocity value   (0 to 127)
		 *
		 * @param raw   Raw velocity value
		 * @return      MIDI velocity value
		 */
		int getMidiVelocityFromRaw( const float raw );

		/**
		 * display a layer's start velocity in a tooltip
		 *
		 * @param pLayer    The layer
		 * @param pEvent    The event carrying mouse position
		 */
		void showLayerStartVelocity( const std::shared_ptr<H2Core::InstrumentLayer> pLayer,
									 QMouseEvent* pEvent );

		/**
		 * display a layer's end velocity in a tooltip
		 *
		 * @param pLayer    The layer
		 * @param pEvent    The event carrying mouse position
		 */
		void showLayerEndVelocity( const std::shared_ptr<H2Core::InstrumentLayer> pLayer,
								   QMouseEvent* pEvent );

		virtual void selectedInstrumentChangedEvent() override;
	virtual void drumkitLoadedEvent() override;
	virtual void updateSongEvent(int) override;
		/** Used to detect changed in the font*/
		int getPointSizeButton() const;
};

inline void LayerPreview::setSelectedLayer( int nLayer ) {
	if ( nLayer != m_nSelectedLayer ) {
		m_nSelectedLayer = nLayer;
	}
}

#endif
