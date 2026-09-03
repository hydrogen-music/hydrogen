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

#ifndef H2_HYDROGEN_PLUGIN_H
#define H2_HYDROGEN_PLUGIN_H

#include <cstdint>
#include <memory>
#include <vector>

#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace H2Core {

class Hydrogen;
class PluginAudioDriver;
class PluginMidiDriver;
class EngineSession;

/**
 * Format-agnostic plugin engine wrapper (ADR 0013/0014).
 *
 * Owns one headless Hydrogen instance driven by the host-driven Plugin audio
 * and MIDI drivers, and exposes the lifecycle a plugin host needs: activate /
 * process (host buffers + transport + MIDI) / deactivate, plus state save/load
 * through the `.h2project` codec (ADR 0017/0020/0025). The native CLAP and LV2
 * entry points are thin C-ABI shims over this class; keeping the real logic
 * here means one implementation serves every format and stays unit-testable
 * without any plugin SDK.
 *
 * \ingroup docCore
 */
class HydrogenPlugin {
public:
	/**
	 * @param fSampleRate  Initial sample rate in Hz.
	 * @param nMaxBlockSize Largest block the host will request (frames).
	 * @param nBuses       Number of stereo output buses (ADR 0019); 0 disables
	 *                     per-instrument bus routing (master only).
	 */
	HydrogenPlugin( double fSampleRate, unsigned nMaxBlockSize, int nBuses );
	~HydrogenPlugin();

	/** Update the sample rate / max block size before processing starts. */
	void activate( double fSampleRate, unsigned nMaxBlockSize );
	void deactivate();

	/**
	 * Render one block into the host buffers.
	 * @param nFrames   Frames to render (<= max block size).
	 * @param pMasterL  Master left output (host owned, >= nFrames).
	 * @param pMasterR  Master right output.
	 * @param busOut_L  Per-bus left buffers (size getBusCount() or empty).
	 * @param busOut_R  Per-bus right buffers.
	 * @param bRolling  Host transport rolling this block.
	 * @param fBpm      Host tempo (<= 0 keeps the engine tempo).
	 * @param nFrame    Host transport frame at the start of the block.
	 */
	void process( uint32_t nFrames, float* pMasterL, float* pMasterR,
				  const std::vector<float*>& busOut_L,
				  const std::vector<float*>& busOut_R,
				  bool bRolling, double fBpm, long long nFrame );

	// Host MIDI for the upcoming block (queued, dispatched at process()).
	void noteOn( int nKey, int nVelocity, int nChannel, int nSampleOffset = 0 );
	void noteOff( int nKey, int nChannel, int nSampleOffset = 0 );
	void controlChange( int nParameter, int nValue, int nChannel,
						int nSampleOffset = 0 );

	int getBusCount() const;

	/** Serialize the current song to a plugin-state buffer (ADR 0017/0020/0025).
	 * @param bEmbedSamples ON → portable `.h2project`; OFF → song-only. */
	std::vector<unsigned char> saveState( bool bEmbedSamples );
	/** Replace the current song from a plugin-state buffer (embedded or
	 * song-only, auto-detected). */
	bool loadState( const std::vector<unsigned char>& data );

	Hydrogen* getHydrogen() const { return m_pHydrogen; }

	// ── Out-of-process editor lifecycle (ADR 0016) ─────────────────
	/** Open the editor for this instance: start serving the engine over IPC
	 * (#EngineSession) and, when @a bLaunchProcess, spawn the editor process
	 * `hydrogen --connect-via-ipc <endpoint>`. Idempotent; returns false only if the
	 * serve loop could not bind. @a bLaunchProcess == false starts serving without
	 * spawning (used by tests, which act as the editor themselves). */
	bool openEditor( bool bLaunchProcess = true );
	/** Close the editor: terminate the process (if any) and stop serving. Called
	 * automatically on destruction. */
	void closeEditor();
	bool isEditorOpen() const { return m_bEditorOpen; }
	/** Whether the editor *process* is currently alive. Polled (no event loop
	 * needed), so a host idle callback can detect the user closing the window
	 * (e.g. the LV2 ui:idleInterface). False when no process was launched. */
	bool isEditorProcessRunning() const;
	/** The IPC endpoint the editor attaches to (empty while closed). */
	const QString& getEditorEndpoint() const { return m_sEditorEndpoint; }
	/** Override the editor binary outright (highest precedence). */
	void setEditorBinary( const QString& sPath ) { m_sEditorBinary = sPath; }
	/** Directory to look in for the bundled editor binary — the plugin's own
	 * install location, supplied by the format shim (CLAP `init(plugin_path)`,
	 * LV2 `instantiate(bundle_path)`). */
	void setEditorSearchDir( const QString& sDir ) { m_sEditorSearchDir = sDir; }

	/** Resolve the editor binary to launch, in precedence order: @a sExplicit
	 * override → `$HYDROGEN_EDITOR_PATH` → a `hydrogen` executable found relative
	 * to @a sSearchDir (the installed plugin location) → `hydrogen` on `PATH`.
	 * Static + pure (aside from the env var) so it is unit-testable. */
	static QString resolveEditorBinary( const QString& sExplicit,
										const QString& sSearchDir );

private:
	QString editorBinary() const;
	void launchEditorProcess();
	/** Process-exit handler. Respawns only on a crash (bounded); a clean exit
	 * (the user closed the editor window) leaves it closed. */
	void onEditorProcessFinished( bool bCrashed );

	Hydrogen* m_pHydrogen;
	std::shared_ptr<PluginAudioDriver> m_pAudioDriver;
	std::shared_ptr<PluginMidiDriver> m_pMidiDriver;
	int m_nBuses;

	/** Engine-side serve loop for the attached editor; owned. */
	std::unique_ptr<EngineSession> m_pEditorSession;
	/** The spawned editor process; owned (null when not launched). */
	std::unique_ptr<QProcess> m_pEditorProcess;
	QString m_sEditorEndpoint;
	QString m_sEditorBinary;
	QString m_sEditorSearchDir;
	bool m_bEditorOpen = false;
	/** Set during closeEditor() so the process-exit handler does not respawn. */
	bool m_bEditorClosing = false;
	/** Bounded auto-respawn count, so a persistently crashing editor can't loop
	 * forever (ADR 0016 respawn-on-crash). */
	int m_nEditorRespawns = 0;
	static constexpr int knMaxEditorRespawns = 3;
};

};

#endif
