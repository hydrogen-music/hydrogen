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

#ifndef APPVEYOR_H
#define APPVEYOR_H

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QUrl>

namespace AppVeyor {

enum class TestOutcome {
    None,
    Running,
    Passed,
    Failed,
    Ignored,
    Skipped,
    Inconclusive,
    NotFound,
    Cancelled,
    NotRunnable
};


struct Test {
    QString name;
    QString framework;
    QString fileName;
    TestOutcome outcome;
    int duration = 0;
    QString errorMessage;
    QString errorStackTrace;
    QString stdOut;
    QString stdErr;

    Test();
    Test(const QString &name, const QString &framework, const QString &fileName, TestOutcome outcome = TestOutcome::None);
    void toJson(QJsonObject &json) const;
};


/**
 * \brief An interface to AppVeyor build worker API
 *
 * \see https://www.appveyor.com/docs/build-worker-api/
 */
class BuildWorkerApiClient {
    public:
    BuildWorkerApiClient();

    /**
     * \brief Report new test result to AppVeyor build worker
     *
     * \see https://www.appveyor.com/docs/build-worker-api/#add-tests
     */
    void addTest(const Test &testData);

    /**
     * \brief Update existing test result to AppVeyor build worker
     *
     * \see https://www.appveyor.com/docs/build-worker-api/#update-tests
     */
    void updateTest(const Test &testData);

    private:
    /**
     * \brief Whether test reporting is enabled
     *
     * Test reporting is enabled when m_ApiRoot is set, and it's a valid URL.
     */
    bool m_bEnabled;

    /**
     * \brief URL of build worker API endpoint
     *
     * URL of build worker API endpoint. Test progress is reported there.
     * This variable is initialized from APPVEYOR_API_URL environment
     * variable.
     */
    QUrl m_ApiRoot;

    void doSyncRequest(QByteArray method, QString endpoint, const QJsonDocument &payload);
};

};

#endif
