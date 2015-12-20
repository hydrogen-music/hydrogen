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

#include <hydrogen/lilypond/lilypond.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/pattern.h>

H2Core::LilyPond::LilyPond() {
}

void H2Core::LilyPond::extractData( const Song &song ) {
	// Retreive metadata
	m_sName = song.__name;
	m_sAuthor = song.__author;
	m_fBPM = song.__bpm;

	// Get the main information about the music
	const std::vector<PatternList *> *group = song.get_pattern_group_vector();
	if ( !group ) {
		m_measures.clear();
		return;
	}
	unsigned nSize = group->size();
	m_measures = std::vector<notes_t>( nSize );
	for ( unsigned nPatternList = 0; nPatternList < nSize; nPatternList++ ) {
		if ( PatternList *pPatternList = ( *group )[ nPatternList ] ) {
			addPatternList( *pPatternList, m_measures[ nPatternList ] );
		}
	}
}

void H2Core::LilyPond::write( const QString &sFilename ) const {
	( void )sFilename;
}

void H2Core::LilyPond::addPatternList( const PatternList &list, notes_t &to ) {
	to.clear();
	for ( unsigned nPattern = 0; nPattern < list.size(); nPattern++ ) {
		if ( const Pattern *pPattern = list.get( nPattern ) ) {
			addPattern( *pPattern, to );
		}
	}
}

void H2Core::LilyPond::addPattern( const Pattern &pattern, notes_t &notes ) {
	notes.reserve( pattern.get_length() );
	for ( unsigned nNote = 0; nNote < pattern.get_length(); nNote++ ) {
		if ( nNote >= notes.size() ) {
			notes.push_back( std::vector<std::pair<int, float> >() );
		}

		const Pattern::notes_t *pPatternNotes = pattern.get_notes();
		if ( !pPatternNotes ) {
			continue;
		}
		FOREACH_NOTE_CST_IT_BOUND( pPatternNotes, it, nNote ) {
			if ( Note *pNote = it->second ) {
				int nId = pNote->get_instrument_id();
				float fVelocity = pNote->get_velocity();
				notes[ nNote ].push_back( std::make_pair( nId, fVelocity ) );
			}
		}
	}
}
