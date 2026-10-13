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

#include "PluginUiMixerModel.h"

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>

#include <plugin/HydrogenPlugin.h>

namespace H2Core {

PluginUiMixerModel::PluginUiMixerModel( HydrogenPlugin* pPlugin )
	: m_pPlugin( pPlugin )
{
}

PluginUiMixerModel::~PluginUiMixerModel()
{
}

std::shared_ptr<CoreActionController> PluginUiMixerModel::controller() const
{
	return m_pPlugin != nullptr ? m_pPlugin->getHydrogen()->getCoreActionController()
								: nullptr;
}

bool PluginUiMixerModel::setStripVolume( int nStrip, float fVolume )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setStripVolume( nStrip, fVolume, /*bSelectStrip=*/false );
}

bool PluginUiMixerModel::setStripPan( int nStrip, float fPan )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setStripPanSym( nStrip, fPan, /*bSelectStrip=*/false );
}

bool PluginUiMixerModel::setStripIsMuted( int nStrip, bool bMuted )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setStripIsMuted( nStrip, bMuted, /*bSelectStrip=*/false );
}

bool PluginUiMixerModel::setStripIsSoloed( int nStrip, bool bSoloed )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setStripIsSoloed( nStrip, bSoloed, /*bSelectStrip=*/false );
}

bool PluginUiMixerModel::setMasterVolume( float fVolume )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setMasterVolume( fVolume );
}

bool PluginUiMixerModel::setMasterIsMuted( bool bMuted )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setMasterIsMuted( bMuted );
}

bool PluginUiMixerModel::setInstrumentOutputBus( int nInstrument, int nBus )
{
	auto pCAC = controller();
	if ( pCAC == nullptr ) {
		return false;
	}
	return pCAC->setInstrumentOutputBus( nInstrument, nBus );
}

int PluginUiMixerModel::instrumentOutputBus( int nInstrument ) const
{
	const auto pInstrument = instrument( nInstrument );
	return pInstrument != nullptr ? pInstrument->getOutputBus() : 0;
}

int PluginUiMixerModel::effectiveOutputBus( int nInstrument,
											int nBusCount ) const
{
	const int nStored = instrumentOutputBus( nInstrument );
	// Implicit 1-to-1 default (ADR 0019): the bus matching the kit
	// position.
	const int nCandidate = nStored >= 0 ? nStored : nInstrument;
	if ( nCandidate >= 0 && nCandidate < nBusCount ) {
		return nCandidate;
	}
	// Beyond the available buses the instrument feeds the master only.
	return -1;
}

int PluginUiMixerModel::stripCount() const
{
	if ( m_pPlugin == nullptr || m_pPlugin->getHydrogen() == nullptr ) {
		return 0;
	}
	const auto pSong = m_pPlugin->getHydrogen()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ||
		 pSong->getDrumkit()->getInstruments() == nullptr ) {
		return 0;
	}
	return pSong->getDrumkit()->getInstruments()->size();
}

QString PluginUiMixerModel::stripName( int nStrip ) const
{
	const auto pInstrument = instrument( nStrip );
	return pInstrument != nullptr ? pInstrument->getName() : QString();
}

float PluginUiMixerModel::stripVolume( int nStrip ) const
{
	const auto pInstrument = instrument( nStrip );
	return pInstrument != nullptr ? pInstrument->getVolume() : 0.0f;
}

float PluginUiMixerModel::stripPan( int nStrip ) const
{
	const auto pInstrument = instrument( nStrip );
	return pInstrument != nullptr ? pInstrument->getPan() : 0.0f;
}

bool PluginUiMixerModel::stripIsMuted( int nStrip ) const
{
	const auto pInstrument = instrument( nStrip );
	return pInstrument != nullptr ? pInstrument->isMuted() : false;
}

bool PluginUiMixerModel::stripIsSoloed( int nStrip ) const
{
	const auto pInstrument = instrument( nStrip );
	return pInstrument != nullptr ? pInstrument->isSoloed() : false;
}

float PluginUiMixerModel::masterVolume() const
{
	if ( m_pPlugin == nullptr || m_pPlugin->getHydrogen() == nullptr ) {
		return 0.0f;
	}
	const auto pSong = m_pPlugin->getHydrogen()->getSong();
	return pSong != nullptr ? pSong->getVolume() : 0.0f;
}

bool PluginUiMixerModel::masterIsMuted() const
{
	if ( m_pPlugin == nullptr || m_pPlugin->getHydrogen() == nullptr ) {
		return false;
	}
	const auto pSong = m_pPlugin->getHydrogen()->getSong();
	return pSong != nullptr ? pSong->getIsMuted() : false;
}

int PluginUiMixerModel::busCount() const
{
	return m_pPlugin != nullptr ? m_pPlugin->getBusCount() : 0;
}

std::shared_ptr<Instrument> PluginUiMixerModel::instrument(
	int nStrip ) const
{
	if ( m_pPlugin == nullptr || m_pPlugin->getHydrogen() == nullptr ) {
		return nullptr;
	}
	const auto pSong = m_pPlugin->getHydrogen()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return nullptr;
	}
	const auto pInstruments = pSong->getDrumkit()->getInstruments();
	// Guarded range check: a UI polling loop must not spam the log
	// when the kit shrank between two frames.
	if ( pInstruments == nullptr || nStrip < 0 ||
		 nStrip >= pInstruments->size() ) {
		return nullptr;
	}
	return pInstruments->get( nStrip );
}

void PluginUiMixerModel::acceptMeterSnapshot(
	const EngineTelemetrySnapshot& snapshot )
{
	std::lock_guard<std::mutex> lock( m_mutex );
	m_snapshot = snapshot;
	m_bHasSnapshot = true;
}

bool PluginUiMixerModel::hasMeterSnapshot() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_bHasSnapshot;
}

float PluginUiMixerModel::instrumentPeakL( int nInstrument ) const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	if ( ! m_bHasSnapshot || nInstrument < 0 ||
		 nInstrument >= m_snapshot.instPeakCount ) {
		return 0.0f;
	}
	return m_snapshot.peakL[ nInstrument ];
}

float PluginUiMixerModel::instrumentPeakR( int nInstrument ) const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	if ( ! m_bHasSnapshot || nInstrument < 0 ||
		 nInstrument >= m_snapshot.instPeakCount ) {
		return 0.0f;
	}
	return m_snapshot.peakR[ nInstrument ];
}

float PluginUiMixerModel::masterPeakL() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_bHasSnapshot ? m_snapshot.masterPeakL : 0.0f;
}

float PluginUiMixerModel::masterPeakR() const
{
	std::lock_guard<std::mutex> lock( m_mutex );
	return m_bHasSnapshot ? m_snapshot.masterPeakR : 0.0f;
}

} // namespace H2Core
