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

#include <hydrogen/object.h>

namespace H2Core
{
	/** \ingroup docCore*/
	class Timeline : public H2Core::Object
	{
		public:
			Timeline();

		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }

			///sample editor vectors
			void		sortTimelineVector();
			void		sortTimelineTagVector();

			/// timeline vector
			struct HTimelineVector
			{
				int		m_htimelinebeat;		//beat position in timeline
				float	m_htimelinebpm;		//BPM
			};

			/// timeline tag vector
			struct HTimelineTagVector
			{
				int		m_htimelinetagbeat;		//beat position in timeline
				QString m_htimelinetag;			// tag
			};


			std::vector<HTimelineVector> m_timelinevector;
			std::vector<HTimelineTagVector> m_timelinetagvector;

			struct TimelineComparator
			{
				bool operator()( HTimelineVector const& lhs, HTimelineVector const& rhs)
				{
					return lhs.m_htimelinebeat < rhs.m_htimelinebeat;
				}
			};

			struct TimelineTagComparator
			{
				bool operator()( HTimelineTagVector const& lhs, HTimelineTagVector const& rhs)
				{
					return lhs.m_htimelinetagbeat < rhs.m_htimelinetagbeat;
				}
			};
		private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;

	};
};

#endif // TIMELINE_H
