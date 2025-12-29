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
#ifndef MIDI_EVENT_H
#define MIDI_EVENT_H

#include <QString>

#include <core/Midi/MidiAction.h>
#include <core/Object.h>

#include <memory>

namespace H2Core {

/**
 * This class represents an incoming MIDI message that Hydrogen will act upon
 * using a #MidiAction.
 *
 * \ingroup docCore docMIDI */
class MidiEvent : public H2Core::Object<MidiEvent> {
	H2_OBJECT( MidiEvent )
   public:
	/** Subset of incoming MIDI events that will be handled by
		Hydrogen. */
	enum class Type {
		Null,
		Note,
		CC,
		PC,
		MmcStop,
		MmcPlay,
		MmcPause,
		MmcDeferredPlay,
		MmcFastForward,
		MmcRewind,
		MmcRecordStrobe,
		MmcRecordExit,
		MmcRecordReady
	};
	static QString TypeToQString( const Type& Type );
	static Type QStringToType( const QString& sType );
	/** Retrieve the string representation for all available
	 * #Type. */
	static QStringList getAllTypes();

	MidiEvent();
	MidiEvent( const std::shared_ptr<MidiEvent> pOther );

	const Type& getType() const;
	void setType( const Type& type );

	int getParameter() const;
	void setParameter( int nValue );

	std::shared_ptr<MidiAction> getMidiAction() const;
	void setMidiAction( std::shared_ptr<MidiAction> pAction );

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
	Type m_type;
	int m_nParameter;
	std::shared_ptr<MidiAction> m_pMidiAction;
};

inline const MidiEvent::Type& MidiEvent::getType() const
{
	return m_type;
}

inline void MidiEvent::setType( const MidiEvent::Type& type )
{
	m_type = type;
}

inline int MidiEvent::getParameter() const
{
	return m_nParameter;
}

inline void MidiEvent::setParameter( int nValue )
{
	m_nParameter = nValue;
}

inline std::shared_ptr<MidiAction> MidiEvent::getMidiAction() const
{
	return m_pMidiAction;
}

inline void MidiEvent::setMidiAction( std::shared_ptr<MidiAction> pAction )
{
	m_pMidiAction = pAction;
}
}  // namespace H2Core

#endif
