/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "PreferencesSchemaTest.h"

#include "TestHelper.h"

#include <core/Basics/Event.h>
#include <core/Helpers/Filesystem.h>
#include <core/Logger.h>
#include <core/Midi/Midi.h>
#include <core/Midi/MidiAction.h>
#include <core/Midi/MidiEvent.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/PreferencesKeys.h>
#include <core/Preferences/PreferencesSchema.h>

#include <QColor>
#include <QCoreApplication>
#include <QFile>
#include <QKeySequence>
#include <QTextStream>

#include <initializer_list>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

using namespace H2Core;

namespace {

// ---- perturbation ----------------------------------------------------------

template <typename T>
void perturbValue( T& value )
{
	// Enums, WindowProperties, and shared_ptr sub-objects are perturbed by
	// perturbSpecialized() below: they either reject plain arithmetic or
	// would leave their serialized value range.
	if constexpr ( std::is_same_v<T, bool> ) {
		value = !value;
	}
	else if constexpr ( std::is_same_v<T, int> ||
						std::is_same_v<T, unsigned> ) {
		value = value + 1;
	}
	else if constexpr ( std::is_same_v<T, float> ) {
		// A short, exactly printable value: the round-trip must not depend
		// on decimal precision of the XML float format.
		value = 0.75f;
	}
	else if constexpr ( std::is_same_v<T, QString> ) {
		value = value + "X";
	}
	else if constexpr ( std::is_same_v<T, QStringList> ) {
		value.append( "X" );
	}
	else if constexpr ( std::is_same_v<T, std::vector<QColor>> ) {
		// The read codec always yields nMaxPatternColors entries, so the
		// container size must stay untouched.
		if ( !value.empty() ) {
			value.front() = QColor( 12, 34, 56 );
			value.back() = QColor( 65, 43, 21 );
		}
	}
}

void perturbSpecialized( PreferencesData& data )
{
	data.m_bpmTap = PreferencesData::BpmTap::BeatCounter;
	data.m_beatCounter = PreferencesData::BeatCounter::TapAndPlay;
	data.m_audioDriver = PreferencesData::AudioDriver::Oss;
	data.m_interpolateMode = Interpolation::InterpolateMode::Cosine;
	data.m_JackTrackOutputMode = PreferencesData::JackTrackOutputMode::preFader;
	data.m_midiDriver = PreferencesData::MidiDriver::PortMidi;
	data.m_midiSendNoteOff = PreferencesData::MidiSendNoteOff::OnCustomLengths;
	// Aif, Aifc and Aiff are synonyms sharing the "aiff" suffix: the config
	// format preserves the audio format, not the enum spelling
	// (AudioFormatFromSuffix canonicalizes to Aif).
	data.m_exportFormat = Filesystem::AudioFormat::Aif;
	// The out-of-range channels use dedicated sentinel values in the legacy
	// encodings - exercise them instead of regular channels.
	data.m_midiActionChannel = Midi::ChannelAll;
	data.m_midiFeedbackChannel = Midi::ChannelOff;

	// Plain ints, but serialized as one of two string constants: the
	// generic +1 perturbation would leave the valid range.
	data.m_nJackTransportMode = PreferencesData::NO_JACK_TRANSPORT;
	data.m_bJackTimebaseMode = PreferencesData::USE_JACK_TIMEBASE_CONTROL;

	data.m_pTheme->m_pInterface->m_layout = InterfaceTheme::Layout::Tabbed;
	data.m_pTheme->m_pInterface->m_uiScalingPolicy =
		InterfaceTheme::ScalingPolicy::Larger;
	data.m_pTheme->m_pInterface->m_iconColor = InterfaceTheme::IconColor::White;
	data.m_pTheme->m_pInterface->m_coloringMethod =
		InterfaceTheme::ColoringMethod::Custom;
	data.m_pTheme->m_pFont->m_fontSize = FontTheme::FontSize::Large;

	data.m_mainFormProperties.set(
		111, 222, 333, 444, false, QByteArray( "geometryMain" )
	);
	data.m_mixerProperties.set(
		10, 20, 30, 40, false, QByteArray( "geometryMixer" )
	);
	data.m_patternEditorProperties.set(
		50, 60, 70, 80, true, QByteArray( "geometryPattern" )
	);
	data.m_songEditorProperties.set(
		90, 100, 110, 120, true, QByteArray( "geometrySong" )
	);
	data.m_rackProperties.set(
		130, 140, 150, 160, false, QByteArray( "geometryRack" )
	);
	data.m_audioEngineInfoProperties.set(
		170, 180, 190, 200, true, QByteArray( "geometryInfo" )
	);
	data.m_playlistEditorProperties.set(
		210, 220, 230, 240, false, QByteArray( "geometryPlaylist" )
	);
	data.m_directorProperties.set(
		250, 260, 270, 280, true, QByteArray( "geometryDirector" )
	);

	auto pColor = std::make_shared<ColorTheme>( data.m_pTheme->m_pColor );
	pColor->m_songEditor_backgroundColor = QColor( 1, 2, 3 );
	pColor->m_songEditor_selectedRowTextColor = QColor( 4, 5, 6 );
	data.m_pTheme->m_pColor = pColor;

	data.m_pShortcuts->insertShortcut(
		QKeySequence( "Ctrl+Alt+Shift+P" ), Shortcuts::Action::Panic
	);

	data.m_pMidiEventMap->registerEvent(
		MidiEvent::Type::CC, Midi::parameterFromInt( 42 ),
		std::make_shared<MidiAction>( MidiAction::Type::BpmIncr ),
		Event::Trigger::Suppress, nullptr
	);

	data.m_pMidiInstrumentMap->setUseGlobalInputChannel(
		!data.m_pMidiInstrumentMap->getUseGlobalInputChannel()
	);
	data.m_pMidiInstrumentMap->setInput( MidiInstrumentMap::Input::Custom );
	data.m_pMidiInstrumentMap->setOutput( MidiInstrumentMap::Output::Constant );
}

void perturbAll( PreferencesData& data )
{
	auto tuple = PreferencesSchema::tieAllMembers( data );
	std::apply(
		[]( auto&&... elems ) { ( ..., perturbValue( elems ) ); }, tuple
	);
	perturbSpecialized( data );
	// The write codec replaces a non-executable rubberband path with a
	// placeholder; point at a binary that actually exists.
	data.m_sRubberBandCLIexecutable = QCoreApplication::applicationFilePath();
}

// ---- comparison ------------------------------------------------------------

template <typename T>
struct is_shared_ptr : std::false_type {};
template <typename T>
struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

QString serializeShared( const std::shared_ptr<ColorTheme>& p )
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "probe" );
	p->saveTo( root );
	return doc.toString();
}

