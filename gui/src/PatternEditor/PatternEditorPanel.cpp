/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/instrument.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
using namespace H2Core;


#include "PatternEditorPanel.h"
#include "PatternEditorInstrumentList.h"
#include "PatternEditorRuler.h"
#include "NotePropertiesRuler.h"

#include "../MainForm.h"
#include "../widgets/Button.h"
#include "../widgets/Fader.h"
#include "../widgets/PixmapWidget.h"

#include "../Skin.h"
#include "../SongEditor/SongEditorPanel.h"

#include <cmath>

#include <QtGui>

PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
 : QWidget( pParent )
 , Object( "PatternEditorPanel" )
 , m_pPattern( NULL )
 , m_bEnablePatternResize( true )
{
	setAcceptDrops(true);

	Preferences *pPref = Preferences::getInstance();


// Editor TOP
	PixmapWidget *editor_top = new PixmapWidget(0);
	editor_top->setPixmap("/patternEditor/editor_top.png", true);
	editor_top->setFixedHeight(62);

	QHBoxLayout *editor_top_hbox = new QHBoxLayout(editor_top);
	editor_top_hbox->setSpacing(0);
	editor_top_hbox->setMargin(0);


	// PATTERN size
	__pattern_size_combo = new LCDCombo(NULL, 4);
	__pattern_size_combo->setToolTip( trUtf8("Select pattern size") );
	for ( int i = 1; i <= 32; i++) {
		__pattern_size_combo->addItem( QString( "%1" ).arg( i ) );
	}
	__pattern_size_combo->update();
	connect(__pattern_size_combo, SIGNAL( valueChanged( QString ) ), this, SLOT( patternSizeChanged(QString) ) );
	editor_top_hbox->addWidget(__pattern_size_combo);


	// GRID resolution
	__resolution_combo = new LCDCombo(NULL, 7);
	__resolution_combo->setToolTip(trUtf8("Select grid resolution"));
	__resolution_combo->addItem( "4" );
	__resolution_combo->addItem( "8" );
	__resolution_combo->addItem( "16" );
	__resolution_combo->addItem( "32" );
	__resolution_combo->addItem( "64" );
	__resolution_combo->addSeparator();
	__resolution_combo->addItem( "4T" );
	__resolution_combo->addItem( "8T" );
	__resolution_combo->addItem( "16T" );
	__resolution_combo->addItem( "32T" );
	__resolution_combo->addSeparator();
	__resolution_combo->addItem( "off" );
	__resolution_combo->update();
	connect(__resolution_combo, SIGNAL(valueChanged(QString)), this, SLOT(gridResolutionChanged(QString)));
	editor_top_hbox->addWidget(__resolution_combo);


	// Hear notes btn
	ToggleButton *hearNotesBtn = new ToggleButton(
			NULL,
			"/patternEditor/btn_hear_on.png",
			"/patternEditor/btn_hear_off.png",
			"/patternEditor/btn_hear_off.png",
			QSize(15, 13)
	);
	hearNotesBtn->setToolTip( trUtf8( "Hear new notes" ) );
	connect( hearNotesBtn, SIGNAL(clicked(Button*)), this, SLOT( hearNotesBtnClick(Button*)));
	editor_top_hbox->addWidget(hearNotesBtn);

	// restore hear new notes button state
	hearNotesBtn->setPressed( pPref->getHearNewNotes() );


	// Record events btn
	ToggleButton* recordEventsBtn = new ToggleButton(
			NULL,
			"/patternEditor/btn_record_on.png",
			"/patternEditor/btn_record_off.png",
			"/patternEditor/btn_record_off.png",
			QSize(15, 13)
	);
	recordEventsBtn->setPressed( pPref->getRecordEvents());
	recordEventsBtn->setToolTip( trUtf8( "Record keyboard/midi events" ) );
	connect( recordEventsBtn, SIGNAL(clicked(Button*)), this, SLOT( recordEventsBtnClick(Button*)));
	editor_top_hbox->addWidget(recordEventsBtn);


	// quantize
	ToggleButton* quantizeEventsBtn = new ToggleButton(
			NULL,
			"/patternEditor/btn_quant_on.png",
			"/patternEditor/btn_quant_off.png",
			"/patternEditor/btn_quant_off.png",
			QSize(15, 13)
	);
	quantizeEventsBtn->setPressed( pPref->getQuantizeEvents());
	quantizeEventsBtn->setToolTip( trUtf8( "Quantize keyboard/midi events to grid" ) );
	connect( quantizeEventsBtn, SIGNAL(clicked(Button*)), this, SLOT( quantizeEventsBtnClick(Button*)));
	editor_top_hbox->addWidget(quantizeEventsBtn);


	// zoom-in btn
	Button *zoom_in_btn = new Button(
			NULL,
			"",
			"",
			"",
			QSize(20, 20)
	);
	zoom_in_btn->setText("+");
	zoom_in_btn->setToolTip( trUtf8( "Zoom in" ) );
	connect(zoom_in_btn, SIGNAL(clicked(Button*)), this, SLOT( zoomInBtnClicked(Button*) ) );
	editor_top_hbox->addWidget(zoom_in_btn);


	// zoom-out btn
	Button *zoom_out_btn = new Button(
			NULL,
			"",
			"",
			"",
			QSize(20, 20)
	);
	zoom_out_btn->setText("-");
	zoom_out_btn->setToolTip( trUtf8( "Zoom out" ) );
	connect( zoom_out_btn, SIGNAL(clicked(Button*)), this, SLOT( zoomOutBtnClicked(Button*) ) );
	editor_top_hbox->addWidget(zoom_out_btn);


	// show drum editor btn
	__show_drum_btn = new ToggleButton(
			NULL,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize(40, 17),
			true
	);
	__show_drum_btn->setText( trUtf8("Drum") );
	__show_drum_btn->setPressed( true );
	__show_drum_btn->setToolTip( trUtf8( "Show drum editor" ) );
	connect(__show_drum_btn, SIGNAL(clicked(Button*)), this, SLOT( showDrumEditorBtnClick(Button*)));
	editor_top_hbox->addWidget(__show_drum_btn);


	// show piano roll btn
	__show_piano_btn = new ToggleButton(
			NULL,
			"/skin_btn_on.png",
			"/skin_btn_off.png",
			"/skin_btn_over.png",
			QSize(40, 17),
			true
	);
	__show_piano_btn->setText( trUtf8("Piano") );
	__show_piano_btn->setPressed( false );
	__show_piano_btn->setToolTip( trUtf8( "Show piano roll editor" ) );
	connect(__show_piano_btn, SIGNAL(clicked(Button*)), this, SLOT( showPianoEditorBtnClick(Button*)));
	editor_top_hbox->addWidget(__show_piano_btn);
// End Editor TOP


// RULER____________________________________

	// Ruler ScrollView
	m_pRulerScrollView = new QScrollArea( NULL );
	m_pRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pRulerScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pRulerScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pRulerScrollView->setFixedHeight( 25 );

	// Ruler
	m_pPatternEditorRuler = new PatternEditorRuler( m_pRulerScrollView->viewport() );

	m_pRulerScrollView->setWidget( m_pPatternEditorRuler );

//~ RULER


// EDITOR _____________________________________
	// Editor scrollview
	m_pEditorScrollView = new QScrollArea( NULL );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pEditorScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	// Editor
	m_pDrumPatternEditor = new DrumPatternEditor( m_pEditorScrollView->viewport(), this );

	m_pEditorScrollView->setWidget( m_pDrumPatternEditor );

	connect( m_pEditorScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorScroll(int) ) );




	m_pPianoRollScrollView = new QScrollArea( NULL );
	m_pPianoRollScrollView->setFrameShape( QFrame::NoFrame );
	m_pPianoRollScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPianoRollScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	m_pPianoRollEditor = new PianoRollEditor( m_pPianoRollScrollView->viewport() );
	m_pPianoRollScrollView->setWidget( m_pPianoRollEditor );

	connect( m_pPianoRollScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorScroll(int) ) );


	m_pPianoRollScrollView->hide();
