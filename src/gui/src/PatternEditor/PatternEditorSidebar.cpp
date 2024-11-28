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

#include "PatternEditorSidebar.h"

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

using namespace H2Core;

SidebarRow::SidebarRow( QWidget* pParent, DrumPatternRow row, int nWidth )
	: PixmapWidget(pParent)
	, m_bIsSelected( false )
	, m_bEntered( false )
	, m_nWidth( nWidth )
{
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	const auto pPref = H2Core::Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	int h = pPref->getPatternEditorGridHeight();
	resize( m_nWidth, h );

	QFont nameFont( pPref->getTheme().m_font.m_sLevel2FontFamily,
					getPointSize( pPref->getTheme().m_font.m_fontSize ) );

	m_pNameLbl = new QLabel(this);
	m_pNameLbl->resize( m_nWidth - 2 * SidebarRow::m_nButtonWidth -
						SidebarRow::m_nMargin, h );
	m_pNameLbl->move( SidebarRow::m_nMargin, 1 );
	m_pNameLbl->setFont( nameFont );

	if ( row.nInstrumentID != EMPTY_INSTR_ID ) {
		/*: Text displayed on the button for muting an instrument. Its
		  size is designed for a single character.*/
		m_pMuteBtn = new Button( this, QSize( SidebarRow::m_nButtonWidth, height() - 1 ),
								 Button::Type::Toggle, "",
								 pCommonStrings->getSmallMuteButton(),
								 true, QSize(), tr("Mute instrument"),
								 false, true );
		m_pMuteBtn->move( m_nWidth - 2 * SidebarRow::m_nButtonWidth, 0 );
		m_pMuteBtn->setChecked( false );
		m_pMuteBtn->setObjectName( "SidebarRowMuteButton" );
		connect(m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteClicked() ));

		/*: Text displayed on the button for soloing an instrument. Its
		  size is designed for a single character.*/
		m_pSoloBtn = new Button( this, QSize( SidebarRow::m_nButtonWidth, height() - 1 ),
								 Button::Type::Toggle, "",
								 pCommonStrings->getSmallSoloButton(),
								 false, QSize(), tr("Solo"),
								 false, true );
		m_pSoloBtn->move( m_nWidth - SidebarRow::m_nButtonWidth, 0 );
		m_pSoloBtn->setChecked( false );
		m_pSoloBtn->setObjectName( "SidebarRowSoloButton" );
		connect(m_pSoloBtn, SIGNAL( clicked() ), this, SLOT(soloClicked()));

		m_pSampleWarning = new Button( this, QSize( 15, 13 ), Button::Type::Icon,
									   "warning.svg", "", false, QSize(),
									   tr( "Some samples for this instrument failed to load." ),
									   true );
		m_pSampleWarning->move( 128, 5 );
		m_pSampleWarning->hide();
		connect(m_pSampleWarning, SIGNAL( clicked() ),
				this, SLOT( sampleWarningClicked() ));
	}
	else {
		m_pMuteBtn = nullptr;
		m_pSoloBtn = nullptr;
		m_pSampleWarning = nullptr;
	}

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
			 &SidebarRow::selectInstrumentNotes );

	m_pFunctionPopup->addSection( tr( "Edit all patterns" ) );
	m_pFunctionPopup->addAction( tr( "Cut notes"), this, SLOT( functionCutNotesAllPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Copy notes"), this, SLOT( functionCopyAllInstrumentPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Paste notes" ), this, SLOT( functionPasteAllInstrumentPatterns() ) );
	m_pFunctionPopup->addAction( tr( "Delete notes" ), this, SLOT( functionDeleteNotesAllPatterns() ) );

	m_pFunctionPopup->addSection( tr( "Instrument" ) );
	m_pFunctionPopup->addAction( pCommonStrings->getActionAddInstrument(),
								 HydrogenApp::get_instance()->getMainForm(),
								 SLOT( action_drumkit_addInstrument() ) );
	m_pFunctionPopup->addAction( pCommonStrings->getActionRenameInstrument(),
								 this, SLOT( functionRenameInstrument() ) );
	auto deleteAction =
		m_pFunctionPopup->addAction( pCommonStrings->getActionDeleteInstrument() );
	connect( deleteAction, &QAction::triggered, this, [=](){
		HydrogenApp::get_instance()->getMainForm()->
			functionDeleteInstrument( m_nInstrumentNumber );} );
	m_pFunctionPopup->setObjectName( "PatternEditorFunctionPopup" );

	set( row );

	updateStyleSheet();
}

