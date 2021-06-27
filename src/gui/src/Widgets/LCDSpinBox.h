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

#ifndef LCDSPINBOX_H
#define LCDSPINBOX_H

#include <QtGui>
#include <QDoubleSpinBox>

#include <core/Object.h>

/** Updating the font family in QDoubleSpinBox is not supported and
	changing the font size (both via setFont()) yields erratic results.*/
class LCDSpinBox : public QDoubleSpinBox, public H2Core::Object
{
    H2_OBJECT
	Q_OBJECT

public:

	enum class Type {
		Int,
		Double
	};

	LCDSpinBox( QWidget *pParent, QSize size, Type type, double fMin = 0.0, double fMax = 1.0 );
	~LCDSpinBox();

public slots:
	void onPreferencesChanged( bool bAppearanceOnly );
	
private:
	void updateStyleSheet();
	QSize m_size;
	Type m_type;

	QColor m_lastHighlightColor;
	QColor m_lastAccentColor;
	QColor m_lastSpinBoxSelectionColor;
	QColor m_lastSpinBoxSelectionTextColor;
	QColor m_lastAccentTextColor;

	bool m_bEntered;

	virtual QString textFromValue( double fValue ) const;
	virtual double valueFromText( const QString& sText ) const;	
	virtual void paintEvent( QPaintEvent *ev );
	virtual void enterEvent( QEvent *ev );
	virtual void leaveEvent( QEvent *ev );
};

#endif
