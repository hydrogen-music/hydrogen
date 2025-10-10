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

#include "SongEditor.h"
#include "SongEditorPanel.h"
#include "../HydrogenApp.h"
#include "../InstrumentEditor/WaveDisplay.h"
#include "../Skin.h"

#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>

using namespace H2Core;

PlaybackTrackWaveDisplay::PlaybackTrackWaveDisplay(QWidget* pParent)
 : WaveDisplay( pParent )
 , m_fTick( 0 )
{
	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( width() * pixelRatio,
									   height() * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	
	setAcceptDrops(true);
}

PlaybackTrackWaveDisplay::~PlaybackTrackWaveDisplay()
{
	delete m_pBackgroundPixmap;
}


void PlaybackTrackWaveDisplay::dropEvent(QDropEvent* event)
{
	const QMimeData* mimeData = event->mimeData();
	QString sText = event->mimeData()->text();

	if ( mimeData->hasUrls() ) {
		QList<QUrl> urlList = mimeData->urls();

		//someone dragged a file from an external tool.
		//check if is a supported sample, and then try to load it
		if( sText.startsWith("file://")) {
			Hydrogen::get_instance()->loadPlaybackTrack( urlList.at(0).toLocalFile() );	
			HydrogenApp* pH2App = HydrogenApp::get_instance();
			
			pH2App->getSongEditorPanel()->updatePlaybackTrack();
		}
	}
}

void PlaybackTrackWaveDisplay::dragEnterEvent(QDragEnterEvent * event)
{
	if(event->mimeData()->hasFormat("text/plain")) {
		event->acceptProposedAction();
	}
}

void PlaybackTrackWaveDisplay::dragMoveEvent(QDragMoveEvent *event)
{
	event->accept();
}

void PlaybackTrackWaveDisplay::updateDisplay( std::shared_ptr<H2Core::InstrumentLayer> pLayer )
{
	INFOLOG("");
	auto pH2App = HydrogenApp::get_instance();
	const auto pPref = Preferences::get_instance();
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ||
		 width() != m_pBackgroundPixmap->width() ||
		 height() != m_pBackgroundPixmap->height() ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width() * pixelRatio ,
										  height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	int nCurrentWidth = width();

	if ( pLayer == nullptr || nCurrentWidth <= 0 ){
		m_pLayer = nullptr;
		m_nActiveWidth = 0;
		m_sSampleName = tr( "No playback track selected" );

		QPainter painter( m_pBackgroundPixmap );
		createBackground( &painter );
		update();
		return;
	}

	m_pLayer = pLayer;
	
	if ( nCurrentWidth != m_nCurrentWidth ) {
		delete[] m_pPeakData;
		m_pPeakData = new int[ nCurrentWidth ];
		
		m_nCurrentWidth = nCurrentWidth;
	}
	
	// Initialise everything with 0..
	memset( m_pPeakData, 0, nCurrentWidth * sizeof( m_pPeakData[ 0 ] ) );
	
	if ( pLayer && pLayer->getSample() ) {

		m_sSampleName = m_pLayer->getSample()->getFilename();

		const auto nSongLengthInTicks = pSong->lengthInTicks();
		const auto pColumns = pSong->getPatternGroupVector();
		const auto nMaxBars = pPref->getMaxBars();
		auto pSampleData = pLayer->getSample()->getData_L();
		const int nSampleLength = m_pLayer->getSample()->getFrames();
		const float fGain = height() / 2.0 * pLayer->getGain();

		int nSongEditorGridWidth;
		if ( pH2App->getSongEditorPanel() != nullptr ) {
			nSongEditorGridWidth = pH2App->getSongEditorPanel()->getSongEditor()->getGridWidth();
		} else {
			//this might happen during init of SongEditorPanel
			nSongEditorGridWidth = 16;
		}
		
		int nRenderStartPosition = 0.8 * nSongEditorGridWidth;

		int nTotalTicks = 0;
		long long nTotalFrames = 0;
		int nSamplePos = 0;
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
			const long long nNextEndFrame =
				TransportPosition::computeFrameFromTick(
					  static_cast<double>(nTotalTicks), &fMismatch );

			// We have not enough room to render all the details, so we need to
			// coarse grain.
			const int nFramesPerPixel = ( nNextEndFrame - nTotalFrames ) /
										nSongEditorGridWidth;

			// Render all peaks corresponding to the column
			int nnVal;
			for ( int ii = nRenderStartPosition;
				 ( ii < nRenderStartPosition + nSongEditorGridWidth ) &&
				 ( ii < nCurrentWidth ); ++ii ) {
				nnVal = 0;
				for ( int jj = 0; jj < nFramesPerPixel; ++jj ) {
					if ( nSamplePos < nSampleLength ) {
						const int nNewVal = (int)( pSampleData[ nSamplePos ] * fGain );
						if ( nNewVal > nnVal ) {
							nnVal = nNewVal;
						}
					}

					++nSamplePos;
				}
				m_pPeakData[ ii ] = nnVal;
			}
			nRenderStartPosition += nSongEditorGridWidth;
			nTotalFrames = nNextEndFrame;

			if ( nTotalTicks <= nSongLengthInTicks ) {
				m_nActiveWidth = nRenderStartPosition;
			}

			if ( nTotalFrames >= nSampleLength ) {
				break;
			}
		}
	}
	else {
		m_sSampleName = "-";
	}

	QPainter painter( m_pBackgroundPixmap );
	createBackground( &painter );
	update();
}

void PlaybackTrackWaveDisplay::updatePosition( float fTick ) {
	m_fTick = fTick;
	update();
}

void PlaybackTrackWaveDisplay::paintEvent( QPaintEvent *ev ) {

	if (!isVisible()) {
		return;
	}

	QPainter painter( this );
	
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 width() != m_pBackgroundPixmap->width() ||
		 height() != m_pBackgroundPixmap->height() ) {
		updateDisplay( m_pLayer );
	}
	
	// Render the wave display.
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap,
						QRectF( pixelRatio * ev->rect().x(),
								pixelRatio * ev->rect().y(),
								pixelRatio * ev->rect().width(),
								pixelRatio * ev->rect().height() ) );

	// Draw playhead
	auto pSongEditorPanel = HydrogenApp::get_instance()->getSongEditorPanel();
	if ( m_fTick != -1 && pSongEditorPanel != nullptr ) {
		if ( pSongEditorPanel->getSongEditor() != nullptr ) {
			int nX = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
									   m_fTick *
									   static_cast<float>(pSongEditorPanel->getSongEditor()->
														  getGridWidth()) -
									   static_cast<float>(Skin::nPlayheadWidth) / 2 );
			int nOffset = Skin::getPlayheadShaftOffset();
			Skin::setPlayheadPen( &painter, false );
			painter.drawLine( nX + nOffset, 0, nX + nOffset, height() );
		}
	}

}
