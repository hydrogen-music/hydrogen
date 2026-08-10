/*
 * Hydrogen
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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/IPC/HeadlessEngineLauncher.h>

#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

#include <QtCore/QCoreApplication>

namespace H2Core {

Hydrogen* HeadlessEngineLauncher::createHeadlessEngine()
{
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Null;
	pPref->m_midiDriver = Preferences::MidiDriver::None;
	pPref->setOscServerEnabled( false );

	auto pHydrogen = new Hydrogen( pPref, Hydrogen::ProcessMode::Headless, -1 );
	pHydrogen->setFullyOperational( true );

	return pHydrogen;
}

QString HeadlessEngineLauncher::makeEndpoint()
{
	static std::atomic<unsigned> s_nCounter{ 0 };
	return QString( "hydrogen-headless-%1-%2" )
		.arg( QCoreApplication::applicationPid() )
		.arg( s_nCounter.fetch_add( 1 ) );
}

QString HeadlessEngineLauncher::formatConnectionInfo( const QString& sEndpoint )
{
	QString sInfo;
	sInfo += "========================================\n";
	sInfo += "IPC Server Started\n";
	sInfo += "========================================\n";
	sInfo += QString( "Endpoint: %1\n" ).arg( sEndpoint );
	sInfo += "\n";
	sInfo += "To connect the Hydrogen GUI, run:\n";
	sInfo += QString( "  hydrogen -c %1\n" ).arg( sEndpoint );
	sInfo += QString( "  hydrogen --connect-via-ipc %1\n" ).arg( sEndpoint );
	sInfo += "\n";
	sInfo += "Press Ctrl+C to stop the server\n";
	sInfo += "========================================\n";
	return sInfo;
}

} // namespace H2Core
