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
	 * along with this program; if not, write to the Free Software
	 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	 *
	 */
#ifndef MIDISENSE_WIDGET_H
#define MIDISENSE_WIDGET_H

#include <QtGui>
#include <QtWidgets>

#include <memory>
#include <core/Object.h>
#include <core/MidiAction.h>
#include <core/IO/MidiCommon.h>

/** \ingroup docGUI docWidgets docMIDI*/
class MidiSenseWidget :  public QDialog , public H2Core::Object<MidiSenseWidget>
	{
	H2_OBJECT(MidiSenseWidget)
	Q_OBJECT

	public:
		explicit MidiSenseWidget(QWidget*,bool bDirectWrite = false , std::shared_ptr<Action> pAction = nullptr);
		~MidiSenseWidget();

		H2Core::MidiMessage::Event getLastMidiEvent() const;
		int getLastMidiEventParameter() const;

	private slots:
		void		updateMidi();

	private:
		H2Core::MidiMessage::Event 		m_lastMidiEvent;
		int								m_nLastMidiEventParameter;
		QTimer*							m_pUpdateTimer;
		QLabel*							m_pURLLabel;
		std::shared_ptr<Action>			m_pAction;
		bool							m_bDirectWrite;
};

inline H2Core::MidiMessage::Event MidiSenseWidget::getLastMidiEvent() const {
	return m_lastMidiEvent;
}
inline int MidiSenseWidget::getLastMidiEventParameter() const {
	return m_nLastMidiEventParameter;
}
#endif
