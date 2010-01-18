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

#include "PatternEditorInstrumentList.h"

#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/note.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/Song.h>
using namespace H2Core;

#include "PatternEditorPanel.h"
#include "DrumPatternEditor.h"
#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"
#include "../widgets/Button.h"

#include <QtGui>
#include <cassert>

using namespace std;





InstrumentLine::InstrumentLine(QWidget* pParent)
  : PixmapWidget(pParent, "InstrumentLine")
  , m_bIsSelected(false)
{
	int h = Preferences::get_instance()->getPatternEditorGridHeight();
	setFixedSize(181, h);

	m_pNameLbl = new QLabel(this);
	m_pNameLbl->resize( 145, h );
	m_pNameLbl->move( 10, 1 );
	QFont nameFont;
	nameFont.setPointSize( 10 );
	nameFont.setBold( true );
	m_pNameLbl->setFont(nameFont);

	m_pMuteBtn = new ToggleButton(
			this,
			"/patternEditor/btn_mute_on.png",
			"/patternEditor/btn_mute_off.png",
			"/patternEditor/btn_mute_off.png",
			QSize( 9, 9 )
	);
	//m_pMuteBtn->setText( "M" );
	m_pMuteBtn->move( 155, 5 );
	m_pMuteBtn->setPressed(false);
	connect(m_pMuteBtn, SIGNAL(clicked(Button*)), this, SLOT(muteClicked()));

	m_pSoloBtn = new ToggleButton(
			this,
			"/patternEditor/btn_solo_on.png",
			"/patternEditor/btn_solo_off.png",
			"/patternEditor/btn_solo_off.png",
			QSize( 9, 9 )
	);
	//m_pSoloBtn->setText( "S" );
	m_pSoloBtn->move( 165, 5 );
	m_pSoloBtn->setPressed(false);
	connect(m_pSoloBtn, SIGNAL(clicked(Button*)), this, SLOT(soloClicked()));



	// Popup menu
	m_pFunctionPopup = new QMenu( this );
	m_pFunctionPopup->addAction( trUtf8( "Clear notes" ), this, SLOT( functionClearNotes() ) );
	m_pFunctionPopupSub = new QMenu( trUtf8( "Fill notes ..." ), m_pFunctionPopup );
	m_pFunctionPopupSub->addAction( trUtf8( "Fill all notes" ), this, SLOT( functionFillAllNotes() ) );
	m_pFunctionPopupSub->addAction( trUtf8( "Fill 1/2 notes" ), this, SLOT( functionFillEveryTwoNotes() ) );
	m_pFunctionPopupSub->addAction( trUtf8( "Fill 1/3 notes" ), this, SLOT( functionFillEveryThreeNotes() ) );
	m_pFunctionPopupSub->addAction( trUtf8( "Fill 1/4 notes" ), this, SLOT( functionFillEveryFourNotes() ) );
	m_pFunctionPopupSub->addAction( trUtf8( "Fill 1/6 notes" ), this, SLOT( functionFillEverySixNotes() ) );
	m_pFunctionPopupSub->addAction( trUtf8( "Fill 1/8 notes" ), this, SLOT( functionFillEveryEightNotes() ) );
	m_pFunctionPopup->addMenu( m_pFunctionPopupSub );
	m_pFunctionPopup->addAction( trUtf8( "Randomize velocity" ), this, SLOT( functionRandomizeVelocity() ) );
	m_pFunctionPopup->addSeparator();
	m_pFunctionPopup->addAction( trUtf8( "Delete instrument" ), this, SLOT( functionDeleteInstrument() ) );

	m_bIsSelected = true;
	setSelected(false);
}



void InstrumentLine::setName(const QString& sName)
{
	m_pNameLbl->setText(sName);
}



void InstrumentLine::setSelected(bool bSelected)
{
	if (bSelected == m_bIsSelected) {
		return;
	}
	m_bIsSelected = bSelected;
	if (m_bIsSelected) {
		setPixmap( "/patternEditor/instrument_line_selected.png");
	}
	else {
		setPixmap( "/patternEditor/instrument_line.png");
	}
}



