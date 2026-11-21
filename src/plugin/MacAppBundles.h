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

#ifndef H2C_MAC_APP_BUNDLES_H
#define H2C_MAC_APP_BUNDLES_H

#ifdef __APPLE__

#include <optional>

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace H2Core {

/** Access to Hydrogen application bundles on macOS (ADR 0038). Kept in a
 * translation unit of its own so the CoreFoundation/LaunchServices headers
 * do not leak into the format-agnostic plugin code. */
namespace MacAppBundles {

/** The bundle identifier the Hydrogen application is registered with. */
constexpr const char* sBundleIdentifier = "org.hydrogenmusic.Hydrogen";

struct Info {
	/** Absolute path of the bundle's executable (`CFBundleExecutable`
	 * inside `Contents/MacOS`). It is only set if the file exists and is
	 * executable. */
	QString sExecutable;
	/** `CFBundleShortVersionString`; empty if missing. */
	QString sVersion;
};

/** Reads @a sBundlePath's `Contents/Info.plist` (XML or binary). Returns
 * std::nullopt if there is no readable property list or it names no
 * launchable executable. */
std::optional<Info> read( const QString& sBundlePath );

/** Hydrogen application bundles on this system, without duplicates and
 * in this order: `/Applications/Hydrogen.app`, `~/Applications/Hydrogen.app`
 * (if present), then every further copy LaunchServices knows for
 * #sBundleIdentifier. */
QStringList installed();

}  // namespace MacAppBundles
}  // namespace H2Core

#endif  // __APPLE__

#endif  // H2C_MAC_APP_BUNDLES_H
