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

#ifndef H2C_LOGGER_H
#define H2C_LOGGER_H

#include <cassert>
#include <list>
#include <pthread.h>
#include <memory>
#include <QtCore/QString>
#include <QStringList>

#include <core/config.h>

namespace H2Core {

/**
 * Class for writing logs to the console
 */
/** \ingroup docCore docDebugging*/
class Logger {
	public:
		/** possible logging bits */
		enum log_levels {
			None            = 0x00,
			Error           = 0x01,
			Warning         = 0x02,
			Info            = 0x04,
			Debug           = 0x08,
			Constructors    = 0x10,
			Locks   = 0x20
		};

		/** message queue type */
		typedef std::list<QString> queue_t;

		/**
		 * create the logger instance if not exists, set the log level and return the instance
		 * \param msk the logging level bitmask
		 */
	static Logger* bootstrap( unsigned msk,
							  const QString& sLogFilePath = QString(),
							  bool bUseStdout = true,
							  bool bLogTimestamps = false,
							  bool bLogColors = true );
		/**
		 * If #__instance equals 0, a new H2Core::Logger
		 * singleton will be created and stored in it.
		 *
		 * It is called in Hydrogen::create_instance().
		 */
	static Logger* create_instance( const QString& sLogFilePath = QString(),
									bool bUseStdout = true,
									bool bLogTimestamps = false,
									bool bLogColors = true );

		/**
		 * Creates a standalone, per-instance #Logger (ADR 0015, T1.6) owned by
		 * the caller (e.g. a #Hydrogen instance) — its own queue, worker thread
		 * and log file. Unlike #create_instance() this does **not** touch the
		 * process-default #__instance, so multiple instances can coexist. */
		static std::shared_ptr<Logger> createInstanceLogger(
			const QString& sLogFilePath,
			bool bUseStdout = true,
			bool bLogTimestamps = false,
			bool bLogColors = true );

		/**
		 * Returns a pointer to the current H2Core::Logger
		 * singleton stored in #__instance.
		 */
		static Logger* get_instance(){ assert(__instance); return __instance; }

		/** destructor */
		~Logger();

		/** Checks whether the Logger instances was already created and can be
		 * used by other parts of Hydrogen.*/
		static bool isAvailable() {
			return __instance == nullptr ? false : true;
		}

		/**
		 * Ambient-context logger resolution (ADR 0015, T1.6).
		 *
		 * Returns the #Logger of the innermost active #Scope on the current
		 * thread, or — when no scope is active (static/early-startup/unscoped
		 * contexts) — the process-default logger (#__instance, set at
		 * bootstrap). The log macros resolve through this **at log time**, so a
		 * wrong or absent context can only misroute a log line to the default,
		 * never change behaviour. */
		static Logger* currentLogger() {
			return __pCurrent != nullptr ? __pCurrent : __instance;
		}

		/**
		 * RAII guard pushing @a pLogger as the current-thread logger for the
		 * lifetime of the scope (restoring the previous one on destruction).
		 * Set at the per-instance entry points (audio process callback, command
		 * dispatch, song/kit load, per-instance workers). */
		class Scope {
		public:
			explicit Scope( Logger* pLogger )
				: m_pPrevious( Logger::__pCurrent ) {
				Logger::__pCurrent = pLogger;
			}
			~Scope() { Logger::__pCurrent = m_pPrevious; }
			Scope( const Scope& ) = delete;
			Scope& operator=( const Scope& ) = delete;
		private:
			Logger* m_pPrevious;
		};

		/**
		 * return true if the level is set in the bitmask
		 * \param lvl the level to check
		 */
		bool should_log( unsigned lvl ) const       { return (lvl&__bit_msk); }
		/**
		 * set the bitmask
		 * \param msk the new bitmask to set
		 */
		static void set_bit_mask( unsigned msk )    { __bit_msk = msk; }
		/** return the current log level bit mask */
		static unsigned bit_mask()                  { return __bit_msk; }

	/**
	 * Waits till the logger thread poped all remaining messages from
	 * #__msg_queue.
	 *
	 * Note that this function will neither lock #__msg_queue nor
	 * prevent routines from adding new messages to the queue.
	 */
	void flush() const;

		/**
		 * parse a log level string and return the corresponding bit mask
		 * \param lvl the log level string
		 */
		static unsigned parse_log_level( const char* lvl );

		/**
		 * the log function
		 * \param level used to output the corresponding level string
		 * \param sClassName the name of the calling class
		 * \param func_name the name of the calling function/method
		 * \param sMsg the message to log
		 * \param sColor alternate color
		 */
		void log( unsigned level, const QString& sClassName,
				  const char* func_name, const QString& sMsg,
				  const QString& sColor = "" );
		/**
		 * needed for being able to access logger internal
		 * \param param is a pointer to the logger instance
		 */
		friend void* loggerThread_func( void* param );

		/** @name Crash context management
		 * Access the crash context string that can be used to report what caused a crash.  The crash-context
		 * string is a thread-local property, and may be read by a fatal exception handler (which will execute
		 * in the same thread that caused the crash). This avoids potential contention for locking and
		 * unlocking of a shared crash context structure.
		 * @{
		 */
		static void setCrashContext( QString *pContext );
		static QString *getCrashContext();
		/** @} */

		bool getLogColors() const;
		/** Whether this logger also writes to stdout. Used so a per-instance
		 * logger can mirror the process default's verbosity (ADR 0015, T1.6). */
		bool getUseStdout() const { return m_bUseStdout; }

		/** Helper class to preserve and restore recursive crash context strings using an RAAI pattern */
		class CrashContext {
			QString *pSavedContext;
			QString *pThisContext;
		public:
			CrashContext( QString *pContext );
			CrashContext( QString sContext );
			~CrashContext();
		};

	private:
		/**
		 * Object holding the current H2Core::Logger
		 * singleton. It is initialized with NULL, set with
		 * create_instance(), and accessed with
		 * get_instance().
		 */
		static Logger* __instance;
		/** Thread-local current-context logger for #currentLogger()/#Scope
		 * (ADR 0015, T1.6). nullptr ⇒ fall back to the process default. */
		static thread_local Logger* __pCurrent;
		bool __running;                 ///< set to true when the logger thread is running
		pthread_mutex_t __mutex;        ///< lock for adding or removing elements only
		queue_t __msg_queue;            ///< the message queue
		static unsigned __bit_msk;      ///< the bitmask of log_level_t
		static const char* __levels[];  ///< levels strings
		pthread_cond_t __messages_available;
		/** Worker thread servicing this logger's queue. Per-instance (ADR 0015,
		 * T1.6) so multiple loggers each drain their own queue/file. */
		pthread_t m_loggerThread;
	QString m_sLogFilePath;

		QStringList m_prefixList;
		QStringList m_colorList;
		QString m_sColorOff;

	bool m_bUseStdout;
		bool m_bLogTimestamps;
		bool m_bLogColors;

		thread_local static QString *pCrashContext;

		/** constructor */
	Logger( const QString& sLogFilePath = QString(), bool bUseStdout = true,
			bool bLogTimestamps = false, bool bLogColors = true );

#ifndef HAVE_SSCANF
		/**
		 * convert an hex string to an integer.
		 * returns -1 on failure.
		 * \param str the hex string to convert
		 * \param len the length of the string
		 */
		static int hextoi( const char* str, long len );
#endif // HAVE_SSCANF
};

inline bool Logger::getLogColors() const {
	return m_bLogColors;
}

};

#endif // H2C_LOGGER_H

/* vim: set softtabstop=4 noexpandtab: */
