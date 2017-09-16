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
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/LocalFileMng.h>
using namespace H2Core;

#include "UndoActions.h"
#include "PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "DrumPatternEditor.h"
#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"
#include "../widgets/Button.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif
#include <QClipboard>
#include <cassert>
#include <algorithm> // for std::min

using namespace std;

const char* InstrumentLine::__class_name = "InstrumentLine";

InstrumentLine::InstrumentLine(QWidget* pParent)
	: PixmapWidget(pParent, __class_name)
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
			"/mixerPanel/btn_mute_on.png",
			"/mixerPanel/btn_mute_off.png",
			"/mixerPanel/btn_mute_off.png",
			QSize( 18, 13 )
	);
	m_pMuteBtn->move( 145, 5 );
	m_pMuteBtn->setPressed(false);
	m_pMuteBtn->setToolTip( trUtf8("Mute instrument") );
	connect(m_pMuteBtn, SIGNAL(clicked(Button*)), this, SLOT(muteClicked()));

	m_pSoloBtn = new ToggleButton(
			this,
			"/mixerPanel/btn_solo_on.png",
			"/mixerPanel/btn_solo_off.png",
			"/mixerPanel/btn_solo_off.png",
			QSize( 18, 13 )
	);
	m_pSoloBtn->move( 163, 5 );
	m_pSoloBtn->setPressed(false);
	m_pSoloBtn->setToolTip( trUtf8("Solo") );
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
	m_pFunctionPopupSub->addAction( trUtf8( "Fill 1/12 notes" ), this, SLOT( functionFillEveryTwelveNotes() ) );
	m_pFunctionPopupSub->addAction( trUtf8( "Fill 1/16 notes" ), this, SLOT( functionFillEverySixteenNotes() ) );
	m_pFunctionPopup->addMenu( m_pFunctionPopupSub );

	m_pFunctionPopup->addAction( trUtf8( "Randomize velocity" ), this, SLOT( functionRandomizeVelocity() ) );
	m_pFunctionPopup->addSeparator();

	m_pCopyPopupSub = new QMenu( trUtf8( "Copy notes ..." ), m_pFunctionPopup );
	m_pCopyPopupSub->addAction( trUtf8( "Only for this pattern" ), this, SLOT( functionCopyInstrumentPattern() ) );
	m_pCopyPopupSub->addAction( trUtf8( "For all patterns" ), this, SLOT( functionCopyAllInstrumentPatterns() ) );
	m_pFunctionPopup->addMenu( m_pCopyPopupSub );

	m_pPastePopupSub = new QMenu( trUtf8( "Paste notes ..." ), m_pFunctionPopup );
	m_pPastePopupSub->addAction( trUtf8( "Only for this pattern" ), this, SLOT( functionPasteInstrumentPattern() ) );
	m_pPastePopupSub->addAction( trUtf8( "For all patterns" ), this, SLOT( functionPasteAllInstrumentPatterns() ) );
	m_pFunctionPopup->addMenu( m_pPastePopupSub );

	m_pFunctionPopup->addSeparator();
	m_pFunctionPopup->addAction( trUtf8( "Rename instrument" ), this, SLOT( functionRenameInstrument() ) );
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
	Hydrogen *pEngine = Hydrogen::get_instance();
	Song *pSong = pEngine->getSong();
	InstrumentList *pInstrList = pSong->get_instrument_list();
	Instrument *pInstr = pInstrList->get( m_nInstrumentNumber );

	CoreActionController* pCoreActionController = pEngine->getCoreActionController();
	pCoreActionController->setStripIsMuted( m_nInstrumentNumber, !pInstr->is_muted() );
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
		const int width = m_pMuteBtn->x() - 5; // clickable field width
		const float velocity = std::min((float)ev->x()/(float)width, 1.0f);
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
	Hydrogen * pEngine = Hydrogen::get_instance();
	int selectedPatternNr = pEngine->getSelectedPatternNumber();
	Pattern *pPattern = getCurrentPattern();
	Instrument *pSelectedInstrument = pEngine->getSong()->get_instrument_list()->get( m_nInstrumentNumber );

	std::list< Note* > noteList;
	const Pattern::notes_t* notes = pPattern->get_notes();
	FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
		Note *pNote = it->second;
		assert( pNote );
		if ( pNote->get_instrument() == pSelectedInstrument ) {
			noteList.push_back( pNote );
		}
	}
	if( noteList.size() > 0 ){
		SE_clearNotesPatternEditorAction *action = new SE_clearNotesPatternEditorAction( noteList, m_nInstrumentNumber,selectedPatternNr);
		HydrogenApp::get_instance()->m_undoStack->push( action );
	}
}

