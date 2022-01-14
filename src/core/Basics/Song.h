/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#ifndef SONG_H
#define SONG_H


#include <QString>
#include <QDomNode>
#include <vector>
#include <map>
#include <memory>

#include <core/Object.h>

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
class Timeline;

/**
\ingroup H2CORE
\brief	Read XML file of a song
*/
/** \ingroup docCore*/
class SongReader : public H2Core::Object<SongReader>
{
		H2_OBJECT(SongReader)
	public:
		SongReader();
		~SongReader();
		const QString getPath( const QString& filename ) const;
		std::shared_ptr<Song> readSong( const QString& filename );

	private:
		QString m_sSongVersion;

		/// Dato un XmlNode restituisce un oggetto Pattern
		Pattern* getPattern( QDomNode pattern, InstrumentList* instrList );
};

/**
\ingroup H2CORE
\brief	Song class
*/
/** \ingroup docCore docDataStructure */
class Song : public H2Core::Object<Song>, public std::enable_shared_from_this<Song>
{
		H2_OBJECT(Song)
	public:
		enum class Mode {
			Pattern = 0,
			Song = 1
		};

		Song( const QString& sName, const QString& sAuthor, float fBpm, float fVolume );
		~Song();

		static std::shared_ptr<Song> getEmptySong();
		static std::shared_ptr<Song> getDefaultSong();

	bool getIsTimelineActivated() const;
	void setIsTimelineActivated( bool bIsTimelineActivated );

		bool getIsMuted() const;
		void setIsMuted( bool bIsMuted );

		unsigned getResolution() const;
		void setResolution( unsigned resolution );

		float getBpm() const;
		void setBpm( float fBpm );

		const QString& getName() const;
		void setName( const QString& sName );
		
		void setVolume( float fVolume );
		float getVolume() const;

		void setMetronomeVolume( float fVolume );
		float getMetronomeVolume() const;

		PatternList* getPatternList() const;
		void setPatternList( PatternList* pList );

		/** Return a pointer to a vector storing all Pattern
		 * present in the Song.
		 * \return #m_pPatternGroupSequence */
		std::vector<PatternList*>* getPatternGroupVector();
		/** Return a pointer to a vector storing all Pattern
		 * present in the Song.
		 * \return #m_pPatternGroupSequence */
		const std::vector<PatternList*>* getPatternGroupVector() const;

		/** Sets the vector storing all Pattern present in the
		 * Song #m_pPatternGroupSequence.
		 * \param pGroupVect Pointer to a vector containing all
		 *   Pattern of the Song.*/
		void setPatternGroupVector( std::vector<PatternList*>* pGroupVect );

		/** get the length of the song, in tick units */
		int lengthInTicks() const;

		static std::shared_ptr<Song> 	load( const QString& sFilename );
		bool 			save( const QString& sFilename );

		/**
		  Remove all the notes in the song that play on instrument I.
		  The function is real-time safe (it locks the audio data while deleting notes)
		*/
		void purgeInstrument( std::shared_ptr<Instrument> pInstr );


		InstrumentList*		getInstrumentList() const;
		void			setInstrumentList( InstrumentList* pList );

		void			setNotes( const QString& sNotes );
		const QString&		getNotes() const;

		void			setLicense( const QString& sLicense );
		const QString&		getLicense() const;

		void			setAuthor( const QString& sAuthor );
		const QString&		getAuthor() const;

		const QString&		getFilename() const;
		void			setFilename( const QString& sFilename );
							
		bool			getIsLoopEnabled() const;
		void			setIsLoopEnabled( bool bEnabled );
							
		float			getHumanizeTimeValue() const;
		void			setHumanizeTimeValue( float fValue );
							
		float			getHumanizeVelocityValue() const;
		void			setHumanizeVelocityValue( float fValue );
							
		float			getSwingFactor() const;
		void			setSwingFactor( float fFactor );

		Mode			getMode() const;
		void			setMode( Mode mode );
							
