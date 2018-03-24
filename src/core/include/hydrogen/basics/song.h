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
		float __bpm;			///< Beats per minute

		QString __name;		///< song name
		QString __author;	///< author of the song
		QString __license;	///< license of the song

		/*
		// internal delay FX
		bool m_bDelayFXEnabled;
		float m_fDelayFXWetLevel;
		float m_fDelayFXFeedback;
		unsigned m_nDelayFXTime;
		//~ internal delay fx
		*/

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
		float get_volume()
		{
			return __volume;
		}

		void set_metronome_volume( float volume )
		{
			__metronome_volume = volume;
		}
		float get_metronome_volume()
		{
			return __metronome_volume;
		}

		PatternList* get_pattern_list()
		{
			return __pattern_list;
		}
		void set_pattern_list( PatternList* pattern_list )
		{
			__pattern_list = pattern_list;
		}

		std::vector<PatternList*>* get_pattern_group_vector()
		{
			return __pattern_group_sequence;
		}

		const std::vector<PatternList*>* get_pattern_group_vector() const
		{
			return __pattern_group_sequence;
		}

		void set_pattern_group_vector( std::vector<PatternList*>* vect )
		{
			__pattern_group_sequence = vect;
		}

		static Song* load( const QString& sFilename );
		bool save( const QString& sFilename );

		InstrumentList* get_instrument_list()
		{
			return __instrument_list;
		}
		void set_instrument_list( InstrumentList* list )
		{
			__instrument_list = list;
		}


		void set_notes( const QString& notes )
		{
			__notes = notes;
		}
		const QString& get_notes()
		{
			return __notes;
		}

		void set_license( const QString& license )
		{
			__license = license;
		}
		const QString& get_license()
		{
			return __license;
		}

		const QString& get_author()
		{
			return __author;
		}

		const QString& get_filename()
		{
			return __filename;
		}
		void set_filename( const QString& filename )
		{
			__filename = filename;
		}

		bool is_loop_enabled()
		{
			return __is_loop_enabled;
		}
		void set_loop_enabled( bool enabled )
		{
			__is_loop_enabled = enabled;
		}

		float get_humanize_time_value()
		{
			return __humanize_time_value;
		}
		void set_humanize_time_value( float value )
		{
			__humanize_time_value = value;
		}

		float get_humanize_velocity_value()
		{
			return __humanize_velocity_value;
		}
		void set_humanize_velocity_value( float value )
		{
			__humanize_velocity_value = value;
		}

		float get_swing_factor()
		{
			return __swing_factor;
		}
		void set_swing_factor( float factor );

		SongMode get_mode()
		{
			return __song_mode;
		}
		void set_mode( SongMode mode )
		{
			__song_mode = mode;
		}

		void set_is_modified(bool is_modified);

		bool get_is_modified()
		{
			return __is_modified;
		}

		std::vector<DrumkitComponent*>* get_components()
		{
			return __components;
		}

		AutomationPath *get_velocity_automation_path() const
		{
			return __velocity_automation_path;
		}

		DrumkitComponent* get_component( int ID );

		void readTempPatternList( QString filename );

		int get_latest_round_robin( float start_velocity )
		{
			if ( __latest_round_robins.find(start_velocity) == __latest_round_robins.end() )
				return 0;
			else
				return __latest_round_robins[start_velocity];
		}
		void set_latest_round_robin( float start_velocity, int latest_round_robin )
		{
			__latest_round_robins[start_velocity] = latest_round_robin;
		}

		QString&	get_playback_track_filename()
		{
			return __playback_track_filename;
		}
		void	set_playback_track_filename( QString filename )
		{
			__playback_track_filename = filename;
		}

		bool	get_playback_track_enabled()
		{
			return __playback_track_enabled;
		}
		void	set_playback_track_enabled( bool enabled )
		{
			__playback_track_enabled = enabled;
		}

		float	get_playback_track_volume()
		{
			return __playback_track_volume;
		}
		void	set_playback_track_volume( float volume )
		{
			__playback_track_volume = volume;
		}


	private:
		float								__volume;					///< volume of the song (0.0..1.0)
		float								__metronome_volume;			///< Metronome volume
		QString								__notes;
		PatternList*						__pattern_list;				///< Pattern list
		std::vector<PatternList*>*			__pattern_group_sequence;	///< Sequence of pattern groups
		InstrumentList*						__instrument_list;			///< Instrument list
		std::vector<DrumkitComponent*>*		__components;				///< list of drumkit component
		QString								__filename;
		bool								__is_loop_enabled;
		float								__humanize_time_value;
		float								__humanize_velocity_value;
		float								__swing_factor;
		bool								__is_modified;
		std::map< float, int> 				__latest_round_robins;
		SongMode							__song_mode;
		QString								__playback_track_filename;
		bool								__playback_track_enabled;
		float								__playback_track_volume;
		AutomationPath*						__velocity_automation_path;
};



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