//~ EDITOR






// INSTRUMENT LIST
	// Instrument list scrollview
	m_pInstrListScrollView = new QScrollArea( NULL );
	m_pInstrListScrollView->setFrameShape( QFrame::NoFrame );
	m_pInstrListScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pInstrListScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	// Instrument list
	m_pInstrumentList = new PatternEditorInstrumentList( m_pInstrListScrollView->viewport(), this );
	m_pInstrListScrollView->setWidget( m_pInstrumentList );
	m_pInstrListScrollView->setFixedWidth( m_pInstrumentList->width() );

	connect( m_pInstrListScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorScroll(int) ) );
//~ INSTRUMENT LIST




// NOTE_VELOCITY EDITOR
	m_pNoteVelocityScrollView = new QScrollArea( NULL );
	m_pNoteVelocityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteVelocityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityEditor = new NotePropertiesRuler( m_pNoteVelocityScrollView->viewport(), this, NotePropertiesRuler::VELOCITY );
	m_pNoteVelocityScrollView->setWidget( m_pNoteVelocityEditor );
	m_pNoteVelocityScrollView->setFixedHeight( 100 );
//~ NOTE_VELOCITY EDITOR


// NOTE_VELOCITY EDITOR
	m_pNotePanScrollView = new QScrollArea( NULL );
	m_pNotePanScrollView->setFrameShape( QFrame::NoFrame );
	m_pNotePanScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanEditor = new NotePropertiesRuler( m_pNotePanScrollView->viewport(), this, NotePropertiesRuler::PAN );
	m_pNotePanScrollView->setWidget( m_pNotePanEditor );
	m_pNotePanScrollView->setFixedHeight( 100 );
