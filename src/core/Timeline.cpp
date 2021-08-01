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


#include <algorithm>
#include <core/Timeline.h>

namespace H2Core
{
	const char* Timeline::__class_name = "Timeline";

	Timeline::Timeline() : Object( __class_name )
	{
	}

	Timeline::~Timeline() {
		m_tempoMarkers.clear();
		m_tags.clear();
	}

	void Timeline::addTempoMarker( int nBar, float fBpm ) {
		
		if ( fBpm < MIN_BPM ) {
			fBpm = MIN_BPM;
			WARNINGLOG( QString( "Provided bpm %1 is too low. Assigning lower bound %2 instead" )
						.arg( fBpm ).arg( MIN_BPM ) );
		} else if ( fBpm > MAX_BPM ) {
			fBpm = MAX_BPM;
			WARNINGLOG( QString( "Provided bpm %1 is too high. Assigning upper bound %2 instead" )
						.arg( fBpm ).arg( MAX_BPM ) );
		}

		std::shared_ptr<TempoMarker> pTempoMarker( new TempoMarker );
		pTempoMarker->nBar = nBar;
		pTempoMarker->fBpm = fBpm;

		m_tempoMarkers.push_back( pTempoMarker );
		sortTempoMarkers();
	}

	void Timeline::deleteTempoMarker( int nBar ) {

		// Erase the value to set the new value
		if ( m_tempoMarkers.size() >= 1 ){
			for ( int t = 0; t < m_tempoMarkers.size(); t++ ){
				if ( m_tempoMarkers[t]->nBar == nBar ) {
					m_tempoMarkers.erase( m_tempoMarkers.begin() +  t);
				}
			}
		}
	}

	float Timeline::getTempoAtBar( int nBar, bool bSticky ) const {
		float fBpm = 0;

		if ( bSticky ) {
			for ( int i = 0; i < static_cast<int>(m_tempoMarkers.size()); i++) {
				if ( m_tempoMarkers[i]->nBar > nBar ) {
					break;
				}
				fBpm = m_tempoMarkers[i]->fBpm;
			}
		} else {
			for ( int t = 0; t < static_cast<int>(m_tempoMarkers.size()); t++ ){
				if ( m_tempoMarkers[t]->nBar == nBar ){
					fBpm = m_tempoMarkers[t]->fBpm;
				}
			}
		}

		return fBpm;
	}

	void Timeline::addTag( int nBar, QString sTag ) {
		
		std::shared_ptr<Tag> pTag( new Tag );
		pTag->nBar = nBar;
		pTag->sTag = sTag;

		m_tags.push_back( std::move( pTag ) );
		sortTags();
	}

	void Timeline::deleteTag( int nBar ) {

		// Erase the value to set the new value
		if ( m_tags.size() >= 1 ){
			for ( int t = 0; t < m_tags.size(); t++ ){
				if ( m_tags[t]->nBar == nBar ) {
					m_tags.erase( m_tags.begin() +  t);
				}
			}
		}
		
		sortTags();
	}

	const QString Timeline::getTagAtBar( int nBar, bool bSticky ) const {

		QString sCurrentTag("");

		if ( bSticky ) {
			for ( int t = 0; t < static_cast<int>(m_tags.size()); t++ ){
				if ( m_tags[t]->nBar > nBar ){
					break;
				}
				sCurrentTag = m_tags[t]->sTag;
			}
		} else {
			for ( int t = 0; t < static_cast<int>(m_tags.size()); t++ ){
				if ( m_tags[t]->nBar == nBar ){
					sCurrentTag =  m_tags[t]->sTag;
				}
			}
		}

		return sCurrentTag;
	}
	
	void Timeline::sortTempoMarkers()
	{
		//sort the timeline vector to beats a < b
		sort(m_tempoMarkers.begin(), m_tempoMarkers.end(), TempoMarkerComparator());
	}

	void Timeline::sortTags()
	{
		//sort the timeline vector to beats a < b
		sort(m_tags.begin(), m_tags.end(), TagComparator());
	}

};