void InstrumentLine::functionCopyInstrumentPattern()
{
	Hydrogen * pEngine = Hydrogen::get_instance();
	int selectedPatternNr = pEngine->getSelectedPatternNumber();
	Song *song = pEngine->getSong();
	assert(song);

	// Serialize & put to clipboard
	QString serialized = LocalFileMng::copyInstrumentLineToString(song, selectedPatternNr, m_nInstrumentNumber);
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(serialized);
}

void InstrumentLine::functionCopyAllInstrumentPatterns()
{
	Hydrogen * pEngine = Hydrogen::get_instance();
	Song *song = pEngine->getSong();
	assert(song);

	// Serialize & put to clipboard
	QString serialized = LocalFileMng::copyInstrumentLineToString(song, -1, m_nInstrumentNumber);
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(serialized);
}

void InstrumentLine::functionPasteInstrumentPattern()
{
	Hydrogen * pEngine = Hydrogen::get_instance();
	int selectedPatternNr = pEngine->getSelectedPatternNumber();

	functionPasteInstrumentPatternExec(selectedPatternNr);
}

void InstrumentLine::functionPasteAllInstrumentPatterns()
{
	functionPasteInstrumentPatternExec(-1);
}

void InstrumentLine::functionPasteInstrumentPatternExec(int patternID)
{
	Hydrogen * pEngine = Hydrogen::get_instance();
	Song *song = pEngine->getSong();
	assert(song);

	// This is a note list for pasted notes collection
	std::list< Pattern* > patternList;

	// Get from clipboard & deserialize
	QClipboard *clipboard = QApplication::clipboard();
	QString serialized = clipboard->text();
	if (!LocalFileMng::pasteInstrumentLineFromString(song, serialized, patternID, m_nInstrumentNumber, patternList))
		return;

	// Ignore empty result
	if (patternList.size() <= 0)
		return;

	// Create action
	SE_pasteNotesPatternEditorAction *action = new SE_pasteNotesPatternEditorAction(patternList);
	HydrogenApp::get_instance()->m_undoStack->push(action);
}


void InstrumentLine::functionFillAllNotes(){ functionFillNotes(1); }
void InstrumentLine::functionFillEveryTwoNotes(){ functionFillNotes(2); }
void InstrumentLine::functionFillEveryThreeNotes(){ functionFillNotes(3); }
void InstrumentLine::functionFillEveryFourNotes(){ functionFillNotes(4); }
void InstrumentLine::functionFillEverySixNotes(){ functionFillNotes(6); }
void InstrumentLine::functionFillEveryEightNotes(){ functionFillNotes(8); }
void InstrumentLine::functionFillEveryTwelveNotes(){ functionFillNotes(12); }
void InstrumentLine::functionFillEverySixteenNotes(){ functionFillNotes(16); }

