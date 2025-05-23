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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#ifndef LCDTextEdit_H
#define LCDTextEdit_H

#include <QtGui>
#include <QtWidgets>
#include <QTextEdit>

#include <core/config.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

/** Wrapper around QTextEdit to provide highlighting while hovering.
 *
 * Why is it called LCD...? To keep the name in line with the other widget,
 * which happen to be designed like a LCD display in older versions of
 * Hydrogen. */
/** \ingroup docGUI docWidgets*/
class LCDTextEdit : public QTextEdit, public H2Core::Object<LCDTextEdit>
{
    H2_OBJECT(LCDTextEdit)
	Q_OBJECT

public:
	LCDTextEdit( QWidget* pParent, bool bIsActive = true );
	~LCDTextEdit();

	bool getIsActive() const;
	void setIsActive( bool bIsActive );

	bool getIsHovered() const;

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

protected:
	virtual void paintEvent( QPaintEvent *ev ) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent( QEvent *ev ) override;

private:
	void updateFont();
	void updateStyleSheet();

	bool m_bEntered;
	bool m_bIsActive;
	std::vector<int> m_fontPointSizes;
};
inline bool LCDTextEdit::getIsActive() const {
	return m_bIsActive;
}
inline bool LCDTextEdit::getIsHovered() const {
	return m_bEntered;
}

#endif
