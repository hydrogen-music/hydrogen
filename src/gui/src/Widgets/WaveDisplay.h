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

#ifndef WAVE_DISPLAY
#define WAVE_DISPLAY

#include <QtGui>
#include <QtWidgets>
#include <vector>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "WidgetWithScalableFont.h"

namespace H2Core {
class InstrumentLayer;
}

/** \ingroup docGUI*/
class WaveDisplay : public QWidget,
					protected WidgetWithScalableFont<8, 10, 12>,
					public H2Core::Object<WaveDisplay> {
	H2_OBJECT( WaveDisplay )
	Q_OBJECT

   public:
	static constexpr int nGradientScaling = 130;

	enum class Channel { Left, Right };
	enum class Label { SampleName, Fallback };
	/** If we have enough width to display all the available data, we render
	 * the actual wave form. If not, we calculate the envelope (maximum
	 * values within a time slice) of both positive and negative data and
	 * render them insted. */
	enum class Type { Envelope, Wave };

	explicit WaveDisplay( QWidget* pParent, Channel channel = Channel::Left );
	~WaveDisplay();

	virtual void setLayer( std::shared_ptr<H2Core::InstrumentLayer> pLayer );
	virtual void updateBackground();

	virtual void mouseDoubleClickEvent( QMouseEvent* ev ) override;
	virtual void paintEvent( QPaintEvent* ev ) override;
	virtual void resizeEvent( QResizeEvent* event ) override;

	void setSampleNameAlignment( const Qt::AlignmentFlag& flag );

   public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

   signals:
	void doubleClicked( QWidget* pWidget );

   protected:
	virtual void drawPeakData();
	virtual void updatePeakData();

	QPixmap* m_pBackgroundPixmap;
	QPixmap* m_pPeakDataPixmap;

	Channel m_channel;
	Label m_label;
	Type m_type;

	Qt::AlignmentFlag m_SampleNameAlignment;
	QString m_sSampleName;
	QString m_sFallbackLabel;
	std::vector<int> m_peakData;
	/** In case we render the envelope, we use this member to keep track of
	 * the minimum values of the wave form.*/
	std::vector<int> m_peakDataMin;

	int m_nActiveWidth;

	std::shared_ptr<H2Core::InstrumentLayer> m_pLayer;
};

inline void WaveDisplay::setSampleNameAlignment( const Qt::AlignmentFlag& flag )
{
	m_SampleNameAlignment = flag;
}

#endif