QString serializeShared( const std::shared_ptr<Shortcuts>& p )
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "probe" );
	p->saveTo( root );
	return doc.toString();
}

QString serializeShared( const std::shared_ptr<MidiEventMap>& p )
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "probe" );
	p->saveTo( root, true /*bSilent*/ );
	return doc.toString();
}

QString serializeShared( const std::shared_ptr<MidiInstrumentMap>& p )
{
	XMLDoc doc;
	XMLNode root = doc.set_root( "probe" );
	p->saveTo( root );
	return doc.toString();
}

bool equal( const WindowProperties& a, const WindowProperties& b )
{
	return a.x == b.x && a.y == b.y && a.width == b.width &&
		   a.height == b.height && a.visible == b.visible &&
		   a.m_geometry == b.m_geometry;
}

template <typename T>
QString describe( const T& value )
{
	if constexpr ( std::is_same_v<T, QString> ) {
		return value;
	}
	else if constexpr ( std::is_same_v<T, bool> ) {
		return value ? "true" : "false";
	}
	else if constexpr ( std::is_same_v<T, float> ) {
		return QString::number( value );
	}
	else if constexpr ( std::is_same_v<T, QStringList> ||
						std::is_same_v<T, std::vector<QColor>> ) {
		return QString::number( value.size() ) + " entries";
	}
	else if constexpr ( std::is_enum_v<T> ) {
		return QString::number( static_cast<int>( value ) );
	}
	else if constexpr ( std::is_same_v<T, WindowProperties> ) {
		return "window geometry";
	}
	else if constexpr ( is_shared_ptr<T>::value ) {
		return "composite";
	}
	else {
		return QString::number( value );
	}
}

