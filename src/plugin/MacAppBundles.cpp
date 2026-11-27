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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include <plugin/MacAppBundles.h>

#ifdef __APPLE__

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

// AssertMacros.h (pulled in by CoreServices) defines lowercase macros like
// `check` and `verify` unless told otherwise; they would clash with
// identifiers in Qt and our own headers.
#ifndef __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES
#define __ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES 0
#endif
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

namespace H2Core {
namespace MacAppBundles {

namespace {
QString stringValue( CFDictionaryRef dict, const char* sKey )
{
	CFStringRef key = CFStringCreateWithCString( kCFAllocatorDefault, sKey,
												 kCFStringEncodingUTF8 );
	if ( key == nullptr ) {
		return QString();
	}
	QString sResult;
	const void* pValue = CFDictionaryGetValue( dict, key );
	if ( pValue != nullptr && CFGetTypeID( pValue ) == CFStringGetTypeID() ) {
		sResult = QString::fromCFString( static_cast<CFStringRef>( pValue ) );
	}
	CFRelease( key );
	return sResult;
}
}  // namespace

std::optional<Info> read( const QString& sBundlePath )
{
	// The property list is parsed directly rather than through CFBundle:
	// CFBundle caches bundles per process, which would hide an app that
	// was updated in place while the host keeps running.
	QFile file( QDir( sBundlePath ).filePath( "Contents/Info.plist" ) );
	if ( ! file.open( QIODevice::ReadOnly ) ) {
		return std::nullopt;
	}
	const QByteArray bytes = file.readAll();
	CFDataRef data = CFDataCreate(
		kCFAllocatorDefault, reinterpret_cast<const UInt8*>( bytes.constData() ),
		bytes.size() );
	if ( data == nullptr ) {
		return std::nullopt;
	}
	CFPropertyListRef plist = CFPropertyListCreateWithData(
		kCFAllocatorDefault, data, kCFPropertyListImmutable, nullptr, nullptr );
	CFRelease( data );
	if ( plist == nullptr ) {
		return std::nullopt;
	}
	if ( CFGetTypeID( plist ) != CFDictionaryGetTypeID() ) {
		CFRelease( plist );
		return std::nullopt;
	}
	const auto dict = static_cast<CFDictionaryRef>( plist );
	const QString sExecutableName = stringValue( dict, "CFBundleExecutable" );
	const QString sVersion = stringValue( dict, "CFBundleShortVersionString" );
	CFRelease( plist );

	// A name with a path separator would point outside the bundle.
	if ( sExecutableName.isEmpty() || sExecutableName.contains( '/' ) ) {
		return std::nullopt;
	}
	const QFileInfo executable(
		QDir( sBundlePath ).filePath( "Contents/MacOS/" + sExecutableName ) );
	if ( ! executable.isFile() || ! executable.isExecutable() ) {
		return std::nullopt;
	}
	return Info{ executable.absoluteFilePath(), sVersion };
}

QStringList installed()
{
	QStringList bundles;
	const auto add = [ &bundles ]( const QString& sPath ) {
		const QFileInfo info( sPath );
		if ( ! info.isDir() ) {
			return;
		}
		const QString sAbsolute = info.absoluteFilePath();
		if ( ! bundles.contains( sAbsolute ) ) {
			bundles << sAbsolute;
		}
	};

	add( QStringLiteral( "/Applications/Hydrogen.app" ) );
	add( QDir::homePath() + QStringLiteral( "/Applications/Hydrogen.app" ) );

	CFStringRef identifier = CFStringCreateWithCString(
		kCFAllocatorDefault, sBundleIdentifier, kCFStringEncodingUTF8 );
	if ( identifier == nullptr ) {
		return bundles;
	}
	CFArrayRef urls =
		LSCopyApplicationURLsForBundleIdentifier( identifier, nullptr );
	CFRelease( identifier );
	if ( urls == nullptr ) {
		// Not an error: no copy is registered with LaunchServices.
		return bundles;
	}
	for ( CFIndex ii = 0; ii < CFArrayGetCount( urls ); ++ii ) {
		const auto url =
			static_cast<CFURLRef>( CFArrayGetValueAtIndex( urls, ii ) );
		CFStringRef path = CFURLCopyFileSystemPath( url, kCFURLPOSIXPathStyle );
		if ( path != nullptr ) {
			add( QString::fromCFString( path ) );
			CFRelease( path );
		}
	}
	CFRelease( urls );
	return bundles;
}

}  // namespace MacAppBundles
}  // namespace H2Core

#endif  // __APPLE__
