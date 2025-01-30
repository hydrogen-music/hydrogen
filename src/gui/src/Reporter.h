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

#ifndef REPORTER_H
#define REPORTER_H

#include <QtGui>
#include <QtWidgets>
#include <deque>
#include <cassert>
#include <memory>
#include <iostream>
#include <set>


/** Crash reporter class
 *
 * The crash reporter provides a split-process model for reporting crashes from Hydrogen, with a simple GUI
 * giving options to view the log file, other details or open a browser to the issue tracker.
 *
 * Early on, the calling process should call #Reporter::spawn(). For the parent process, this causes the child
 * to be spawned (adding "--child" to command line arguments), and the parent process will wait for the
 * termination of the child, and report a crash if an abnormal exit occurs, or just exit silently
 * otherwise. The parent never returns from #Reporter::spawn().
 *
 * The child process will identify itself through having "--child" in the command line arguments, and continue
 * as normal.
 *
 * The Logger 'crash context' can be reported by the child process during a crash by using #Reporter::report().
 *
 * The parent process passes through stdout and sterr from the child process, but also notes any reported
 * crash context, which is communicated by stderr with a specific prefix. Recent lines from stderr / stdout
 * are recorded in the parent (reporter) process to be displayed as crash details if needed.
 */

class Reporter : public QObject
{
	Q_OBJECT

	QProcess *m_pChild;
	std::deque< QString > m_lines;

	static QString m_sPrefix;
	QString m_sContext;
	static QString m_sLogFile;

	void addLine( const QString& s );

	void waitForFinished();

	static std::set<QProcess *> m_children;

 public:

	Reporter( QProcess *child );
	~Reporter();

	/** Report some crash details in a crashing child (mostly, the Logger 'crash context' string) */
	static void report( void );

	/** Potentially spawn child process */
	static void spawn( int argc, char *argv[] );

	static void handleSignal( int nSignal );

public slots:

	void on_readyReadStandardError( void );

	void on_readyReadStandardOutput( void );

	void on_openLog( void );

	void on_finished( int exitCode, QProcess::ExitStatus exitStatus );
};

#endif
