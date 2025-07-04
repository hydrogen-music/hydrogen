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

#include <core/config.h>

#include <QFile>

#ifdef H2CORE_HAVE_QT6
  #include <QStringConverter>
#else
  #include <QTextCodec>
#endif

#include <core/Lilipond/Lilypond.h>
#include <core/Basics/Note.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Song.h>

/*
 * Header of LilyPond file
 * It contains the notation style (states the position of notes), and for this
 * it follows the "Guide to Standardized Drumset Notation" by Norman Weinberg.
 *
 * Note that the GM-kit uses two unconventional instruments: "Stick" and
 * "Hand Clap", so for those I did what I could and used the recommended
 * triangle notehead to distinguish them for drum and cymbal notation.
 */
static const char *sHeader =
        "\\version \"2.16.2\"\n" // Current version on Ubuntu LTS
        "\n"
        "#(define gmStyle\n"
        "    '(\n"
        "     (bassdrum       default   #f          -3) ; Kick\n"
        "     (lowoodblock    triangle  #f          0)  ; Stick\n"
        "     (snare          default   #f          1)  ; Snare\n"
        "     (maracas        triangle  #f          -3) ; Hand Clap\n"
        "     (highfloortom   default   #f          -1) ; Tom Low\n"
        "     (hihat          cross     #f          5)  ; Closed HH\n"
        "     (lowtom         default   #f          2)  ; Tom Mid\n"
        "     (pedalhihat     cross     #f          -5) ; Pedal HH\n"
        "     (hightom        default   #f          3)  ; Tom Hi\n"
        "     (openhihat      cross     \"open\"      5)  ; Open HH\n"
        "     (cowbell        triangle  #f          3)  ; Cowbell\n"
        "     (ridecymbal     cross     #f          4)  ; Main Ride\n"
        "     (crashcymbal    cross     #f          6)  ; Main Crash\n"
        "     (ridecymbala    cross     #f          4)  ; Additional Ride\n"
        "     (crashcymbala   cross     #f          7)  ; Additional Crash\n"
        "     ))\n"
        "\n";

H2Core::LilyPond::LilyPond() :
	m_fBPM( 0 )
{
}

void H2Core::LilyPond::extractData( const Song &song ) {
	// Retrieve metadata
	m_sName = song.getName();
	m_sAuthor = song.getAuthor();
	m_fBPM = song.getBpm();

	// Get the main information about the music
	const auto pGroup = song.getPatternGroupVector();
	if ( !pGroup || pGroup->size() == 0 ) {
		m_Measures.clear();
		return;
	}
	unsigned nSize = pGroup->size();
	m_Measures = std::vector<notes_t>( nSize );
	for ( unsigned nPatternList = 0; nPatternList < nSize; nPatternList++ ) {
		if ( auto pPatternList = ( *pGroup )[ nPatternList ] ) {
			addPatternList( *pPatternList, m_Measures[ nPatternList ] );
		}
	}
}

void H2Core::LilyPond::write( const QString &sFilename ) const {
	QFile file( sFilename );
	if ( ! file.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
		ERRORLOG( QString( "Unable to open file [%1] for writing" )
				  .arg( sFilename ) );
		return;
	}

	QTextStream stream( &file );
#ifdef H2CORE_HAVE_QT6
	stream.setEncoding( QStringConverter::Utf8 );
#else
	stream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
#endif

	stream << sHeader;
	stream << "\\header {\n";
	stream << "    title = \"" << m_sName << "\"\n";
	stream << "    composer = \"" << m_sAuthor << "\"\n";
	stream << "    tagline = \"Generated by Hydrogen " H2CORE_VERSION "\"\n";
	stream << "}\n\n";

	stream << "\\score {\n";
	stream << "    \\new DrumStaff <<\n";
	stream << "        \\set DrumStaff.drumStyleTable = #(alist->hash-table "
	        "gmStyle)\n";
	stream << "        \\override Staff.TimeSignature #'style = #'() % Display "
	        "4/4 signature\n";
	stream << "        \\set Staff.beamExceptions = #'()             % Beam "
	        "quavers two by two\n";
	stream << "        \\drummode {\n";
	stream << "            \\tempo 4 = " << static_cast<int>( m_fBPM ) << "\n\n";
	writeMeasures( stream );
	stream << "\n        }\n";
	stream << "    >>\n";
	stream << "}\n";

	file.close();
}

void H2Core::LilyPond::addPatternList( const PatternList &list, notes_t &to ) {
	to.clear();
	for ( unsigned nPattern = 0; nPattern < list.size(); nPattern++ ) {
		if ( const auto& pPattern = list.get( nPattern ) ) {
			addPattern( *pPattern, to );
		}
	}
}

void H2Core::LilyPond::addPattern( const Pattern &pattern, notes_t &notes ) {
	notes.reserve( pattern.getLength() );
	for ( unsigned nNote = 0; nNote < pattern.getLength(); nNote++ ) {
		if ( nNote >= notes.size() ) {
			notes.push_back( std::vector<std::pair<int, float> >() );
		}

		const Pattern::notes_t *pPatternNotes = pattern.getNotes();
		if ( !pPatternNotes ) {
			continue;
		}
		FOREACH_NOTE_CST_IT_BOUND_LENGTH( pPatternNotes, it, nNote, &pattern ) {
			if ( auto pNote = it->second ) {
				int nId = pNote->getInstrumentId();
				float fVelocity = pNote->getVelocity();
				notes[ nNote ].push_back( std::make_pair( nId, fVelocity ) );
			}
		}
	}
}