		bool			getIsModified() const;
		void			setIsModified( bool bIsModified);

	std::vector<DrumkitComponent*>* getComponents() const;

		AutomationPath *	getVelocityAutomationPath() const;

		DrumkitComponent*	getComponent( int nID ) const;

		void			readTempPatternList( const QString& sFilename );
		bool			writeTempPatternList( const QString& sFilename );
							
		QString			copyInstrumentLineToString( int nSelectedPattern, int selectedInstrument );
		bool			pasteInstrumentLineFromString( const QString& sSerialized, int nSelectedPattern, int nSelectedInstrument, std::list<Pattern *>& pPatterns );
							
		int			getLatestRoundRobin( float fStartVelocity );
		void			setLatestRoundRobin( float fStartVelocity, int nLatestRoundRobin );
		/** \return #m_sPlaybackTrackFilename */
		const QString&		getPlaybackTrackFilename() const;
		/** \param sFilename Sets #m_sPlaybackTrackFilename. */
		void			setPlaybackTrackFilename( const QString sFilename );
							
		/** \return #m_bPlaybackTrackEnabled */
		bool			getPlaybackTrackEnabled() const;
		/** Specifies whether a playback track should be used.
		 *
		 * If #m_sPlaybackTrackFilename is set to nullptr,
		 * #m_bPlaybackTrackEnabled will be set to false
		 * regardless of the choice in @a bEnabled.
		 *
		 * \param bEnabled Sets #m_bPlaybackTrackEnabled. */
		bool			setPlaybackTrackEnabled( const bool bEnabled );
							
		/** \return #m_fPlaybackTrackVolume */
		float			getPlaybackTrackVolume() const;
		/** \param fVolume Sets #m_fPlaybackTrackVolume. */
		void			setPlaybackTrackVolume( const float fVolume );

		/** Defines the type of user interaction experienced in the 
			SongEditor.*/
		enum class ActionMode {
			/** Holding a pressed left mouse key will draw a rectangle to
				select a group of Notes.*/
			selectMode = 0,
			/** Holding a pressed left mouse key will draw/delete patterns
				in all grid cells encountered.*/
			drawMode = 1
		};
		ActionMode		getActionMode() const;
		void			setActionMode( const ActionMode actionMode );

		/** Song was incompletely loaded from file (missing samples)
		 */
		bool hasMissingSamples() const;
		void clearMissingSamples();
		
		void setPanLawType( int nPanLawType );
		int getPanLawType() const;
		void setPanLawKNorm( float fKNorm );
		float getPanLawKNorm() const;

		bool isPatternActive( int nColumn, int nRow ) const;

