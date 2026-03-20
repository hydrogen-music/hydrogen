/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "PlaybackTrackWaveDisplay.h"

#include "../HydrogenApp.h"
#include "../Skin.h"
#include "SongEditor.h"
#include "SongEditorPanel.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

using namespace H2Core;

PlaybackTrackWaveDisplay::PlaybackTrackWaveDisplay( QWidget* pParent )
	: WaveDisplay( pParent ), m_fTick( 0 )
{
	m_sFallbackLabel = tr( "No playback track selected" );

	m_type = WaveDisplay::Type::Envelope;

	setAcceptDrops( true );
}

PlaybackTrackWaveDisplay::~PlaybackTrackWaveDisplay()
{
}

void PlaybackTrackWaveDisplay::dropEvent( QDropEvent* event )
{
	const QMimeData* mimeData = event->mimeData();
	QString sText = event->mimeData()->text();

	if ( mimeData->hasUrls() ) {
		QList<QUrl> urlList = mimeData->urls();

		// someone dragged a file from an external tool.
		// check if is a supported sample, and then try to load it
		if ( sText.startsWith( "file://" ) ) {
			Hydrogen::get_instance()->loadPlaybackTrack(
				urlList.at( 0 ).toLocalFile()
			);
			HydrogenApp* pH2App = HydrogenApp::get_instance();

			pH2App->getSongEditorPanel()->updatePlaybackTrack();
		}
	}
}

void PlaybackTrackWaveDisplay::dragEnterEvent( QDragEnterEvent* event )
{
	if ( event->mimeData()->hasFormat( "text/plain" ) ) {
		event->acceptProposedAction();
	}
}

void PlaybackTrackWaveDisplay::dragMoveEvent( QDragMoveEvent* event )
{
	event->accept();
}

void PlaybackTrackWaveDisplay::updatePeakData()
{
	if ( width() != m_peakData.size() || width() != m_peakDataMin.size() ) {
		m_peakData.resize( width() );
		m_peakDataMin.resize( width() );
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( m_pLayer == nullptr || m_pLayer->getSample() == nullptr ||
		 pSong == nullptr ) {
		for ( int ii = 0; ii < m_peakData.size(); ++ii ) {
			m_peakData[ii] = 0;
			m_peakDataMin[ii] = 0;
		}

		drawPeakData();
		update();
		return;
	}

	auto pH2App = HydrogenApp::get_instance();
	const auto nSongLengthInTicks = pSong->lengthInTicks();
	const auto pColumns = pSong->getPatternGroupVector();
	const auto nMaxBars = Preferences::get_instance()->getMaxBars();
	auto pSampleData = m_pLayer->getSample()->getData_L();
	const long long nSampleLength = m_pLayer->getSample()->getFrames();
	const float fGain = height() / 2.0 * m_pLayer->getGain();
	// If sample rates of audio driver and the underlying wave data do not
	// match, our Sampler will resample it on the fly. We have to account for
	// this within the increment.
	const float fStep =
		static_cast<float>( m_pLayer->getSample()->getSampleRate() ) /
		static_cast<float>( Hydrogen::get_instance()
								->getAudioEngine()
								->getAudioDriver()
								->getSampleRate() );

	int nSongEditorGridWidth;
	if ( pH2App->getSongEditorPanel() != nullptr ) {
		nSongEditorGridWidth =
			pH2App->getSongEditorPanel()->getSongEditor()->getGridWidth();
	}
	else {
		// this might happen during init of SongEditorPanel
		nSongEditorGridWidth = 16;
	}

	int nRenderStartPosition = SongEditor::nMargin;

	int nTotalTicks = 0;
	float fTotalFrames = 0;
    long long nLastEndFrame = 0;
	long long nSamplePos = 0;
	double fMismatch;
	for ( int nnColumn = 0; nnColumn < nMaxBars; ++nnColumn ) {
		int nColumnLengthTicks = 0;
		if ( nnColumn < pColumns->size() &&
			 pColumns->at( nnColumn ) != nullptr ) {
			nColumnLengthTicks =
				pColumns->at( nnColumn )->longestPatternLength();
		}

		// No pattern found in this column, use default size.
		if ( nColumnLengthTicks <= 0 ) {
			nColumnLengthTicks = 4 * H2Core::nTicksPerQuarter;
		}
		nTotalTicks += nColumnLengthTicks;
		const long long nNextEndFrame = Transport::computeFrameFromTick(
			static_cast<double>( nTotalTicks ), &fMismatch
		);

		const float fIncrement =
			static_cast<float>( nNextEndFrame - nLastEndFrame ) * fStep;

		// We have not enough room to render all the details, so we need to
		// coarse grain.
		const int nFramesPerPixel = static_cast<int>( std::floor(
			fIncrement / static_cast<float>( nSongEditorGridWidth )
		) );

		// Render all peaks corresponding to the column
		int nMin, nMax;
		for ( int ii = nRenderStartPosition;
			  ( ii < nRenderStartPosition + nSongEditorGridWidth ) &&
			  ( ii < m_peakData.size() );
			  ++ii ) {
			nMin = 0;
			nMax = 0;
			for ( int jj = 0; jj < nFramesPerPixel; ++jj ) {
				if ( nSamplePos < nSampleLength ) {
					const int nNewVal =
						(int) ( pSampleData[nSamplePos] * fGain );
					if ( nNewVal > nMax ) {
						nMax = nNewVal;
					}
					if ( nNewVal < nMin ) {
						nMin = nNewVal;
					}
				}

				++nSamplePos;
			}
			m_peakData[ii] = nMax;
			m_peakDataMin[ii] = nMin;
		}

		nRenderStartPosition += nSongEditorGridWidth;
		nLastEndFrame = nNextEndFrame;
		fTotalFrames += fIncrement;

		if ( nTotalTicks <= nSongLengthInTicks ) {
			m_nActiveWidth = nRenderStartPosition;
		}

		if ( static_cast<long long>( std::floor( fTotalFrames ) ) >=
			 nSampleLength ) {
			break;
		}
	}

	for ( int ii = nRenderStartPosition; ii < m_peakData.size(); ++ii ) {
		m_peakData[ii] = 0;
		m_peakDataMin[ii] = 0;
	}

	// In pattern mode the playback track won't be played back at all.
	if ( pSong->getMode() == Song::Mode::Pattern ) {
		m_nActiveWidth = 0;
	}

	drawPeakData();
	update();
}

void PlaybackTrackWaveDisplay::updatePosition( float fTick )
{
	m_fTick = fTick;
	update();
}

void PlaybackTrackWaveDisplay::paintEvent( QPaintEvent* ev )
{
	if ( !isVisible() ) {
		return;
	}

	WaveDisplay::paintEvent( ev );

	QPainter painter( this );

	// Draw playhead
	auto pSongEditorPanel = HydrogenApp::get_instance()->getSongEditorPanel();
	if ( m_fTick != -1 && pSongEditorPanel != nullptr ) {
		if ( pSongEditorPanel->getSongEditor() != nullptr ) {
			int nX = static_cast<int>(
				static_cast<float>( SongEditor::nMargin ) + 1 +
				m_fTick * static_cast<float>(
							  pSongEditorPanel->getSongEditor()->getGridWidth()
						  ) -
				static_cast<float>( Skin::nPlayheadWidth ) / 2
			);
			int nOffset = Skin::getPlayheadShaftOffset();
			Skin::setPlayheadPen( &painter, false );
			painter.drawLine( nX + nOffset, 0, nX + nOffset, height() );
		}
	}
}
