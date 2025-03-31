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

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class InstrumentLayer;
}

/** \ingroup docGUI*/
class WaveDisplay : public QWidget,
					protected WidgetWithScalableFont<8, 10, 12>,
					public H2Core::Object<WaveDisplay>
{
    H2_OBJECT(WaveDisplay)
	Q_OBJECT

	public:

		static constexpr int nGradientScaling = 130;

		explicit WaveDisplay(QWidget* pParent);
		~WaveDisplay();

	
		virtual void	updateDisplay( std::shared_ptr<H2Core::InstrumentLayer> pLayer );

		virtual void	paintEvent( QPaintEvent *ev ) override;
		virtual void	resizeEvent( QResizeEvent * event ) override;
		virtual void	mouseDoubleClickEvent(QMouseEvent *ev) override;
		
		void			setSampleNameAlignment( const Qt::AlignmentFlag& flag );

public slots:
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
	
	signals:
		void doubleClicked(QWidget *pWidget);

	protected:

	void createBackground( QPainter* painter );
	
		Qt::AlignmentFlag			m_SampleNameAlignment;
		QString						m_sSampleName;
		int *						m_pPeakData;
		
		/*
		 * Used to re-initialise m_pPeakData if width has changed
		 */
		
		int							m_nCurrentWidth;
		
		std::shared_ptr<H2Core::InstrumentLayer>	m_pLayer;
};

inline void WaveDisplay::setSampleNameAlignment( const Qt::AlignmentFlag& flag )
{
	m_SampleNameAlignment = flag;
}

#endif
