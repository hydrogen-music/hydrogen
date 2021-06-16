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

#include <core/AudioEngine.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/LocalFileMng.h>
using namespace H2Core;

#include "CommonStrings.h"
#include "UndoActions.h"
#include "PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "DrumPatternEditor.h"
#include "../HydrogenApp.h"
#include "../Mixer/Mixer.h"
#include "../Widgets/Button.h"

#include <QtGui>
#include <QtWidgets>
#include <QClipboard>

#include <cassert>
#include <algorithm> // for std::min

const char* InstrumentLine::__class_name = "InstrumentLine";

InstrumentLine::InstrumentLine(QWidget* pParent)
	: PixmapWidget(pParent, __class_name)
	, m_bIsSelected(false)
{
	int h = Preferences::get_instance()->getPatternEditorGridHeight();
	setFixedSize(181, h);

	m_lastUsedFontSize = Preferences::get_instance()->getFontSize();	
	QFont nameFont( Preferences::get_instance()->getLevel2FontFamily(), getPointSize( m_lastUsedFontSize ) );
	nameFont.setBold( true );

	m_pNameLbl = new QLabel(this);
	m_pNameLbl->resize( 145, h );
	m_pNameLbl->move( 10, 1 );
	m_pNameLbl->setFont(nameFont);

	/*: Text displayed on the button for muting an instrument. Its
	  size is designed for a single character.*/
	m_pMuteBtn = new Button( this, QSize( 18, 13 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getSmallMuteButton(), true, QSize(), tr("Mute instrument") );
	m_pMuteBtn->move( 145, 5 );
	m_pMuteBtn->setChecked(false);
	m_pMuteBtn->setObjectName( "MuteButton" );
	connect(m_pMuteBtn, SIGNAL( pressed() ), this, SLOT( muteClicked() ));

	/*: Text displayed on the button for soloing an instrument. Its
	  size is designed for a single character.*/
	m_pSoloBtn = new Button( this, QSize( 18, 13 ), Button::Type::Toggle, "", HydrogenApp::get_instance()->getCommonStrings()->getSmallSoloButton(), false, QSize(), tr("Solo") );
	m_pSoloBtn->move( 163, 5 );
	m_pSoloBtn->setChecked(false);
	m_pSoloBtn->setObjectName( "SoloButton" );
	connect(m_pSoloBtn, SIGNAL( pressed() ), this, SLOT(soloClicked()));

	m_pSampleWarning = new Button( this, QSize( 15, 13 ), Button::Type::Push, "warning.svg", "", false, QSize(), tr( "Some samples for this instrument failed to load." ) );
	m_pSampleWarning->move( 128, 5 );
	m_pSampleWarning->hide();
	connect(m_pSampleWarning, SIGNAL( pressed() ), this, SLOT( sampleWarningClicked() ));


	// Popup menu
	m_pFunctionPopup = new QMenu( this );
	m_pFunctionPopup->addAction( tr( "Delete notes" ), this, SLOT( functionClearNotes() ) );

	m_pFunctionPopupSub = new QMenu( tr( "Fill notes ..." ), m_pFunctionPopup );
	m_pFunctionPopupSub->addAction( tr( "Fill all notes" ), this, SLOT( functionFillAllNotes() ) );
	m_pFunctionPopupSub->addAction( tr( "Fill 1/2 notes" ), this, SLOT( functionFillEveryTwoNotes() ) );
	m_pFunctionPopupSub->addAction( tr( "Fill 1/3 notes" ), this, SLOT( functionFillEveryThreeNotes() ) );
	m_pFunctionPopupSub->addAction( tr( "Fill 1/4 notes" ), this, SLOT( functionFillEveryFourNotes() ) );
	m_pFunctionPopupSub->addAction( tr( "Fill 1/6 notes" ), this, SLOT( functionFillEverySixNotes() ) );
	m_pFunctionPopupSub->addAction( tr( "Fill 1/8 notes" ), this, SLOT( functionFillEveryEightNotes() ) );
	m_pFunctionPopupSub->addAction( tr( "Fill 1/12 notes" ), this, SLOT( functionFillEveryTwelveNotes() ) );
	m_pFunctionPopupSub->addAction( tr( "Fill 1/16 notes" ), this, SLOT( functionFillEverySixteenNotes() ) );
	m_pFunctionPopup->addMenu( m_pFunctionPopupSub );

	m_pFunctionPopup->addAction( tr( "Randomize velocity" ), this, SLOT( functionRandomizeVelocity() ) );
	m_pFunctionPopup->addAction( tr( "Select notes" ), this, &InstrumentLine::selectInstrumentNotes );

	m_pFunctionPopup->addSection( tr( "Edit all patterns" ) );
	m_pFunctionPopup->addAction( tr( "Cut notes"), this, SLOT( functionCutNotesAllPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Copy notes"), this, SLOT( functionCopyAllInstrumentPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Paste notes" ), this, SLOT( functionPasteAllInstrumentPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Delete notes" ), this, SLOT( functionDeleteNotesAllPatterns() ) );

	m_pFunctionPopup->addSection( tr( "Instrument" ) );
	m_pFunctionPopup->addAction( tr( "Rename instrument" ), this, SLOT( functionRenameInstrument() ) );
	m_pFunctionPopup->addAction( tr( "Delete instrument" ), this, SLOT( functionDeleteInstrument() ) );
	m_pFunctionPopup->setObjectName( "PatternEditorFunctionPopup" );

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
	m_pMuteBtn->setChecked(isMuted);
}


void InstrumentLine::setSoloed( bool soloed )
{
	m_pSoloBtn->setChecked( soloed );
}


void InstrumentLine::setSamplesMissing( bool bSamplesMissing )
{
	if ( bSamplesMissing ) {
		m_pSampleWarning->show();
	} else {
		m_pSampleWarning->hide();
	}
}



void InstrumentLine::muteClicked()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();
	auto pInstr = pInstrList->get( m_nInstrumentNumber );
	pHydrogen->setSelectedInstrumentNumber( m_nInstrumentNumber );

	CoreActionController* pCoreActionController = pHydrogen->getCoreActionController();
	pCoreActionController->setStripIsMuted( m_nInstrumentNumber, !pInstr->is_muted() );
}



void InstrumentLine::soloClicked()
{
	HydrogenApp::get_instance()->getMixer()->soloClicked( m_nInstrumentNumber );
}

void InstrumentLine::sampleWarningClicked()
{
	QMessageBox::information( this, "Hydrogen",
							  tr( "One or more samples for this instrument failed to load. This may be because the"
								  " songfile uses an older default drumkit. This might be fixed by opening a new "
								  "drumkit." ) );
}

void InstrumentLine::selectInstrumentNotes()
{
	HydrogenApp::get_instance()->getPatternEditorPanel()->selectInstrumentNotes( m_nInstrumentNumber );
}

void InstrumentLine::mousePressEvent(QMouseEvent *ev)
{
	Hydrogen::get_instance()->setSelectedInstrumentNumber( m_nInstrumentNumber );
	HydrogenApp::get_instance()->getPatternEditorPanel()->updatePianorollEditor();

	if ( ev->button() == Qt::LeftButton ) {
		const int width = m_pMuteBtn->x() - 5; // clickable field width
		const float velocity = std::min((float)ev->x()/(float)width, 1.0f);
		const float fPan = 0.f;
		const int nLength = -1;

		Song *pSong = Hydrogen::get_instance()->getSong();
		auto pInstr = pSong->getInstrumentList()->get( m_nInstrumentNumber );
		const float fPitch = pInstr->get_pitch_offset();

		Note *pNote = new Note( pInstr, 0, velocity, fPan, nLength, fPitch);
		Hydrogen::get_instance()->getAudioEngine()->getSampler()->noteOn(pNote);
	}
	else if (ev->button() == Qt::RightButton ) {
		m_pFunctionPopup->popup( QPoint( ev->globalX(), ev->globalY() ) );
	}

	// propago l'evento al parent: serve per il drag&drop
	PixmapWidget::mousePressEvent(ev);
}



H2Core::Pattern* InstrumentLine::getCurrentPattern()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	assert( pPatternList != nullptr );

	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	if ( nSelectedPatternNumber != -1 ) {
		Pattern* pCurrentPattern = pPatternList->get( nSelectedPatternNumber );
		return pCurrentPattern;
	}
	return nullptr;
}




void InstrumentLine::functionClearNotes()
{
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	int selectedPatternNr = pHydrogen->getSelectedPatternNumber();
	Pattern *pPattern = getCurrentPattern();
	auto pSelectedInstrument = pHydrogen->getSong()->getInstrumentList()->get( m_nInstrumentNumber );

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
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}
}


