/*
 * Hydrogen
 * Copyright(c) 2015 by Sacha Delanoue
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

#ifndef LILYPOND_H
#define LILYPOND_H

#include <QString>

#include <utility>
#include <vector>

namespace H2Core {

class Pattern;
class PatternList;
class Song;

/// A class to convert a Hydrogen song to LilyPond format
class LilyPond {
public:
	LilyPond();

	/*
	 * Retreive all needed data from an Hydrogen song
	 * @param song the Hydrogen song to convert
	 */
	void extractData( const Song &song );

	/*
	 * Write the LilyPond format into a file
	 * @param sFilename name of output file
	 */
	void write( const QString &sFilename ) const;

private:
	/*
	 * This structure represents the notes in a measure.
	 * A measure is a vector containing the notes in it.
	 * The index in the main vector is 1/48th of a beat.
	 * An element in the main vector is the list of notes at this moment.
	 * A note is represented by its intrument and its velocity.
	 */
	typedef std::vector<std::vector<std::pair<int, float> > > notes_t;

	/*
	 * Retreive the information in a PatternList
	 * @param list  the PatternList where the information is
	 * @param notes where to store the information to
	 */
	static void addPatternList( const PatternList &list, notes_t &notes );

	/*
	 * Retreive the information in a Pattern
	 * @param pattern the Pattern where the information is
	 * @param notes   where to store the information to
	 */
	static void addPattern( const Pattern &pattern, notes_t &notes );

	std::vector<notes_t> m_measures; ///< Representation of the song
	QString m_sName;                 ///< Name of the song
	QString m_sAuthor;               ///< Author of the song
	float m_fBPM;                    ///< BPM of the song
};
}

#endif // LILYPOND_H
