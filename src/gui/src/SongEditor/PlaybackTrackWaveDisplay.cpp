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
}


void PlaybackTrackWaveDisplay::updateDisplay( H2Core::InstrumentLayer *pLayer )
{
	int currentWidth = width();
	
	if(!pLayer || currentWidth <= 0){
		m_pLayer = nullptr;
		m_sSampleName = "-";
		
		return;
	}
	
	if(currentWidth != m_nCurrentWidth){
		delete[] m_pPeakData;
		m_pPeakData = new int[ currentWidth ];
		memset( m_pPeakData, 0, currentWidth );
		
		m_nCurrentWidth = currentWidth;
	}
	
	if ( pLayer && pLayer->get_sample() ) {
		m_pLayer = pLayer;
		m_sSampleName = pLayer->get_sample()->get_filename();
		
		Song* pSong = Hydrogen::get_instance()->getSong();
		std::vector<PatternList*> *pPatternColumns = pSong->get_pattern_group_vector();
		int nColumns = pPatternColumns->size();
		
		int nPatternSize;
			
		for ( int patternPosition = 0; patternPosition < nColumns; ++patternPosition ) {
			PatternList *pColumn = ( *pPatternColumns )[ patternPosition ];
			if ( pColumn->size() != 0 ) {
				nPatternSize = pColumn->get( 0 )->get_length();
			} else {
				nPatternSize = MAX_NOTES;
			}
		}
		
		//decide how many patterns the sample lasts
		
		//length (in seconds) of one pattern is: (nPatternSize/24) / ((pEngine->getSong()->__bpm * 2) / 60)
		float fSeconds = (nPatternSize/24) / ((pSong->__bpm * 2) / 60);
		
		float sec = ( float )( m_pLayer->get_sample()->get_frames() / (float)m_pLayer->get_sample()->get_sample_rate() );
		
		QString qsec;
		qsec = QString::asprintf( "%2.2f", fSeconds );
		
		int SongEditorSquaresNeededForPlaybackTrack = (int) (sec / fSeconds);
		
		INFOLOG(QString("Length(s) of one pattern is" + qsec));
		INFOLOG(QString("Playback track must be scaled to %1 squares").arg((( SongEditorSquaresNeededForPlaybackTrack ))));
		
		HydrogenApp* pH2App = HydrogenApp::get_instance();
		
		//Size of each square (pixel)
		int nSongEditorGridWith;
		if(pH2App->getSongEditorPanel())
		{
			nSongEditorGridWith = pH2App->getSongEditorPanel()->getSongEditor()->getGridWidth();			
		} else {
			//during init of SongEditorPanel
			nSongEditorGridWith = 16;
		}
		
		if(SongEditorSquaresNeededForPlaybackTrack == 0) SongEditorSquaresNeededForPlaybackTrack = 1;
		
		int nPlaybackTrackWidth = SongEditorSquaresNeededForPlaybackTrack * nSongEditorGridWith;

//		INFOLOG( "[updateDisplay] sample: " + m_sSampleName  );

		int nSampleLength = pLayer->get_sample()->get_frames();
		int nScaleFactor = nSampleLength / nPlaybackTrackWidth ;

		float fGain = height() / 2.0 * pLayer->get_gain();

		float *pSampleData = pLayer->get_sample()->get_data_l();

		int nSamplePos = 0;
		int nVal = 0;
		int nStartOffset = 0.8 * nSongEditorGridWith; //one square
		
		//We're starting to paint with an offset, everything smaller is init. with 0
		for ( int i = nStartOffset; i < currentWidth; ++i ){
			nVal = 0;
			
			if(i < nPlaybackTrackWidth + nStartOffset)
			{
				for ( int j = 0; j < nScaleFactor; ++j ) {
					if ( j < nSampleLength ) {
						int newVal = (int)( pSampleData[ nSamplePos ] * fGain );
						if ( newVal > nVal ) {
							nVal = newVal;
						}
					}
					++nSamplePos;
				}
			}
			
			m_pPeakData[ i ] = nVal;
		}
	}
	else {
		m_sSampleName = "-";
		for ( int i =0; i < m_nCurrentWidth; ++i ){
			m_pPeakData[ i ] = 0;
		}
		
	}

	update();
}

