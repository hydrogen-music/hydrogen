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

#ifndef LCDCOMBO_H
#define LCDCOMBO_H


#include <QtGui>
#include <QComboBox>
#include "WidgetWithScalableFont.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

/** \ingroup docGUI docWidgets*/
class LCDCombo : public QComboBox, protected WidgetWithScalableFont<6, 8, 9>, public H2Core::Object<LCDCombo>
{
	H2_OBJECT(LCDCombo)
	Q_OBJECT

public:
	explicit LCDCombo( QWidget *pParent, QSize size = QSize( 0, 0 ), bool bModifyOnChange = false );
	~LCDCombo();

	void setSize( QSize size );
	void setModifyOnChange( bool bModifyOnChange );
	
	virtual void showPopup() override;
	void addItem(const QString &text, const QVariant &userData = QVariant());
	
	bool getIsActive() const;
	void setIsActive( bool bIsActive );

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );
	void handleIsModified( int );

private:
	void updateStyleSheet();
	QSize m_size;

	bool m_bEntered;
	bool m_bIsActive;

	/** Whether Hydrogen::setIsModified() is invoked with `true` as
		soon as the value of the widget does change.*/
	bool m_bModifyOnChange;

	/** Keep track of the text width of the items added. It is used to
		determine the size of the popup in order to ensure all content
		fits inside.*/
	int m_nMaxWidth;
		
	virtual void paintEvent( QPaintEvent *ev ) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent( QEvent *ev ) override;
};
inline void LCDCombo::setModifyOnChange( bool bModifyOnChange ) {
	m_bModifyOnChange = bModifyOnChange;
}
inline bool LCDCombo::getIsActive() const {
	return m_bIsActive;
}
#endif
