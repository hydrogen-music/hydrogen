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
#ifndef MIDIMAP_H
#define MIDIMAP_H

#include <memory>
#include <vector>

#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEvent.h>
#include <core/Object.h>

#include <QtCore/QMutex>

class MidiAction;

namespace H2Core {

class XMLNode;

/** \ingroup docCore docMIDI */
class MidiEventMap : public H2Core::Object<MidiEventMap>
{
	H2_OBJECT(MidiEventMap)
public:
	MidiEventMap();
	~MidiEventMap();

	static std::shared_ptr<MidiEventMap> loadFrom( const H2Core::XMLNode& node,
											  bool bSilent = false );
	void saveTo( H2Core::XMLNode& node, bool bSilent = false ) const;
	
	void reset();  ///< Reinitializes the object.

	void
	registerEvent( const MidiEvent::Type&, int nParameter, std::shared_ptr<MidiAction> );

	const std::vector<std::shared_ptr<MidiEvent>>& getMidiEvents() const;

	/** Returns all MMC actions which are linked to the given event. */
	std::vector<std::shared_ptr<MidiAction>> getMMCActions( const QString& sEventString );
	/** Returns all note actions which are linked to the given event. */
	std::vector<std::shared_ptr<MidiAction>> getNoteActions( int nNote );
	/** Returns the cc Midiaction which was linked to the given event. */
	std::vector<std::shared_ptr<MidiAction>> getCCActions( int nParameter );
	/** Returns the pc Midiaction which was linked to the given event. */
	std::vector<std::shared_ptr<MidiAction>> getPCActions();
		
	std::vector<int> findCCValuesByTypeAndParam1( MidiAction::Type type,
												 const QString& sParam1 );
	std::vector<int> findCCValuesByType( MidiAction::Type type );

	/**
	 * @returns a list of all MIDI events registered to a particular
	 *   @a pAction grouped in MIDI event type name and MIDI event
	 *   parameter pairs.
	 */
	std::vector<std::pair<H2Core::MidiEvent::Type, int>>
	getRegisteredMidiEvents( std::shared_ptr<MidiAction> pAction ) const;

	void removeRegisteredMidiEvents( std::shared_ptr<MidiAction> pAction );

	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true )
		const override;

   private:
	std::vector<std::shared_ptr<MidiEvent>> m_events;

	QMutex __mutex;
};

inline const std::vector<std::shared_ptr<MidiEvent>>&
MidiEventMap::getMidiEvents() const
{
	return m_events;
}
};	// namespace H2Core

#endif
