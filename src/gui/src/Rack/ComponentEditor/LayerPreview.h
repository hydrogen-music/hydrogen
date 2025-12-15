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
#include <set>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Instrument.h>
#include "../../Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class InstrumentLayer;
}

class ComponentView;

/** \ingroup docGUI*/
class LayerPreview : public QWidget, protected WidgetWithScalableFont<6, 8, 10>,
					 public H2Core::Object<LayerPreview>
{
    H2_OBJECT(LayerPreview)
	Q_OBJECT

   public:
	static constexpr int nBorder = 1;
	static constexpr int nHeader = 20;
	static constexpr int nLayerHeight = 16;
	static constexpr int nBorderGrabMargin = 5;

	explicit LayerPreview( ComponentView* pComponentView );
	~LayerPreview();

	void updatePreview();

   private:
	enum class Drag {
		/** No drag action */
		None,
        /** A drag was initiated but either not sufficient time nor distance was
         * covered to decide which kind of drag to perform. */
        Initialized,
		/** Vertical drag to change the order of layers within a component. */
		Position,
		/** Horizontal dragging the left or right border of a layer to change
		   its end velocity */
		VelocityEnd,
		/** Horizontal dragging the left or right border of a layer to change
		   its start velocity */
		VelocityStart,
	};
	static QString DragToQString( const Drag& drag );

	struct LayerInfo {
		int nStartX;
		int nEndX;
		int nStartY;
		int nId;
		bool bSelected;

		/** Based on this operator we will insert the overall information of
		   each layer into a std::set in order to create the headers later on.
		   We do so from highest to lowest velocity (#nEndX takes precedeence)
		   and do so for each unique layer (#nStartX is taken into account but
		   #nId is not if there are multiple layers bearing the same width and
		   position, they will only be represented by a single header item. An
		   exception are selected layers, so we can ensure the highlight in the
		   header is done). */
		bool operator<( const LayerInfo& other ) const
		{
			if ( nEndX != other.nEndX ) {
				return nEndX < other.nEndX;
			}
			else if ( nStartX != other.nStartX ) {
				return nStartX < other.nStartX;
			}
			else {
				return bSelected;
			}
		}

		QString toQString() const
		{
			return QString(
					   "[LayerInfo] nStartX: %1, nEndX: %2, nStartY: %3, nId: "
					   "%4, "
					   "bSelected: %5"
			)
				.arg( nStartX )
				.arg( nEndX )
				.arg( nStartY )
				.arg( nId )
				.arg( bSelected );
		}
	};

	static int yToLayer( int nY );

	void dragMoveEvent( QDragMoveEvent* event ) override;
	void dragEnterEvent( QDragEnterEvent* event ) override;
	void dropEvent( QDropEvent* event ) override;
	void paintEvent( QPaintEvent* ev ) override;
	virtual void mouseDoubleClickEvent( QMouseEvent* ev ) override;
	virtual void mousePressEvent( QMouseEvent* ev ) override;
	virtual void mouseReleaseEvent( QMouseEvent* ev ) override;
	virtual void mouseMoveEvent( QMouseEvent* ev ) override;

	ComponentView* m_pComponentView;

	std::set<LayerInfo> m_layerInfos;

	QPixmap m_speakerPixmap;
	Drag m_drag;
    QPointF m_dragStartPoint;
	quint64 m_dragStartTimeStamp;

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
	void showLayerStartVelocity(
		const std::shared_ptr<H2Core::InstrumentLayer> pLayer,
		QMouseEvent* pEvent
	);

	/**
	 * display a layer's end velocity in a tooltip
	 *
	 * @param pLayer    The layer
	 * @param pEvent    The event carrying mouse position
	 */
	void showLayerEndVelocity(
		const std::shared_ptr<H2Core::InstrumentLayer> pLayer,
		QMouseEvent* pEvent
	);

	/** Used to detect changed in the font*/
	int getPointSizeButton() const;
};

#endif
