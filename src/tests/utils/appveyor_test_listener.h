/*
 * Hydrogen
 * Copyright(c) 2002-2020 by the Hydrogen Team
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

#ifndef APPVEYOR_TEST_LISTENER_H
#define APPVEYOR_TEST_LISTENER_H

#include <cppunit/TestListener.h>

#include "appveyor_rest_client.h"

#include <chrono>
#include <unordered_map>

#include <QString>

class AppVeyorTestListener : public CppUnit::TestListener
{
    AppVeyor::BuildWorkerApiClient *m_pClient;
    using time_point = std::chrono::steady_clock::time_point;
    std::unordered_map<CppUnit::Test *, AppVeyor::Test> m_testResults;
    std::unordered_map<CppUnit::Test *, time_point> m_startTime;

    public:
    AppVeyorTestListener( AppVeyor::BuildWorkerApiClient &client );
    void startTest(CppUnit::Test *) override;
    void addFailure(const CppUnit::TestFailure &fail) override;
    void endTest(CppUnit::Test *) override;

    QString testFailureMessage(const CppUnit::TestFailure &fail);
    QString testFailureStackTrace(const CppUnit::TestFailure &fail);
};

#endif
