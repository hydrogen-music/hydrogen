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
#ifndef MIDISENSE_WIDGET_H
#define MIDISENSE_WIDGET_H

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <hydrogen/object.h>
#include <hydrogen/midi_action.h>

class MidiSenseWidget : public QDialog ,public H2Core::Object
	{
	H2_OBJECT
	Q_OBJECT

	public:
		MidiSenseWidget(QWidget*,bool m_DirectWrite = false , MidiAction* m_pAction = NULL);
		~MidiSenseWidget();

		QString		m_sLastMidiEvent;
		int			m_LastMidiEventParameter;
	
	private slots:
		void		updateMidi();

	private:
		QTimer*		m_pUpdateTimer;
		QLabel*		m_pURLLabel;
		MidiAction* m_pAction;
		bool		m_DirectWrite;
};

#endif
