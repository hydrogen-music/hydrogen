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

#include "AppveyorRestClient.h"

#include <QBuffer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>

namespace AppVeyor {

QString toString(TestOutcome outcome)
{
    switch (outcome) {
        case TestOutcome::None: return "None";
        case TestOutcome::Running: return "Running";
        case TestOutcome::Passed: return "Passed";
        case TestOutcome::Failed: return "Failed";
        case TestOutcome::Ignored: return "Ignored";
        case TestOutcome::Skipped: return "Skipped";
        case TestOutcome::Inconclusive: return "Inconclusive";
        case TestOutcome::NotFound: return "NotFound";
        case TestOutcome::Cancelled: return "Cancelled";
        case TestOutcome::NotRunnable: return "NotRunnable";
    }
    return "";
}

Test::Test()
    : outcome{ TestOutcome::None }
{
}

Test::Test(const QString &name, const QString &framework, const QString &fileName, TestOutcome outcome)
    : name{ name }, framework{ framework }, fileName{ fileName }, outcome{ outcome }
{
}

void Test::toJson(QJsonObject &json) const
{
    json["testName"] = name;
    json["testFramework"] = framework;
    json["fileName"] = fileName;
    json["outcome"] = toString(outcome);
    json["durationMilliseconds"] = duration;
    json["ErrorMessage"] = errorMessage;
    json["ErrorStackTrace"] = errorStackTrace;
    json["StdOut"] = stdOut;
    json["stdErr"] = stdErr;
}


BuildWorkerApiClient::BuildWorkerApiClient()
{
    QString apiRoot = QProcessEnvironment::systemEnvironment().value("APPVEYOR_API_URL", "");
    if( ! apiRoot.isEmpty() ) {
        m_ApiRoot = QUrl(apiRoot);
        m_bEnabled = m_ApiRoot.isValid();
    }
}


void BuildWorkerApiClient::addTest(const Test &testData)
{
    if( ! m_bEnabled ) return;

    QJsonObject json;
    testData.toJson(json);
    QJsonDocument jsonDoc(json);
    doSyncRequest("POST", "api/tests", jsonDoc);
}


void BuildWorkerApiClient::updateTest(const Test &testData)
{
    if( ! m_bEnabled ) return;

    QJsonObject json;
    testData.toJson(json);
    QJsonDocument jsonDoc(json);
    doSyncRequest("PUT", "api/tests", jsonDoc);
}


void BuildWorkerApiClient::doSyncRequest(QByteArray method, QString endpoint, const QJsonDocument &payload)
{
    QUrl endpointUrl(endpoint);

    QByteArray jsonPayload = payload.toJson(QJsonDocument::Compact);

    QNetworkAccessManager qnam;
    QNetworkRequest request{m_ApiRoot.resolved(endpointUrl)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    #if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
    auto reply = qnam.sendCustomRequest(request, method, jsonPayload);
    #else
    QBuffer buffer(&jsonPayload);
    auto reply = qnam.sendCustomRequest(request, method, &buffer);
    #endif
    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}

};
