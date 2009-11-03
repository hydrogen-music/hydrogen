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

#include "config.h"
#include <QtGui>

#include <hydrogen/Object.h>
#include <hydrogen/instrument.h>
#include "../EventListener.h"


class LayerPreview : public QWidget, public Object, public EventListener
{
	Q_OBJECT

	public:
		LayerPreview(QWidget* pParent);
		~LayerPreview();

		void updateAll();

		void paintEvent(QPaintEvent *ev);
		virtual void mousePressEvent(QMouseEvent *ev);
		virtual void mouseReleaseEvent(QMouseEvent *ev);
		virtual void mouseMoveEvent ( QMouseEvent *ev );

	private:
		static const int m_nLayerHeight = 10;
		QPixmap m_speakerPixmap;
		H2Core::Instrument *m_pInstrument;
		int m_nSelectedLayer;
		bool m_bMouseGrab;
		bool m_bGrabLeft;

		virtual void selectedInstrumentChangedEvent();
};


#endif
