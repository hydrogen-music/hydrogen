/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef LAYER_PREVIEW_H
#define LAYER_PREVIEW_H

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <core/Object.h>
#include <core/Basics/Instrument.h>
#include "../EventListener.h"

namespace H2Core
{
class InstrumentLayer;
}

using H2Core::InstrumentLayer;


class LayerPreview : public QWidget, public H2Core::Object, public EventListener
{
    H2_OBJECT
	Q_OBJECT

	public:
		LayerPreview(QWidget* pParent);
		~LayerPreview();

		void updateAll();

		void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent ( QMouseEvent *ev );

		void set_selected_component( int SelectedComponent );

	private:
		static const int		m_nLayerHeight = 10;
		QPixmap					m_speakerPixmap;
		H2Core::Instrument *	m_pInstrument;
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
		void showLayerStartVelocity( const InstrumentLayer* pLayer, const QMouseEvent* pEvent );

		/**
		 * display a layer's end velocity in a tooltip
		 *
		 * @param pLayer    The layer
		 * @param pEvent    The event carrying mouse position
		 */
		void showLayerEndVelocity( const InstrumentLayer* pLayer, const QMouseEvent* pEvent );

		virtual void selectedInstrumentChangedEvent();
};


#endif
