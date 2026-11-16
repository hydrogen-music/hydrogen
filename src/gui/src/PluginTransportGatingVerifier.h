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
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#ifndef H2C_PLUGIN_TRANSPORT_GATING_VERIFIER_H
#define H2C_PLUGIN_TRANSPORT_GATING_VERIFIER_H

#include <QObject>

/**
 * \ingroup docGUI
 *
 * PluginTransportGatingVerifier (ADR 0026)
 *
 * Out-of-process check for the plugin-host transport gating: under a
 * plugin-host engine the host owns transport, so the editor's transport
 * buttons (MainToolBar play/stop/rewind/ffwd, PlaylistEditor
 * rewind/stop/ffwd) must be deactivated, while record stays available and
 * the PlaylistEditor play button remains active as the song selector (it
 * selects without starting playback).
 *
 * The harness (plugin_editor_smoke --verify-transport-gating) spawns the
 * GUI with --verify-plugin-transport-gating against a HydrogenPlugin
 * engine; once the GUI is up, this verifier inspects the button states and
 * quits the application with the verdict (0 = gating correct,
 * 1 = mismatch). Like EditorPathExerciser it turns a GUI-state assertion
 * into an exit code a CTest can fail on.
 */
class PluginTransportGatingVerifier : public QObject {
	Q_OBJECT

public:
	explicit PluginTransportGatingVerifier( QObject* pParent = nullptr );
	~PluginTransportGatingVerifier() override = default;

	/** Inspect the widgets and quit with the verdict. Scheduled via
	 * QTimer from main() so the startup event queue (including the
	 * post-sync UpdateSong events) drains first. */
	void start();

private:
	/** Assert-by-exit-code helpers for the two widgets. */
	bool verifyMainToolBar();
	bool verifyPlaylistEditor();
};

#endif // H2C_PLUGIN_TRANSPORT_GATING_VERIFIER_H
