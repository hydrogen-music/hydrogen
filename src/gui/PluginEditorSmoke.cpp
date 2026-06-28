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

// Plugin-editor attach integration test (ADR 0016/0018).
//
// This is the success counterpart to the EditorBadEndpoint smoke test. It stands
// up the engine side exactly as a plugin host (DAW) does — a HydrogenPlugin that
// owns a headless engine and serves it over IPC via EngineSession — and then
// launches the *real* Hydrogen GUI in out-of-process editor mode
// (`hydrogen --plugin-editor <endpoint>`) against that live endpoint, just like
// HydrogenPlugin::launchEditorProcess() would when the host calls "show GUI".
//
// The one deviation from the production spawn is that we add `--quit-after-startup`
// so the editor, once it has connected, built MainForm over the IPC-backed engine
// handle and shown the window, schedules an immediate clean quit (the same trick
// GuiStartup uses). A clean (zero) exit therefore proves the whole chain works:
//   spawn  ->  connect to endpoint  ->  build the GUI over the IPC mirror  ->
//   show the window  ->  tear everything down without crashing.
// If the editor cannot connect it exits with Reporter::EXIT_CODE_CLEAN_FAILURE
// (3) instead, and a crash shows up as a signal/non-zero — both fail the test.
//
// We drive the QProcess ourselves (rather than calling openEditor(true)) only so
// we can pass `--quit-after-startup`; the engine side still goes through the exact
// production serve path (openEditor(false) starts EngineSession on its bridge
// thread without spawning).
//
// Usage: plugin_editor_smoke <hydrogen-binary> <data-dir>

#include <plugin/HydrogenPlugin.h>

#include <core/Helpers/Filesystem.h>
#include <core/Logger.h>
#include <core/Object.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <iostream>

// Keep in sync with Reporter::EXIT_CODE_CLEAN_FAILURE: the editor's "could not
// connect to the engine endpoint" abort. Seeing it here means the engine was not
// serving / the editor failed to attach — a real failure for this test.
static constexpr int knCleanFailure = 3;

static int fail( const QString& sMsg ) {
	std::cerr << "FAIL: " << sMsg.toStdString() << std::endl;
	return 1;
}

int main( int argc, char** argv ) {
	if ( argc < 3 ) {
		std::cerr << "usage: " << argv[0]
				  << " <hydrogen-binary> <data-dir>" << std::endl;
		return 2;
	}
	const QString sEditorBinary = QString::fromLocal8Bit( argv[1] );
	const QString sDataDir = QString::fromLocal8Bit( argv[2] );

	// QProcess and the IPC local sockets both need a QCoreApplication in the
	// process. We never run its event loop: the EngineSession serve loop lives on
	// its own thread and the QProcess waits below are synchronous.
	QCoreApplication app( argc, argv );

	// Bring up just enough core to create an engine (same bootstrap order the GUI
	// and the unit-test runner use), pointed at the shipped data dir.
	auto* pLogger = H2Core::Logger::bootstrap(
		H2Core::Logger::Error | H2Core::Logger::Warning, "", true, true );
	H2Core::Base::bootstrap( pLogger, false );
	H2Core::Filesystem::bootstrap( pLogger, sDataDir );

	// ── Host role ──────────────────────────────────────────────────────────
	// Construct the engine wrapper a DAW loads, then start serving it over IPC
	// without spawning the editor (we spawn it ourselves below). This is the exact
	// production serve path: EngineSession::start() binds the endpoint on a bridge
	// thread.
	H2Core::HydrogenPlugin plugin( 44100, 1024, 0 );
	if ( ! plugin.openEditor( /*bLaunchProcess=*/false ) ) {
		return fail( "engine could not start serving the editor IPC endpoint" );
	}
	const QString sEndpoint = plugin.getEditorEndpoint();
	if ( sEndpoint.isEmpty() ) {
		return fail( "engine reported an empty editor endpoint" );
	}
	std::cout << "host: serving engine on endpoint [" << sEndpoint.toStdString()
			  << "]" << std::endl;

	// ── Editor role ────────────────────────────────────────────────────────
	// Launch the real GUI in editor mode against the live endpoint. --child skips
	// the crash Reporter wrapper; --quit-after-startup shows the window then quits
	// cleanly (and suppresses the development-build warning dialog that would
	// otherwise block headless). The platform is forced offscreen via the
	// environment (set by CTest).
	QProcess editor;
	editor.setProcessChannelMode( QProcess::ForwardedChannels );
	editor.start( sEditorBinary, QStringList()
				  << QStringLiteral( "--nosplash" )
				  << QStringLiteral( "--child" )
				  << QStringLiteral( "--quit-after-startup" )
				  << QStringLiteral( "--plugin-editor" ) << sEndpoint
				  << QStringLiteral( "-P" ) << sDataDir );

	if ( ! editor.waitForStarted( 15000 ) ) {
		return fail( QString( "could not start editor process [%1]: %2" )
						 .arg( sEditorBinary )
						 .arg( editor.errorString() ) );
	}
	std::cout << "host: launched editor [" << sEditorBinary.toStdString()
			  << "], waiting for it to attach, show and quit..." << std::endl;

	if ( ! editor.waitForFinished( 90000 ) ) {
		editor.kill();
		editor.waitForFinished( 5000 );
		return fail( "editor did not attach/show/quit within the timeout "
					 "(window never came up?)" );
	}

	const QProcess::ExitStatus status = editor.exitStatus();
	const int nExitCode = editor.exitCode();

	// Tear the engine side down cleanly (stops the serve loop, joins the bridge
	// thread); the plugin destructor then disposes of the engine.
	plugin.closeEditor();

	if ( status != QProcess::NormalExit ) {
		return fail( QString( "editor crashed (terminated by signal), exit code %1" )
						 .arg( nExitCode ) );
	}
	if ( nExitCode == knCleanFailure ) {
		return fail( "editor could not attach to the engine endpoint "
					 "(EXIT_CODE_CLEAN_FAILURE)" );
	}
	if ( nExitCode != 0 ) {
		return fail( QString( "editor exited with non-zero code %1" )
						 .arg( nExitCode ) );
	}

	std::cout << "OK: editor attached to the engine, showed its window and "
				 "exited cleanly; host torn down" << std::endl;
	return 0;
}