	std::shared_ptr<Timeline> getTimeline() const;
	
		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix, bool bShort = true ) const override;
	
	friend std::shared_ptr<Song> SongReader::readSong( const QString& filename );
	
	private:

	/** Whether the Timeline button was pressed in the GUI or it was
		activated via an OSC command. */
	bool m_bIsTimelineActivated;
							
		bool m_bIsMuted;
		///< Resolution of the song (number of ticks per quarter)
		unsigned m_resolution;
		/**
		 * Current speed in beats per minutes.
		 *
		 * See TransportInfo::m_fBpm for how the handling of the
		 * different tempo instances work.
		 */
		float m_fBpm;
		
		///< song name
		QString m_sName;
		///< author of the song
		QString m_sAuthor;
		
		///< volume of the song (0.0..1.0)
		float	m_fVolume;
		///< Metronome volume
		float	m_fMetronomeVolume;
		QString			m_sNotes;
		///< Pattern list
		PatternList*	m_pPatternList;
		///< Sequence of pattern groups
		std::vector<PatternList*>* m_pPatternGroupSequence;
		///< Instrument list
		InstrumentList*	       	m_pInstrumentList;
		///< list of drumkit component
		std::vector<DrumkitComponent*>*	m_pComponents;				
		QString			m_sFilename;
		bool			m_bIsLoopEnabled;
		float			m_fHumanizeTimeValue;
		float			m_fHumanizeVelocityValue;
		float			m_fSwingFactor;
		bool			m_bIsModified;
		std::map< float, int> 	m_latestRoundRobins;
		Mode			m_mode;
		
		/** Name of the file to be loaded as playback track.
		*
		 * It is set by setPlaybackTrackFilename() and
		 * queried by getPlaybackTrackFilename().
		 *
		 * The playback track itself is loaded in
		 * Sampler::reinitialize_playback_track().
		 */
		QString			m_sPlaybackTrackFilename;
		/** Whether the playback track should be used at all.
		 *
		 * It is set by setPlaybackTrackEnabled() and
		 * queried by getPlaybackTrackEnabled().
		 *
		 * The playback track itself is loaded in
		 * Sampler::reinitialize_playback_track().
		 */
		bool			m_bPlaybackTrackEnabled;
		/** Volume of the playback track.
		 *
		 * It is set by setPlaybackTrackVolume() and
		 * queried by getPlaybackTrackVolume().
		 *
		 * The playback track itself is loaded in
		 * Sampler::reinitialize_playback_track().
		 */
		float			m_fPlaybackTrackVolume;
		AutomationPath*		m_pVelocityAutomationPath;
		///< license of the song
		QString			m_sLicense;

		/** Stores the type of interaction with the SongEditor. */
		ActionMode		m_actionMode;
		
		int m_nPanLawType;
		// k such that L^k+R^k = 1. Used in constant k-Norm pan law
		float m_fPanLawKNorm;

	void setTimeline( std::shared_ptr<Timeline> pTimeline );
	std::shared_ptr<Timeline> m_pTimeline;

};

inline bool Song::getIsTimelineActivated() const {
	return m_bIsTimelineActivated;
}
inline void Song::setIsTimelineActivated( bool bIsTimelineActivated ) {
	m_bIsTimelineActivated = bIsTimelineActivated;
}
inline std::shared_ptr<Timeline> Song::getTimeline() const {
	return m_pTimeline;
}
inline void Song::setTimeline( std::shared_ptr<Timeline> pTimeline ) {
	m_pTimeline = pTimeline;
}

inline bool Song::getIsMuted() const
{
	return m_bIsMuted;
}

inline void Song::setIsMuted( bool bIsMuted )
{
	m_bIsMuted = bIsMuted;
	setIsModified( true );
}

inline unsigned Song::getResolution() const
{
	return m_resolution;
}

inline void Song::setResolution( unsigned resolution )
{
	m_resolution = resolution;
	setIsModified( true );
}

inline float Song::getBpm() const
{
	return m_fBpm;
}

inline void Song::setName( const QString& sName )
{
	m_sName = sName;
	setIsModified( true );
}

inline const QString& Song::getName() const
{
	return m_sName;
}

inline float Song::getVolume() const
{
	return m_fVolume;
}

inline void Song::setVolume( float fValue )
{
	m_fVolume = fValue;
	setIsModified( true );
}

inline float Song::getMetronomeVolume() const
{
	return m_fMetronomeVolume;
}

inline void Song::setMetronomeVolume( float fValue )
{
	m_fMetronomeVolume = fValue;
	setIsModified( true );
}

inline bool Song::getIsModified() const 
{
	return m_bIsModified;
}

inline InstrumentList* Song::getInstrumentList() const
{
	return m_pInstrumentList;
}

inline void Song::setInstrumentList( InstrumentList* pList )
{
	m_pInstrumentList = pList;
}

inline PatternList* Song::getPatternList() const
{
	return m_pPatternList;
}

inline void Song::setPatternList( PatternList* pList )
{
	m_pPatternList = pList;
}

inline std::vector<PatternList*>* Song::getPatternGroupVector() {
	return m_pPatternGroupSequence;
}

inline const std::vector<PatternList*>* Song::getPatternGroupVector() const
{
	return m_pPatternGroupSequence;
}

inline void Song::setPatternGroupVector( std::vector<PatternList*>* pGroupVector )
{
	m_pPatternGroupSequence = pGroupVector;
}

