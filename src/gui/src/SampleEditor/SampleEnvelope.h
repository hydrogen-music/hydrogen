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

#ifndef SAMPLE_ENVELOPE
#define SAMPLE_ENVELOPE

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <memory>

class SampleEditor;

namespace H2Core {
class InstrumentLayer;
class EnvelopePoint;
}  // namespace H2Core

/** \ingroup docGUI*/
class SampleEnvelope : public QWidget, public H2Core::Object<SampleEnvelope> {
	H2_OBJECT( SampleEnvelope )
	Q_OBJECT

   public:
	static constexpr int nPointWidth = 8;
	static constexpr int nToolTipHeight = 20;
	static constexpr int nToolTipWidth = 40;

	explicit SampleEnvelope( SampleEditor* pParent );
	~SampleEnvelope();

	void setEnabled( bool bEnabled );
	void setLayer( std::shared_ptr<H2Core::InstrumentLayer> pLayer );

   private:
	enum class Style { None, Hovered, Selected, Background };

	void mouseMoveEvent( QMouseEvent* ev ) override;
	void mousePressEvent( QMouseEvent* ev ) override;
	void mouseReleaseEvent( QMouseEvent* ev ) override;
	void paintEvent( QPaintEvent* ev ) override;

	std::vector<H2Core::EnvelopePoint> getElementsAtPoint( const QPoint& point
	);

	void drawLine(
		QPainter& painter,
		const std::vector<H2Core::EnvelopePoint>& envelope,
		QColor color,
		Style style
	);
	void drawPoint(
		QPainter& painter,
		const H2Core::EnvelopePoint& point,
		QColor color,
		Style style
	);

	void updateMouseSelection( QMouseEvent* ev );
	void updateEnvelope();

	SampleEditor* m_pSampleEditor;

	bool m_bEnabled;

	QString m_sSelectedEnvelopePointValue;

	/** Cache for undo/redo actions during drag moving. Otherwise, the operation
	 * would be to inefficient. */
	std::shared_ptr<H2Core::EnvelopePoint> m_pDragPoint;
	std::shared_ptr<H2Core::EnvelopePoint> m_pHoveredPoint;
	int m_nDragStartX;
	int m_nDragStartY;

	int m_nSelectedEnvelopePoint;
};

inline void SampleEnvelope::setEnabled( bool bEnabled )
{
	m_bEnabled = bEnabled;
}

#endif