//~ NOTE_VELOCITY EDITOR



	// external horizontal scrollbar
	m_pPatternEditorHScrollBar = new QScrollBar( Qt::Horizontal , NULL  );
	connect( m_pPatternEditorHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	// external vertical scrollbar
	m_pPatternEditorVScrollBar = new QScrollBar( Qt::Vertical, NULL );
	connect( m_pPatternEditorVScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );



	QPalette label_palette;
	label_palette.setColor( QPalette::Foreground, QColor( 230, 230, 230 ) );

	QFont boldFont;
	boldFont.setBold( true );
	m_pPatternNameLbl = new QLabel( NULL );
	m_pPatternNameLbl->setFont( boldFont );
	m_pPatternNameLbl->setText( "pattern name label" );
	//m_pPatternNameLbl->setFixedWidth(200);
	m_pPatternNameLbl->setPalette(label_palette);






// NOTE_PROPERTIES BUTTONS
	PixmapWidget *pPropertiesPanel = new PixmapWidget( NULL );
	pPropertiesPanel->setColor( QColor( 255, 0, 0 ) );
	pPropertiesPanel->setFixedSize( 181, 100 );

	QVBoxLayout *pPropertiesVBox = new QVBoxLayout( pPropertiesPanel );
	pPropertiesVBox->setSpacing( 0 );
	pPropertiesVBox->setMargin( 0 );


	LCDCombo* pPropertiesCombo = new LCDCombo( NULL, 20);
	pPropertiesCombo->setToolTip(trUtf8("Select note properties"));
	pPropertiesCombo->addItem( trUtf8("Velocity") );
	pPropertiesCombo->addItem( trUtf8("Pan") );
	pPropertiesCombo->update();
	connect( pPropertiesCombo, SIGNAL(valueChanged(QString)), this, SLOT(propertiesComboChanged(QString)));

	pPropertiesVBox->addWidget( pPropertiesCombo );


//~ NOTE_PROPERTIES BUTTONS


// LAYOUT
	QWidget *pMainPanel = new QWidget();

	QGridLayout *pGrid = new QGridLayout();
	pGrid->setSpacing( 0 );
	pGrid->setMargin( 0 );

	pGrid->addWidget( editor_top, 0, 0, 1, 3 );

	pGrid->addWidget( m_pPatternNameLbl, 1, 0 );
	pGrid->addWidget( m_pRulerScrollView, 1, 1 );

	pGrid->addWidget( m_pInstrListScrollView, 2, 0 );

	pGrid->addWidget( m_pEditorScrollView, 2, 1 );
	pGrid->addWidget( m_pPianoRollScrollView, 2, 1 );

	pGrid->addWidget( m_pPatternEditorVScrollBar, 2, 2 );
	pGrid->addWidget( m_pPatternEditorHScrollBar, 10, 1 );
	pGrid->addWidget( m_pNoteVelocityScrollView, 4, 1 );
	pGrid->addWidget( m_pNotePanScrollView, 4, 1 );

	pGrid->addWidget( pPropertiesPanel, 4, 0 );
	pGrid->setRowStretch( 2, 100 );
	pMainPanel->setLayout( pGrid );





	// restore grid resolution
	int nIndex;
	if ( pPref->isPatternEditorUsingTriplets() == false ) {
		switch ( pPref->getPatternEditorGridResolution() ) {
			case 4:
				__resolution_combo->set_text( "4" );
				nIndex = 0;
				break;

			case 8:
				__resolution_combo->set_text( "8" );
				nIndex = 1;
				break;

			case 16:
				__resolution_combo->set_text( "16" );
				nIndex = 2;
				break;

			case 32:
				__resolution_combo->set_text( "32" );
				nIndex = 3;
				break;

			case 64:
				__resolution_combo->set_text( "64" );
				nIndex = 4;
				break;

			default:
				ERRORLOG( "Wrong grid resolution: " + to_string( pPref->getPatternEditorGridResolution() ) );
				__resolution_combo->set_text( "4" );
				nIndex = 0;
		}
	}
	else {
		switch ( pPref->getPatternEditorGridResolution() ) {
			case 8:
				__resolution_combo->set_text( "4T" );
				nIndex = 5;
				break;

			case 16:
				__resolution_combo->set_text( "8T" );
				nIndex = 6;
				break;

			case 32:
				__resolution_combo->set_text( "16T" );
				nIndex = 7;
				break;

			case 64:
				__resolution_combo->set_text( "32T" );
				nIndex = 8;
				break;

			default:
				ERRORLOG( "Wrong grid resolution: " + to_string( pPref->getPatternEditorGridResolution() ) );
				__resolution_combo->set_text( "4T" );
				nIndex = 5;
		}
	}
	gridResolutionChanged(__resolution_combo->getText());






	// LAYOUT
	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );
	this->setLayout( pVBox );

	pVBox->addWidget( pMainPanel );

	HydrogenApp::getInstance()->addEventListener( this );

	selectedPatternChangedEvent(); // force an update

	pPropertiesCombo->set_text( trUtf8("Velocity"));
}




