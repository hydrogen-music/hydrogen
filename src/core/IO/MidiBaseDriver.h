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
#include <QtCore/QMutex>
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

		void clearHandledInput();
		void clearHandledOutput();

		std::shared_ptr<MidiInput::HandledInput> handleMessage(
			const MidiMessage& msg ) override;
		std::shared_ptr<MidiOutput::HandledOutput> sendMessage(
			const MidiMessage& msg ) override;

		std::vector< std::shared_ptr<MidiInput::HandledInput> > getHandledInputs();
		std::vector< std::shared_ptr<MidiOutput::HandledOutput> > getHandledOutputs();

		/** In an ideal MIDI device the audio engine would be driven by either
		 * an internal or external clock providing the next tick on predefined
		 * rate/tempo. But in Hydrogen we do not process tick by tick in the
		 * audio engine but act on a buffer of frames (which can be converted
		 * into ticks) the audio driver provided us for the current process
		 * cycle. These buffers are a lot bigger than the resolution of a MIDI
		 * clock thus #H2Core::AudioEngine can not itself trigger those
		 * messages.
		 *
		 * Instead, we use a timer sending them at a predefined rate (which we
		 * have to change every time a new tempo is encountered). That is far
		 * from ideal as it easily allows for clock drifts. But hopefully it is
		 * enough to be useful. */
		void startMidiClockStream( float fBpm );
		void stopMidiClockStream();

		virtual QString toQString( const QString& sPrefix = "",
								   bool bShort = true ) const override {
			return "";
		}

	private:

		/** The Core thread running the #AudioEngine and the MIDI drivers does
		 * fill both #m_handledInputs and #m_handledOutputs. It is accessed,
		 * however, by a different thread running the GUI.
		 *
		 * To avoid race conditions, we wrap all summary handlers in shared_ptr
		 * and do not return the actual deques holding the latest value, but a
		 * snapshot taken at the time the getter method was called.
		 *
		 * @{ */
		QMutex m_inputMutex;
		QMutex m_outputMutex;

		std::deque< std::shared_ptr<MidiInput::HandledInput> > m_handledInputs;
		std::deque< std::shared_ptr<MidiOutput::HandledOutput> > m_handledOutputs;
		/** #} */
};

inline void MidiBaseDriver::clearHandledInput() {
	m_handledInputs.clear();
}
inline void MidiBaseDriver::clearHandledOutput() {
	m_handledOutputs.clear();
}

};

#endif
