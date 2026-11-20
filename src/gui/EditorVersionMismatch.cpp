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

// Version-gate integration test (ADR 0018).
//
// The hello handshake carries the version identity of both peers, and
// compatibility is decided by the wire-protocol tag alone (the application
// version rides along as diagnostics only). This exercises the editor side
// of the gate end to end, process boundary and all: a fake engine (an
// IpcServer on its own thread) answers the real GUI's hello with a
// correlated hello announcing a DIFFERENT wire-protocol version, and the
// real GUI is launched out-of-process with `--connect-via-ipc` against that
// endpoint.
//
// Expected outcome: EditorSession::connect() returns nullptr and main()
// aborts with Reporter::EXIT_CODE_CLEAN_FAILURE (3) — the same clean path a
// dead endpoint takes (EditorBadEndpoint), proving the refusal is graceful:
// no crash, no half-wired GUI. An exit 0 would mean the editor wired itself
// around an incompatible engine — the exact regression this test guards.
//
// Usage: editor_version_mismatch <hydrogen-binary> <data-dir>

#include <core/Helpers/Filesystem.h>
#include <core/Logger.h>
#include <core/Object.h>
#include <core/IPC/IpcChannel.h>
#include <core/IPC/IpcMessage.h>
#include <core/IPC/IpcServer.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QThread>

#include <atomic>
#include <future>
#include <iostream>

// Keep in sync with Reporter::EXIT_CODE_CLEAN_FAILURE: the editor's clean
// "could not connect / incompatible engine" abort code.
static constexpr int knCleanFailure = 3;

static int fail( const QString& sMsg ) {
	std::cerr << "FAIL: " << sMsg.toStdString() << std::endl;
	return 1;
}

// Engine stand-in announcing an incompatible wire protocol: a serve loop on
// its own thread (a QLocalSocket is thread-affine) that answers every hello
// with a correlated hello carrying a different protocol tag. Nothing else
// of the engine protocol is needed — the editor never gets past the
// handshake.
class MismatchedEngine : public QThread {
public:
	explicit MismatchedEngine( const QString& sEndpoint )
		: QThread(), m_sEndpoint( sEndpoint ) {}
	~MismatchedEngine() override { stop(); }

	// Serve on the endpoint; blocks until the listener is up so the
	// spawned editor cannot race the bind.
	bool startServing() {
		m_bRunning = true;
		QThread::start();
		return m_ready.get_future().get();
	}
	void stop() {
		if ( ! isRunning() ) {
			return;
		}
		m_bRunning = false;
		QThread::wait();
	}

	void run() override {
		H2Core::IpcServer server;
		if ( ! server.listen( m_sEndpoint ) ) {
			m_ready.set_value( false );
			return;
		}
		m_ready.set_value( true );

		H2Core::IpcChannel* pConn = nullptr;
		while ( m_bRunning.load() ) {
			if ( pConn == nullptr ) {
				pConn = server.waitForChannel( 10 );
				continue;
			}
			H2Core::IpcMessage msg;
			if ( pConn->receive( msg, 10, false ) &&
				 msg.getOpcode() == H2Core::IpcOpcode::Hello ) {
				// A wire-protocol tag ahead of whatever this build speaks.
				H2Core::IpcHelloVersion version =
					H2Core::IpcHelloVersion::local();
				version.nProtocolVersion += 1;
				H2Core::IpcMessage reply = H2Core::IpcMessage::hello( version );
				reply.setRequestId( msg.getRequestId() );
				pConn->send( reply );
			}
			if ( ! pConn->isConnected() ) {
				delete pConn;
				pConn = nullptr;
			}
		}
		delete pConn;
	}

private:
	QString m_sEndpoint;  // set before the thread starts
	std::atomic<bool> m_bRunning{ false };
	std::promise<bool> m_ready;
};

int main( int argc, char** argv ) {
	if ( argc < 3 ) {
		std::cerr << "usage: " << argv[0]
				  << " <hydrogen-binary> <data-dir>" << std::endl;
		return 2;
	}
	const QString sEditorBinary = QString::fromLocal8Bit( argv[1] );
	const QString sDataDir = QString::fromLocal8Bit( argv[2] );

	// QProcess and the IPC local sockets both need a QCoreApplication in the
	// process. We never run its event loop: the fake engine serves on its
	// own thread and the QProcess waits below are synchronous.
	QCoreApplication app( argc, argv );

	// Just enough core for the IPC classes' logging (same bootstrap order
	// the other harnesses use), pointed at the shipped data dir.
	auto* pLogger = H2Core::Logger::bootstrap(
		H2Core::Logger::Error | H2Core::Logger::Warning, "",
		H2Core::Logger::Option::UseStdout | H2Core::Logger::Option::Timestamps |
			H2Core::Logger::Option::Colors );
	H2Core::Base::bootstrap( pLogger, false );
	H2Core::Filesystem::bootstrap( pLogger, sDataDir );

	// ── Engine role: serve an incompatible protocol ─────────────────────────
	const QString sEndpoint = QString( "h2-version-mismatch-%1" )
		.arg( QCoreApplication::applicationPid() );
	MismatchedEngine engine( sEndpoint );
	if ( ! engine.startServing() ) {
		return fail( "could not bind the fake engine endpoint" );
	}
	std::cout << "host: serving an incompatible engine on endpoint ["
			  << sEndpoint.toStdString() << "]" << std::endl;

	// ── Editor role: the real GUI against that endpoint ─────────────────────
	// --quit-after-startup is a safety net for the regression case: an
	// editor that (wrongly) connects would exit 0 quickly instead of
	// hanging until the timeout. In the expected case the editor aborts at
	// the handshake, long before startup.
	QProcess editor;
	editor.setProcessChannelMode( QProcess::ForwardedChannels );
	editor.start( sEditorBinary, QStringList()
				  << QStringLiteral( "--nosplash" )
				  << QStringLiteral( "--child" )
				  << QStringLiteral( "--quit-after-startup" )
				  << QStringLiteral( "--connect-via-ipc" ) << sEndpoint
				  << QStringLiteral( "-P" ) << sDataDir );

	if ( ! editor.waitForStarted( 15000 ) ) {
		engine.stop();
		return fail( QString( "could not start editor process [%1]: %2" )
						 .arg( sEditorBinary )
						 .arg( editor.errorString() ) );
	}
	std::cout << "host: launched editor, waiting for the refused handshake "
				 "to abort it..." << std::endl;

	if ( ! editor.waitForFinished( 90000 ) ) {
		editor.kill();
		editor.waitForFinished( 5000 );
		engine.stop();
		return fail( "editor did not exit within the timeout (it connected "
					 "and ran instead of being refused?)" );
	}

	const QProcess::ExitStatus status = editor.exitStatus();
	const int nExitCode = editor.exitCode();
	engine.stop();

	if ( status != QProcess::NormalExit ) {
		return fail( QString( "editor crashed (terminated by signal), exit "
							 "code %1" ).arg( nExitCode ) );
	}
	if ( nExitCode == 0 ) {
		return fail( "editor connected to an engine with an incompatible "
					 "IPC protocol — the gate did not refuse" );
	}
	if ( nExitCode != knCleanFailure ) {
		return fail( QString( "editor exited with unexpected code %1 "
							 "(expected the clean-failure abort %2)" )
						 .arg( nExitCode ).arg( knCleanFailure ) );
	}

	std::cout << "OK: editor refused the incompatible engine and aborted "
				 "cleanly (exit " << knCleanFailure << ")" << std::endl;
	return 0;
}
