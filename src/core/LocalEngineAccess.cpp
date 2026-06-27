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

#include <core/LocalEngineAccess.h>

#include <memory>

#include <core/Hydrogen.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/IO/AudioDriver.h>
#include <core/IO/CoreAudioDriver.h>
#include <core/IO/DiskWriterDriver.h>
#include <core/IO/JackDriver.h>
#include <core/IO/MidiBaseDriver.h>
#include <core/IO/StubAudioDriver.h>
#include <core/IO/SoftwareDriver.h>
#include <core/IO/OssDriver.h>
#include <core/IO/PortAudioDriver.h>
#include <core/IO/PulseAudioDriver.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {

// All of these read the live audio driver and resolve its concrete type
// engine-side (ADR 0029). The GUI consumes the value views below — it never
// holds or `dynamic_cast`s a driver pointer, so the same GUI code works against
// an IPC-backed engine (#IpcEngineAccess) where the driver lives in another
// process.

AudioDriverInfo LocalEngineAccess::getAudioDriverInfo() const {
	AudioDriverInfo info;
	const auto pDriver = m_pHydrogen->getAudioDriver();
	if ( pDriver == nullptr ) {
		// kind=None, isPresent=false, isRunning=false
		return info;
	}
	info.isPresent = true;
	// A StubAudioDriver — including a selected-but-not-compiled backend stub deriving
	// from it — means no real audio output is connected.
	info.isRunning =
		std::dynamic_pointer_cast<StubAudioDriver>( pDriver ) == nullptr;

	// The consolidated software driver (ADR 0031) clocks the engine but may be
	// headless (no real output). Its producesAudio flag is the authoritative
	// "running" signal: false → headless (former Null), reported as kind Null.
	if ( const auto pSw = std::dynamic_pointer_cast<SoftwareDriver>( pDriver ) ) {
		info.isRunning = pSw->getProducesAudio();
		info.kind = pSw->getProducesAudio() ?
			Preferences::AudioDriver::Fake : Preferences::AudioDriver::Null;
		return info;
	}

	// Resolve the concrete kind. Each cast is guarded by the same H2CORE_HAVE_*
	// flag the GUI used, so a selected-but-not-compiled backend falls through to
	// Null below.
#ifdef H2CORE_HAVE_JACK
	if ( std::dynamic_pointer_cast<JackDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::Jack;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_ALSA
	if ( const auto pAlsa =
			 std::dynamic_pointer_cast<AlsaAudioDriver>( pDriver ) ) {
		info.kind = Preferences::AudioDriver::Alsa;
		info.connectedDevice = pAlsa->m_sAlsaAudioDevice;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_PORTAUDIO
	if ( std::dynamic_pointer_cast<PortAudioDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::PortAudio;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_COREAUDIO
	if ( std::dynamic_pointer_cast<CoreAudioDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::CoreAudio;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_PULSEAUDIO
	if ( std::dynamic_pointer_cast<PulseAudioDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::PulseAudio;
		return info;
	}
#endif
#ifdef H2CORE_HAVE_OSS
	if ( std::dynamic_pointer_cast<OssDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::Oss;
		return info;
	}
#endif
	if ( std::dynamic_pointer_cast<DiskWriterDriver>( pDriver ) != nullptr ) {
		info.kind = Preferences::AudioDriver::Disk;
		return info;
	}

	// Present but unmatched: the StubAudioDriver fallback (or a backend not surfaced
	// in the UI, e.g. the plugin/fake drivers).
	info.kind = Preferences::AudioDriver::Null;
	return info;
}

int LocalEngineAccess::getAudioSampleRate() const {
	const auto pDriver = m_pHydrogen->getAudioDriver();
	return pDriver != nullptr ? static_cast<int>( pDriver->getSampleRate() ) : 0;
}

int LocalEngineAccess::getAudioBufferSize() const {
	const auto pDriver = m_pHydrogen->getAudioDriver();
	return pDriver != nullptr ? static_cast<int>( pDriver->getBufferSize() ) : 0;
}

int LocalEngineAccess::getAudioLatencyFrames() const {
	const auto pDriver = m_pHydrogen->getAudioDriver();
	return pDriver != nullptr ? pDriver->getLatency() : 0;
}

int LocalEngineAccess::getAudioXRuns() const {
	const auto pDriver = m_pHydrogen->getAudioDriver();
	return pDriver != nullptr ? pDriver->getXRuns() : 0;
}

QStringList LocalEngineAccess::getAudioDevices(
	Preferences::AudioDriver kind, const QString& sHostAPI ) const {
	(void)kind;
	(void)sHostAPI;
#ifdef H2CORE_HAVE_ALSA
	// ALSA devices are enumerated statically, independent of the running driver.
	if ( kind == Preferences::AudioDriver::Alsa ) {
		return AlsaAudioDriver::getAlsaDevices();
	}
#endif
	const auto pDriver = m_pHydrogen->getAudioDriver();
	if ( pDriver == nullptr ) {
		return QStringList();
	}
#ifdef H2CORE_HAVE_PORTAUDIO
	if ( kind == Preferences::AudioDriver::PortAudio ) {
		if ( const auto pPortAudio =
				 std::dynamic_pointer_cast<PortAudioDriver>( pDriver ) ) {
			return pPortAudio->getDevices( sHostAPI );
		}
		return QStringList();
	}
#endif
#ifdef H2CORE_HAVE_COREAUDIO
	if ( kind == Preferences::AudioDriver::CoreAudio ) {
		return pDriver->getDevices();
	}
#endif
	return QStringList();
}

QStringList LocalEngineAccess::getAudioHostAPIs() const {
#ifdef H2CORE_HAVE_PORTAUDIO
	const auto pDriver = m_pHydrogen->getAudioDriver();
	if ( const auto pPortAudio =
			 std::dynamic_pointer_cast<PortAudioDriver>( pDriver ) ) {
		return pPortAudio->getHostAPIs();
	}
#endif
	return QStringList();
}

bool LocalEngineAccess::isExportWritingFailed() const {
	const auto pDriver = std::dynamic_pointer_cast<DiskWriterDriver>(
		m_pHydrogen->getAudioDriver() );
	return pDriver != nullptr && pDriver->writingFailed();
}

MidiDriverInfo LocalEngineAccess::getMidiDriverInfo() const {
	MidiDriverInfo info;
	const auto pDriver = m_pHydrogen->getMidiDriver();
	if ( pDriver != nullptr ) {
		info.isPresent = true;
		info.isInputActive = pDriver->isInputActive();
		info.isOutputActive = pDriver->isOutputActive();
	}
	return info;
}

std::vector<QString> LocalEngineAccess::getMidiPorts(
	MidiBaseDriver::PortType portType ) const {
	const auto pDriver = m_pHydrogen->getMidiDriver();
	if ( pDriver != nullptr ) {
		return pDriver->getExternalPortList( portType );
	}
	return std::vector<QString>();
}

std::vector<std::shared_ptr<MidiInput::HandledInput>>
LocalEngineAccess::getHandledMidiInputs() const {
	const auto pDriver = m_pHydrogen->getMidiDriver();
	if ( pDriver != nullptr ) {
		return pDriver->getHandledInputs();
	}
	return std::vector<std::shared_ptr<MidiInput::HandledInput>>();
}

std::vector<std::shared_ptr<MidiOutput::HandledOutput>>
LocalEngineAccess::getHandledMidiOutputs() const {
	const auto pDriver = m_pHydrogen->getMidiDriver();
	if ( pDriver != nullptr ) {
		return pDriver->getHandledOutputs();
	}
	return std::vector<std::shared_ptr<MidiOutput::HandledOutput>>();
}

}
