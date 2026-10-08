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
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see https://www.gnu.org/licenses
 *
 */

#ifndef PLUGIN_UI_MIXER_MODEL_H
#define PLUGIN_UI_MIXER_MODEL_H

#include <memory>
#include <mutex>

#include <QString>

#include <core/IPC/EngineTelemetry.h>

namespace H2Core {

class CoreActionController;
class HydrogenPlugin;
class Instrument;

/** Headless state + command model behind the mixer view of the plugin
 * UI (#PluginUiWindow).
 *
 * Strip and master controls forward to one CoreActionController command
 * each — the same commands the GUI mixer's undo actions route through —
 * so the engine validates, applies, and emits the parameter events.
 * Strip selection is not driven: the plugin mixer has no selected-strip
 * concept, so every command passes #bSelectStrip = false.
 *
 * Meter values come from the engine's telemetry snapshots (ADR 0018):
 * #acceptMeterSnapshot() is the seam the window's engine-event-sink
 * adapter calls (bridge thread, #EngineEventSink threading contract),
 * the peak reads serve the UI thread. */
class PluginUiMixerModel
{
public:
	explicit PluginUiMixerModel( HydrogenPlugin* pPlugin );
	~PluginUiMixerModel();

	PluginUiMixerModel( const PluginUiMixerModel& ) = delete;
	PluginUiMixerModel& operator=( const PluginUiMixerModel& ) = delete;

	bool setStripVolume( int nStrip, float fVolume );
	/** Symmetric pan in [-1;1]. */
	bool setStripPan( int nStrip, float fPan );
	bool setStripIsMuted( int nStrip, bool bMuted );
	bool setStripIsSoloed( int nStrip, bool bSoloed );
	bool setMasterVolume( float fVolume );
	bool setMasterIsMuted( bool bMuted );

	/** Bus routing (ADR 0019): sets the instrument's explicit output
	 * bus; a negative @a nBus resets it to the implicit 1-to-1 default
	 * (bus = position in the kit). */
	bool setInstrumentOutputBus( int nInstrument, int nBus );
	/** The instrument's stored output bus, or -1 for the implicit 1-to-1
	 * default (0 when the engine is unavailable). */
	int instrumentOutputBus( int nInstrument ) const;
	/** The bus the instrument actually feeds: the stored custom mapping
	 * if set, else the implicit 1-to-1 by kit position; -1 when the
	 * instrument gets no individual bus (beyond @a nBusCount — master
	 * only). */
	int effectiveOutputBus( int nInstrument, int nBusCount ) const;

	// The read side for populating the strips. Each returns the
	// engine's current value, or the documented degenerate value when
	// the engine is unavailable.
	/** Number of strips — the current kit's instrument count (0 when
	 *  the engine is unavailable). */
	int stripCount() const;
	/** The strip's instrument name (empty when the engine is
	 *  unavailable or @a nStrip is out of range). */
	QString stripName( int nStrip ) const;
	/** The strip's volume (0.0 when unavailable or out of range). */
	float stripVolume( int nStrip ) const;
	/** The strip's symmetric pan in [-1;1] (0.0 when unavailable or out
	 *  of range). */
	float stripPan( int nStrip ) const;
	bool stripIsMuted( int nStrip ) const;
	bool stripIsSoloed( int nStrip ) const;
	/** The master volume (0.0 when the engine is unavailable). */
	float masterVolume() const;
	bool masterIsMuted() const;
	/** The host-provided bus count (ADR 0019; 0 when the plugin is
	 *  unavailable). */
	int busCount() const;

	/** Latest telemetry snapshot, as forwarded by the engine event
	 * sink. Cheap and non-blocking (EngineEventSink contract). */
	void acceptMeterSnapshot( const EngineTelemetrySnapshot& snapshot );

	bool hasMeterSnapshot() const;
	/** Peak of instrument @a nInstrument (0.0 when no snapshot covers
	 * it). */
	float instrumentPeakL( int nInstrument ) const;
	float instrumentPeakR( int nInstrument ) const;
	float masterPeakL() const;
	float masterPeakR() const;

private:
	std::shared_ptr<CoreActionController> controller() const;
	/** The strip's instrument in the plugin engine's current kit
	 * (nullptr when the engine is unavailable or @a nStrip is out of
	 * range). */
	std::shared_ptr<Instrument> instrument( int nStrip ) const;

	HydrogenPlugin* m_pPlugin;

	// Latest snapshot under a small mutex: the sink callback fires on
	// the engine session's bridge thread, the reads on the UI thread.
	mutable std::mutex m_mutex;
	bool m_bHasSnapshot = false;
	EngineTelemetrySnapshot m_snapshot;
};

} // namespace H2Core

#endif
