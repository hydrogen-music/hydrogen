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

#include "PluginUiFrameGate.h"

namespace H2Core {

PluginUiFrameGate::PluginUiFrameGate(
	std::chrono::milliseconds nThresholdInterval )
	: m_nThresholdInterval( nThresholdInterval ) {
}

void PluginUiFrameGate::request() {
	m_bPendingRequest = true;
}

bool PluginUiFrameGate::shouldRender( std::chrono::milliseconds now ) const {
	if ( ! m_bPendingRequest ) {
		return false;
	}
	// The first frame is owed unconditionally — a mapped window must
	// not stay blank waiting for input or the interval.
	if ( ! m_bEverRendered ) {
		return true;
	}
	return now - m_nLastRenderTime >= m_nThresholdInterval;
}

void PluginUiFrameGate::markRendered( std::chrono::milliseconds now ) {
	m_bPendingRequest = false;
	m_bEverRendered = true;
	m_nLastRenderTime = now;
}

} // namespace H2Core
