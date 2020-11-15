/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/pattern.h>
using namespace H2Core;


#include "../Skin.h"
#include "../HydrogenApp.h"
#include "../InstrumentEditor/WaveDisplay.h"

#include "PlaybackTrackWaveDisplay.h"
#include "SongEditor.h"
#include "SongEditorPanel.h"

const char* PlaybackTrackWaveDisplay::__class_name = "PlaybackTrackWaveDisplay";

PlaybackTrackWaveDisplay::PlaybackTrackWaveDisplay(QWidget* pParent)
 : WaveDisplay( pParent )
{
	INFOLOG( "INIT" );
	
	setAcceptDrops(true);
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

void PlaybackTrackWaveDisplay::updateDisplay( H2Core::InstrumentLayer *pLayer )
{
	HydrogenApp* pH2App = HydrogenApp::get_instance();
	Preferences* pPref = Preferences::get_instance();
	
	int currentWidth = width();
	
	if(!pLayer || currentWidth <= 0){
		m_pLayer = nullptr;
		m_sSampleName = "-";
		
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
		Song* pSong = Hydrogen::get_instance()->getSong();
		
		m_pLayer = pLayer;
		m_sSampleName = m_pLayer->get_sample()->get_filename();
		
		auto	pSampleData = pLayer->get_sample()->get_data_l();
		int		nSampleLength = m_pLayer->get_sample()->get_frames();
		float	fLengthOfPlaybackTrackInSecs = ( float )( nSampleLength / (float) m_pLayer->get_sample()->get_sample_rate() );
		float	fRemainingLengthOfPlaybackTrack = fLengthOfPlaybackTrackInSecs;		
		float	fGain = height() / 2.0 * pLayer->get_gain();
		int		nSamplePos = 0;
		int		nMaxBars = pPref->getMaxBars();
		
		std::vector<PatternList*> *pPatternColumns = pSong->get_pattern_group_vector();
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
			if(maxPatternSize == 0) maxPatternSize = 192;
			
			//length (in seconds) of one pattern is: (nPatternSize/24) / ((pEngine->getSong()->__bpm * 2) / 60)
			float fLengthOfCurrentPatternInSecs = (maxPatternSize/24) / ((pSong->__bpm * 2) / 60);
			
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

	update();
}

