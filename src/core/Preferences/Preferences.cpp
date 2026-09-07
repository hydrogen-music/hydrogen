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

#include "Preferences.h"
#include "Helpers/Filesystem.h"
#include "Midi/Midi.h"
#include "Sampler/Interpolation.h"

#ifndef WIN32
#include <pwd.h>
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLockFile>
#include <QtCore/QMutexLocker>

#include <core/Basics/InstrumentComponent.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/IO/AlsaAudioDriver.h>
#include <core/Midi/MidiEventMap.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Midi/MidiMessage.h>
#include <core/Preferences/PreferencesKeys.h>
#include <core/Preferences/PreferencesSchema.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
#include <core/Version.h>

#include <QDir>
#include <QProcess>

namespace H2Core {

std::shared_ptr<Preferences> Preferences::create_instance()
{
	// Loads and returns a freshly-owned Preferences. No process-wide singleton
	// is involved: the caller (Hydrogen, or the bootstrap in main) owns the
	// result (ADR 0015). At this early bootstrap point no Hydrogen instance
	// exists yet; the instance is only consulted by Shortcuts to decide whether
	// the GUI is ready, and it is not at config-load time.
	auto pPrefUser = load( Filesystem::userConfigPath(), false, nullptr );
	if ( pPrefUser != nullptr ) {
		pPrefUser->m_bLoadingSuccessful = true;
		return pPrefUser;
	}

	// Fallback to system-level configs (the one we ship)
	auto pPrefSystem = load( Filesystem::systemConfigPath(), false, nullptr );
	if ( pPrefSystem != nullptr ) {
		INFOLOG( QString( "Couldn't load user-level configuration from "
						  "[%1]. Falling back to system-level one in [%2]" )
					 .arg( Filesystem::userConfigPath() )
					 .arg( Filesystem::systemConfigPath() ) );
		pPrefSystem->m_bLoadingSuccessful = true;
		return pPrefSystem;
	}

	ERRORLOG( QString( "Couldn't load config file from neither [%1] nor [%2]." )
				  .arg( Filesystem::userConfigPath() )
				  .arg( Filesystem::systemConfigPath() ) );
	auto pPref = std::make_shared<Preferences>();
	pPref->m_bLoadingSuccessful = false;
	return pPref;
}

Preferences::Preferences()
	: m_bSearchForRubberbandOnLoad( true ),
	  m_nPunchInPos( 0 ),
	  m_nPunchOutPos( -1 ),
	  m_bLoadingSuccessful( false )
{
	// The PreferencesData defaults cover the former initialization list;
	// this body only applies the environment-dependent defaults.
	m_onlineRepos << "https://raw.githubusercontent.com/hydrogen-music/"
					 "Song-and-pattern-repository/refs/heads/main/index.json"
				  << "http://hydrogen-music.org/feeds/index.json";

	//___ MIDI Driver properties
#if defined( H2CORE_HAVE_ALSA )
	m_midiDriver = MidiDriver::Alsa;
#elif defined( H2CORE_HAVE_PORTMIDI )
	m_midiDriver = MidiDriver::PortMidi;
#elif defined( H2CORE_HAVE_COREMIDI )
	m_midiDriver = MidiDriver::CoreMidi;
#elif defined( H2CORE_HAVE_JACK )
	m_midiDriver = MidiDriver::Jack;
#else
	// Set ALSA as fallback if none of the above options are available
	// (although MIDI won't work in this case).
	m_midiDriver = MidiDriver::Alsa;
#endif

	//___  alsa audio driver properties ___
#ifdef H2CORE_HAVE_ALSA
	// Ensure the device read from the local preferences does
	// exist. If not, we try to replace it with a valid one.
	QStringList alsaDevices = AlsaAudioDriver::getAlsaDevices();
	if ( alsaDevices.size() == 0 || alsaDevices.contains( "hw:0" ) ) {
		m_sAlsaAudioDevice = "hw:0";
	}
	else {
		// Fall back to a device found on the system (but not the
		// "null" one).
		if ( alsaDevices[0] != "null" ) {
			m_sAlsaAudioDevice = alsaDevices[0];
		}
		else if ( alsaDevices.size() > 1 ) {
			m_sAlsaAudioDevice = alsaDevices[1];
		}
		else {
			m_sAlsaAudioDevice = "hw:0";
		}
	}
#else
	m_sAlsaAudioDevice = "hw:0";
#endif

	// Find the Rubberband-CLI in system env. If this fails a second test will
	// check individual user settings
	const QStringList commonPaths = QString( getenv( "PATH" ) ).split( ":" );
	for ( const auto& ssPath : commonPaths ) {
		m_sRubberBandCLIexecutable = ssPath + "/rubberband";
		if ( QFile( m_sRubberBandCLIexecutable ).exists() ) {
			m_bSearchForRubberbandOnLoad = false;
			break;
		}
	}
	if ( m_bSearchForRubberbandOnLoad ) {
		// No binary found
		m_sRubberBandCLIexecutable = "Path to Rubberband-CLI";
	}
}

Preferences::Preferences( std::shared_ptr<Preferences> pOther )
	: PreferencesData( *pOther ),
	  m_bSearchForRubberbandOnLoad( pOther->m_bSearchForRubberbandOnLoad ),
	  m_nPunchInPos( pOther->m_nPunchInPos ),
	  m_nPunchOutPos( pOther->m_nPunchOutPos ),
	  m_bLoadingSuccessful( pOther->m_bLoadingSuccessful )
{
	// The theme, shortcuts, and MIDI maps are mutable shared state: keep
	// the copy deep so edits in one instance do not leak into the other.
	m_pTheme = std::make_shared<Theme>( pOther->m_pTheme );
	m_pShortcuts = std::make_shared<Shortcuts>( pOther->m_pShortcuts );
	m_pMidiEventMap = std::make_shared<MidiEventMap>( pOther->m_pMidiEventMap );
	m_pMidiInstrumentMap =
		std::make_shared<MidiInstrumentMap>( pOther->m_pMidiInstrumentMap );
	m_baselineXml = pOther->m_baselineXml;
	// The copy carries identical values, so it inherits the source's
	// write-through clean point exactly.
	m_sPersistedFootprint = pOther->m_sPersistedFootprint;
}

Preferences::~Preferences()
{
}

bool Preferences::hasPendingChanges() const {
	QMutexLocker mx( &m_persistedFootprintMutex );
	return computeFootprint() != m_sPersistedFootprint;
}

QString Preferences::computeFootprint() const {
	PreferencesSchema::WriteContext context;
	context.bSilent = true;
	return PreferencesSchema::currentFootprint( *this, m_fieldOwnership,
												context );
}

void Preferences::refreshPersistedFootprint() const {
	QMutexLocker mx( &m_persistedFootprintMutex );
	m_sPersistedFootprint = computeFootprint();
}

void Preferences::setFieldOwnership( FieldOwnership ownership ) {
	m_fieldOwnership = ownership;
	// The pending check is ownership-masked: re-base the clean point so
	// the mask switch itself does not register as pending.
	refreshPersistedFootprint();
}

std::shared_ptr<Preferences> Preferences::load(
	const QString& sPath,
	const bool bSilent,
	Hydrogen* pHydrogen
)
{
	if ( !Filesystem::fileReadable( sPath, bSilent ) ) {
		return nullptr;
	}

	XMLDoc doc;
	doc.read( sPath, false );
	const XMLNode rootNode = doc.firstChildElement( PreferencesKeys::Root );
	if ( rootNode.isNull() ) {
		ERRORLOG( QString( "Preferences file [%1] ill-formatted. "
						   "<hydrogen_preferences> node not found." )
					  .arg( sPath ) );
		return nullptr;
	}

	if ( !bSilent ) {
		INFOLOG( QString( "Loading preferences from [%1]" ).arg( sPath ) );
	}

	auto pPref = std::make_shared<Preferences>();

	// Retain the on-disk XML as the baseline for concurrency-safe persistence
	// (ADR 0023): save() diffs current-vs-baseline to write only this
	// instance's own changes back to the shared user config.
	pPref->m_baselineXml = doc.toByteArray();

	// Pre-2.0 MIDI input mapping flags. They are not schema rows (replaced by
	// the midiInstrumentMap in 2.0) but the midiInstrumentMap row derives the
	// legacy mapping state from them when its element is missing.
	const bool bPlaySelectedInstrument =
		rootNode.read_bool( "instrumentInputMode", false, true, false, true );
	bool bMidiDiscardNoteAfterAction = false;
	bool bAsOutput = false;
	const XMLNode audioEngineNode =
		rootNode.firstChildElement( PreferencesKeys::AudioEngine );
	if ( !audioEngineNode.isNull() ) {
		const XMLNode midiDriverNode =
			audioEngineNode.firstChildElement( PreferencesKeys::MidiDriver );
		if ( !midiDriverNode.isNull() ) {
			// Used in versions prior to 2.0 to indicate that only MIDI action
			// should be triggered by incoming MIDI messages but no realtime
			// notes.
			bMidiDiscardNoteAfterAction = midiDriverNode.read_bool(
				"discard_note_after_action", false, true, false, true
			);
			// Kept for backward compatibility of MIDI input mapping to
			// versions prior to 2.0.
			bAsOutput = midiDriverNode.read_bool(
				"fixed_mapping", false, true, true, true
			);
		}
	}

	PreferencesSchema::ReadContext context;
	context.pHydrogen = pHydrogen;
	context.bSilent = bSilent;
	context.bSearchForRubberband = pPref->m_bSearchForRubberbandOnLoad;
	context.bEmitStructuralWarnings = true;
	context.bLegacyFixedMapping = bAsOutput;
	context.bLegacyDiscardNoteAfterAction = bMidiDiscardNoteAfterAction;
	context.bLegacyPlaySelectedInstrument = bPlaySelectedInstrument;
	context.bApplyLegacyMidiInput = true;

	PreferencesSchema::readRows( rootNode, *pPref, std::nullopt, context );

	// Surface config drift (typos, stale or foreign elements) instead of
	// silently dropping it (ADR 0023 amendment).
	PreferencesSchema::checkForUnknownElements( rootNode );

	// The write-through clean point: the post-load state, migrations
	// included, so a freshly loaded instance starts out clean (ADR 0023).
	pPref->refreshPersistedFootprint();

	return pPref;
}

bool Preferences::saveCopyAs( const QString& sPath, const bool bSilent ) const
{
	if ( !bSilent ) {
		INFOLOG( QString( "Saving preferences file into [%1]" ).arg( sPath ) );
	}

	XMLDoc doc;
	XMLNode rootNode = doc.set_root( PreferencesKeys::Root );

	// hydrogen version
	rootNode.write_int( "formatVersion", nCurrentFormatVersion );
	rootNode.write_string( "version", QString( get_version().c_str() ) );

	PreferencesSchema::WriteContext context;
	context.bSilent = bSilent;
	PreferencesSchema::writeRows( rootNode, *this, std::nullopt, context );

	return doc.write( sPath );
}

bool Preferences::save( const bool bSilent ) const
{
	const QString sPath = Filesystem::userConfigPath();
	if ( !bSilent ) {
		INFOLOG( QString( "Saving preferences file into [%1]" ).arg( sPath ) );
	}

	// The lock file cannot be created in a nonexistent directory (fresh
	// install); create the config directory first.
	const QString sConfigDir = QFileInfo( sPath ).absoluteDir().absolutePath();
	if ( ! Filesystem::dirExists( sConfigDir, true ) ) {
		Filesystem::mkdir( sConfigDir );
	}

	// Cross-process lock around the read-merge-write cycle (ADR 0023).
	// Bounded retry: a single attempt could silently drop this save, while
	// an unbounded block could freeze the GUI on a stuck lock holder.
	QLockFile lock( sPath + ".lock" );
	lock.setStaleLockTime( 30000 );
	bool bLocked = false;
	for ( int ii = 0; ii < 3 && ! bLocked; ++ii ) {
		bLocked = lock.tryLock( 1000 );
	}
	if ( ! bLocked ) {
		ERRORLOG( QString( "Unable to lock [%1] within the retry budget - "
						   "another process holds the lock or it cannot be "
						   "created; save aborted" )
					  .arg( sPath ) );
		return false;
	}

	// Re-read the shared config: everything on disk this instance did not
	// change must survive the field-level merge (ADR 0023).
	QByteArray diskBytes;
	{
		QFile file( sPath );
		if ( file.open( QIODevice::ReadOnly ) ) {
			diskBytes = file.readAll();
			file.close();
		}
	}

	XMLDoc diskDoc;
	const bool bParsed =
		! diskBytes.isEmpty() && diskDoc.setContent( diskBytes );
	const bool bDiskUsable =
		bParsed && diskDoc.documentElement().tagName() == PreferencesKeys::Root;
	if ( ! diskBytes.isEmpty() && ! bParsed ) {
		// Corrupt (a crashed writer left garbage behind): never merge onto
		// or echo the broken document - self-heal by writing a full
		// ownership-masked snapshot (ADR 0023).
		ERRORLOG( QString( "Preferences file [%1] is corrupt; rewriting it "
						   "from the current state" )
					  .arg( sPath ) );
	}
	else if ( bParsed && ! bDiskUsable ) {
		// Not a hydrogen config at all (e.g. a foreign file ended up in the
		// path): never merge onto it.
		ERRORLOG( QString( "Preferences file [%1] has an unexpected root "
						   "element; rewriting it from the current state" )
					  .arg( sPath ) );
	}

	XMLDoc doc;
	// The XML declaration is appended by set_root() for fresh documents;
	// the merged document needs it explicitly (importNode only brings the
	// root element).
	doc.appendChild( doc.createProcessingInstruction(
		"xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
	XMLNode rootNode;
	if ( bDiskUsable ) {
		rootNode = doc.importNode( diskDoc.documentElement(), true );
		doc.appendChild( rootNode );
	}
	else {
		rootNode = doc.createElement( PreferencesKeys::Root );
		doc.appendChild( rootNode );
	}
	// Refresh the file metadata in place: write_* appends, which would move
	// the elements to the end of the document.
	const auto updateHeader = [ &doc ]( XMLNode& root, const char* sName,
									   const QString& sValue ) {
		QDomElement element = root.firstChildElement( sName );
		if ( element.isNull() ) {
			root.write_string( sName, sValue );
			return;
		}
		while ( ! element.lastChild().isNull() ) {
			element.removeChild( element.lastChild() );
		}
		element.appendChild( doc.createTextNode( sValue ) );
	};
	updateHeader( rootNode, "formatVersion",
				  QString::number( nCurrentFormatVersion ) );
	updateHeader( rootNode, "version", QString( get_version().c_str() ) );

	// The load baseline; empty for a never-loaded instance, which then
	// writes a full snapshot.
	XMLDoc baselineDoc;
	const bool bHaveBaseline =
		! m_baselineXml.isEmpty() && baselineDoc.setContent( m_baselineXml );
	if ( ! m_baselineXml.isEmpty() && ! bHaveBaseline ) {
		// Cannot happen for a baseline that passed load(); stay safe and
		// fall back to a full write.
		WARNINGLOG( "Unparseable load baseline; writing a full snapshot" );
	}
	// A usable disk document is merged incrementally against the baseline.
	// Without one (never loaded, missing or corrupt file) the save is a
	// full ownership-masked snapshot: a partial write would produce an
	// incomplete config (ADR 0023 self-heal).
	XMLNode baselineRoot;
	if ( bDiskUsable && bHaveBaseline ) {
		baselineRoot = baselineDoc.documentElement();
	}

	PreferencesSchema::WriteContext context;
	context.bSilent = bSilent;
	PreferencesSchema::persistRows( rootNode, baselineRoot, *this,
									m_fieldOwnership, context );

	if ( ! doc.write( sPath ) ) {
		return false;
	}
	// The load baseline is deliberately NOT refreshed with the merged
	// document: it must keep representing the values *this instance*
	// loaded, so fields other processes changed in the meantime never
	// register as this instance's own changes (and get clobbered). Rows
	// this instance changed are re-written on every save - idempotent and
	// last-writer-wins per field (ADR 0023).
	//
	// The write-through clean point, in contrast, follows the persisted
	// state: explicit saves (dialog OK, OSC, teardown) and write-through
	// saves alike clear the pending flag. A mutation landing between the
	// persist above and this refresh is absorbed as clean without being
	// written - a millisecond window whose consequence is bounded by the
	// next mutation or the teardown save (same family as the deferred
	// in-process save serialization).
	refreshPersistedFootprint();

	return true;
}

Preferences::AudioDriver Preferences::parseAudioDriver( const QString& sDriver )
{
	const QString s = QString( sDriver ).toLower();
	if ( s == "auto" ) {
		return AudioDriver::Auto;
	}
	else if ( s == "jack" || s == "jackaudio" ) {
		return AudioDriver::Jack;
	}
	else if ( s == "oss" ) {
		return AudioDriver::Oss;
	}
	else if ( s == "alsa" ) {
		return AudioDriver::Alsa;
	}
	else if ( s == "pulseaudio" || s == "pulse" ) {
		return AudioDriver::PulseAudio;
	}
	else if ( s == "coreaudio" || s == "core" ) {
		return AudioDriver::CoreAudio;
	}
	else if ( s == "portaudio" || s == "port" ) {
		return AudioDriver::PortAudio;
	}
	else if ( s == "plugin" ) {
		return AudioDriver::Plugin;
	}
	else if ( s == "fake" ) {
		return AudioDriver::Fake;
	}
	else if ( s == "null" ) {
		return AudioDriver::Null;
	}
	else {
		if ( Logger::isAvailable() ) {
			ERRORLOG( QString( "Unable to parse driver [%1]" ).arg( sDriver ) );
		}
		return AudioDriver::None;
	}
}

QString Preferences::audioDriverToQString(
	const Preferences::AudioDriver& driver
)
{
	switch ( driver ) {
		case AudioDriver::Auto:
			return "Auto";
		case AudioDriver::Jack:
			return "JACK";
		case AudioDriver::Oss:
			return "OSS";
		case AudioDriver::Alsa:
			return "ALSA";
		case AudioDriver::PulseAudio:
			return "PulseAudio";
		case AudioDriver::CoreAudio:
			return "CoreAudio";
		case AudioDriver::PortAudio:
			return "PortAudio";
		case AudioDriver::Disk:
			return "Disk";
		case AudioDriver::Fake:
			return "Fake";
		case AudioDriver::Plugin:
			return "Plugin";
		case AudioDriver::Null:
			return "Null";
		case AudioDriver::None:
			return "nullptr";
		default:
			return "Unhandled driver type";
	}
}

Preferences::MidiDriver Preferences::parseMidiDriver( const QString& sDriver )
{
	const QString s = QString( sDriver ).toLower();
	// Ensure compatibility with older versions of the files after
	// capitalization in the GUI (2021-02-05).
	if ( s == "jackmidi" || s == "jack-midi" ) {
		return MidiDriver::Jack;
	}
	else if ( s == "alsa" ) {
		return MidiDriver::Alsa;
	}
	else if ( s == "portmidi" ) {
		return MidiDriver::PortMidi;
	}
	else if ( s == "coremidi" ) {
		return MidiDriver::CoreMidi;
	}
	else if ( s == "plugin" ) {
		return MidiDriver::Plugin;
	}
	else {
		// The LoopBack driver is only used in unit tests. Thus, it should not
		// be written to disk and does not have to be parsed.
		if ( Logger::isAvailable() ) {
			ERRORLOG( QString( "Unable to parse driver [%1]" ).arg( sDriver ) );
		}
		return MidiDriver::None;
	}
}

QString Preferences::midiDriverToQString( const Preferences::MidiDriver& driver
)
{
	switch ( driver ) {
		case MidiDriver::Alsa:
			return "ALSA";
		case MidiDriver::CoreMidi:
			return "CoreMIDI";
		case MidiDriver::Jack:
			return "JACK-MIDI";
		case MidiDriver::None:
			return "nullptr";
		case MidiDriver::PortMidi:
			return "PortMidi";
		case MidiDriver::LoopBack:
			return "LoopBack";
		case MidiDriver::Plugin:
			return "Plugin";
		default:
			return "Unhandled driver type";
	}
}

bool Preferences::checkJackSupport()
{
	// Check whether the Logger is already available.
	const bool bUseLogger = Logger::isAvailable();

#ifndef H2CORE_HAVE_JACK
	if ( bUseLogger ) {
		INFOLOG( "Hydrogen was compiled without JACK support." );
	}
	return false;
#else
#ifndef H2CORE_HAVE_DYNAMIC_JACK_CHECK
	if ( bUseLogger ) {
		INFOLOG( "JACK support enabled." );
	}
	return true;
#else
	/**
	 * Calls @a sExecutable in a subprocess using the @a sOption CLI
	 * option and reports the results.
	 *
	 * @return An empty string indicates, that the call exited with a
	 *   code other than zero.
	 */
	auto checkExecutable = [&]( const QString& sExecutable,
								const QString& sOption ) {
		QProcess process;
		process.start( sExecutable, QStringList( sOption ) );
		process.waitForFinished( -1 );

		if ( process.exitCode() != 0 ) {
			return QString( "" );
		}

		QString sStdout = process.readAllStandardOutput();
		if ( sStdout.isEmpty() ) {
			return QString( "No output" );
		}

		return QString( sStdout.trimmed() );
	};

	bool bJackSupport = false;

	// Classic JACK
	QString sCapture = checkExecutable( "jackd", "--version" );
	if ( !sCapture.isEmpty() ) {
		bJackSupport = true;
		if ( bUseLogger ) {
			INFOLOG( QString( "'jackd' of version [%1] found." ).arg( sCapture )
			);
		}
	}

	// JACK compiled with DBus support (maybe this one is packaged but
	// the classical one isn't).
	//
	// `jackdbus` is supposed to be run by the DBus message daemon and
	// does not have proper CLI options. But it does not fail by
	// passing a `-h` either and this will serve for checking its
	// presence.
	sCapture = checkExecutable( "jackdbus", "-h" );
	if ( !sCapture.isEmpty() ) {
		bJackSupport = true;
		if ( bUseLogger ) {
			INFOLOG( "'jackdbus' found." );
		}
	}

	// Pipewire JACK interface
	//
	// `pw-jack` has no version query CLI option (yet). But showing
	// the help will serve for checking its presence.
	sCapture = checkExecutable( "pw-jack", "-h" );
	if ( !sCapture.isEmpty() ) {
		bJackSupport = true;
		if ( bUseLogger ) {
			INFOLOG( "'pw-jack' found." );
		}
	}

	if ( bUseLogger ) {
		if ( bJackSupport ) {
			INFOLOG( "Dynamic JACK discovery succeeded. JACK support enabled."
			);
		}
		else {
			WARNINGLOG( "Dynamic JACK discovery failed. JACK support disabled."
			);
		}
	}

	return bJackSupport;
#endif
#endif
}

std::vector<Preferences::AudioDriver> Preferences::getSupportedAudioDrivers()
{
	std::vector<AudioDriver> drivers;

	// We always do a fresh check. Maybe dynamical discovery will yield a
	// different result this time.
	bool bJackSupported = checkJackSupport();

	// The order of the assigned drivers is important as Hydrogen uses
	// it when trying different drivers in case "Auto" was selected.
#if defined( WIN32 )
#ifdef H2CORE_HAVE_PORTAUDIO
	drivers.push_back( AudioDriver::PortAudio );
#endif
	if ( bJackSupported ) {
		drivers.push_back( AudioDriver::Jack );
	}
#elif defined( __APPLE__ )
#ifdef H2CORE_HAVE_COREAUDIO
	drivers.push_back( AudioDriver::CoreAudio );
#endif
	if ( bJackSupported ) {
		drivers.push_back( AudioDriver::Jack );
	}
#ifdef H2CORE_HAVE_PULSEAUDIO
	drivers.push_back( AudioDriver::PulseAudio );
#endif
#ifdef H2CORE_HAVE_PORTAUDIO
	drivers.push_back( AudioDriver::PortAudio );
#endif
#else /* Linux */
	if ( bJackSupported ) {
		drivers.push_back( AudioDriver::Jack );
	}
#ifdef H2CORE_HAVE_PULSEAUDIO
	drivers.push_back( AudioDriver::PulseAudio );
#endif
#ifdef H2CORE_HAVE_ALSA
	drivers.push_back( AudioDriver::Alsa );
#endif
#ifdef H2CORE_HAVE_OSS
	drivers.push_back( AudioDriver::Oss );
#endif
#ifdef H2CORE_HAVE_PORTAUDIO
	drivers.push_back( AudioDriver::PortAudio );
#endif
#endif

	return drivers;
}

QString Preferences::toQString( const QString& sPrefix, bool bShort ) const
{
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( !bShort ) {
		sOutput =
			QString( "%1[Preferences]\n" )
				.arg( sPrefix )
				.append( QString( "%1%2m_bPlaySamplesOnClicking: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bPlaySamplesOnClicking ) )
				.append( QString( "%1%2m_bFollowPlayhead: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bFollowPlayhead ) )
				.append( QString( "%1%2m_bpmTap: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg(
								 m_bpmTap == BpmTap::TapTempo ? "Tap Tempo"
															  : "Beat Counter"
							 ) )
				.append( QString( "%1%2m_beatCounter: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg(
								 m_beatCounter == BeatCounter::Tap
									 ? "Tap"
									 : "Tap and Play"
							 ) )
				.append( QString( "%1%2m_nBeatCounterDriftCompensation: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nBeatCounterDriftCompensation ) )
				.append( QString( "%1%2m_nBeatCounterStartOffset: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nBeatCounterStartOffset ) )
				.append( QString( "%1%2m_onlineRepos: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_onlineRepos.join( ',' ) ) )
				.append( QString( "%1%2m_audioDriver: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( audioDriverToQString( m_audioDriver ) ) )
				.append( QString( "%1%2m_bUseMetronome: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bUseMetronome ) )
				.append( QString( "%1%2m_fMetronomeVolume: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_fMetronomeVolume ) )
				.append( QString( "%1%2m_nMaxNotes: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nMaxNotes ) )
				.append(
					QString( "%1%2m_interpolateMode: %3\n" )
						.arg( sPrefix )
						.arg( s )
						.arg( Interpolation::ModeToQString( m_interpolateMode )
						)
				)
				.append( QString( "%1%2m_nBufferSize: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nBufferSize ) )
				.append( QString( "%1%2m_nSampleRate: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nSampleRate ) )
				.append( QString( "%1%2m_sOSSDevice: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sOSSDevice ) )
				.append( QString( "%1%2m_midiDriver: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( midiDriverToQString( m_midiDriver ) ) )
				.append( QString( "%1%2m_sMidiPortName: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sMidiPortName ) )
				.append( QString( "%1%2m_sMidiOutputPortName: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sMidiOutputPortName ) )
				.append( QString( "%1%2m_midiActionChannel: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( static_cast<int>( m_midiActionChannel ) ) )
				.append( QString( "%1%2m_bMidiNoteOffIgnore: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bMidiNoteOffIgnore ) )
				.append( QString( "%1%2m_bEnableMidiFeedback: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bEnableMidiFeedback ) )
				.append( QString( "%1%2m_midiFeedbackChannel: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( static_cast<int>( m_midiFeedbackChannel ) ) )
				.append( QString( "%1%2m_bMidiClockInputHandling: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bMidiClockInputHandling ) )
				.append( QString( "%1%2m_bMidiTransportInputHandling: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bMidiTransportInputHandling ) )
				.append( QString( "%1%2m_bMidiClockOutputSend: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bMidiClockOutputSend ) )
				.append( QString( "%1%2m_bMidiTransportOutputSend: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bMidiTransportOutputSend ) )
				.append( QString( "%1%2m_midiSendNoteOff: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( static_cast<int>( m_midiSendNoteOff ) ) )
				.append( QString( "%1%2m_bOscServerEnabled: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bOscServerEnabled ) )
				.append( QString( "%1%2m_bOscFeedbackEnabled: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bOscFeedbackEnabled ) )
				.append( QString( "%1%2m_nOscServerPort: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nOscServerPort ) )
				.append( QString( "%1%2m_sAlsaAudioDevice: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sAlsaAudioDevice ) )
				.append( QString( "%1%2m_sPortAudioDevice: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sPortAudioDevice ) )
				.append( QString( "%1%2m_sPortAudioHostAPI: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sPortAudioHostAPI ) )
				.append( QString( "%1%2m_nLatencyTarget: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nLatencyTarget ) )
				.append( QString( "%1%2m_sCoreAudioDevice: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sCoreAudioDevice ) )
				.append( QString( "%1%2m_sJackPortName1: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sJackPortName1 ) )
				.append( QString( "%1%2m_sJackPortName2: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sJackPortName2 ) )
				.append( QString( "%1%2m_nJackTransportMode: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nJackTransportMode ) )
				.append( QString( "%1%2m_bJackConnectDefaults: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bJackConnectDefaults ) )
				.append( QString( "%1%2m_bJackTrackOuts: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bJackTrackOuts ) )
				.append( QString( "%1%2m_bJackEnforceInstrumentName: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bJackEnforceInstrumentName ) )
				.append( QString( "%1%2m_JackTrackOutputMode: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( static_cast<int>( m_JackTrackOutputMode ) ) )
				.append( QString( "%1%2m_bJackTimebaseEnabled: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bJackTimebaseEnabled ) )
				.append( QString( "%1%2m_bJackTimebaseMode: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bJackTimebaseMode ) )
				.append( QString( "%1%2m_nAutosavesPerHour: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nAutosavesPerHour ) )
				.append( QString( "%1%2m_sRubberBandCLIexecutable: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sRubberBandCLIexecutable ) )
				.append( QString( "%1%2m_bCountIn: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bCountIn ) )
				.append( QString( "%1%2m_sDefaultEditor: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sDefaultEditor ) )
				.append( QString( "%1%2m_sPreferredLanguage: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sPreferredLanguage ) )
				.append(
					QString( "%1%2m_bUseRelativeFileNamesForPlaylists: %3\n" )
						.arg( sPrefix )
						.arg( s )
						.arg( m_bUseRelativeFileNamesForPlaylists )
				)
				.append( QString( "%1%2m_bShowDevelWarning: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bShowDevelWarning ) )
				.append( QString( "%1%2m_bShowNoteOverwriteWarning: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bShowNoteOverwriteWarning ) )
				.append( QString( "%1%2m_sLastSongPath: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sLastSongPath ) )
				.append( QString( "%1%2m_sLastPlaylistPath: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_sLastPlaylistPath ) )
				.append( QString( "%1%2m_customSoundLibraryDirs: [%3]\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_customSoundLibraryDirs.join( ", " ) ) )
				.append( QString( "%1%2m_bHearNewNotes: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bHearNewNotes ) )
				.append( QString( "%1%2m_nPunchInPos: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nPunchInPos ) )
				.append( QString( "%1%2m_nPunchOutPos: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nPunchOutPos ) )
				.append( QString( "%1%2m_bQuantizeEvents: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bQuantizeEvents ) )
				.append( QString( "%1%2m_recentFiles: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_recentFiles.join( ',' ) ) )
				.append( QString( "%1%2m_nMaxBars: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nMaxBars ) )
				.append( QString( "%1%2m_bSearchForRubberbandOnLoad: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bSearchForRubberbandOnLoad ) )
				.append( QString( "%1%2m_bUseTheRubberbandBpmChangeEvent: %3\n"
				)
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bUseTheRubberbandBpmChangeEvent ) )
				.append( QString( "%1%2m_bShowInstrumentPeaks: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bShowInstrumentPeaks ) )
				.append( QString( "%1%2m_nPatternEditorGridResolution: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nPatternEditorGridResolution ) )
				.append( QString( "%1%2m_bPatternEditorUsingTriplets: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bPatternEditorUsingTriplets ) )
				.append(
					QString( "%1%2m_bPatternEditorAlwaysShowTypeLabels: %3\n" )
						.arg( sPrefix )
						.arg( s )
						.arg( m_bPatternEditorAlwaysShowTypeLabels )
				)
				.append( QString( "%1%2m_bHideKeyboardCursor: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bHideKeyboardCursor ) )
				.append( QString( "%1%2m_bShowPlaybackTrack: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bShowPlaybackTrack ) )
				.append( QString( "%1%2m_nLastOpenTab: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nLastOpenTab ) )
				.append( QString( "%1%2m_bShowAutomationArea: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_bShowAutomationArea ) )
				.append( QString( "%1%2m_nPatternEditorGridHeight: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nPatternEditorGridHeight ) )
				.append( QString( "%1%2m_nPatternEditorGridWidth: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nPatternEditorGridWidth ) )
				.append( QString( "%1%2m_nSongEditorGridHeight: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nSongEditorGridHeight ) )
				.append( QString( "%1%2m_nSongEditorGridWidth: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_nSongEditorGridWidth ) )
				.append( QString( "%1%2m_mainFormProperties: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_mainFormProperties.toQString( s, bShort ) )
				)
				.append( QString( "%1%2m_mixerProperties: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_mixerProperties.toQString( s, bShort ) ) )
				.append(
					QString( "%1%2m_patternEditorProperties: %3\n" )
						.arg( sPrefix )
						.arg( s )
						.arg( m_patternEditorProperties.toQString( s, bShort ) )
				)
				.append( QString( "%1%2m_songEditorProperties: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_songEditorProperties.toQString( s, bShort )
							 ) )
				.append( QString( "%1%2m_rackProperties: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_rackProperties.toQString( s, bShort ) ) )
				.append( QString( "%1%2m_audioEngineInfoProperties: %3\n" )
							 .arg( sPrefix )
							 .arg( s )
							 .arg( m_audioEngineInfoProperties.toQString(
								 s, bShort
							 ) ) );
		sOutput
			.append( QString( "%1%2m_playlistEditorProperties: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_playlistEditorProperties.toQString( s, bShort )
						 ) )
			.append( QString( "%1%2m_directorProperties: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_directorProperties.toQString( s, bShort ) ) )
			.append( QString( "%1%2m_sLastExportPatternAsDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastExportPatternAsDirectory ) )
			.append( QString( "%1%2m_sLastExportSongDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastExportSongDirectory ) )
			.append( QString( "%1%2m_sLastSaveSongAsDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastSaveSongAsDirectory ) )
			.append( QString( "%1%2m_sLastOpenSongDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastOpenSongDirectory ) )
			.append( QString( "%1%2m_sLastOpenPatternDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastOpenPatternDirectory ) )
			.append( QString( "%1%2m_sLastExportLilypondDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastExportLilypondDirectory ) )
			.append( QString( "%1%2m_sLastExportMidiDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastExportMidiDirectory ) )
			.append( QString( "%1%2m_sLastImportDrumkitDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastImportDrumkitDirectory ) )
			.append( QString( "%1%2m_sLastExportDrumkitDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastExportDrumkitDirectory ) )
			.append( QString( "%1%2m_sLastSaveDrumkitAsDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastSaveDrumkitAsDirectory ) )
			.append( QString( "%1%2m_sLastOpenLayerDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastOpenLayerDirectory ) )
			.append( QString( "%1%2m_sLastOpenPlaybackTrackDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastOpenPlaybackTrackDirectory ) )
			.append( QString( "%1%2m_sLastAddSongToPlaylistDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastAddSongToPlaylistDirectory ) )
			.append( QString( "%1%2m_sLastPlaylistDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastPlaylistDirectory ) )
			.append( QString( "%1%2m_sLastPlaylistScriptDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastPlaylistScriptDirectory ) )
			.append( QString( "%1%2m_sLastImportThemeDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastImportThemeDirectory ) )
			.append( QString( "%1%2m_sLastExportThemeDirectory: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_sLastExportThemeDirectory ) )
			.append( QString( "%1%2m_nExportSampleDepthIdx: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nExportSampleDepthIdx ) )
			.append( QString( "%1%2m_nExportSampleRateIdx: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nExportSampleRateIdx ) )
			.append( QString( "%1%2m_nExportModeIdx: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nExportModeIdx ) )
			.append( QString( "%1%2m_exportFormat: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( Filesystem::AudioFormatToSuffix( m_exportFormat )
						 ) )
			.append( QString( "%1%2m_fExportCompressionLevel: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_fExportCompressionLevel ) )
			.append( QString( "%1%2m_nMidiExportMode: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nMidiExportMode ) )
			.append( QString( "%1%2m_bMidiExportUseHumanization: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bMidiExportUseHumanization ) )
			.append( QString( "%1%2m_bSoundLibraryShowName: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bSoundLibraryShowName ) )
			.append( QString( "%1%2m_bSoundLibraryShowAuthor: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bSoundLibraryShowAuthor ) )
			.append( QString( "%1%2m_bSoundLibraryShowInfo: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bSoundLibraryShowInfo ) )
			.append( QString( "%1%2m_bSoundLibraryShowLicense: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bSoundLibraryShowLicense ) )
			.append( QString( "%1%2m_bSoundLibraryShowPath: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bSoundLibraryShowPath ) )
			.append( QString( "%1%2m_bSoundLibraryShowTags: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bSoundLibraryShowTags ) )
			.append( QString( "%1%2m_bSoundLibraryShowVersion: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bSoundLibraryShowVersion ) )
			.append( QString( "%1%2m_nSoundLibraryLastTab: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nSoundLibraryLastTab ) )
			.append( QString( "%1%2m_nRackLastTab: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_nRackLastTab ) )
			.append( QString( "%1%2m_bShowExportSongLicenseWarning: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bShowExportSongLicenseWarning ) )
			.append( QString( "%1%2m_bShowExportDrumkitLicenseWarning: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bShowExportDrumkitLicenseWarning ) )
			.append( QString( "%1%2m_bShowExportDrumkitCopyleftWarning: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bShowExportDrumkitCopyleftWarning ) )
			.append( QString( "%1%2m_bShowExportDrumkitAttributionWarning: %3\n"
			)
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bShowExportDrumkitAttributionWarning ) )
			.append( QString( "%1%2m_pTheme: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_pTheme->toQString( s, bShort ) ) )
			.append( QString( "%1%2m_pShortcuts: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_pShortcuts->toQString( s, bShort ) ) )
			.append( QString( "%1%2m_pMidiEventMap: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_pMidiEventMap->toQString( s, bShort ) ) )
			.append( QString( "%1%2m_pMidiInstrumentMap: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_pMidiInstrumentMap->toQString( s, bShort ) ) )
			.append( QString( "%1%2m_bLoadingSuccessful: %3\n" )
						 .arg( sPrefix )
						 .arg( s )
						 .arg( m_bLoadingSuccessful ) );
	}
	else {
		sOutput =
			QString( "[Preferences] " )
				.append( QString( "m_bPlaySamplesOnClicking: %1" )
							 .arg( m_bPlaySamplesOnClicking ) )
				.append( QString( ", m_bFollowPlayhead: %1" )
							 .arg( m_bFollowPlayhead ) )
				.append( QString( ", m_bpmTap: %1" )
							 .arg(
								 m_bpmTap == BpmTap::TapTempo ? "Tap Tempo"
															  : "Beat Counter"
							 ) )
				.append( QString( ", m_beatCounter: %1" )
							 .arg(
								 m_beatCounter == BeatCounter::Tap
									 ? "Tap"
									 : "Tap and Play"
							 ) )
				.append( QString( ", m_nBeatCounterDriftCompensation: %1" )
							 .arg( m_nBeatCounterDriftCompensation ) )
				.append( QString( ", m_nBeatCounterStartOffset: %1" )
							 .arg( m_nBeatCounterStartOffset ) )
				.append( QString( ", m_onlineRepos: %1" )
							 .arg( m_onlineRepos.join( ',' ) ) )
				.append( QString( ", m_audioDriver: %1" )
							 .arg( audioDriverToQString( m_audioDriver ) ) )
				.append(
					QString( ", m_bUseMetronome: %1" ).arg( m_bUseMetronome )
				)
				.append( QString( ", m_fMetronomeVolume: %1" )
							 .arg( m_fMetronomeVolume ) )
				.append( QString( ", m_nMaxNotes: %1" ).arg( m_nMaxNotes ) )
				.append(
					QString( ", m_interpolateMode: %1" )
						.arg( Interpolation::ModeToQString( m_interpolateMode )
						)
				)
				.append( QString( ", m_nBufferSize: %1" ).arg( m_nBufferSize ) )
				.append( QString( ", m_nSampleRate: %1" ).arg( m_nSampleRate ) )
				.append( QString( ", m_sOSSDevice: %1" ).arg( m_sOSSDevice ) )
				.append( QString( ", m_midiDriver: %1" )
							 .arg( midiDriverToQString( m_midiDriver ) ) )
				.append(
					QString( ", m_sMidiPortName: %1" ).arg( m_sMidiPortName )
				)
				.append( QString( ", m_sMidiOutputPortName: %1" )
							 .arg( m_sMidiOutputPortName ) )
				.append( QString( ", m_midiActionChannel: %1" )
							 .arg( static_cast<int>( m_midiActionChannel ) ) )
				.append( QString( ", m_bMidiNoteOffIgnore: %1" )
							 .arg( m_bMidiNoteOffIgnore ) )
				.append( QString( ", m_bEnableMidiFeedback: %1" )
							 .arg( m_bEnableMidiFeedback ) )
				.append( QString( ", m_midiFeedbackChannel: %1" )
							 .arg( static_cast<int>( m_midiFeedbackChannel ) ) )
				.append( QString( ", m_bMidiClockInputHandling: %1" )
							 .arg( m_bMidiClockInputHandling ) )
				.append( QString( ", m_bMidiTransportInputHandling: %1" )
							 .arg( m_bMidiTransportInputHandling ) )
				.append( QString( ", m_bMidiClockOutputSend: %1" )
							 .arg( m_bMidiClockOutputSend ) )
				.append( QString( ", m_bMidiTransportOutputSend: %1" )
							 .arg( m_bMidiTransportOutputSend ) )
				.append( QString( ", m_midiSendNoteOff: %1" )
							 .arg( static_cast<int>( m_midiSendNoteOff ) ) )
				.append( QString( "], m_bOscServerEnabled: %1" )
							 .arg( m_bOscServerEnabled ) )
				.append( QString( ", m_bOscFeedbackEnabled: %1" )
							 .arg( m_bOscFeedbackEnabled ) )
				.append(
					QString( ", m_nOscServerPort: %1" ).arg( m_nOscServerPort )
				)
				.append( QString( ", m_sAlsaAudioDevice: %1" )
							 .arg( m_sAlsaAudioDevice ) )
				.append( QString( ", m_sPortAudioDevice: %1" )
							 .arg( m_sPortAudioDevice ) )
				.append( QString( ", m_sPortAudioHostAPI: %1" )
							 .arg( m_sPortAudioHostAPI ) )
				.append(
					QString( ", m_nLatencyTarget: %1" ).arg( m_nLatencyTarget )
				)
				.append( QString( ", m_sCoreAudioDevice: %1" )
							 .arg( m_sCoreAudioDevice ) )
				.append(
					QString( ", m_sJackPortName1: %1" ).arg( m_sJackPortName1 )
				)
				.append(
					QString( ", m_sJackPortName2: %1" ).arg( m_sJackPortName2 )
				)
				.append( QString( ", m_nJackTransportMode: %1" )
							 .arg( m_nJackTransportMode ) )
				.append( QString( ", m_bJackConnectDefaults: %1" )
							 .arg( m_bJackConnectDefaults ) )
				.append(
					QString( ", m_bJackTrackOuts: %1" ).arg( m_bJackTrackOuts )
				)
				.append( QString( ", m_bJackEnforceInstrumentName: %1" )
							 .arg( m_bJackEnforceInstrumentName ) )
				.append( QString( ", m_JackTrackOutputMode: %1" )
							 .arg( static_cast<int>( m_JackTrackOutputMode ) ) )
				.append( QString( ", m_bJackTimebaseEnabled: %1" )
							 .arg( m_bJackTimebaseEnabled ) )
				.append( QString( ", m_bJackTimebaseMode: %1" )
							 .arg( m_bJackTimebaseMode ) )
				.append( QString( ", m_nAutosavesPerHour: %1" )
							 .arg( m_nAutosavesPerHour ) )
				.append( QString( ", m_sRubberBandCLIexecutable: %1" )
							 .arg( m_sRubberBandCLIexecutable ) )
				.append( QString( ", m_bCountIn: %1" ).arg( m_bCountIn ) )
				.append(
					QString( ", m_sDefaultEditor: %1" ).arg( m_sDefaultEditor )
				)
				.append( QString( ", m_sPreferredLanguage: %1" )
							 .arg( m_sPreferredLanguage ) )
				.append( QString( ", m_bUseRelativeFileNamesForPlaylists: %1" )
							 .arg( m_bUseRelativeFileNamesForPlaylists ) )
				.append( QString( ", m_bShowDevelWarning: %1" )
							 .arg( m_bShowDevelWarning ) )
				.append( QString( ", m_bShowNoteOverwriteWarning: %1" )
							 .arg( m_bShowNoteOverwriteWarning ) )
				.append(
					QString( ", m_sLastSongPath: %1" ).arg( m_sLastSongPath )
				)
				.append( QString( ", m_sLastPlaylistPath: %1" )
							 .arg( m_sLastPlaylistPath ) )
				.append( QString( ", m_customSoundLibraryDirs: [%1]" )
							 .arg( m_customSoundLibraryDirs.join( ", " ) ) )
				.append(
					QString( ", m_bHearNewNotes: %1" ).arg( m_bHearNewNotes )
				)
				.append( QString( ", m_nPunchInPos: %1" ).arg( m_nPunchInPos ) )
				.append( QString( ", m_nPunchOutPos: %1" ).arg( m_nPunchOutPos )
				)
				.append( QString( ", m_bQuantizeEvents: %1" )
							 .arg( m_bQuantizeEvents ) )
				.append( QString( ", m_recentFiles: %1" )
							 .arg( m_recentFiles.join( ',' ) ) )
				.append( QString( ", m_nMaxBars: %1" ).arg( m_nMaxBars ) )
				.append( QString( ", m_bSearchForRubberbandOnLoad: %1" )
							 .arg( m_bSearchForRubberbandOnLoad ) )
				.append( QString( ", m_bUseTheRubberbandBpmChangeEvent: %1" )
							 .arg( m_bUseTheRubberbandBpmChangeEvent ) )
				.append( QString( ", m_bShowInstrumentPeaks: %1" )
							 .arg( m_bShowInstrumentPeaks ) )
				.append( QString( ", m_nPatternEditorGridResolution: %1" )
							 .arg( m_nPatternEditorGridResolution ) )
				.append( QString( ", m_bPatternEditorUsingTriplets: %1" )
							 .arg( m_bPatternEditorUsingTriplets ) )
				.append( QString( ", m_bPatternEditorAlwaysShowTypeLabels: %1" )
							 .arg( m_bPatternEditorAlwaysShowTypeLabels ) )
				.append( QString( ", m_bHideKeyboardCursor: %1" )
							 .arg( m_bHideKeyboardCursor ) )
				.append( QString( ", m_bShowPlaybackTrack: %1" )
							 .arg( m_bShowPlaybackTrack ) )
				.append( QString( ", m_nLastOpenTab: %1" ).arg( m_nLastOpenTab )
				)
				.append( QString( ", m_bShowAutomationArea: %1" )
							 .arg( m_bShowAutomationArea ) )
				.append( QString( ", m_nPatternEditorGridHeight: %1" )
							 .arg( m_nPatternEditorGridHeight ) )
				.append( QString( ", m_nPatternEditorGridWidth: %1" )
							 .arg( m_nPatternEditorGridWidth ) )
				.append( QString( ", m_nSongEditorGridHeight: %1" )
							 .arg( m_nSongEditorGridHeight ) )
				.append( QString( ", m_nSongEditorGridWidth: %1" )
							 .arg( m_nSongEditorGridWidth ) )
				.append( QString( ", m_mainFormProperties: %1" )
							 .arg( m_mainFormProperties.toQString( "", bShort )
							 ) )
				.append( QString( ", m_mixerProperties: %1" )
							 .arg( m_mixerProperties.toQString( "", bShort ) ) )
				.append( QString( ", m_patternEditorProperties: %1" )
							 .arg( m_patternEditorProperties.toQString(
								 "", bShort
							 ) ) )
				.append(
					QString( ", m_songEditorProperties: %1" )
						.arg( m_songEditorProperties.toQString( "", bShort ) )
				)
				.append( QString( ", m_rackProperties: %1" )
							 .arg( m_rackProperties.toQString( "", bShort ) ) )
				.append( QString( ", m_audioEngineInfoProperties: %1" )
							 .arg( m_audioEngineInfoProperties.toQString(
								 "", bShort
							 ) ) );
		sOutput
			.append(
				QString( ", m_playlistEditorProperties: %1" )
					.arg( m_playlistEditorProperties.toQString( "", bShort ) )
			)
			.append( QString( ", m_directorProperties: %1" )
						 .arg( m_directorProperties.toQString( "", bShort ) ) )
			.append( QString( ", m_sLastExportPatternAsDirectory: %1" )
						 .arg( m_sLastExportPatternAsDirectory ) )
			.append( QString( ", m_sLastExportSongDirectory: %1" )
						 .arg( m_sLastExportSongDirectory ) )
			.append( QString( ", m_sLastSaveSongAsDirectory: %1" )
						 .arg( m_sLastSaveSongAsDirectory ) )
			.append( QString( ", m_sLastOpenSongDirectory: %1" )
						 .arg( m_sLastOpenSongDirectory ) )
			.append( QString( ", m_sLastOpenPatternDirectory: %1" )
						 .arg( m_sLastOpenPatternDirectory ) )
			.append( QString( ", m_sLastExportLilypondDirectory: %1" )
						 .arg( m_sLastExportLilypondDirectory ) )
			.append( QString( ", m_sLastExportMidiDirectory: %1" )
						 .arg( m_sLastExportMidiDirectory ) )
			.append( QString( ", m_sLastImportDrumkitDirectory: %1" )
						 .arg( m_sLastImportDrumkitDirectory ) )
			.append( QString( ", m_sLastExportDrumkitDirectory: %1" )
						 .arg( m_sLastExportDrumkitDirectory ) )
			.append( QString( ", m_sLastSaveDrumkitAsDirectory: %1" )
						 .arg( m_sLastSaveDrumkitAsDirectory ) )
			.append( QString( ", m_sLastOpenLayerDirectory: %1" )
						 .arg( m_sLastOpenLayerDirectory ) )
			.append( QString( ", m_sLastOpenPlaybackTrackDirectory: %1" )
						 .arg( m_sLastOpenPlaybackTrackDirectory ) )
			.append( QString( ", m_sLastAddSongToPlaylistDirectory: %1" )
						 .arg( m_sLastAddSongToPlaylistDirectory ) )
			.append( QString( ", m_sLastPlaylistDirectory: %1" )
						 .arg( m_sLastPlaylistDirectory ) )
			.append( QString( ", m_sLastPlaylistScriptDirectory: %1" )
						 .arg( m_sLastPlaylistScriptDirectory ) )
			.append( QString( ", m_sLastImportThemeDirectory: %1" )
						 .arg( m_sLastImportThemeDirectory ) )
			.append( QString( ", m_sLastExportThemeDirectory: %1" )
						 .arg( m_sLastExportThemeDirectory ) )
			.append( QString( ", m_nExportSampleDepthIdx: %1" )
						 .arg( m_nExportSampleDepthIdx ) )
			.append( QString( ", m_nExportSampleRateIdx: %1" )
						 .arg( m_nExportSampleRateIdx ) )
			.append( QString( ", m_nExportModeIdx: %1" ).arg( m_nExportModeIdx )
			)
			.append( QString( ", m_exportFormat: %1" )
						 .arg( Filesystem::AudioFormatToSuffix( m_exportFormat )
						 ) )
			.append( QString( ", m_fExportCompressionLevel: %1" )
						 .arg( m_fExportCompressionLevel ) )
			.append(
				QString( ", m_nMidiExportMode: %1" ).arg( m_nMidiExportMode )
			)
			.append( QString( ", m_bMidiExportUseHumanization: %1" )
						 .arg( m_bMidiExportUseHumanization ) )
			.append( QString( ", m_bSoundLibraryShowName: %1" )
						 .arg( m_bSoundLibraryShowName ) )
			.append( QString( ", m_bSoundLibraryShowAuthor: %1" )
						 .arg( m_bSoundLibraryShowAuthor ) )
			.append( QString( ", m_bSoundLibraryShowInfo: %1" )
						 .arg( m_bSoundLibraryShowInfo ) )
			.append( QString( ", m_bSoundLibraryShowLicense: %1" )
						 .arg( m_bSoundLibraryShowLicense ) )
			.append( QString( ", m_bSoundLibraryShowPath: %1" )
						 .arg( m_bSoundLibraryShowPath ) )
			.append( QString( ", m_bSoundLibraryShowTags: %1" )
						 .arg( m_bSoundLibraryShowTags ) )
			.append( QString( ", m_bSoundLibraryShowVersion: %1" )
						 .arg( m_bSoundLibraryShowVersion ) )
			.append( QString( ", m_nSoundLibraryLastTab: %1" )
						 .arg( m_nSoundLibraryLastTab ) )
			.append( QString( ", m_nRackLastTab: %1" ).arg( m_nRackLastTab ) )
			.append( QString( ", m_bShowExportSongLicenseWarning: %1" )
						 .arg( m_bShowExportSongLicenseWarning ) )
			.append( QString( ", m_bShowExportDrumkitLicenseWarning: %1" )
						 .arg( m_bShowExportDrumkitLicenseWarning ) )
			.append( QString( ", m_bShowExportDrumkitCopyleftWarning: %1" )
						 .arg( m_bShowExportDrumkitCopyleftWarning ) )
			.append( QString( ", m_bShowExportDrumkitAttributionWarning: %1" )
						 .arg( m_bShowExportDrumkitAttributionWarning ) )
			.append( QString( ", m_pTheme: %1" )
						 .arg( m_pTheme->toQString( "", bShort ) ) )
			.append( QString( ", m_pShortcuts: %1" )
						 .arg( m_pShortcuts->toQString( "", bShort ) ) )
			.append( QString( ", m_pMidiEventMap: %1" )
						 .arg( m_pMidiEventMap->toQString( "", bShort ) ) )
			.append( QString( ", m_pMidiInstrumentMap: %1" )
						 .arg( m_pMidiInstrumentMap->toQString( "", bShort ) ) )
			.append( QString( ", m_bLoadingSuccessful: %1" )
						 .arg( m_bLoadingSuccessful ) );
	}

	return sOutput;
}

// -----------------------

QByteArray Preferences::corePropsToXml() const
{
	XMLDoc doc;
	XMLNode rootNode = doc.set_root( PreferencesKeys::Root );

	// The IPC fragment intentionally carries no formatVersion/version: it is
	// only ever consumed by applyCorePropsFromXml() within a running session.
	PreferencesSchema::WriteContext context;
	context.bSilent = true;
	PreferencesSchema::writeRows(
		rootNode, *this, PreferencesSchema::Owner::Core, context
	);

	return doc.toString().toUtf8();
}

void Preferences::applyCorePropsFromXml( const QByteArray& xml )
{
	XMLDoc doc;
	if ( !doc.setContent( xml ) ) {
		ERRORLOG( "Unable to parse core preferences XML" );
		return;
	}
	const XMLNode rootNode = doc.firstChildElement( PreferencesKeys::Root );
	if ( rootNode.isNull() ) {
		ERRORLOG( "Core preferences XML: no root node found" );
		return;
	}

	// IPC fragments are trusted: missing structural nodes are skipped
	// silently, and a missing midiInstrumentMap element keeps the current
	// mapping (no legacy derivation).
	PreferencesSchema::ReadContext context;
	context.bSilent = true;
	context.bSearchForRubberband = m_bSearchForRubberbandOnLoad;
	context.bEmitStructuralWarnings = false;
	context.bApplyLegacyMidiInput = false;

	PreferencesSchema::readRows(
		rootNode, *this, PreferencesSchema::Owner::Core, context
	);
}

QString Preferences::ChangesToQString( Preferences::Changes changes )
{
	QStringList changesList;

	if ( changes & Changes::None ) {
		changesList << "None";
	}
	if ( changes & Changes::Font ) {
		changesList << "Font";
	}
	if ( changes & Changes::Colors ) {
		changesList << "Colors";
	}
	if ( changes & Changes::AppearanceTab ) {
		changesList << "AppearanceTab";
	}
	if ( changes & Changes::GeneralTab ) {
		changesList << "GeneralTab";
	}
	if ( changes & Changes::AudioTab ) {
		changesList << "AudioTab";
	}
	if ( changes & Changes::MidiTab ) {
		changesList << "MidiTab";
	}
	if ( changes & Changes::OscTab ) {
		changesList << "OscTab";
	}
	if ( changes & Changes::ShortcutTab ) {
		changesList << "ShortcutTab";
	}

	return std::move( QString( "[%1]" ).arg( changesList.join( ", " ) ) );
}
};	// namespace H2Core