void InstrumentLine::functionFillNotes( int every )
{
	Hydrogen *pEngine = Hydrogen::get_instance();

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


	Song *pSong = pEngine->getSong();

	QStringList notePositions;

	Pattern* pCurrentPattern = getCurrentPattern();
	if (pCurrentPattern != NULL) {
		int nPatternSize = pCurrentPattern->get_length();
		int nSelectedInstrument = pEngine->getSelectedInstrumentNumber();

		if (nSelectedInstrument != -1) {
			Instrument *instrRef = (pSong->get_instrument_list())->get( nSelectedInstrument );

			for (int i = 0; i < nPatternSize; i += nResolution) {
				bool noteAlreadyPresent = false;
				const Pattern::notes_t* notes = pCurrentPattern->get_notes();
				FOREACH_NOTE_CST_IT_BOUND(notes,it,i) {
					Note *pNote = it->second;
					if ( pNote->get_instrument() == instrRef ) {
						// note already exists
						noteAlreadyPresent = true;
						break;
					}
				}

				if ( noteAlreadyPresent == false ) {
					notePositions << QString("%1").arg(i);
				}
			}
			SE_fillNotesRightClickAction *action = new SE_fillNotesRightClickAction( notePositions, nSelectedInstrument, pEngine->getSelectedPatternNumber() );
			HydrogenApp::get_instance()->m_undoStack->push( action );
		}
	}

}



void InstrumentLine::functionRandomizeVelocity()
{
	Hydrogen *pEngine = Hydrogen::get_instance();

	PatternEditorPanel *pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();
	DrumPatternEditor *pPatternEditor = pPatternEditorPanel->getDrumPatternEditor();


	int nBase;
	if ( pPatternEditor->isUsingTriplets() ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int nResolution = 4 * MAX_NOTES / ( nBase * pPatternEditor->getResolution() );

	Song *pSong = pEngine->getSong();

	QStringList noteVeloValue;
	QStringList oldNoteVeloValue;

	Pattern* pCurrentPattern = getCurrentPattern();
	if (pCurrentPattern != NULL) {
		int nPatternSize = pCurrentPattern->get_length();
		int nSelectedInstrument = pEngine->getSelectedInstrumentNumber();

		if (nSelectedInstrument != -1) {
			Instrument *instrRef = (pSong->get_instrument_list())->get( nSelectedInstrument );

			for (int i = 0; i < nPatternSize; i += nResolution) {
				const Pattern::notes_t* notes = pCurrentPattern->get_notes();
				FOREACH_NOTE_CST_IT_BOUND(notes,it,i) {
					Note *pNote = it->second;
					if ( pNote->get_instrument() == instrRef ) {
						float fVal = ( rand() % 100 ) / 100.0;
						oldNoteVeloValue <<  QString("%1").arg( pNote->get_velocity() );
						fVal = pNote->get_velocity() + ( ( fVal - 0.50 ) / 2 );
						if ( fVal < 0  ) {
							fVal = 0;
						}
						if ( fVal > 1 ) {
							fVal = 1;
						}
						noteVeloValue << QString("%1").arg(fVal);
					}
				}
			}
			SE_randomVelocityRightClickAction *action = new SE_randomVelocityRightClickAction( noteVeloValue, oldNoteVeloValue, nSelectedInstrument, pEngine->getSelectedPatternNumber() );
			HydrogenApp::get_instance()->m_undoStack->push( action );
		}
	}
}



void InstrumentLine::functionRenameInstrument()
{
	// This code is pretty much a duplicate of void InstrumentEditor::labelClicked
	// in InstrumentEditor.cpp
	Hydrogen * pEngine = Hydrogen::get_instance();
	Instrument *pSelectedInstrument = pEngine->getSong()->get_instrument_list()->get( m_nInstrumentNumber );

	QString sOldName = pSelectedInstrument->get_name();
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", trUtf8( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );
	if ( bIsOkPressed  ) {
		pSelectedInstrument->set_name( sNewName );

#ifdef H2CORE_HAVE_JACK
		AudioEngine::get_instance()->lock( RIGHT_HERE );
		Hydrogen *engine = Hydrogen::get_instance();
		engine->renameJackPorts(engine->getSong());
		AudioEngine::get_instance()->unlock();
#endif

		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	}
	else
	{
		// user entered nothing or pressed Cancel
	}

}

void InstrumentLine::functionDeleteInstrument()
{
	Hydrogen * pEngine = Hydrogen::get_instance();
	Instrument *pSelectedInstrument = pEngine->getSong()->get_instrument_list()->get( m_nInstrumentNumber );

	std::list< Note* > noteList;
	Song* song = pEngine->getSong();
	PatternList *patList = song->get_pattern_list();

	QString instrumentName =  pSelectedInstrument->get_name();
	QString drumkitName = pEngine->getCurrentDrumkitname();

	for ( int i = 0; i < patList->size(); i++ ) {
		H2Core::Pattern *pPattern = song->get_pattern_list()->get(i);
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() == pSelectedInstrument ) {
				pNote->set_pattern_idx( i );
				noteList.push_back( pNote );
			}
		}
	}
	SE_deleteInstrumentAction *action = new SE_deleteInstrumentAction( noteList, drumkitName, instrumentName, m_nInstrumentNumber );
	HydrogenApp::get_instance()->m_undoStack->push( action );
}



