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

#ifndef H2_MIDI_OUTPUT_H
#define H2_MIDI_OUTPUT_H

#include <core/Helpers/Time.h>
#include <core/Midi/MidiMessage.h>
#include <core/Object.h>

#include <QString>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace H2Core {

class MidiMessage;

/**
 * MIDI input base class
 */
/** \ingroup docCore docMIDI */
class MidiOutput : public virtual Object<MidiOutput>
{
	H2_OBJECT(MidiOutput)

	public:
		struct HandledOutput {
			TimePoint timePoint;
			MidiMessage::Type type;
			int nData1;
			int nData2;
			int nChannel;

			QString toQString() const;
		};

		MidiOutput();
		virtual ~MidiOutput();

		/** Checks whether output part of the MIDI driver was properly set up
		 * and could send MIDI events. This does not mean yet that there is an
		 * established connection to another MIDI device. Such a connection
		 * could (depending on the driver and OS) be established outside of
		 * Hydrogen without letting us know. */
		virtual bool isOutputActive() const = 0;

		/** @returns true in case #msg could be sent. */
		virtual std::shared_ptr<HandledOutput> sendMessage( const MidiMessage& msg );

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
		 * enough to be useful.
		 *
		 * For now Hydrogen does not support continuous tempo changes but tempo
		 * is either changed on user interaction or when entering a new column.
		 * Either event occur much more rarely then MIDI Clock ticks. Thus, we
		 * went for a lock-free sending routine in order to save resources and
		 * require for restarting the routine in case the tempo/interval does
		 * change. This can be done by consecutive calls for this method with
		 * different @a fBpm values. */
		void startMidiClockStream( float fBpm );
		void stopMidiClockStream();

		/** Blocks the invoking thread till the next MIDI clock tick was sent by
		 * Hydrogen. */
		void waitForNextMidiClockTick();

	private:
		virtual void sendControlChangeMessage( const MidiMessage& msg ) = 0;
		virtual void sendNoteOffMessage( const MidiMessage& msg ) = 0;
		virtual void sendNoteOnMessage( const MidiMessage& msg ) = 0;
	/** Not purely virtual since it is required in midiClockStream().  */
	virtual void sendSystemRealTimeMessage( const MidiMessage& msg ){};

		static void midiClockStream( void* pInstance );

		/** Shared data
		 * @{ */
		std::condition_variable m_midiClockCV;
		std::mutex m_midiClockMutex;
		bool m_bSendClockTick;
		bool m_bNotifyOnNextTick;
		std::chrono::duration<float> m_interval;
		std::chrono::duration<float> m_intervalCompensation;
		long m_nAverageIntervalNs;
		TimePoint m_lastTick;
		long long m_nTickCount;
		/** @} */

		std::shared_ptr< std::thread > m_pClockThread;
};

};

#endif

