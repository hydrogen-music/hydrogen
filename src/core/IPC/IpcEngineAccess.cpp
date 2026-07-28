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

int IpcEngineAccess::getSelectedPatternNumber() const {
	if ( m_pChannel != nullptr ) {
		IpcMessage reply;
		if ( m_pChannel->request( IpcMessage( IpcOpcode::GetSelectedPattern ),
								  reply ) ) {
			const auto& args = reply.getArgs();
			if ( ! args.isEmpty() ) {
				return args[0].toInt();
			}
		}
	}
	// Fall back to the mirror's (possibly stale) value on failure.
	return m_pMirror->getSelectedPatternNumber();
}

int IpcEngineAccess::getSelectedInstrumentNumber() const {
	if ( m_pChannel != nullptr ) {
		IpcMessage reply;
		if ( m_pChannel->request( IpcMessage( IpcOpcode::GetSelectedInstrument ),
								  reply ) ) {
			const auto& args = reply.getArgs();
			if ( ! args.isEmpty() ) {
				return args[0].toInt();
			}
		}
	}
	return m_pMirror->getSelectedInstrumentNumber();
}

bool IpcEngineAccess::getRecordEnabled() const {
	if ( m_pChannel != nullptr ) {
		IpcMessage reply;
		if ( m_pChannel->request( IpcMessage( IpcOpcode::GetRecordEnabled ),
								  reply ) ) {
			const auto& args = reply.getArgs();
			if ( ! args.isEmpty() ) {
				return args[0].toBool();
			}
		}
	}
	return m_pMirror->getRecordEnabled();
}

};
