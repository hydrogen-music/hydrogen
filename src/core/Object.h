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

#ifndef H2C_OBJECT_H
#define H2C_OBJECT_H

#include <core/config.h>
#include <core/Globals.h>
#include <core/Helpers/Time.h>
#include <core/Logger.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <QDebug>
#include <QtCore>
#include <unistd.h>

namespace H2Core {

/**
 * Process-unique, immutable instance identity carried by every #Base
 * (ADR 0028).
 *
 * The value is a pair of a per-process @a epoch (seeded once at startup off the
 * real-time path) and a monotonically increasing @a counter (advanced by a
 * wait-free atomic `fetch_add`, hence safe to mint while copying #Note objects
 * on the audio thread). Two ids are equal iff both fields match; the epoch makes
 * an id minted by another process (e.g. the out-of-process editor) never
 * spuriously match a local object. Runtime-only — never serialised.
 */
struct Uuid {
	uint64_t epoch;
	uint64_t counter;

	Uuid( uint64_t nEpoch = 0, uint64_t nCounter = 0 ) :
		epoch( nEpoch ), counter( nCounter ) {}

	/** A default-constructed (never-minted) id. */
	bool isNull() const { return epoch == 0 && counter == 0; }

	bool operator==( const Uuid& other ) const {
		return epoch == other.epoch && counter == other.counter;
	}
	bool operator!=( const Uuid& other ) const { return !( *this == other ); }

	QString toQString() const;
	static Uuid fromQString( const QString& sUuid, bool bSilent = false );
	friend std::ostream& operator<<( std::ostream& out, const Uuid& u ) {
		out << u.toQString().toStdString();
		return out;
	}

	/** Mints a fresh, globally-unique id. Lock-free / real-time-safe. */
	static Uuid mint();
};

inline uint qHash( const Uuid& uuid, uint seed = 0 ) noexcept {
	return ::qHash( uuid.epoch, seed ) ^ ::qHash( uuid.counter, seed );
}

/**
 * The single instance-identity comparator (ADR 0028). Two managed objects are
 * "the same" iff they are the same pointer or carry the same #Uuid. This is the
 * one place identity is decided, so no call site has to reason about whether a
 * raw pointer comparison is safe — and, unlike a bare pointer compare, it stays
 * meaningful once an id is carried across the editor/engine process split.
 *
 * In-process this is exactly `a == b` (every object has a unique id), so routing
 * the existing pointer-identity checks through it is behaviour-preserving.
 */
template <typename T>
inline bool sameObject( const std::shared_ptr<T>& a,
						const std::shared_ptr<T>& b ) {
	if ( a == b ) {
		// Identical managed pointer, including the both-null case.
		return true;
	}
	if ( a == nullptr || b == nullptr ) {
		return false;
	}
	return a->getUuid() == b->getUuid();
}

/** an objects class map item type */
typedef struct atomic_obj_cpt_t {
	std::atomic<int> constructed;
	std::atomic<int> destructed;
	atomic_obj_cpt_t() : constructed(0), destructed(0) {}
} atomic_obj_cpt_t;

typedef struct {
	int constructed;
	int destructed;
} obj_cpt_t;

/** the objects class map types */
typedef std::map<const char*, obj_cpt_t> object_map_t;
typedef std::map<const char*, const atomic_obj_cpt_t*> object_internal_map_t;

/**
 * Base class.
 */
/** \ingroup docCore docDebugging*/
class Base {
	public:
		Base() : m_uuid( Uuid::mint() ) {
#ifdef H2CORE_HAVE_DEBUG
			if ( __count ) {
				++__objects_count;
			}
#endif
		}
		// A copy is a distinct object and therefore mints its own fresh
		// identity (ADR 0028) — "pointer identity, but as a value".
		Base(const Base &other) : m_uuid( Uuid::mint() ) {
#ifdef H2CORE_HAVE_DEBUG
			if ( __count ) {
				++__objects_count;
			}
#endif
		}
		// Assignment transfers data, not identity: an object keeps the id it was
		// born with. #m_uuid is deliberately left untouched.
		Base& operator=( const Base& ) { return *this; }

		/** \return the immutable per-instance identity (ADR 0028). */
		const Uuid& getUuid() const { return m_uuid; }
		/** Only intended to be used for reconstruction of an external objects
		 * during IPC transport. */
		void setUuid( const Uuid& uuid ) { m_uuid = uuid; }
		static const char *_class_name() { return "Object"; }        ///< return the class name
		virtual const char* class_name() const { return _class_name(); }
		/**
		 * enable/disable class instances counting
		 * \param flag the counting status to set
		 */
		static void set_count( bool flag );
		static bool count_active() { return __count; }
		static int objects_count() { return __objects_count; }

