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

#ifndef LILYPOND_H
#define LILYPOND_H

#include <QString>
#include <QTextStream>

#include <utility>
#include <vector>

#include <core/Basics/Instrument.h>
#include <core/Object.h>

namespace H2Core {

class Pattern;
class PatternList;
class Song;

/// A class to convert a Hydrogen song to LilyPond format
/** \ingroup docCore*/
class LilyPond : public H2Core::Object<LilyPond>{
	H2_OBJECT(LilyPond)
public:
	LilyPond();

	/*
	 * Retrieve all needed data from an Hydrogen song
	 * @param song the Hydrogen song to convert
	 */
	void extractData( const Song &song );

	/*
	 * Write the LilyPond format into a file
	 * @param sFileName name of output file
	 */
	void write( const QString &sFileName ) const;

private:
	/*
	 * This structure represents the notes in a measure. A measure is a vector
	 * containing the notes in it. The index in the main vector is
	 * 1/H2Core::nTicksPerQuarter-th of a beat. An element in the main vector is
	 * the list of notes at this moment. A note is represented by its instrument
	 * and its velocity.
	 */
	typedef std::vector<std::vector<std::pair<Instrument::Id, float> > > notes_t;

	/*
	 * Retrieve the information in a PatternList
	 * @param list  the PatternList where the information is
	 * @param notes where to store the information to
	 */
	static void addPatternList( const PatternList &list, notes_t &notes );

	/*
	 * Retrieve the information in a Pattern
	 * @param pattern the Pattern where the information is
	 * @param notes   where to store the information to
	 */
	static void addPattern( const Pattern &pattern, notes_t &notes );

	/// Write measures in LilyPond format to stream
	void writeMeasures( QTextStream &stream ) const;

	/// Write upper voice of given measure to stream
	void writeUpper( QTextStream &stream, unsigned nMeasure ) const;

	/// Write lower voice of given measure to stream
	void writeLower( QTextStream &stream, unsigned nMeasure ) const;

	/*
	 * Write voice of given measure to stream, ignore certain notes
	 * @param stream    the stream to write to
	 * @param nMeasure  the measure to write
	 * @param whiteList the list of notes to consider, the other are ignored
	 */
	void writeVoice( QTextStream &stream,
	                 unsigned nMeasure,
	                 const std::vector<Instrument::Id> &whiteList ) const;

	std::vector<notes_t>	m_Measures;		///< Representation of the song
	QString					m_sName;		///< Name of the song
	QString					m_sAuthor;		///< Author of the song
	float					m_fBPM;			///< BPM of the song
};	
}

#endif // LILYPOND_H
