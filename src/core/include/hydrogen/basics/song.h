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

#ifndef SONG_H
#define SONG_H


#include <QString>
#include <QDomNode>
#include <vector>
#include <map>

#include <hydrogen/object.h>

class TiXmlNode;

namespace H2Core
{

class ADSR;
class Sample;
class Note;
class Instrument;
class InstrumentList;
class Pattern;
class Song;
class DrumkitComponent;
class PatternList;
class AutomationPath;

/**
\ingroup H2CORE
\brief	Song class
*/
class Song : public H2Core::Object
{
		H2_OBJECT
	public:
		enum SongMode {
			PATTERN_MODE,
			SONG_MODE
		};

		bool __is_muted;
		unsigned __resolution;		///< Resolution of the song (number of ticks per quarter)
		/**
		 * Current speed in beats per minutes.
		 *
		 * One of its purposes is an intermediate storage of the
		 * tempo at the current transport position in
		 * Hydrogen::setTimelineBpm() in order to detect local changes
		 * in speed (set by the user).
		 */
		float __bpm;

		QString __name;		///< song name
		QString __author;	///< author of the song

		static Song* get_empty_song();
		static Song* get_default_song();

		Song( const QString& name, const QString& author, float bpm, float volume );
		~Song();

		/**
		  Remove all the notes in the song that play on instrument I.
		  The function is real-time safe (it locks the audio data while deleting notes)
		*/
		void purge_instrument( Instrument* I );

		void set_volume( float volume )
		{
			__volume = volume;
		}
		float get_volume() const
		{
			return __volume;
		}

		void set_metronome_volume( float volume )
		{
			__metronome_volume = volume;
		}
		float get_metronome_volume() const
		{
			return __metronome_volume;
		}

		PatternList* get_pattern_list() const
		{
			return __pattern_list;
		}
		void set_pattern_list( PatternList* pattern_list )
		{
			__pattern_list = pattern_list;
		}

		/** Return a pointer to a vector storing all Pattern
		 * present in the Song.
		 * \return #__pattern_group_sequence */
		std::vector<PatternList*>* get_pattern_group_vector()
		{
			return __pattern_group_sequence;
		}
		/** Return a pointer to a vector storing all Pattern
		 * present in the Song.
		 * \return #__pattern_group_sequence */
		const std::vector<PatternList*>* get_pattern_group_vector() const
		{
			return __pattern_group_sequence;
		}

		/** Sets the vector storing all Pattern present in the
		 * Song #__pattern_group_sequence.
		 * \param vect Pointer to a vector containing all
		 *   Pattern of the Song.*/
		void set_pattern_group_vector( std::vector<PatternList*>* vect )
		{
			__pattern_group_sequence = vect;
		}

		static Song* 		load( const QString& sFilename );
		bool 			save( const QString& sFilename );

		InstrumentList*		get_instrument_list() const;
		void			set_instrument_list( InstrumentList* list );

		void			set_notes( const QString& notes );
		const QString&		get_notes();

		void			set_license( const QString& license );
		const QString&		get_license();
		const QString&		get_author();

		const QString&		get_filename();
		void			set_filename( const QString& filename );
							
		bool			is_loop_enabled() const;
		void			set_loop_enabled( bool enabled );
							
		float			get_humanize_time_value() const;
		void			set_humanize_time_value( float value );
							
		float			get_humanize_velocity_value() const;
		void			set_humanize_velocity_value( float value );
							
		float			get_swing_factor() const;
		void			set_swing_factor( float factor );
							
		SongMode		get_mode() const;
		void			set_mode( SongMode mode );
							
		void			set_is_modified(bool is_modified);
		bool			get_is_modified() const;

		std::vector<DrumkitComponent*>* get_components() const;

		AutomationPath *	get_velocity_automation_path() const;

		DrumkitComponent*	get_component( int ID );

		void			readTempPatternList( const QString& filename );
		bool			writeTempPatternList( const QString& filename );
							
		QString			copyInstrumentLineToString( int selectedPattern, int selectedInstrument );
		bool			pasteInstrumentLineFromString( const QString& serialized, int selectedPattern, int selectedInstrument, std::list<Pattern *>& patterns );
							
		int			get_latest_round_robin( float start_velocity );
		void			set_latest_round_robin( float start_velocity, int latest_round_robin );
		/** \return #__playback_track_filename */
		QString&		get_playback_track_filename();
		/** \param filename Sets #__playback_track_filename. */
		void			set_playback_track_filename( const QString filename );
							
		/** \return #__playback_track_enabled */
		bool			get_playback_track_enabled() const;
		/** Specifies whether a playback track should be used.
		 *
		 * If #__playback_track_filename is set to nullptr,
		 * #__playback_track_enabled will be set to false
		 * regardless of the choice in @a enabled.
		 *
		 * \param enabled Sets #__playback_track_enabled. */
		bool			set_playback_track_enabled( const bool enabled );
							
		/** \return #__playback_track_volume */
		float			get_playback_track_volume() const;
		/** \param volume Sets #__playback_track_volume. */
		void			set_playback_track_volume( const float volume );

