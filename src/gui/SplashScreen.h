/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: SplashScreen.h,v 1.6 2005/05/01 19:50:59 comix Exp $
 *
 */


#ifndef SPLASH_SCREEN_H
#define SPLASH_SCREEN_H

#include <qlabel.h>
#include <qpixmap.h>
#include <qtimer.h>

#include <string>
using std::string;

#include "config.h"
#include "HydrogenApp.h"

#include "lib/Object.h"

/**
 * Fader and VuMeter widget
 */
class SplashScreen : public QWidget, public Object
{
	Q_OBJECT
	private:
		static const uint width = 400;
		static const uint height = 300;

	public:
		/** Constructor */
		SplashScreen();

		/** Destructor */
		~SplashScreen();

	public slots:
		void closeSplash();
};


#endif
