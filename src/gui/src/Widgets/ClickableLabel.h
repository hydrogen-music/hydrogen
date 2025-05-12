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

#ifndef CLICKABLE_LABEL_H
#define CLICKABLE_LABEL_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>

#include <QtGui>
#include <QtWidgets>

/** Custom QLabel that emits a signal when clicked.
 *
 * The label tries to be smart when choosing the font size. It knows
 * its own size and decreases the font size - if the original would
 * make the text overflow - until the text fits.
 *
 */
/** \ingroup docGUI docWidgets*/
class ClickableLabel : public QLabel, public H2Core::Object<ClickableLabel>
{
	H2_OBJECT(ClickableLabel)
	Q_OBJECT

public:
	/** The individual colors of the text won't be exposed but are up
		to the palette/application-wide settings.*/
	enum class Color {
		Bright,
		Dark
	};
	
	explicit ClickableLabel( QWidget *pParent, QSize size = QSize( 0, 0 ),
							 QString sText = "", Color color = Color::Bright,
							 bool bIsEditable = false );

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );
	void setText( const QString& sNewText );

signals:
	void labelClicked( ClickableLabel* pLabel );

private:
	void updateStyleSheet();
	void updateFont( QString sFontFamily, H2Core::FontTheme::FontSize fontSize );

	virtual void mousePressEvent( QMouseEvent * e ) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent( QEvent * e ) override;
	virtual void paintEvent( QPaintEvent * e ) override;
	QSize m_size;
	Color m_color;

	/** If set to true a highlight will be painted when hovered. This
		should be set if a callback is connected and the user is able to
		change its content.*/
	bool m_bIsEditable;
	bool m_bEntered;
};


#endif

