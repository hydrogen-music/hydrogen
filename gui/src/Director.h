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

#ifndef METRO_BLINKER_H
#define METRO_BLINKER_H

#include "config.h"

#include <QDialog>
#include "ui_Director_UI.h"
#include <hydrogen/Object.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include "EventListener.h"


class Director : public QDialog, public Ui_Director_UI, public Object, public EventListener

{
	Q_OBJECT
	public:
		
		Director( QWidget* pParent );
		~Director();

		virtual void metronomeEvent( int nValue );	
		virtual void paintEvent( QPaintEvent*);

	private slots:

		void updateMetronomBackground();
		//void updateBackground();


	private:
		QTimer *timer;
		QColor p_color;
//		QPixmap *m_pBackground;
		QPalette blinkerPalette;
		int p_counter;
		int p_fadealpha;
		float p_bpm;
		int p_bar;
		int p_wechselblink;
		QString TAG;
		QString TAG2;
		QString songName;
		int tagbeat;

};


#endif

