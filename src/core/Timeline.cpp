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


#include <algorithm>
#include <core/Timeline.h>
#include <core/Hydrogen.h>
#include <core/Basics/Song.h>

namespace H2Core
{

Timeline::Timeline() : Object( )
					 , m_fDefaultBpm( 120 ) {
	updateTempoMarkers();
}

Timeline::~Timeline() {
	m_tempoMarkers.clear();
	m_tags.clear();
}

void Timeline::activate() {
	setDefaultBpm( Hydrogen::get_instance()->getSong()->getBpm() );
}

void Timeline::deactivate() {
}

void Timeline::setDefaultBpm( float fDefaultBpm ) {
	if ( m_fDefaultBpm != fDefaultBpm ) {
		m_fDefaultBpm = fDefaultBpm;
		updateTempoMarkers();
	}
}

void Timeline::addTempoMarkers( const std::vector<std::shared_ptr<TempoMarker>>& tempoMarkers) {
	// Sanity checks
	for ( auto& mmarker : tempoMarkers ) {
		if ( hasColumnTempoMarker( mmarker->nColumn ) ) {
			ERRORLOG( QString( "There is already a tempo marker present in column %1. Please remove it first." )
					  .arg( mmarker->nColumn ) );
			return;
		}

		for ( const auto& ootherMarker : tempoMarkers ) {
			if ( ootherMarker != mmarker && ootherMarker->nColumn == mmarker->nColumn ) {
				ERRORLOG( QString( "Supplied tempoMarkers [%1] and [%2] share the same column" )
						  .arg( mmarker->toQString() )
						  .arg( ootherMarker->toQString() ) );
				return;
			}
		}

		if ( mmarker->fBpm < MIN_BPM ) {
			mmarker->fBpm = MIN_BPM;
			WARNINGLOG( QString( "Provided bpm %1 is too low. Assigning lower bound %2 instead" )
						.arg( mmarker->fBpm ).arg( MIN_BPM ) );
		} else if ( mmarker->fBpm > MAX_BPM ) {
			mmarker->fBpm = MAX_BPM;
			WARNINGLOG( QString( "Provided bpm %1 is too high. Assigning upper bound %2 instead" )
						.arg( mmarker->fBpm ).arg( MAX_BPM ) );
		}

	}

	for ( const auto& mmarker : tempoMarkers ) {
		m_tempoMarkers.push_back( mmarker );
	}
	updateTempoMarkers();
}

void Timeline::addTempoMarker( int nColumn, float fBpm ) {
	if ( fBpm < MIN_BPM ) {
		fBpm = MIN_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too low. Assigning lower bound %2 instead" )
					.arg( fBpm ).arg( MIN_BPM ) );
	} else if ( fBpm > MAX_BPM ) {
		fBpm = MAX_BPM;
		WARNINGLOG( QString( "Provided bpm %1 is too high. Assigning upper bound %2 instead" )
					.arg( fBpm ).arg( MAX_BPM ) );
	}

	// If a marker is already present in the provided column, we just replace
	// it.
	bool bTempoMarkerFound = false;
	for ( int ii = 0; ii < m_tempoMarkers.size(); ++ii ){
		if ( m_tempoMarkers[ ii ] != nullptr &&
			 m_tempoMarkers[ ii ]->nColumn == nColumn ) {
			m_tempoMarkers[ ii ] = std::make_shared<TempoMarker>( nColumn, fBpm );
			bTempoMarkerFound = true;
			break;
		}
	}

	if ( ! bTempoMarkerFound ) {
		m_tempoMarkers.push_back( std::make_shared<TempoMarker>(nColumn, fBpm) );
	}
	updateTempoMarkers();
}

void Timeline::deleteTempoMarker( int nColumn ) {
	// Erase the value to set the new value
	if ( m_tempoMarkers.size() >= 1 ){
		for ( int t = 0; t < m_tempoMarkers.size(); t++ ){
			if ( m_tempoMarkers[t]->nColumn == nColumn ) {
				m_tempoMarkers.erase( m_tempoMarkers.begin() + t );
			}
		}
	}

	updateTempoMarkers();
}

