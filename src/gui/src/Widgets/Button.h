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


#ifndef BUTTON_H
#define BUTTON_H

#include <memory>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/MidiAction.h>

#include "MidiLearnable.h"
#include "WidgetWithScalableFont.h"

#include <QtGui>
#include <QPushButton>


/**
 * Generic Button with SVG icons or text.
 *
 * The class comes in two different types, as Button::Type::Push and
 * Button::Type::Toggle. If it is set checkabale via
 * Button::Type::Toggle, the button will change color after it is
 * clicked by mouse _and_ the button is released. The action
 * associated with the button will, however, be triggered once the
 * button is clicked (this is implemented in the parent wigets and can
 * be changed. But giving immediate feedback seems to be intuitive.)
 *
 * Most icons used are black and white. For those, the black one will
 * be used in unchecked state and the white one in checked.
 * 
 * Buttons are MIDI learnable. This means they can be associated with
 * a MIDI action. If done, the action (and the binding) will show up
 * in the tooltip.
 */
/** \ingroup docGUI docWidgets*/
class Button : public QPushButton, protected WidgetWithScalableFont<6, 8, 10>,  public H2Core::Object<Button>, public MidiLearnable
{
    H2_OBJECT(Button)
	Q_OBJECT

public:

	enum class Type {
		/** Button is not set checkable.*/
		Push,
		/** Button is set checkable.*/
		Toggle
	};
	
	/**
	 * Either the path to a SVG image or a text to be displayed has to
	 * be provided. If both are given, the icon will be used over the
	 * text. If the text should be used instead, @a sIcon must the
	 * an empty string.
	 *
	 * \param pParent
	 * \param size
	 * \param type
	 * \param sIcon
	 * \param sText
	 * \param bUseRedBackground
	 * \param iconSize
	 * \param sBaseTooltip
	 * \param bColorful If set to false, the icon @a sIcon is expected
	 * to exist in both subfolders "black" and "white" in the "icons"
	 * folder. If the button is not checked, the black version is used
	 * and if checked, the white one is used instead.
	 */
	Button(
		   QWidget *pParent,
		   QSize size = QSize(),
		   Type type = Type::Toggle,
		   const QString& sIcon = "",
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
	void setAction( std::shared_ptr<Action> pAction );
	
	bool getIsActive() const;
	void setIsActive( bool bIsActive );

	void setSize( QSize size );
	/**  Overwrites the automatically set value. If @a nPixelSize is
		 negative, the automatically set value will be used instead.*/
	void setFixedFontSize( int nPixelSize );
	int getFixedFontSize() const;

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );

signals:
	void clicked(Button *pBtn);
	void rightClicked(Button *pBtn);
	void mousePress(Button *pBtn);

private:
	void updateStyleSheet();
	void updateFont();
	void updateTooltip();
	void updateIcon();

	bool m_bUseRedBackground;
	Type m_type;
	QSize m_size;
	QSize m_iconSize;
	QString m_sBaseTooltip;
	QString m_sRegisteredMidiEvent;
	QString m_sIcon;
	int m_nRegisteredMidiParameter;
	int m_nFixedFontSize;

	QString m_sBorderRadius;

	bool m_bColorful;
	bool m_bLastCheckedState;

	bool m_bIsActive;

	virtual void mousePressEvent(QMouseEvent *ev) override;
	virtual void paintEvent( QPaintEvent* ev) override;

};
inline bool Button::getIsActive() const {
	return m_bIsActive;
}

inline void Button::setFixedFontSize( int nPixelSize ) {
	m_nFixedFontSize = nPixelSize;
}
inline int Button::getFixedFontSize() const {
	return m_nFixedFontSize;
}

#endif
