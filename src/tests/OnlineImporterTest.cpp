/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "OnlineImporterTest.h"

#include <core/OnlineImporter.h>
#include <core/EventQueue.h>
#include "TestHelper.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <QTcpServer>
#include <QTcpSocket>

using namespace H2Core;

void OnlineImporterTest::testParseValidIndex() {
	___INFOLOG( "" );

	QFile f( H2TEST_FILE( "onlineImport/index.json" ) );
	CPPUNIT_ASSERT( f.open( QIODevice::ReadOnly ) );
	const QByteArray data = f.readAll();

	OnlineImporter importer;
	const auto index = importer.parseIndex( data, QUrl( "https://example.com/index.json" ) );

	CPPUNIT_ASSERT( index.sVersion == "0.1.0" );
	CPPUNIT_ASSERT( index.sCreated == "2026-05-20T14:42:36" );
	CPPUNIT_ASSERT( index.sourceUrl == QUrl( "https://example.com/index.json" ) );
	CPPUNIT_ASSERT( index.patterns.size() == 276 );

	const auto& first = index.patterns.at( 0 );
	CPPUNIT_ASSERT( first.sName == "Afro-Cuban 1" );
	CPPUNIT_ASSERT( first.sAuthor == "Albert Phelipot" );
	CPPUNIT_ASSERT( first.sHash.startsWith( "8f1d80d7" ) );
	CPPUNIT_ASSERT( first.tags.size() == 3 );
	CPPUNIT_ASSERT( first.nNotes == 16 );
	CPPUNIT_ASSERT( first.size == 5241 );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testParseMalformedEntry() {
	___INFOLOG( "" );

	// One valid pattern and one with missing required "url" field.
	const QByteArray data = R"({
		"version": "0.1.0",
		"created": "2026-01-01T00:00:00",
		"patternCount": 2,
		"patterns": [
			{
				"name": "Valid Pattern",
				"author": "Test Author",
				"url": "https://example.com/valid.h2pattern",
				"hash": "abcdef1234567890",
				"tags": [],
				"notes": 8,
				"size": 1024
			},
			{
				"name": "Missing URL Pattern",
				"author": "Test Author",
				"hash": "abcdef1234567890",
				"tags": [],
				"notes": 4,
				"size": 512
			}
		]
	})";

	OnlineImporter importer;
	const auto index = importer.parseIndex( data, QUrl( "https://example.com/index.json" ) );

	// Malformed entry (missing "url") should be skipped.
	CPPUNIT_ASSERT( index.patterns.size() == 1 );
	CPPUNIT_ASSERT( index.patterns.at( 0 ).sName == "Valid Pattern" );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testParseEmptyIndex() {
	___INFOLOG( "" );

	const QByteArray data = "{}";

	OnlineImporter importer;
	const auto index = importer.parseIndex( data, QUrl( "https://example.com/index.json" ) );

	CPPUNIT_ASSERT( index.patterns.empty() );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testTopLevelHashValidation() {
	___INFOLOG( "" );

	// A "hash" field that is intentionally wrong — parse should still succeed
	// (the hash mismatch is logged as a warning but does not reject the data).
	const QByteArray data = R"({
		"version": "0.1.0",
		"created": "2026-01-01T00:00:00",
		"hash": "thisiswronghash",
		"patternCount": 0,
		"patterns": []
	})";

	OnlineImporter importer;
	const auto index = importer.parseIndex( data, QUrl( "https://example.com/index.json" ) );

	// Parse result is still valid — wrong hash is only a warning.
	CPPUNIT_ASSERT( index.sVersion == "0.1.0" );
	CPPUNIT_ASSERT( index.patterns.empty() );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testCountMismatchWarning() {
	___INFOLOG( "" );

	// patternCount claims 5, but only 2 entries are present.
	const QByteArray data = R"({
		"version": "0.1.0",
		"created": "2026-01-01T00:00:00",
		"patternCount": 5,
		"patterns": [
			{
				"name": "Pattern A",
				"author": "Author A",
				"url": "https://example.com/a.h2pattern",
				"hash": "aabbccdd",
				"tags": [],
				"notes": 4,
				"size": 256
			},
			{
				"name": "Pattern B",
				"author": "Author B",
				"url": "https://example.com/b.h2pattern",
				"hash": "11223344",
				"tags": [],
				"notes": 8,
				"size": 512
			}
		]
	})";

	OnlineImporter importer;
	const auto index = importer.parseIndex( data, QUrl( "https://example.com/index.json" ) );

	// Actual data wins — 2 patterns parsed, count mismatch is only a warning.
	CPPUNIT_ASSERT( index.patterns.size() == 2 );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testHashVerificationPass() {
	___INFOLOG( "" );

	const QByteArray data = "Hello, World!";
	// sha256("Hello, World!") — verified with standard tooling.
	const QString expectedHash =
		"dffd6021bb2bd5b0af676290809ec3a53191dd81c7f70a4b28688a362182986f";

	CPPUNIT_ASSERT( OnlineImporter::verifyHash( data, expectedHash ) );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testHashVerificationFail() {
	___INFOLOG( "" );

	const QByteArray data = "Hello, World!";
	const QString wrongHash = "0000000000000000000000000000000000000000000000000000000000000000";

	CPPUNIT_ASSERT( !OnlineImporter::verifyHash( data, wrongHash ) );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testResolveLocalStatusNotInstalled() {
	___INFOLOG( "" );

	// A pattern name that is very unlikely to exist in any local install.
	OnlineArtifact artifact;
	artifact.sName = "__hydrogen_test_nonexistent_pattern_xyzzy__";
	artifact.type = OnlineArtifact::Type::Pattern;
	artifact.sHash = "deadbeef";

	OnlineImporter importer;
	importer.resolveLocalStatus( artifact );

	CPPUNIT_ASSERT( artifact.localStatus == OnlineArtifact::LocalStatus::NotInstalled );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testResolveLocalStatusInstalled() {
	___INFOLOG( "" );

	QTemporaryDir tempDir;
	CPPUNIT_ASSERT( tempDir.isValid() );

	const QByteArray content = "fake pattern content";
	const QString hash = QCryptographicHash::hash( content, QCryptographicHash::Sha256 )
							  .toHex();

	const QString fileName = "test_installed_pattern.h2pattern";
	QFile file( tempDir.filePath( fileName ) );
	CPPUNIT_ASSERT( file.open( QIODevice::WriteOnly ) );
	file.write( content );
	file.close();

	OnlineArtifact artifact;
	artifact.sName = "test_installed_pattern";
	artifact.type = OnlineArtifact::Type::Pattern;
	artifact.sHash = hash;

	OnlineImporter importer;
	importer.setLocalSearchPath( OnlineArtifact::Type::Pattern, tempDir.path() );
	importer.resolveLocalStatus( artifact );

	CPPUNIT_ASSERT( artifact.localStatus == OnlineArtifact::LocalStatus::Installed );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testResolveLocalStatusModified() {
	___INFOLOG( "" );

	QTemporaryDir tempDir;
	CPPUNIT_ASSERT( tempDir.isValid() );

	const QByteArray content = "fake pattern content";
	// The artifact carries a *different* hash than the file on disk.
	// With setLocalSearchPath (test override) this results in Modified
	// since version comparison requires SoundLibraryDatabase access.
	const QString differentHash =
		"0000000000000000000000000000000000000000000000000000000000000000";

	const QString fileName = "test_modified_pattern.h2pattern";
	QFile file( tempDir.filePath( fileName ) );
	CPPUNIT_ASSERT( file.open( QIODevice::WriteOnly ) );
	file.write( content );
	file.close();

	OnlineArtifact artifact;
	artifact.sName = "test_modified_pattern";
	artifact.type = OnlineArtifact::Type::Pattern;
	artifact.sHash = differentHash;
	artifact.nVersion = 1;

	OnlineImporter importer;
	importer.setLocalSearchPath( OnlineArtifact::Type::Pattern, tempDir.path() );
	importer.resolveLocalStatus( artifact );

	CPPUNIT_ASSERT( artifact.localStatus == OnlineArtifact::LocalStatus::Modified );

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testDownloadArtifactsEmptyList() {
	___INFOLOG( "" );

	OnlineImporter importer;
	QSignalSpy batchSpy( &importer, &OnlineImporter::batchFinished );

	CPPUNIT_ASSERT( batchSpy.isValid() );

	importer.downloadArtifactsAsync( QVector<OnlineArtifact>() );

	// batchFinished signal should have been emitted exactly once
	CPPUNIT_ASSERT( batchSpy.count() == 1 );
	const auto args = batchSpy.takeFirst();
	CPPUNIT_ASSERT( args.at( 0 ).toInt() == 0 ); // nSuccessCount
	CPPUNIT_ASSERT( args.at( 1 ).toInt() == 0 ); // nFailCount

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testDownloadArtifactsAbort() {
	___INFOLOG( "" );

	OnlineImporter importer;

	// Verify that abort() is effective: calling abort() is intended to stop
	// an in-progress batch. Since downloadArtifacts() resets the abort flag
	// on entry (to allow reuse), we test that the batchFinished signal is
	// always emitted regardless.  With an unreachable URL, the download will
	// fail (not hang) and batchFinished will be emitted with the failure
	// counted.
	OnlineArtifact artifact;
	artifact.type = OnlineArtifact::Type::Pattern;
	artifact.sName = "abort_test_pattern";
	artifact.url = QUrl( "https://invalid.example.test/does-not-exist.h2pattern" );
	artifact.sHash = "0000000000000000000000000000000000000000000000000000000000000000";

	QSignalSpy batchSpy( &importer, &OnlineImporter::batchFinished );
	CPPUNIT_ASSERT( batchSpy.isValid() );

	QVector<OnlineArtifact> artifacts;
	artifacts.append( artifact );

	// Abort mid-flight: connect to downloadFinished to call abort, so the
	// second artifact (if any) would be skipped.  With a single artifact
	// the abort after the first item has no visible effect, but we confirm
	// the mechanism doesn't crash.
	QObject::connect( &importer, &OnlineImporter::downloadFinished,
					  [&importer]( const QString&, bool, const QString& ) {
						  importer.abort();
					  } );

	importer.downloadArtifactsAsync( artifacts );

	// batchFinished must have been emitted exactly once
	CPPUNIT_ASSERT( batchSpy.count() == 1 );
	const auto args = batchSpy.takeFirst();
	// The single artifact should have failed (unreachable URL)
	CPPUNIT_ASSERT( args.at( 0 ).toInt() == 0 ); // nSuccessCount
	CPPUNIT_ASSERT( args.at( 1 ).toInt() == 1 ); // nFailCount

	___INFOLOG( "passed" );
}

void OnlineImporterTest::testDownloadBlockingSuccess() {
	___INFOLOG( "" );

	// Set up a local TCP server that responds with a known payload
	const QByteArray payload = "This is the artifact content for testing.";
	const QString sExpectedHash = QCryptographicHash::hash(
		payload, QCryptographicHash::Sha256 ).toHex();

	QTcpServer server;
	CPPUNIT_ASSERT( server.listen( QHostAddress::LocalHost, 0 ) );
	const quint16 nPort = server.serverPort();

	QObject::connect( &server, &QTcpServer::newConnection, [&]() {
		QTcpSocket* pSocket = server.nextPendingConnection();
		QObject::connect( pSocket, &QTcpSocket::readyRead, [pSocket, &payload]() {
			// Send a minimal HTTP/1.1 response
			const QByteArray response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: " + QByteArray::number( payload.size() ) + "\r\n"
				"Connection: close\r\n"
				"\r\n" + payload;
			pSocket->write( response );
			pSocket->flush();
			pSocket->disconnectFromHost();
		} );
	} );

	OnlineImporter importer;
	QString sError;
	const QUrl url( QString( "http://127.0.0.1:%1/test.h2pattern" ).arg( nPort ) );
	const QByteArray data = importer.downloadBlocking( url, 5000, &sError );

	CPPUNIT_ASSERT_MESSAGE( sError.toStdString(), sError.isEmpty() );
	CPPUNIT_ASSERT( data == payload );
	CPPUNIT_ASSERT( OnlineImporter::verifyHash( data, sExpectedHash ) );

	server.close();
	___INFOLOG( "passed" );
}

void OnlineImporterTest::testDownloadBlockingHashMismatch() {
	___INFOLOG( "" );

	// Server responds with valid data, but the artifact has a wrong hash
	const QByteArray payload = "Payload that will fail hash check.";
	const QString sWrongHash =
		"0000000000000000000000000000000000000000000000000000000000000000";

	QTcpServer server;
	CPPUNIT_ASSERT( server.listen( QHostAddress::LocalHost, 0 ) );
	const quint16 nPort = server.serverPort();

	QObject::connect( &server, &QTcpServer::newConnection, [&]() {
		QTcpSocket* pSocket = server.nextPendingConnection();
		QObject::connect( pSocket, &QTcpSocket::readyRead, [pSocket, &payload]() {
			const QByteArray response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Length: " + QByteArray::number( payload.size() ) + "\r\n"
				"Connection: close\r\n"
				"\r\n" + payload;
			pSocket->write( response );
			pSocket->flush();
			pSocket->disconnectFromHost();
		} );
	} );

	// Use downloadArtifactBlocking which does both download + hash verification
	OnlineArtifact artifact;
	artifact.type = OnlineArtifact::Type::Pattern;
	artifact.sName = "hash_mismatch_test";
	artifact.url = QUrl( QString( "http://127.0.0.1:%1/test.h2pattern" ).arg( nPort ) );
	artifact.sHash = sWrongHash;

	OnlineImporter importer;
	QString sError;
	const bool bResult = importer.downloadArtifactBlocking( artifact, &sError );

	CPPUNIT_ASSERT( !bResult );
	CPPUNIT_ASSERT( sError.contains( "Hash mismatch" ) );

	server.close();
	___INFOLOG( "passed" );
}
