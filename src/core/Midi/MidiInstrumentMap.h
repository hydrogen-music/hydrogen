/*
 * Hydrogen
 * Copyright(c) 2023-2023 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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

#ifndef MIDI_INSTRUMENT_MAP_H
#define MIDI_INSTRUMENT_MAP_H

#include <QString>
#include <map>
#include <memory>

#include <core/Object.h>

namespace H2Core {

class Drumkit;
class Instrument;
class XMLNode;

class MidiInstrumentMap : public H2Core::Object<MidiInstrumentMap> {
	H2_OBJECT(MidiInstrumentMap)

public:
	/** Specifies which incoming MIDI notes will be associated with which
     * instrument of the current drumkit. */
	enum class Input {
		/** No mapping of incoming MIDI notes is done.
		 *
		 * Available since 2.0. */
		None = 0,
		/** Options configured for outgoing MIDI note and channel apply for the
		 * input as well.
		 *
		 * This is one of the classic pre-2.0 options were the input note and
		 * channel could not be customized directly. Left for compatibility. */
		AsOutput = 1,
		/** Note and channel mappings can be set to arbitrary values for all
		 * instruments.
		 *
		 * Available since 2.0. */
		Custom = 2,
		/** All incoming MIDI events will be mapped to the currently selected
		 * instrument and different note values will result in different pitchs.
		 *
		 * Pre-2.0 option. */
		SelectedInstrument = 3,
		/** Incoming notes will be mapped to instruments based on their order in
		 * the current drumkit.
		 *
		 * This is one of the classic pre-2.0 options were the input note and
		 * channel could not be customized directly. Left for compatibility. */
		Order = 4
	};
	static QString InputToQString( Input mapping );

	/** Specifies which instrument of the current drumkit will send which
	 * outgoing MIDI note. */
	enum class Output {
		/** No outgoing MIDI notes will be send.
		 *
		 * Available since 2.0. */
		None = 0,
		/** The value set does apply to a C2-pitched note of the corresponding
		 * instrument. For notes with higher or lower pitch, the resulting
		 * MIDI event will have an offset with the same difference.
		 *
		 * Pre-2.0 option. */
		Offset = 1,
		/** All send MIDI event - regardless of the (pattern) notes' individual
		 * pitch - will have the same note and channel values.
		 *
		 * Available since 2.0. */
		Constant = 2
	};
	static QString OutputToQString( Output mapping );

	/** Part fo the mapping uniquely identifying a NOTE_ON or NOTE_OFF MIDI
     * event. */
	struct NoteRef {
		NoteRef() : nNote( -1 ), nChannel( -1 ){};
		int nNote;
		int nChannel;

		bool isNull() const { return nNote == -1 && nChannel == -1; };

		QString toQString( const QString& sPrefix, bool bShort ) const;
	};

	MidiInstrumentMap();
	MidiInstrumentMap( std::shared_ptr<MidiInstrumentMap> pOther );
	~MidiInstrumentMap();

	void saveTo( XMLNode& node ) const;
	static std::shared_ptr<MidiInstrumentMap> loadFrom( const XMLNode& node, bool bSilent = false );

	std::shared_ptr<Instrument> mapInstrument( int nNote, int nChannel,
											  std::shared_ptr<Drumkit> pDrumkit ) const;
	NoteRef getMapping( std::shared_ptr<Instrument> pInstrument ) const;

	Input getInput() const;
	void setInput( Input mapping );
	Output getOutput() const;
	void setOutput( Output mapping );
	bool getUseGlobalInputChannel() const;
	void setUseGlobalInputChannel( bool bUse );
	int getGlobalInputChannel() const;
	void setGlobalInputChannel( int nChannel );
	bool getUseGlobalOutputChannel() const;
	void setUseGlobalOutputChannel( bool bUse );
	int getGlobalOutputChannel() const;
	void setGlobalOutputChannel( int nChannel );

	const std::map<QString, NoteRef>& getCustomInputMappingsType() const;
	const std::map<int, NoteRef>& getCustomInputMappingsId() const;

	/** Note that the current design of MidiInstrumentMap does not feature a way to
     * delete a custom input mapping since there is no UX counterpart either.
     * The idea is to allow the user to add further mappings for the current kit
     * while still keeping those for the previous one. After all, the number of
     * individual instrument types and ids is rather small and we should not be
     * at risk for this object to grow too large. */
	void insertCustomInputMapping( std::shared_ptr<Instrument> pInstrument,
								  int nNote, int nChannel );

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;

private:

		Input m_input;
		Output m_output;
		bool m_bUseGlobalInputChannel;
		int m_nGlobalInputChannel;
		bool m_bUseGlobalOutputChannel;
		int m_nGlobalOutputChannel;

	/** Instrument type-based note mapping. This one takes precedeence over the
     * id-based one. */
	std::map<QString, NoteRef> m_customInputMappingsType;
	/** Mapping for typeless instruments - new ones or those of custom legacy
     * kits. */
	std::map<int, NoteRef> m_customInputMappingsId;

};

inline MidiInstrumentMap::Input MidiInstrumentMap::getInput() const {
	return m_input;
}
inline void MidiInstrumentMap::setInput( MidiInstrumentMap::Input mapping ) {
	m_input = mapping;
}
inline MidiInstrumentMap::Output MidiInstrumentMap::getOutput() const {
	return m_output;
}
inline void MidiInstrumentMap::setOutput( MidiInstrumentMap::Output mapping ) {
	m_output = mapping;
}
inline bool MidiInstrumentMap::getUseGlobalInputChannel() const {
	return m_bUseGlobalInputChannel;
}
inline void MidiInstrumentMap::setUseGlobalInputChannel( bool bUse ){
	m_bUseGlobalInputChannel = bUse;
}
inline int MidiInstrumentMap::getGlobalInputChannel() const {
	return m_nGlobalInputChannel;
}
inline bool MidiInstrumentMap::getUseGlobalOutputChannel() const {
	return m_bUseGlobalOutputChannel;
}
inline void MidiInstrumentMap::setUseGlobalOutputChannel( bool bUse ){
	m_bUseGlobalOutputChannel = bUse;
}
inline int MidiInstrumentMap::getGlobalOutputChannel() const {
	return m_nGlobalOutputChannel;
}
inline const std::map<QString, MidiInstrumentMap::NoteRef>& MidiInstrumentMap::getCustomInputMappingsType() const {
	return m_customInputMappingsType;
}
inline const std::map<int, MidiInstrumentMap::NoteRef>& MidiInstrumentMap::getCustomInputMappingsId() const {
	return m_customInputMappingsId;
}
};

#endif
