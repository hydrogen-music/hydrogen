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

#ifndef TIMELINE_H
#define TIMELINE_H

#include <memory>

#include <core/Object.h>

namespace H2Core
{

/**
 * Timeline class storing and handling all TempoMarkers and Tags.
 *
 * The design of the Timeline is a little bit involved and needs a
 * couple of words to explain it. As soon as the Timeline is
 * activated, it is required to yield a tempo for each column which
 * solely depends on the TempoMarkers set by the user. Since a
 * TempoMarker determines the tempo for all columns equal or
 * bigger its own, there always needs a be one present at the very
 * beginning of the Song.
 *
 * For a more convenient UX the user shouldn't bother about the first
 * one too much as a TempoMarker is usually put after the first column
 * to indicate a _change_ in tempo. Therefore, when a TempoMarker is
 * added to an empty Timeline (to any location but the first column),
 * the Timeline pretends to have a tempo marker in the beginning
 * holding the current Song::m_fBpm (the tempo set via the BPM
 * widget). This special first marker can only be "removed" by the
 * user by adding a TempoMarker to the first column. In addition, it's
 * tempo gets updated when toggling the Timeline. This, it does not
 * immediately respond to changes of the song tempo (using the BPM
 * widget or MIDI/OSC commands). The special tempo marker is prepended
 * in the getAllTempoMarkers() functions.
 *
 * The calling function will not notice any difference between the
 * provided TempoMarkers and has to use
 * isFirstTempoMarkerSpecial() instead.
 *
 * All methods altering the TempoMarker and Tag are members of
 * this class and the former are added as const structs to
 * m_tempoMarkers or m_tags. To alter one of them, one has to
 * delete it and add a new, altered version.
 */
/** \ingroup docCore*/
class Timeline : public H2Core::Object<Timeline>
{
	H2_OBJECT(Timeline)

public:
	Timeline();
	~Timeline();

	/**
	 * TempoMarker specifies a change in speed during the
	 * Song.
	 */
	struct TempoMarker {
		TempoMarker( int column, float bpm ) : nColumn( column ), fBpm( bpm ) {};
		int		nColumn;		// beat position in timeline
		float	fBpm;		// tempo in beats per minute

		/**
		 * @param nDecimals Number of digits after point.
		 *
		 * @return A text representation of #fBpm limited to @a
		 *   nDecimals decimals after point and trailing zeros
		 *   removed.
		 */
		QString getPrettyString( int nDecimals = 2 ) const;
		QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
	};

	/** 
	 * Tag specifies a note added to a certain position in the
	 Song.
	*/
	struct Tag
	{
		Tag( int column, const QString& tag ) : nColumn( column ), sTag( tag ){};
		int		nColumn;		// beat position in timeline
		QString sTag;		// tag

		QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
	};

	/** 
	 * Registers the current playback tempo to m_fDefaultBpm.
	 */
	void activate();
	/**
	 * Convencience function in order to create a symmetric pair with activate
	 */
	void deactivate();

		void setDefaultBpm( float fDefaultBpm );

	/** Adds a TempoMarker to the Timeline.
	 *
	 * Fails if there is already a #TempoMarker present at @a nColumn.
	 *
	 * @param nColumn Position of the Timeline to query for a 
	 *   tempo marker.
	 * @param fBpm New tempo in beats per minute. All values
	 *   below 30 and above 500 will be cut.
	 */
	void		addTempoMarker( int nColumn, float fBpm );
	/** Variant of #addTempoMarker() to add more than one at a time.*/
	void		addTempoMarkers( const std::vector<std::shared_ptr<TempoMarker>>& );

	/** Delete all tempo markers except for the first one and
	 * mark the tempo of the Timeline m_bUnset.
	 *
	 * @param nColumn Position of the Timeline to delete the
	 * tempo marker at (if one is present).
	 */
	void		deleteTempoMarker( int nColumn );
	/** Deletes all TempoMarkers set by the user. But not the
		special one.*/
	void		deleteAllTempoMarkers();
	/**
	 * Returns the tempo of the Song at a given column.
	 *
	 * There does not have to be a TempoMarker present at @a nColumn
	 * as the function takes all preceding ones into account as well.
	 *
	 * @param nColumn Position of the Timeline to query for a 
	 *   tempo marker.
	 */
	float		getTempoAtColumn( int nColumn ) const;