template <std::size_t I = 0, typename Tuple>
void compareTuple(
	const Tuple& a,
	const Tuple& b,
	QString& sReport,
	int& nFailures
)
{
	if constexpr ( I < std::tuple_size_v<Tuple> ) {
		using Elem = std::remove_reference_t<decltype( std::get<I>( a ) )>;
		const Elem& ea = std::get<I>( a );
		const Elem& eb = std::get<I>( b );
		bool bEqual = false;
		if constexpr ( is_shared_ptr<Elem>::value ) {
			// Pointer equality would always fail: loadFrom() hands back a
			// fresh object. Compare the serialized form instead.
			bEqual = serializeShared( ea ) == serializeShared( eb );
		}
		else if constexpr ( std::is_same_v<Elem, WindowProperties> ) {
			bEqual = equal( ea, eb );
		}
		else {
			bEqual = ( ea == eb );
		}
		if ( !bEqual ) {
			++nFailures;
			sReport += QString( "  [%1] %2 != %3\n" )
						   .arg( static_cast<int>( I ) )
						   .arg( describe( ea ) )
						   .arg( describe( eb ) );
		}
		compareTuple<I + 1>( a, b, sReport, nFailures );
	}
}

}  // namespace

void PreferencesSchemaTest::setUp()
{
	// Ensure Error-level messages are emitted regardless of the suite's
	// configured log level; restored in tearDown().
	m_nPreviousBitMask = Logger::bit_mask();
	Logger::set_bit_mask( m_nPreviousBitMask | Logger::Error );
}

void PreferencesSchemaTest::tearDown()
{
	Logger::set_bit_mask( m_nPreviousBitMask );
}

void PreferencesSchemaTest::testSchemaRoundTrip()
{
	___INFOLOG( "" );

	PreferencesData source;
	perturbAll( source );

	XMLDoc doc;
	XMLNode root = doc.set_root( "hydrogen_preferences" );
	PreferencesSchema::WriteContext writeContext;
	writeContext.bSilent = true;
	PreferencesSchema::writeRows( root, source, std::nullopt, writeContext );

	PreferencesData loaded;
	PreferencesSchema::ReadContext readContext;
	readContext.bSilent = true;
	readContext.bEmitStructuralWarnings = false;
	readContext.bSearchForRubberband = true;
	PreferencesSchema::readRows( root, loaded, std::nullopt, readContext );

	auto a = PreferencesSchema::tieAllMembers( source );
	auto b = PreferencesSchema::tieAllMembers( loaded );
	QString sReport;
	int nFailures = 0;
	compareTuple( a, b, sReport, nFailures );

	CPPUNIT_ASSERT_MESSAGE(
		QString( "%1 of %2 schema rows lost data:\n%3" )
			.arg( nFailures )
			.arg( static_cast<int>( std::tuple_size_v<decltype( a )> ) )
			.arg( sReport )
			.toStdString(),
		nFailures == 0
	);

	___INFOLOG( "passed" );
}