float Timeline::getTempoAtColumn( int nColumn ) const {
	auto pHydrogen = Hydrogen::get_instance();
		
	if ( m_tempoMarkers.size() == 0 ) {
		return m_fDefaultBpm;
	}

	float fBpm = m_fDefaultBpm;
	// When transport is stopped nColumn is set to -1 by the
	// AudioEngine.
	if ( nColumn == -1 ) {
		nColumn = 0;
	}
	if ( isFirstTempoMarkerSpecial() && nColumn < m_tempoMarkers[ 0 ]->nColumn ) {
		fBpm = m_fDefaultBpm;
	} else {
		for ( int ii = 0; ii < static_cast<int>(m_tempoMarkers.size()); ii++) {
			if ( m_tempoMarkers[ ii ]->nColumn > nColumn ) {
				break;
			}
			fBpm = m_tempoMarkers[ ii ]->fBpm;
		}
	}
	return fBpm;
}

bool Timeline::isFirstTempoMarkerSpecial() const {
	if ( m_tempoMarkers.size() == 0 ) {
		return true;
	}

	return m_tempoMarkers[ 0 ]->nColumn != 0;
}

bool Timeline::hasColumnTempoMarker( int nColumn ) const {
	for ( const auto& tt : m_tempoMarkers ){
		if ( tt->nColumn == nColumn ) {
			return true;
		}
	}
	return false;
}

std::shared_ptr<const Timeline::TempoMarker> Timeline::getTempoMarkerAtColumn( int nColumn ) const {
	if ( isFirstTempoMarkerSpecial() && nColumn == 0 ) {

		std::shared_ptr<TempoMarker> pTempoMarker = std::make_shared<TempoMarker>(
			0, Hydrogen::get_instance()->getSong()->getBpm() );
		return pTempoMarker;
	}
	
	for ( const auto& tt : m_tempoMarkers ){
		if ( tt->nColumn == nColumn ) {
			return tt;
		}
	}
	return nullptr;
}

void Timeline::updateTempoMarkers() {
	if ( isFirstTempoMarkerSpecial() ) {

		std::shared_ptr<TempoMarker> pTempoMarker =
			std::make_shared<TempoMarker>( 0, m_fDefaultBpm );

		const int nNumberOfTempoMarkers = m_tempoMarkers.size();

		m_allTempoMarkers.clear();
		m_allTempoMarkers.resize( nNumberOfTempoMarkers + 1 );
		m_allTempoMarkers[ 0 ] = pTempoMarker;

		if ( nNumberOfTempoMarkers != 0 ) {
			for ( int ii = 0; ii < nNumberOfTempoMarkers; ++ii ) {
				// Since the returned vector is const, there is no
				// need to make a deep copy.
				m_allTempoMarkers[ ii + 1 ] = m_tempoMarkers[ ii ];
			}
		}
	}
	else {
		m_allTempoMarkers = m_tempoMarkers;
	}

	sortTempoMarkers();
}
		
void Timeline::sortTempoMarkers() {
	sort( m_tempoMarkers.begin(), m_tempoMarkers.end(),
		  TempoMarkerComparator() );
}

void Timeline::addTags( const std::vector<std::shared_ptr<Tag>>& tags) {
	// Sanity checks
	for ( const auto& ttag : tags ) {
		if ( hasColumnTag( ttag->nColumn ) ) {
			ERRORLOG( QString( "There is already a tag present in column %1. Please remove it first." )
					  .arg( ttag->nColumn ) );
			return;
		}

		for ( const auto& ootherTag : tags ) {
			if ( ootherTag != ttag && ootherTag->nColumn == ttag->nColumn ) {
				ERRORLOG( QString( "Supplied tags [%1] and [%2] share the same column" )
						  .arg( ttag->toQString() )
						  .arg( ootherTag->toQString() ) );
				return;
			}
		}
	}

	for ( const auto& ttag : tags ) {
		m_tags.push_back( ttag );
	}

	sortTags();
}

void Timeline::addTag( int nColumn, const QString& sTag ) {
		
	if ( hasColumnTag( nColumn ) ) {
		ERRORLOG( QString( "There is already a tag present in column %1. Please remove it first." )
				  .arg( nColumn ) );
		return;
	}
	
	m_tags.push_back( std::make_shared<Tag>(nColumn, sTag) );
	sortTags();
}

void Timeline::deleteTag( int nColumn ) {

	// Erase the value to set the new value
	if ( m_tags.size() >= 1 ){
		for ( int t = 0; t < m_tags.size(); t++ ){
			if ( m_tags[t]->nColumn == nColumn ) {
				m_tags.erase( m_tags.begin() +  t);
			}
		}
	}
		
	sortTags();
}