PatternEditorPanel::~PatternEditorPanel()
{
}



void PatternEditorPanel::syncToExternalHorizontalScrollbar(int)
{
//	INFOLOG( "[syncToExternalHorizontalScrollbar]" );

	// drum Editor
	m_pEditorScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
	m_pEditorScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );

	// piano roll Editor
	m_pPianoRollScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
	m_pPianoRollScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );


	// Ruler
	m_pRulerScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// Instrument list
	m_pInstrListScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );

	// Velocity ruler
	m_pNoteVelocityScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// pan ruler
	m_pNotePanScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
}


void PatternEditorPanel::on_patternEditorScroll(int nValue)
{
//	INFOLOG( "[on_patternEditorScroll]" );
	m_pPatternEditorVScrollBar->setValue( nValue );
}




void PatternEditorPanel::gridResolutionChanged( QString str )
{
	int nResolution;
	bool bUseTriplets = false;

	if ( str.contains( "off" ) ) {
		nResolution=MAX_NOTES;
	}
	else if ( str.contains( "T" ) ) {
		bUseTriplets = true;
		QString temp = str;
		temp.chop( 1 );
		nResolution = temp.toInt() * 2;
	}
	else {
		nResolution = str.toInt();
	}

	//INFOLOG( to_string( nResolution ) );
	m_pDrumPatternEditor->setResolution( nResolution, bUseTriplets );

	Preferences::getInstance()->setPatternEditorGridResolution( nResolution );
	Preferences::getInstance()->setPatternEditorUsingTriplets( bUseTriplets );
}