//////

const char* PatternEditorInstrumentList::__class_name = "PatternEditorInstrumentList";

PatternEditorInstrumentList::PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel )
 : QWidget( parent )
 , Object( __class_name )
{
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

	unsigned nInstruments = pInstrList->size();
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
		int nInstruments = song->get_instrument_list()->size();
		if ( nInstruments < MAX_INSTRUMENTS ) {
			event->acceptProposedAction();
		}
	}
}


void PatternEditorInstrumentList::dropEvent(QDropEvent *event)
{
	//WARNINGLOG("Drop!");
	QString sText = event->mimeData()->text();


	if(sText.startsWith("Songs:") || sText.startsWith("Patterns:") || sText.startsWith("move pattern:") || sText.startsWith("drag pattern:")) return;

	if (sText.startsWith("move instrument:")) {

		Hydrogen *engine = Hydrogen::get_instance();
		int nSourceInstrument = engine->getSelectedInstrumentNumber();

		// Starting point for instument list is 50 lower than
		// on the drum pattern editor

		int pos_y = ( event->pos().x() >= m_nEditorWidth ) ? event->pos().y() - 50 : event->pos().y();

		int nTargetInstrument = pos_y / m_nGridHeight;

		if( nTargetInstrument >= engine->getSong()->get_instrument_list()->size() ){
			nTargetInstrument = engine->getSong()->get_instrument_list()->size() - 1;
		}

		if ( nSourceInstrument == nTargetInstrument ) {
			event->acceptProposedAction();
			return;
		}

		SE_moveInstrumentAction *action = new SE_moveInstrumentAction( nSourceInstrument, nTargetInstrument );
		HydrogenApp::get_instance()->m_undoStack->push( action );

		event->acceptProposedAction();
	}
	if( sText.startsWith("importInstrument:") ) {
		//an instrument was dragged from the soundlibrary browser to the patterneditor

		sText = sText.remove(0,QString("importInstrument:").length());

		QStringList tokens = sText.split( "::" );
		QString sDrumkitName = tokens.at( 0 );
		QString sInstrumentName = tokens.at( 1 );

		int nTargetInstrument = event->pos().y() / m_nGridHeight;

		/*
				"X > 181": border between the instrument names on the left and the grid
				Because the right part of the grid starts above the name column, we have to subtract the difference
		*/
		if (  event->pos().x() > 181 ) nTargetInstrument = ( event->pos().y() - 90 )  / m_nGridHeight ;

		Hydrogen *engine = Hydrogen::get_instance();
		if( nTargetInstrument > engine->getSong()->get_instrument_list()->size() ){
			nTargetInstrument = engine->getSong()->get_instrument_list()->size();
		}

		SE_dragInstrumentAction *action = new SE_dragInstrumentAction( sDrumkitName, sInstrumentName, nTargetInstrument);
		HydrogenApp::get_instance()->m_undoStack->push( action );

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

	pDrag->start( Qt::CopyAction | Qt::MoveAction );

	// propago l'evento
	QWidget::mouseMoveEvent(event);
}