inline void Song::setNotes( const QString& sNotes )
{
	m_sNotes = sNotes;
	setIsModified( true );
}

inline const QString& Song::getNotes() const
{
	return m_sNotes;
}

inline void Song::setLicense( const QString& sLicense )
{
	m_sLicense = sLicense;
	setIsModified( true );
}

inline const QString& Song::getLicense() const
{
	return m_sLicense;
}

inline void Song::setAuthor( const QString& sAuthor )
{
	m_sAuthor = sAuthor;
	setIsModified( true );
}

inline const QString& Song::getAuthor() const
{
	return m_sAuthor;
}

inline const QString& Song::getFilename() const
{
	return m_sFilename;
}

inline void Song::setFilename( const QString& sFilename )
{
	m_sFilename = sFilename;
	setIsModified( true );
}

inline bool Song::getIsLoopEnabled() const
{
	return m_bIsLoopEnabled;
}

inline void Song::setIsLoopEnabled( bool bEnabled )
{
	m_bIsLoopEnabled = bEnabled;
	setIsModified( true );
}

inline float Song::getHumanizeTimeValue() const
{
	return m_fHumanizeTimeValue;
}

inline void Song::setHumanizeTimeValue( float fValue )
{
	m_fHumanizeTimeValue = fValue;
	setIsModified( true );
}

inline float Song::getHumanizeVelocityValue() const
{
	return m_fHumanizeVelocityValue;
}

inline void Song::setHumanizeVelocityValue( float fValue )
{
	m_fHumanizeVelocityValue = fValue;
	setIsModified( true );
}

inline float Song::getSwingFactor() const
{
	return m_fSwingFactor;
}

inline Song::Mode Song::getMode() const
{
	return m_mode;
}

inline void Song::setMode( Song::Mode mode )
{
	m_mode = mode;
	setIsModified( true );
}

inline std::vector<DrumkitComponent*>* Song::getComponents() const
{
	return m_pComponents;
}

inline AutomationPath* Song::getVelocityAutomationPath() const
{
	return m_pVelocityAutomationPath;
}

inline int Song::getLatestRoundRobin( float fStartVelocity )
{
	if ( m_latestRoundRobins.find( fStartVelocity ) == m_latestRoundRobins.end() ) {
		return 0;
	} else {
		return m_latestRoundRobins[ fStartVelocity ];
	}
}

inline void Song::setLatestRoundRobin( float fStartVelocity, int nLatestRoundRobin )
{
	m_latestRoundRobins[ fStartVelocity ] = nLatestRoundRobin;
	setIsModified( true );
}

inline const QString& Song::getPlaybackTrackFilename() const
{
	return m_sPlaybackTrackFilename;
}

inline void Song::setPlaybackTrackFilename( const QString sFilename )
{
	m_sPlaybackTrackFilename = sFilename;
	setIsModified( true );
}

inline bool Song::getPlaybackTrackEnabled() const
{
	return m_bPlaybackTrackEnabled;
}

inline bool Song::setPlaybackTrackEnabled( const bool bEnabled )
{
	if ( m_sPlaybackTrackFilename == nullptr ) {
		return false;
	}
	m_bPlaybackTrackEnabled = bEnabled;
	setIsModified( true );
	return bEnabled;
}

inline float Song::getPlaybackTrackVolume() const
{
	return m_fPlaybackTrackVolume;
}

inline void Song::setPlaybackTrackVolume( const float fVolume )
{
	m_fPlaybackTrackVolume = fVolume;
	setIsModified( true );
}

inline Song::ActionMode Song::getActionMode() const {
	return m_actionMode;
}

inline void Song::setPanLawType( int nPanLawType ) {
	m_nPanLawType = nPanLawType;
	setIsModified( true );
}

inline int Song::getPanLawType() const {
	return m_nPanLawType;
} 

inline float Song::getPanLawKNorm() const {
	return m_fPanLawKNorm;
}

};

#endif
