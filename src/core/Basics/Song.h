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

#ifndef SONG_H
#define SONG_H


#include <QString>
#include <QDomNode>
#include <vector>
#include <map>
#include <memory>

#include <core/License.h>
#include <core/Object.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Xml.h>

class TiXmlNode;

namespace H2Core
{

class ADSR;
class AutomationPath;
class Drumkit;
class GridPoint;
class Instrument;
class Note;
class Pattern;
class PatternList;
class Sample;
class SoundLibraryDatabase;
class Timeline;

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
			Song = 1,
			/** Used in case no song is set and both pattern and song
				editor are not ready to operate yet.*/
			None = 2
		};
		static QString ModeToQString( const Mode& mode );

		/** Defines the type of user interaction experienced in the 
			SongEditor.*/
		enum class ActionMode {
			/** Holding a pressed left mouse key will draw a rectangle to
				select a group of Notes.*/
			selectMode = 0,
			/** Holding a pressed left mouse key will draw/delete patterns
				in all grid cells encountered.*/
			drawMode = 1,
			/** Used in case no song is set and both pattern and song
				editor are not ready to operate yet.*/
			None = 2
		};
		static QString ActionModeToQString( const ActionMode& actionMode );

		enum class LoopMode {
			Disabled = 0,
			Enabled = 1,
			/**
			 * Transport is still in loop mode (frames and ticks
			 * larger than song size are allowed) but playback ends
			 * the next time the end of the song is reached.
			 */
			Finishing = 2
		};
		static QString LoopModeToQString( const LoopMode& loopMode );

	/** Determines how patterns will be added to
	 * AudioEngine::m_pPlayingPatterns if transport is in
	 * Song::Mode::Pattern.
	 */
	enum class PatternMode {
		/** An arbitrary number of pattern can be played.*/
		Stacked = 0,
		/** Only one pattern - the one currently selected in the GUI -
		 * will be played back.*/
		Selected = 1,
		/** Null element used to indicate that either no song is
		 * present or Song::Mode::Song was selected
		 */
		None = 2
	};
		static QString PatternModeToQString( const PatternMode& patternMode );

	/** Determines the state of the Playback track with respect to
		audio processing*/
	enum class PlaybackTrack {
		/** No proper playback track file set yet*/
		Unavailable = 0,
		/** Valid file set but the playback track is muted via the GUI*/
		Muted = 1,
		/** Valid file set and ready for playback.*/
		Enabled = 2,
		/** Null element used to indicate that either no song is
		 * present*/
		None = 3
	};
		static QString PlaybackTrackToQString( const PlaybackTrack& playbackTrack );

		/** Please do not #H2Core::Hydrogen::setSong() a song created using this
		 * constructor. It is just a minimal version with not all its members
		 * properly initialized and can causes crashes (in the
		 * #H2Core::AudioEngine) when used directly. Please use getEmptySong()
		 * instead. */
		Song( const QString& sName = "Untitled Song",
			  const QString& sAuthor = "Unknown Author",
			  float fBpm = 120,
			  float fVolume = 0.5 );
		~Song();

		/** Creates the default / fallback song.
		 *
		 * @param pDB When creating an empty song during startup, the
		 *   #H2Core::Hydrogen singleton might not be ready yet. This can be
		 *   compensated by passing the created instance directly instead. */
		static std::shared_ptr<Song> getEmptySong(
			std::shared_ptr<SoundLibraryDatabase> pDB = nullptr );

	static std::shared_ptr<Song> 	load( const QString& sFilename, bool bSilent = false );
	/** Writes the song as .h2song to disk.
	 *
	 * @param sFilename Absolute path to write the song to.
	 * \param bSilent if set to true, all log messages except of errors and
	 *   warnings are suppressed.
	 */
	bool 			save( const QString& sFilename, bool bSilent = false );

	bool getIsTimelineActivated() const;
	void setIsTimelineActivated( bool bIsTimelineActivated );
	
	bool getIsPatternEditorLocked() const;
	void setIsPatternEditorLocked( bool bIsPatternEditorLocked );

		bool getIsMuted() const;
		void setIsMuted( bool bIsMuted );

		float getBpm() const;
		void setBpm( float fBpm );

		int getVersion() const;
		void setVersion( int nVersion );

		const QString& getName() const;
		void setName( const QString& sName );
		
		void setVolume( float fVolume );
		float getVolume() const;

		void setMetronomeVolume( float fVolume );
		float getMetronomeVolume() const;

		std::shared_ptr<PatternList> getPatternList() const;
		void setPatternList( std::shared_ptr<PatternList> pList );

		std::shared_ptr<Drumkit> getDrumkit() const;
		void setDrumkit( std::shared_ptr<Drumkit> pDrumkit );

		/** Return a pointer to a vector storing all Pattern
		 * present in the Song.
		 * \return #m_pPatternGroupSequence */
		std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > getPatternGroupVector();
		/** Return a pointer to a vector storing all Pattern
		 * present in the Song.
		 * \return #m_pPatternGroupSequence */
		const std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > getPatternGroupVector() const;

		/** Sets the vector storing all Pattern present in the
		 * Song #m_pPatternGroupSequence.
		 * \param pGroupVect Pointer to a vector containing all
		 *   Pattern of the Song.*/
		void setPatternGroupVector( std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > pGroupVect );

		/** get the length of the song, in tick units */
		long lengthInTicks() const;

		void			setNotes( const QString& sNotes );
		const QString&		getNotes() const;

		void			setLicense( const License& license );
		const License&		getLicense() const;

		void			setAuthor( const QString& sAuthor );
		const QString&		getAuthor() const;

		const QString&		getFilename() const;
		void			setFilename( const QString& sFilename );
							
		const LoopMode&	getLoopMode() const;
		void			setLoopMode( const LoopMode& loopMode );
		bool			isLoopEnabled() const;
							
		const PatternMode& getPatternMode() const;
		void			setPatternMode( const PatternMode& patternMode );
							
		float			getHumanizeTimeValue() const;
		void			setHumanizeTimeValue( float fValue );
							
		float			getHumanizeVelocityValue() const;
		void			setHumanizeVelocityValue( float fValue );
							
		float			getSwingFactor() const;
		void			setSwingFactor( float fFactor );

		const Mode&		getMode() const;
		void			setMode( const Mode& mode );
							
		bool			getIsModified() const;
		void			setIsModified( bool bIsModified);

		AutomationPath*	getVelocityAutomationPath() const;

		void			loadTempPatternList( const QString& sFilename );
		bool			saveTempPatternList( const QString& sFilename ) const;
							
		int			getLatestRoundRobin( float fStartVelocity ) const;
		void			setLatestRoundRobin( float fStartVelocity, int nLatestRoundRobin );
		/** \return #m_sPlaybackTrackFilename */
		const QString&		getPlaybackTrackFilename() const;
		/** \param sFilename Sets #m_sPlaybackTrackFilename. */
		void			setPlaybackTrackFilename( const QString& sFilename );
							
		/** \return #m_bPlaybackTrackEnabled */
		bool			getPlaybackTrackEnabled() const;
		/** Specifies whether a playback track should be used.
		 *
		 * \param bEnabled Sets #m_bPlaybackTrackEnabled. */
		void			setPlaybackTrackEnabled( const bool bEnabled );
							
		/** \return #m_fPlaybackTrackVolume */
		float			getPlaybackTrackVolume() const;
		/** \param fVolume Sets #m_fPlaybackTrackVolume. */
		void			setPlaybackTrackVolume( const float fVolume );

		PlaybackTrack getPlaybackTrackState() const;

	
		const ActionMode& getActionMode() const;
		void			setActionMode( const ActionMode& actionMode );

		/** Song was incompletely loaded from file (missing samples)
		 */
		bool hasMissingSamples() const;
		void clearMissingSamples();
		
		void setPanLawType( int nPanLawType );
		int getPanLawType() const;
		void setPanLawKNorm( float fKNorm );
		float getPanLawKNorm() const;

		bool isPatternActive( const GridPoint& gridPoint ) const;

	std::shared_ptr<Timeline> getTimeline() const;

	std::vector<std::shared_ptr<Note>> getAllNotes() const;

		const QString& getLastLoadedDrumkitPath() const;
		void setLastLoadedDrumkitPath( const QString& sPath );

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const override;
	
