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

#include <QString>
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

		MidiBaseDriver();
		virtual ~MidiBaseDriver();

		virtual void close() = 0;
		virtual std::vector<QString> getExternalPortList( const PortType& portType ) = 0;
		virtual void open() = 0;

		virtual QString toQString( const QString& sPrefix = "",
								   bool bShort = true ) const override {
			return "";
		}
};

};

#endif
