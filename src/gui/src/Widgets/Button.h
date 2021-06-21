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


#ifndef BUTTON_H
#define BUTTON_H


#include <core/Object.h>
#include <core/Preferences.h>
#include <core/MidiAction.h>

#include "MidiLearnable.h"

#include <QtGui>
#include <QPushButton>


/**
 * Generic Button with SVG icons or text.
 */
class Button : public QPushButton, public H2Core::Object, public MidiLearnable
{
    H2_OBJECT
	Q_OBJECT

public:

	enum class Type {
		Push,
		Toggle
	};
	
	/**
	 * Either the path to a SVG image or a text to be displayed has to
	 * be provided. If both are given, the icon will be used over the
	 * text. If the text should be used instead, @a sIcon must the
	 * an empty string.
	 *
	 * \param bColorful If set to false, the icon @a sIcon is expected
	 * to exist in both subfolders "black" and "white" in the "icons"
	 * folder. If the button is not checked, the black version is used
	 * and if checked, the white one is used instead.
	 */
	Button(
		   QWidget *pParent,
		   QSize size,
		   Type type,
		   const QString& sIcon,
		   const QString& sText = "",
		   bool bUseRedBackground = false,
		   QSize iconSize = QSize( 0, 0 ),
		   QString sBaseTooltip = "",
		   bool bColorful = false
		   );
	virtual ~Button();
	
	Button(const Button&) = delete;
	Button& operator=( const Button& rhs ) = delete;

	void setBaseToolTip( const QString& sNewTip );
	void setAction( Action* pAction );

public slots:
	void onPreferencesChanged( bool bAppearanceOnly );

signals:
	void clicked(Button *pBtn);
	void rightClicked(Button *pBtn);
	void mousePress(Button *pBtn);

private:
	void updateFont();
	void updateTooltip();
	
	Type m_type;
	QSize m_size;
	QSize m_iconSize;
	QString m_sBaseTooltip;
	QString m_sRegisteredMidiEvent;
	QString m_sIcon;
	int m_nRegisteredMidiParameter;

	bool m_bColorful;
	bool m_bLastCheckedState;

	void mousePressEvent(QMouseEvent *ev);
	void paintEvent( QPaintEvent* ev);
	
	/** Used to detect changed in the font*/
	QString m_sLastUsedFontFamily;
	/** Used to detect changed in the font*/
	H2Core::Preferences::FontSize m_lastUsedFontSize;
};

#endif
