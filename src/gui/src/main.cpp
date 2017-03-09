/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif
#include <QLibraryInfo>
#include <hydrogen/config.h>
#include <hydrogen/version.h>
#include <getopt.h>

#include "SplashScreen.h"
#include "HydrogenApp.h"
#include "MainForm.h"
#include "PlaylistEditor/PlaylistDialog.h"

#ifdef H2CORE_HAVE_LASH
#include <hydrogen/LashClient.h>
#endif
#ifdef H2CORE_HAVE_JACKSESSION
#include <jack/session.h>
#endif

#include <hydrogen/midi_map.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/globals.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/playlist.h>
#include <hydrogen/helpers/filesystem.h>

#include <signal.h>
#include <iostream>

//
// Set the palette used in the application
//
void setPalette( QApplication *pQApp )
{
	// create the default palette
	QPalette defaultPalette;

	// A general background color.
	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );

	// A general foreground color.
	defaultPalette.setColor( QPalette::Foreground, QColor( 255, 255, 255 ) );

	// Used as the background color for text entry widgets; usually white or another light color.
	defaultPalette.setColor( QPalette::Base, QColor( 88, 94, 112 ) );

	// Used as the alternate background color in views with alternating row colors
	defaultPalette.setColor( QPalette::AlternateBase, QColor( 138, 144, 162 ) );

	// The foreground color used with Base. This is usually the same as the Foreground, in which case it must provide good contrast with Background and Base.
	defaultPalette.setColor( QPalette::Text, QColor( 255, 255, 255 ) );

	// The general button background color. This background can be different from Background as some styles require a different background color for buttons.
	defaultPalette.setColor( QPalette::Button, QColor( 88, 94, 112 ) );

	// A foreground color used with the Button color.
	defaultPalette.setColor( QPalette::ButtonText, QColor( 255, 255, 255 ) );


	// Lighter than Button color.
	defaultPalette.setColor( QPalette::Light, QColor( 138, 144, 162 ) );

	// Between Button and Light.
	defaultPalette.setColor( QPalette::Midlight, QColor( 128, 134, 152 ) );

	// Darker than Button.
	defaultPalette.setColor( QPalette::Dark, QColor( 58, 62, 72 ) );

	// Between Button and Dark.
	defaultPalette.setColor( QPalette::Mid, QColor( 81, 86, 99 ) );

	// A very dark color. By default, the shadow color is Qt::black.
	defaultPalette.setColor( QPalette::Shadow, QColor( 255, 255, 255 ) );


	// A color to indicate a selected item or the current item.
	defaultPalette.setColor( QPalette::Highlight, QColor( 116, 124, 149 ) );

	// A text color that contrasts with Highlight.
	defaultPalette.setColor( QPalette::HighlightedText, QColor( 255, 255, 255 ) );

	pQApp->setPalette( defaultPalette );
	pQApp->setStyleSheet("QToolTip {padding: 1px; border: 1px solid rgb(199, 202, 204); background-color: rgb(227, 243, 252); color: rgb(64, 64, 66);}"); 
}


static int setup_unix_signal_handlers()
{
#ifndef WIN32
	struct sigaction usr1;

	usr1.sa_handler = MainForm::usr1SignalHandler;
	sigemptyset(&usr1.sa_mask);
	usr1.sa_flags = 0;
	usr1.sa_flags |= SA_RESTART;

	if (sigaction(SIGUSR1, &usr1, 0) > 0)
		return 1;

	return 0;
#endif
}

