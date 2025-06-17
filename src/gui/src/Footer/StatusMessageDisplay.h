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

#ifndef STATUS_MESSAGE_Display_H
#define STATUS_MESSAGE_Display_H


#include "LCDDisplay.h"

/** Non-interactive display for status messages in the
	#PlayerControl. Shows a popup list of previous messages when clicking it.*/
/** \ingroup docGUI docWidgets*/
class StatusMessageDisplay : public LCDDisplay, public H2Core::Object<StatusMessageDisplay>
{
    H2_OBJECT(StatusMessageDisplay)
	Q_OBJECT

public:
	StatusMessageDisplay( QWidget* pParent, const QSize& size );
	~StatusMessageDisplay();

	void showMessage( const QString& sMessage, const QString& sCaller = "" );

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

private slots:
	void onStatusTimerEvent();
	void onScrollTimerEvent();
	
private:
	void updateStyleSheet();
	void reset();
	void updateMaxLength();
	void displayMessage( const QString& sMessage );
	
	QStringList m_statusMessages;
	QString m_sScrollMessage;

	/** Amount of time in milliseconds for which the status message
		will be displayed*/
	int m_nShowTimeout;
	/** Amount of time in milliseconds that pass between chopping
	 *	characters for messages to long to display as a whole.
	 *
	 * Is supposed to be smaller than #m_nShowTimeout.
	 */
	int m_nScrollTimeout;
	/** Amount of time in milliseconds that pass before a message is
	 * being scrolled. Important in order for the user to be able to
	 * read it properly.
	 *
	 * Is supposed to be smaller than #m_nShowTimeout.
	 */
	int m_nPreScrollTimeout;
	bool m_bPreScroll;

	int m_nHistorySize;
	QString m_sLastCaller;
	
	QTimer* m_pStatusTimer;
	QTimer* m_pScrollTimer;
	
	bool m_bEntered;
		
	virtual void paintEvent( QPaintEvent *ev ) override;
#ifdef H2CORE_HAVE_QT6
		virtual void enterEvent( QEnterEvent *ev ) override;
#else
		virtual void enterEvent( QEvent *ev ) override;
#endif
	virtual void leaveEvent( QEvent *ev ) override;
	virtual void mousePressEvent( QMouseEvent* ev ) override;
};

#endif
