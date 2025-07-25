/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Playlist.h>
#include <core/config.h>
#include <core/EventQueue.h>
#include <core/Globals.h>
#include <core/H2Exception.h>
#include <core/Helpers/Filesystem.h>
#include <core/Helpers/Translations.h>
#include <core/Hydrogen.h>
#include <core/Logger.h>
#include <core/Midi/MidiActionManager.h>
#ifdef H2CORE_HAVE_OSC
  #include <core/NsmClient.h>
#endif
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>
#include <core/Version.h>

#include "HydrogenApp.h"
#include "MainForm.h"
#include "Parser.h"
#include "MainToolBar/MainToolBar.h"
#include "Reporter.h"
#include "ShotList.h"
#include "Skin.h"
#include "SplashScreen.h"

#ifdef HAVE_EXECINFO_H
  #include <execinfo.h>
#endif
#include <getopt.h>
#include <iostream>
#include <map>
#include <set>
#include <signal.h>
#ifdef WIN32
  #include <stdio.h>
  #include <windows.h>
#endif

#include <QLibraryInfo>
#include <QProcess>
#include <QSslSocket>
#ifndef H2CORE_HAVE_QT6
  #include <QTextCodec>
#endif
#include <QtGui>
#include <QtWidgets>

namespace H2Core {
	void init_gui_object_map();
};

// Handle a fatal signal, allowing the logger to complete any outstanding messages before re-raising the
// signal to allow normal termination.
static void handleFatalSignal( int nSignal )
{
	// First disable signal handler to allow normal termination
	signal( nSignal, SIG_DFL );

	// Report current context to the crash reporter
	Reporter::report();

	___ERRORLOG( QString( "Fatal signal %1" ).arg( nSignal ) );

#ifdef HAVE_EXECINFO_H
	// Print out stack backtrace if we can
	const int nMaxFrames = 128;
	void *frames[ nMaxFrames ];
	int nFrames = backtrace( frames, nMaxFrames );
	char **symbols = backtrace_symbols( frames, nFrames );
	for ( int i = 0; i < nFrames; i++ ) {
		___ERRORLOG( QString("%1").arg( symbols[i] ) );
	}
#endif

	// Allow logger to complete
	H2Core::Logger* pLogger = H2Core::Logger::get_instance();
	if ( pLogger ) {
		delete pLogger;
	}

	raise( nSignal );
}

static int setup_unix_signal_handlers()
{
#ifndef WIN32
	struct sigaction usr1;

	usr1.sa_handler = MainForm::usr1SignalHandler;
	sigemptyset(&usr1.sa_mask);
	usr1.sa_flags = 0;
	usr1.sa_flags |= SA_RESTART;

	if (sigaction(SIGUSR1, &usr1, nullptr) > 0) {
		return 1;
	}

#endif

	for ( int nSignal : { SIGSEGV, SIGILL, SIGFPE, SIGABRT,
#ifndef WIN32
						 SIGBUS, SIGTRAP
#endif
		} ) {
		signal( nSignal, handleFatalSignal );
	}

	return 0;
}

static void setApplicationIcon(QApplication *app)
{
	QIcon icon;
	icon.addFile(Skin::getImagePath() + "/icon16.png", QSize(16, 16));
	icon.addFile(Skin::getImagePath() + "/icon24.png", QSize(24, 24));
	icon.addFile(Skin::getImagePath() + "/icon32.png", QSize(32, 32));
	icon.addFile(Skin::getImagePath() + "/icon48.png", QSize(48, 48));
	icon.addFile(Skin::getImagePath() + "/icon64.png", QSize(64, 64));
	app->setWindowIcon(icon);
}


// QApplication class.
class H2QApplication : public QApplication {

	QString m_sInitialFileOpen;
	QWidget *m_pMainForm;

public:
	H2QApplication( int &argc, char **argv )
		: QApplication(argc, argv) {
		m_pMainForm = nullptr;
	}

	bool event( QEvent *e ) override
	{
		if ( e->type() == QEvent::FileOpen ) {
			QFileOpenEvent *fe = dynamic_cast<QFileOpenEvent*>( e );
			assert( fe != nullptr );

			if ( m_pMainForm ) {
				// Forward to MainForm if it's initialised and ready to handle a FileOpenEvent.
				QApplication::sendEvent( m_pMainForm, e );
			} else  {
				// Keep requested file until ready
				m_sInitialFileOpen = fe->file();
			}
			return true;
		}
		return QApplication::event( e );
	}

