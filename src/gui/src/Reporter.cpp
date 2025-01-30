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
#include "Reporter.h"
#include "Parser.h"

#include <iostream>
#include <signal.h>

#include <core/Hydrogen.h>
#include <core/Helpers/Filesystem.h>
#include <core/Logger.h>


QString Reporter::m_sPrefix = "Fatal error in: ";
QString Reporter::m_sLogFile;

std::set<QProcess *> Reporter::m_children;

using namespace H2Core;

void Reporter::addLine( const QString& s )
{
	// Keep only a few lines of the output
	while ( m_lines.size() > 128 ) {
		// Record context
		if ( m_lines[0].startsWith( m_sPrefix )) {
			m_sContext = m_lines[0];
			Logger::setCrashContext( &m_sContext );
		}
		m_lines.pop_front();
	}
	if ( m_lines.size() == 0 ) {
		m_lines.push_back( QString( "" ) );
	}
	
	QStringList parts = s.split( "\n" );
	QString sLastLine = m_lines.back();
	m_lines.pop_back();
	// Append the first part to the last line
	sLastLine += parts.takeFirst();
	m_lines.push_back( sLastLine );
	for ( auto &s : parts ) {
		m_lines.push_back( s );
	}
}

Reporter::Reporter( QProcess *pChild )
{
	assert( pChild != nullptr );
	this->m_pChild = pChild;
	m_children.insert( pChild );

	if ( m_sLogFile.isEmpty() ) {
		m_sLogFile = H2Core::Filesystem::log_file_path();
	}

	connect( pChild, &QProcess::readyReadStandardOutput,
			 this, &Reporter::on_readyReadStandardOutput );
	connect( pChild, &QProcess::readyReadStandardError,
			 this, &Reporter::on_readyReadStandardError );
	connect( pChild, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
			 this, &Reporter::on_finished );
}

Reporter::~Reporter()
{
	m_children.erase( this->m_pChild );
}

void Reporter::waitForFinished()
{
	assert( m_pChild );
	while ( m_pChild->state() != QProcess::NotRunning ) {
		m_pChild->waitForFinished();
	}
}


void Reporter::report( void )
{
	std::cerr.flush();
	std::cout.flush();
	QString *pContext = Logger::getCrashContext();
	if ( pContext != nullptr ) {
		std::cerr << m_sPrefix.toStdString() << pContext->toStdString()
				  << std::endl;
		std::cerr.flush();
	}
}


void Reporter::on_readyReadStandardError( void )
{
	std::string s = m_pChild->readAllStandardError().toStdString();
	std::cerr << s.c_str();
	addLine( s.c_str() );
}

void Reporter::on_readyReadStandardOutput( void )
{
	std::string s = m_pChild->readAllStandardOutput().toStdString();
	std::cout << s.c_str();
	addLine( s.c_str() );
}

void Reporter::on_openLog( void )
{
	qDebug() << "Open log...";
	QDesktopServices::openUrl( QUrl::fromLocalFile( Reporter::m_sLogFile ) );
}

void Reporter::on_finished( int exitCode, QProcess::ExitStatus exitStatus )
{
	on_readyReadStandardError();
	on_readyReadStandardOutput();

	if ( m_pChild->exitStatus() != QProcess::NormalExit
		 || m_pChild->exitCode() != 0 ) {

		char *argv[] = { (char *)"-" };
		int argc = 1;
		QApplication app ( argc, argv );
		app.setApplicationName( "Hydrogen" );

		QString sDetails;
		for ( QString &s : m_lines ) {
			// Filter out escape sequences
			s.remove( "\e[0m" );
			s.remove( "\e[31m" );
			s.remove( "\e[32m" );
			s.remove( "\e[35m" );
			s.remove( "\e[35;1m" );
			s.remove( "\e[36m" );
			sDetails += s + "\n";
			if ( s.startsWith( m_sPrefix ) ) {
				m_sContext = s;
			}
		}

		QMessageBox msgBox;
		msgBox.setText( tr( "Hydrogen exited abnormally" ) );

		QString sInformative;
		if ( !m_sContext.isNull() ) {
			sInformative = m_sContext + "\n\n";
		}
		sInformative += tr( "You can check the Hydrogen issue tracker on Github to see if this issue "
							"is already known about. "
							"If not, you can report it there to help the development team get you back on track "
							"and improve Hydrogen for the future." ) + "\n";
		msgBox.setInformativeText( sInformative );

		msgBox.setStandardButtons( QMessageBox::Ok );
		msgBox.setDefaultButton( QMessageBox::Discard );
		msgBox.setWindowTitle( "Hydrogen" );
		msgBox.setIcon( QMessageBox::Critical );

		msgBox.setDetailedText( sDetails );

		QPushButton *pLogButton = msgBox.addButton( tr( "Open log file..." ),
													QMessageBox::ActionRole );

		QPushButton *pIssuesButton = msgBox.addButton( tr( "Github Issue tracker..." ),
													   QMessageBox::ActionRole );

		do {
			msgBox.exec();
			QAbstractButton *pPushed = msgBox.clickedButton();
			
			if ( pLogButton == pPushed ) {
				QDesktopServices::openUrl( QUrl::fromLocalFile( Reporter::m_sLogFile ) );

			} else if ( pPushed == pIssuesButton ) {
				QDesktopServices::openUrl( QUrl( "https://github.com/hydrogen-music/hydrogen/issues") );

			} else {
				break;
			}
		} while ( true );

	}
}

void Reporter::handleSignal( int nSignal )
{
	// First disable signal handler to allow normal termination
	signal( nSignal, SIG_DFL );

	for ( QProcess *pChild : m_children ) {
#ifndef WIN32
		kill( pChild->processId(), nSignal );
#else
		// On Windows, we can't use kill() to pass along the signal we received, so just use
		// QProcess::terminate()
		pChild->terminate();
#endif
	}

	raise( nSignal );
}

static void handleSignal( int nSignal ) {
	Reporter::handleSignal( nSignal );
}

void Reporter::spawn(int argc, char *argv[])
{
	Parser parser;
	if ( ! parser.parse( argc, argv ) ) {
		std::cerr << "ERROR: Unable to parse CLI arguments. Abort..."
				  << std::endl;
		exit( 1 );
	}

	// --child option was supplied indicating that we do not want to use the
	// Reporter.
	if ( parser.getNoReporter() ) {
		return;
	}

	if ( ! parser.getLogFile().isEmpty() ) {
		Reporter::m_sLogFile = parser.getLogFile();
	}

	QStringList arguments;
	for ( int ii = 1; ii < argc; ii++ ) {
		arguments << QString( argv[ii] );
	}
	arguments << "--child";

	QProcess subProcess;
	subProcess.start(argv[0], arguments);

	// Signal handler
	for ( int nSignal : { SIGINT, SIGTERM
#ifndef WIN32
						 , SIGHUP
#endif
		} ) {
		signal( nSignal, ::handleSignal );
	}

	Reporter reporter( &subProcess );
	reporter.waitForFinished();
	exit( subProcess.exitCode() );
}