void InstrumentLine::setNumber(int nIndex)
{
	m_nInstrumentNumber = nIndex;
}



void InstrumentLine::setMuted(bool isMuted)
{
	m_pMuteBtn->setPressed(isMuted);
}


void InstrumentLine::setSoloed( bool soloed )
{
	m_pSoloBtn->setPressed( soloed );
}



void InstrumentLine::muteClicked()
{
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->get_instrument_list();

	Instrument *pInstr = instrList->get(m_nInstrumentNumber);
	pInstr->set_muted( !pInstr->is_muted());
}



void InstrumentLine::soloClicked()
{
	HydrogenApp::get_instance()->getMixer()->soloClicked( m_nInstrumentNumber );
}



void InstrumentLine::mousePressEvent(QMouseEvent *ev)
{
	Hydrogen::get_instance()->setSelectedInstrumentNumber( m_nInstrumentNumber );
	HydrogenApp::get_instance()->getPatternEditorPanel()->updatePianorollEditor();

	if ( ev->button() == Qt::LeftButton ) {
		const float velocity = 0.8f;
		const float pan_L = 0.5f;
		const float pan_R = 0.5f;
		const int nLength = -1;
		const float fPitch = 0.0f;
		Song *pSong = Hydrogen::get_instance()->getSong();
		
		Instrument *pInstr = pSong->get_instrument_list()->get( m_nInstrumentNumber );
		
		Note *pNote = new Note( pInstr, 0, velocity, pan_L, pan_R, nLength, fPitch);
		AudioEngine::get_instance()->get_sampler()->note_on(pNote);
	}
	else if (ev->button() == Qt::RightButton ) {
		m_pFunctionPopup->popup( QPoint( ev->globalX(), ev->globalY() ) );
	}

	// propago l'evento al parent: serve per il drag&drop
	PixmapWidget::mousePressEvent(ev);
}



H2Core::Pattern* InstrumentLine::getCurrentPattern()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	assert( pPatternList != NULL );

	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( nSelectedPatternNumber != -1 ) {
		Pattern* pCurrentPattern = pPatternList->get( nSelectedPatternNumber );
		return pCurrentPattern;
	}
	return NULL;
}




void InstrumentLine::functionClearNotes()
{
// 	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

	Hydrogen * H = Hydrogen::get_instance();
	Pattern *pCurrentPattern = getCurrentPattern();

	int nSelectedInstrument = m_nInstrumentNumber;
	Instrument *pSelectedInstrument = H->getSong()->get_instrument_list()->get( nSelectedInstrument );
	
	pCurrentPattern->purge_instrument( pSelectedInstrument );
// 	std::multimap <int, Note*>::iterator pos;
// 	for ( pos = pCurrentPattern->note_map.begin(); pos != pCurrentPattern->note_map.end(); ++pos ) {
// 		Note *pNote = pos->second;
// 		assert( pNote );
// 		if ( pNote->get_instrument() != pSelectedInstrument ) {
// 			continue;
// 		}
// 
// 		delete pNote;
// 		pCurrentPattern->note_map.erase( pos );
// 	}
// 	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	// this will force an update...
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}



void InstrumentLine::functionFillAllNotes(){ functionFillNotes(1); }
void InstrumentLine::functionFillEveryTwoNotes(){ functionFillNotes(2); }
void InstrumentLine::functionFillEveryThreeNotes(){ functionFillNotes(3); }
void InstrumentLine::functionFillEveryFourNotes(){ functionFillNotes(4); }
void InstrumentLine::functionFillEverySixNotes(){ functionFillNotes(6); }
void InstrumentLine::functionFillEveryEightNotes(){ functionFillNotes(8); }

