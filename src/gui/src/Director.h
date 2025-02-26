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

#ifndef METRO_BLINKER_H
#define METRO_BLINKER_H


#include <QDialog>
#include "ui_Director_UI.h"
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Hydrogen.h>
#include "EventListener.h"


/** \ingroup docGUI*/
class Director :  public QDialog, public Ui_Director_UI,  public H2Core::Object<Director>, public EventListener
{
	H2_OBJECT(Director)
	Q_OBJECT
public:

	explicit Director( QWidget* pParent );
	~Director();
	
	Director(const Director&) = delete;
	Director& operator=( const Director& rhs ) = delete;

	virtual void updateSongEvent( int nValue ) override;
	virtual void timelineUpdateEvent( int nValue ) override;
	virtual void bbtChangedEvent() override;
	virtual void tempoChangedEvent( int nValue ) override;
	
	virtual void paintEvent( QPaintEvent*) override;
	virtual void keyPressEvent( QKeyEvent* ev ) override;
	virtual void closeEvent( QCloseEvent* ev ) override;

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	void resizeEvent( QResizeEvent *event ) override;

private slots:
	void updateMetronomBackground();


private:
	enum FontUpdate {
		SongName = 0x001,
		TagCurrent = 0x002,
		TagNext = 0x004
	};
		void updateBBT();
	/** @return true in case either #m_sTagCurrent or #m_sTagNext did
	 * change.*/
	bool updateTags();
	void updateLabelContainers();
	void updateFontSize( FontUpdate update );

	QTimer				*m_pTimer;
	QColor				m_Color;
	QPalette			m_BlinkerPalette;
	int					m_nBeat;
	int					m_nBar;
	int					m_nFlashingArea;
	QString				m_sTagCurrent;
	QString				m_sTagNext;
	QString				m_sSongName;
	QRect				m_rectSongName;
	QRect				m_rectTagCurrent;
	QRect				m_rectTagNext;
	QFont				m_fontSongName;
	QFont				m_fontTagCurrent;
	QFont				m_fontTagNext;
};


#endif