		/**
		 * output the full objects map to a given ostream
		 * \param out the ostream to write to
		 * \param map Object map to print out. Per default the current
		 * object map __objects_map will be used.
		 */
		static void write_objects_map_to( std::ostream& out, object_map_t* map = nullptr );
		static void write_objects_map_to_cerr() { Base::write_objects_map_to( std::cerr ); }  ///< output objects map to stderr

		/**
		 * must be called before any Object instantiation !
		 * \param logger the logger instance used to send messages to
		 * \param count should we count objects instances or not
		 */
		static int bootstrap( Logger* logger, bool count=false );
		static Logger* logger()                 { return __logger; }            ///< return the logger instance

	/**
	 * Measures the current time and stores it in #__last_clock. In
	 * case #__last_clock was already set - base_clock() was invoked
	 * at least once - the time difference will be logged with DEBUG
	 * level.
	 */
	static QString base_clock( const QString& sMsg );
	static QString base_clock_in( const QString& sMsg );
	
		/** \return Total numbers of objects being alive. */
		static int getAliveObjectCount();
		/** \return Copy of the object map. */

		static object_map_t getObjectMap();
		/** Creates the difference between a snapshot of the object
		 * map and its current state and prints it to std::cout.
		 *
		 * \param map Object map retrieved using getObjectMap().
		 */
		static void printObjectMapDiff( const object_map_t& map );
		/** String used to format the debugging string output of some
			core classes.*/
		static QString sPrintIndention;

		/** Formatted string version for debugging purposes.
		 * \param sPrefix String prefix which will be added in front of
		 * every new line
		 * \param bShort Instead of the whole content of all classes
		 * stored as members just a single unique identifier will be
		 * displayed without line breaks.
		 *
		 * \return String presentation of current object.*/
		virtual QString toQString( const QString& sPrefix = "", bool bShort = true ) const;
		/** Prints content of toQString() via DEBUGLOG
		 *
		 * \param bShort Whether to display the content of the member
		 * class variables and to use line breaks.
		 */
		void Print( bool bShort = true ) const;

		/** Print the current stack at point into the debug log.*/
		void logBacktrace() const;
	protected:
		~Base() {
#ifdef H2CORE_HAVE_DEBUG
			if ( __count ) {
				--__objects_count;
			}
#endif
		}
		static bool __count;               ///< should we count class instances
		static Logger * __logger;
		static bool bLogColors;
		static void registerClass(const char *name, const atomic_obj_cpt_t *counters);
		static TimePoint m_lastTimePoint;
	