void InstrumentLine::functionFillNotes( int every )
{
	Hydrogen *pEngine = Hydrogen::get_instance();

	const float velocity = 0.8f;
	const float pan_L = 0.5f;
	const float pan_R = 0.5f;
	const float fPitch = 0.0f;
	const int nLength = -1;

	PatternEditorPanel *pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();
	DrumPatternEditor *pPatternEditor = pPatternEditorPanel->getDrumPatternEditor();
	int nBase;
	if ( pPatternEditor->isUsingTriplets() ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int nResolution = 4 * MAX_NOTES * every / ( nBase * pPatternEditor->getResolution() );


	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine


	Song *pSong = pEngine->getSong();

	Pattern* pCurrentPattern = getCurrentPattern();
	if (pCurrentPattern != NULL) {
		int nPatternSize = pCurrentPattern->get_length();
		int nSelectedInstrument = pEngine->getSelectedInstrumentNumber();

		if (nSelectedInstrument != -1) {
			Instrument *instrRef = (pSong->get_instrument_list())->get( nSelectedInstrument );

			for (int i = 0; i < nPatternSize; i += nResolution) {
				bool noteAlreadyPresent = false;

				std::multimap <int, Note*>::iterator pos;
				for ( pos = pCurrentPattern->note_map.lower_bound( i ); pos != pCurrentPattern->note_map.upper_bound( i ); ++pos ) {
					Note *pNote = pos->second;
					if ( pNote->get_instrument() == instrRef ) {
						// note already exists
						noteAlreadyPresent = true;
						break;
					}
				}

				if ( noteAlreadyPresent == false ) {
					// create the new note
					Note *pNote = new Note( instrRef, i, velocity, pan_L, pan_R, nLength, fPitch );
					//pNote->setInstrument(instrRef);
					pCurrentPattern->note_map.insert( std::make_pair( i, pNote ) );
				}
			}
		}
	}
	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	// this will force an update...
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );
}





void InstrumentLine::functionRandomizeVelocity()
{
	Hydrogen *pEngine = Hydrogen::get_instance();

	PatternEditorPanel *pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();
	DrumPatternEditor *pPatternEditor = pPatternEditorPanel->getDrumPatternEditor();

	AudioEngine::get_instance()->lock( RIGHT_HERE );	// lock the audio engine

	int nBase;
	if ( pPatternEditor->isUsingTriplets() ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int nResolution = 4 * MAX_NOTES / ( nBase * pPatternEditor->getResolution() );

	Song *pSong = pEngine->getSong();

	Pattern* pCurrentPattern = getCurrentPattern();
	if (pCurrentPattern != NULL) {
		int nPatternSize = pCurrentPattern->get_length();
		int nSelectedInstrument = pEngine->getSelectedInstrumentNumber();

		if (nSelectedInstrument != -1) {
			Instrument *instrRef = (pSong->get_instrument_list())->get( nSelectedInstrument );

			for (int i = 0; i < nPatternSize; i += nResolution) {
				std::multimap <int, Note*>::iterator pos;
				for ( pos = pCurrentPattern->note_map.lower_bound(i); pos != pCurrentPattern->note_map.upper_bound( i ); ++pos ) {
					Note *pNote = pos->second;
					if ( pNote->get_instrument() == instrRef ) {
						float fVal = ( rand() % 100 ) / 100.0;
						fVal = pNote->get_velocity() + ( ( fVal - 0.50 ) / 2 );
						if ( fVal < 0  ) {
							fVal = 0;
						}
						if ( fVal > 1 ) {
							fVal = 1;
						}
						pNote->set_velocity(fVal);
					}
				}
			}
		}
	}
	AudioEngine::get_instance()->unlock();	// unlock the audio engine

	// this will force an update...
	EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

}





void InstrumentLine::functionDeleteInstrument()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	pEngine->removeInstrument( m_nInstrumentNumber, false );
	
	AudioEngine::get_instance()->lock( RIGHT_HERE );
#ifdef JACK_SUPPORT
	pEngine->renameJackPorts();
#endif
	AudioEngine::get_instance()->unlock();
}



//////



PatternEditorInstrumentList::PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel )
 : QWidget( parent )
 , Object( "PatternEditorInstrumentList" )
{
	//setAttribute(Qt::WA_NoBackground);

	//INFOLOG("INIT");
	m_pPattern = NULL;
 	m_pPatternEditorPanel = pPatternEditorPanel;

	m_nGridHeight = Preferences::get_instance()->getPatternEditorGridHeight();

	m_nEditorWidth = 181;
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;

	resize( m_nEditorWidth, m_nEditorHeight );


	setAcceptDrops(true);

	for ( int i = 0; i < MAX_INSTRUMENTS; ++i) {
		m_pInstrumentLine[i] = NULL;
	}


	updateInstrumentLines();

	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateInstrumentLines() ) );
	m_pUpdateTimer->start(50);

}



