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

#ifndef LCDDisplay_H
#define LCDDisplay_H


#include <QtGui>
#include <QtWidgets>
#include <QLineEdit>

#include <core/Object.h>
#include <core/Preferences.h>

#include "WidgetWithScalableFont.h"

#include <vector>

/** Non-interactive display used for both numerical values and the
	status display.*/
class LCDDisplay : public QLineEdit, protected WidgetWithScalableFont<7, 9, 11>, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

public:
	LCDDisplay( QWidget* pParent, QSize size, bool bFixedFont = false );
	~LCDDisplay();

	void setUseRedFont( bool bUseRedFont );

public slots:
	void onPreferencesChanged( bool bAppearanceOnly );

private:
	void updateFont();
	void updateStyleSheet();
	QSize m_size;

	bool m_bFixedFont;
	bool m_bUseRedFont;

	QColor m_lastWindowColor;
	QColor m_lastButtonRedColor;
	QColor m_lastWindowTextColor;
	/** Used to detect changed in the font*/
	QString m_sLastUsedFontFamily;
	/** Used to detect changed in the font*/
	H2Core::Preferences::FontSize m_lastUsedFontSize;
	
	virtual void paintEvent( QPaintEvent *ev );
};

#endif