private:

	static std::shared_ptr<Song> loadFrom( const XMLNode& pNode,
										   const QString& sFilename,
										   bool bSilent = false );
	void saveTo( XMLNode& pNode, bool bSilent = false ) const;

	void loadVirtualPatternsFrom( const XMLNode& pNode, bool bSilent = false );
	void loadPatternGroupVectorFrom( const XMLNode& pNode, bool bSilent = false );
	void saveVirtualPatternsTo( XMLNode& pNode, bool bSilent = false ) const;
	void savePatternGroupVectorTo( XMLNode& pNode, bool bSilent = false ) const;

	/** Whether the Timeline button was pressed in the GUI or it was
		activated via an OSC command. */
	bool m_bIsTimelineActivated;
							
		bool m_bIsMuted;
		/**
		 * Current speed in beats per minutes.
		 *
		 * See TransportPosition::m_fBpm for how the handling of the
		 * different tempo instances work.
		 */
		float m_fBpm;

		int m_nVersion;
		
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
		std::shared_ptr<PatternList>	m_pPatternList;
		///< Sequence of pattern groups
		std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > m_pPatternGroupSequence;

		/** Current drumkit
		 *
		 * This one is either based on the last kit loaded from the
		 * `SoundLibraryDatabase` or is a brand new kit. */
		std::shared_ptr<Drumkit> m_pDrumkit;

		QString			m_sFilename;

		/**
		 * The three states of this enum is just a way to handle the
		 * playback within Hydrogen. Not its content but the output of
		 * isLoopEnabled(), whether enabled or disabled, will be
		 * written to disk.
		 */
		LoopMode		m_loopMode;
		PatternMode		m_patternMode;
		/**
		 * Factor to scale the random contribution when humanizing
		 * timing between 0 and #AudioEngine::fHumanizeTimingSD.
		 *
		 * Supported range [0,1].
		 */
		float			m_fHumanizeTimeValue;
		/**
		 * Factor to scale the random contribution when humanizing
		 * velocity between 0 and #AudioEngine::fHumanizeVelocitySD.
		 *
		 * Supported range [0,1].
		 */
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
		License			m_license;

		/** Stores the type of interaction with the SongEditor. */
		ActionMode		m_actionMode;

	/**
	 * If set to true, the user won't be able to select a pattern via
	 * the SongEditor. Instead, the pattern recorded note would be
	 * inserted into is displayed. In single pattern/selected pattern
	 * mode this is the one pattern being played back and in stacked
	 * pattern mode this is the bottom-most one.
	 *
	 * This mode is only supported in Song mode.
	 */
	bool m_bIsPatternEditorLocked;
		
		int m_nPanLawType;
		// k such that L^k+R^k = 1. Used in constant k-Norm pan law
		float m_fPanLawKNorm;

	void setTimeline( std::shared_ptr<Timeline> pTimeline );
	std::shared_ptr<Timeline> m_pTimeline;

	/** Unique identifier of the drumkit last loaded.
	 *
	 * This is a convenience variable allowing to cycle through the different
	 * kits using MIDI and OSC commands (load next/previous kit).
	 *
	 * Note: In older versions of Hydrogen (< 1.3.0) this variable was used to
	 * determine the location of samples of the current song with relative file
	 * paths. This is not done anymore since a song does now contain a proper
	 * #H2Core::Drumkit. That's why this member variable was repurposed.
	 */
	QString m_sLastLoadedDrumkitPath;

		/** Used to indicate changes in the underlying XSD file. */
		static constexpr int nCurrentFormatVersion = 2;
};