PatternEditorInstrumentList::~PatternEditorInstrumentList()
{
	//INFOLOG( "DESTROY" );
	m_pUpdateTimer->stop();
}




///
/// Create a new InstrumentLine
///
InstrumentLine* PatternEditorInstrumentList::createInstrumentLine()
{
	InstrumentLine *pLine = new InstrumentLine(this);
	return pLine;
}


void PatternEditorInstrumentList::moveInstrumentLine( int nSourceInstrument , int nTargetInstrument )
{
		Hydrogen *engine = Hydrogen::get_instance();
		AudioEngine::get_instance()->lock( RIGHT_HERE );

		Song *pSong = engine->getSong();
		InstrumentList *pInstrumentList = pSong->get_instrument_list();

		if ( ( nTargetInstrument > (int)pInstrumentList->get_size() ) || ( nTargetInstrument < 0) ) {
			AudioEngine::get_instance()->unlock();
			return;
		}


		// move instruments...

		Instrument *pSourceInstr = pInstrumentList->get(nSourceInstrument);
		if ( nSourceInstrument < nTargetInstrument) {
			for (int nInstr = nSourceInstrument; nInstr < nTargetInstrument; nInstr++) {
				Instrument * pInstr = pInstrumentList->get(nInstr + 1);
				pInstrumentList->replace( pInstr, nInstr );
			}
			pInstrumentList->replace( pSourceInstr, nTargetInstrument );
		}
		else {
			for (int nInstr = nSourceInstrument; nInstr >= nTargetInstrument; nInstr--) {
				Instrument * pInstr = pInstrumentList->get(nInstr - 1);
				pInstrumentList->replace( pInstr, nInstr );
			}
			pInstrumentList->replace( pSourceInstr, nTargetInstrument );
		}

		#ifdef JACK_SUPPORT
		engine->renameJackPorts();
		#endif

		AudioEngine::get_instance()->unlock();
		engine->setSelectedInstrumentNumber( nTargetInstrument );

		pSong->__is_modified = true;
}

///
/// Update every InstrumentLine, create or destroy lines if necessary.
///
void PatternEditorInstrumentList::updateInstrumentLines()
{
	//INFOLOG( "Update lines" );

	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	Mixer * mixer = HydrogenApp::get_instance()->getMixer();

	unsigned nSelectedInstr = pEngine->getSelectedInstrumentNumber();

	unsigned nInstruments = pInstrList->get_size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {
		if ( nInstr >= nInstruments ) {	// unused instrument! let's hide and destroy the mixerline!
			if ( m_pInstrumentLine[ nInstr ] ) {
				delete m_pInstrumentLine[ nInstr ];
				m_pInstrumentLine[ nInstr ] = NULL;

				int newHeight = m_nGridHeight * nInstruments;
				resize( width(), newHeight );

			}
			continue;
		}
		else {
			if ( m_pInstrumentLine[ nInstr ] == NULL ) {
				// the instrument line doesn't exists..I'll create a new one!
				m_pInstrumentLine[ nInstr ] = createInstrumentLine();
				m_pInstrumentLine[nInstr]->move( 0, m_nGridHeight * nInstr );
				m_pInstrumentLine[nInstr]->show();

				int newHeight = m_nGridHeight * nInstruments;
				resize( width(), newHeight );
			}
			InstrumentLine *pLine = m_pInstrumentLine[ nInstr ];
			Instrument* pInstr = pInstrList->get(nInstr);
			assert(pInstr);

			pLine->setNumber(nInstr);
			pLine->setName( pInstr->get_name() );
			pLine->setSelected( nInstr == nSelectedInstr );
			pLine->setMuted( pInstr->is_muted() );
			if ( mixer ) {
				pLine->setSoloed( mixer->isSoloClicked( nInstr ) );
			}

		}
	}

}