	// Set the MainForm pointer and forward any requested open event.
	void setMainForm( QWidget *pMainForm )
	{
		m_pMainForm = pMainForm;
		if ( !m_sInitialFileOpen.isEmpty() ) {
			QFileOpenEvent ev( m_sInitialFileOpen );
			QApplication::sendEvent( m_pMainForm, &ev );

		}
	}
};




int main(int argc, char *argv[])
{

#ifdef WIN32
	// In case Hydrogen was started using a CLI attach its output to
	// the latter. 
	if ( AttachConsole(ATTACH_PARENT_PROCESS)) {
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		freopen("CONIN$", "w", stdin);
	}
#endif
	Reporter::spawn( argc, argv );
	try {
#ifndef H2CORE_HAVE_QT6
  #if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
		QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  #endif
#endif
		// Create bootstrap QApplication to get H2 Core set up with correct Filesystem paths before starting GUI application.
		QCoreApplication *pBootStrApp = new QCoreApplication( argc, argv );

		Parser parser;
		if ( ! parser.parse( argc, argv ) ) {
			std::cerr << "Error: Unable to parse CLI arguments. Abort..."
					  << std::endl;
			exit( 1 );
		}

		QString sSongFilename = parser.getSongFilename();

		std::cout << H2Core::getAboutText().toStdString();
		
		setup_unix_signal_handlers();
		QString sInitialisingCrashContext( "Initialising Hydrogen" );
		H2Core::Logger::setCrashContext( &sInitialisingCrashContext );

		// Man your battle stations... this is not a drill.
		auto pLogger = H2Core::Logger::bootstrap(
			parser.getLogLevel(), parser.getLogFile(), true,
			parser.getLogTimestamps(), parser.getLogColors() );
		H2Core::Base::bootstrap(
			pLogger, pLogger->should_log( H2Core::Logger::Debug ) );

		H2Core::Filesystem::bootstrap(
			pLogger, parser.getSysDataPath(), parser.getUsrDataPath(),
			parser.getConfigFilePath(), parser.getLogFile() );
		H2Core::Preferences::create_instance();
		// See below for H2Core::Hydrogen.

		___INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
#ifdef H2CORE_HAVE_QT6
		const QString sEncoding = QLocale::system().name();
#else
		const QString sEncoding = QTextCodec::codecForLocale()->name();
#endif
		___INFOLOG( QString( "System encoding: [%1]" ).arg( sEncoding ) );
		___INFOLOG( "Using data path: " + H2Core::Filesystem::sys_data_path() );

		auto pPref = H2Core::Preferences::get_instance();
		pPref->setH2ProcessName( QString(argv[0]) );

#if QT_VERSION >= QT_VERSION_CHECK( 5, 14, 0)
		/* Apply user-specified rounding policy. This is mostly to handle non-integral factors on Windows. */
		Qt::HighDpiScaleFactorRoundingPolicy policy;

		switch ( pPref->getTheme().m_interface.m_uiScalingPolicy ) {
		case H2Core::InterfaceTheme::ScalingPolicy::Smaller:
			policy = Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor;
			break;
		case H2Core::InterfaceTheme::ScalingPolicy::System:
			policy = Qt::HighDpiScaleFactorRoundingPolicy::PassThrough;
			break;
		case H2Core::InterfaceTheme::ScalingPolicy::Larger:
			policy = Qt::HighDpiScaleFactorRoundingPolicy::Ceil;
			break;
		}
		QGuiApplication::setHighDpiScaleFactorRoundingPolicy( policy );
#endif

		// Force layout
		if ( ! parser.getUiLayout().isEmpty() ) {
			if ( parser.getUiLayout() == "tabbed" ) {
				pPref->getThemeWritable().m_interface.m_layout =
					H2Core::InterfaceTheme::Layout::Tabbed;
			} else {
				pPref->getThemeWritable().m_interface.m_layout =
					H2Core::InterfaceTheme::Layout::SinglePane;
			}
		}

		if ( parser.getOscPort() != -1 ) {
			pPref->m_nOscTemporaryPort = parser.getOscPort();
		}

		if( ! parser.getInstallDrumkitPath().isEmpty() ){
			if ( ! H2Core::Drumkit::install( parser.getInstallDrumkitPath() ) ) {
				___ERRORLOG( QString( "Unable to install drumkit [%1]" )
							 .arg( parser.getInstallDrumkitPath() ) );
				exit( 1  );
			}
			exit( 0 );
		}

		if ( ! parser.getAudioDriver().isEmpty() ) {
			pPref->m_audioDriver =
				H2Core::Preferences::parseAudioDriver( parser.getAudioDriver() );
		}

		delete pBootStrApp;
		H2QApplication* pQApp = new H2QApplication( argc, argv );
		pQApp->setApplicationName( "Hydrogen" );
		pQApp->setApplicationVersion( QString::fromStdString( H2Core::get_version() ) );

		// Process any pending events before showing splash screen. This allows macOS to show previous-crash
		// warning dialogs before they are covered by the splash screen.
		pQApp->processEvents();

		QString family = pPref->getTheme().m_font.m_sApplicationFontFamily;
		pQApp->setFont( QFont( family, 10 ) );

		QTranslator qttor( nullptr );
		QTranslator tor( nullptr );
		QLocale locale = QLocale::system();
		if ( locale != QLocale::c() ) {
			QStringList languages;
			QString sPreferredLanguage = pPref->getPreferredLanguage();
			if ( !sPreferredLanguage.isNull() ) {
				languages << sPreferredLanguage;
			}
			languages << locale.uiLanguages();
			if ( H2Core::Translations::loadTranslation( languages, qttor, QString( "qt" ),
#ifdef H2CORE_HAVE_QT6
														QLibraryInfo::path(QLibraryInfo::TranslationsPath) ) ) {
#else
														QLibraryInfo::location(QLibraryInfo::TranslationsPath) ) ) {
#endif
				pQApp->installTranslator( &qttor );
			} else {
				___INFOLOG( QString("Warning: No Qt translation for locale %1 found.").arg(locale.name()));
			}
			
			QString sTranslationPath = H2Core::Filesystem::i18n_dir();
			QString sTranslationFile( "hydrogen" );
			bool bTransOk = H2Core::Translations::loadTranslation( languages, tor, sTranslationFile, sTranslationPath );
			if (bTransOk) {
				___INFOLOG( "Using locale: " + sTranslationPath );
			} else {
				___INFOLOG( "Warning: no locale found: " + sTranslationPath );
			}
			if (tor.isEmpty()) {
				___INFOLOG( "Warning: error loading locale: " + sTranslationPath );
			}
		}
		pQApp->installTranslator( &tor );

		const QString sStyle = pPref->getTheme().m_interface.m_sQTStyle;
		if ( !sStyle.isEmpty() ) {
			pQApp->setStyle( sStyle );
		}

		Skin::setPalette( pQApp );
		setApplicationIcon( pQApp );

		// This must be done after creating a QApplication instance
		// and setting its locale as some Qt standard keys are used as
		// defaults.
		if ( pPref->getShortcuts()->requiresDefaults() ) {
			pPref->getShortcuts()->createDefaultShortcuts();
		}

		if ( ! pPref->getLoadingSuccessful() ) {

			QMessageBox::critical( nullptr, "Hydrogen",
				QString( QT_TRANSLATE_NOOP( "Startup",															   "No [hydrogen.conf] file found. Hydrogen was not installed properly. Aborting..." ) ) );
			
			// Neither the Preferences on system level nor the ones at
			// user level could be loaded successfully. Hydrogen was
			// most probably not installed properly. Abort.
			delete pQApp;

			___ERRORLOG( "No preferences file found. Aborting..." );
			delete H2Core::Logger::get_instance();

			exit( 1 );
		}

#ifdef H2CORE_HAVE_APPIMAGE
		// Within an AppImage we provide our own version of Qt. But we
		// also have to ensure to provide an OpenSSL shared object
		// compatible with that particular version. As shipping
		// `libssl.so` and `libcrypto.so` is discouraged and
		// blacklisted by `linuxdeploy-qt`, we manually put them in
		// the application folder. This way Qt tries to load the
		// system's libraries first and only falls back to the ones we
		// provide in case it couldn't find it.
		//
		// With the following logs we can determine if a fallback did
		// happen.
		
		QString sCurrentDir = QDir::currentPath();
		QDir::setCurrent( QCoreApplication::applicationDirPath() );
		QProcess process;
		process.start( "openssl", QStringList( "version" ));
		process.waitForFinished(-1);
		QString sStdout = process.readAllStandardOutput();
		___INFOLOG( QString( "current: %1, application: %2" ).arg( sCurrentDir )
					.arg( QCoreApplication::applicationDirPath() ) );
		___INFOLOG( QString( "OpenSSL supported: [%1], version OS: [%2], Qt runtime: [%3], Qt build: [%4]" )
					.arg( QSslSocket::supportsSsl() )
					.arg( sStdout.trimmed() )
					.arg( QSslSocket::sslLibraryVersionString() )
					.arg( QSslSocket::sslLibraryBuildVersionString() ) );
		QDir::setCurrent( sCurrentDir );
#endif


		SplashScreen *pSplash = new SplashScreen();

#ifdef H2CORE_HAVE_OSC
		// Check for being under session management without the
		// NsmClient class available yet.
		if ( parser.getNoSplashScreen() ||  getenv( "NSM_URL" ) ) {
			pSplash->hide();
		}
		else {
			pSplash->show();
		}
#endif
#ifndef H2CORE_HAVE_OSC
		pSplash->show();
#endif

		// Hydrogen here to honor all preferences.
		H2Core::Hydrogen::create_instance();
		auto pHydrogen = H2Core::Hydrogen::get_instance();
		
		pHydrogen->startNsmClient();

		// If the NSM_URL variable is present, Hydrogen will not
		// initialize the audio driver and leaves this to the callback
		// function nsm_open_cb of the NSM client (which will be
		// called by now). However, the presence of the environmental
		// variable does not guarantee for a session management and if
		// no audio driver is initialized yet, we will do it here. 
		if ( pHydrogen->getAudioOutput() == nullptr ) {
			// Starting drivers can take some time, so show the wait cursor to let the user know that, yes,
			// we're definitely busy.
			QApplication::setOverrideCursor( Qt::WaitCursor );
			pHydrogen->restartAudioDriver();
			QApplication::restoreOverrideCursor();
		}

		MainForm *pMainForm =
			new MainForm( pQApp, sSongFilename,
						  parser.getPlaylistFilename() );
		auto pHydrogenApp = HydrogenApp::get_instance();
		pMainForm->show();
		// Update visibility button states.
		pHydrogenApp->getMainToolBar()->updateActions();
	
		pSplash->finish( pMainForm );

		if( ! parser.getDrumkitToLoad().isEmpty() ) {
			H2Core::CoreActionController::setDrumkit( parser.getDrumkitToLoad() );
		}

		pQApp->setMainForm( pMainForm );

		// Tell the core that the GUI is now fully loaded and ready.
		pHydrogen->setGUIState( H2Core::Hydrogen::GUIState::ready );

		if ( ! parser.getShotList().isEmpty() ) {
			ShotList *sl = new ShotList( parser.getShotList() );
			sl->shoot();
		}

		// All GUI setup is complete, any spurious widget-driven
		// flagging of song modified state will be complete, so clear
		// the modification flag. This does not apply in case we are
		// restoring unsaved changes applied to an empty song during
		// the previous session.
		if ( pHydrogen->getSong()->getFilename() !=
			 H2Core::Filesystem::empty_path(
				 H2Core::Filesystem::Type::Song ) ) {
#ifdef H2CORE_HAVE_OSC
			// Mark empty song created in a new NSM session modified
			// in order to emphasis that an initial song save is
			// required to generate the song file and link the
			// associated drumkit in the session folder.
			if ( NsmClient::get_instance() != nullptr &&
				 NsmClient::get_instance()->getIsNewSession() ) {
				
				NsmClient::get_instance()->sendDirtyState( true );
				pHydrogen->setIsModified( true );
			}
			else {
				NsmClient::get_instance()->sendDirtyState( false );
				pHydrogen->setIsModified( false );
			}
#else
			pHydrogen->setIsModified( false );
#endif
		}

		H2Core::Logger::setCrashContext( nullptr );

		// The Preferences is provided as a shared pointer. We discard our local
		// reference in order to not prevent cleanup to the old instance in case
		// it is replaced while Hydrogen is running.
		pPref = nullptr;

		pQApp->exec();

		QString sShutdownCrashContext( "Shutting down Hydrogen" );
		H2Core::Logger::setCrashContext( &sShutdownCrashContext );

		// When cancelling the PreferencesDialog, Hydrogen will revert to the
		// previous state of Preferences by reading the user-level hydrogen.conf
		// file again and creating a new instance.
		pPref = H2Core::Preferences::get_instance();

		pPref->save();
		delete pSplash;
		delete pMainForm;
		delete pQApp;
		delete H2Core::EventQueue::get_instance();

		delete MidiActionManager::get_instance();

		___INFOLOG( "Quitting..." );
		std::cout << "\nBye..." << std::endl;

		// There is no particular need to clean up the Preferences outselves.
		// This is just done in order for it to not appear in the objects map
		// printed below.
		pPref->replaceInstance( nullptr );
		pPref = nullptr;

		delete H2Core::Logger::get_instance();

		if (H2Core::Base::count_active()) {
			H2Core::Base::write_objects_map_to_cerr();
		}

	}
	catch ( const H2Core::H2Exception& ex ) {
		std::cerr << "[main] Exception: " << ex.what() << std::endl;
		return 1;
	}
	catch (...) {
		std::cerr << "[main] Unknown exception X-(" << std::endl;
		return 1;
	}

	return 0;
}

