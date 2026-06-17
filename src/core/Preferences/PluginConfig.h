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

#ifndef H2C_PLUGIN_CONFIG_H
#define H2C_PLUGIN_CONFIG_H

#include <core/Object.h>

#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QString>

namespace H2Core {

/**
 * Layered plugin configuration (ADR 0022) and concurrency-safe persistence of
 * the shared user config (ADR 0023), operating on the Preferences XML. The
 * config path is supplied by the caller (`Filesystem::userConfigPath()`) - it is
 * XDG-derived on Linux ($XDG_CONFIG_HOME/hydrogen/hydrogen.conf, with ~/.hydrogen
 * only as a legacy fallback) and platform-specific on macOS/Windows.
 *
 * A plugin instance composes its Preferences as **base ← override**:
 *  - the *base layer* is the shared config (theme, shortcuts, language, GUI
 *    layout, MIDI action mappings, …);
 *  - the *override layer* is a host/state-owned subset (audio/MIDI I/O, sample
 *    rate, buffer size, JACK/OSC, recent/last-file) that supersedes the base and
 *    is **never written back** to the shared config.
 *
 * The override field set defined here is the single source of truth for both
 * what the override layer supplies (applyOverride) and what is excluded from a
 * shared-config write (mergeForWrite / persist).
 *
 * Persistence is a per-field 3-way merge under a cross-process lock: re-read the
 * current on-disk config, apply only the base-layer fields the user changed this
 * session (current ≠ baseline), leave every other field as the freshly-read disk
 * state (so concurrent edits from other instances survive), and write atomically.
 * Change tracking is diff-against-baseline — no dirty-set, no setter
 * instrumentation.
 *
 * \ingroup docCore
 */
class PluginConfig : public H2Core::Object<PluginConfig> {
	H2_OBJECT(PluginConfig)
public:
	/** \return true if the given Preferences XML leaf/subtree path belongs to
	 * the host/state-owned override layer (and so must not be persisted to the
	 * shared config). Paths are slash-joined element names from the document
	 * root, e.g. "hydrogen_preferences/audio_engine/samplerate". */
	static bool isOverridePath( const QString& sPath );

	/**
	 * Compose the base layer with override values (ADR 0022). Returns @a baseXml
	 * with each override path set to the supplied host/state value.
	 * @param overrideValues path → value for override-layer fields.
	 */
	static QByteArray applyOverride( const QByteArray& baseXml,
									 const QMap<QString, QString>& overrideValues );

	/**
	 * Field-level 3-way merge for a shared-config write (ADR 0023), in memory.
	 * Starts from @a diskXml; for every base-layer field where @a currentXml
	 * differs from @a baselineXml (the user changed it this session) the current
	 * value is applied; override fields are never written; all other fields keep
	 * their freshly-read disk value. Returns the merged XML.
	 */
	static QByteArray mergeForWrite( const QByteArray& diskXml,
									 const QByteArray& baselineXml,
									 const QByteArray& currentXml );

	/**
	 * Concurrency-safe persist of base-layer changes to @a sPath (ADR 0023):
	 * acquire an exclusive cross-process lock (QLockFile), re-read the on-disk
	 * config, mergeForWrite() the changes onto it, and write atomically
	 * (QSaveFile). Safe under parallel teardown by multiple instances.
	 * @param baselineXml the config this instance loaded.
	 * @param currentXml  this instance's current config.
	 * @param pMergedOut  if non-null, receives the merged XML written to disk
	 *                    (the caller's new baseline).
	 * @return true on success.
	 */
	static bool persist( const QString& sPath, const QByteArray& baselineXml,
						 const QByteArray& currentXml,
						 QByteArray* pMergedOut = nullptr );
};

};

#endif
