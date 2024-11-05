/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2024 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "PatternEditorInstrumentList.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/CoreActionController.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>
using namespace H2Core;

#include "CommonStrings.h"
#include "UndoActions.h"
#include "PatternEditorPanel.h"
#include "InstrumentEditor/InstrumentEditorPanel.h"
#include "DrumPatternEditor.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "../Widgets/Button.h"
#include "../Skin.h"

#include <QtGui>
#include <QtWidgets>
#include <QClipboard>

#include <cassert>
#include <algorithm> // for std::min


InstrumentLine::InstrumentLine(QWidget* pParent)
	: PixmapWidget(pParent)
	, m_bIsSelected( false )
	, m_bEntered( false )
{
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	const auto pPref = H2Core::Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	int h = pPref->getPatternEditorGridHeight();
	setFixedSize(181, h);

	QFont nameFont( pPref->getTheme().m_font.m_sLevel2FontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );

	m_pNameLbl = new QLabel(this);
	m_pNameLbl->resize( 145, h );
	m_pNameLbl->move( 10, 1 );
	m_pNameLbl->setFont(nameFont);

	/*: Text displayed on the button for muting an instrument. Its
	  size is designed for a single character.*/
	m_pMuteBtn = new Button( this, QSize( InstrumentLine::m_nButtonWidth, height() - 1 ),
							 Button::Type::Toggle, "",
							 pCommonStrings->getSmallMuteButton(),
							 true, QSize(), tr("Mute instrument"),
							 false, true );
	m_pMuteBtn->move( 145, 0 );
	m_pMuteBtn->setChecked(false);
	m_pMuteBtn->setObjectName( "InstrumentLineMuteButton" );
	connect(m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteClicked() ));

	/*: Text displayed on the button for soloing an instrument. Its
	  size is designed for a single character.*/
	m_pSoloBtn = new Button( this, QSize( InstrumentLine::m_nButtonWidth, height() - 1 ),
							 Button::Type::Toggle, "",
							 pCommonStrings->getSmallSoloButton(),
							 false, QSize(), tr("Solo"),
							 false, true );
	m_pSoloBtn->move( 163, 0 );
	m_pSoloBtn->setChecked(false);
	m_pSoloBtn->setObjectName( "InstrumentLineSoloButton" );
	connect(m_pSoloBtn, SIGNAL( clicked() ), this, SLOT(soloClicked()));

	m_pSampleWarning = new Button( this, QSize( 15, 13 ), Button::Type::Icon,
								   "warning.svg", "", false, QSize(),
								   tr( "Some samples for this instrument failed to load." ),
								   true );
	m_pSampleWarning->move( 128, 5 );
	m_pSampleWarning->hide();
	connect(m_pSampleWarning, SIGNAL( clicked() ), this, SLOT( sampleWarningClicked() ));


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

	auto selectNotesAction = m_pFunctionPopup->addAction( tr( "Select notes" ) );
	connect( selectNotesAction, &QAction::triggered, this,
			 &InstrumentLine::selectInstrumentNotes );

	m_pFunctionPopup->addSection( tr( "Edit all patterns" ) );
	m_pFunctionPopup->addAction( tr( "Cut notes"), this, SLOT( functionCutNotesAllPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Copy notes"), this, SLOT( functionCopyAllInstrumentPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Paste notes" ), this, SLOT( functionPasteAllInstrumentPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Delete notes" ), this, SLOT( functionDeleteNotesAllPatterns() ) );

	m_pFunctionPopup->addSection( tr( "Instrument" ) );
	m_pFunctionPopup->addAction( pCommonStrings->getActionAddInstrument(),
								 HydrogenApp::get_instance()->getMainForm(),
								 SLOT( action_drumkit_addInstrument() ) );
	m_pFunctionPopup->addAction( tr( "Rename instrument" ), this, SLOT( functionRenameInstrument() ) );
	auto deleteAction =
		m_pFunctionPopup->addAction( pCommonStrings->getActionDeleteInstrument() );
	connect( deleteAction, &QAction::triggered, this, [=](){
		HydrogenApp::get_instance()->getMainForm()->
			functionDeleteInstrument( m_nInstrumentNumber );} );
	m_pFunctionPopup->setObjectName( "PatternEditorFunctionPopup" );

	updateStyleSheet();
}

void InstrumentLine::set( std::shared_ptr<Instrument> pInstrument ) {
	if ( pInstrument == nullptr ) {
		ERRORLOG( "Imvalid instrument" );
		return;
	}

	setName( pInstrument->get_name() );
	setMuted( pInstrument->is_muted() );
	setSoloed( pInstrument->is_soloed() );
	setSamplesMissing( pInstrument->has_missing_samples() );

	// Create a tool tip uniquely stating the drumkit the instrument belongs to.
	QString sToolTip( pInstrument->get_name() );
	if ( ! pInstrument->get_drumkit_path().isEmpty() ) {
		// Instrument belongs to a kit in the SoundLibrary (and was not created
		// anew).
		QString sKit = Hydrogen::get_instance()->getSoundLibraryDatabase()->
			getUniqueLabel( pInstrument->get_drumkit_path() );
		if ( sKit.isEmpty() ) {
			// This should not happen. But drumkit.xml files can be created by
			// hand and we should account for it.
			sKit = pInstrument->get_drumkit_path();
		}

		/*: Shown in a tooltop and indicating the drumkit (to the right of this
		 *  string) an instrument (to the left of this string) is loaded
		 *  from. */
		sToolTip.append( " (" ).append( tr( "imported from" ) )
			.append( QString( " [%1])" ).arg( sKit ) );
	}

	setToolTip( sToolTip );
}

void InstrumentLine::setName(const QString& sName)
{
	if ( m_pNameLbl->text() != sName ){
		m_pNameLbl->setText(sName);
	}
}

void InstrumentLine::setToolTip(const QString& sToolTip)
{
	if ( m_pNameLbl->toolTip() != sToolTip ){
		m_pNameLbl->setToolTip( sToolTip );
	}
}



void InstrumentLine::setSelected( bool bSelected )
{
	if ( bSelected == m_bIsSelected ) {
		return;
	}
	
	m_bIsSelected = bSelected;

	updateStyleSheet();
	update();
}

void InstrumentLine::updateStyleSheet() {

	const auto pPref = H2Core::Preferences::get_instance();

	QColor textColor;
	if ( m_bIsSelected ) {
		textColor = pPref->getTheme().m_color.m_patternEditor_selectedRowTextColor;
	} else {
		textColor = pPref->getTheme().m_color.m_patternEditor_textColor;
	}

	m_pNameLbl->setStyleSheet( QString( "\
QLabel {\
   color: %1;\
   font-weight: bold;\
 }" ).arg( textColor.name() ) );
}

void InstrumentLine::enterEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void InstrumentLine::leaveEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void InstrumentLine::paintEvent( QPaintEvent* ev ) {
	const auto pPref = Preferences::get_instance();
	auto pHydrogenApp = HydrogenApp::get_instance();
	
	QPainter painter(this);

	QColor backgroundColor;
	if ( m_bIsSelected ) {
		backgroundColor = pPref->getTheme().m_color.m_patternEditor_selectedRowColor.darker( 114 );
	} else {
		if ( m_nInstrumentNumber == 0 ||
			 m_nInstrumentNumber % 2 == 0 ) {
			backgroundColor = pPref->getTheme().m_color.m_patternEditor_backgroundColor.darker( 120 );
		} else {
			backgroundColor = pPref->getTheme().m_color.m_patternEditor_alternateRowColor.darker( 132 );
		}
	}

	// Make the background slightly lighter when hovered.
	bool bHovered = false;
	if ( m_bEntered ) {
		bHovered = true;
	}

	Skin::drawListBackground( &painter, QRect( 0, 0, width(), height() ),
							  backgroundColor, bHovered );

	// Draw border indicating cursor position
	if (  m_bIsSelected &&
		  m_pPatternEditorPanel->getDrumPatternEditor() != nullptr &&
		  m_pPatternEditorPanel->getDrumPatternEditor()->hasFocus() &&
		  ! pHydrogenApp->hideKeyboardCursor() ) {

		QPen pen;

		pen.setColor( pPref->getTheme().m_color.m_cursorColor );

		pen.setWidth( 2 );
		painter.setPen( pen );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawRoundedRect( QRect( 1, 1, width() - 2 * InstrumentLine::m_nButtonWidth - 1,
										height() - 2 ), 4, 4 );
	}
}


void InstrumentLine::setNumber(int nIndex)
{
	if ( m_nInstrumentNumber != nIndex ) {
		m_nInstrumentNumber = nIndex;
		update();
	}
}



void InstrumentLine::setMuted(bool isMuted)
{
	if ( ! m_pMuteBtn->isDown() &&
		 m_pMuteBtn->isChecked() != isMuted ) {
		m_pMuteBtn->setChecked(isMuted);
	}
}


void InstrumentLine::setSoloed( bool soloed )
{
	if ( ! m_pSoloBtn->isDown() &&
		 m_pSoloBtn->isChecked() != soloed ) {
		m_pSoloBtn->setChecked( soloed );
	}
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
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}
	
	auto pInstrList = pSong->getDrumkit()->getInstruments();
	auto pInstr = pInstrList->get( m_nInstrumentNumber );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument [%1]" )
				  .arg( m_nInstrumentNumber ) );
		return;
	}
	
	pHydrogen->setSelectedInstrumentNumber( m_nInstrumentNumber );

	H2Core::CoreActionController::setStripIsMuted(
		m_nInstrumentNumber, !pInstr->is_muted() );
}