const QString Timeline::getTagAtColumn( int nColumn ) const {

	QString sCurrentTag("");

	for ( int t = 0; t < static_cast<int>(m_tags.size()); t++ ){
		if ( m_tags[t]->nColumn > nColumn ){
			break;
		}
		sCurrentTag = m_tags[t]->sTag;
	}

	return sCurrentTag;
}

bool Timeline::hasColumnTag( int nColumn ) const {
	for ( const auto& tt : m_tags ){
		if ( tt->nColumn == nColumn ) {
			return true;
		}
	}
	return false;
}

void Timeline::sortTags()
{
	//sort the timeline vector to beats a < b
	sort(m_tags.begin(), m_tags.end(), TagComparator());
}

QString Timeline::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[Timeline]\n" ).arg( sPrefix )
			.append( QString( "%1%2m_fDefaultBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( m_fDefaultBpm ) )
			.append( QString( "%1%2m_tempoMarkers:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& tt : m_tempoMarkers ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( "%1[column: %2 , bpm: %3]\n" ).arg( sPrefix + s + s ).arg( tt->nColumn ).arg( tt->fBpm ) );
			}
		}
		sOutput.append( QString( "%1%2m_allTempoMarkers:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& tt : m_allTempoMarkers ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( "%1[column: %2 , bpm: %3]\n" )
								.arg( sPrefix + s + s ).arg( tt->nColumn ).arg( tt->fBpm ) );
			}
		}
		sOutput.append( QString( "%1%2m_tags:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& tt : m_tags ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( "%1[column: %2 , tag: %3]\n" ).arg( sPrefix + s + s ).arg( tt->nColumn ).arg( tt->sTag ) );
			}
		}
	} else {
		
		sOutput = QString( "%1[Timeline] " ).arg( sPrefix )
			.append( QString( "m_fDefaultBpm: %1, " ).arg( m_fDefaultBpm ) )
			.append( QString( "m_tempoMarkers: [" ) );
		for ( const auto& tt : m_tempoMarkers ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( " [column: %1 , bpm: %2]" ).arg( tt->nColumn ).arg( tt->fBpm ) );
			}
		}
		sOutput.append( QString( "], m_allTempoMarkers: [" ) );
		for ( const auto& tt : m_allTempoMarkers ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( " [column: %1 , bpm: %2]" )
								.arg( tt->nColumn ).arg( tt->fBpm ) );
			}
		}
		sOutput.append( QString( "], m_tags: [" ) );
		for ( const auto& tt : m_tags ) {
			if ( tt != nullptr ) {
				sOutput.append( QString( " [column: %1 , tag: %2]" ).arg( tt->nColumn ).arg( tt->sTag ) );
			}
		}
		sOutput.append(" ]");
	}
		
	return sOutput;
}

QString Timeline::TempoMarker::getPrettyString( int nDecimals ) const {

	int nPrec = 7;
	if ( nDecimals >= 0 ) {
		nPrec = std::min( nDecimals + ( fBpm >= 100 ? 3 : 2 ),
						  7 );
	}
	return QString::number( fBpm, 'g', nPrec );
}

QString Timeline::TempoMarker::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[TempoMarker]\n" ).arg( sPrefix )
			.append( QString( "%1%2nColumn: %3\n" ).arg( sPrefix ).arg( s ).arg( nColumn ) )
			.append( QString( "%1%2fBpm: %3\n" ).arg( sPrefix ).arg( s ).arg( fBpm ) );
	} else {
		
		sOutput = QString( "%1[TempoMarker] " ).arg( sPrefix )
			.append( QString( "nColumn: %3, " ).arg( nColumn ) )
			.append( QString( "fBpm: %3" ).arg( fBpm ) );
	}
		
	return sOutput;
}

QString Timeline::Tag::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[TempoMarker]\n" ).arg( sPrefix )
			.append( QString( "%1%2nColumn: %3\n" ).arg( sPrefix ).arg( s ).arg( nColumn ) )
			.append( QString( "%1%2sTag: %3\n" ).arg( sPrefix ).arg( s ).arg( sTag ) );
	} else {
		
		sOutput = QString( "%1[TempoMarker] " ).arg( sPrefix )
			.append( QString( "nColumn: %3, " ).arg( nColumn ) )
			.append( QString( "sTag: %3" ).arg( sTag ) );
	}
		
	return sOutput;
}

};