	private:
		/** Immutable per-instance identity, minted at construction (ADR 0028). */
		Uuid m_uuid;
		static std::atomic<int> __objects_count;        ///< total objects count
		static object_internal_map_t __objects_map;      ///< objects classes and instances count structure
		static pthread_mutex_t __mutex;         ///< yeah this has to be thread safe
};

std::ostream& operator<<( std::ostream& os, const Base& object );
std::ostream& operator<<( std::ostream& os, const Base* object );


inline QDebug operator<<( QDebug d, Base *o ) {
	d << ( o ? o->toQString( "", true ) : "(nullptr)" );
	return d;
}

inline QDebug operator<<( QDebug d, std::shared_ptr<Base> o ) {
	d << ( o ? o->toQString( "", true ) : "(nullptr)" );
	return d;
}

template<typename T> class Object: public Base {
	public:
		Object() {
#ifdef H2CORE_HAVE_DEBUG
			if ( __logger && __logger->should_log( Logger::Constructors ) ) {
				__logger->log( Logger::Debug, nullptr, T::_class_name(), "Constructor" );
			}
			if ( __count ) {
				if ( ! counters.constructed ) {
					registerClass(T::_class_name(), &counters);
				}
				++counters.constructed;
			}
#endif
		}
		Object(const Object<T> &other) {
#ifdef H2CORE_HAVE_DEBUG
			if( __logger && __logger->should_log( Logger::Constructors ) ) {
				__logger->log( Logger::Debug, nullptr, T::_class_name(), "Copy Constructor" );
			}
			if ( __count ) {
				if ( ! counters.constructed ) {
					registerClass(T::_class_name(), &counters);
				}
				++counters.constructed;
			}
#endif
		}
	protected:
		~Object() {
#ifdef H2CORE_HAVE_DEBUG
			if( __logger && __logger->should_log( Logger::Constructors ) ) {
				__logger->log( Logger::Debug, nullptr, T::_class_name(), "Destructor" );
			}
			if ( __count ) {
				++counters.destructed;
			}
#endif
		}
	private:
		static atomic_obj_cpt_t counters;
};
template<typename T> atomic_obj_cpt_t Object<T>::counters;


// Object inherited class declaration macro
#define H2_OBJECT(name)                                                 \
	public: static const char* _class_name() { return #name; } \
			const char* class_name() const override { return _class_name(); }

// LOG MACROS
// ADR 0015 / T1.6: log macros resolve the logger through the ambient context
// (Logger::currentLogger()) at log time — the active per-instance Scope, or the
// process default when none is set — instead of the static Base::__logger /
// Logger::get_instance() singleton.
#define __LOG_METHOD(   lvl, msg )  if( H2Core::Logger::currentLogger()->should_log( (lvl) ) ) { H2Core::Logger::currentLogger()->log( (lvl), _class_name(), __FUNCTION__, QString( "%1" ).arg( msg ) ); }
#define __LOG_CLASS(    lvl, msg )  if( H2Core::Logger::currentLogger()->should_log( (lvl) ) ) { H2Core::Logger::currentLogger()->log( (lvl), _class_name(), __FUNCTION__, QString( "%1" ).arg( msg ) ); }
#define __LOG_OBJ(      lvl, msg )  if( __object->logger()->should_log( (lvl) ) )       { __object->logger()->log( (lvl), 0, __PRETTY_FUNCTION__, QString( "%1" ).arg( msg ) ); }
#define __LOG_STATIC(   lvl, msg )  if( H2Core::Logger::currentLogger()->should_log( (lvl) ) ) { H2Core::Logger::currentLogger()->log( (lvl), 0, __PRETTY_FUNCTION__, QString( "%1" ).arg( msg ) ); }
#define __LOG( logger,  lvl, msg )  if( (logger)->should_log( (lvl) ) )                 { (logger)->log( (lvl), 0, 0, QString( "%1" ).arg( msg ) ); }

// Object instance method logging macros
#define IPCLOG(x)       __LOG_METHOD( H2Core::Logger::Ipc,     (x) );
#define DEBUGLOG(x)     __LOG_METHOD( H2Core::Logger::Debug,   (x) );
#define INFOLOG(x)      __LOG_METHOD( H2Core::Logger::Info,    (x) );
#define WARNINGLOG(x)   __LOG_METHOD( H2Core::Logger::Warning, (x) );
#define ERRORLOG(x)     __LOG_METHOD( H2Core::Logger::Error,   (x) );

// Object class method logging macros
#define _IPCLOG(x)      __LOG_CLASS( H2Core::Logger::Ipc,     (x) );
#define _DEBUGLOG(x)    __LOG_CLASS( H2Core::Logger::Debug,   (x) );
#define _INFOLOG(x)     __LOG_CLASS( H2Core::Logger::Info,    (x) );
#define _WARNINGLOG(x)  __LOG_CLASS( H2Core::Logger::Warning, (x) );
#define _ERRORLOG(x)    __LOG_CLASS( H2Core::Logger::Error,   (x) );

// logging macros using an Base *__object ( thread :  Base * __object = ( Base * )param; )
#define __IPCLOG(x)     __LOG_OBJ( H2Core::Logger::Ipc,        (x) );
#define __DEBUGLOG(x)   __LOG_OBJ( H2Core::Logger::Debug,      (x) );
#define __INFOLOG(x)    __LOG_OBJ( H2Core::Logger::Info,       (x) );
#define __WARNINGLOG(x) __LOG_OBJ( H2Core::Logger::Warning,    (x) );
#define __ERRORLOG(x)   __LOG_OBJ( H2Core::Logger::Error,      (x) );

// logging macros using  ( thread :  Base * __object = ( Base * )param; )
#define ___IPCLOG(x)     __LOG_STATIC( H2Core::Logger::Ipc,      (x) );
#define ___DEBUGLOG(x)   __LOG_STATIC( H2Core::Logger::Debug,    (x) );
#define ___INFOLOG(x)    __LOG_STATIC( H2Core::Logger::Info,     (x) );
#define ___WARNINGLOG(x) __LOG_STATIC(H2Core::Logger::Warning,   (x) );
#define ___ERRORLOG(x)   __LOG_STATIC( H2Core::Logger::Error,    (x) );

// Can be called without or with a single argument
#define CLOCK(...)      __LOG_METHOD( H2Core::Logger::Debug, base_clock( QString( "%1" ).arg( #__VA_ARGS__ ) ) );
#define CLOCKIN(...)    __LOG_METHOD( H2Core::Logger::Debug, base_clock_in( QString( "%1" ).arg( #__VA_ARGS__ ) ) );

};

#endif // H2C_OBJECT_H

/* vim: set softtabstop=4 noexpandtab: */