void SidebarRow::set( DrumPatternRow row ) {
	auto pHydrogen = Hydrogen::get_instance();
	QString sRowName, sToolTipDrumkit;
	bool bIsSoloed = false, bIsMuted = false;

	if ( row.nInstrumentID != EMPTY_INSTR_ID ) {
		auto pSong = pHydrogen->getSong();
		if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
			const auto pInstrument =
				pSong->getDrumkit()->getInstruments()->find( row.nInstrumentID );
			if ( pInstrument != nullptr ) {
				setSelected( pHydrogen->getSelectedInstrumentNumber() ==
					pSong->getDrumkit()->getInstruments()->index( pInstrument ) );
				sRowName = pInstrument->get_name();
				setMuted( pInstrument->is_muted() );
				setSoloed( pInstrument->is_soloed() );
				setSamplesMissing( pInstrument->has_missing_samples() );

				if ( ! pInstrument->get_drumkit_path().isEmpty() ) {
					// Instrument belongs to a kit in the SoundLibrary (and was
					// not created anew).
					QString sKit = pHydrogen->getSoundLibraryDatabase()->
						getUniqueLabel( pInstrument->get_drumkit_path() );
					if ( sKit.isEmpty() ) {
						// This should not happen. But drumkit.xml files can be
						// created by hand and we should account for it.
						sKit = pInstrument->get_drumkit_path();
					}

					/*: Shown in a tooltop and indicating the drumkit (to the right of this string) an instrument (to the left of this string) is loaded from. */
					sToolTipDrumkit = QString( " (" ).append( tr( "imported from" ) )
						.append( QString( " [%1])" ).arg( sKit ) );
				}
			}
		}
	}
	else {
		m_bIsSelected = false;
	}

	if ( ! row.sType.isEmpty() ) {
		sRowName.append( " | " ).append( row.sType );
	}
	setName( sRowName );

	// Create a tool tip uniquely stating the drumkit the instrument belongs to.
	QString sToolTip = QString( "%1%2" ).arg( sRowName ).arg( sToolTipDrumkit );

	setToolTip( sToolTip );
}

void SidebarRow::setName(const QString& sName)
{
	if ( m_pNameLbl->text() != sName ){
		m_pNameLbl->setText(sName);
	}
}

void SidebarRow::setToolTip(const QString& sToolTip)
{
	if ( m_pNameLbl->toolTip() != sToolTip ){
		m_pNameLbl->setToolTip( sToolTip );
	}
}



void SidebarRow::setSelected( bool bSelected )
{
	if ( bSelected == m_bIsSelected ) {
		return;
	}
	
	m_bIsSelected = bSelected;

	updateStyleSheet();
	update();
}

void SidebarRow::updateStyleSheet() {

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

void SidebarRow::enterEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void SidebarRow::leaveEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void SidebarRow::paintEvent( QPaintEvent* ev ) {
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
		painter.drawRoundedRect( QRect( 1, 1, width() - 2 * SidebarRow::m_nButtonWidth - 1,
										height() - 2 ), 4, 4 );
	}
}


void SidebarRow::setNumber(int nIndex)
{
	if ( m_nInstrumentNumber != nIndex ) {
		m_nInstrumentNumber = nIndex;
		update();
	}
}



void SidebarRow::setMuted(bool isMuted)
{
	if ( m_pMuteBtn != nullptr && ! m_pMuteBtn->isDown() &&
		 m_pMuteBtn->isChecked() != isMuted ) {
		m_pMuteBtn->setChecked(isMuted);
	}
}


void SidebarRow::setSoloed( bool soloed )
{
	if ( m_pSoloBtn != nullptr && ! m_pSoloBtn->isDown() &&
		 m_pSoloBtn->isChecked() != soloed ) {
		m_pSoloBtn->setChecked( soloed );
	}
}


