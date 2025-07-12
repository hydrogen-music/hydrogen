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

#ifndef MIDI_BASE_DRIVER_H
#define MIDI_BASE_DRIVER_H

#include <core/IO/MidiInput.h>
#include <core/IO/MidiOutput.h>
#include <core/Midi/MidiMessage.h>

#include <QString>
#include <QTime>

#include <deque>
#include <vector>

namespace H2Core {

/** \ingroup docCore docMIDI */
class MidiBaseDriver : public Object<MidiBaseDriver>,
					   public virtual MidiInput,
					   public virtual MidiOutput
{
	H2_OBJECT(MidiBaseDriver)

	public:
		enum class PortType {
			Input,
			Output
		};
		static QString portTypeToQString( const PortType& portType );

		static constexpr int nBacklogSize = 200;

		MidiBaseDriver();
		virtual ~MidiBaseDriver();

		virtual void close() = 0;
		virtual std::vector<QString> getExternalPortList( const PortType& portType ) = 0;
		virtual void open() = 0;

		MidiInput::HandledInput handleMessage( const MidiMessage& msg ) override;
		MidiOutput::HandledOutput sendMessage( const MidiMessage& msg ) override;

		const std::deque<MidiInput::HandledInput>& getHandledInputs() const;
		const std::deque<MidiOutput::HandledOutput>& getHandledOutputs() const;

		virtual QString toQString( const QString& sPrefix = "",
								   bool bShort = true ) const override {
			return "";
		}

	private:

		std::deque<MidiInput::HandledInput> m_handledInputs;
		std::deque<MidiOutput::HandledOutput> m_handledOutputs;
};

inline const std::deque<MidiInput::HandledInput>& MidiBaseDriver::getHandledInputs() const {
	return m_handledInputs;
}
inline const std::deque<MidiOutput::HandledOutput>& MidiBaseDriver::getHandledOutputs() const {
	return m_handledOutputs;
}

};

#endif
