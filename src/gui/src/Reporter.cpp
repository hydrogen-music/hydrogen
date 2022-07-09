/*
 * Hydrogen
 * Copyright(c) 2022 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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
#include <iostream>
#include "core/Hydrogen.h"
#include "core/Helpers/Filesystem.h"
#include "core/Logger.h"
#include "Reporter.h"


QString Reporter::sPrefix = "Fatal error in: ";

using namespace H2Core;

void Reporter::addLine( QString s )
{
	// Keep only a few lines of the output
	while ( lines.size() > 128 ) {
		// Record context
		if ( lines[0].startsWith( sPrefix )) {
			sContext = lines[0];
			Logger::setCrashContext( &sContext );
		}
		lines.pop_front();
	}
	if ( lines.size() == 0 ) {
		lines.push_back( QString( "" ) );
	}
	
	QStringList parts = s.split( "\n" );
	QString sLastLine = lines.back();
	lines.pop_back();
	// Append the first part to the last line
	sLastLine += parts.takeFirst();
	lines.push_back( sLastLine );
	for ( auto &s : parts ) {
		lines.push_back( s );
	}
}

Reporter::Reporter( QProcess *child )
{
	assert( child != nullptr );
	this->child = child;

	connect( child, &QProcess::readyReadStandardOutput,
			 this, &Reporter::on_readyReadStandardOutput );
	connect( child, &QProcess::readyReadStandardError,
			 this, &Reporter::on_readyReadStandardError );
	connect( child, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
			 this, &Reporter::on_finished );
}

void Reporter::waitForFinished()
{
	assert( child );
	while ( child->state() != QProcess::NotRunning ) {
		child->waitForFinished();
	}
}


void Reporter::report( void )
{
	std::cerr.flush();
	std::cout.flush();
	QString *pContext = Logger::getCrashContext();
	if ( pContext != nullptr ) {
		std::cerr << sPrefix.toStdString() << pContext->toStdString()
				  << std::endl;
		std::cerr.flush();
	}
}


void Reporter::on_readyReadStandardError( void )
{
	std::string s = child->readAllStandardError().toStdString();
	std::cerr << s.c_str();
	addLine( s.c_str() );
}

void Reporter::on_readyReadStandardOutput( void )
{
	std::string s = child->readAllStandardOutput().toStdString();
	std::cout << s.c_str();
	addLine( s.c_str() );
}

void Reporter::on_openLog( void )
{
	qDebug() << "Open log...";
	QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::log_file_path() ) );
}

void Reporter::on_finished( int exitCode, QProcess::ExitStatus exitStatus )
{
	on_readyReadStandardError();
	on_readyReadStandardOutput();

	if ( child->exitStatus() != QProcess::NormalExit ) {

		char *argv[] = { (char *)"-" };
		int argc = 1;
		QApplication app ( argc, argv );
		app.setApplicationName( "Hydrogen" );

		QString sDetails;
		for ( QString &s : lines ) {
			// Filter out escape sequences
			s.remove( "\e[0m" );
			s.remove( "\e[31m" );
			s.remove( "\e[32m" );
			s.remove( "\e[35m" );
			s.remove( "\e[35;1m" );
			s.remove( "\e[36m" );
			sDetails += s + "\n";
			if ( s.startsWith( sPrefix ) ) {
				sContext = s;
			}
		}

		QMessageBox msgBox;
		msgBox.setText( tr( "Hydrogen exited abnormally" ) );

		QString sInformative;
		if ( !sContext.isNull() ) {
			sInformative = sContext + "\n\n";
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
				QDesktopServices::openUrl( QUrl::fromLocalFile( H2Core::Filesystem::log_file_path() ) );

			} else if ( pPushed == pIssuesButton ) {
				QDesktopServices::openUrl( QUrl( "https://github.com/hydrogen-music/hydrogen/issues") );

			} else {
				break;
			}
		} while ( true );

	}
}

void Reporter::spawn(int argc, char *argv[])
{
	QStringList arguments;
	for ( int i = 1; i < argc; i++ ) {
		if ( argv[i] == QString("--child") ) {
			return;
		}
		arguments << QString( argv[i] );
	}

	QProcess subProcess;
	arguments << "--child";
	subProcess.start(argv[0], arguments);

	Reporter reporter( &subProcess );
	reporter.waitForFinished();
	exit( subProcess.exitCode() );
}