void InstrumentLine::soloClicked()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No song set yet" );
		return;
	}
	
	auto pInstrList = pSong->getDrumkit()->getInstruments();
	auto pInstr = pInstrList->get( m_nInstrumentNumber );
	if ( pInstr == nullptr ) {
		ERRORLOG( QString( "Unable to retrieve instrument [%1]" )
				  .arg( m_nInstrumentNumber ) );
		return;
	}
	
	pHydrogen->setSelectedInstrumentNumber( m_nInstrumentNumber );

	H2Core::CoreActionController::setStripIsSoloed(
		m_nInstrumentNumber, !pInstr->is_soloed() );
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
	m_pPatternEditorPanel->selectInstrumentNotes( m_nInstrumentNumber );
}

void InstrumentLine::mousePressEvent(QMouseEvent *ev)
{
	m_pPatternEditorPanel->setSelectedRowDB( m_nInstrumentNumber );
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();

	if ( ev->button() == Qt::LeftButton ) {

		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
			ERRORLOG( "No song set yet" );
			return;
		}
		auto pInstr = pSong->getDrumkit()->getInstruments()->get( m_nInstrumentNumber );
		if ( pInstr != nullptr && pInstr->hasSamples() ) {

			const int nWidth = m_pMuteBtn->x() - 5; // clickable field width
			const float fVelocity = std::min((float)ev->x()/(float)nWidth, 1.0f);
			Note *pNote = new Note( pInstr, 0, fVelocity);
			Hydrogen::get_instance()->getAudioEngine()->getSampler()->noteOn(pNote);
		}
		
	}
	else if (ev->button() == Qt::RightButton ) {
		m_pFunctionPopup->popup( QPoint( ev->globalX(), ev->globalY() ) );
	}

	// propago l'evento al parent: serve per il drag&drop
	PixmapWidget::mousePressEvent(ev);
}