void InstrumentLine::functionCopyAllInstrumentPatterns()
{
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	Song *song = pHydrogen->getSong();
	assert(song);

	// Serialize & put to clipboard
	QString serialized = song->copyInstrumentLineToString( -1, m_nInstrumentNumber );
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(serialized);
}


void InstrumentLine::functionPasteAllInstrumentPatterns()
{
	functionPasteInstrumentPatternExec(-1);
}

void InstrumentLine::functionPasteInstrumentPatternExec(int patternID)
{
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	Song *song = pHydrogen->getSong();
	assert(song);

	// This is a note list for pasted notes collection
	std::list< Pattern* > patternList;

	// Get from clipboard & deserialize
	QClipboard *clipboard = QApplication::clipboard();
	QString serialized = clipboard->text();
	if ( !song->pasteInstrumentLineFromString( serialized, patternID, m_nInstrumentNumber, patternList ) ) {
		return;
	}

	// Ignore empty result
	if (patternList.size() <= 0) {
		return;
	}

	// Create action
	SE_pasteNotesPatternEditorAction *action = new SE_pasteNotesPatternEditorAction(patternList);
	HydrogenApp::get_instance()->m_pUndoStack->push(action);
}

void InstrumentLine::functionDeleteNotesAllPatterns()
{
	Song *pSong = Hydrogen::get_instance()->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	auto pSelectedInstrument = pSong->getInstrumentList()->get( m_nInstrumentNumber );
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

	pUndo->beginMacro( tr( "Delete all notes on %1" ).arg( pSelectedInstrument->get_name()  ) );
	for ( int nPattern = 0; nPattern < pPatternList->size(); nPattern++ ) {
		std::list< Note* > noteList;
		Pattern *pPattern = pPatternList->get( nPattern );
		const Pattern::notes_t* notes = pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END( notes, it) {
			if ( it->second->get_instrument() == pSelectedInstrument ) {
				noteList.push_back( it->second );
			}
		}
		if ( noteList.size() > 0 ) {
			pUndo->push( new SE_clearNotesPatternEditorAction( noteList, m_nInstrumentNumber, nPattern ) );
		}
	}
	pUndo->endMacro();
}

