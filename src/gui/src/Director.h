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
	virtual void metronomeEvent( int nValue ) override;
	virtual void paintEvent( QPaintEvent*) override;
	virtual void keyPressEvent( QKeyEvent* ev ) override;
	virtual void closeEvent( QCloseEvent* ev ) override;

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );

private slots:
	void updateMetronomBackground();


private:
	QTimer				*m_pTimer;
	QColor				m_Color;
	QPalette			m_BlinkerPalette;
	int					m_nCounter;
	float				m_fBpm;
	int					m_nBar;
	int					m_nFlashingArea;
	QString				m_sTAG;
	QString				m_sTAG2;
	QString				m_sSongName;
};


#endif