void InstrumentLine::mouseDoubleClickEvent( QMouseEvent* ev ) {
	functionRenameInstrument();
}

void InstrumentLine::functionClearNotes()
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}
	auto pSelectedInstrument =
		pSong->getDrumkit()->getInstruments()->get( m_nInstrumentNumber );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	std::list< Note* > noteList;
	const Pattern::notes_t* notes = pPattern->getNotes();
	FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
		Note *pNote = it->second;
		assert( pNote );
		if ( pNote->get_instrument() == pSelectedInstrument ) {
			noteList.push_back( pNote );
		}
	}
	if( noteList.size() > 0 ){
		SE_clearNotesPatternEditorAction *action =
			new SE_clearNotesPatternEditorAction(
				noteList,
				m_nInstrumentNumber,
				m_pPatternEditorPanel->getPatternNumber() );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}
}


void InstrumentLine::functionCopyAllInstrumentPatterns()
{
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready" );
		return;
	}

	const auto pSelectedInstrument =
		pSong->getDrumkit()->getInstruments()->get( m_nInstrumentNumber );
	if ( pSelectedInstrument == nullptr ) {
		WARNINGLOG( "No instrument selected" );
		return;
	}

	// Serialize & put to clipboard
	H2Core::XMLDoc doc;
	auto rootNode = doc.set_root( "serializedPatternList" );
	pSong->getPatternList()->save_to( rootNode, pSelectedInstrument );

	const QString sSerialized = doc.toString();
	if ( sSerialized.isEmpty() ) {
		ERRORLOG( QString( "Unable to serialize instrument line [%1]" )
				  .arg( m_nInstrumentNumber ) );
		return;
	}
	
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText( sSerialized );
}