int main(int argc, char *argv[])
{
	try {	
		QApplication* pQApp = new QApplication( argc, argv );
		pQApp->setApplicationName( "Hydrogen" );
		pQApp->setApplicationVersion( QString::fromStdString( H2Core::get_version() ) );
		
		QCommandLineParser parser;
		
		QString aboutText = QString( "\nHydrogen " ) + QString::fromStdString( H2Core::get_version() )  + QString( " [" ) + QString::fromStdString( __DATE__ ) + QString( "]  [http://www.hydrogen-music.org]" ) + 
		QString( "\nCopyright 2002-2008 Alessandro Cominu\nCopyright 2008-2016 The hydrogen development team" ) + 
		QString( "\nHydrogen comes with ABSOLUTELY NO WARRANTY\nThis is free software, and you are welcome to redistribute it under certain conditions. See the file COPYING for details.\n" );
		
		parser.setApplicationDescription( aboutText );
		
		QCommandLineOption audioDriverOption( QStringList() << "d" << "driver", "Use the selected audio driver (jack, alsa, oss)", "Audiodriver");
		QCommandLineOption installDrumkitOption( QStringList() << "i" << "install", "Install a drumkit (*.h2drumkit)" , "File");
		QCommandLineOption noSplashScreenOption( QStringList() << "n" << "nosplash", "Hide splash screen" );
		QCommandLineOption playlistFileNameOption( QStringList() << "p" << "playlist", "Load a playlist (*.h2playlist) at startup", "File" );
		QCommandLineOption systemDataPathOption( QStringList() << "P" << "data", "Use an alternate system data path", "Path" );
		QCommandLineOption songFileOption( QStringList() << "s" << "song", "Load a song (*.h2song) at startup", "File" );
		QCommandLineOption kitOption( QStringList() << "k" << "kit", "Load a drumkit at startup", "DrumkitName" );
		QCommandLineOption verboseOption( QStringList() << "V" << "verbose", "Level, if present, may be None, Error, Warning, Info, Debug or 0xHHHH","Level");
		
		parser.addHelpOption();
		parser.addVersionOption();
		parser.addOption( audioDriverOption );
		parser.addOption( installDrumkitOption );
		parser.addOption( noSplashScreenOption );
		parser.addOption( playlistFileNameOption );
		parser.addOption( systemDataPathOption );
		parser.addOption( songFileOption );
		parser.addOption( kitOption );
		parser.addOption( verboseOption );
		
		
		//Conditional options
		#ifdef H2CORE_HAVE_JACKSESSION 
			QCommandLineOption jackSessionOption(QStringList() << "S" << "jacksessionid", "ID - Start a JackSessionHandler session");
			parser.addOption(jackSessionOption);
		#endif
			
		// Evaluate the options
		parser.process(*pQApp);
		QString sSelectedDriver = parser.value( audioDriverOption );
		QString sDrumkitName = parser.value( installDrumkitOption );
		bool	bNoSplash = parser.isSet( noSplashScreenOption );
		QString sPlaylistFilename = parser.value( playlistFileNameOption );
		QString sSysDataPath = parser.value( systemDataPathOption );
		QString sSongFilename = parser.value ( songFileOption );
		QString sDrumkitToLoad = parser.value( kitOption );
		QString sVerbosityString = parser.value( verboseOption );
		
		unsigned logLevelOpt = H2Core::Logger::Error;
		if( parser.isSet(verboseOption) ){
			if( !sVerbosityString.isEmpty() )
			{
				logLevelOpt =  H2Core::Logger::parse_log_level( sVerbosityString.toLocal8Bit() );
			} else {
				logLevelOpt = H2Core::Logger::Error|H2Core::Logger::Warning;
			}
		}
		
		#ifdef H2CORE_HAVE_JACKSESSION
				QString sessionId;
		#endif
		
		std::cout << aboutText.toStdString();
		
		setup_unix_signal_handlers();

		// Man your battle stations... this is not a drill.
		H2Core::Logger::create_instance();
		H2Core::Logger::set_bit_mask( logLevelOpt );
		H2Core::Logger* logger = H2Core::Logger::get_instance();
		H2Core::Object::bootstrap( logger, logger->should_log(H2Core::Logger::Debug) );
		if(sSysDataPath.length()==0 ) {
			H2Core::Filesystem::bootstrap( logger );
		} else {
			H2Core::Filesystem::bootstrap( logger, sSysDataPath );
		}
		MidiMap::create_instance();
		H2Core::Preferences::create_instance();
		// See below for H2Core::Hydrogen.


		___INFOLOG( QString("Using QT version ") + QString( qVersion() ) );
		___INFOLOG( "Using data path: " + H2Core::Filesystem::sys_data_path() );

		H2Core::Preferences *pPref = H2Core::Preferences::get_instance();
		pPref->setH2ProcessName( QString(argv[0]) );

#ifdef H2CORE_HAVE_LASH

		LashClient::create_instance("hydrogen", "Hydrogen", &argc, &argv);
		LashClient* lashClient = LashClient::get_instance();

#endif
		if( ! sDrumkitName.isEmpty() ){
			H2Core::Drumkit::install( sDrumkitName );
			exit(0);
		}
		
		if (sSelectedDriver == "auto") {
			pPref->m_sAudioDriver = "Auto";
		}
		else if (sSelectedDriver == "jack") {
			pPref->m_sAudioDriver = "Jack";
		}
		else if ( sSelectedDriver == "oss" ) {
			pPref->m_sAudioDriver = "Oss";
		}
		else if ( sSelectedDriver == "alsa" ) {
			pPref->m_sAudioDriver = "Alsa";
		}

		QString family = pPref->getApplicationFontFamily();
		pQApp->setFont( QFont( family, pPref->getApplicationFontPointSize() ) );

		QTranslator qttor( 0 );
		QTranslator tor( 0 );
		QString sTranslationFile = QString("hydrogen.") + QLocale::system().name();
		QString sLocale = QLocale::system().name();
		if ( sLocale != "C") {
			if (qttor.load( QString( "qt_" ) + sLocale,
				QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
				pQApp->installTranslator( &qttor );
			else
				___INFOLOG( QString("Warning: No Qt translation for locale %1 found.").arg(QLocale::system().name()));


			QString sTranslationPath = "data/i18n";
			QString total = sTranslationPath + "/" + sTranslationFile + ".qm";

			bool bTransOk = tor.load( total, "." );
			if ( bTransOk ) {
				___INFOLOG( QString( "Using locale: %1/%2" ).arg( sTranslationPath ).arg( sTranslationFile ) );
			}
			else {
				sTranslationPath = H2Core::Filesystem::i18n_dir();
				total = sTranslationPath + "/" + sTranslationFile + ".qm";
				bTransOk = tor.load( total, "." );
				if (bTransOk) {
					___INFOLOG( "Using locale: " + sTranslationPath + "/" + sTranslationFile );
				}
				else {
					___INFOLOG( "Warning: no locale found: " + sTranslationPath + "/" + sTranslationFile );
				}
			}
			if (tor.isEmpty()) {
				___INFOLOG( "Warning: error loading locale: " +  total );
			}
		}
		pQApp->installTranslator( &tor );

		QString sStyle = pPref->getQTStyle();
		if ( !sStyle.isEmpty() ) {
			pQApp->setStyle( sStyle );
		}

		setPalette( pQApp );

		SplashScreen *pSplash = new SplashScreen();

		if (bNoSplash) {
			pSplash->hide();
		}
		else {
			pSplash->show();
		}

#ifdef H2CORE_HAVE_LASH
		if ( H2Core::Preferences::get_instance()->useLash() ){
			if (lashClient->isConnected())
			{
				lash_event_t* lash_event = lashClient->getNextEvent();
				if (lash_event && lash_event_get_type(lash_event) == LASH_Restore_File)
				{
					// notify client that this project was not a new one
					lashClient->setNewProject(false);

					sSongFilename = "";
					sSongFilename.append( QString::fromLocal8Bit(lash_event_get_string(lash_event)) );
					sSongFilename.append("/hydrogen.h2song");

					//H2Core::Logger::get_instance()->log("[LASH] Restore file: " + sSongFilename);

					lash_event_destroy(lash_event);
				}
				else if (lash_event)
				{
					//H2Core::Logger::get_instance()->log("[LASH] ERROR: Instead of restore file got event: " + lash_event_get_type(lash_event));
					lash_event_destroy(lash_event);
				}
			}
		}
#endif

#ifdef H2CORE_HAVE_JACKSESSION
		if(!sessionId.isEmpty()){
			pPref->setJackSessionUUID( sessionId );

			/*
			 * imo, jack sessions use jack as default audio driver.
			 * hydrogen remember last used audiodriver.
			 * here we make it save that hydrogen start in a jacksession case
			 * every time with jack as audio driver
			 */
			pPref->m_sAudioDriver = "Jack";

		}

		/*
		 * the use of applicationFilePath() make it
		 * possible to use different executables.
		 * for example if you start hydrogen from a local
		 * build directory.
		 */

		QString path = pQApp->applicationFilePath();
		pPref->setJackSessionApplicationPath( path );
#endif

		// Hydrogen here to honor all preferences.
		H2Core::Hydrogen::create_instance();

#ifdef H2CORE_HAVE_OSC
		H2Core::Hydrogen::get_instance()->startNsmClient();

		QString NsmSongFilename = pPref->getNsmSongName();

		if(!NsmSongFilename.isEmpty())
		{
			sSongFilename = NsmSongFilename;
		}
#endif

		MainForm *pMainForm = new MainForm( pQApp, sSongFilename );
		pMainForm->show();
		pSplash->finish( pMainForm );

		if( ! sPlaylistFilename.isEmpty() ){
			bool loadlist = HydrogenApp::get_instance()->getPlayListDialog()->loadListByFileName( sPlaylistFilename );
			if ( loadlist ){
				Playlist::get_instance()->setNextSongByNumber( 0 );
			} else {
				___ERRORLOG ( "Error loading the playlist" );
			}
		}

		if( ! sDrumkitToLoad.isEmpty() ) {
			H2Core::Drumkit* drumkitInfo = H2Core::Drumkit::load_by_name( sDrumkitToLoad, true );
			if ( drumkitInfo ) {
				H2Core::Hydrogen::get_instance()->loadDrumkit( drumkitInfo );
				HydrogenApp::get_instance()->onDrumkitLoad( drumkitInfo->get_name() );
			} else {
				___ERRORLOG ( "Error loading the drumkit" );
			}
		}

		pQApp->exec();

		delete pSplash;
		delete pMainForm;
		delete pQApp;
		delete pPref;
		delete H2Core::EventQueue::get_instance();
		delete H2Core::AudioEngine::get_instance();

		delete MidiMap::get_instance();
		delete MidiActionManager::get_instance();

		___INFOLOG( "Quitting..." );
		std::cout << "\nBye..." << endl;
		delete H2Core::Logger::get_instance();

		if (H2Core::Object::count_active()) {
			H2Core::Object::write_objects_map_to_cerr();
		}

	}
	catch ( const H2Core::H2Exception& ex ) {
		std::cerr << "[main] Exception: " << ex.what() << std::endl;
	}
	catch (...) {
		std::cerr << "[main] Unknown exception X-(" << std::endl;
	}

	return 0;
}

