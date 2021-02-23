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

#ifndef TIMELINE_H
#define TIMELINE_H

#include <memory>

#include <core/Object.h>

namespace H2Core
{
	/**
	 * Timeline class storing and handling all TempoMarkers and Tags.
	 *
	 * All methods altering the TempoMarker and Tag are members of
	 * this class and the former are added as const structs to
	 * m_tempoMarkers or m_tags. To alter one of them, one has to
	 * delete it and add a new, altered version.
	 */
	class Timeline : public H2Core::Object
	{
		H2_OBJECT

		public:
			Timeline();
			~Timeline();

			/**
			 * TempoMarker specifies a change in speed during the
			 * Song.
			 */
			struct TempoMarker
			{
				int		nBar;		// beat position in timeline
				float	fBpm;		// tempo in beats per minute
			};

			/** 
			 * Tag specifies a note added to a certain position in the
			Song.
			*/
			struct Tag
			{
				int		nBar;		// beat position in timeline
				QString sTag;		// tag
			};


			/**
			 * @param nBar Position of the Timeline to query for a 
			 *   tempo marker.
			 * @param fBpm New tempo in beats per minute. All values
			 *   below 30 and above 500 will be cut.
			 */
			void		addTempoMarker( int nBar, float fBpm );
			/**
			 * @param nBar Position of the Timeline to delete the
			 * tempo marker at (if one is present).
			 */
			void		deleteTempoMarker( int nBar );
			void		deleteAllTempoMarkers();
			/**
			 * Returns the tempo of the Song at a given bar.
			 *
			 * @param nBar Position of the Timeline to query for a 
			 *   tempo marker.
			 * @param bSticky If set to true either the tempo marker
			 *   at `nBar` or - if none is present - the nearest
			 *   previous tempo marker is returned. If set to false,
			 *   only the precise position `nBar` is taken into
			 *   account.
			 *
			 * TODO: For now the function returns 0 if the bar is
			 * positioned _before_ the first tempo marker. The calling
			 * routine Hydrogen::getTimelineBpm() will take care of
			 * this and replaces it with pSong->__bpm. This will be
			 * taken care of with #854.
			 */
			float		getTempoAtBar( int nBar, bool bSticky ) const;

			/**
			 * @return std::vector<std::shared_ptr<const TempoMarker>>
			 * Provides read-only access to m_tempoMarker.
			 */
			const std::vector<std::shared_ptr<const TempoMarker>> getAllTempoMarkers() const;

			/**
			 * @param nBar Position of the Timeline to query for a 
			 *   tag.
			 * @param sTag New tag in beats per minute.
			 */
			void		addTag( int nBar, QString sTag );
			/**
			 * @param nBar Position of the Timeline to delete the tag
			 * at (if one is present).
			 */
			void		deleteTag( int nBar );
			void 		deleteAllTags();
			/**
			 * Returns the tag of the Song at a given bar.
			 *
			 * @param nBar Position of the Timeline to query for a 
			 *   tag.
			 * @param bSticky If set to true either the tag at `nBar`
			 *   or - if none is present - the nearest previous tag is
			 *   returned. If set to false, only the precise position
			 *   `nBar` is taken into account.
			 * 
			 * The function returns "" if the bar is positioned
			 * _before_ the first tag or none is present at all.
			 */

			const QString getTagAtBar( int nBar, bool bSticky ) const;
			/**
			 * @return std::vector<std::shared_ptr<const Tag>>
			 * Provides read-only access to m_tags.
			 */
			const std::vector<std::shared_ptr<const Tag>> getAllTags() const;
		private:
			void		sortTempoMarkers();
			void		sortTags();

			std::vector<std::shared_ptr<const TempoMarker>> m_tempoMarkers;
			std::vector<std::shared_ptr<const Tag>> m_tags;

			struct TempoMarkerComparator
			{
				bool operator()( const std::shared_ptr<const TempoMarker> lhs, const std::shared_ptr<const TempoMarker> rhs)
				{
					return lhs->nBar < rhs->nBar;
				}
			};
			struct TagComparator
			{
				bool operator()( const std::shared_ptr<const Tag> lhs, const std::shared_ptr<const Tag> rhs)
				{
					return lhs->nBar < rhs->nBar;
				}
			};
	};
inline void Timeline::deleteAllTempoMarkers() {
	m_tempoMarkers.clear();
}
inline void Timeline::deleteAllTags() {
	m_tags.clear();
}
inline const std::vector<std::shared_ptr<const Timeline::TempoMarker>> Timeline::getAllTempoMarkers() const {
	return m_tempoMarkers;
}
inline const std::vector<std::shared_ptr<const Timeline::Tag>> Timeline::getAllTags() const {
	return m_tags;
}
};
#endif // TIMELINE_H