void InstrumentLine::functionPasteAllInstrumentPatterns()
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready" );
		return;
	}

	// Get from clipboard & deserialize
	QClipboard *clipboard = QApplication::clipboard();
	const QString sSerialized = clipboard->text();
	if ( sSerialized.isEmpty() ) {
		INFOLOG( "Serialized pattern list is empty" );
		return;
	}

	const auto doc = H2Core::XMLDoc( sSerialized );
	const auto rootNode = doc.firstChildElement( "serializedPatternList" );
	if ( rootNode.isNull() ) {
		ERRORLOG( QString( "Unable to parse serialized pattern list [%1]" )
				  .arg( sSerialized ) );
		return;
	}

	const auto pPatternList = PatternList::load_from(
		rootNode, pSong->getDrumkit()->getExportName() );
	if ( pPatternList == nullptr ) {
		ERRORLOG( QString( "Unable to deserialized pattern list [%1]" )
				  .arg( sSerialized ) );
		return;
	}

	const auto pInstrumentList = pSong->getDrumkit()->getInstruments();

	// Those pattern contain only notes for a single instrument. This must be
	// replaced with the one belonging to this InstrumentLine. Or notes will end
	// up in the wrong row.
	for ( auto& ppPattern : *pPatternList ) {
		if ( ppPattern != nullptr ) {
			for ( auto& [ _, ppNote ] : *ppPattern->getNotes() ) {
				if ( ppNote != nullptr ) {
					ppNote->set_instrument_id( m_nInstrumentNumber );
					ppNote->setType(
						pInstrumentList->get( m_nInstrumentNumber )->getType() );
				}
			}
		}
	}

	// Ignore empty result
	if ( pPatternList->size() <= 0 ) {
		INFOLOG( "Deserialized pattern list is empty" );
		return;
	}

	// Create action
	SE_pasteNotesPatternEditorAction *action =
		new SE_pasteNotesPatternEditorAction( pPatternList );
	HydrogenApp::get_instance()->m_pUndoStack->push(action);
}

void InstrumentLine::functionDeleteNotesAllPatterns()
{
	std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
	PatternList *pPatternList = pSong->getPatternList();
	auto pSelectedInstrument = pSong->getDrumkit()->getInstruments()->get( m_nInstrumentNumber );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}
	QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

	pUndo->beginMacro( tr( "Delete all notes on %1" ).arg( pSelectedInstrument->get_name()  ) );
	for ( int nPattern = 0; nPattern < pPatternList->size(); nPattern++ ) {
		std::list< Note* > noteList;
		auto pPattern = pPatternList->get( nPattern );
		const Pattern::notes_t* notes = pPattern->getNotes();
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
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	DrumPatternEditor *pPatternEditor = m_pPatternEditorPanel->getDrumPatternEditor();
	int nBase;
	if ( pPatternEditor->isUsingTriplets() ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int nResolution = 4 * MAX_NOTES * every / ( nBase * pPatternEditor->getResolution() );

	QStringList notePositions;

	int nPatternSize = pPattern->getLength();
	auto pSelectedInstrument = m_pPatternEditorPanel->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}
	int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();

	for (int i = 0; i < nPatternSize; i += nResolution) {
		bool noteAlreadyPresent = false;
		const Pattern::notes_t* notes = pPattern->getNotes();
		FOREACH_NOTE_CST_IT_BOUND_LENGTH(notes,it,i,pPattern) {
			Note *pNote = it->second;
			if ( pNote->get_instrument() == pSelectedInstrument ) {
				// note already exists
				noteAlreadyPresent = true;
				break;
			}
		}

		if ( noteAlreadyPresent == false ) {
			notePositions << QString("%1").arg(i);
		}
	}
	SE_fillNotesRightClickAction *action = new SE_fillNotesRightClickAction(
		notePositions, nSelectedInstrument,
		m_pPatternEditorPanel->getPatternNumber() );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}