void InstrumentLine::functionCutNotesAllPatterns()
{
	functionCopyAllInstrumentPatterns();
	functionDeleteNotesAllPatterns();
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
	Hydrogen *pHydrogen = Hydrogen::get_instance();

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


	Song *pSong = pHydrogen->getSong();

	QStringList notePositions;

	Pattern* pCurrentPattern = getCurrentPattern();
	if (pCurrentPattern != nullptr) {
		int nPatternSize = pCurrentPattern->get_length();
		int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();

		if (nSelectedInstrument != -1) {
			auto instrRef = (pSong->getInstrumentList())->get( nSelectedInstrument );

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
			SE_fillNotesRightClickAction *action = new SE_fillNotesRightClickAction( notePositions, nSelectedInstrument, pHydrogen->getSelectedPatternNumber() );
			HydrogenApp::get_instance()->m_pUndoStack->push( action );
		}
	}

}



void InstrumentLine::functionRandomizeVelocity()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();

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

	Song *pSong = pHydrogen->getSong();

	QStringList noteVeloValue;
	QStringList oldNoteVeloValue;

	Pattern* pCurrentPattern = getCurrentPattern();
	if (pCurrentPattern != nullptr) {
		int nPatternSize = pCurrentPattern->get_length();
		int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();

		if (nSelectedInstrument != -1) {
			auto instrRef = (pSong->getInstrumentList())->get( nSelectedInstrument );

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
			SE_randomVelocityRightClickAction *action = new SE_randomVelocityRightClickAction( noteVeloValue, oldNoteVeloValue, nSelectedInstrument, pHydrogen->getSelectedPatternNumber() );
			HydrogenApp::get_instance()->m_pUndoStack->push( action );
		}
	}
}