void PatternEditorPanel::selectedPatternChangedEvent()
{
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	int nSelectedPatternNumber = Hydrogen::get_instance()->getSelectedPatternNumber();

	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->get_size() ) ) {
		// update pattern name text
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
		QString sCurrentPatternName = m_pPattern->get_name().c_str();
		this->setWindowTitle( ( trUtf8( "Pattern editor - %1").arg( sCurrentPatternName ) ) );
		//m_pNameLCD->setText( sCurrentPatternName );
		m_pPatternNameLbl->setText( sCurrentPatternName );

		// update pattern size combobox
		int nPatternSize = m_pPattern->get_lenght();
		int nEighth = MAX_NOTES / 8;
		for ( int i = 1; i <= 32; i++ ) {
			if ( nPatternSize == nEighth * i ) {
				__pattern_size_combo->set_text( QString( "%1" ).arg( i ) );
				break;
			}
		}
	}
	else {
		m_pPattern = NULL;

		this->setWindowTitle( ( trUtf8( "Pattern editor - %1").arg(QString("No pattern selected.")) ) );
		//m_pNameLCD->setText( trUtf8( "No pattern selected" ) );
		m_pPatternNameLbl->setText( trUtf8( "No pattern selected" ) );
	}

	resizeEvent( NULL ); // force an update of the scrollbars
}



void PatternEditorPanel::hearNotesBtnClick(Button *ref)
{
	Preferences *pref = ( Preferences::getInstance() );
	pref->setHearNewNotes( ref->isPressed() );

	if (ref->isPressed() ) {
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Hear new notes = On" ), 2000 );
	}
	else {
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Hear new notes = Off" ), 2000 );
	}

}



void PatternEditorPanel::recordEventsBtnClick(Button *ref)
{
	Preferences *pref = ( Preferences::getInstance() );
	pref->setRecordEvents( ref->isPressed() );

	if (ref->isPressed() ) {
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Record keyboard/midi events = On" ), 2000 );
	}
	else {
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Record keyboard/midi events = Off" ), 2000 );
	}

}



void PatternEditorPanel::quantizeEventsBtnClick(Button *ref)
{
	Preferences *pref = ( Preferences::getInstance() );
	pref->setQuantizeEvents( ref->isPressed() );

	if (ref->isPressed() ) {
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Quantize incoming keyboard/midi events = On" ), 2000 );
	}
	else {
		( HydrogenApp::getInstance() )->setStatusBarMessage( trUtf8( "Quantize incoming keyboard/midi events = Off" ), 2000 );
	}
}




void PatternEditorPanel::stateChangedEvent(int state)
{
	if ( state == STATE_READY) {
		m_bEnablePatternResize = true;
	}
	else {
		m_bEnablePatternResize = false;
	}
}



void PatternEditorPanel::resizeEvent( QResizeEvent *ev )
{
	UNUSED( ev );
	QScrollArea *pScrollArea = NULL;

	if ( m_pPianoRollScrollView->isVisible() ) {
		pScrollArea = m_pPianoRollScrollView;
	}
	else {
		pScrollArea = m_pEditorScrollView;
	}


	m_pPatternEditorHScrollBar->setMinimum( pScrollArea->horizontalScrollBar()->minimum() );
	m_pPatternEditorHScrollBar->setMaximum( pScrollArea->horizontalScrollBar()->maximum() );
	m_pPatternEditorHScrollBar->setSingleStep( pScrollArea->horizontalScrollBar()->singleStep() );
	m_pPatternEditorHScrollBar->setPageStep( pScrollArea->horizontalScrollBar()->pageStep() );

	m_pPatternEditorVScrollBar->setMinimum( pScrollArea->verticalScrollBar()->minimum() );
	m_pPatternEditorVScrollBar->setMaximum( pScrollArea->verticalScrollBar()->maximum() );
	m_pPatternEditorVScrollBar->setSingleStep( pScrollArea->verticalScrollBar()->singleStep() );
	m_pPatternEditorVScrollBar->setPageStep( pScrollArea->verticalScrollBar()->pageStep() );
}




