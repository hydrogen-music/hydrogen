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

#ifndef BEAT_TAP_H
#define BEAT_TAP_H

#include <core/Object.h>

#include <QtGui>
#include <QtWidgets>

#include "../Widgets/WidgetWithScalableFont.h"

class BpmTap : public QWidget,
			   protected WidgetWithScalableFont<8, 10, 12>,
			   public H2Core::Object<BpmTap>
{
    H2_OBJECT(BpmTap)
	Q_OBJECT

public:
		static constexpr int nMargin = 1;

		explicit BpmTap( QWidget* pParent );
		~BpmTap();

		void setBackgroundColor( const QColor& color );
		void setBorderColor( const QColor& color );

		void updateBpmTap();
		void updateIcons();
		void updateStyleSheet();

private:
		QToolButton* m_pBeatLengthUpBtn;
		QToolButton* m_pBeatLengthDownBtn;
		QLabel* m_pBeatLengthLabel;
		QLabel* m_pTotalBeatsLabel;
		QToolButton* m_pTotalBeatsUpBtn;
		QToolButton* m_pTotalBeatsDownBtn;
		QAction* m_pTapTempoAction;
		QAction* m_pBeatCounterTapAction;
		QAction* m_pBeatCounterTapAndPlayAction;
		QToolButton* m_pTapButton;

		QColor m_backgroundColor;
};

inline void BpmTap::setBackgroundColor( const QColor& color ) {
	m_backgroundColor = color;
}

#endif