void InstrumentLine::functionRenameInstrument()
{
	// This code is pretty much a duplicate of void InstrumentEditor::labelClicked
	// in InstrumentEditor.cpp
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	auto pSelectedInstrument = pHydrogen->getSong()->getInstrumentList()->get( m_nInstrumentNumber );

	QString sOldName = pSelectedInstrument->get_name();
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );
	if ( bIsOkPressed  ) {
		pSelectedInstrument->set_name( sNewName );

#ifdef H2CORE_HAVE_JACK
		pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
		Hydrogen *engine = Hydrogen::get_instance();
		engine->renameJackPorts(engine->getSong());
		pHydrogen->getAudioEngine()->unlock();
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
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	Song* pSong = pHydrogen->getSong();
		
	auto pSelectedInstrument = pSong->getInstrumentList()->get( m_nInstrumentNumber );
	std::list< Note* > noteList;

	QString sInstrumentName =  pSelectedInstrument->get_name();
	QString sDrumkitName = pHydrogen->getCurrentDrumkitName();

	for ( int i = 0; i < pSong->getPatternList()->size(); i++ ) {
		H2Core::Pattern *pPattern = pSong->getPatternList()->get(i);
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
	SE_deleteInstrumentAction *action = new SE_deleteInstrumentAction( noteList, sDrumkitName, sInstrumentName, m_nInstrumentNumber );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}

void InstrumentLine::onPreferencesChanged( bool bAppearanceOnly ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( m_pNameLbl->font().family() != pPref->getLevel2FontFamily() ||
		 m_lastUsedFontSize != pPref->getFontSize() ) {
		m_lastUsedFontSize = Preferences::get_instance()->getFontSize();
		m_pNameLbl->setFont( QFont( pPref->getLevel2FontFamily(), getPointSize( m_lastUsedFontSize ) ) );
	}
}


//////

const char* PatternEditorInstrumentList::__class_name = "PatternEditorInstrumentList";

PatternEditorInstrumentList::PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel )
 : QWidget( parent )
 , Object( __class_name )
{
	//INFOLOG("INIT");
	m_pPattern = nullptr;
	m_pPatternEditorPanel = pPatternEditorPanel;

	m_nGridHeight = Preferences::get_instance()->getPatternEditorGridHeight();

	m_nEditorWidth = 181;
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;

	resize( m_nEditorWidth, m_nEditorHeight );


	setAcceptDrops(true);

	for ( int i = 0; i < MAX_INSTRUMENTS; ++i) {
		m_pInstrumentLine[i] = nullptr;
	}


	updateInstrumentLines();

	m_pUpdateTimer = new QTimer( this );
	connect( m_pUpdateTimer, SIGNAL( timeout() ), this, SLOT( updateInstrumentLines() ) );
	m_pUpdateTimer->start(50);

	QScrollArea *pScrollArea = dynamic_cast< QScrollArea *>( parentWidget()->parentWidget() );
	assert( pScrollArea );
	m_pDragScroller = new DragScroller( pScrollArea );
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
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, pLine, &InstrumentLine::onPreferencesChanged );
	return pLine;
}



///
/// Update every InstrumentLine, create or destroy lines if necessary.
///
void PatternEditorInstrumentList::updateInstrumentLines()
{
	//INFOLOG( "Update lines" );

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrList = pSong->getInstrumentList();

	unsigned nSelectedInstr = pHydrogen->getSelectedInstrumentNumber();

	unsigned nInstruments = pInstrList->size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {
		if ( nInstr >= nInstruments ) {	// unused instrument! let's hide and destroy the mixerline!
			if ( m_pInstrumentLine[ nInstr ] ) {
				delete m_pInstrumentLine[ nInstr ];
				m_pInstrumentLine[ nInstr ] = nullptr;

				int newHeight = m_nGridHeight * nInstruments;
				resize( width(), newHeight );

			}
			continue;
		}
		else {
			if ( m_pInstrumentLine[ nInstr ] == nullptr ) {
				// the instrument line doesn't exists..I'll create a new one!
				m_pInstrumentLine[ nInstr ] = createInstrumentLine();
				m_pInstrumentLine[nInstr]->move( 0, m_nGridHeight * nInstr );
				m_pInstrumentLine[nInstr]->show();

				int newHeight = m_nGridHeight * nInstruments;
				resize( width(), newHeight );
			}
			InstrumentLine *pLine = m_pInstrumentLine[ nInstr ];
			auto pInstr = pInstrList->get(nInstr);
			assert(pInstr);

			pLine->setNumber(nInstr);
			pLine->setName( pInstr->get_name() );
			pLine->setSelected( nInstr == nSelectedInstr );
			pLine->setMuted( pInstr->is_muted() );
			pLine->setSoloed( pInstr->is_soloed() );

			pLine->setSamplesMissing( pInstr->has_missing_samples() );
		}
	}

}