void H2Core::LilyPond::writeMeasures( QTextStream &stream ) const {
	unsigned nSignature = 0; ///< Numerator of the time signature
	for ( unsigned nMeasure = 0; nMeasure < m_Measures.size(); nMeasure++ ) {
		// Start a new measure
		stream << "\n            % Measure " << nMeasure + 1 << "\n";
		unsigned nNewSignature = m_Measures[ nMeasure ].size() /
			H2Core::nTicksPerQuarter;
		if ( nSignature != nNewSignature ) { // Display time signature change
			nSignature = nNewSignature;
			stream << "            \\time " << nSignature << "/4\n";
		}

		// Display the notes
		stream << "            << {\n";
		writeUpper( stream, nMeasure );
		stream << "            } \\\\ {\n";
		writeLower( stream, nMeasure );
		stream << "            } >>\n";
	}
}

void H2Core::LilyPond::writeUpper( QTextStream &stream,
                                   unsigned nMeasure ) const {
	// On the upper voice, we want only cymbals and mid and high toms
	std::vector<int> whiteList;
	whiteList.push_back( 6 );  // Closed HH
	whiteList.push_back( 7 );  // Tom Mid
	whiteList.push_back( 9 );  // Tom Hi
	whiteList.push_back( 10 ); // Open HH
	whiteList.push_back( 11 ); // Cowbell
	whiteList.push_back( 12 ); // Ride Jazz
	whiteList.push_back( 13 ); // Crash
	whiteList.push_back( 14 ); // Ride Rock
	whiteList.push_back( 15 ); // Crash Jazz
	writeVoice( stream, nMeasure, whiteList );
}

void H2Core::LilyPond::writeLower( QTextStream &stream,
                                   unsigned nMeasure ) const {
	std::vector<int> whiteList;
	whiteList.push_back( 0 ); // Kick
	whiteList.push_back( 1 ); // Stick
	whiteList.push_back( 2 ); // Snare Jazz
	whiteList.push_back( 3 ); // Hand Clap
	whiteList.push_back( 4 ); // Snare Jazz
	whiteList.push_back( 5 ); // Tom Low
	whiteList.push_back( 8 ); // Pedal HH
	writeVoice( stream, nMeasure, whiteList );
}

///< Mapping of GM-kit instrument to LilyPond names
static const char *const sNames[] = { "bd",   "wbl",   "sn",    "mar",
	                                  "sn",   "tomfh", "hh",    "toml",
	                                  "hhp",  "tomh",  "hho",   "cb",
	                                  "cymr", "cymc",  "cymra", "cymca" };

///< Write group of note (may also be a rest or a single note)
static void writeNote( QTextStream &stream, const std::vector<int> &notes ) {
	switch ( notes.size() ) {
	case 0: stream << "r"; break;
	case 1: stream << sNames[ notes[ 0 ] ]; break;
	default:
		stream << "<";
		for ( unsigned i = 0; i < notes.size(); i++ ) {
			stream << sNames[ notes[ i ] ] << " ";
		}
		stream << ">";
	}
}

///< Write duration in LilyPond format, from number of 1/48th of a beat
static void writeDuration( QTextStream &stream, unsigned duration ) {
	if ( H2Core::nTicksPerQuarter % duration == 0 ) {
		// This is a basic note
		if ( duration % 2 ) {
			return; // TODO Triplet, unsupported yet
		}
		stream << 4 * H2Core::nTicksPerQuarter / duration;

	} else if ( duration % 3 == 0 &&
				H2Core::nTicksPerQuarter % ( duration * 2 / 3 ) == 0 ) {
		// This is a dotted note
		if ( duration % 2 ) {
			return; // TODO Triplet, unsupported yet
		}
		stream << 4 * H2Core::nTicksPerQuarter / ( duration * 2 / 3 ) << ".";

	} else {
		// Neither basic nor dotted, we have to split it and add a rest
		for ( int pow = 3; pow >= 0; --pow ) {
			if ( 3 * ( 1 << pow ) < duration ) {
				stream << 8 * ( 3 - pow ) << " r";
				writeDuration( stream, duration - 3 * ( 1 << pow ) );
				break;
			}
		}
	}
}

void H2Core::LilyPond::writeVoice( QTextStream &stream,
                                   unsigned nMeasure,
                                   const std::vector<int> &whiteList ) const {
	stream << "                ";
	const notes_t &measure = m_Measures[ nMeasure ];
	for ( unsigned nStart = 0; nStart < measure.size();
		  nStart += H2Core::nTicksPerQuarter ) {
		unsigned lastNote = nStart;
		for ( unsigned nTime = nStart; nTime < nStart + H2Core::nTicksPerQuarter;
			  nTime++ ) {
			// Get notes played at this current time
			std::vector<int> notes;
			const std::vector<std::pair<int, float> > &input = measure[ nTime ];
			for ( unsigned nNote = 0; nNote < input.size(); nNote++ ) {
				if ( std::find( whiteList.begin(),
				                whiteList.end(),
				                input[ nNote ].first ) != whiteList.end() ) {
					notes.push_back( input[ nNote ].first );
				}
			}

			// Write them if there are any
			if ( !notes.empty() || nTime == nStart ) {
				// First write duration of last note
				if ( nTime != nStart ) {
					writeDuration( stream, nTime - lastNote );
					lastNote = nTime;
				}

				// Then write next note
				stream << " ";
				writeNote( stream, notes );
			}
		}
		writeDuration( stream, nStart + H2Core::nTicksPerQuarter - lastNote );
	}
	stream << "\n";
}