void SidebarRow::setSamplesMissing( bool bSamplesMissing )
{
	if ( m_pSampleWarning != nullptr ) {
		if ( bSamplesMissing ) {
			m_pSampleWarning->show();
		} else {
			m_pSampleWarning->hide();
		}
	}
}



void SidebarRow::muteClicked()
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



void SidebarRow::soloClicked()
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

void SidebarRow::sampleWarningClicked()
{
	QMessageBox::information( this, "Hydrogen",
							  tr( "One or more samples for this instrument failed to load. This may be because the"
								  " songfile uses an older default drumkit. This might be fixed by opening a new "
								  "drumkit." ) );
}

void SidebarRow::selectInstrumentNotes()
{
	m_pPatternEditorPanel->getVisibleEditor()->selectAllNotesInRow(
		m_nInstrumentNumber );
}

void SidebarRow::mousePressEvent(QMouseEvent *ev)
{
	m_pPatternEditorPanel->setSelectedRowDB( m_nInstrumentNumber );

	if ( ev->button() == Qt::LeftButton ) {

		std::shared_ptr<Song> pSong = Hydrogen::get_instance()->getSong();
		if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
			ERRORLOG( "No song set yet" );
			return;
		}
		auto pInstr = pSong->getDrumkit()->getInstruments()->get( m_nInstrumentNumber );
		if ( m_pMuteBtn != nullptr && pInstr != nullptr && pInstr->hasSamples() ) {

			const int nWidth = m_pMuteBtn->x() - 5; // clickable field width
			const float fVelocity = std::min(
				(float)ev->x()/(float)nWidth, VELOCITY_MAX );
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

void SidebarRow::mouseDoubleClickEvent( QMouseEvent* ev ) {
	functionRenameInstrument();
}

void SidebarRow::functionClearNotes()
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


void SidebarRow::functionCopyAllInstrumentPatterns()
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


void SidebarRow::functionPasteAllInstrumentPatterns()
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
	// replaced with the one belonging to this SidebarRow. Or notes will end
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

void SidebarRow::functionDeleteNotesAllPatterns()
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

void SidebarRow::functionCutNotesAllPatterns()
{
	functionCopyAllInstrumentPatterns();
	functionDeleteNotesAllPatterns();
}


void SidebarRow::functionFillAllNotes(){ functionFillNotes(1); }
void SidebarRow::functionFillEveryTwoNotes(){ functionFillNotes(2); }
void SidebarRow::functionFillEveryThreeNotes(){ functionFillNotes(3); }
void SidebarRow::functionFillEveryFourNotes(){ functionFillNotes(4); }
void SidebarRow::functionFillEverySixNotes(){ functionFillNotes(6); }
void SidebarRow::functionFillEveryEightNotes(){ functionFillNotes(8); }
void SidebarRow::functionFillEveryTwelveNotes(){ functionFillNotes(12); }
void SidebarRow::functionFillEverySixteenNotes(){ functionFillNotes(16); }

void SidebarRow::functionFillNotes( int every )
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


void SidebarRow::functionRenameInstrument()
{
	// This code is pretty much a duplicate of void InstrumentEditor::labelClicked
	// in InstrumentEditor.cpp
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}
	auto pSelectedInstrument =
		pSong->getDrumkit()->getInstruments()->get( m_nInstrumentNumber );
	if ( pSelectedInstrument == nullptr ) {
		ERRORLOG( "No instrument selected" );
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const QString sOldName = pSelectedInstrument->get_name();
	bool bIsOkPressed;
	const QString sNewName = QInputDialog::getText(
		this, "Hydrogen", pCommonStrings->getActionRenameInstrument(),
		QLineEdit::Normal, sOldName, &bIsOkPressed );
	if ( bIsOkPressed ) {
		auto pNewInstrument = std::make_shared<Instrument>(pSelectedInstrument);
		pNewInstrument->set_name( sNewName );

		auto pAction = new SE_replaceInstrumentAction(
			pNewInstrument, pSelectedInstrument,
			SE_replaceInstrumentAction::Type::RenameInstrument,
			sNewName, sOldName );
		HydrogenApp::get_instance()->m_pUndoStack->push( pAction );
	}
}

void SidebarRow::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
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

PatternEditorSidebar::PatternEditorSidebar( QWidget *parent )
	: QWidget( parent )
	, m_nDragStartY( -1 )
 {

	HydrogenApp::get_instance()->addEventListener( this );
	
	//INFOLOG("INIT");
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	m_nGridHeight = Preferences::get_instance()->getPatternEditorGridHeight();

	m_nEditorWidth = 181;
	m_nEditorHeight = m_nGridHeight * m_pPatternEditorPanel->getRowNumberDB();

	resize( m_nEditorWidth, m_nEditorHeight );

	setAcceptDrops(true);

	updateRows();

	QScrollArea *pScrollArea = dynamic_cast< QScrollArea *>( parentWidget()->parentWidget() );
	assert( pScrollArea );
	m_pDragScroller = new DragScroller( pScrollArea );
}



PatternEditorSidebar::~PatternEditorSidebar()
{
	//INFOLOG( "DESTROY" );
	delete m_pDragScroller;
}

void PatternEditorSidebar::updateEditor() {
	updateRows();

	for ( auto& rrow : m_rows ) {
		rrow->update();
	}

	update();
}

///
/// Update every SidebarRow, create or destroy lines if necessary.
///
void PatternEditorSidebar::updateRows()
{
	if ( m_nEditorHeight !=
		 m_nGridHeight * m_pPatternEditorPanel->getRowNumberDB() ) {
		m_nEditorHeight = m_nGridHeight * m_pPatternEditorPanel->getRowNumberDB();
		resize( m_nEditorWidth, m_nEditorHeight );
	}

	int nnIndex = 0;
	for ( const auto& rrow : m_pPatternEditorPanel->getDB() ) {
		if ( nnIndex < m_rows.size() ) {
			// row already exists do a lazy update instead of recreating it.
			m_rows[ nnIndex ]->setNumber( nnIndex );
			m_rows[ nnIndex ]->set( rrow );
		}
		else {
			// row in DB does not has its counterpart in the sidebar yet. Create
			// it.
			auto pRow = std::make_shared<SidebarRow>( this, rrow, m_nEditorWidth );
			pRow->setNumber( nnIndex );
			pRow->move( 0, m_nGridHeight * nnIndex + 1 );
			pRow->show();
			m_rows.push_back( pRow );
		}
		++nnIndex;
	}

	const int nRows = m_pPatternEditorPanel->getRowNumberDB();
	while ( nRows < m_rows.size() && m_rows.size() > 0 ) {
		// There are rows not required anymore
		m_rows.pop_back();
	}
}
	
void PatternEditorSidebar::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void PatternEditorSidebar::dropEvent(QDropEvent *event)
{
	if ( ! event->mimeData()->hasFormat("text/plain") ) {
		event->ignore();
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
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

	// Starting point for instument list is 50 lower than on the drum pattern
	// editor
	int nPosY;
	if ( event->pos().x() >= m_nEditorWidth ) {
		nPosY = event->pos().y() - 50;
	} else {
		nPosY = event->pos().y();
	}

	int nTargetRow = nPosY / m_nGridHeight;

	// There might be rows in the pattern editor not corresponding to the
	// current kit. Since we only support rearranging rows corresponding to
	// valid instruments we will move the dragged one to the end of the
	// instrument list in case it was dragged beyond it.
	if ( nTargetRow >= pInstrumentList->size() ) {
		nTargetRow = pInstrumentList->size() - 1;
	}

	if ( sText.startsWith( "move instrument:" ) ) {

		sText.remove( 0, QString( "move instrument:" ).length() );

		bool bOk = false;
		const int nSourceRow = sText.toInt( &bOk, 10 );

		if ( nSourceRow == nTargetRow ) {
			event->acceptProposedAction();
			return;
		}
		
		HydrogenApp::get_instance()->m_pUndoStack->push(
			new SE_moveInstrumentAction( nSourceRow, nTargetRow ) );

		event->acceptProposedAction();
	}
	else if ( sText.startsWith( "importInstrument:" ) ) {
		// an instrument was dragged from the soundlibrary browser to the
		// pattern editor
		sText.remove( 0, QString( "importInstrument:" ).length() );

		QStringList tokens = sText.split( "::" );
		const QString sDrumkitPath = tokens.at( 0 );
		const QString sInstrumentName = tokens.at( 1 );

		// Load Instrument
		const auto pCommonString = HydrogenApp::get_instance()->getCommonStrings();

		const auto pNewDrumkit =
			pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitPath );
		if ( pNewDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve kit [%1] for instrument [%2]" )
					  .arg( sDrumkitPath ).arg( sInstrumentName ) );
			QMessageBox::critical( this, "Hydrogen",
								   pCommonString->getInstrumentLoadError() );
			return;
		}
		const auto pTargetInstrument =
			pNewDrumkit->getInstruments()->find( sInstrumentName );
		if ( pTargetInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve instrument [%1] from kit [%2]" )
					  .arg( sInstrumentName ).arg( sDrumkitPath ) );
			QMessageBox::critical( this, "Hydrogen",
								   pCommonString->getInstrumentLoadError() );
			return;
		}

		// Appending in this action is done by setting the target row to -1.
		int nTargetRowSE = nTargetRow;
		if ( nTargetRow == pInstrumentList->size() - 1 ) {
			nTargetRowSE = -1;
			// Select the row after the current "end" of the drumkit.
			++nTargetRow;
		}
		// We provide a copy of the instrument in order to not leak any changes
		// into the original kit.
		auto pAction = new SE_addInstrumentAction(
			std::make_shared<Instrument>(pTargetInstrument), nTargetRowSE,
			SE_addInstrumentAction::Type::DropInstrument );
		HydrogenApp::get_instance()->m_pUndoStack->push( pAction );

		event->acceptProposedAction();
	}
	else {
		// Unknown drop action
		return;
	}

	m_pPatternEditorPanel->setSelectedRowDB( nTargetRow );
}