void PreferencesSchemaTest::testUnknownElementsReported()
{
	___INFOLOG( "" );

	const QString sLogPath =
		Filesystem::tmpDir().append( "preferencesSchemaUnknown.log" );
	auto pLogger =
		Logger::createInstanceLogger( sLogPath, false, false, false );

	{
		Logger::Scope scope( pLogger );

		XMLDoc doc;
		XMLNode root = doc.set_root( "hydrogen_preferences" );
		root.write_int( "maxBars", 42 );	   // covered by a schema row
		root.write_int( "totally_bogus", 1 );  // no schema row covers this
		PreferencesSchema::checkForUnknownElements( root );
	}
	// Destroying the logger joins its worker thread, flushing + closing the
	// file deterministically before it is read.
	delete pLogger;

	QFile file( sLogPath );
	CPPUNIT_ASSERT( file.open( QIODevice::ReadOnly | QIODevice::Text ) );
	const QString sLog = QTextStream( &file ).readAll();
	file.close();

	CPPUNIT_ASSERT( sLog.contains( "totally_bogus" ) );
	CPPUNIT_ASSERT( ! sLog.contains( "maxBars" ) );

	Filesystem::rm( sLogPath, false, true );

	___INFOLOG( "passed" );
}

void PreferencesSchemaTest::testLegacyElementsTolerated()
{
	___INFOLOG( "" );

	const QString sLogPath =
		Filesystem::tmpDir().append( "preferencesSchemaLegacy.log" );
	auto pLogger =
		Logger::createInstanceLogger( sLogPath, false, false, false );

	{
		Logger::Scope scope( pLogger );

		XMLDoc doc;
		XMLNode root = doc.set_root( "hydrogen_preferences" );

		// Elements written by pre-2.0 versions and dropped with the
		// LADSPA FX panel (ADR 0024). They are expected migration input
		// in old config files, not config drift, and must not be
		// reported.
		XMLNode recentFXNode = root.createNode( "recentlyUsedEffects" );
		recentFXNode.createNode( "FX" );
		XMLNode guiNode = root.createNode( PreferencesKeys::Gui );
		guiNode.write_bool( "isFXTabVisible", true );
		for ( int ii = 0; ii < 4; ii++ ) {
			guiNode.createNode(
				QString( "ladspaFX_properties%1" ).arg( ii ) );
		}

		// Control: genuine drift must still be reported.
		root.write_int( "totally_bogus", 1 );

		PreferencesSchema::checkForUnknownElements( root );
	}
	// Destroying the logger joins its worker thread, flushing + closing
	// the file deterministically before it is read.
	delete pLogger;

	QFile file( sLogPath );
	CPPUNIT_ASSERT( file.open( QIODevice::ReadOnly | QIODevice::Text ) );
	const QString sLog = QTextStream( &file ).readAll();
	file.close();

	CPPUNIT_ASSERT( ! sLog.contains( "recentlyUsedEffects" ) );
	CPPUNIT_ASSERT( ! sLog.contains( "isFXTabVisible" ) );
	CPPUNIT_ASSERT( ! sLog.contains( "ladspaFX_properties" ) );
	CPPUNIT_ASSERT( sLog.contains( "totally_bogus" ) );

	Filesystem::rm( sLogPath, false, true );

	___INFOLOG( "passed" );
}

void PreferencesSchemaTest::testLegacyDefaultConfigClean()
{
	___INFOLOG( "" );

	const QString sLogPath =
		Filesystem::tmpDir().append( "preferencesSchemaLegacyFile.log" );
	auto pLogger =
		Logger::createInstanceLogger( sLogPath, false, false, false );

	{
		Logger::Scope scope( pLogger );

		XMLDoc doc;
		CPPUNIT_ASSERT( doc.read(
			H2TEST_FILE( "preferences/legacy-1.2.conf" ) ) );
		XMLNode root = doc.firstChildElement( "hydrogen_preferences" );
		CPPUNIT_ASSERT( ! root.isNull() );

		// The verbatim 1.2.6 default config — the migration input every
		// pre-2.0 user config derives from. It must pass the unknown
		// element check without any report.
		PreferencesSchema::checkForUnknownElements( root );
	}
	// Destroying the logger joins its worker thread, flushing + closing
	// the file deterministically before it is read.
	delete pLogger;

	QFile file( sLogPath );
	CPPUNIT_ASSERT( file.open( QIODevice::ReadOnly | QIODevice::Text ) );
	const QString sLog = QTextStream( &file ).readAll();
	file.close();

	CPPUNIT_ASSERT_MESSAGE(
		sLog.toStdString(), ! sLog.contains( "Unknown element" ) );

	Filesystem::rm( sLogPath, false, true );

	___INFOLOG( "passed" );
}