void PatternEditorPanel::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
//	m_pPatternEditorVScrollBar->setValue( m_pPatternEditorVScrollBar->maximum() );
}


/// richiamato dall'uso dello scroll del mouse
void PatternEditorPanel::contentsMoving(int dummy)
{
	UNUSED( dummy );
	//INFOLOG( "contentsMoving" );
	syncToExternalHorizontalScrollbar(0);
}



void PatternEditorPanel::selectedInstrumentChangedEvent()
{
  //m_pNoteVelocityEditor->updateEditor();
  //m_pNotePanEditor->updateEditor();

	resizeEvent(NULL);	// force a scrollbar update
}



void PatternEditorPanel::showDrumEditorBtnClick(Button *ref)
{
	UNUSED( ref );
	__show_drum_btn->setPressed( true );
	__show_piano_btn->setPressed( false );


	m_pPianoRollScrollView->hide();
	m_pEditorScrollView->show();
	m_pInstrListScrollView->show();

	m_pDrumPatternEditor->selectedInstrumentChangedEvent(); // force an update

	// force a re-sync of extern scrollbars
	resizeEvent( NULL );
}



void PatternEditorPanel::showPianoEditorBtnClick(Button *ref)
{
	UNUSED( ref );
	__show_piano_btn->setPressed( true );
	__show_drum_btn->setPressed( false );


	m_pPianoRollScrollView->show();
	m_pEditorScrollView->hide();
	m_pInstrListScrollView->hide();

	m_pPianoRollEditor->selectedInstrumentChangedEvent(); // force an update

	// force a re-sync of extern scrollbars
	resizeEvent( NULL );
}




void PatternEditorPanel::zoomInBtnClicked(Button *ref)
{
	UNUSED( ref );
	m_pPatternEditorRuler->zoomIn();
	m_pDrumPatternEditor->zoomIn();
	m_pNoteVelocityEditor->zoomIn();

	resizeEvent( NULL );
}



void PatternEditorPanel::zoomOutBtnClicked(Button *ref)
{
	UNUSED( ref );
	m_pPatternEditorRuler->zoomOut();
	m_pDrumPatternEditor->zoomOut();
	m_pNoteVelocityEditor->zoomOut();

	resizeEvent( NULL );
}



void PatternEditorPanel::patternSizeChanged( QString str )
{
	INFOLOG( "pattern size changed" );

	uint nEighth = MAX_NOTES / 8;
	int nSelected = str.toInt();

	if ( m_pPattern->get_lenght() == nEighth * nSelected ) {
		// non e' necessario aggiornare
		return;
	}


	if ( !m_pPattern ) {
		return;
	}

	if ( !m_bEnablePatternResize ) {
		QMessageBox::information( this, "Hydrogen", trUtf8( "Is not possible to change the pattern size when playing." ) );
		return;
	}


	if ( nSelected > 0 && nSelected <= 32 ) {
		m_pPattern->set_lenght( nEighth * nSelected );
		//m_pPatternSizeLCD->setText( QString( "%1" ).arg( nSelected ) );
	}
	else {
		ERRORLOG( "[patternSizeChanged] Unhandled case " + to_string( nSelected ) );
	}

	m_pPatternEditorRuler->updateEditor( true );	// redraw all
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePanEditor->updateEditor();

	resizeEvent( NULL );

	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}



