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

#include <core/OnlineImporter.h>
#include <core/EventQueue.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/SoundLibrary/SoundLibraryInfo.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace H2Core
{

OnlineImporter::OnlineImporter( QObject* pParent )
	: QObject( pParent )
	, m_bAborted( false )
	, m_pAsyncNAM( new QNetworkAccessManager( this ) )
	, m_pAsyncReply( nullptr )
	, m_pAsyncTimeout( new QTimer( this ) )
	, m_nAsyncIndex( 0 )
	, m_nAsyncSuccess( 0 )
	, m_nAsyncFail( 0 )
{
	m_pAsyncTimeout->setSingleShot( true );
	connect( m_pAsyncTimeout, &QTimer::timeout,
			 this, &OnlineImporter::onAsyncTimeout );
}

OnlineImporter::~OnlineImporter()
{
}

QString OnlineImporter::deriveSourceFolder( const QUrl& sourceUrl )
{
	// Strip protocol prefix (e.g. "https://") — QUrl::RemovedScheme is Qt6 only,
	// so we do it manually for Qt 5.15+ compatibility.
	QString s = sourceUrl.toString();
	int nSchemeEnd = s.indexOf( "://" );
	if ( nSchemeEnd > 0 ) {
		s.remove( 0, nSchemeEnd + 3 );
	}
	// Strip trailing /index.json
	if ( s.endsWith( "/index.json" ) ) {
		s.chop( 11 ); // length of "/index.json"
	}
	// Replace remaining path separators with underscores for a flat folder name
	s.replace( '/', '_' );
	return s;
}

OnlineIndex OnlineImporter::parseIndex( const QByteArray& jsonData,
                                         const QUrl& sourceUrl )
{
	OnlineIndex index;
	index.sourceUrl = sourceUrl;

	QJsonParseError parseError;
	const QJsonDocument doc = QJsonDocument::fromJson( jsonData, &parseError );
	if ( parseError.error != QJsonParseError::NoError ) {
		ERRORLOG( QString( "Failed to parse index JSON from '%1': %2" )
					  .arg( sourceUrl.toString() )
					  .arg( parseError.errorString() ) );
		return index;
	}

	if ( !doc.isObject() ) {
		ERRORLOG( QString( "Index from '%1' is not a JSON object" )
					  .arg( sourceUrl.toString() ) );
		return index;
	}

	const QJsonObject root = doc.object();

	// Top-level metadata
	index.sVersion = root.value( "version" ).toString();
	index.sCreated = root.value( "created" ).toString();
	index.sHash = root.value( "hash" ).toString();

	// Top-level hash validation (optional)
	if ( !index.sHash.isEmpty() ) {
		if ( !verifyIndexHash( root, index.sHash ) ) {
			WARNINGLOG( QString( "Top-level hash mismatch in index from '%1'" )
							.arg( sourceUrl.toString() ) );
		}
	}

	// Parse artifact arrays
	const int nDeclaredPatternCount = root.value( "patternCount" ).toInt( -1 );
	const int nDeclaredSongCount = root.value( "songCount" ).toInt( -1 );
	const int nDeclaredDrumkitCount = root.value( "drumkitCount" ).toInt( -1 );

	auto parseArtifactArray = [&]( const QString& sKey,
								   OnlineArtifact::Type type,
								   QVector<OnlineArtifact>& target ) {
		const QJsonArray arr = root.value( sKey ).toArray();
		for ( const auto& val : arr ) {
			if ( !val.isObject() ) {
				continue;
			}
			const QJsonObject obj = val.toObject();

			// Required fields
			const QString sName = obj.value( "name" ).toString();
			const QString sUrl = obj.value( "url" ).toString();
			const QString sHash = obj.value( "hash" ).toString();

			if ( sName.isEmpty() || sUrl.isEmpty() || sHash.isEmpty() ) {
				WARNINGLOG( QString( "Skipping malformed '%1' entry in index "
									 "from '%2': missing name, url, or hash" )
								.arg( sKey )
								.arg( sourceUrl.toString() ) );
				continue;
			}

			OnlineArtifact artifact;
			artifact.type = type;
			artifact.sName = sName;
			artifact.url = QUrl( sUrl );
			artifact.sourceUrl = sourceUrl;
			artifact.sHash = sHash;
			artifact.sAuthor = obj.value( "author" ).toString();
			artifact.sDescription = obj.value( "description" ).toString();
			artifact.nVersion = obj.value( "version" ).toInt( 0 );
			artifact.nFormatVersion = obj.value( "formatVersion" ).toInt( 0 );
			artifact.size = static_cast<qint64>(
				obj.value( "size" ).toDouble( 0 ) );
			artifact.sLicense = obj.value( "license" ).toString();

			// Tags
			const QJsonArray tagsArray = obj.value( "tags" ).toArray();
			for ( const auto& tag : tagsArray ) {
				if ( tag.isString() ) {
					artifact.tags.append( tag.toString() );
				}
			}

			// Type-specific fields
			artifact.nNotes = obj.value( "notes" ).toInt( -1 );
			artifact.nPatternCount = obj.value( "patterns" ).toInt( -1 );
			artifact.sFolderName = obj.value( "folderName" ).toString();
			artifact.nInstruments = obj.value( "instruments" ).toInt( -1 );
			artifact.nComponents = obj.value( "components" ).toInt( -1 );
			artifact.nSamples = obj.value( "samples" ).toInt( -1 );

			// instrumentTypes (shared by patterns and drumkits)
			const QJsonArray instrumentTypesArray =
				obj.value( "instrumentTypes" ).toArray();
			for ( const auto& it : instrumentTypesArray ) {
				if ( it.isString() ) {
					artifact.instrumentTypes.append( it.toString() );
				}
			}

			artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;

			target.append( artifact );
		}
	};

	parseArtifactArray( "patterns", OnlineArtifact::Type::Pattern,
						index.patterns );
	parseArtifactArray( "songs", OnlineArtifact::Type::Song, index.songs );
	parseArtifactArray( "drumkits", OnlineArtifact::Type::Drumkit,
						index.drumkits );

	// Count mismatch warnings
	if ( nDeclaredPatternCount >= 0 &&
		 nDeclaredPatternCount != index.patterns.size() ) {
		WARNINGLOG( QString( "Pattern count mismatch in index from '%1': "
							 "declared %2, parsed %3" )
						.arg( sourceUrl.toString() )
						.arg( nDeclaredPatternCount )
						.arg( index.patterns.size() ) );
	}
	if ( nDeclaredSongCount >= 0 &&
		 nDeclaredSongCount != index.songs.size() ) {
		WARNINGLOG( QString( "Song count mismatch in index from '%1': "
							 "declared %2, parsed %3" )
						.arg( sourceUrl.toString() )
						.arg( nDeclaredSongCount )
						.arg( index.songs.size() ) );
	}
	if ( nDeclaredDrumkitCount >= 0 &&
		 nDeclaredDrumkitCount != index.drumkits.size() ) {
		WARNINGLOG( QString( "Drumkit count mismatch in index from '%1': "
							 "declared %2, parsed %3" )
						.arg( sourceUrl.toString() )
						.arg( nDeclaredDrumkitCount )
						.arg( index.drumkits.size() ) );
	}

	return index;
}

bool OnlineImporter::verifyHash( const QByteArray& data,
                                  const QString& sExpectedHash )
{
	const QString sComputed =
		QCryptographicHash::hash( data, QCryptographicHash::Sha256 ).toHex();
	return sComputed.compare( sExpectedHash, Qt::CaseInsensitive ) == 0;
}

bool OnlineImporter::verifyIndexHash( const QJsonObject& root,
                                      const QString& sExpectedHash )
{
	QJsonObject rootWithoutHash = root;
	rootWithoutHash.remove( "hash" );
	const QByteArray dataWithoutHash =
		QJsonDocument( rootWithoutHash ).toJson( QJsonDocument::Compact );
	const QString sComputedHash =
		QCryptographicHash::hash( dataWithoutHash, QCryptographicHash::Sha256 )
			.toHex();
	return sComputedHash == sExpectedHash;
}

void OnlineImporter::setLocalSearchPath( OnlineArtifact::Type type,
                                         const QString& sPath )
{
	m_localSearchPaths[static_cast<int>( type )] = sPath;
}

void OnlineImporter::resolveLocalStatus( OnlineArtifact& artifact )
{
	const int nTypeKey = static_cast<int>( artifact.type );

	// Test override: if a local search path is set, use the legacy
	// file-based approach (with version comparison).
	if ( m_localSearchPaths.contains( nTypeKey ) ) {
		resolveLocalStatusFromPath( artifact,
								   m_localSearchPaths.value( nTypeKey ) );
		return;
	}

	// Production path: use SoundLibraryDatabase
	auto pHydrogen = Hydrogen::get_instance();
	if ( pHydrogen == nullptr ) {
		WARNINGLOG( "Hydrogen instance not available — cannot resolve "
					"local status" );
		artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
		return;
	}

	auto pDB = pHydrogen->getSoundLibraryDatabase();
	if ( pDB == nullptr ) {
		WARNINGLOG( "SoundLibraryDatabase not available" );
		artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
		return;
	}

	// Find a matching local artifact by name and source URL in the User
	// context.
	const QString sSourceFolder = deriveSourceFolder( artifact.sourceUrl );
	QString sRootFolder;
	const std::vector<std::shared_ptr<SoundLibraryInfo>>* pInfos = nullptr;
	switch ( artifact.type ) {
	case OnlineArtifact::Type::Pattern:
		pInfos = &pDB->getPatternInfos();
		sRootFolder = Filesystem::userPatternsDir();
		break;
	case OnlineArtifact::Type::Song:
		pInfos = &pDB->getSongInfos();
		sRootFolder = Filesystem::userSongsDir();
		break;
	case OnlineArtifact::Type::Drumkit:
		pInfos = &pDB->getDrumkitInfos();
		sRootFolder = Filesystem::userDrumkitsDir();
		break;
	}

	if ( pInfos == nullptr ) {
		artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
		return;
	}

	if ( !sSourceFolder.isEmpty() ) {
		sRootFolder += sSourceFolder + "/";
	}
	if ( !Filesystem::dirExists( sRootFolder, true ) ) {
		artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
		return;
	}

	QString sTargetPath;
	if ( artifact.type == OnlineArtifact::Type::Drumkit ) {
		QString sTargetDir = Filesystem::userDrumkitsDir();
		if ( ! sSourceFolder.isEmpty() ) {
			sTargetDir += sSourceFolder + "/";
		}
		sTargetDir += artifact.sFolderName;
		if ( !Filesystem::dirExists( sTargetDir, true ) ) {
			artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
			return;
		}
		sTargetPath = Filesystem::drumkitPathFromDir( sTargetDir );
	}
	else {
		sTargetPath = artifactToTargetPath( artifact, nullptr );
	}

	std::shared_ptr<SoundLibraryInfo> pLocalInfo = nullptr;
	for ( const auto& pInfo : *pInfos ) {
		if ( pInfo != nullptr &&
			 pInfo->getContext() == Filesystem::Context::User &&
			 pInfo->getPath() == sTargetPath ) {
			pLocalInfo = pInfo;
			break;
		}
	}

	if ( pLocalInfo == nullptr ) {
		artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
		return;
	}

	if ( artifact.type == OnlineArtifact::Type::Drumkit ) {

		// Drumkits are directories — no hash comparison possible.
		if ( artifact.sName == pLocalInfo->getName() &&
			 artifact.sAuthor == pLocalInfo->getAuthor() ) {
			if ( artifact.nVersion == pLocalInfo->getVersion() ) {
				artifact.localStatus = OnlineArtifact::LocalStatus::Installed;
			}
			else if ( artifact.nVersion > pLocalInfo->getVersion() ) {
				artifact.localStatus =
					OnlineArtifact::LocalStatus::UpdateAvailable;
			}
			else {
				artifact.localStatus = OnlineArtifact::LocalStatus::Modified;
			}
		}
		else {
			artifact.localStatus = OnlineArtifact::LocalStatus::Modified;
		}
	}
	else {
		// Patterns and songs: hash the local file for exact match detection.
		QFile file( pLocalInfo->getPath() );
		if ( !file.open( QIODevice::ReadOnly ) ) {
			WARNINGLOG( QString( "Unable to open local file '%1' for hash "
								 "comparison" )
							.arg( pLocalInfo->getPath() ) );
			artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
			return;
		}

		const QByteArray localData = file.readAll();
		file.close();

		if ( verifyHash( localData, artifact.sHash ) ) {
			artifact.localStatus = OnlineArtifact::LocalStatus::Installed;
		}
		else if ( artifact.nVersion > pLocalInfo->getVersion() ) {
			artifact.localStatus =
				OnlineArtifact::LocalStatus::UpdateAvailable;
		}
		else {
			// Hash differs but version is not newer → local modification
			artifact.localStatus = OnlineArtifact::LocalStatus::Modified;
		}
	}
}

void OnlineImporter::resolveAllLocalStatuses( OnlineIndex& index )
{
	for ( auto& artifact : index.patterns ) {
		resolveLocalStatus( artifact );
	}
	for ( auto& artifact : index.songs ) {
		resolveLocalStatus( artifact );
	}
	for ( auto& artifact : index.drumkits ) {
		resolveLocalStatus( artifact );
	}
}

void OnlineImporter::resolveLocalStatusFromPath( OnlineArtifact& artifact,
                                                 const QString& sSearchDir )
{
	if ( artifact.type == OnlineArtifact::Type::Drumkit ) {
		// Drumkits are directories — no hash comparison possible.
		const QDir dir( sSearchDir + "/" + artifact.sName );
		if ( !dir.exists() ) {
			artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
			return;
		}
		// Without SoundLibraryDatabase we cannot reliably determine the
		// local version from test fixtures. Mark as Installed if directory
		// exists.
		artifact.localStatus = OnlineArtifact::LocalStatus::Installed;
	}
	else {
		// Patterns and songs are single files
		const QString sSuffix =
			( artifact.type == OnlineArtifact::Type::Pattern )
				? ".h2pattern"
				: ".h2song";
		const QString sFilePath =
			sSearchDir + "/" + artifact.sName + sSuffix;

		QFile file( sFilePath );
		if ( !file.exists() ) {
			artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
			return;
		}

		if ( !file.open( QIODevice::ReadOnly ) ) {
			WARNINGLOG( QString( "Unable to open local file '%1' for hash "
								 "comparison" )
							.arg( sFilePath ) );
			artifact.localStatus = OnlineArtifact::LocalStatus::NotInstalled;
			return;
		}

		const QByteArray localData = file.readAll();
		file.close();

		if ( verifyHash( localData, artifact.sHash ) ) {
			artifact.localStatus = OnlineArtifact::LocalStatus::Installed;
		}
		else {
			// In test mode (setLocalSearchPath) we don't have access to
			// local version metadata from SoundLibraryInfo. Use version
			// from the artifact struct to determine status: if the
			// remote version > 0, assume local version is 0 (default)
			// for test simplicity.  Real production code uses
			// resolveLocalStatus() which queries SoundLibraryDatabase.
			artifact.localStatus = OnlineArtifact::LocalStatus::Modified;
		}
	}
}

QByteArray OnlineImporter::downloadBlocking( const QUrl& url,
                                             int nTimeoutMs,
                                             QString* pError )
{
	assert( QCoreApplication::instance() != nullptr );

	QNetworkAccessManager nam;
	QNetworkRequest request( url );
	request.setRawHeader( "User-Agent", "Hydrogen" );
	request.setAttribute( QNetworkRequest::RedirectPolicyAttribute,
						  QNetworkRequest::NoLessSafeRedirectPolicy );
	request.setMaximumRedirectsAllowed( 10 );

	QNetworkReply* pReply = nam.get( request );

	QEventLoop loop;
	QTimer timer;
	timer.setSingleShot( true );

	QObject::connect( pReply, &QNetworkReply::finished, &loop, &QEventLoop::quit );
	QObject::connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
	timer.start( nTimeoutMs );

	loop.exec();

	if ( !timer.isActive() ) {
		// Timeout occurred
		pReply->abort();
		pReply->deleteLater();
		const QString sErr = QString( "Download of '%1' timed out after %2ms" )
								 .arg( url.toString() )
								 .arg( nTimeoutMs );
		ERRORLOG( sErr );
		if ( pError != nullptr ) {
			*pError = sErr;
		}
		return QByteArray();
	}

	timer.stop();

	if ( pReply->error() != QNetworkReply::NoError ) {
		const QString sErr =
			QString( "Download of '%1' failed: %2" )
				.arg( url.toString() )
				.arg( pReply->errorString() );
		ERRORLOG( sErr );
		if ( pError != nullptr ) {
			*pError = sErr;
		}
		pReply->deleteLater();
		return QByteArray();
	}

	const QByteArray data = pReply->readAll();
	pReply->deleteLater();
	return data;
}

OnlineIndex OnlineImporter::fetchAndParseIndex( const QUrl& url,
                                                int nTimeoutMs,
                                                QString* pError )
{
	const QByteArray data = downloadBlocking( url, nTimeoutMs, pError );
	if ( data.isEmpty() ) {
		return OnlineIndex();
	}
	return parseIndex( data, url );
}

QVector<OnlineIndex> OnlineImporter::fetchAllIndices( const QStringList& urls )
{
	QVector<OnlineIndex> results;
	for ( const auto& sUrl : urls ) {
		QString sError;
		const auto index = fetchAndParseIndex(
			QUrl( sUrl ), OnlineImporter::nDefaultTimeoutMs, &sError
		);
		if ( !sError.isEmpty() ) {
			WARNINGLOG( QString( "Failed to fetch index '%1': %2" )
							.arg( sUrl )
							.arg( sError ) );
		}
		else {
			results.append( index );
		}
	}
	return results;
}

bool OnlineImporter::downloadArtifactBlocking( const OnlineArtifact& artifact,
                                               QString* pError )
{
	const QByteArray data = downloadBlocking(
		artifact.url, OnlineImporter::nDefaultTimeoutMs, pError
	);
	if ( data.isEmpty() ) {
		return false;
	}
	return installDownloadedArtifact( artifact, data, pError );
}

bool OnlineImporter::installDownloadedArtifact(
	const OnlineArtifact& artifact,
	const QByteArray& data,
	QString* pError )
{
	// Verify hash
	if ( !verifyHash( data, artifact.sHash ) ) {
		const QString sErr =
			QString( "Hash mismatch for artifact '%1'. Discarding download." )
				.arg( artifact.sName );
		ERRORLOG( sErr );
		if ( pError != nullptr ) {
			*pError = sErr;
		}
		return false;
	}

	QString sDestDir;
	QString sDestPath = artifactToTargetPath( artifact, &sDestDir );

	// Ensure the (possibly nested) destination directory exists
	if ( artifact.type != OnlineArtifact::Type::Drumkit ) {
		Filesystem::mkdir( sDestDir );
	}

	QFile file( sDestPath );
	if ( !file.open( QIODevice::WriteOnly ) ) {
		const QString sErr =
			QString( "Unable to write to '%1'" ).arg( sDestPath );
		ERRORLOG( sErr );
		if ( pError != nullptr ) {
			*pError = sErr;
		}
		return false;
	}

	file.write( data );
	file.close();

	if ( artifact.type == OnlineArtifact::Type::Drumkit ) {
		QString sInstalledDir;
		QString sTargetDir = Filesystem::userDrumkitsDir() +
							 deriveSourceFolder( artifact.sourceUrl );
		if ( !Drumkit::install(
				 sDestPath, sTargetDir, &sInstalledDir, nullptr, false
			 ) ) {
			const QString sErr =
				QString( "Unable to install bundled drumkit [%1] to [%2]" )
					.arg( sDestPath ).arg( sTargetDir );
			ERRORLOG( sErr );
			if ( pError != nullptr ) {
				*pError = sErr;
			}
			return false;
		}
		Filesystem::rm( sDestPath );
		sDestPath = sInstalledDir;
	}
	INFOLOG( QString( "Successfully downloaded artifact '%1' to '%2'" )
				 .arg( artifact.sName )
				 .arg( sDestPath ) );
	return true;
}

void OnlineImporter::downloadArtifactsAsync(
	const QVector<OnlineArtifact>& artifacts )
{
	if ( m_pAsyncReply != nullptr ||
		 m_nAsyncIndex < m_asyncQueue.size() ) {
		WARNINGLOG( "Async batch already in progress; ignoring new request" );
		return;
	}

	m_bAborted = false;
	m_asyncQueue = artifacts;
	m_nAsyncIndex = 0;
	m_nAsyncSuccess = 0;
	m_nAsyncFail = 0;

	if ( m_asyncQueue.isEmpty() ) {
		finishAsyncBatch();
		return;
	}

	startNextAsyncDownload();
}

void OnlineImporter::startNextAsyncDownload()
{
	if ( m_bAborted || m_nAsyncIndex >= m_asyncQueue.size() ) {
		finishAsyncBatch();
		return;
	}

	const auto& artifact = m_asyncQueue[m_nAsyncIndex];

	QNetworkRequest request( artifact.url );
	request.setRawHeader( "User-Agent", "Hydrogen" );
	request.setAttribute( QNetworkRequest::RedirectPolicyAttribute,
						  QNetworkRequest::NoLessSafeRedirectPolicy );
	request.setMaximumRedirectsAllowed( 10 );

	m_pAsyncReply = m_pAsyncNAM->get( request );
	connect( m_pAsyncReply, &QNetworkReply::finished,
			 this, &OnlineImporter::onAsyncReplyFinished );
	// Forward per-request byte progress, but scaled so it represents the
	// current artifact's slice of the overall batch (see onAsyncBytesProgress).
	connect( m_pAsyncReply, &QNetworkReply::downloadProgress,
			 this, &OnlineImporter::onAsyncBytesProgress );

	// Emit a progress tick at the start of each artifact so the bar advances
	// even before any bytes arrive (e.g. during DNS / TLS handshake).
	emit downloadProgress( static_cast<qint64>( m_nAsyncIndex ) * 100,
						   static_cast<qint64>( m_asyncQueue.size() ) * 100 );

	m_pAsyncTimeout->start( nDefaultTimeoutMs );
}

void OnlineImporter::onAsyncBytesProgress( qint64 bytesReceived,
                                            qint64 bytesTotal )
{
	if ( m_asyncQueue.isEmpty() ) {
		return;
	}
	// Each artifact contributes 100 units to overall progress. Within the
	// current artifact, byte progress contributes 0..100 of those units when
	// the total is known; otherwise the slice stays at its lower bound.
	int nWithinArtifact = 0;
	if ( bytesTotal > 0 ) {
		nWithinArtifact = static_cast<int>(
			( bytesReceived * 100 ) / bytesTotal );
		if ( nWithinArtifact > 100 ) {
			nWithinArtifact = 100;
		}
	}
	const qint64 nProgress =
		static_cast<qint64>( m_nAsyncIndex ) * 100 + nWithinArtifact;
	const qint64 nLimit = static_cast<qint64>( m_asyncQueue.size() ) * 100;
	emit downloadProgress( nProgress, nLimit );
}

void OnlineImporter::onAsyncReplyFinished()
{
	m_pAsyncTimeout->stop();

	QNetworkReply* pReply = m_pAsyncReply;
	if ( pReply == nullptr ) {
		// Spurious or post-cleanup signal — ignore.
		return;
	}
	m_pAsyncReply = nullptr;

	if ( m_bAborted ) {
		INFOLOG( "Async batch aborted by user" );
		pReply->deleteLater();
		finishAsyncBatch();
		return;
	}

	const auto& artifact = m_asyncQueue[m_nAsyncIndex];

	bool bOk = false;
	QString sError;

	if ( pReply->error() != QNetworkReply::NoError ) {
		sError = QString( "Download of '%1' failed: %2" )
					 .arg( artifact.url.toString() )
					 .arg( pReply->errorString() );
		ERRORLOG( sError );
	}
	else {
		const QByteArray data = pReply->readAll();
		bOk = installDownloadedArtifact( artifact, data, &sError );
	}
	pReply->deleteLater();

	if ( bOk ) {
		++m_nAsyncSuccess;
		emit downloadFinished( artifact.sName, true, QString() );
	}
	else {
		++m_nAsyncFail;
		emit downloadFinished( artifact.sName, false, sError );
		EventQueue::get_instance()->pushEvent(
			Event::Type::OnlineImportProgress, nProgressError );
	}

	++m_nAsyncIndex;

	// Snap progress to the artifact boundary — covers both the success path
	// (QNAM's final (total, total) tick may not arrive) and the failure path
	// (no progress signal at all). Bytes within the next artifact will refine
	// from this point.
	emit downloadProgress(
		static_cast<qint64>( m_nAsyncIndex ) * 100,
		static_cast<qint64>( m_asyncQueue.size() ) * 100 );

	// Report overall batch progress as percentage (0–100) on the EventQueue.
	const int nPercent = static_cast<int>(
		( m_nAsyncIndex * 100 ) / m_asyncQueue.size() );
	EventQueue::get_instance()->pushEvent(
		Event::Type::OnlineImportProgress, nPercent );

	startNextAsyncDownload();
}

void OnlineImporter::onAsyncTimeout()
{
	if ( m_pAsyncReply != nullptr ) {
		WARNINGLOG( QString( "Async download timed out after %1ms" )
						.arg( nDefaultTimeoutMs ) );
		// Aborting the reply triggers QNetworkReply::finished with
		// OperationCanceledError; onAsyncReplyFinished handles the rest.
		m_pAsyncReply->abort();
	}
}

void OnlineImporter::finishAsyncBatch()
{
	m_nAsyncIndex = 0;
	m_asyncQueue.clear();
	emit batchFinished( m_nAsyncSuccess, m_nAsyncFail );
	EventQueue::get_instance()->pushEvent(
		Event::Type::OnlineImportProgress, nProgressComplete );
}

void OnlineImporter::abort()
{
	m_bAborted = true;

	QNetworkReply* pReply = m_pAsyncReply;
	if ( pReply == nullptr ) {
		// No request in flight. Either the batch already finished, or we
		// were called re-entrantly from inside onAsyncReplyFinished — in
		// that case startNextAsyncDownload() will observe m_bAborted and
		// finish the batch for us. Either way, do not emit batchFinished
		// here to avoid a double emission.
		return;
	}

	// Disconnect first so the reply's finished/downloadProgress signals do
	// not re-enter our slots from inside QNetworkReply::abort() — that path
	// has caused crashes when our slot tears down state while Qt's network
	// code is still unwinding. Finish the batch synchronously below instead.
	m_pAsyncReply = nullptr;
	m_pAsyncTimeout->stop();
	pReply->disconnect( this );
	pReply->abort();
	pReply->deleteLater();

	finishAsyncBatch();
}

QString OnlineImporter::artifactToTargetPath(
	const OnlineArtifact& artifact,
	QString* pTargetDir
) const
{
	// Determine destination path within a per-source subfolder
	QString sSourceFolder = deriveSourceFolder( artifact.sourceUrl );
	QString sDestDir;
	QString sSuffix;
	switch ( artifact.type ) {
		case OnlineArtifact::Type::Pattern:
			sDestDir = Filesystem::userPatternsDir() + sSourceFolder;
			sSuffix = ".h2pattern";
			break;
		case OnlineArtifact::Type::Song:
			sDestDir = Filesystem::userSongsDir() + sSourceFolder;
			sSuffix = ".h2song";
			break;
		case OnlineArtifact::Type::Drumkit:
			// Only bundled drumkits can be downloaded. We do so into a
			// temporary file and extract the kit right away.
			sDestDir = Filesystem::cacheDir();
			sSuffix = ".h2drumkit";
			break;
	}

	if ( pTargetDir != nullptr ) {
		*pTargetDir = sDestDir;
	}

	return sDestDir + "/" + artifact.sName + sSuffix;
}

QString OnlineArtifact::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;

	QString sType;
	switch ( type ) {
		case Type::Pattern:
			sType = "Pattern";
			break;
		case Type::Song:
			sType = "Song";
			break;
		case Type::Drumkit:
			sType = "Drumkit";
			break;
	}

	QString sLocalStatus;
	switch ( localStatus ) {
		case LocalStatus::NotInstalled:
			sLocalStatus = "NotInstalled";
			break;
		case LocalStatus::Installed:
			sLocalStatus = "Installed";
			break;
		case LocalStatus::Modified:
			sLocalStatus = "Modified";
			break;
		case LocalStatus::UpdateAvailable:
			sLocalStatus = "UpdateAvailable";
			break;
	}

	if ( !bShort ) {
		sOutput = QString( "%1[OnlineArtifact]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2type: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sType ) )
					  .append( QString( "%1%2sName: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sName ) )
					  .append( QString( "%1%2url: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( url.toString() ) )
					  .append( QString( "%1%2sourceUrl: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sourceUrl.toString() ) )
					  .append( QString( "%1%2sHash: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sHash ) )
					  .append( QString( "%1%2sAuthor: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sAuthor ) )
					  .append( QString( "%1%2sDescription: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sDescription ) )
					  .append( QString( "%1%2nVersion: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nVersion ) )
					  .append( QString( "%1%2nFormatVersion: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nFormatVersion ) )
					  .append( QString( "%1%2tags: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( tags.join( ", " ) ) )
					  .append( QString( "%1%2size: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( size ) )
					  .append( QString( "%1%2sLicense: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sLicense ) )
					  .append( QString( "%1%2nNotes: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nNotes ) )
					  .append( QString( "%1%2nPatternCount: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nPatternCount ) )
					  .append( QString( "%1%2sFolderName: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sFolderName ) )
					  .append( QString( "%1%2nInstruments: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nInstruments ) )
					  .append( QString( "%1%2nComponents: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nComponents ) )
					  .append( QString( "%1%2nSamples: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( nSamples ) )
					  .append( QString( "%1%2instrumentTypes: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( instrumentTypes.join( ", " ) ) )
					  .append( QString( "%1%2localStatus: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sLocalStatus ) );
	}
	else {
		sOutput =
			QString( "[OnlineArtifact]" )
				.append( QString( " type: %1" ).arg( sType ) )
				.append( QString( ", sName: %1" ).arg( sName ) )
				.append( QString( ", url: %1" ).arg( url.toString() ) )
				.append(
					QString( ", sourceUrl: %1" ).arg( sourceUrl.toString() )
				)
				.append( QString( ", sHash: %1" ).arg( sHash ) )
				.append( QString( ", sAuthor: %1" ).arg( sAuthor ) )
				.append( QString( ", sDescription: %1" ).arg( sDescription ) )
				.append( QString( ", nVersion: %1" ).arg( nVersion ) )
				.append(
					QString( ", nFormatVersion: %1" ).arg( nFormatVersion )
				)
				.append( QString( ", tags: %1" ).arg( tags.join( ", " ) ) )
				.append( QString( ", size: %1" ).arg( size ) )
				.append( QString( ", sLicense: %1" ).arg( sLicense ) )
				.append( QString( ", nNotes: %1" ).arg( nNotes ) )
				.append( QString( ", nPatternCount: %1" ).arg( nPatternCount )
				)
				.append( QString( ", sFolderName: %1" ).arg( sFolderName ) )
				.append( QString( ", nInstruments: %1" ).arg( nInstruments ) )
				.append( QString( ", nComponents: %1" ).arg( nComponents ) )
				.append( QString( ", nSamples: %1" ).arg( nSamples ) )
				.append( QString( ", instrumentTypes: %1" )
							 .arg( instrumentTypes.join( ", " ) ) )
				.append( QString( ", localStatus: %1" ).arg( sLocalStatus ) );
	}

	return sOutput;
}

QString OnlineIndex::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;

	if ( !bShort ) {
		sOutput = QString( "%1[OnlineIndex]\n" )
					  .arg( sPrefix )
					  .append( QString( "%1%2sVersion: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sVersion ) )
					  .append( QString( "%1%2sCreated: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sCreated ) )
					  .append( QString( "%1%2sourceUrl: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sourceUrl.toString() ) )
					  .append( QString( "%1%2sHash: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( sHash ) )
					  .append( QString( "%1%2patterns: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( patterns.size() ) )
					  .append( QString( "%1%2songs: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( songs.size() ) )
					  .append( QString( "%1%2drumkits: %3\n" )
								   .arg( sPrefix )
								   .arg( s )
								   .arg( drumkits.size() ) );
	}
	else {
		sOutput =
			QString( "[OnlineIndex] " )
				.append( QString( "sVersion: %1" ).arg( sVersion ) )
				.append( QString( ", sCreated: %1" ).arg( sCreated ) )
				.append(
					QString( ", sourceUrl: %1" ).arg( sourceUrl.toString() )
				)
				.append( QString( ", sHash: %1" ).arg( sHash ) )
				.append( QString( ", patterns: %1" ).arg( patterns.size() ) )
				.append( QString( ", songs: %1" ).arg( songs.size() ) )
				.append( QString( ", drumkits: %1" ).arg( drumkits.size() ) );
	}

	return sOutput;
}

} // namespace H2Core
