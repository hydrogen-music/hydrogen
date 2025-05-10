/*
 * Hydrogen
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
#include "Parser.h"

#include <QtGui>
#include <QtWidgets>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QStringList>
#include <iostream>

#include <core/Helpers/Filesystem.h>
#include <core/Preferences/Preferences.h>
#include <core/Version.h>

Parser::Parser() {
}

Parser::~Parser() {
}

bool Parser::parse( int argc, char* argv[] ) {
	QCoreApplication* pApp = nullptr;
	if ( QCoreApplication::instance() == nullptr) {
		pApp = new QCoreApplication( argc, argv );
		pApp->setApplicationVersion(
			QString::fromStdString( H2Core::get_version() ) );
		assert( QCoreApplication::instance() == pApp );
	}

	QCommandLineParser parser;

	parser.setApplicationDescription( H2Core::getAboutText() );

	QStringList availableAudioDrivers;
	for ( const auto& ddriver : H2Core::Preferences::getSupportedAudioDrivers() ) {
		availableAudioDrivers <<
			H2Core::Preferences::audioDriverToQString( ddriver );
	}
	availableAudioDrivers << H2Core::Preferences::audioDriverToQString(
		H2Core::Preferences::AudioDriver::Auto );

	QCommandLineOption audioDriverOption(
		QStringList() << "d" << "driver",
		QString( "Use the selected audio driver (%1)" )
		.arg( availableAudioDrivers.join( ", " ) ), "Audiodriver");
	QCommandLineOption playlistFileNameOption(
		QStringList() << "p" <<
		"playlist", "Load a playlist (*.h2playlist) at startup", "File" );
	QCommandLineOption songFileOption(
		QStringList() << "s" <<
		"song", "Load a song (*.h2song) at startup", "File" );
	QCommandLineOption kitOption(
		QStringList() << "k" << "kit", "Load a drumkit at startup", "DrumkitName" );

	QCommandLineOption installDrumkitOption(
		QStringList() << "i" <<
		"install", "Install a drumkit (*.h2drumkit)", "File");

	QCommandLineOption verboseOption(
		QStringList() << "V" <<
		"verbose", "Level, if present, may be None, Error, Warning, Info, Debug, Constructors, Locks, or 0xHHHH", "Level" );
	QCommandLineOption logFileOption(
		QStringList() << "L" << "log-file", "Alternative log file path", "Path" );
	QCommandLineOption logTimestampsOption(
		QStringList() << "T" <<
		"log-timestamps", "Add timestamps to all log messages" );
	QCommandLineOption logColorsOption(
		QStringList() << "log-colors", "Use ANSI colors in log messages" );
	QCommandLineOption noLogColorsOption(
		QStringList() << "no-log-colors",
		"Suppress ANSI colors in log messages" );

	QCommandLineOption systemDataPathOption(
		QStringList() << "P" <<
		"data", "Use an alternate system data path", "Path" );
	QCommandLineOption configFileOption(
		QStringList() << "config", "Use an alternate config file", "Path" );
	QCommandLineOption uiLayoutOption(
		QStringList() << "layout", "UI layout ('tabbed' or 'single')", "Layout" );
#ifdef H2CORE_HAVE_OSC
	QCommandLineOption oscPortOption(
		QStringList() << "O" <<
		"osc-port", "Custom port for OSC connections", "int" );
#endif
	QCommandLineOption noSplashScreenOption(
		QStringList() << "n" << "nosplash", "Hide splash screen" );

	QCommandLineOption shotListOption(
		QStringList() << "t" <<
		"shotlist", "Shot list of widgets to grab", "ShotList" );

	QCommandLineOption noReporterOption(
		QStringList() << "child", "Child process (no crash reporter)");

	parser.addHelpOption();
	parser.addVersionOption();

	parser.addOption( audioDriverOption );
	parser.addOption( playlistFileNameOption );
	parser.addOption( songFileOption );
	parser.addOption( kitOption );

	parser.addOption( installDrumkitOption );

	parser.addOption( verboseOption );
	parser.addOption( logFileOption );
	parser.addOption( logTimestampsOption );
	parser.addOption( logColorsOption );
	parser.addOption( noLogColorsOption );

	parser.addOption( systemDataPathOption );
	parser.addOption( configFileOption );
	parser.addOption( uiLayoutOption );
#ifdef H2CORE_HAVE_OSC
	parser.addOption( oscPortOption );
#endif
	parser.addOption( noSplashScreenOption );

	parser.addOption( shotListOption );

	parser.addOption( noReporterOption );

	parser.addPositionalArgument( "file", "Song, playlist or Drumkit file" );

	// Evaluate the options
	parser.process( *( QCoreApplication::instance() ) );

	m_sAudioDriver = parser.value( audioDriverOption );
	m_sPlaylistFilename = parser.value( playlistFileNameOption );
	m_sSongFilename = parser.value ( songFileOption );
	m_sDrumkitToLoad = parser.value( kitOption );

	m_sInstallDrumkitPath = parser.value( installDrumkitOption );

	QString sVerbosityString = parser.value( verboseOption );
	m_logLevel = H2Core::Logger::Error;
	if ( parser.isSet( verboseOption ) ) {
		if ( ! sVerbosityString.isEmpty() ) {
			m_logLevel =  H2Core::Logger::parse_log_level(
				sVerbosityString.toLocal8Bit() );
		} else {
			m_logLevel = H2Core::Logger::Error | H2Core::Logger::Warning;
		}
	}
	m_sLogFile = parser.value( logFileOption );
	m_bLogTimestamps = parser.isSet( logTimestampsOption );

#ifdef WIN32
	m_bLogColors = false;
#else
	m_bLogColors = true;
#endif

	// If both options are present, having colors wins.
	if ( parser.isSet( logColorsOption ) ) {
		m_bLogColors = true;
	}
	else if ( parser.isSet( noLogColorsOption ) ) {
		m_bLogColors = false;
	}

	m_sSysDataPath = parser.value( systemDataPathOption );
	m_sConfigFilePath = parser.value( configFileOption );
	m_sUiLayout = parser.value( uiLayoutOption );
	m_nOscPort = -1;
#ifdef H2CORE_HAVE_OSC
	QString sOscPort = parser.value( oscPortOption );
	if ( ! sOscPort.isEmpty() ) {
		bool bOk;
		m_nOscPort = sOscPort.toInt( &bOk, 10 );
		if ( ! bOk ) {
			std::cerr << "Unable to parse 'osc-port' option. Please provide an integer value"
					  << std::endl;
			return false;
		}
	}
#endif

	m_bNoSplashScreen = parser.isSet( noSplashScreenOption );

	m_sShotList = parser.value( shotListOption );

	m_bNoReporter = parser.isSet( noReporterOption );

	// Operating system GUIs typically pass documents to open as simple
	// positional arguments to the process command line. Handling this here
	// enables "Open with" as well as default document bindings to work.
	QString sArg;
	foreach ( sArg, parser.positionalArguments() ) {
		if ( sArg.endsWith( H2Core::Filesystem::songs_ext ) ) {
			m_sSongFilename = sArg;
		}
		if ( sArg.endsWith( H2Core::Filesystem::drumkit_ext ) ) {
			m_sInstallDrumkitPath = sArg;
		}
		if ( sArg.endsWith( H2Core::Filesystem::playlist_ext ) ) {
			m_sPlaylistFilename = sArg;
		}
	}

	if ( pApp != nullptr ) {
		delete pApp;
	}

	return true;
}
