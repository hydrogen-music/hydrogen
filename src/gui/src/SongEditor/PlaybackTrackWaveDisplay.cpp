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

#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentLayer.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Pattern.h>
using namespace H2Core;


#include "../HydrogenApp.h"
#include "../InstrumentEditor/WaveDisplay.h"
#include "../Skin.h"

#include "PlaybackTrackWaveDisplay.h"
#include "SongEditor.h"
#include "SongEditorPanel.h"

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
			
			pH2App->getSongEditorPanel()->updatePlaybackTrackIfNecessary();
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
	HydrogenApp* pH2App = HydrogenApp::get_instance();
	Preferences* pPref = Preferences::get_instance();

	QColor defaultColor = pPref->getColorTheme()->m_songEditor_backgroundColor;
	
	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ||
		 width() != m_pBackgroundPixmap->width() ||
		 height() != m_pBackgroundPixmap->height() ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width()  * pixelRatio , height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	int currentWidth = width();

	if( pLayer == nullptr || currentWidth <= 0 ){
		m_pLayer = nullptr;
		m_sSampleName = tr( "No playback track selected" );

		QPainter painter( m_pBackgroundPixmap );
		createBackground( &painter );
		update();
		return;
	}
	
	if(currentWidth != m_nCurrentWidth){
		delete[] m_pPeakData;
		m_pPeakData = new int[ currentWidth ];
		
		m_nCurrentWidth = currentWidth;
	}
	
	//initialise everything with 0..	
	memset( m_pPeakData, 0, currentWidth * sizeof(m_pPeakData[0]) );	
	
	if ( pLayer && pLayer->get_sample() ) {
		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		
		m_pLayer = pLayer;
		m_sSampleName = m_pLayer->get_sample()->get_filename();
		
		auto	pSampleData = pLayer->get_sample()->get_data_l();
		int		nSampleLength = m_pLayer->get_sample()->get_frames();
		float	fLengthOfPlaybackTrackInSecs = ( float )( nSampleLength / (float) m_pLayer->get_sample()->get_sample_rate() );
		float	fRemainingLengthOfPlaybackTrack = fLengthOfPlaybackTrackInSecs;		
		float	fGain = height() / 2.0 * pLayer->get_gain();
		int		nSamplePos = 0;
		int		nMaxBars = pPref->getMaxBars();
		
		std::vector<PatternList*> *pPatternColumns = pSong->getPatternGroupVector();
		int nColumns = pPatternColumns->size();

		int nSongEditorGridWith;
		if( pH2App->getSongEditorPanel() ) {
			nSongEditorGridWith = pH2App->getSongEditorPanel()->getSongEditor()->getGridWidth();
		} else {
			//this might happen during init of SongEditorPanel
			nSongEditorGridWith = 16;
		}
		
		int nRenderStartPosition = 0.8 * nSongEditorGridWith;		
		
		for ( int patternPosition = 0; patternPosition < nMaxBars; ++patternPosition ) {
			int maxPatternSize = 0;
			
			if( patternPosition < nColumns ) {
				PatternList *pColumn = ( *pPatternColumns )[ patternPosition ];
				
				for ( unsigned j = 0; j < pColumn->size(); j++ ) {
					const Pattern *pPattern = pColumn->get( j );
					int nPatternSize = pPattern->get_length();	
					
					if(maxPatternSize < nPatternSize) {
						maxPatternSize = nPatternSize;
					}
				}
			}
			
			//No pattern found in this column, use default size (Size: 8)
			if( maxPatternSize == 0 ) {
				maxPatternSize = 192;
			}
			
			//length (in seconds) of one pattern is: (nPatternSize/24) / ((ppSong->getBpm() * 2) / 60)
			float fLengthOfCurrentPatternInSecs = (maxPatternSize/24) / ((pSong->getBpm() * 2) / 60);
			
			if( fRemainingLengthOfPlaybackTrack >= fLengthOfCurrentPatternInSecs ) {
				//only a part of the PlaybackTrack will fit into this Pattern
				float nScaleFactor = fLengthOfCurrentPatternInSecs / fLengthOfPlaybackTrackInSecs;
				int nSamplesToRender = nScaleFactor * nSampleLength;
				
				int nVal = 0;
				
				for ( int i = nRenderStartPosition; i < nRenderStartPosition + nSongEditorGridWith ; ++i ) {
					if( i < m_nCurrentWidth ) {
						nVal = 0;
						
						int nSamplesToRenderInThisStep =  (nSamplesToRender / nSongEditorGridWith);
						for ( int j = 0; j < nSamplesToRenderInThisStep; ++j ) {
							if ( nSamplePos < nSampleLength ) {
								int newVal = (int)( pSampleData[ nSamplePos ] * fGain );
								if ( newVal > nVal ) {
									nVal = newVal;
								}
							}
							
							++nSamplePos;
						}
					
						m_pPeakData[ i ] = nVal;
					}
				}
				
				nRenderStartPosition += nSongEditorGridWith;
				fRemainingLengthOfPlaybackTrack -= fLengthOfCurrentPatternInSecs;
			}
		}
	} else {
		m_sSampleName = "-";
		for ( int i =0; i < m_nCurrentWidth; ++i ){
			m_pPeakData[ i ] = 0;
		}
		
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
