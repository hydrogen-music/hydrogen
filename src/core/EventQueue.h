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

#ifndef EVENT_QUEUE_H
#define EVENT_QUEUE_H

#include <core/Basics/Event.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/Note.h>
#include <core/Object.h>

#include <cassert>
#include <deque>
#include <memory>
#include <mutex>
#include <random>
#include <vector>

namespace H2Core
{

/** Object handling the communication between the core of Hydrogen and
 * its GUI.
 *
 * Whenever a specific condition is met or occasion happens within the core part
 * of Hydrogen (its engine), an Event will be added to the EventQueue singleton.
 * The GUI checks the content of this queue on a regular basis using
 * HydrogenApp::onEventQueueTimer(). The actual frequency is set in the
 * constructor HydrogenApp::HydrogenApp() to 20 times per second. Now, whenever
 * an Event of a certain Event::Type is encountered, the corresponding function
 * in the EventListener will be invoked to respond to the condition of the
 * engine. For details about the mapping of Event::Type to functions please see
 * the documentation of HydrogenApp::onEventQueueTimer().*/
/** \ingroup docCore docEvent */
class EventQueue : public H2Core::Object<EventQueue>
{
	H2_OBJECT(EventQueue)
public:

	/** Maximum number of events to be stored in the
		H2Core::EventQueue::m_eventQueue.*/
	static constexpr int nMaxEvents = 1024;

	/**
	* If #__instance equals 0, a new EventQueue singleton will be
	 * created and stored in it.
	 *
	 * It is called in Hydrogen::create_instance().
	 */
	static void create_instance();
	/**
	 * Returns a pointer to the current EventQueue singleton
	 * stored in #__instance.
	 */
	static EventQueue* get_instance() { assert(__instance); return __instance; }
	~EventQueue();

	/**
	 * Queues the next event into the EventQueue.
	 *
	 * The event itself will be constructed inside the function and will be two
	 * properties: an Event::Type @a type and a value @a nValue. Since the event
	 * written to the queue most recently is indexed with #m_nWriteIndex, this
	 * variable is incremented once and its modulo with respect to #MAX_EVENTS
	 * is calculated to determine the position of insertion into
	 * #m_eventQueue.
	 *
	 * The modulo operation is necessary because #m_nWriteIndex will be only
	 * incremented and does not respect the actual length of #m_eventQueue
	 * itself.
	 *
	 * \param type Type of the event, which will be queued.
	 * \param nValue Value specifying the content of the new event.
	 *
	 * \returns the ID of the created #H2Core::Event.
	 */
	long pushEvent( const Event::Type type, const int nValue );
	/**
	 * Reads out the next event of the EventQueue.
	 *
	 * Since the event read out most recently is indexed with #m_nReadIndex,
	 * this variable is incremented once and its modulo with respect to
	 * #MAX_EVENTS is calculated to determine the event returned from
	 * #m_eventQueue.
	 *
	 * The modulo operation is necessary because #m_nReadIndex will be only
	 * incremented and does not respect the actual length of #m_eventQueue
	 * itself.
	 *
	 * \return Next event in line.
	 */
	std::unique_ptr<Event> popEvent();

	/** Removes all events of type @a type from the queue. */
	void dropEvents( const Event::Type& type );

	struct AddMidiNoteVector {
		int nColumn;       // position
		Instrument::Id id; // specifies the instrument triggered
		int nPattern;      // pattern number
		int nLength;
		float fVelocity;
		float fPan;
		Note::Key key;
		Note::Octave octave;
	};
	std::vector<AddMidiNoteVector> m_addMidiNoteVector;

	bool getSilent() const;
	void setSilent( bool bSilent );

	/** The EventQueue holds the seeded random engine used to assign each new
     * #H2Core::Event a (more or less) unique id on creation. */
	long createEventId();
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true );

private:
	EventQueue();
	static EventQueue *__instance;

	std::deque< std::unique_ptr<Event> >m_eventQueue;

	std::mutex m_mutex;

	/** Whether or not to push log messages.*/
	bool m_bSilent;

	std::default_random_engine m_randomEngine;
	std::uniform_int_distribution<long> m_randomDistribution;
};

inline bool EventQueue::getSilent() const {
	return m_bSilent;
}
inline void EventQueue::setSilent( bool bSilent ) {
	m_bSilent = bSilent;
}

};

#endif
