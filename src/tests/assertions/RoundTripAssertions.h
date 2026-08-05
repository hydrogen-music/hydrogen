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
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#ifndef ROUND_TRIP_ASSERTIONS_H
#define ROUND_TRIP_ASSERTIONS_H

#include <core/Basics/Adsr.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Playlist.h>
#include <core/Basics/Song.h>
#include <core/Preferences/Preferences.h>

#include <QString>
#include <memory>

/**
 * \ingroup tests
 *
 * Field-by-field comparison helpers for IPC round-trip tests (ADR 0033).
 *
 * Each method compares every serializable member of a core type. Members that
 * are intentionally not serialized (m_sPath, m_bIsModified, m_bIsEdited,
 * m_context, runtime audio/MIDI state) are excluded — any other member that
 * fails to round-trip is a bug.
 *
 * When adding or deleting a member in a Basics class, update the
 * corresponding assert method here so the round-trip test catches it.
 * See AGENTS.md "IPC round-trip tests for Basics members".
 */
class RoundTripAssertions {
public:
	// ── Top-level types ──────────────────────────────────────────────
 static void assertSongEqual(
	 std::shared_ptr<H2Core::Song> a,
	 std::shared_ptr<H2Core::Song> b
 );
 static void assertDrumkitEqual(
	 std::shared_ptr<H2Core::Drumkit> a,
	 std::shared_ptr<H2Core::Drumkit> b
 );
 static void assertInstrumentEqual(
	 std::shared_ptr<H2Core::Instrument> a,
	 std::shared_ptr<H2Core::Instrument> b
 );
 static void assertPatternEqual(
	 std::shared_ptr<H2Core::Pattern> a,
	 std::shared_ptr<H2Core::Pattern> b
 );
 static void assertPlaylistEqual(
	 std::shared_ptr<H2Core::Playlist> a,
	 std::shared_ptr<H2Core::Playlist> b
 );
 static void assertPlaylistEntryEqual(
	 std::shared_ptr<H2Core::PlaylistEntry> a,
	 std::shared_ptr<H2Core::PlaylistEntry> b
 );
 static void assertCorePreferencesEqual(
	 std::shared_ptr<H2Core::Preferences> a,
	 std::shared_ptr<H2Core::Preferences> b
 );

 // ── Sub-object types ─────────────────────────────────────────────
 static void assertAdsrEqual(
	 std::shared_ptr<H2Core::ADSR> a,
	 std::shared_ptr<H2Core::ADSR> b
 );
 static void assertNoteEqual(
	 std::shared_ptr<H2Core::Note> a,
	 std::shared_ptr<H2Core::Note> b
 );
 static void assertComponentEqual(
	 std::shared_ptr<H2Core::InstrumentComponent> a,
	 std::shared_ptr<H2Core::InstrumentComponent> b
 );
 static void assertLayerEqual(
	 std::shared_ptr<H2Core::InstrumentLayer> a,
	 std::shared_ptr<H2Core::InstrumentLayer> b
 );
 static void
 assertLicenseEqual( const H2Core::License& a, const H2Core::License& b );
 static void assertAutomationPathEqual(
	 std::shared_ptr<H2Core::AutomationPath> a,
	 std::shared_ptr<H2Core::AutomationPath> b
 );

private:
 static void
 assertStringEqual( const QString& sLabel, const QString& a, const QString& b );
 static void assertFloatEqual( const QString& sLabel, float a, float b );
 static void assertIntEqual( const QString& sLabel, int a, int b );
 static void
 assertUuidEqual( const QString& sLabel, H2Core::Uuid a, H2Core::Uuid b );
 static void
 assertLongLongEqual( const QString& sLabel, long long a, long long b );
 static void assertBoolEqual( const QString& sLabel, bool a, bool b );
};

#endif // ROUND_TRIP_ASSERTIONS_H
