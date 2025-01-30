/*
 * Hydrogen
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef COLORSELECTIONBUTTON_H
#define COLORSELECTIONBUTTON_H


#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include <QtGui>
#include <QPushButton>
#include <QColor>

/** QPushButton opening a QColorDialog when clicked and displaying the
 * selected color as background - with neither text nor an icon present.
 * 
 */
/** \ingroup docGUI docWidgets*/
class ColorSelectionButton : public QPushButton, public H2Core::Object<ColorSelectionButton>
{
    H2_OBJECT(ColorSelectionButton)
	Q_OBJECT

public:
	ColorSelectionButton( QWidget *pParent,
						  const QColor& sInitialColor = Qt::black, int nSize = 0 );
	~ColorSelectionButton();

	const QColor& getColor() const;
	void setColor( const QColor& color );

	/** see #m_bHiding */
	void pretendToHide();
	/** see #m_bHiding */
	void pretendToShow();
	
signals:
	void colorChanged();

private:
	bool m_bMouseOver;
	/**
	 * To allow buttons to be listed in a grid within a grid inside
	 * the PreferencesDialog > Appearance > Interface tab, a small
	 * hack is required.
	 *
	 * Using the regular show() and hide() methods when increasing the
	 * number of colors in the Song Editor, both spacing and placement
	 * of the buttons will be really awkward. I would like to put them
	 * in a right-aligned horizontal list spanning multiple rows with
	 * buttons having a constant spacing between them. However, when
	 * when using a regular QGridLayout and having, say, 2 buttons - 2
	 * colors in the song editor - and all the others are hidden, Qt
	 * will create two columns of same size taking all the space and
	 * put the buttons into them. The result looks like one button is
	 * placed on the right border (what we expect) and the other one
	 * right into the middle of the field of the outer grid (what we
	 * do not expect). This looks awkward until enough buttons are
	 * shown to fill one entire line.
	 *
	 * Introducing spacers, column stretching, or similar things lead
	 * to the buttons being pushed slightly outside the outer grid
	 * layout. When resizing the preferences dialog only the button
	 * list was changing position which was really ugly.
	 *
	 * As a work around, we do not hide any of the buttons but just
	 * paint them transparently as if they are not there. This we call
	 * #m_bHiding and trigger it with #pretendToHide() and
	 * #pretendToShow(). We always have a sufficient amount of colors
	 * to fill a row and expanding of the dialog works like a charm
	 * again.
	 */
	bool m_bHiding;

	virtual void mousePressEvent(QMouseEvent *ev) override;
	virtual void enterEvent(QEvent *ev) override;
	virtual void leaveEvent(QEvent *ev) override;
	virtual void paintEvent( QPaintEvent* ev) override;

	QColor m_sColor;
};

inline const QColor& ColorSelectionButton::getColor() const {
	return m_sColor;
}

#endif