void PatternEditorInstrumentList::dragEnterEvent(QDragEnterEvent *event)
{
	INFOLOG( "[dragEnterEvent]" );
	if ( event->mimeData()->hasFormat("text/plain") ) {
		Song *song = (Hydrogen::get_instance())->getSong();
		int nInstruments = song->get_instrument_list()->get_size();
		if ( nInstruments < MAX_INSTRUMENTS ) {
			event->acceptProposedAction();
		}
	}
}


void PatternEditorInstrumentList::dropEvent(QDropEvent *event)
{
	//WARNINGLOG("Drop!");
	QString sText = event->mimeData()->text();
	//ERRORLOG(sText);
	

	if(sText.startsWith("Songs:") || sText.startsWith("Patterns:") || sText.startsWith("move pattern:") || sText.startsWith("drag pattern:")) return;

	if (sText.startsWith("move instrument:")) {

		Hydrogen *engine = Hydrogen::get_instance();
		int nSourceInstrument = engine->getSelectedInstrumentNumber();

		int nTargetInstrument = event->pos().y() / m_nGridHeight;

		if ( nSourceInstrument == nTargetInstrument ) {
			event->acceptProposedAction();
			return;
		}

		moveInstrumentLine( nSourceInstrument , nTargetInstrument );

		event->acceptProposedAction();
	}
	if( sText.startsWith("importInstrument:") ) {
		//an instrument was dragged from the soundlibrary browser to the patterneditor

		sText = sText.remove(0,QString("importInstrument:").length());

		QStringList tokens = sText.split( "::" );
		QString sDrumkitName = tokens.at( 0 );
		QString sInstrumentName = tokens.at( 1 );
		
		Instrument *pNewInstrument = Instrument::load_instrument( sDrumkitName, sInstrumentName );
		if( pNewInstrument == NULL ) return;		

		Hydrogen *pEngine = Hydrogen::get_instance();

		// create a new valid ID for this instrument
		int nID = -1;
		for ( uint i = 0; i < pEngine->getSong()->get_instrument_list()->get_size(); ++i ) {
			Instrument* pInstr = pEngine->getSong()->get_instrument_list()->get( i );
			if ( pInstr->get_id().toInt() > nID ) {
				nID = pInstr->get_id().toInt();
			}
		}
		++nID;

		pNewInstrument->set_id( QString("%1").arg( nID ) );

		AudioEngine::get_instance()->lock( RIGHT_HERE );
		pEngine->getSong()->get_instrument_list()->add( pNewInstrument );

		#ifdef JACK_SUPPORT
		pEngine->renameJackPorts();
		#endif

		AudioEngine::get_instance()->unlock();

	
		int nTargetInstrument = event->pos().y() / m_nGridHeight;

		/*
		    "X > 181": border between the instrument names on the left and the grid
		    Because the right part of the grid starts above the name column, we have to subtract the difference 
		*/

		if (  event->pos().x() > 181 ) nTargetInstrument = ( event->pos().y() - 90 )  / m_nGridHeight ;

		//move instrument to the position where it was dropped
		moveInstrumentLine(pEngine->getSong()->get_instrument_list()->get_size() - 1 , nTargetInstrument );



		// select the new instrument
		pEngine->setSelectedInstrumentNumber(nTargetInstrument);

		event->acceptProposedAction();
	}
}



void PatternEditorInstrumentList::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		__drag_start_position = event->pos();
	}

}



void PatternEditorInstrumentList::mouseMoveEvent(QMouseEvent *event)
{
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}
	if ( abs(event->pos().y() - __drag_start_position.y()) < (int)m_nGridHeight) {
		return;
	}

	Hydrogen *pEngine = Hydrogen::get_instance();
	int nSelectedInstr = pEngine->getSelectedInstrumentNumber();
	Instrument *pInstr = pEngine->getSong()->get_instrument_list()->get(nSelectedInstr);

	QString sText = QString("move instrument:%1").arg( pInstr->get_name() );

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData);
	//drag->setPixmap(iconPixmap);

	pDrag->start( Qt::CopyAction | Qt::MoveAction );

	// propago l'evento
	QWidget::mouseMoveEvent(event);
}



