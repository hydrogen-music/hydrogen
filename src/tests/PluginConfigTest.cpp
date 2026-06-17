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

#include "PluginConfigTest.h"

#include <core/Object.h>
#include <core/Preferences/PluginConfig.h>
#include <core/Preferences/PreferencesKeys.h>

#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>
#include <QtXml/QDomDocument>

#include <initializer_list>

using namespace H2Core;

namespace {

namespace K = PreferencesKeys;

// Non-centralized base leaves used only by this test (not part of the shared
// PreferencesKeys set, which only covers structure + override fields).
const QString LANG = "preferredLanguage";
const QString GUI = "gui";
const QString GUI_X = "x";
const QString OSC_ENABLED = "oscEnabled";

// Join element names into a document path, sharing PreferencesKeys with the
// production code so the test cannot drift from the real schema.
QString path( std::initializer_list<QString> parts ) {
	QString out;
	for ( const auto& part : parts ) {
		if ( ! out.isEmpty() ) {
			out += "/";
		}
		out += part;
	}
	return out;
}

QString element( const QString& sTag, const QString& sValue ) {
	return QString( "<%1>%2</%1>" ).arg( sTag ).arg( sValue );
}

// A minimal Preferences-shaped config: base fields (language, gui/x) plus a few
// host/state-owned override fields (audio_engine + osc). Element names come from
// PreferencesKeys wherever the schema centralizes them.
QByteArray makeConfig( const QString& sLang, int nGuiX, int nSampleRate,
					   bool bOscEnabled ) {
	const QString osc = QString( "<%1>%2</%1>" )
		.arg( K::OscConfiguration )
		.arg( element( OSC_ENABLED, bOscEnabled ? "true" : "false" ) );
	const QString audioEngine = QString( "<%1>%2%3%4%5</%1>" )
		.arg( K::AudioEngine )
		.arg( element( K::SampleRate, QString::number( nSampleRate ) ) )
		.arg( element( K::BufferSize, "1024" ) )
		.arg( element( K::AudioDriver, "Auto" ) )
		.arg( osc );
	const QString root = QString( "<%1>%2%3%4</%1>" )
		.arg( K::Root )
		.arg( element( LANG, sLang ) )
		.arg( QString( "<%1>%2</%1>" ).arg( GUI ).arg( element( GUI_X, QString::number( nGuiX ) ) ) )
		.arg( audioEngine );
	return root.toUtf8();
}

QString leaf( const QByteArray& xml, const QString& sPath ) {
	QDomDocument doc;
	if ( ! doc.setContent( xml ) ) {
		return QString( "<parse-error>" );
	}
	const QStringList parts = sPath.split( '/' );
	QDomElement el = doc.documentElement();
	if ( el.tagName() != parts[0] ) {
		return QString( "<no-root>" );
	}
	for ( int i = 1; i < parts.size(); ++i ) {
		el = el.firstChildElement( parts[i] );
		if ( el.isNull() ) {
			return QString( "<missing>" );
		}
	}
	return el.text();
}

} // namespace

void PluginConfigTest::testOverrideMembership() {
	___INFOLOG( "" );

	// Host/state-owned fields are in the override layer.
	CPPUNIT_ASSERT( PluginConfig::isOverridePath(
		path( { K::Root, K::AudioEngine, K::SampleRate } ) ) );
	CPPUNIT_ASSERT( PluginConfig::isOverridePath(
		path( { K::Root, K::AudioEngine, K::BufferSize } ) ) );
	CPPUNIT_ASSERT( PluginConfig::isOverridePath(
		path( { K::Root, K::AudioEngine, K::AudioDriver } ) ) );
	CPPUNIT_ASSERT( PluginConfig::isOverridePath(
		path( { K::Root, K::AudioEngine, K::JackDriver, "jack_track_outs" } ) ) );
	CPPUNIT_ASSERT( PluginConfig::isOverridePath(
		path( { K::Root, K::AudioEngine, K::OscConfiguration, OSC_ENABLED } ) ) );
	CPPUNIT_ASSERT( PluginConfig::isOverridePath(
		path( { K::Root, K::RecentUsedSongs } ) ) );

	// Base (shared) fields are not.
	CPPUNIT_ASSERT( ! PluginConfig::isOverridePath(
		path( { K::Root, LANG } ) ) );
	CPPUNIT_ASSERT( ! PluginConfig::isOverridePath(
		path( { K::Root, GUI, GUI_X } ) ) );

	___INFOLOG( "passed" );
}