void PatternEditorPanel::moveUpBtnClicked(Button *)
{
	Hydrogen *engine = Hydrogen::get_instance();
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	AudioEngine::get_instance()->lock( "PatternEditorPanel::moveUpBtnClicked" );

	Song *pSong = engine->getSong();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();

	if ( ( nSelectedInstrument - 1 ) >= 0 ) {
		Instrument *pTemp = pInstrumentList->get( nSelectedInstrument - 1 );
		pInstrumentList->replace( pInstrumentList->get( nSelectedInstrument ), nSelectedInstrument - 1 );
		pInstrumentList->replace( pTemp, nSelectedInstrument );

/*
		// devo spostare tutte le note...
		PatternList *pPatternList = pSong->getPatternList();
		for ( int nPattern = 0; nPattern < pPatternList->getSize(); nPattern++ ) {
			Pattern *pPattern = pPatternList->get( nPattern );
			Sequence *pSeq1 = pPattern->m_pSequenceList->get( nSelectedInstrument );
			Sequence *pSeq2 = pPattern->m_pSequenceList->get( nSelectedInstrument - 1 );

			// swap notelist..
			map <int, Note*> noteList = pSeq1->m_noteList;
			pSeq1->m_noteList = pSeq2->m_noteList;
			pSeq2->m_noteList = noteList;
		}
*/
		AudioEngine::get_instance()->unlock();
		engine->setSelectedInstrumentNumber( nSelectedInstrument - 1 );

		pSong->m_bIsModified = true;
	}
	else {
		AudioEngine::get_instance()->unlock();
	}
}



void PatternEditorPanel::moveDownBtnClicked(Button *)
{
	Hydrogen *engine = Hydrogen::get_instance();
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	AudioEngine::get_instance()->lock( "PatternEditorPanel::moveDownBtnClicked" );

	Song *pSong = engine->getSong();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();

	if ( ( nSelectedInstrument + 1 ) < (int)pInstrumentList->get_size() ) {
		Instrument *pTemp = pInstrumentList->get( nSelectedInstrument + 1 );
		pInstrumentList->replace( pInstrumentList->get( nSelectedInstrument ), nSelectedInstrument + 1 );
		pInstrumentList->replace( pTemp, nSelectedInstrument );

/*
		// devo spostare tutte le note...
		PatternList *pPatternList = pSong->getPatternList();
		for ( int nPattern = 0; nPattern < pPatternList->getSize(); nPattern++ ) {
			Pattern *pPattern = pPatternList->get( nPattern );
			Sequence *pSeq1 = pPattern->m_pSequenceList->get( nSelectedInstrument );
			Sequence *pSeq2 = pPattern->m_pSequenceList->get( nSelectedInstrument + 1 );

			// swap notelist..
			map <int, Note*> noteList = pSeq1->m_noteList;
			pSeq1->m_noteList = pSeq2->m_noteList;
			pSeq2->m_noteList = noteList;
		}
*/
		AudioEngine::get_instance()->unlock();
		engine->setSelectedInstrumentNumber( nSelectedInstrument + 1 );

		pSong->m_bIsModified = true;
	}
	else {
		AudioEngine::get_instance()->unlock();
	}

}




void PatternEditorPanel::dragEnterEvent(QDragEnterEvent *event)
{
	m_pInstrumentList->dragEnterEvent( event );
}



void PatternEditorPanel::dropEvent(QDropEvent *event)
{
	m_pInstrumentList->dropEvent( event );
}



void PatternEditorPanel::propertiesComboChanged( QString text )
{
	if ( text == trUtf8( "Velocity" ) ) {
		m_pNotePanScrollView->hide();
		m_pNoteVelocityScrollView->show();

		m_pNoteVelocityEditor->updateEditor();
	}
	else if ( text == trUtf8( "Pan" ) ) {
		m_pNoteVelocityScrollView->hide();
		m_pNotePanScrollView->show();

		m_pNotePanEditor->updateEditor();
	}
	else if ( text == trUtf8( "Cutoff" ) ) {
	}
	else if ( text == trUtf8( "Resonance" ) ) {
	}
	else {
		ERRORLOG( "Unknown text: " + text.toStdString() );
	}
}
