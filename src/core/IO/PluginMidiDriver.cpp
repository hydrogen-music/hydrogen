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

#include <core/IO/PluginMidiDriver.h>

#include <algorithm>

namespace H2Core {

PluginMidiDriver::PluginMidiDriver( Hydrogen* pHydrogen )
	// Virtual bases must be initialized by the most-derived class (ADR 0015).
		: MidiInput( pHydrogen )
		, MidiOutput( pHydrogen )
		, MidiBaseDriver( pHydrogen )
		, m_bActive( false ) {
}

PluginMidiDriver::~PluginMidiDriver() {
	close();
}

void PluginMidiDriver::open() {
	m_bActive = true;
}

void PluginMidiDriver::close() {
	m_bActive = false;
	clearHostEvents();
}

std::vector<QString> PluginMidiDriver::getExternalPortList( const PortType& ) {
	// The host is the only "port"; there is nothing external to enumerate.
	return std::vector<QString>();
}

bool PluginMidiDriver::isInputActive() const {
	return m_bActive;
}

bool PluginMidiDriver::isOutputActive() const {
	return m_bActive;
}

void PluginMidiDriver::enqueueHostEvent( const MidiMessage& msg,
										 int nSampleOffset ) {
	m_hostEvents.push_back( { nSampleOffset, msg } );
}

void PluginMidiDriver::dispatchHostEvents() {
	if ( m_hostEvents.empty() ) {
		return;
	}

	// Stable-sort by sample offset so events fire in host order; ties keep
	// their enqueue order (e.g. a note-off before a note-on at the same frame).
	std::stable_sort( m_hostEvents.begin(), m_hostEvents.end(),
					  []( const HostEvent& a, const HostEvent& b ) {
						  return a.nSampleOffset < b.nSampleOffset;
					  } );

	for ( const auto& event : m_hostEvents ) {
		handleInputMessageSync( event.msg );
	}

	m_hostEvents.clear();
}

void PluginMidiDriver::clearHostEvents() {
	m_hostEvents.clear();
}

size_t PluginMidiDriver::getHostEventCount() const {
	return m_hostEvents.size();
}

QString PluginMidiDriver::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	if ( bShort ) {
		return QString( "[PluginMidiDriver] m_bActive: %1, queued: %2" )
			.arg( m_bActive ).arg( m_hostEvents.size() );
	}
	return QString( "%1[PluginMidiDriver]\n" ).arg( sPrefix )
		.append( QString( "%1%2m_bActive: %3\n" ).arg( sPrefix ).arg( s )
				 .arg( m_bActive ) )
		.append( QString( "%1%2queued host events: %3\n" ).arg( sPrefix ).arg( s )
				 .arg( m_hostEvents.size() ) );
}

};
