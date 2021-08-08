/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <core/Preferences.h>
#include "../Widgets/WidgetWithScalableFont.h"

namespace H2Core
{
	class InstrumentLayer;
}

class WaveDisplay : public QWidget, protected WidgetWithScalableFont<8, 10, 12>, public H2Core::Object
{
    H2_OBJECT(WaveDisplay)
	Q_OBJECT

	public:
		explicit WaveDisplay(QWidget* pParent);
		~WaveDisplay();

		virtual void	updateDisplay( std::shared_ptr<H2Core::InstrumentLayer> pLayer );

		void			paintEvent( QPaintEvent *ev );
		void			resizeEvent( QResizeEvent * event );
		void			mouseDoubleClickEvent(QMouseEvent *ev);
		
		void			setSampleNameAlignment(Qt::AlignmentFlag flag);

public slots:
		void onPreferencesChanged( bool bAppearanceOnly );
	
	signals:
		void doubleClicked(QWidget *pWidget);

	protected:
		Qt::AlignmentFlag			m_SampleNameAlignment;
		QPixmap						m_Background;
		QString						m_sSampleName;
		int *						m_pPeakData;
		
		/*
		 * Used to re-initialise m_pPeakData if width has changed
		 */
		
		int							m_nCurrentWidth;
		
		std::shared_ptr<H2Core::InstrumentLayer>	m_pLayer;
		/** Used to detect changed in the font*/
		QString m_sLastUsedFontFamily;
		/** Used to detect changed in the font*/
		H2Core::Preferences::FontSize m_lastUsedFontSize;
};

inline void WaveDisplay::setSampleNameAlignment(Qt::AlignmentFlag flag)
{
	m_SampleNameAlignment = flag;
}

#endif