inline bool Song::getIsTimelineActivated() const {
	return m_bIsTimelineActivated;
}
inline void Song::setIsTimelineActivated( bool bIsTimelineActivated ) {
	m_bIsTimelineActivated = bIsTimelineActivated;
}
inline bool Song::getIsPatternEditorLocked() const {
	return m_bIsPatternEditorLocked;
}
inline void Song::setIsPatternEditorLocked( bool bIsPatternEditorLocked ) {
	m_bIsPatternEditorLocked = bIsPatternEditorLocked;
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
}

inline float Song::getBpm() const
{
	return m_fBpm;
}

inline void Song::setVersion( int nVersion ) {
	m_nVersion = nVersion;
}
inline int Song::getVersion() const {
	return m_nVersion;
}


inline void Song::setName( const QString& sName )
{
	m_sName = sName;
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
}

inline float Song::getMetronomeVolume() const
{
	return m_fMetronomeVolume;
}

inline void Song::setMetronomeVolume( float fValue )
{
	m_fMetronomeVolume = fValue;
}

inline bool Song::getIsModified() const 
{
	return m_bIsModified;
}

inline std::shared_ptr<Drumkit> Song::getDrumkit() const
{
	return m_pDrumkit;
}

inline std::shared_ptr<PatternList> Song::getPatternList() const
{
	return m_pPatternList;
}

inline void Song::setPatternList( std::shared_ptr<PatternList> pList )
{
	m_pPatternList = pList;
}

