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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <core/IPC/IpcEngineAccess.h>

#include <core/IO/DiskWriterDriver.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcMessage.h>

namespace H2Core {

void IpcEngineAccess::sequencerPlay() {
	// Transport start is engine-authoritative: forward it to the real engine and
	// reflect it locally so the GUI's transport widgets update immediately.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::Play ) );
	}
	m_pMirror->sequencerPlay();
}

void IpcEngineAccess::sequencerStop() {
	// Transport stop is engine-authoritative: forward it to the real engine and
	// reflect it locally so the GUI's transport widgets update immediately.
	if ( m_pChannel != nullptr ) {
		m_pChannel->send( IpcMessage( IpcOpcode::Stop ) );
	}
	m_pMirror->sequencerStop();
}

void IpcEngineAccess::setSelectedInstrumentNumber(
	int nInstrument, Event::Trigger trigger )
{
	// Instrument selection is engine-relevant: the headless engine's
	// MIDI-to-selected-instrument routing follows it. Forward the change and
	// apply it locally so the GUI updates immediately; the engine's echo
	// event re-applies the same value on the mirror (idempotent).
	if ( m_pChannel != nullptr ) {
		m_pChannel->send(
			IpcMessage( IpcOpcode::SetSelectedInstrument ).arg( nInstrument ) );
	}
	m_pMirror->setSelectedInstrumentNumber( nInstrument, trigger );
}

QStringList IpcEngineAccess::getAudioDevices(
		Preferences::AudioDriver kind, const QString& sHostAPI
) const {
	QStringList devices;
	const auto info = m_pMirror->getAudioDriverInfo();

	if ( kind == info.kind && kind == Preferences::AudioDriver::PortAudio ) {
		if ( info.audioDevices.find( sHostAPI ) != info.audioDevices.end() ) {
			return info.audioDevices.at( sHostAPI );
		}
		else {
			ERRORLOG( QString( "Unknown Host API [%1]. Available ones [%2]" )
					  .arg( sHostAPI ).arg( info.hostApis.join( ", " ) ));
			return devices;
		}
	}
	else {
		if ( info.audioDevices.size() > 1 ) {
			WARNINGLOG( "More audio device nodes than expected." );
		}
		return info.audioDevices.begin()->second;
	}
}

bool IpcEngineAccess::isExportWritingFailed() const
{
	const auto pDriver =
		std::dynamic_pointer_cast<DiskWriterDriver>( m_pMirror->getAudioDriver()
		);
	return pDriver != nullptr && pDriver->writingFailed();
}
};