void InstrumentLine::functionRenameInstrument()
{
	// This code is pretty much a duplicate of void InstrumentEditor::labelClicked
	// in InstrumentEditor.cpp
	Hydrogen * pHydrogen = Hydrogen::get_instance();
	auto pSelectedInstrument = pHydrogen->getSong()->getDrumkit()->getInstruments()->get( m_nInstrumentNumber );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	QString sOldName = pSelectedInstrument->get_name();
	bool bIsOkPressed;
	QString sNewName = QInputDialog::getText( this, "Hydrogen", tr( "New instrument name" ), QLineEdit::Normal, sOldName, &bIsOkPressed );
	if ( bIsOkPressed  ) {
		pSelectedInstrument->set_name( sNewName );

		if ( pHydrogen->hasJackAudioDriver() ) {
			pHydrogen->getAudioEngine()->lock( RIGHT_HERE );
			pHydrogen->renameJackPorts( pHydrogen->getSong() );
			pHydrogen->getAudioEngine()->unlock();
		}

		// this will force an update...
		EventQueue::get_instance()->push_event( EVENT_SELECTED_INSTRUMENT_CHANGED, -1 );

	}
	else
	{
		// user entered nothing or pressed Cancel
	}
}

void InstrumentLine::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		m_pNameLbl->setFont( QFont( pPref->getTheme().m_font.m_sLevel2FontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) ) );
	}

	if ( changes & H2Core::Preferences::Changes::Colors ) {
		updateStyleSheet();
		update();
	}
}


//////

PatternEditorInstrumentList::PatternEditorInstrumentList( QWidget *parent )
 : QWidget( parent )
 {

	HydrogenApp::get_instance()->addEventListener( this );
	
	//INFOLOG("INIT");
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	m_nGridHeight = Preferences::get_instance()->getPatternEditorGridHeight();

	m_nEditorWidth = 181;
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS + 1;

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
	delete m_pDragScroller;
}




///
/// Create a new InstrumentLine
///
InstrumentLine* PatternEditorInstrumentList::createInstrumentLine()
{
	InstrumentLine *pLine = new InstrumentLine(this);
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 pLine, &InstrumentLine::onPreferencesChanged );
	return pLine;
}

void PatternEditorInstrumentList::repaintInstrumentLines() {
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstrList = pSong->getDrumkit()->getInstruments();

	unsigned nInstruments = pInstrList->size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {
		if ( nInstr < nInstruments &&
			 m_pInstrumentLine[ nInstr ] != nullptr ) {
			m_pInstrumentLine[ nInstr ]->update();
		}
	}
}

void PatternEditorInstrumentList::selectedInstrumentChangedEvent() {

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstrList = pSong->getDrumkit()->getInstruments();

	unsigned nSelectedInstr = pHydrogen->getSelectedInstrumentNumber();

	unsigned nInstruments = pInstrList->size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {
		if ( nInstr < nInstruments &&
			 m_pInstrumentLine[ nInstr ] != nullptr ) {
			
			InstrumentLine *pLine = m_pInstrumentLine[ nInstr ];
			pLine->setSelected( nInstr == nSelectedInstr );
		}
	}
}