void PatternEditorSidebar::mousePressEvent( QMouseEvent *event ) {
	if ( event->button() != Qt::LeftButton ) {
		return;
	}

	if ( m_pPatternEditorPanel->getRowDB(
			 m_pPatternEditorPanel->getSelectedRowDB() ).nInstrumentID !=
		 EMPTY_INSTR_ID ) {
		// Drag started at a line corresponding to an instrument of the current
		// drumkit.
		m_nDragStartY = event->pos().y();
	}
	else {
		m_nDragStartY = -1;
	}
}

void PatternEditorSidebar::mouseMoveEvent(QMouseEvent *event)
{
	// Button needs to stay pressed.
	if ( ! ( event->buttons() & Qt::LeftButton ) ) {
		return;
	}

	// No valid drag. Maybe it was started using a instrument type only row.
	if ( m_nDragStartY == -1 ) {
		return;
	}

	if ( abs( event->pos().y() - m_nDragStartY ) < m_nGridHeight ) {
		// Still within the same row.
		return;
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	// Instrument corresponding to the selected line in the pattern editor.
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();
	auto pInstrument = pSong->getDrumkit()->getInstruments()->find(
		m_pPatternEditorPanel->getRowDB( nSelectedRow ).nInstrumentID );
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "No instrument selected found for row [%1]" )
				  .arg( nSelectedRow ) );
		return;
	}

	const QString sText = QString( "move instrument:%1" ).arg( nSelectedRow );

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData );

	m_pDragScroller->startDrag();
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
	m_pDragScroller->endDrag();

	QWidget::mouseMoveEvent(event);
}


void PatternEditorSidebar::instrumentMuteSoloChangedEvent( int nInstrumentIndex ) {

	if ( nInstrumentIndex == -1 ) {
		updateRows();
	}
	else {
		// Update a specific line
		const auto row = m_pPatternEditorPanel->getRowDB( nInstrumentIndex );
		if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
			ERRORLOG( QString( "Invalid row [%1]" ).arg( nInstrumentIndex ) );
			return;
		}

		if ( nInstrumentIndex >= m_rows.size() ) {
			// This should not happen
			updateRows();
		}
		else {
			m_rows[ nInstrumentIndex ]->set( row );
		}
	}
}
