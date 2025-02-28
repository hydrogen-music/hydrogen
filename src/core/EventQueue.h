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
#include <core/Basics/Note.h>
#include <core/Object.h>

#include <cassert>
#include <memory>
#include <mutex>
#include <vector>

/** Maximum number of events to be stored in the
    H2Core::EventQueue::m_eventsBuffer.*/
#define MAX_EVENTS 1024

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
public:/**
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
	 * #m_eventsBuffer.
	 *
	 * The modulo operation is necessary because #m_nWriteIndex will be only
	 * incremented and does not respect the actual length of #m_eventsBuffer
	 * itself.
	 *
	 * \param type Type of the event, which will be queued.
	 * \param nValue Value specifying the content of the new event.
	 */
	void pushEvent( const Event::Type type, const int nValue );
	/**
	 * Reads out the next event of the EventQueue.
	 *
	 * Since the event read out most recently is indexed with #m_nReadIndex,
	 * this variable is incremented once and its modulo with respect to
	 * #MAX_EVENTS is calculated to determine the event returned from
	 * #m_eventsBuffer.
	 *
	 * The modulo operation is necessary because #m_nReadIndex will be only
	 * incremented and does not respect the actual length of #m_eventsBuffer
	 * itself.
	 *
	 * \return Next event in line.
	 */
	std::unique_ptr<Event> popEvent();

	struct AddMidiNoteVector {
		int m_column;       // position
		int m_instrumentId; // specifies the instrument triggered
		int m_pattern;      // pattern number
		int m_length;
		float f_velocity;
		float f_pan;
		Note::Key nk_noteKeyVal;
		Note::Octave no_octaveKeyVal;
	};
	std::vector<AddMidiNoteVector> m_addMidiNoteVector;

	bool getSilent() const;
	void setSilent( bool bSilent );
	
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
	/**
	 * Constructor of the EventQueue class.
	 *
	 * It fills all #MAX_EVENTS slots of the #m_eventsBuffer with
	 * #H2Core::Event::Type::None and assigns itself to #__instance. Called by
	 * create_instance().
	 */
	EventQueue();
	/**
	 * Object holding the current EventQueue singleton. It is initialized with
	 * nullptr, set in EventQueue(), and accessed via get_instance().
	 */
	static EventQueue *__instance;

	/**
	 * Continuously growing number indexing the event, which has been read from
	 * the EventQueue most recently.
	 *
	 * It is incremented with each call to popEvent().
	 */
	volatile unsigned int m_nReadIndex;
	/**
	 * Continuously growing number indexing the event, which has been written to
	 * the EventQueue most recently.
	 *
	 * It is incremented with each call to pushEvent().
	 */
	volatile unsigned int m_nWriteIndex;
	/**
	 * Array of all events contained in the EventQueue.
	 *
	 * Its length is set to #MAX_EVENTS and it gets initialized with
	 * #H2Core::Event::Type::None in EventQueue().
	 */
	std::vector< std::unique_ptr<Event> >m_eventsBuffer;;

	/**
	 * Mutex to lock access to queue.
	 */
	std::mutex m_mutex;

	/** Whether or not to push log messages.*/
	bool m_bSilent;
};

inline bool EventQueue::getSilent() const {
	return m_bSilent;
}
inline void EventQueue::setSilent( bool bSilent ) {
	m_bSilent = bSilent;
}

};

#endif