	bool hasColumnTempoMarker( int nColumn ) const;
	std::shared_ptr<const Timeline::TempoMarker> getTempoMarkerAtColumn( int nColumn ) const;
	/**
	 * @return std::vector<std::shared_ptr<const TempoMarker>>
	 * Provides read-only access to m_tempoMarker.
	 */
	const std::vector<std::shared_ptr<const TempoMarker>>& getAllTempoMarkers() const;

	/** Whether there is a TempoMarker introduced by the user at the
		first column. If not, the Timeline pretends that there is one by
		returning the current Song's tempo. The latter is referred to
		by "special tempo marker".*/
	bool isFirstTempoMarkerSpecial() const;

	/** Adds a Tag to the Timeline.
	 *
	 * Fails if there is already a #Tag present at @a nColumn.
	 *
	 * @param nColumn Position of the Timeline to query for a 
	 *   tag.
	 * @param sTag New tag in beats per minute.
	 */
	void		addTag( int nColumn, const QString& sTag );
	/** Variant of #addTag() to add more than one at a time.*/
	void		addTags( const std::vector<std::shared_ptr<Tag>>& );
	/**
	 * @param nColumn Position of the Timeline to delete the tag
	 * at (if one is present).
	 */
	void		deleteTag( int nColumn );
	void 		deleteAllTags();
	/**
	 * Returns the tag of the Song at a given column.
	 *
	 * @param nColumn Position of the Timeline to query for a 
	 *   tag.
	 * 
	 * The function returns "" if the column is positioned
	 * _before_ the first tag or none is present at all.
	 */

	const QString getTagAtColumn( int nColumn ) const;
	/**
	 * @return std::vector<std::shared_ptr<const Tag>>
	 * Provides read-only access to m_tags.
	 */
	const std::vector<std::shared_ptr<const Tag>>& getAllTags() const;
	bool hasColumnTag( int nColumn ) const;
	
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
	void		sortTempoMarkers();
	void		sortTags();
	/** Constructs/updates #m_allTempoMarkers based on #m_tempoMarkers. */
	void		updateTempoMarkers();


	/** Version of #m_tempoMarkers containing the optional special one too.
	 * Since the later must not be written to disk, this member is kept to
	 * increase performance. */
	std::vector<std::shared_ptr<const TempoMarker>> m_allTempoMarkers;
	std::vector<std::shared_ptr<const TempoMarker>> m_tempoMarkers;
	std::vector<std::shared_ptr<const Tag>> m_tags;

	/**
	 * Tempo used for the special tempo marker.
	 *
	 * It's the task of the calling function to ensure this is set to
	 * the last Song::m_fBpm when activating the Timeline.
	 */
	float m_fDefaultBpm;
	
	struct TempoMarkerComparator
	{
		bool operator()( const std::shared_ptr<const TempoMarker> lhs, const std::shared_ptr<const TempoMarker> rhs)
		{
			return lhs->nColumn < rhs->nColumn;
		}
	};
	struct TagComparator
	{
		bool operator()( const std::shared_ptr<const Tag> lhs, const std::shared_ptr<const Tag> rhs)
		{
			return lhs->nColumn < rhs->nColumn;
		}
	};
};
	
inline void Timeline::deleteAllTempoMarkers() {
		m_tempoMarkers.clear();
}
inline void Timeline::deleteAllTags() {
	m_tags.clear();
}
inline const std::vector<std::shared_ptr<const Timeline::TempoMarker>>& Timeline::getAllTempoMarkers() const {
	return m_allTempoMarkers;
}
inline const std::vector<std::shared_ptr<const Timeline::Tag>>& Timeline::getAllTags() const {
	return m_tags;
}
};
#endif // TIMELINE_H
