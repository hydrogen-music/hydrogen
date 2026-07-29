/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

// Editor-mode assert guard integration test (ADR 0033).
//
// Stands up the engine side exactly as a plugin host (DAW) does — a
// HydrogenPlugin that owns a headless engine and serves it over IPC via
// EngineSession — and then launches the real Hydrogen GUI out-of-process
// with --exercise-editor-paths. In that mode the GUI connects via IPC,
// enters ProcessMode::Editor, and the EditorPathExerciser dispatches every
// safe Shortcuts::Action through MainForm::executeShortcut.
//
// If any action reaches an ASSERT_NO_EDITOR_MODE site, the macro calls
// assert(false) → SIGABRT, killing the GUI process. The test detects this as
// a non-zero/signal exit and fails. A clean (zero) exit means no shortcut
// path hit an editor-mode assert.
//
// Usage: plugin_editor_assert_guard <hydrogen-binary> <data-dir>

#include <plugin/HydrogenPlugin.h>

#include <core/Helpers/Filesystem.h>
#include <core/Logger.h>
#include <core/Object.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QThread>

#include <iostream>

static constexpr int knCleanFailure = 3;

static int fail( const QString& sMsg ) {
	std::cerr << "FAIL: " << sMsg.toStdString() << std::endl;
	return 1;
}

int main( int argc, char** argv ) {
	if ( argc < 3 ) {
		std::cerr << "usage: " << argv[0]
				  << " <hydrogen-binary> <data-dir>\n"
				  << "Spawns the GUI in editor mode with --exercise-editor-paths\n"
				  << "to check no shortcut action hits an ASSERT_NO_EDITOR_MODE."
				  << std::endl;
		return 2;
	}
	const QString sEditorBinary = QString::fromLocal8Bit( argv[1] );
	const QString sDataDir = QString::fromLocal8Bit( argv[2] );

	QCoreApplication app( argc, argv );

	auto* pLogger = H2Core::Logger::bootstrap(
		H2Core::Logger::Error | H2Core::Logger::Warning | H2Core::Logger::Info |
			H2Core::Logger::Debug | H2Core::Logger::Ipc,
		"", true, true
	);
	H2Core::Base::bootstrap( pLogger, false );
	H2Core::Filesystem::bootstrap( pLogger, sDataDir );

	// Host role: serve the engine over IPC without spawning the editor.
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

	// Editor role: launch the real GUI in editor mode with the exerciser.
	QProcess editor;
	editor.setProcessChannelMode( QProcess::ForwardedChannels );
	editor.start( sEditorBinary, QStringList()
				  << QStringLiteral( "--nosplash" )
				  << QStringLiteral( "--child" )
				  << QStringLiteral( "-V" ) << QStringLiteral( "Ipc" )
				  << QStringLiteral( "--exercise-editor-paths" )
				  << QStringLiteral( "--connect-via-ipc" ) << sEndpoint
				  << QStringLiteral( "-P" ) << sDataDir );

	if ( ! editor.waitForStarted( 15000 ) ) {
		return fail( QString( "could not start editor process [%1]: %2" )
						 .arg( sEditorBinary )
						 .arg( editor.errorString() ) );
	}
	std::cout << "host: launched editor [" << sEditorBinary.toStdString()
			  << "], waiting for it to attach and exercise all paths..."
			  << std::endl;

	// The exerciser dispatches ~80+ actions with 50ms spacing, plus 500ms
	// startup delay. Allow generous time for completion.
	if ( ! editor.waitForFinished( 120000 ) ) {
		editor.kill();
		editor.waitForFinished( 5000 );
		return fail( "editor did not complete within the timeout" );
	}

	const QProcess::ExitStatus status = editor.exitStatus();
	const int nExitCode = editor.exitCode();

	plugin.closeEditor();

	if ( status != QProcess::NormalExit ) {
		return fail( QString( "editor crashed (terminated by signal), "
							  "exit code %1 — an ASSERT_NO_EDITOR_MODE was hit" )
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

	std::cout << "OK: editor exercised all safe shortcut paths in editor mode "
				 "without hitting any ASSERT_NO_EDITOR_MODE" << std::endl;
	return 0;
}
