/*
 * Hydrogen
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

#ifndef LOOP_BACK_MIDI_DRIVER_H
#define LOOP_BACK_MIDI_DRIVER_H

#include <core/IO/MidiBaseDriver.h>
#include <core/Midi/MidiMessage.h>

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace H2Core {

/** Helper driver used for unit and integration tests.
 *
 * Each provided output MIDI message will immediately be "received" as input
 * message.
 *
 * \ingroup docCore docMIDI */
class LoopBackMidiDriver : public Object<LoopBackMidiDriver>
						 , public virtual MidiBaseDriver
{
	H2_OBJECT(LoopBackMidiDriver)

	static constexpr int nMaxQueueSize = 100;
	static constexpr int nBacklogSize = 1000;

	public:
		LoopBackMidiDriver();
		virtual ~LoopBackMidiDriver();

		void close() override;
		std::vector<QString> getExternalPortList( const PortType& portType ) override;
		bool isInputActive() const override;
		bool isOutputActive() const override;
		void open() override;

		/** Creates a copy of all messages in the backlog. */
		std::vector<MidiMessage> getBacklogMessages();
		void clearBacklogMessages();
		int getMessageQueueSize();

		QString toQString( const QString& sPrefix = "", bool bShort = true );

	private:
		void sendControlChangeMessage( const MidiMessage& msg ) override;
		void sendNoteOnMessage( const MidiMessage& msg ) override;
		void sendNoteOffMessage( const MidiMessage& msg ) override;
		void sendSystemRealTimeMessage( const MidiMessage& msg ) override;

		/** @a pInstance is expecting a pointer to the instance of the class for
		 * which the message handling thread is spawned. */
		static void messageHandler( void* pInstance );
		void enqueueMessage( const MidiMessage& msg );

		bool m_bActive;
		std::shared_ptr< std::thread > m_pMessageHandler;

		/** Shared data
		 * @{ */
		std::condition_variable m_messageHandlerCV;
		std::mutex m_messageHandlerMutex;
		std::deque<MidiMessage> m_messageQueue;
		std::deque<MidiMessage> m_backlogQueue;
		/** @} */
};

};

#endif
