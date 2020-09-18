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


#include <algorithm>
#include <hydrogen/timeline.h>

namespace H2Core
{
	const char* Timeline::__class_name = "Timeline";

	Timeline::Timeline() : Object( __class_name )
	{
	}

	void Timeline::addTempoMarker( int nBar, float fBpm ) {
		
		if ( fBpm < 30.0 ) {
			fBpm = 30.0;
		} else if ( fBpm > 500.0 ) {
			fBpm = 500.0;
		}

		HTimelineVector tlvector = { nBar, fBpm };

		m_timelinevector.push_back( &tlvector );
		sortTimelineVector();
	}

	void Timeline::deleteTempoMarker( int nBar ) {

		// Erase the value to set the new value
		if ( m_timelinevector.size() >= 1 ){
			for ( int t = 0; t < m_timelinevector.size(); t++ ){
				if ( m_timelinevector[t]->m_htimelinebeat == nBar ) {
					m_timelinevector.erase( m_timelinevector.begin() +  t);
				}
			}
		}
	}

	float Timeline::getTempoAtBar( int nBar, bool bSticky ) const {
		float fBpm = 0;

		if ( bSticky ) {
			for ( int i = 0; i < static_cast<int>(m_timelinevector.size()); i++) {
				if ( m_timelinevector[i]->m_htimelinebeat > nBar ) {
					break;
				}
				fBpm = m_timelinevector[i]->m_htimelinebpm;
			}
		} else {
			for ( int t = 0; t < static_cast<int>(m_timelinevector.size()); t++ ){
				if ( m_timelinevector[t]->m_htimelinebeat == nBar ){
					fBpm = m_timelinevector[t]->m_htimelinebpm;
				}
			}
		}

		return fBpm;
	}

	void Timeline::addTag( int nBar, QString sTag ) {
		
		HTimelineTagVector tlvector = { nBar, sTag };

		m_timelinetagvector.push_back( &tlvector );
		sortTimelineTagVector();
	}

	void Timeline::deleteTag( int nBar ) {

		// Erase the value to set the new value
		if ( m_timelinetagvector.size() >= 1 ){
			for ( int t = 0; t < m_timelinetagvector.size(); t++ ){
				if ( m_timelinetagvector[t]->m_htimelinetagbeat == nBar ) {
					m_timelinetagvector.erase( m_timelinetagvector.begin() +  t);
				}
			}
		}
		
		sortTimelineTagVector();
	}

	const QString Timeline::getTagAtBar( int nBar, bool bSticky ) const {

		QString sTag("");

		if ( bSticky ) {
			for ( int t = 0; t < static_cast<int>(m_timelinetagvector.size()); t++ ){
				if ( m_timelinetagvector[t]->m_htimelinetagbeat > nBar ){
					break;
				}
				sTag = m_timelinetagvector[t]->m_htimelinetag;
			}
		} else {
			for ( int t = 0; t < static_cast<int>(m_timelinetagvector.size()); t++ ){
				if ( m_timelinetagvector[t]->m_htimelinetagbeat == nBar ){
					sTag =  m_timelinetagvector[t]->m_htimelinetag;
				}
			}
		}

		return sTag;
	}
	
	void Timeline::sortTimelineVector()
	{
		//sort the timeline vector to beats a < b
		sort(m_timelinevector.begin(), m_timelinevector.end(), TimelineComparator());
	}

	void Timeline::sortTimelineTagVector()
	{
		//sort the timeline vector to beats a < b
		sort(m_timelinetagvector.begin(), m_timelinetagvector.end(), TimelineTagComparator());
	}

};