void PreferencesSchemaTest::testOverrideLayerMembership()
{
	___INFOLOG( "" );

	namespace K = PreferencesKeys;

	// The host/state-owned override set (ADR 0022), pinned exactly: the
	// audio driver trio, the MIDI driver identity, the per-driver device
	// subtrees, the JACK and OSC state, the last-opened file pointers, and
	// the recent-songs list. A row joining or leaving this set is a
	// layering decision that must be deliberate, not drift.
	std::set<QString> expected;
	const auto add = [ &expected ]( const QString& sParent,
									const QString& sKey ) {
		expected.insert(
			sParent.isEmpty() ? sKey : sParent + "/" + sKey );
	};
	const auto driver = [ &add ]( const char* sSubtree,
								  std::initializer_list<QString> keys ) {
		const QString sParent =
			QString( K::AudioEngine ) + "/" + QString( sSubtree );
		for ( const auto& sKey : keys ) {
			add( sParent, sKey );
		}
	};

	add( "", K::RecentUsedSongs );
	add( K::AudioEngine, K::AudioDriver );
	add( K::AudioEngine, K::BufferSize );
	add( K::AudioEngine, K::SampleRate );
	driver( K::MidiDriver,
			{ K::MidiDriverName, K::MidiPortName, K::MidiOutputPortName } );
	driver( K::OssDriver, { "ossDevice" } );
	driver( K::PortAudioDriver,
			{ "portAudioDevice", "portAudioHostAPI", "latencyTarget" } );
	driver( K::CoreAudioDriver, { "coreAudioDevice" } );
	driver( K::AlsaAudioDriver, { "alsa_audio_device" } );
	driver( K::JackDriver,
			{ "jack_port_name_1", "jack_port_name_2", "jack_transport_mode",
			  "jack_timebase_enabled", "jack_transport_mode_master",
			  "jack_connect_defaults", "jack_track_output_mode",
			  "jack_track_outs", "jack_enforce_instrument_name" } );
	driver( K::OscConfiguration,
			{ "oscServerPort", "oscEnabled", "oscFeedbackEnabled" } );
	add( K::Files, K::LastSongFilename );
	add( K::Files, K::LastPlaylistFilename );

	std::set<QString> actual;
	for ( int ii = 0; ii < PreferencesSchema::kSchemaRowCount; ++ii ) {
		const auto& row = PreferencesSchema::kSchemaRows[ ii ];
		if ( row.layer != PreferencesSchema::Layer::Override ) {
			continue;
		}
		QString sParent;
		for ( int jj = 0; jj < 3 && row.path[ jj ] != nullptr; ++jj ) {
			if ( jj > 0 ) {
				sParent += "/";
			}
			sParent += QString( row.path[ jj ] );
		}
		actual.insert(
			sParent.isEmpty() ? QString( row.key )
							  : sParent + "/" + QString( row.key ) );
	}

	if ( actual != expected ) {
		// Name the drift instead of leaving a bare count mismatch.
		QString sMissing, sExtra;
		for ( const auto& s : expected ) {
			if ( actual.count( s ) == 0 ) {
				sMissing += s + " ";
			}
		}
		for ( const auto& s : actual ) {
			if ( expected.count( s ) == 0 ) {
				sExtra += s + " ";
			}
		}
		CPPUNIT_ASSERT_MESSAGE(
			QString( "Override layer drifted.\nmissing: [%1]\nextra: [%2]" )
				.arg( sMissing, sExtra )
				.toStdString(),
			false );
	}
	CPPUNIT_ASSERT_EQUAL( static_cast<int>( expected.size() ),
						  static_cast<int>( actual.size() ) );

	___INFOLOG( "passed" );
}
