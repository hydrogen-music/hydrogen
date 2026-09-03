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

#include <plugin/HydrogenPlugin.h>

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Helpers/H2Project.h>
#include <core/Hydrogen.h>
#include <core/IO/PluginAudioDriver.h>
#include <core/IO/PluginMidiDriver.h>
#include <core/IPC/EngineSession.h>
#include <core/IPC/HeadlessEngineLauncher.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/Preferences.h>

#include <atomic>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

namespace H2Core {

static std::shared_ptr<Preferences> makePluginPreferences( double fSampleRate,
														   unsigned nMaxBlockSize ) {
	auto pPref = Preferences::create_instance();
	pPref->m_audioDriver = Preferences::AudioDriver::Plugin;
	pPref->m_midiDriver = Preferences::MidiDriver::Plugin;
	pPref->m_nBufferSize = nMaxBlockSize;
	pPref->m_nSampleRate = static_cast<int>( fSampleRate );
	pPref->setOscServerEnabled( false );
	return pPref;
}

HydrogenPlugin::HydrogenPlugin( double fSampleRate, unsigned nMaxBlockSize,
								int nBuses )
	: m_pHydrogen( nullptr )
	, m_pAudioDriver( nullptr )
	, m_pMidiDriver( nullptr )
	, m_nBuses( nBuses ) {
	m_pHydrogen = new Hydrogen(
		makePluginPreferences( fSampleRate, nMaxBlockSize ),
		H2Core::ProcessMode::Headless, -1
	);
	m_pHydrogen->setFullyOperational( true );

	m_pAudioDriver = std::dynamic_pointer_cast<PluginAudioDriver>(
		m_pHydrogen->getAudioDriver() );
	m_pMidiDriver = std::dynamic_pointer_cast<PluginMidiDriver>(
		m_pHydrogen->getMidiDriver() );

	if ( m_pAudioDriver != nullptr ) {
		m_pAudioDriver->setSampleRate( static_cast<unsigned>( fSampleRate ) );
	}

	// The empty song is set directly by the Hydrogen ctor, which (unlike
	// setSong()) does not load the kit's samples; load them so notes render.
	if ( m_pHydrogen->getSong() != nullptr &&
		 m_pHydrogen->getSong()->getDrumkit() != nullptr ) {
		m_pHydrogen->getSong()->getDrumkit()->loadSamples(
			120, m_pHydrogen->getPreferences().get() );
	}
}

HydrogenPlugin::~HydrogenPlugin() {
	// Tear down the editor (process + serve loop) before the engine it serves.
	closeEditor();
	m_pMidiDriver.reset();
	m_pAudioDriver.reset();
	delete m_pHydrogen;
	m_pHydrogen = nullptr;
}

void HydrogenPlugin::activate( double fSampleRate, unsigned /*nMaxBlockSize*/ ) {
	if ( m_pAudioDriver != nullptr ) {
		m_pAudioDriver->setSampleRate( static_cast<unsigned>( fSampleRate ) );
	}
}

void HydrogenPlugin::deactivate() {
}

void HydrogenPlugin::process( uint32_t nFrames, float* pMasterL, float* pMasterR,
							  const std::vector<float*>& busOut_L,
							  const std::vector<float*>& busOut_R,
							  bool bRolling, double fBpm, long long nFrame ) {
	if ( m_pHydrogen == nullptr || m_pAudioDriver == nullptr ) {
		return;
	}

	m_pAudioDriver->setHostBuffers( pMasterL, pMasterR, nFrames );
	m_pAudioDriver->setHostTransport( bRolling, fBpm, nFrame );
	m_pAudioDriver->setBusBuffers( busOut_L, busOut_R );

	if ( m_pMidiDriver != nullptr ) {
		m_pMidiDriver->dispatchHostEvents();
	}

	AudioEngine::audioEngine_process( nFrames, m_pHydrogen );
}

void HydrogenPlugin::noteOn( int nKey, int nVelocity, int nChannel,
							 int nSampleOffset ) {
	if ( m_pMidiDriver == nullptr ) {
		return;
	}
	m_pMidiDriver->enqueueHostEvent(
		MidiMessage( MidiMessage::Type::NoteOn,
					 Midi::parameterFromIntClamp( nKey ),
					 Midi::parameterFromIntClamp( nVelocity ),
					 static_cast<Midi::Channel>( nChannel ) ),
		nSampleOffset );
}

void HydrogenPlugin::noteOff( int nKey, int nChannel, int nSampleOffset ) {
	if ( m_pMidiDriver == nullptr ) {
		return;
	}
	m_pMidiDriver->enqueueHostEvent(
		MidiMessage( MidiMessage::Type::NoteOff,
					 Midi::parameterFromIntClamp( nKey ),
					 Midi::ParameterMinimum,
					 static_cast<Midi::Channel>( nChannel ) ),
		nSampleOffset );
}

void HydrogenPlugin::controlChange( int nParameter, int nValue, int nChannel,
									int nSampleOffset ) {
	if ( m_pMidiDriver == nullptr ) {
		return;
	}
	m_pMidiDriver->enqueueHostEvent(
		MidiMessage( MidiMessage::Type::ControlChange,
					 Midi::parameterFromIntClamp( nParameter ),
					 Midi::parameterFromIntClamp( nValue ),
					 static_cast<Midi::Channel>( nChannel ) ),
		nSampleOffset );
}

int HydrogenPlugin::getBusCount() const {
	return m_nBuses;
}

std::vector<unsigned char> HydrogenPlugin::saveState( bool bEmbedSamples ) {
	if ( m_pHydrogen == nullptr || m_pHydrogen->getSong() == nullptr ) {
		return {};
	}
	return H2Project::toState( m_pHydrogen->getSong(), bEmbedSamples, true );
}

bool HydrogenPlugin::loadState( const std::vector<unsigned char>& data ) {
	if ( m_pHydrogen == nullptr ) {
		return false;
	}
	auto pSong = H2Project::fromState( data, m_pHydrogen, true );
	if ( pSong == nullptr ) {
		return false;
	}
	return m_pHydrogen->getCoreActionController()->setSong( pSong );
}

// ── Out-of-process editor lifecycle (ADR 0016) ─────────────────────────────

namespace {
// Qt's QProcess / local-socket classes need a QCoreApplication in the process.
// A standalone build and the unit tests already have one (QApplication /
// test main); a bare (non-Qt) plugin host does not, so create a minimal one the
// first time we need it. Left to leak — it is process-global and lives as long
// as the plugin library is loaded.
void ensureQtApplication() {
	if ( QCoreApplication::instance() != nullptr ) {
		return;
	}
	static int s_argc = 1;
	static char s_arg0[] = "hydrogen-plugin";
	static char* s_argv[] = { s_arg0, nullptr };
	new QCoreApplication( s_argc, s_argv );
}
} // namespace

QString HydrogenPlugin::resolveEditorBinary( const QString& sExplicit,
											 const QString& sSearchDir ) {
	if ( ! sExplicit.isEmpty() ) {
		return sExplicit;
	}
	const QByteArray sEnv = qgetenv( "HYDROGEN_EDITOR_PATH" );
	if ( ! sEnv.isEmpty() ) {
		return QString::fromLocal8Bit( sEnv );
	}

#if defined( _WIN32 )
	const QString sExe = QStringLiteral( "hydrogen.exe" );
#else
	const QString sExe = QStringLiteral( "hydrogen" );
#endif

	// Look for the editor relative to the installed plugin. The plugin can sit at
	// various depths (dev build dir, or e.g. <prefix>/lib/{clap,vst3,lv2}/... while
	// the editor is at <prefix>/bin), so walk up a few levels and, at each, look
	// for the editor next to it or under a sibling bin/ (T6.2 install layout).
	if ( ! sSearchDir.isEmpty() ) {
		QDir dir( sSearchDir );
		for ( int nLevel = 0; nLevel < 6; ++nLevel ) {
			const QStringList candidates = {
				dir.filePath( sExe ),                       // same dir
				dir.filePath( QStringLiteral( "bin/" ) + sExe ), // sibling bin/
#if defined( __APPLE__ )
				dir.filePath( "Hydrogen.app/Contents/MacOS/Hydrogen" ),
#endif
			};
			for ( const QString& sCandidate : candidates ) {
				const QFileInfo info( sCandidate );
				if ( info.isFile() && info.isExecutable() ) {
					return info.absoluteFilePath();
				}
			}
			if ( ! dir.cdUp() ) {
				break;
			}
		}
	}

	// Last resort: rely on PATH (or a packaging-set HYDROGEN_EDITOR_PATH /
	// setEditorBinary()).
	return sExe;
}

QString HydrogenPlugin::editorBinary() const {
	return resolveEditorBinary( m_sEditorBinary, m_sEditorSearchDir );
}

bool HydrogenPlugin::openEditor( bool bLaunchProcess ) {
	if ( m_bEditorOpen ) {
		return true;
	}
	if ( m_pHydrogen == nullptr ) {
		return false;
	}

	ensureQtApplication();

	if ( m_sEditorEndpoint.isEmpty() ) {
		m_sEditorEndpoint = QString( "hydrogen-editor-%1-%2" )
								.arg( QCoreApplication::applicationPid() )
								.arg( m_pHydrogen->getInstanceId() );
	}
	m_pEditorSession = EngineSession::start( m_pHydrogen, m_sEditorEndpoint );
	if ( m_pEditorSession == nullptr ) {
		m_sEditorEndpoint.clear();
		return false;
	}

	m_bEditorOpen = true;
	m_bEditorClosing = false;
	m_nEditorRespawns = 0;
	if ( bLaunchProcess ) {
		launchEditorProcess();
	}
	return true;
}

void HydrogenPlugin::launchEditorProcess() {
	m_pEditorProcess = std::make_unique<QProcess>();
	QObject::connect(
		m_pEditorProcess.get(),
		QOverload<int, QProcess::ExitStatus>::of( &QProcess::finished ),
		[this]( int, QProcess::ExitStatus status ) {
			onEditorProcessFinished( status == QProcess::CrashExit );
		} );
	m_pEditorProcess->start(
		editorBinary(),
		QStringList() << QStringLiteral( "--connect-via-ipc" ) << m_sEditorEndpoint );
}

void HydrogenPlugin::onEditorProcessFinished( bool bCrashed ) {
	if ( m_bEditorClosing || ! m_bEditorOpen ) {
		return; // deliberate close — not a crash
	}
	if ( ! bCrashed ) {
		// The user closed the editor window. Leave the engine running and the
		// serve loop accepting, so the host can reopen the editor later.
		m_bEditorOpen = false;
		return;
	}
	// The editor crashed. The engine keeps running and the serve loop keeps
	// accepting (ADR 0016); respawn a bounded number of times so a persistently
	// crashing editor can't loop forever.
	if ( m_nEditorRespawns < knMaxEditorRespawns ) {
		++m_nEditorRespawns;
		launchEditorProcess();
	}
	else {
		m_bEditorOpen = false;
	}
}

bool HydrogenPlugin::isEditorProcessRunning() const {
	return m_pEditorProcess != nullptr &&
		m_pEditorProcess->state() != QProcess::NotRunning;
}

void HydrogenPlugin::closeEditor() {
	m_bEditorClosing = true;
	if ( m_pEditorProcess != nullptr ) {
		if ( m_pEditorProcess->state() != QProcess::NotRunning ) {
			m_pEditorProcess->terminate();
			if ( ! m_pEditorProcess->waitForFinished( 2000 ) ) {
				m_pEditorProcess->kill();
				m_pEditorProcess->waitForFinished( 2000 );
			}
		}
		m_pEditorProcess.reset();
	}
	// Stops the serve loop and joins the bridge thread.
	m_pEditorSession.reset();
	m_sEditorEndpoint.clear();
	m_bEditorOpen = false;
	m_bEditorClosing = false;
	m_nEditorRespawns = 0;
}

};
