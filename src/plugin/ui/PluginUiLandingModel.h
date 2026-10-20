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

// The landing view's model (proposal 0006 TU4.1): the load/install commands
// behind the landing view's buttons, the drumkit listing feeding the picker,
// and the editor button + executable override — all through the plugin's
// engine commands (ADR 0022's plugin-instance layer). Pure logic, no imgui
// types (TU1.3): the renderers only forward widget state in and results out.

#ifndef H2C_PLUGIN_UI_LANDING_MODEL_H
#define H2C_PLUGIN_UI_LANDING_MODEL_H

#include <QString>
#include <mutex>
#include <vector>

#include <core/IPC/EngineEventSink.h>

namespace H2Core {

class HydrogenPlugin;

class PluginUiLandingModel {
public:
	explicit PluginUiLandingModel( HydrogenPlugin* pPlugin );
	~PluginUiLandingModel();

	PluginUiLandingModel( const PluginUiLandingModel& ) = delete;
	PluginUiLandingModel& operator=( const PluginUiLandingModel& ) = delete;

	/** Loads the .h2song at @a sPath and installs it as the engine's
	 * current song (loadSong + setSong). False without touching the
	 * current song when the path does not load. */
	bool loadSong( const QString& sPath );
	/** Loads the .h2pattern at @a sPath and installs it at
	 * @a nPatternNumber of the current song (loadPattern +
	 * setPattern). False without touching the song when the path does
	 * not load. */
	bool loadPattern( const QString& sPath, int nPatternNumber,
					  bool bReplace );
	/** Loads the drumkit folder at @a sPath and installs it into the
	 * current song (Drumkit::load + setDrumkit). False without touching
	 * the song when the path does not load. */
	bool loadDrumkit( const QString& sPath );
	/** One row of the drumkit picker: the metadata shown in the table
	 * (name, author, license, version) and on hover (info, HTML tags
	 * stripped for plain-text display), plus the path that identifies
	 * the kit (its canonical drumkit.xml path) — the picker's
	 * selection key for loadDrumkit(). */
	struct DrumkitSummary {
		QString sName;
		QString sAuthor;
		QString sLicense;
		QString sInfo;
		int nVersion = 0;
		QString sPath;
	};
	/** All drumkits known to the sound library database, sorted by
	 * name (case-insensitive). Empty without a plugin or before the
	 * database's initial scan finished (waitForInitialScan). */
	std::vector<DrumkitSummary> drumkits() const;
	/** The info section's data: the latest #EngineInfoSnapshot the
	 * engine session's publish cycle delivered (via
	 * #acceptInfoSnapshot), or the idle defaults before the first
	 * delivery — a pure cache read, the model never touches the
	 * engine here (ADR 0035 UI-3). */
	EngineInfoSnapshot infoSnapshot() const;
	/** Caches the latest publish-cycle info snapshot (the bridge
	 * thread's call, via the window's engine-event-sink adapter —
	 * EngineEventSink's threading contract). */
	void acceptInfoSnapshot( const EngineInfoSnapshot& snapshot );
	/** Whether the cached info snapshot moved since the last poll —
	 * the window's cue that the info section's displayed surface went
	 * stale and a frame is owed. UI thread only: the poll latches the
	 * cache into the displayed snapshot. */
	bool pollInfoChanged();
	/** The picker's info conversion: rich text (HTML, as drumkit.xml's
	 * <info> element stores it) to plain text — a pure-QtCore
	 * reimplementation of the GUI's QtGui path
	 * (QTextDocumentFragment::fromHtml( ... ).toPlainText(),
	 * InfoView's tooltip), which the plugin UI cannot link: tags
	 * dropped, entities decoded, block boundaries as newlines,
	 * whitespace collapsed per white-space mode. Pinned case-by-case
	 * by the reference battery in PluginUiModelTest. */
	static QString htmlToPlainText( const QString& sHtml );
	/** The editor binary that would be launched right now: the
	 * explicit override if set, else the resolved default. Empty
	 * without a plugin. */
	QString editorExecutable() const;
	/** Overrides the editor binary outright (highest precedence); an
	 * empty @a sPath restores the default resolution. False without a
	 * plugin. */
	bool setEditorExecutable( const QString& sPath );
	/** Opens the external editor — the landing view's button owns this
	 * (UI-5 retires the hosts' direct `show` path). */
	bool openEditor( bool bLaunchProcess = true );

private:
	HydrogenPlugin* m_pPlugin = nullptr;

	// The latest delivered snapshot under a small mutex: the sink
	// callback fires on the engine session's bridge thread, the reads
	// on the UI thread.
	mutable std::mutex m_mutex;
	EngineInfoSnapshot m_infoCache;
	// What the last rendered frame showed — the poll's diff base (UI
	// thread only).
	EngineInfoSnapshot m_infoDisplayed;
};

} // namespace H2Core

#endif // H2C_PLUGIN_UI_LANDING_MODEL_H
