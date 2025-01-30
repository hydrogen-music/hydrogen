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

#include "TestHelper.h"

#include "core/Object.h"
#include <core/Basics/Song.h>
#include <core/IO/DiskWriterDriver.h>

#include <QProcess>
#include <QProcessEnvironment>
#include <QStringList>
#include <exception>
#include <random>
#include <chrono>

#include <cppunit/extensions/HelperMacros.h>

static const QString APP_DATA_DIR = "/data/";
static const QString TEST_DATA_DIR = "/src/tests/data/";

TestHelper* TestHelper::m_pInstance = nullptr;

void TestHelper::createInstance()
{
	if ( m_pInstance == nullptr ) {
		m_pInstance = new TestHelper;
	}
}


/**
 * \brief Execute command and return captured output
 * \param args Command to run and its parameters
 * \return Captured standard output
 */
QString qx(QStringList args)
{
	QProcess proc;
	proc.start(args.first(), args.mid(1));
	proc.waitForFinished();
	if ( proc.exitCode() != 0 ) {
		throw std::runtime_error("Not a git repository");
	}
	auto path = proc.readAllStandardOutput();
	return QString(path).trimmed();
}


/**
 * \brief Check whether directory is Hydrogen source root dir
 * \param dir Path to directory
 * \return Whether dir points to Hydrogen source dir
 **/
bool checkRootDir(const QString &dir)
{
	QFile f( dir + TEST_DATA_DIR + "/drumkits/baseKit/drumkit.xml" );
	return f.exists();
}


/**
 * \brief Try to find Hydrogen source dir
 * \throws std::runtime_error when source dir cannot be find
 *
 * This function tries to find Hydrogen source dir in order to
 * find data files required by tests. First, environment
 * variable H2_HOME is examined. If it's not set or doesn't point
 * to valid directory, this function tries to run "git rev-parse --show-toplevel"
 * to get root directory of git repository. If that fails,
 * current directory is examined. If it doesn't point to
 * source dir, std::runtime_error is thrown.
 **/
QString findRootDir()
{
	/* Get root dir from H2_HOME env variable */
	auto env_root_dir = QProcessEnvironment::systemEnvironment().value("H2_HOME", "");
	if (env_root_dir != "") {
		if (checkRootDir( env_root_dir ) ) {
			return env_root_dir;
		} else {
			___ERRORLOG( QString( "Directory %1 not usable" ).arg( env_root_dir ) );
		}
	}

	/* Try git root directory */
	try {
		auto git_root_dir = qx({"git", "rev-parse", "--show-toplevel"});
		if (checkRootDir( git_root_dir ) ) {
			return git_root_dir;
		} else {
			___ERRORLOG( QString( "Directory %1 not usable" ).arg( git_root_dir ) );
		}
	} catch (std::runtime_error &e) {
		___WARNINGLOG( "Can't find git root directory" );
	}

	/* As last resort, use current dir */
	if (checkRootDir( "." ) ) {
		return ".";
	}
	throw std::runtime_error( "Can't find suitable data directory. Consider setting H2_HOME environment variable" );
}

TestHelper::TestHelper()
{
	auto root_dir = findRootDir();
	___INFOLOG( QString( "Using test data directory: %1" ).arg( root_dir ) );
	m_sDataDir = root_dir + APP_DATA_DIR;
	m_sTestDataDir = root_dir + TEST_DATA_DIR;
}
