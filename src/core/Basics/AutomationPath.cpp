/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Basics/AutomationPath.h>
#include <core/Basics/Song.h>

namespace H2Core {

AutomationPath::AutomationPath( float fMin, float fMax, float fDef )
	: Object(), m_fMin( fMin ), m_fMax( fMax ), m_fDef( fDef )
{
}

std::shared_ptr<AutomationPath>
AutomationPath::loadFrom( const XMLNode& node, bool bSilent )
{
	auto pPath = std::make_shared<AutomationPath>();

	auto point = node.firstChildElement();
	while ( !point.isNull() ) {
		if ( point.tagName() == "point" ) {
			bool hasX = false;
			bool hasY = false;
			float x = point.attribute( "x" ).toFloat( &hasX );
			float y = point.attribute( "y" ).toFloat( &hasY );

			if ( hasX && hasY ) {
				pPath->addPoint( x, y );
			}
		}
		point = point.nextSiblingElement();
	}

	return pPath;
}

void AutomationPath::saveTo( XMLNode& node, bool bSilent ) const
{
	for ( const auto& point : m_points ) {
		auto element = node.ownerDocument().createElement( "point" );
		element.setAttribute( "x", point.first );
		element.setAttribute( "y", point.second );
		node.appendChild( element );
	}
}

/**
 * \brief Get value at given location
 * \param x Location
 *
 * If location is between points, value is computed
 **/
float AutomationPath::getValue( float x ) const noexcept
{
	if ( m_points.empty() ) {
		return m_fDef;
	}

	auto f = m_points.begin();
	if ( x <= f->first ) {
		return f->second;
	}

	auto l = m_points.rbegin();
	if ( x >= l->first ) {
		return l->second;
	}

	auto i = m_points.lower_bound( x );
	auto p1 = *i;
	auto p0 = *( --i );
	float x1 = p0.first;
	float y1 = p0.second;
	float x2 = p1.first;
	float y2 = p1.second;

	float d = ( x - x1 ) / ( x2 - x1 );

	return y1 + ( y2 - y1 ) * d;
}

/**
 * \brief Add a point to path
 * \param x X coordinate
 * \param y Y coordinate
 **/
void AutomationPath::addPoint( float x, float y )
{
	m_points[x] = y;
}

/**
 * \brief Compare two paths
 *
 * Two paths are considered equal, if they have the same settings
 * (min, max, default) and points in the same places.
 */
bool operator==( const AutomationPath& lhs, const AutomationPath& rhs )
{
	return lhs.m_fMin == rhs.m_fMin && lhs.m_fMax == rhs.m_fMax &&
		   lhs.m_fDef == rhs.m_fDef && lhs.m_points == rhs.m_points;
}

bool operator!=( const AutomationPath& lhs, const AutomationPath& rhs )
{
	return !( lhs == rhs );
}

QString AutomationPath::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput =
			QString( "%1[AutomationPath]\n" )
				.arg( sPrefix )
				.append( QString( "%1%2m_uuid: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( getUuid().toQString() ) )
				.append( QString( "%1%2m_fMin: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_fMin ) )
				.append( QString( "%1%2m_fMax: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_fMax ) )
				.append( QString( "%1%2m_fDef: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_fDef ) )
				.append( QString( "%1%2m_points:\n" ).arg( sPrefix ).arg( s ) );
		for ( const auto& pp : m_points ) {
			sOutput.append( QString( "%1%2%3 : %4\n" )
								.arg( sPrefix )
								.arg( s )
								.arg( pp.first )
								.arg( pp.second ) );
		}
	}
	else {
		sOutput =
			QString( "[AutomationPath]" )
				.append( QString( " m_uuid: %1" ).arg( getUuid().toQString() ) )
				.append( QString( ", m_fMin: %1" ).arg( m_fMin ) )
				.append( QString( ", m_fMax: %1" ).arg( m_fMax ) )
				.append( QString( ", m_fDef: %1" ).arg( m_fDef ) )
				.append( QString( ", m_points: [" ) );
		for ( const auto& pp : m_points ) {
			sOutput.append(
				QString( "(%1: %4) " ).arg( pp.first ).arg( pp.second )
			);
		}
		sOutput.append( "]" );
	}

	return sOutput;
}

/**
 * \brief Find point near specific location
 *
 * If point is faound, iterator pointing to it is returned.
 * Otherwise, AutomationPath::end() is returned.
 **/
AutomationPath::iterator AutomationPath::find( float x )
{
	const float limit = 0.5f;

	if ( m_points.empty() ) {
		return m_points.end();
	}

	auto i = m_points.lower_bound( x );

	if ( i != m_points.end() ) {
		if ( i->first - x <= limit ) {
			return i;
		}
	}

	/* If there is a point before, check whether
	 * it is a close match */
	if ( i != m_points.begin() ) {
		--i;
		if ( x - i->first <= limit ) {
			return i;
		}
	}

	return m_points.end();
}

/**
 * \brief Move point to other location
 * \param in Iterator pointing to point to be moved
 * \param x Destination X coordinate
 * \param y Destination Y coordinate
 **/
AutomationPath::iterator
AutomationPath::move( iterator& in, float x, float y )
{
	m_points.erase( in );
	auto rv = m_points.insert( std::make_pair( x, y ) );
	return rv.first;
}

/**
 * \brief Remove point from path
 * \param x Point location
 **/
void AutomationPath::removePoint( float x )
{
	auto it = find( x );
	if ( it != m_points.end() ) {
		m_points.erase( it );
	}
}

}  // namespace H2Core
