/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#ifndef ONLINE_IMPORTER_H
#define ONLINE_IMPORTER_H

#include <core/Object.h>

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QUrl>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace H2Core {

class Hydrogen;

/** Describes a single downloadable artifact from an online index. */
struct OnlineArtifact {
	enum class Type { Pattern, Song, Drumkit };

	Type type;
	QString sName;
	QUrl url;
	/** URL of the index this artifact was fetched from */
	QUrl sourceUrl;
	/** sha256 hex string */
	QString sHash;
	QString sAuthor;
	QString sDescription;
	/** User-set revision number */
	int nVersion;
	/** Hydrogen format revision */
	int nFormatVersion;
	QStringList tags;
	/** Size in bytes */
	qint64 size;
	QString sLicense;

	// Pattern-specific
	/** Number of notes; -1 if not applicable */
	int nNotes;

	// Song-specific
	/** Number of patterns in the song; -1 if not applicable */
	int nPatternCount;

	// Drumkit-specific
	/** Name of the top-level folder in the archive. */
	QString sFolderName;
	/** Number of instruments; -1 if not applicable */
	int nInstruments;
	/** Number of components; -1 if not applicable */
	int nComponents;
	/** Number of samples; -1 if not applicable */
	int nSamples;

	// Shared by patterns AND drumkits (used for instrument↔pattern mapping)
	/** Instrument type list; empty if not applicable */
	QStringList instrumentTypes;

	/** Installation state computed locally — not derived from the index JSON */
	enum class LocalStatus { NotInstalled, Installed, Modified, UpdateAvailable };
	LocalStatus localStatus;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
};

/** Represents a parsed online index document fetched from a remote URL. */
struct OnlineIndex {
	/** hydrogen-index format version */
	QString sVersion;
	/** ISO 8601 creation timestamp */
	QString sCreated;
	/** URL this index was fetched from */
	QUrl sourceUrl;

	QVector<OnlineArtifact> patterns;
	QVector<OnlineArtifact> songs;
	QVector<OnlineArtifact> drumkits;

	/** Optional top-level hash for integrity verification */
	QString sHash;

	QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
};

/**
 * Fetches and parses online artifact indices, resolves local installation
 * status, and manages artifact downloads.
 *
 * \b Requirements:
 * - A QCoreApplication instance must exist before constructing this class,
 *   as it relies on Qt's network stack and event loop infrastructure.
 *
 * \b Threading:
 * - downloadBlocking() and downloadArtifactBlocking() MUST NOT be called from
 *   the main GUI thread. They spin a local QEventLoop which causes reentrancy
 *   and will deadlock or corrupt state when invoked on the GUI thread. Use
 *   them only from worker threads, CLI contexts, or tests.
 * - downloadArtifactsAsync() is safe to call from the GUI thread. It drives a
 *   single QNetworkAccessManager via signals/slots — no nested event loop is
 *   spun — and communicates results via downloadProgress(),
 *   downloadFinished(), and batchFinished() signals.
 *
 * Internally uses QNetworkAccessManager paired with a local QEventLoop for
 * synchronous operations; the async path uses a member QNetworkAccessManager
 * with a timeout QTimer and chains artifacts in onAsyncReplyFinished().
 *
 * \ingroup docCore
 */
class H2CORE_API OnlineImporter : public QObject, public H2Core::Object<OnlineImporter> {
	H2_OBJECT(OnlineImporter)
	Q_OBJECT

public:
	/** Sentinel value emitted as progress when a download has failed. */
	static constexpr int nProgressError = -1;
	/** Sentinel value emitted as progress when a download has completed. */
	static constexpr int nProgressComplete = 101;
		static constexpr int nDefaultTimeoutMs = 60000;

	explicit OnlineImporter( Hydrogen* pHydrogen, QObject* pParent = nullptr );
	~OnlineImporter();

	// --- Index operations ---

	/**
	 * Synchronously fetches \a url and parses it as an index document.
	 *
	 * On failure, returns a default-constructed OnlineIndex and, if
	 * \a pError is non-null, writes a human-readable description into it.
	 */
	OnlineIndex fetchAndParseIndex( const QUrl& url,
	                                int nTimeoutMs = nDefaultTimeoutMs,
	                                QString* pError = nullptr );

	/** Parses raw JSON \a jsonData as an index originating from \a sourceUrl. */
	OnlineIndex parseIndex( const QByteArray& jsonData, const QUrl& sourceUrl );

	/** Fetches and parses every index in \a urls, returning all results. */
	QVector<OnlineIndex> fetchAllIndices( const QStringList& urls );

	// --- Local status ---

	/** Checks the local installation state and updates \a artifact.localStatus. */
	void resolveLocalStatus( OnlineArtifact& artifact );

	/** Calls resolveLocalStatus() for every artifact in \a index. */
	void resolveAllLocalStatuses( OnlineIndex& index );

	/**
	 * Override the local search path for a given artifact type.
	 * Used in tests to redirect lookups to a temporary directory.
	 * If not set, the default Filesystem::user*Dir() paths are used.
	 */
	void setLocalSearchPath( OnlineArtifact::Type type, const QString& sPath );

	// --- Download operations (synchronous — CLI/tests only, NOT GUI thread) ---

	/**
	 * Blocks until \a url has been fully downloaded and returns the raw bytes.
	 *
	 * \warning MUST NOT be called from the main GUI thread. See class docs.
	 */
	QByteArray downloadBlocking( const QUrl& url,
	                             int nTimeoutMs = nDefaultTimeoutMs,
	                             QString* pError = nullptr );

	/**
	 * Downloads and installs a single \a artifact synchronously.
	 *
	 * \warning MUST NOT be called from the main GUI thread. See class docs.
	 * \return \c true on success; on failure writes a description into
	 *         \a pError if non-null.
	 */
	bool downloadArtifactBlocking( const OnlineArtifact& artifact,
	                               QString* pError = nullptr );

	// --- Download operations (asynchronous — safe from GUI thread) ---

	/**
	 * Begins asynchronous download and installation of all \a artifacts.
	 *
	 * Returns immediately. Artifacts are downloaded one at a time via
	 * QNetworkAccessManager; the next download starts from the finished
	 * slot of the previous one. If a batch is already in progress, the
	 * call is logged and ignored.
	 *
	 * Progress and completion are reported via downloadProgress(),
	 * downloadFinished(), and batchFinished() signals.
	 */
	void downloadArtifactsAsync( const QVector<OnlineArtifact>& artifacts );

	// --- Hash verification ---

	/**
	 * Returns \c true if the SHA-256 digest of \a data matches
	 * \a sExpectedHash (hex string, case-insensitive).
	 */
	static bool verifyHash( const QByteArray& data,
	                        const QString& sExpectedHash );

	/**
	 * Verifies the top-level integrity hash of an index JSON object.
	 *
	 * Removes the "hash" key from \a root, re-serializes the result to
	 * compact JSON, and compares its SHA-256 digest with \a sExpectedHash
	 * (hex string, case-sensitive).
	 *
	 * \return \c true if the computed hash matches \a sExpectedHash.
	 */
	static bool verifyIndexHash( const QJsonObject& root,
	                             const QString& sExpectedHash );

public slots:
	// --- Control ---

	/** Requests cancellation of any in-progress download operation. */
	void abort();

signals:
	void downloadProgress( qint64 bytesReceived, qint64 bytesTotal );
	void downloadFinished( const QString& sArtifactName, bool bSuccess,
	                       const QString& sError );
	void batchFinished( int nSuccessCount, int nFailCount );

private slots:
	/** Async chain entry: handles QNetworkReply::finished for the current
	 * artifact, then advances the queue. */
	void onAsyncReplyFinished();
	/** Aborts the current async reply when the per-request timer fires. */
	void onAsyncTimeout();
	/** Receives QNetworkReply::downloadProgress for the current artifact and
	 * emits OnlineImporter::downloadProgress scaled to the whole batch. */
	void onAsyncBytesProgress( qint64 bytesReceived, qint64 bytesTotal );

private:
	/** Back-pointer to the owning Hydrogen instance (ADR 0015). */
	Hydrogen* m_pHydrogen;

	bool m_bAborted;
	/** Optional override paths for local status resolution (used in tests). */
	QMap<int, QString> m_localSearchPaths;

	// --- Async batch state (only valid while a batch is in progress) ---
	QNetworkAccessManager* m_pAsyncNAM;
	QNetworkReply* m_pAsyncReply;
	QTimer* m_pAsyncTimeout;
	QVector<OnlineArtifact> m_asyncQueue;
	int m_nAsyncIndex;
	int m_nAsyncSuccess;
	int m_nAsyncFail;

	/** Starts the next download in the async queue, or finishes the batch. */
	void startNextAsyncDownload();
	/** Emits batchFinished and pushes the completion event. */
	void finishAsyncBatch();
	/** Verifies hash, writes \a data to the target path, and installs drumkits.
	 * Returns true on success; on failure writes a description into \a pError. */
	bool installDownloadedArtifact( const OnlineArtifact& artifact,
	                                const QByteArray& data,
	                                QString* pError );

	QString artifactToTargetPath( const OnlineArtifact& artifact, QString* pTargetDir ) const;

	/** File-based status resolution using a specific directory path.
	 * Used by tests via setLocalSearchPath() to bypass SoundLibraryDatabase. */
	void resolveLocalStatusFromPath( OnlineArtifact& artifact,
	                                 const QString& sSearchDir );

	/**
	 * Derives a flat folder name from a source index URL.
	 * Strips protocol prefix and trailing /index.json, replaces remaining
	 * path separators with underscores.
	 * E.g. https://hydrogen-music.org/online/index.json → "hydrogen-music.org_online"
	 */
	static QString deriveSourceFolder( const QUrl& sourceUrl );
};

} // namespace H2Core

#endif // ONLINE_IMPORTER_H