		/** Song was incompletely loaded from file (missing samples)
		 */
		bool has_missing_samples();

	private:
		///< volume of the song (0.0..1.0)
		float			__volume;
		///< Metronome volume
		float			__metronome_volume;
		QString			__notes;
		///< Pattern list
		PatternList*		__pattern_list;
		///< Sequence of pattern groups
		std::vector<PatternList*>* __pattern_group_sequence;
		///< Instrument list
		InstrumentList*	       	__instrument_list;
		///< list of drumkit component
		std::vector<DrumkitComponent*>*	__components;				
		QString			__filename;
		bool			__is_loop_enabled;
		float			__humanize_time_value;
		float			__humanize_velocity_value;
		float			__swing_factor;
		bool			__is_modified;
		std::map< float, int> 	__latest_round_robins;
		SongMode		__song_mode;
		
		/** Name of the file to be loaded as playback track.
		 *
		 * It is set by set_playback_track_filename() and
		 * queried by get_playback_track_filename().
		 *
		 * The playback track itself is loaded in
		 * Sampler::reinitialize_playback_track().
		 */
		QString			__playback_track_filename;
		/** Whether the playback track should be used at all.
		 *
		 * It is set by set_playback_track_enabled() and
		 * queried by get_playback_track_enabled().
		 *
		 * The playback track itself is loaded in
		 * Sampler::reinitialize_playback_track().
		 */
		bool			__playback_track_enabled;
		/** Volume of the playback track.
		 *
		 * It is set by set_playback_track_volume() and
		 * queried by get_playback_track_volume().
		 *
		 * The playback track itself is loaded in
		 * Sampler::reinitialize_playback_track().
		 */
		float			__playback_track_volume;
		AutomationPath*		__velocity_automation_path;
		///< license of the song
		QString			__license;
};

inline bool Song::get_is_modified() const 
{
	return __is_modified;
}

inline InstrumentList* Song::get_instrument_list() const
{
	return __instrument_list;
}

inline void Song::set_instrument_list( InstrumentList* list )
{
	__instrument_list = list;
}

inline void Song::set_notes( const QString& notes )
{
	__notes = notes;
}

inline const QString& Song::get_notes()
{
	return __notes;
}

inline void Song::set_license( const QString& license )
{
	__license = license;
}

inline const QString& Song::get_license()
{
	return __license;
}

inline const QString& Song::get_author()
{
	return __author;
}

inline const QString& Song::get_filename()
{
	return __filename;
}

inline void Song::set_filename( const QString& filename )
{
	__filename = filename;
}

inline bool Song::is_loop_enabled() const
{
	return __is_loop_enabled;
}

inline void Song::set_loop_enabled( bool enabled )
{
	__is_loop_enabled = enabled;
}

inline float Song::get_humanize_time_value() const
{
	return __humanize_time_value;
}

inline void Song::set_humanize_time_value( float value )
{
	__humanize_time_value = value;
}

inline float Song::get_humanize_velocity_value() const
{
	return __humanize_velocity_value;
}

inline void Song::set_humanize_velocity_value( float value )
{
	__humanize_velocity_value = value;
}

inline float Song::get_swing_factor() const
{
	return __swing_factor;
}

inline Song::SongMode Song::get_mode() const
{
	return __song_mode;
}

inline void Song::set_mode( Song::SongMode mode )
{
	__song_mode = mode;
}

inline std::vector<DrumkitComponent*>* Song::get_components() const
{
	return __components;
}

inline AutomationPath * Song::get_velocity_automation_path() const
{
	return __velocity_automation_path;
}

inline int Song::get_latest_round_robin( float start_velocity )
{
	if ( __latest_round_robins.find(start_velocity) == __latest_round_robins.end() )
		return 0;
	else
		return __latest_round_robins[start_velocity];
}

inline void Song::set_latest_round_robin( float start_velocity, int latest_round_robin )
{
	__latest_round_robins[start_velocity] = latest_round_robin;
}

inline QString& Song::get_playback_track_filename()
{
	return __playback_track_filename;
}

inline void Song::set_playback_track_filename( const QString filename )
{
	__playback_track_filename = filename;
}

inline bool Song::get_playback_track_enabled() const
{
	return __playback_track_enabled;
}

inline bool Song::set_playback_track_enabled( const bool enabled )
{
	if ( __playback_track_filename == nullptr ) {
		return false;
	}
	__playback_track_enabled = enabled;
	return enabled;
}

inline float Song::get_playback_track_volume() const
{
	return __playback_track_volume;
}

inline void Song::set_playback_track_volume( const float volume )
{
	__playback_track_volume = volume;
}

/**
\ingroup H2CORE
\brief	Read XML file of a song
*/
class SongReader : public H2Core::Object
{
		H2_OBJECT
	public:
		SongReader();
		~SongReader();
		const QString getPath( const QString& filename );
		Song* readSong( const QString& filename );

	private:
		QString m_sSongVersion;

		/// Dato un XmlNode restituisce un oggetto Pattern
		Pattern* getPattern( QDomNode pattern, InstrumentList* instrList );
};

};

#endif
