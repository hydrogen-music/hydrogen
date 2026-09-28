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

#include "core/Logger.h"
#include "core/Helpers/Filesystem.h"
#include <core/Version.h>

#include <cstdio>
#include <chrono>
#include <thread>
#include <QtCore/QDir>
#include <QDateTime>
#include <QFile>
#include <QTextStream>

#ifdef H2CORE_HAVE_QT6
  #include <QStringConverter>
#else
  #include <QTextCodec>
#endif

#ifdef WIN32
#include <windows.h>
#include <io.h>    // _get_osfhandle() for the append-mode write lock
#else
#include <sys/file.h>    // flock() for the append-mode write lock
#endif

namespace H2Core {

unsigned Logger::__bit_msk = 0;
Logger* Logger::__instance=nullptr;
std::atomic<int> Logger::__nInstanceLoggers( 0 );
thread_local Logger* Logger::__pCurrent = nullptr;
const char* Logger::__levels[] = { "None", "Error", "Warning", "Info", "Debug", "Ipc", "Constructors", "Locks" };
thread_local QString *Logger::pCrashContext = nullptr;

// Defined out-of-line (rather than inline in Logger.h) so the only references
// to the __pCurrent thread_local live inside the core DLL. MinGW's
// --export-all-symbols does not export TLS data symbols, so inlining these into
// a DLL consumer (h2cli, tests, gui) leaves __pCurrent undefined at link time.
Logger* Logger::currentLogger() {
	return __pCurrent != nullptr ? __pCurrent : __instance;
}

Logger::Scope::Scope( Logger* pLogger )
	: m_pPrevious( Logger::__pCurrent ) {
	Logger::__pCurrent = pLogger;
}

Logger::Scope::~Scope() {
	Logger::__pCurrent = m_pPrevious;
}

namespace {

/** Exclusive advisory lock held around each append-mode write+flush pair so
 * concurrent writers can never interleave inside a line. flock() locks per
 * open file description, so it also serializes two Logger objects within the
 * same process - which POSIX record locks (fcntl) would not do. Best
 * effort: if locking fails the write still happens (a possibly interleaved
 * line beats a lost one).
 *
 * Deliberately not QLockFile (Preferences uses it for its long-held config
 * lock, where its semantics fit): QLockFile locks through a sidecar .lock
 * file created and removed per acquisition - per-line filesystem churn plus
 * one more orphanable file per log file - and its stale-lock stealing
 * (holder-PID-alive check) misfires exactly under this lock's threat model:
 * append mode exists because PIDs are reused across sessions, so a reused
 * PID makes a dead holder look alive (permanent lockout) while a steal can
 * race a live holder mid-write. The kernel drops flock()/LockFileEx() when
 * the fd or process dies - no heuristics - and the failure bias matches the
 * logger's: never block, never drop a line. */
class LogWriteLock {
	public:
		LogWriteLock( QFile& logFile, bool bLock )
			: m_logFile( logFile )
			, m_bLock( bLock ) {
			if ( m_bLock ) {
				lock();
			}
		}
		~LogWriteLock() {
			if ( m_bLock ) {
				unlock();
			}
		}
		LogWriteLock( const LogWriteLock& ) = delete;
		LogWriteLock& operator=( const LogWriteLock& ) = delete;
	private:
		void lock() {
#ifdef WIN32
			HANDLE hFile = reinterpret_cast<HANDLE>(
				_get_osfhandle( m_logFile.handle() ) );
			if ( hFile != INVALID_HANDLE_VALUE ) {
				OVERLAPPED overlapped = { 0 };
				LockFileEx( hFile, LOCKFILE_EXCLUSIVE_LOCK, 0,
							MAXDWORD, MAXDWORD, &overlapped );
			}
#else
			::flock( m_logFile.handle(), LOCK_EX );
#endif
		}
		void unlock() {
#ifdef WIN32
			HANDLE hFile = reinterpret_cast<HANDLE>(
				_get_osfhandle( m_logFile.handle() ) );
			if ( hFile != INVALID_HANDLE_VALUE ) {
				OVERLAPPED overlapped = { 0 };
				UnlockFileEx( hFile, 0, MAXDWORD, MAXDWORD,
							  &overlapped );
			}
#else
			::flock( m_logFile.handle(), LOCK_UN );
#endif
		}
		QFile& m_logFile;
		bool m_bLock;
};

}

void* loggerThread_func( void* param ) {
	if ( param == nullptr ) {
		return nullptr;
	}
	Logger* pLogger = ( Logger* )param;
#ifdef WIN32
#  ifdef H2CORE_HAVE_DEBUG
	::AllocConsole();
//	::SetConsoleTitle( "Hydrogen debug log" );
	SetConsoleOutputCP( CP_UTF8 );
	freopen( "CONOUT$", "wt", stdout );
#  endif
#endif

	QTextStream stdoutStream( stdout );
	QTextStream stderrStream( stderr );
#ifdef H2CORE_HAVE_QT6
	stdoutStream.setEncoding( QStringConverter::Utf8 );
	stderrStream.setEncoding( QStringConverter::Utf8 );
#else
	stdoutStream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
	stderrStream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
#endif

	bool bUseLogFile = true;
	QFile logFile( pLogger->m_sLogFilePath );
	QTextStream logFileStream = QTextStream();
	QIODevice::OpenMode openMode = QIODevice::WriteOnly | QIODevice::Text;
	if ( pLogger->m_bAppend ) {
		// Append keeps any previous content (a pid-named plugin log can
		// recur after pid reuse) instead of truncating it away.
		openMode |= QIODevice::Append;
	}
	if ( logFile.open( openMode ) ) {
		logFileStream.setDevice( &logFile );
#ifdef H2CORE_HAVE_QT6
		logFileStream.setEncoding( QStringConverter::Utf8 );
#else
		logFileStream.setCodec( QTextCodec::codecForName( "UTF-8" ) );
#endif
	}
	else {
		stderrStream <<
			QString( "Error: can't open log file [%1] for writing...\n" )
			.arg( pLogger->m_sLogFilePath );
		stderrStream.flush();
		bUseLogFile = false;
	}
	Logger::queue_t* queue = &pLogger->__msg_queue;
	// Drain loop. Uses a predicate around pthread_cond_wait() to avoid lost
	// wakeups, and — crucially for short-lived per-instance loggers (ADR 0015,
	// T1.6) — always performs a final drain once __running is cleared, so
	// messages queued just before destruction are flushed before the file is
	// closed (the dtor joins this thread).
	bool bDraining = true;
	while ( bDraining ) {
		pthread_mutex_lock( &pLogger->__mutex );
		while ( pLogger->__running && queue->empty() ) {
			pthread_cond_wait( &pLogger->__messages_available, &pLogger->__mutex );
		}
		// Stop once shutdown has been requested and nothing is left to write.
		bDraining = pLogger->__running || ! queue->empty();
		// Detach the pending messages under the lock, then write them out
		// without holding it.
		Logger::queue_t pending;
		pending.swap( *queue );
		pthread_mutex_unlock( &pLogger->__mutex );

		for ( const auto& sEntry : pending ) {
			if ( pLogger->m_bUseStdout ) {
				stdoutStream << sEntry;
				stdoutStream.flush();
			}
			if ( bUseLogFile ) {
				// The lock keeps concurrent appenders from interleaving
				// inside a line (see LogWriteLock). Qt's Windows append
				// mode is not atomic - unlike O_APPEND it writes at the
				// (stale) per-handle file pointer instead of the file
				// end - so seek to the current end inside the lock
				// before writing, or a concurrent appender's lines get
				// overwritten.
				LogWriteLock lock( logFile, pLogger->m_bAppend );
				if ( pLogger->m_bAppend ) {
					logFile.seek( logFile.size() );
				}
				logFileStream << sEntry;
				logFileStream.flush();
			}
		}
	}
	if ( bUseLogFile ) {
		LogWriteLock lock( logFile, pLogger->m_bAppend );
		if ( pLogger->m_bAppend ) {
			// Same stale-pointer protection as the drain loop above.
			logFile.seek( logFile.size() );
		}
		// Farewell marker for a clean shutdown (a crash truncates the file
		// without it). Formatted inline like a log() line — the drain loop
		// above has exited, so enqueuing through log() would go nowhere —
		// keeping every line in the file correlatable (ADR 0015, T1.6).
		logFileStream << pLogger->timestampPrefix()
					  << "(I) [Logger::~Logger] Stop logger\n";
		logFileStream.flush();
	}
	logFile.close();
#ifdef WIN32
	::FreeConsole();
#endif

	stderrStream.flush();
	stdoutStream.flush();
	pthread_exit( nullptr );
	return nullptr;
}

Logger* Logger::bootstrap( unsigned msk, const QString& sLogFilePath,
						   Options options ) {
	Logger::set_bit_mask( msk );

	// When starting Hydrogen after a fresh install with no user-level .hydrogen
	// folder, opening the log file will fail as the .hydrogen folder as whole
	// does not exist yet. It is created as part of the bootstrap of
	// `Filesystem`.
	QFileInfo logFileInfo;
	if ( ! sLogFilePath.isEmpty() ) {
		logFileInfo = QFileInfo( sLogFilePath );
	}
	else {
		logFileInfo = QFileInfo( Filesystem::logFilePath() );
	}
	const auto dir = logFileInfo.absoluteDir();
	if ( ! dir.exists() ) {
		Filesystem::mkdir( dir.absolutePath() );
	}

	return Logger::create_instance( sLogFilePath, options );
}

Logger* Logger::create_instance( const QString& sLogFilePath,
								 Options options ) {
	if ( __instance == nullptr ) {
		__instance = new Logger( sLogFilePath, options );
	}
	return __instance;
}

Logger* Logger::createInstanceLogger( const QString& sLogFilePath,
									  Options options ) {
	// Standalone, owner-managed logger (own queue/thread/file). Deliberately
	// does NOT touch __instance — the process default stays as the unscoped
	// fallback (ADR 0015, T1.6).
	auto pLogger = new Logger( sLogFilePath, options );
	pLogger->m_bIsInstanceLogger = true;
	__nInstanceLoggers.fetch_add( 1, std::memory_order_relaxed );
	return pLogger;
}

Logger::Logger( const QString& sLogFilePath, Options options )
	: __running( true )
	, m_sLogFilePath( sLogFilePath )
	, m_bUseStdout( options.testFlag( Option::UseStdout ) )
	, m_bLogTimestamps( options.testFlag( Option::Timestamps ) )
	, m_bLogColors( options.testFlag( Option::Colors ) )
	, m_bAppend( options.testFlag( Option::Append ) ) {
	m_prefixList << ""
				 << "(E) "
				 << "(W) "
				 << "(I) "
				 << "(D) "
				 << "(IPC) "
				 << "(C) "
				 << "(L) ";

	if ( !m_bLogColors ) {
		m_colorList << "" << "" << "" << "" << "" << "" << "" << "";
		m_sColorOff = "";
	}
	else {
		m_colorList << ""
					<< "\033[31m"
					<< "\033[36m"
					<< "\033[32m"
					<< "\033[35m"
					<< "\033[33m"
					<< "\033[35;1m"
					<< "\033[35;1m";
		m_sColorOff = "\033[0m";
	}

	// Sanity checks.
	QFileInfo fiLogFile( m_sLogFilePath );
	QFileInfo fiParentFolder( fiLogFile.absolutePath() );
	if ( ( fiLogFile.exists() && ! fiLogFile.isWritable() ) ||
		 ( ! fiLogFile.exists() && ! fiParentFolder.isWritable() ) ) {
		m_sLogFilePath = "";
	}
	
	if ( m_sLogFilePath.isEmpty() ) {
		m_sLogFilePath = Filesystem::logFilePath();
	}

	pthread_attr_t attr;
	pthread_attr_init( &attr );
	pthread_mutex_init( &__mutex, nullptr );
	pthread_cond_init( &__messages_available, nullptr );
	pthread_create( &m_loggerThread, &attr, loggerThread_func, this );

	if ( should_log( Info ) ) {
		log( Info, "Logger", "Logger", QString( "Starting Hydrogen version [%1]" )
			 .arg( QString::fromStdString( get_version() ) ) );
		log( Info, "Logger", "Logger", QString( "Using log file [%1]" )
			 .arg( m_sLogFilePath ) );
	}
}

Logger::~Logger() {
	if ( m_bIsInstanceLogger ) {
		__nInstanceLoggers.fetch_sub( 1, std::memory_order_relaxed );
	}
	__running = false;
	pthread_cond_broadcast ( &__messages_available );
	pthread_join( m_loggerThread, nullptr );
}

QString Logger::timestampPrefix() const {
	if ( ! m_bLogTimestamps ) {
		return QString();
	}
	return QString( "[%1] " )
		.arg( QDateTime::currentDateTime().toString( "hh:mm:ss.zzz" ) );
}

void Logger::log( unsigned level, const QString& sClassName, const char* func_name,
				  const QString& sMsg, const QString& sColor ) {

	if( level == None ){
		return;
	}

	int i;
	switch( level ) {
	case Error:
		i = 1;
		break;
	case Warning:
		i = 2;
		break;
	case Info:
		i = 3;
		break;
	case Debug:
		i = 4;
		break;
	case Ipc:
		i = 5;
		break;
	case Constructors:
		i = 6;
		break;
	case Locks:
		i = 7;
		break;
	default:
		i = 0;
		break;
	}

	const QString sTimestampPrefix = timestampPrefix();

	QString sCol = "";
	if ( m_bLogColors ) {
		sCol = sColor.isEmpty() ? m_colorList[ i ] : sColor;
	}

	// Audit surface (ADR 0015, T1.6): while per-instance loggers are alive, a
	// line reaching the process default did not resolve through an instance
	// scope — mark it so routing gaps stay greppable instead of silently
	// blending into process-level output.
	QString sMsgFull = sMsg;
	if ( this == __instance &&
		 __nInstanceLoggers.load( std::memory_order_relaxed ) > 0 ) {
		sMsgFull.prepend( "[unscoped] " );
	}

	const QString tmp = QString( "%1%2%3[%4::%5] %6%7\n" )
		.arg( sCol ).arg( sTimestampPrefix ).arg( m_prefixList[i] )
		.arg( sClassName ).arg( func_name ).arg( sMsgFull ).arg( m_sColorOff );

	pthread_mutex_lock( &__mutex );
	__msg_queue.push_back( tmp );
	pthread_mutex_unlock( &__mutex );
	pthread_cond_broadcast( &__messages_available );
}

void Logger::flush() const {

	int nTimeout = 100;
	for ( int ii = 0; ii < nTimeout; ++ii ) {
		if ( __msg_queue.empty() ) {
			break;
		}

		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
	}
	return;
}

unsigned Logger::parse_log_level( const char* level ) {
	unsigned log_level = Logger::None;
	if ( 0 == strncasecmp( level, __levels[0], strlen( __levels[0] ) ) ) {
		log_level = Logger::None;
	}
	else if ( 0 == strncasecmp( level, __levels[1], strlen( __levels[1] ) ) ) {
		log_level = Logger::Error;
	}
	else if ( 0 == strncasecmp( level, __levels[2], strlen( __levels[2] ) ) ) {
		log_level = Logger::Error | Logger::Warning;
	}
	else if ( 0 == strncasecmp( level, __levels[3], strlen( __levels[3] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info;
	}
	else if ( 0 == strncasecmp( level, __levels[4], strlen( __levels[4] ) ) ) {
		log_level =
			Logger::Error | Logger::Warning | Logger::Info | Logger::Debug;
	}
	else if ( 0 == strncasecmp( level, __levels[5], strlen( __levels[5] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info |
					Logger::Debug | Logger::Ipc;
	}
	else if ( 0 == strncasecmp( level, __levels[6], strlen( __levels[6] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info |
					Logger::Debug | Logger::Ipc | Logger::Constructors;
	}
	else if ( 0 == strncasecmp( level, __levels[7], strlen( __levels[7] ) ) ) {
		log_level = Logger::Error | Logger::Warning | Logger::Info |
					Logger::Debug | Logger::Ipc | Logger::Locks;
	}
	else {
#ifdef HAVE_SSCANF
		int val = sscanf( level,"%x",&log_level );
		if( val != 1 ) {
			log_level = Logger::Error;
		}
#else
		log_level = hextoi( level, -1 );
		if( log_level==-1 ) {
			log_level = Logger::Error;
		}
#endif
	}
	return log_level;
}

#ifndef HAVE_SSCANF
int Logger::hextoi( const char* str, long len ) {
	long pos = 0;
	char c = 0;
	int v = 0;
	int res = 0;
	bool leading_zero = false;

	while( 1 ) {
		if( ( len!=-1 ) && ( pos>=len ) ) {
			break;
		}
		c = str[pos];
		if( c==0 ) {
			break;
		} else if( c=='x' || c=='X' ) {
			if ( ( pos==1 ) && leading_zero ) {
				assert( res == 0 );
				pos++;
				continue;
			} else {
				return -1;
			}
		} else if( c>='a' ) {
			v = c-'a'+10;
		} else if( c>='A' ) {
			v = c-'A'+10;
		} else if( c>='0' ) {
			if ( ( c=='0' ) && ( pos==0 ) ) {
				leading_zero = true;
			}
			v = c-'0';
		} else {
			return -1;
		}
		if( v>15 ) {
			return -1;
		}
		//assert( v == (v & 0xF) );
		res = ( res << 4 ) | v;
		assert( ( res & 0xF ) == ( v & 0xF ) );
		pos++;
	}
	return res;
}
#endif // HAVE_SSCANF


Logger::CrashContext::CrashContext( QString *pContext ) {
	pSavedContext = Logger::pCrashContext;
	Logger::pCrashContext = pContext;
	pThisContext = nullptr;
}

Logger::CrashContext::CrashContext( QString sContext ) {
	pSavedContext = Logger::pCrashContext;
	// Copy context string
	pThisContext = new QString( sContext );
	Logger::pCrashContext = pThisContext;
}

Logger::CrashContext::~CrashContext() {
	Logger::pCrashContext = pSavedContext;
	if ( pThisContext ) {
		delete pThisContext;
	}
}

void Logger::setCrashContext( QString* pContext )
{
	Logger::pCrashContext = pContext;
}
QString* Logger::getCrashContext()
{
	return Logger::pCrashContext;
}

};	// namespace H2Core

/* vim: set softtabstop=4 noexpandtab: */