void PluginConfigTest::testLayering() {
	___INFOLOG( "" );

	// Base layer from the shared config; override values from host/state.
	const QByteArray base = makeConfig( "en", 10, 44100, false );
	QMap<QString, QString> overrideValues;
	overrideValues[ path( { K::Root, K::AudioEngine, K::SampleRate } ) ] = "48000";
	overrideValues[ path( { K::Root, K::AudioEngine, K::BufferSize } ) ] = "256";
	overrideValues[ path( { K::Root, K::AudioEngine, K::OscConfiguration,
							OSC_ENABLED } ) ] = "true";

	const QByteArray composed = PluginConfig::applyOverride( base, overrideValues );

	// Override fields take the host/state values ...
	CPPUNIT_ASSERT_EQUAL( std::string( "48000" ),
		leaf( composed, path( { K::Root, K::AudioEngine, K::SampleRate } ) ).toStdString() );
	CPPUNIT_ASSERT_EQUAL( std::string( "256" ),
		leaf( composed, path( { K::Root, K::AudioEngine, K::BufferSize } ) ).toStdString() );
	CPPUNIT_ASSERT_EQUAL( std::string( "true" ),
		leaf( composed,
			  path( { K::Root, K::AudioEngine, K::OscConfiguration, OSC_ENABLED } ) )
			.toStdString() );
	// ... while base fields keep the shared-config values.
	CPPUNIT_ASSERT_EQUAL( std::string( "en" ),
		leaf( composed, path( { K::Root, LANG } ) ).toStdString() );
	CPPUNIT_ASSERT_EQUAL( std::string( "10" ),
		leaf( composed, path( { K::Root, GUI, GUI_X } ) ).toStdString() );

	___INFOLOG( "passed" );
}

void PluginConfigTest::testBaseChangePersistedOverrideExcluded() {
	___INFOLOG( "" );

	const QByteArray disk = makeConfig( "en", 10, 44100, false );
	const QByteArray baseline = disk;

	// This session changed one base field (language) and one override field
	// (samplerate, a host value).
	const QByteArray current = makeConfig( "de", 10, 48000, false );

	const QByteArray merged =
		PluginConfig::mergeForWrite( disk, baseline, current );

	// The base change is written ...
	CPPUNIT_ASSERT_EQUAL( std::string( "de" ),
		leaf( merged, path( { K::Root, LANG } ) ).toStdString() );
	// ... but the override change is NOT (stays at the disk value).
	CPPUNIT_ASSERT_EQUAL( std::string( "44100" ),
		leaf( merged, path( { K::Root, K::AudioEngine, K::SampleRate } ) ).toStdString() );

	___INFOLOG( "passed" );
}

void PluginConfigTest::testSurvivesReload() {
	___INFOLOG( "" );

	QTemporaryDir tmp;
	CPPUNIT_ASSERT( tmp.isValid() );
	const QString sPath = tmp.path() + "/hydrogen.conf";

	const QByteArray baseline = makeConfig( "en", 10, 44100, false );
	{
		QFile f( sPath );
		CPPUNIT_ASSERT( f.open( QIODevice::WriteOnly ) );
		f.write( baseline );
		f.close();
	}

	// Change a base field this session and persist.
	const QByteArray current = makeConfig( "fr", 99, 44100, false );
	CPPUNIT_ASSERT( PluginConfig::persist( sPath, baseline, current ) );

	// Reload from disk: the base change survives.
	QByteArray reloaded;
	{
		QFile f( sPath );
		CPPUNIT_ASSERT( f.open( QIODevice::ReadOnly ) );
		reloaded = f.readAll();
		f.close();
	}
	CPPUNIT_ASSERT_EQUAL( std::string( "fr" ),
		leaf( reloaded, path( { K::Root, LANG } ) ).toStdString() );
	CPPUNIT_ASSERT_EQUAL( std::string( "99" ),
		leaf( reloaded, path( { K::Root, GUI, GUI_X } ) ).toStdString() );

	___INFOLOG( "passed" );
}
