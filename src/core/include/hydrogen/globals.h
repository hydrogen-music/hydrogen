/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef H2C_GLOBALS_H
#define H2C_GLOBALS_H

#define MAX_INSTRUMENTS         1000
#define MAX_COMPONENTS          32

#define MAX_NOTES               192

#define MAX_LAYERS              16

#define MAX_FX		        4

#define MAX_BUFFER_SIZE         8192

#define MIDI_OUT_NOTE_MIN       0
#define MIDI_OUT_NOTE_MAX       127
#define MIDI_OUT_CHANNEL_MIN    -1
#define MIDI_OUT_CHANNEL_MAX    15

#define SAMPLE_CHANNELS         2

#define TWOPI                   6.28318530717958647692

#define UNUSED( v )             (v = v)

// m_nBeatCounter
//100,000 ms in 1 second.
#define							US_DIVIDER .000001
// ~m_nBeatCounter


#endif // H2C_GLOBALS_H