///
/// Update every InstrumentLine, create or destroy lines if necessary.
///
void PatternEditorInstrumentList::updateInstrumentLines()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstrList = pSong->getDrumkit()->getInstruments();

	unsigned nSelectedInstr = pHydrogen->getSelectedInstrumentNumber();

	unsigned nInstruments = pInstrList->size();
	for ( unsigned nInstr = 0; nInstr < MAX_INSTRUMENTS; ++nInstr ) {
		if ( nInstr >= nInstruments ) {	// unused instrument! let's hide and destroy the mixerline!
			if ( m_pInstrumentLine[ nInstr ] ) {
				delete m_pInstrumentLine[ nInstr ];
				m_pInstrumentLine[ nInstr ] = nullptr;

				int newHeight = m_nGridHeight * nInstruments + 1;
				resize( width(), newHeight );
			}
			continue;
		}
		else {
			if ( m_pInstrumentLine[ nInstr ] == nullptr ) {
				// the instrument line doesn't exists..I'll create a new one!
				m_pInstrumentLine[ nInstr ] = createInstrumentLine();
				m_pInstrumentLine[nInstr]->move( 0, m_nGridHeight * nInstr + 1 );
				m_pInstrumentLine[nInstr]->show();

				int newHeight = m_nGridHeight * nInstruments;
				resize( width(), newHeight );
			}
			InstrumentLine *pLine = m_pInstrumentLine[ nInstr ];
			auto pInstr = pInstrList->get(nInstr);
			assert(pInstr);

			pLine->setNumber( nInstr );
			pLine->set( pInstr );
			pLine->setSelected( nInstr == nSelectedInstr );
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

	auto pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	int nInstruments = pInstrumentList->size();
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

		int nSourceInstrument = pHydrogen->getSelectedInstrumentNumber();

		// Starting point for instument list is 50 lower than
		// on the drum pattern editor

		int pos_y = ( event->pos().x() >= m_nEditorWidth ) ? event->pos().y() - 50 : event->pos().y();

		int nTargetInstrument = pos_y / m_nGridHeight;

		if( nTargetInstrument >= pInstrumentList->size() ){
			nTargetInstrument = pInstrumentList->size() - 1;
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
		QString sDrumkitPath = tokens.at( 0 );
		QString sInstrumentName = tokens.at( 1 );

		int nTargetInstrument = event->pos().y() / m_nGridHeight;

		/*
				"X > 181": border between the instrument names on the left and the grid
				Because the right part of the grid starts above the name column, we have to subtract the difference
		*/
		if (  event->pos().x() > 181 ) {
			nTargetInstrument = ( event->pos().y() - 90 )  / m_nGridHeight ;
		}

		if( nTargetInstrument > pInstrumentList->size() ){
			nTargetInstrument = pInstrumentList->size();
		}

		// Load Instrument
		const auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();

		const auto pNewDrumkit = pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitPath );
		if ( pNewDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve kit [%1] for instrument [%2]" )
					  .arg( sDrumkitPath ).arg( sInstrumentName ) );
			QMessageBox::critical( this, "Hydrogen",
								   pCommonString->getInstrumentLoadError() );
			return;
		}
		const auto pTargetInstrument = pNewDrumkit->getInstruments()->find( sInstrumentName );
		if ( pTargetInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve instrument [%1] from kit [%2]" )
					  .arg( sInstrumentName ).arg( sDrumkitPath ) );
			QMessageBox::critical( this, "Hydrogen",
								   pCommonString->getInstrumentLoadError() );
			return;
		}

		// We provide a copy of the instrument in order to not leak any changes
		// into the original kit.
		auto pAction = new SE_addInstrumentAction(
			std::make_shared<Instrument>(pTargetInstrument), nTargetInstrument,
			SE_addInstrumentAction::Type::DropInstrument );
		HydrogenApp::get_instance()->m_pUndoStack->push( pAction );

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

	auto pSelectedInstrument = m_pPatternEditorPanel->getSelectedInstrument();
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	QString sText = QString("move instrument:%1")
		.arg( pSelectedInstrument->get_name() );

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


void PatternEditorInstrumentList::instrumentParametersChangedEvent( int nInstrumentNumber ) {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pInstrumentList = pSong->getDrumkit()->getInstruments();

	if ( nInstrumentNumber == -1 ) {
		// Update all lines.
		for ( int ii = 0; ii < MAX_INSTRUMENTS; ++ii ) {
			auto pInstrumentLine = m_pInstrumentLine[ ii ];
			if ( pInstrumentLine != nullptr ) {
				auto pInstrument = pInstrumentList->get( ii );
				if ( pInstrument == nullptr ) {
					ERRORLOG( QString( "Instrument [%1] associated to InstrumentLine [%1] not found" )
							  .arg( ii ) );
					return;
				}

				pInstrumentLine->set( pInstrument );
			}
		}
	}
	else {
		// Update a specific line
		auto pInstrument = pInstrumentList->get( nInstrumentNumber );
		if ( pInstrument == nullptr ) {
			ERRORLOG( QString( "Instrument [%1] not found" )
					  .arg( nInstrumentNumber ) );
			return;
		}
	
		auto pInstrumentLine = m_pInstrumentLine[ nInstrumentNumber ];
		if ( pInstrumentLine == nullptr ) {
			ERRORLOG( QString( "No InstrumentLine for instrument [%1] created yet" )
					  .arg( nInstrumentNumber ) );
			return;
		}

		pInstrumentLine->set( pInstrument );
	}
}
