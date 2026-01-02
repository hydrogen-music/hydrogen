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

#ifndef MIDI_H
#define MIDI_H

#include <algorithm>

namespace H2Core {
/** \ingroup docCore docMIDI */
namespace Midi {
    enum class Channel : int {};
	static constexpr Channel ChannelMinimum = Channel( 0 );
	static constexpr Channel ChannelDefault = Channel( 9 );
	static constexpr Channel ChannelMaximum = Channel( 15 );
	static constexpr Channel ChannelOff = Channel( -1 );
	static constexpr Channel ChannelAll = Channel( -2 );
	static constexpr Channel ChannelInvalid = Channel( -3 );
	static Channel channelFromInt( int nChannel )
	{
		if ( nChannel >= static_cast<int>( ChannelMinimum ) &&
			 nChannel <= static_cast<int>( ChannelMaximum ) ) {
			return static_cast<Channel>( nChannel );
		}
		else if ( nChannel == static_cast<int>( ChannelOff ) ) {
			return ChannelOff;
		}
		else if ( nChannel == static_cast<int>( ChannelAll ) ) {
			return ChannelAll;
		}
		else {
			return ChannelInvalid;
		}
	}
	static Channel channelFromIntClamp( int nChannel )
	{
		return static_cast<Channel>( std::clamp(
			nChannel, static_cast<int>( ChannelMinimum ),
			static_cast<int>( ChannelMaximum )
		) );
	}

	enum class Note : int {};
	static constexpr Note NoteMinimum = Note( 0 );
	static constexpr Note NoteDefault = Note( 36 );
	static constexpr Note NoteMaximum = Note( 127 );
	static constexpr Note NoteInvalid = Note( -1 );
	/** When recording notes using MIDI NOTE_ON events this offset will be
	 * applied to the provided pitch in order to map it to an instrument
	 * number in the current drmmkit. It corresponds to the electric bass
	 * drum in the General MIDI notation. */
	static constexpr Note NoteOffset = Note( 36 );
	static Note noteFromInt( int nNote )
	{
		if ( nNote >= static_cast<int>( NoteMinimum ) &&
			 nNote <= static_cast<int>( NoteMaximum ) ) {
			return static_cast<Note>( nNote );
		}
		else {
			return NoteInvalid;
		}
	}
	static Note noteFromIntClamp( int nNote )
	{
		return static_cast<Note>( std::clamp(
			nNote, static_cast<int>( NoteMinimum ),
			static_cast<int>( NoteMaximum )
		) );
	}
};	// namespace Midi
};	// namespace H2Core

#endif
