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

#include "AppveyorTestListener.h"

#include <cppunit/SourceLine.h>
#include <cppunit/Exception.h>
#include <cppunit/Message.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>

#include <chrono>



AppVeyorTestListener::AppVeyorTestListener( AppVeyor::BuildWorkerApiClient &client )
    : m_pClient{ &client }
{
}


void AppVeyorTestListener::startTest(CppUnit::Test *test)
{
    AppVeyor::Test testResult(QString::fromStdString(test->getName()),
                              "CppUnit",
                              "tests.cpp",
                              AppVeyor::TestOutcome::Running);

    m_pClient->addTest( testResult );

    m_testResults.insert(std::make_pair(test, testResult));
    m_startTime.insert(std::make_pair(test, std::chrono::steady_clock::now()));
}


void AppVeyorTestListener::addFailure(const CppUnit::TestFailure &fail)
{
    AppVeyor::Test &testResult = m_testResults[fail.failedTest()];
    testResult.outcome = AppVeyor::TestOutcome::Failed; // AppVeyor doesn't distinguish failed tests from errors
    testResult.errorMessage = testFailureMessage( fail );
    testResult.errorStackTrace = testFailureStackTrace( fail );
}


void AppVeyorTestListener::endTest(CppUnit::Test *test)
{
    time_point end_time = std::chrono::steady_clock::now();
    time_point start_time = m_startTime[test];

    AppVeyor::Test testResult = m_testResults[test];
    if( testResult.outcome == AppVeyor::TestOutcome::Running ) {
        testResult.outcome = AppVeyor::TestOutcome::Passed;
    }
    testResult.duration = std::chrono::duration_cast<std::chrono::milliseconds> (end_time - start_time).count();

    m_pClient->updateTest( testResult );
}


QString AppVeyorTestListener::testFailureMessage(const CppUnit::TestFailure &fail)
{
    auto message = fail.thrownException()->message();
    return QString( "%1\n%2" ).arg(
            QString::fromStdString( message.shortDescription() ),
            QString::fromStdString( message.details() )
    );
}

QString AppVeyorTestListener::testFailureStackTrace( const CppUnit::TestFailure &fail )
{
    auto src = fail.sourceLine();
    return QString( "%1:%2" ).arg(
            QString::fromStdString( src.fileName() ),
            QString::number( src.lineNumber() )
    );
}
