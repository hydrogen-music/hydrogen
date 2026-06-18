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

#include <core/IPC/IpcEngineBridge.h>

#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/IPC/IpcChannel.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

namespace H2Core {

bool IpcEngineBridge::dispatchCommand( const IpcMessage& msg,
									   Hydrogen* pHydrogen ) {
	if ( pHydrogen == nullptr ) {
		return false;
	}
	auto pController = pHydrogen->getCoreActionController();
	if ( pController == nullptr ) {
		return false;
	}
	const QVector<QVariant>& args = msg.getArgs();

	switch ( msg.getOpcode() ) {
	case IpcOpcode::Play:
		pHydrogen->sequencerPlay();
		return true;
	case IpcOpcode::Stop:
		pHydrogen->sequencerStop();
		return true;
	case IpcOpcode::Quit:
		return pController->quit();
	case IpcOpcode::SetBpm:
		if ( args.size() >= 1 ) {
			return pController->setBpm( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::SetMasterVolume:
		if ( args.size() >= 1 ) {
			return pController->setMasterVolume( args[0].toFloat() );
		}
		return false;
	case IpcOpcode::SetMasterIsMuted:
		if ( args.size() >= 1 ) {
			return pController->setMasterIsMuted( args[0].toBool() );
		}
		return false;
	case IpcOpcode::SetMetronomeIsActive:
		if ( args.size() >= 1 ) {
			return pController->setMetronomeIsActive( args[0].toBool() );
		}
		return false;
	case IpcOpcode::LocateToColumn:
		if ( args.size() >= 1 ) {
			return pController->locateToColumn( args[0].toInt() );
		}
		return false;
	case IpcOpcode::LocateToTick:
		if ( args.size() >= 2 ) {
			return pController->locateToTick(
				static_cast<long>( args[0].toLongLong() ), args[1].toBool() );
		}
		return false;
	case IpcOpcode::SelectPattern:
		if ( args.size() >= 1 ) {
			return pController->selectPattern( args[0].toInt() );
		}
		return false;
	case IpcOpcode::SetStripVolume:
		if ( args.size() >= 3 ) {
			return pController->setStripVolume(
				args[0].toInt(), args[1].toFloat(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::SetStripPan:
		if ( args.size() >= 3 ) {
			return pController->setStripPan(
				args[0].toInt(), args[1].toFloat(), args[2].toBool() );
		}
		return false;
	case IpcOpcode::ActivateLoopMode:
		if ( args.size() >= 1 ) {
			return pController->activateLoopMode( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ActivateSongMode:
		if ( args.size() >= 1 ) {
			return pController->activateSongMode( args[0].toBool() );
		}
		return false;
	case IpcOpcode::ActivateRecordMode:
		if ( args.size() >= 1 ) {
			return pController->activateRecordMode( args[0].toBool() );
		}
		return false;
	case IpcOpcode::AddTempoMarker:
		if ( args.size() >= 2 ) {
			return pController->addTempoMarker( args[0].toInt(), args[1].toFloat() );
		}
		return false;
	case IpcOpcode::AddTag:
		if ( args.size() >= 2 ) {
			return pController->addTag( args[0].toInt(), args[1].toString() );
		}
		return false;
	case IpcOpcode::NewPattern:
		if ( args.size() >= 1 ) {
			return pController->newPattern( args[0].toString() );
		}
		return false;
	case IpcOpcode::SetSong: {
		auto pSong = Song::fromXmlBuffer( msg.getPayload(), "<ipc>", true,
										  pHydrogen );
		if ( pSong == nullptr ) {
			return false;
		}
		return pController->setSong( pSong );
	}
	case IpcOpcode::RescanSoundLibrary:
		if ( pHydrogen->getSoundLibraryDatabase() != nullptr ) {
			pHydrogen->getSoundLibraryDatabase()->update();
			return true;
		}
		return false;
	default:
		return false; // Hello / Event / unknown are not engine commands
	}
}

bool IpcEngineBridge::forwardEvent( IpcChannel& channel, Event::Type type,
									int nValue, long nId ) {
	if ( ! isEngineOriginEvent( type ) ) {
		return false; // editor-internal: stays in the editor process
	}
	channel.send( IpcMessage::fromEvent( type, nValue, nId ) );
	return true;
}

};
