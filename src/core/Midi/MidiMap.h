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
#include <map>
#include <cassert>

#include <core/Helpers/Xml.h>
#include <core/Midi/MidiCommon.h>
#include <core/Object.h>

#include <QtCore/QMutex>

class Action;

namespace H2Core {

/** \ingroup docCore docMIDI */
class MidiMap : public H2Core::Object<MidiMap>
{
	H2_OBJECT(MidiMap)
public:
	MidiMap();
	~MidiMap();

	static std::shared_ptr<MidiMap> loadFrom( const H2Core::XMLNode& node,
											  bool bSilent = false );
	void saveTo( H2Core::XMLNode& node, bool bSilent = false ) const;
	
	void reset();  ///< Reinitializes the object.

	/** Sets up the relation between a mmc event and an action */
	void registerMMCEvent( const QString&, std::shared_ptr<Action> );
	/** Sets up the relation between a note event and an action */
	void registerNoteEvent( int , std::shared_ptr<Action> );
	/** Sets up the relation between a cc event and an action */
	void registerCCEvent( int , std::shared_ptr<Action> );
	/** Sets up the relation between a program change and an action */
	void registerPCEvent( std::shared_ptr<Action> );

	const std::multimap<QString, std::shared_ptr<Action>>& getMMCActionMap() const;
	const std::multimap<int, std::shared_ptr<Action>>& getNoteActionMap() const;
	const std::multimap<int, std::shared_ptr<Action>>& getCCActionMap() const;
	
	/** Returns all MMC actions which are linked to the given event. */
	std::vector<std::shared_ptr<Action>> getMMCActions( const QString& sEventString );
	/** Returns all note actions which are linked to the given event. */
	std::vector<std::shared_ptr<Action>> getNoteActions( int nNote );
	/** Returns the cc action which was linked to the given event. */
	std::vector<std::shared_ptr<Action>> getCCActions( int nParameter );
	/** Returns the pc action which was linked to the given event. */
	const std::vector<std::shared_ptr<Action>>& getPCActions() const;
		
	std::vector<int> findCCValuesByActionParam1( const QString& sActionType,
												 const QString& sParam1 );
	std::vector<int> findCCValuesByActionType( const QString& sActionType );

	/**
	 * @returns a list of all MIDI events registered to a particular
	 *   @a pAction grouped in MIDI event type name and MIDI event
	 *   parameter pairs.
	 */
	std::vector<std::pair<H2Core::MidiMessage::Event,int>> getRegisteredMidiEvents( std::shared_ptr<Action> pAction ) const;
	
	/** Formatted string version for debugging purposes.
	 * \param sPrefix String prefix which will be added in front of
	 * every new line
	 * \param bShort Instead of the whole content of all classes
	 * stored as members just a single unique identifier will be
	 * displayed without line breaks.
	 *
	 * \return String presentation of current object.*/
	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
private:

	std::multimap<int, std::shared_ptr<Action>> m_noteActionMap;
	std::multimap<int, std::shared_ptr<Action>> m_ccActionMap;
	std::multimap<QString, std::shared_ptr<Action>> m_mmcActionMap;
	std::vector<std::shared_ptr<Action>> m_pcActionVector;

	QMutex __mutex;
};

inline const std::multimap<QString, std::shared_ptr<Action>>& MidiMap::getMMCActionMap() const {
	return m_mmcActionMap;
}
inline const std::multimap<int, std::shared_ptr<Action>>& MidiMap::getNoteActionMap() const {
	return m_noteActionMap;
}
inline const std::multimap<int, std::shared_ptr<Action>>& MidiMap::getCCActionMap() const {
	return m_ccActionMap;
}
inline const std::vector<std::shared_ptr<Action>>& MidiMap::getPCActions() const {
	return m_pcActionVector;
}

};

#endif
