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

#include <core/Preferences/PluginConfig.h>

#include <core/Preferences/PreferencesKeys.h>

#include <QtCore/QFile>
#include <QtCore/QLockFile>
#include <QtCore/QSaveFile>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <initializer_list>

namespace H2Core {

namespace {
namespace K = PreferencesKeys;

// Join element names into a document path, e.g. {Root, AudioEngine, SampleRate}
// → "hydrogen_preferences/audio_engine/samplerate". Built from PreferencesKeys
// so these paths cannot drift from what Preferences actually writes.
QString keyPath( std::initializer_list<const char*> parts ) {
	QString out;
	for ( const char* part : parts ) {
		if ( ! out.isEmpty() ) {
			out += "/";
		}
		out += QLatin1String( part );
	}
	return out;
}
} // namespace

// ── Override layer: the single source of truth for layer membership (ADR 0022).
// Host/state-owned fields that supersede the base layer and are never written
// back to the shared config. The element names come from PreferencesKeys, so
// they stay in lock-step with Preferences' reader/writer.
static const QStringList& overrideExactPaths() {
	static const QStringList paths = {
		keyPath( { K::Root, K::AudioEngine, K::AudioDriver } ),
		keyPath( { K::Root, K::AudioEngine, K::BufferSize } ),
		keyPath( { K::Root, K::AudioEngine, K::SampleRate } ),
		keyPath( { K::Root, K::AudioEngine, K::MidiDriver, K::MidiDriverName } ),
		keyPath( { K::Root, K::AudioEngine, K::MidiDriver, K::MidiPortName } ),
		keyPath( { K::Root, K::AudioEngine, K::MidiDriver, K::MidiOutputPortName } ),
		keyPath( { K::Root, K::Files, K::LastSongFilename } ),
		keyPath( { K::Root, K::Files, K::LastPlaylistFilename } )
	};
	return paths;
}
// Whole host/state-owned subtrees (device configs, JACK I/O, OSC, recent list).
static const QStringList& overridePrefixes() {
	static const QStringList prefixes = {
		keyPath( { K::Root, K::AudioEngine, K::OssDriver } ),
		keyPath( { K::Root, K::AudioEngine, K::PortAudioDriver } ),
		keyPath( { K::Root, K::AudioEngine, K::CoreAudioDriver } ),
		keyPath( { K::Root, K::AudioEngine, K::AlsaAudioDriver } ),
		keyPath( { K::Root, K::AudioEngine, K::JackDriver } ),
		keyPath( { K::Root, K::AudioEngine, K::OscConfiguration } ),
		keyPath( { K::Root, K::RecentUsedSongs } )
	};
	return prefixes;
}

bool PluginConfig::isOverridePath( const QString& sPath ) {
	if ( overrideExactPaths().contains( sPath ) ) {
		return true;
	}
	for ( const auto& sPrefix : overridePrefixes() ) {
		if ( sPath == sPrefix || sPath.startsWith( sPrefix + "/" ) ) {
			return true;
		}
	}
	return false;
}

namespace {

// A node is an atomic "list unit" if its element children include a repeated tag
// (e.g. <recentUsedSongs><song/><song/></recentUsedSongs>) - such lists are
// merged whole rather than per child, which has no stable path key.
bool isListUnit( const QDomElement& el ) {
	QStringList seen;
	for ( QDomElement c = el.firstChildElement(); ! c.isNull();
		  c = c.nextSiblingElement() ) {
		if ( seen.contains( c.tagName() ) ) {
			return true;
		}
		seen.append( c.tagName() );
	}
	return false;
}

QString serializeChildren( const QDomElement& el ) {
	QString out;
	QTextStream ts( &out );
	for ( QDomNode n = el.firstChild(); ! n.isNull(); n = n.nextSibling() ) {
		n.save( ts, 0 );
	}
	return out;
}

// Flatten a document to path → value: scalar leaves map to their text, atomic
// list units to their serialized children, struct nodes are recursed into.
void collectUnits( const QDomElement& el, const QString& sPrefix,
				   QMap<QString, QString>& out ) {
	const QString sPath = sPrefix.isEmpty()
		? el.tagName() : sPrefix + "/" + el.tagName();
	const QDomElement firstChild = el.firstChildElement();
	if ( firstChild.isNull() ) {
		out.insert( sPath, el.text() );          // scalar leaf
		return;
	}
	if ( isListUnit( el ) ) {
		out.insert( sPath, serializeChildren( el ) ); // atomic list
		return;
	}
	for ( QDomElement c = firstChild; ! c.isNull(); c = c.nextSiblingElement() ) {
		collectUnits( c, sPath, out );           // struct: recurse
	}
}

QDomElement findByPath( const QDomDocument& doc, const QString& sPath ) {
	const QStringList parts = sPath.split( '/' );
	if ( parts.isEmpty() ) {
		return QDomElement();
	}
	QDomElement el = doc.documentElement();
	if ( el.isNull() || el.tagName() != parts[0] ) {
		return QDomElement();
	}
	for ( int i = 1; i < parts.size(); ++i ) {
		el = el.firstChildElement( parts[i] );
		if ( el.isNull() ) {
			return QDomElement();
		}
	}
	return el;
}

// Replace the full contents of dstEl with a deep copy of srcEl's contents.
// Works for both scalars (the text node) and list units (the child elements).
void replaceContents( QDomDocument& dstDoc, QDomElement& dstEl,
					  const QDomElement& srcEl ) {
	while ( ! dstEl.firstChild().isNull() ) {
		dstEl.removeChild( dstEl.firstChild() );
	}
	for ( QDomNode n = srcEl.firstChild(); ! n.isNull(); n = n.nextSibling() ) {
		dstEl.appendChild( dstDoc.importNode( n, true ) );
	}
}

} // namespace

QByteArray PluginConfig::applyOverride(
	const QByteArray& baseXml, const QMap<QString, QString>& overrideValues ) {
	QDomDocument doc;
	if ( ! doc.setContent( baseXml ) ) {
		___ERRORLOG( "Unable to parse base config XML" );
		return baseXml;
	}
	for ( auto it = overrideValues.constBegin(); it != overrideValues.constEnd();
		  ++it ) {
		QDomElement el = findByPath( doc, it.key() );
		if ( el.isNull() ) {
			continue;
		}
		while ( ! el.firstChild().isNull() ) {
			el.removeChild( el.firstChild() );
		}
		el.appendChild( doc.createTextNode( it.value() ) );
	}
	return doc.toByteArray();
}

QByteArray PluginConfig::mergeForWrite( const QByteArray& diskXml,
										const QByteArray& baselineXml,
										const QByteArray& currentXml ) {
	QDomDocument diskDoc;
	QDomDocument baselineDoc;
	QDomDocument currentDoc;
	if ( ! diskDoc.setContent( diskXml ) ||
		 ! baselineDoc.setContent( baselineXml ) ||
		 ! currentDoc.setContent( currentXml ) ) {
		___ERRORLOG( "Unable to parse config XML for merge" );
		return diskXml;
	}

	QMap<QString, QString> baselineMap;
	QMap<QString, QString> currentMap;
	collectUnits( baselineDoc.documentElement(), "", baselineMap );
	collectUnits( currentDoc.documentElement(), "", currentMap );

	for ( auto it = currentMap.constBegin(); it != currentMap.constEnd(); ++it ) {
		const QString& sPath = it.key();
		if ( isOverridePath( sPath ) ) {
			continue; // host/state-owned: never written to the shared config
		}
		// A base field is "changed" if it differs from our load baseline (or is
		// new). Unchanged fields are left as the freshly-read disk state so other
		// instances' concurrent edits survive.
		if ( baselineMap.contains( sPath ) &&
			 baselineMap.value( sPath ) == it.value() ) {
			continue;
		}
		const QDomElement srcEl = findByPath( currentDoc, sPath );
		QDomElement dstEl = findByPath( diskDoc, sPath );
		if ( srcEl.isNull() || dstEl.isNull() ) {
			continue; // field absent on disk (schema drift): skip
		}
		replaceContents( diskDoc, dstEl, srcEl );
	}

	return diskDoc.toByteArray();
}

bool PluginConfig::persist( const QString& sPath, const QByteArray& baselineXml,
							const QByteArray& currentXml, QByteArray* pMergedOut ) {
	// Exclusive cross-process lock for the whole read-merge-write (ADR 0023).
	QLockFile lock( sPath + ".lock" );
	lock.setStaleLockTime( 30000 );
	if ( ! lock.lock() ) {
		___ERRORLOG( QString( "Unable to lock config [%1]" ).arg( sPath ) );
		return false;
	}

	// Re-read the current on-disk state inside the lock.
	QByteArray diskXml = baselineXml;
	{
		QFile file( sPath );
		if ( file.open( QIODevice::ReadOnly ) ) {
			diskXml = file.readAll();
			file.close();
		}
	}

	const QByteArray merged = mergeForWrite( diskXml, baselineXml, currentXml );

	QSaveFile out( sPath );
	if ( ! out.open( QIODevice::WriteOnly ) ) {
		___ERRORLOG( QString( "Unable to open [%1] for writing" ).arg( sPath ) );
		lock.unlock();
		return false;
	}
	out.write( merged );
	const bool bOk = out.commit(); // atomic temp-file + rename
	lock.unlock();

	if ( bOk && pMergedOut != nullptr ) {
		*pMergedOut = merged;
	}
	return bOk;
}

};
