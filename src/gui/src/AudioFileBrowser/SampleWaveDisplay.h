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

#ifndef SAMPLE_WAVE_DISPLAY
#define SAMPLE_WAVE_DISPLAY

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif
#include <hydrogen/object.h>

/** \ingroup docGUI */
class SampleWaveDisplay : public QWidget, public H2Core::Object
{
	Q_OBJECT

	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
		SampleWaveDisplay( QWidget* pParent );
		~SampleWaveDisplay();

		void updateDisplay( QString filename );

		void paintEvent(QPaintEvent *ev);

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
		QPixmap m_Background;
		QString m_sSampleName;
		int *	m_pPeakData;
};


#endif