inline std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > Song::getPatternGroupVector() {
	return m_pPatternGroupSequence;
}

inline const std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > Song::getPatternGroupVector() const
{
	return m_pPatternGroupSequence;
}

inline void Song::setPatternGroupVector( std::shared_ptr< std::vector< std::shared_ptr<PatternList> > > pGroupVector )
{
	m_pPatternGroupSequence = pGroupVector;
}

inline void Song::setNotes( const QString& sNotes )
{
	m_sNotes = sNotes;
}

inline const QString& Song::getNotes() const
{
	return m_sNotes;
}

inline void Song::setLicense( const License& license )
{
	m_license = license;
}

inline const License& Song::getLicense() const
{
	return m_license;
}

inline void Song::setAuthor( const QString& sAuthor )
{
	m_sAuthor = sAuthor;
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
}

inline bool Song::isLoopEnabled() const
{
	return m_loopMode == LoopMode::Enabled ||
		m_loopMode == LoopMode::Finishing;
}

inline const Song::LoopMode& Song::getLoopMode() const
{
	return m_loopMode;
}
inline void Song::setLoopMode( const Song::LoopMode& loopMode )
{
	m_loopMode = loopMode;
}

inline const Song::PatternMode& Song::getPatternMode() const
{
	return m_patternMode;
}
inline void Song::setPatternMode( const Song::PatternMode& patternMode )
{
	m_patternMode = patternMode;
}

inline float Song::getHumanizeTimeValue() const
{
	return m_fHumanizeTimeValue;
}

inline void Song::setHumanizeTimeValue( float fValue )
{
	m_fHumanizeTimeValue = fValue;
}

inline float Song::getHumanizeVelocityValue() const
{
	return m_fHumanizeVelocityValue;
}

inline void Song::setHumanizeVelocityValue( float fValue )
{
	m_fHumanizeVelocityValue = fValue;
}

inline float Song::getSwingFactor() const
{
	return m_fSwingFactor;
}

inline const Song::Mode& Song::getMode() const
{
	return m_mode;
}

inline void Song::setMode( const Song::Mode& mode )
{
	m_mode = mode;
}

inline AutomationPath* Song::getVelocityAutomationPath() const
{
	return m_pVelocityAutomationPath;
}

inline int Song::getLatestRoundRobin( float fStartVelocity ) const
{
	if ( m_latestRoundRobins.find( fStartVelocity ) == m_latestRoundRobins.end() ) {
		return 0;
	} else {
		return m_latestRoundRobins.at( fStartVelocity );
	}
}

inline void Song::setLatestRoundRobin( float fStartVelocity, int nLatestRoundRobin )
{
	m_latestRoundRobins[ fStartVelocity ] = nLatestRoundRobin;
}

inline const QString& Song::getPlaybackTrackFilename() const
{
	return m_sPlaybackTrackFilename;
}

inline void Song::setPlaybackTrackFilename( const QString& sFilename )
{
	m_sPlaybackTrackFilename = sFilename;
}

inline bool Song::getPlaybackTrackEnabled() const
{
	return m_bPlaybackTrackEnabled;
}

inline void Song::setPlaybackTrackEnabled( const bool bEnabled )
{
	m_bPlaybackTrackEnabled = bEnabled;
}

inline float Song::getPlaybackTrackVolume() const
{
	return m_fPlaybackTrackVolume;
}

inline void Song::setPlaybackTrackVolume( const float fVolume )
{
	m_fPlaybackTrackVolume = fVolume;
}
inline Song::PlaybackTrack Song::getPlaybackTrackState() const {
	if ( m_sPlaybackTrackFilename.isEmpty() ) {
		return std::move( PlaybackTrack::Unavailable );
	}

	if ( ! m_bPlaybackTrackEnabled ) {
		return std::move( PlaybackTrack::Muted );
	}

	return std::move( PlaybackTrack::Enabled );
}

inline const Song::ActionMode& Song::getActionMode() const {
	return m_actionMode;
}

inline void Song::setPanLawType( int nPanLawType ) {
	m_nPanLawType = nPanLawType;
}

inline int Song::getPanLawType() const {
	return m_nPanLawType;
} 

inline float Song::getPanLawKNorm() const {
	return m_fPanLawKNorm;
}
inline void Song::setLastLoadedDrumkitPath( const QString& sPath ) {
	m_sLastLoadedDrumkitPath = sPath;
}
inline const QString& Song::getLastLoadedDrumkitPath() const {
	return m_sLastLoadedDrumkitPath;
}
};

#endif