void PatternEditorInstrumentList::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void PatternEditorInstrumentList::dropEvent(QDropEvent *event)
{
	//WARNINGLOG("Drop!");
	if ( ! event->mimeData()->hasFormat("text/plain") ) {
		event->ignore();
		return;
	}
	
	Song* pSong = Hydrogen::get_instance()->getSong();
	int nInstruments = pSong->getInstrumentList()->size();
	if ( nInstruments >= MAX_INSTRUMENTS ) {
		event->ignore();
		QMessageBox::critical( this, "Hydrogen", tr( "Unable to insert further instruments. Maximum possible number" ) +
							   QString( ": %1" ).arg( MAX_INSTRUMENTS ) );
		return;
	}
	
	QString sText = event->mimeData()->text();
	

	if ( sText.startsWith("Songs:") ||
		 sText.startsWith("Patterns:") ||
		 sText.startsWith("move pattern:") ||
		 sText.startsWith("drag pattern:") ) {
		return;
	}

	if (sText.startsWith("move instrument:")) {

		Hydrogen *engine = Hydrogen::get_instance();
		int nSourceInstrument = engine->getSelectedInstrumentNumber();

		// Starting point for instument list is 50 lower than
		// on the drum pattern editor

		int pos_y = ( event->pos().x() >= m_nEditorWidth ) ? event->pos().y() - 50 : event->pos().y();

		int nTargetInstrument = pos_y / m_nGridHeight;

		if( nTargetInstrument >= engine->getSong()->getInstrumentList()->size() ){
			nTargetInstrument = engine->getSong()->getInstrumentList()->size() - 1;
		}

		if ( nSourceInstrument == nTargetInstrument ) {
			event->acceptProposedAction();
			return;
		}
		
		SE_moveInstrumentAction *action = new SE_moveInstrumentAction( nSourceInstrument, nTargetInstrument );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );

		event->acceptProposedAction();
	}
	if( sText.startsWith("importInstrument:") ) {
		//an instrument was dragged from the soundlibrary browser to the patterneditor

		sText = sText.remove(0,QString("importInstrument:").length());

		QStringList tokens = sText.split( "::" );
		QString sDrumkitScope = tokens.at( 0 );
		QString sDrumkitName = tokens.at( 1 );
		QString sInstrumentName = tokens.at( 2 );

		int nTargetInstrument = event->pos().y() / m_nGridHeight;

		/*
				"X > 181": border between the instrument names on the left and the grid
				Because the right part of the grid starts above the name column, we have to subtract the difference
		*/
		if (  event->pos().x() > 181 ) {
			nTargetInstrument = ( event->pos().y() - 90 )  / m_nGridHeight ;
		}

		Hydrogen *engine = Hydrogen::get_instance();
		if( nTargetInstrument > engine->getSong()->getInstrumentList()->size() ){
			nTargetInstrument = engine->getSong()->getInstrumentList()->size();
		}

		// Check whether the drumkit was chosen amongst the system's
		// or user's set.
		H2Core::Filesystem::Lookup lookup;
	  	if ( sDrumkitScope == "system" ) {
			lookup = H2Core::Filesystem::Lookup::system;
		} else {
			lookup = H2Core::Filesystem::Lookup::user;
		}

		SE_dragInstrumentAction *action = new SE_dragInstrumentAction( sDrumkitName, sInstrumentName, nTargetInstrument, lookup );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );

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

	Hydrogen *pHydrogen = Hydrogen::get_instance();
	int nSelectedInstr = pHydrogen->getSelectedInstrumentNumber();
	auto pInstr = pHydrogen->getSong()->getInstrumentList()->get(nSelectedInstr);

	QString sText = QString("move instrument:%1").arg( pInstr->get_name() );

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData);

	m_pDragScroller->startDrag();
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
	m_pDragScroller->endDrag();

	// propago l'evento
	QWidget::mouseMoveEvent(event);
}
