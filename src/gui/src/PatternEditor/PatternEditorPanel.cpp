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

#include <hydrogen/Preferences.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/event_queue.h>
using namespace H2Core;


#include "HydrogenApp.h"
#include "PatternEditorPanel.h"
#include "PatternEditorInstrumentList.h"
#include "PatternEditorRuler.h"
#include "NotePropertiesRuler.h"
#include "DrumPatternEditor.h"
#include "PianoRollEditor.h"

#include "../MainForm.h"
#include "../Widgets/Button.h"
#include "../Widgets/Fader.h"
#include "../Widgets/PixmapWidget.h"
#include "../Widgets/LCDCombo.h"

#include "../Skin.h"
#include "../SongEditor/SongEditorPanel.h"

#include <cmath>


void PatternEditorPanel::updateSLnameLabel( )
{
	QFont font;
	font.setBold( true );
	pSLlabel->setFont( font );
	pSLlabel->setText( Hydrogen::get_instance()->m_currentDrumkit  );
}

const char* PatternEditorPanel::__class_name = "PatternEditorPanel";


PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
 : QWidget( pParent )
 , Object( __class_name )
 , m_pPattern( NULL )
 , m_bEnablePatternResize( true )
{
	setAcceptDrops(true);

	Preferences *pPref = Preferences::get_instance();
	

// Editor TOP
	PixmapWidget *editor_top = new PixmapWidget(0);
	editor_top->setPixmap("/patternEditor/editor_top.png", true);
	editor_top->setFixedHeight(24);

	PixmapWidget *editor_top_2 = new PixmapWidget(0);
	editor_top_2->setPixmap("/patternEditor/editor_top.png", true);
	editor_top_2->setFixedHeight(24);

	QHBoxLayout *editor_top_hbox = new QHBoxLayout(editor_top);
	editor_top_hbox->setSpacing(0);
	editor_top_hbox->setMargin(0);
	editor_top_hbox->setAlignment(Qt::AlignLeft);

	QHBoxLayout *editor_top_hbox_2 = new QHBoxLayout(editor_top_2);
	editor_top_hbox_2->setSpacing(0);
	editor_top_hbox_2->setMargin(0);
	editor_top_hbox_2->setAlignment(Qt::AlignLeft);


	//soundlibrary name
	pSLlabel = new QLabel( NULL );
	pSLlabel->setText( Hydrogen::get_instance()->m_currentDrumkit );
	pSLlabel->setFixedSize( 170, 20 );
	pSLlabel->move( 10, 3 );
	pSLlabel->setToolTip( trUtf8("Loaded Soundlibrary") );
	editor_top_hbox->addWidget( pSLlabel );

//wolke some background images back_size_res
	PixmapWidget *pSizeResol = new PixmapWidget( NULL );
	pSizeResol->setFixedSize( 200, 20 );
	pSizeResol->setPixmap( "/patternEditor/background_res-new.png" );
	pSizeResol->move( 0, 3 );
	editor_top_hbox_2->addWidget( pSizeResol );

	// PATTERN size
	__pattern_size_combo = new LCDCombo(pSizeResol, 4);
	__pattern_size_combo->move( 34, 2 );
	__pattern_size_combo->setToolTip( trUtf8("Select pattern size") );
	for ( int i = 1; i <= 64; i++) {
		__pattern_size_combo->addItem( QString( "%1" ).arg( i ) );
	}
	// is triggered from inside selectedPatternChangedEvent()
	connect(__pattern_size_combo, SIGNAL( valueChanged( int ) ), this, SLOT( patternSizeChanged( int ) ) );


	// GRID resolution
	__resolution_combo = new LCDCombo( pSizeResol , 7);
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
	__resolution_combo->move( 121, 2 );
	// is triggered from inside PatternEditorPanel()
	connect(__resolution_combo, SIGNAL(valueChanged( int )), this, SLOT(gridResolutionChanged( int )));


	PixmapWidget *pRec = new PixmapWidget( NULL );
	pRec->setFixedSize( 158, 20 );
	pRec->setPixmap( "/patternEditor/background_rec-new.png" );
	pRec->move( 0, 3 );
	editor_top_hbox_2->addWidget( pRec );


	// Hear notes btn
	ToggleButton *hearNotesBtn = new ToggleButton(
			pRec,
			"/patternEditor/btn_hear_on.png",
			"/patternEditor/btn_hear_off.png",
			"/patternEditor/btn_hear_off.png",
			QSize(15, 13)
	);
	hearNotesBtn->move( 34, 3 );
	hearNotesBtn->setToolTip( trUtf8( "Hear new notes" ) );
	connect( hearNotesBtn, SIGNAL(clicked(Button*)), this, SLOT( hearNotesBtnClick(Button*)));
	hearNotesBtn->setPressed( pPref->getHearNewNotes() );


	// quantize
	ToggleButton* quantizeEventsBtn = new ToggleButton(
			pRec,
			"/patternEditor/btn_quant_on.png",
			"/patternEditor/btn_quant_off.png",
			"/patternEditor/btn_quant_off.png",
			QSize(15, 13)
	);
	quantizeEventsBtn->move( 90, 3 );
	quantizeEventsBtn->setPressed( pPref->getQuantizeEvents());
	quantizeEventsBtn->setToolTip( trUtf8( "Quantize keyboard/midi events to grid" ) );
	connect( quantizeEventsBtn, SIGNAL(clicked(Button*)), this, SLOT( quantizeEventsBtnClick(Button*)));

	// Editor mode
	__show_drum_btn = new ToggleButton(
				pRec,
				"/patternEditor/btn_drum_piano_on.png",
				"/patternEditor/btn_drum_piano_off.png",
				"/patternEditor/btn_drum_piano_off.png",
				QSize(17, 13)
				);
	__show_drum_btn->move( 137, 3 );
	__show_drum_btn->setPressed( false );
	__show_drum_btn->setToolTip( trUtf8( "Show piano roll editor" ) );
	connect(__show_drum_btn, SIGNAL(clicked(Button*)), this, SLOT( showDrumEditorBtnClick(Button*)));

	__recpredelete = new QComboBox( NULL );
	__recpredelete->setFixedSize( 130, 20 );
	__recpredelete->move( 2, 1 );
	__recpredelete->addItem ( QString( "On play" ));
	__recpredelete->addItem ( QString( "On rec: once fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/1 fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/2 fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/4 fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/8 fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/16 fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/32 fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/64 fp" ));
	__recpredelete->addItem ( QString( "On rec: 1/64" ));
	__recpredelete->addItem ( QString( "On rec: 1/32" ));
	__recpredelete->addItem ( QString( "On rec: 1/16" ));
	__recpredelete->addItem ( QString( "On rec: 1/8" ));
	__recpredelete->addItem ( QString( "On rec: 1/4" ));
	__recpredelete->addItem ( QString( "On rec: 1/2" ));
	__recpredelete->addItem ( QString( "On rec: 1/1" ));
	__recpredelete->addItem ( QString( "On rec: once" ));
	__recpredelete->update();
	__recpredelete->setToolTip( trUtf8( "destructive mode pre delete settings" ) );
	editor_top_hbox_2->addWidget( __recpredelete );
	connect( __recpredelete, SIGNAL( currentIndexChanged( int ) ), this, SLOT( recPreDeleteSelect( int) ) );

	__recpostdelete = new QComboBox( NULL );
	__recpostdelete->setFixedSize( 60, 20 );
	__recpostdelete->move( 2, 1 );
	__recpostdelete->addItem ( QString( "off" ));
	__recpostdelete->addItem ( QString( "1/64" ));
	__recpostdelete->addItem ( QString( "1/32" ));
	__recpostdelete->addItem ( QString( "1/16" ));
	__recpostdelete->addItem ( QString( "1/8" ));
	__recpostdelete->addItem ( QString( "1/4" ));
	__recpostdelete->addItem ( QString( "1/2" ));
	__recpostdelete->addItem ( QString( "1/1" ));
	__recpostdelete->update();
	__recpostdelete->setToolTip( trUtf8( "destructive mode post delete settings" ) );
	editor_top_hbox_2->addWidget( __recpostdelete );
	connect( __recpostdelete, SIGNAL( currentIndexChanged( int ) ), this, SLOT( recPostDeleteSelect( int) ) );


	// zoom-in btn
	Button *zoom_in_btn = new Button(
			NULL,
			"/songEditor/btn_new_on.png",
			"/songEditor/btn_new_off.png",
			"/songEditor/btn_new_over.png",
			QSize(19, 13)
	);
	zoom_in_btn->setToolTip( trUtf8( "Zoom in" ) );
	connect(zoom_in_btn, SIGNAL(clicked(Button*)), this, SLOT( zoomInBtnClicked(Button*) ) );


	// zoom-out btn
	Button *zoom_out_btn = new Button(
			NULL,
			"/songEditor/btn_minus_on.png",
			"/songEditor/btn_minus_off.png",
			"/songEditor/btn_minus_over.png",
			QSize(19, 13)
	);
	zoom_out_btn->setToolTip( trUtf8( "Zoom out" ) );
	connect( zoom_out_btn, SIGNAL(clicked(Button*)), this, SLOT( zoomOutBtnClicked(Button*) ) );


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


//PianoRollEditor
	m_pPianoRollScrollView = new QScrollArea( NULL );
	m_pPianoRollScrollView->setFrameShape( QFrame::NoFrame );
	m_pPianoRollScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pPianoRollScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPianoRollEditor = new PianoRollEditor( m_pPianoRollScrollView->viewport(), this );
	m_pPianoRollScrollView->setWidget( m_pPianoRollEditor );

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


// NOTE_PAN EDITOR
	m_pNotePanScrollView = new QScrollArea( NULL );
	m_pNotePanScrollView->setFrameShape( QFrame::NoFrame );
	m_pNotePanScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanEditor = new NotePropertiesRuler( m_pNotePanScrollView->viewport(), this, NotePropertiesRuler::PAN );
	m_pNotePanScrollView->setWidget( m_pNotePanEditor );
	m_pNotePanScrollView->setFixedHeight( 100 );
//~ NOTE_PAN EDITOR


// NOTE_LEADLAG EDITOR
	m_pNoteLeadLagScrollView = new QScrollArea( NULL );
	m_pNoteLeadLagScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteLeadLagScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagEditor = new NotePropertiesRuler( m_pNoteLeadLagScrollView->viewport(), this, NotePropertiesRuler::LEADLAG );
	m_pNoteLeadLagScrollView->setWidget( m_pNoteLeadLagEditor );
	m_pNoteLeadLagScrollView->setFixedHeight( 100 );
//~ NOTE_LEADLAG EDITOR


// NOTE_NOTEKEY EDITOR


	m_pNoteNoteKeyScrollView = new QScrollArea( NULL );
	m_pNoteNoteKeyScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteNoteKeyScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteNoteKeyScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteNoteKeyEditor = new NotePropertiesRuler( m_pNoteNoteKeyScrollView->viewport(), this, NotePropertiesRuler::NOTEKEY );
	m_pNoteNoteKeyScrollView->setWidget( m_pNoteNoteKeyEditor );
	m_pNoteNoteKeyScrollView->setFixedHeight( 210 );


//~ NOTE_NOTEKEY EDITOR

// NOTE_PROBABILITY EDITOR
	m_pNoteProbabilityScrollView = new QScrollArea( NULL );
	m_pNoteProbabilityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteProbabilityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityEditor = new NotePropertiesRuler( m_pNoteProbabilityScrollView->viewport(), this, NotePropertiesRuler::PROBABILITY );
	m_pNoteProbabilityScrollView->setWidget( m_pNoteProbabilityEditor );
	m_pNoteProbabilityScrollView->setFixedHeight( 100 );
//~ NOTE_PROBABILITY EDITOR



	// external horizontal scrollbar
	m_pPatternEditorHScrollBar = new QScrollBar( Qt::Horizontal , NULL  );
	connect( m_pPatternEditorHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	// external vertical scrollbar
	m_pPatternEditorVScrollBar = new QScrollBar( Qt::Vertical, NULL );
	connect( m_pPatternEditorVScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	QHBoxLayout *pPatternEditorHScrollBarLayout = new QHBoxLayout();
	pPatternEditorHScrollBarLayout->setSpacing( 0 );
	pPatternEditorHScrollBarLayout->setMargin( 0 );
	pPatternEditorHScrollBarLayout->addWidget( m_pPatternEditorHScrollBar );
	pPatternEditorHScrollBarLayout->addWidget( zoom_in_btn );
	pPatternEditorHScrollBarLayout->addWidget( zoom_out_btn );

	QWidget *pPatternEditorHScrollBarContainer = new QWidget();
	pPatternEditorHScrollBarContainer->setLayout( pPatternEditorHScrollBarLayout );


	QPalette label_palette;
	label_palette.setColor( QPalette::Foreground, QColor( 230, 230, 230 ) );

	QFont boldFont;
	boldFont.setBold( true );
	m_pPatternNameLbl = new QLabel( NULL );
	m_pPatternNameLbl->setFont( boldFont );
	m_pPatternNameLbl->setText( "pattern name label" );
	m_pPatternNameLbl->setPalette(label_palette);






// NOTE_PROPERTIES BUTTONS
	PixmapWidget *pPropertiesPanel = new PixmapWidget( NULL );
	pPropertiesPanel->setColor( QColor( 58, 62, 72 ) );

	pPropertiesPanel->setFixedSize( 181, 100 );

	QVBoxLayout *pPropertiesVBox = new QVBoxLayout( pPropertiesPanel );
	pPropertiesVBox->setSpacing( 0 );
	pPropertiesVBox->setMargin( 0 );


	__pPropertiesCombo = new LCDCombo( NULL, 20);
	__pPropertiesCombo->setToolTip(trUtf8("Select note properties"));
	__pPropertiesCombo->addItem( trUtf8("Velocity") );
	__pPropertiesCombo->addItem( trUtf8("Pan") );
	__pPropertiesCombo->addItem( trUtf8("Lead and Lag") );
	__pPropertiesCombo->addItem( trUtf8("NoteKey") );
	__pPropertiesCombo->addItem( trUtf8("Probability") );
	/* __pPropertiesCombo->addItem( trUtf8("Cutoff") ); */
	/* __pPropertiesCombo->addItem( trUtf8("Resonance") ); */
	// is triggered here below
	connect( __pPropertiesCombo, SIGNAL(valueChanged( int )), this, SLOT(propertiesComboChanged( int )));

	pPropertiesVBox->addWidget( __pPropertiesCombo );

//~ NOTE_PROPERTIES BUTTONS


// LAYOUT
	QWidget *pMainPanel = new QWidget();

	QGridLayout *pGrid = new QGridLayout();
	pGrid->setSpacing( 0 );
	pGrid->setMargin( 0 );

	pGrid->addWidget( editor_top, 0, 0);
	pGrid->addWidget( editor_top_2, 0, 1, 1, 3);
	pGrid->addWidget( m_pPatternNameLbl, 1, 0 );
	pGrid->addWidget( m_pRulerScrollView, 1, 1 );

	pGrid->addWidget( m_pInstrListScrollView, 2, 0 );

	pGrid->addWidget( m_pEditorScrollView, 2, 1 );
	pGrid->addWidget( m_pPianoRollScrollView, 2, 1 );

	pGrid->addWidget( m_pPatternEditorVScrollBar, 2, 2 );
	pGrid->addWidget( pPatternEditorHScrollBarContainer, 10, 1 );
	pGrid->addWidget( m_pNoteVelocityScrollView, 4, 1 );
	pGrid->addWidget( m_pNotePanScrollView, 4, 1 );
	pGrid->addWidget( m_pNoteLeadLagScrollView, 4, 1 );
	pGrid->addWidget( m_pNoteNoteKeyScrollView, 4, 1 );
	pGrid->addWidget( m_pNoteProbabilityScrollView, 4, 1 );

	pGrid->addWidget( pPropertiesPanel, 4, 0 );
	pGrid->setRowStretch( 2, 100 );
	pMainPanel->setLayout( pGrid );


	// restore grid resolution
	int nIndex;
	int nRes = pPref->getPatternEditorGridResolution();
	if (nRes == MAX_NOTES) {
		nIndex = 11;
	} else if ( pPref->isPatternEditorUsingTriplets() == false ) {
		switch ( nRes ) {
			case  4: nIndex = 0; break;
			case  8: nIndex = 1; break;
			case 16: nIndex = 2; break;
			case 32: nIndex = 3; break;
			case 64: nIndex = 4; break;
			default:
				nIndex = 0;
				ERRORLOG( QString("Wrong grid resolution: %1").arg( pPref->getPatternEditorGridResolution() ) );
		}
	} else {
		switch ( nRes ) {
			case  8: nIndex = 6; break;
			case 16: nIndex = 7; break;
			case 32: nIndex = 8; break;
			case 64: nIndex = 9; break;
			default:
				nIndex = 6;
				ERRORLOG( QString("Wrong grid resolution: %1").arg( pPref->getPatternEditorGridResolution() ) );
		}
	}
	__resolution_combo->select( nIndex );

	//set pre delete
	__recpredelete->setCurrentIndex(pPref->m_nRecPreDelete);
	__recpostdelete->setCurrentIndex(pPref->m_nRecPostDelete);
	displayorHidePrePostCB();

	// LAYOUT
	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );
	this->setLayout( pVBox );

	pVBox->addWidget( pMainPanel );

	HydrogenApp::get_instance()->addEventListener( this );

	// update
	__pPropertiesCombo->select( 0 );
	selectedPatternChangedEvent();
}




PatternEditorPanel::~PatternEditorPanel()
{
}



void PatternEditorPanel::syncToExternalHorizontalScrollbar(int)
{
	//INFOLOG( "[syncToExternalHorizontalScrollbar]" );

	// drum Editor
	m_pEditorScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
	m_pEditorScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );

	// piano roll Editor
	m_pPianoRollScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// Ruler
	m_pRulerScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// Instrument list
	m_pInstrListScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );

	// Velocity ruler
	m_pNoteVelocityScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// pan ruler
	m_pNotePanScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// leadlag ruler
	m_pNoteLeadLagScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// notekey ruler
	m_pNoteNoteKeyScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

	// Probability ruler
	m_pNoteProbabilityScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );
}


void PatternEditorPanel::on_patternEditorScroll(int nValue)
{
	//INFOLOG( "[on_patternEditorScroll] " + QString::number(nValue)  );
	m_pPatternEditorVScrollBar->setValue( nValue );	
	resizeEvent(NULL);
}




void PatternEditorPanel::gridResolutionChanged( int nSelected )
{
	int nResolution;
	bool bUseTriplets = false;

	if ( nSelected == 11 ) {
		nResolution = MAX_NOTES;
	}
	else if ( nSelected > 4 ) {
		bUseTriplets = true;
		nResolution = 0x1 << (nSelected - 3);
	}
	else {
		nResolution = 0x1 << (nSelected + 2);
	}

	// INFOLOG( QString("idx %1 -> %2 resolution").arg( nSelected ).arg( nResolution ) );
	m_pDrumPatternEditor->setResolution( nResolution, bUseTriplets );
	m_pPianoRollEditor->setResolution( nResolution, bUseTriplets );

	Preferences::get_instance()->setPatternEditorGridResolution( nResolution );
	Preferences::get_instance()->setPatternEditorUsingTriplets( bUseTriplets );
}



void PatternEditorPanel::selectedPatternChangedEvent()
{
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->get_pattern_list();
	int nSelectedPatternNumber = Hydrogen::get_instance()->getSelectedPatternNumber();

	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() ) ) {
		// update pattern name text
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
		QString sCurrentPatternName = m_pPattern->get_name();
		this->setWindowTitle( ( trUtf8( "Pattern editor - %1").arg( sCurrentPatternName ) ) );
		m_pPatternNameLbl->setText( sCurrentPatternName );

		// update pattern size combobox
		int nPatternSize = m_pPattern->get_length();
		int n16th = MAX_NOTES / 16;
		__pattern_size_combo->select( (nPatternSize / n16th) - 1 );
	}
	else {
		m_pPattern = NULL;

		this->setWindowTitle( ( trUtf8( "Pattern editor - %1").arg(QString("No pattern selected.")) ) );
		m_pPatternNameLbl->setText( trUtf8( "No pattern selected" ) );
	}

	resizeEvent( NULL ); // force an update of the scrollbars
}



void PatternEditorPanel::hearNotesBtnClick(Button *ref)
{
	Preferences *pref = ( Preferences::get_instance() );
	pref->setHearNewNotes( ref->isPressed() );

	if (ref->isPressed() ) {
		( HydrogenApp::get_instance() )->setStatusBarMessage( trUtf8( "Hear new notes = On" ), 2000 );
	}
	else {
		( HydrogenApp::get_instance() )->setStatusBarMessage( trUtf8( "Hear new notes = Off" ), 2000 );
	}

}




void PatternEditorPanel::quantizeEventsBtnClick(Button *ref)
{
	Preferences *pref = ( Preferences::get_instance() );
	pref->setQuantizeEvents( ref->isPressed() );

	if (ref->isPressed() ) {
		( HydrogenApp::get_instance() )->setStatusBarMessage( trUtf8( "Quantize incoming keyboard/midi events = On" ), 2000 );
	}
	else {
		( HydrogenApp::get_instance() )->setStatusBarMessage( trUtf8( "Quantize incoming keyboard/midi events = Off" ), 2000 );
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
	QScrollArea *pScrollArea = m_pEditorScrollView;


	pScrollArea = m_pEditorScrollView;

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
  //m_pNoteLeadLagEditor->updateEditor();

	resizeEvent(NULL);	// force a scrollbar update
}


void PatternEditorPanel::showDrumEditorBtnClick(Button *ref)
{
	UNUSED( ref );
	if ( !__show_drum_btn->isPressed() ){
		__show_drum_btn->setToolTip( trUtf8( "Show piano roll editor" ) );
		m_pPianoRollScrollView->hide();
		m_pEditorScrollView->show();
		m_pInstrListScrollView->show();
	
		m_pDrumPatternEditor->selectedInstrumentChangedEvent(); // force an update
	
		// force a re-sync of extern scrollbars
		resizeEvent( NULL );

	}
	else
	{
		__show_drum_btn->setToolTip( trUtf8( "Show drum editor" ) );
		m_pPianoRollScrollView->show();
		m_pPianoRollScrollView->verticalScrollBar()->setValue( 250 );
		m_pEditorScrollView->show();
		m_pInstrListScrollView->show();
	
		m_pPianoRollEditor->selectedPatternChangedEvent();
		m_pPianoRollEditor->updateEditor(); // force an update	
		// force a re-sync of extern scrollbars
		resizeEvent( NULL );
	}
}


void PatternEditorPanel::zoomInBtnClicked(Button *ref)
{
	if(m_pPatternEditorRuler->getGridWidth() >=24){
		return;
	}
	UNUSED( ref );
	m_pPatternEditorRuler->zoomIn();
	m_pDrumPatternEditor->zoom_in();
	m_pNoteVelocityEditor->zoomIn();
	m_pNoteLeadLagEditor->zoomIn();
	m_pNoteNoteKeyEditor->zoomIn();
	m_pNoteProbabilityEditor->zoomIn();
	m_pNotePanEditor->zoomIn();
	m_pPianoRollEditor->zoom_in();		

	resizeEvent( NULL );
}



void PatternEditorPanel::zoomOutBtnClicked(Button *ref)
{
	UNUSED( ref );
	m_pPatternEditorRuler->zoomOut();
	m_pDrumPatternEditor->zoom_out();
	m_pNoteVelocityEditor->zoomOut();
	m_pNoteLeadLagEditor->zoomOut();
	m_pNoteNoteKeyEditor->zoomOut();
	m_pNoteProbabilityEditor->zoomOut();
	m_pNotePanEditor->zoomOut();
	m_pPianoRollEditor->zoom_out();	

	resizeEvent( NULL );
}



void PatternEditorPanel::patternSizeChanged( int nSelected )
{
	// INFOLOG( QString("idx %1 -> %2 eigth").arg( nSelected ).arg( ( MAX_NOTES / 8 ) * ( nSelected + 1 ) ) );

	if ( !m_pPattern ) {
		return;
	}

	int n16th = MAX_NOTES / 16;

	if ( !m_bEnablePatternResize ) {
		__pattern_size_combo->select( ((m_pPattern->get_length() / n16th) - 1), false );
		QMessageBox::information( this, "Hydrogen", trUtf8( "Is not possible to change the pattern size when playing." ) );
		return;
	}

	m_pPattern->set_length( n16th * ( nSelected + 1 ) );

	m_pPatternEditorRuler->updateEditor( true );	// redraw all
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePanEditor->updateEditor();
	m_pNoteLeadLagEditor->updateEditor();
	m_pNoteNoteKeyEditor->updateEditor();
	m_pNoteProbabilityEditor->updateEditor();
	m_pPianoRollEditor->updateEditor();

	resizeEvent( NULL );

	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}



void PatternEditorPanel::moveUpBtnClicked(Button *)
{
	Hydrogen *engine = Hydrogen::get_instance();
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *pSong = engine->getSong();
	InstrumentList *pInstrumentList = pSong->get_instrument_list();

	if ( ( nSelectedInstrument - 1 ) >= 0 ) {
		pInstrumentList->swap( nSelectedInstrument -1, nSelectedInstrument );

		AudioEngine::get_instance()->unlock();
		engine->setSelectedInstrumentNumber( nSelectedInstrument - 1 );

		pSong->set_is_modified( true );
	}
	else {
		AudioEngine::get_instance()->unlock();
	}
}



void PatternEditorPanel::moveDownBtnClicked(Button *)
{
	Hydrogen *engine = Hydrogen::get_instance();
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *pSong = engine->getSong();
	InstrumentList *pInstrumentList = pSong->get_instrument_list();

	if ( ( nSelectedInstrument + 1 ) < (int)pInstrumentList->size() ) {
		pInstrumentList->swap( nSelectedInstrument, nSelectedInstrument + 1 );

		AudioEngine::get_instance()->unlock();
		engine->setSelectedInstrumentNumber( nSelectedInstrument + 1 );

		pSong->set_is_modified( true );
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



void PatternEditorPanel::propertiesComboChanged( int nSelected )
{
	if ( nSelected == 0 ) {				// Velocity
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteNoteKeyScrollView->hide();
		m_pNoteVelocityScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNoteVelocityEditor->updateEditor();
	}
	else if ( nSelected == 1 ) {		// Pan
		m_pNoteVelocityScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteNoteKeyScrollView->hide();
		m_pNotePanScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNotePanEditor->updateEditor();
	}
	else if ( nSelected == 2 ) {		// Lead and Lag
		m_pNoteVelocityScrollView->hide();
		m_pNotePanScrollView->hide();
		m_pNoteNoteKeyScrollView->hide();
		m_pNoteLeadLagScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNoteLeadLagEditor->updateEditor();
	}
	else if ( nSelected == 3 ) {		// NoteKey
		m_pNoteVelocityScrollView->hide();
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteNoteKeyScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNoteNoteKeyEditor->updateEditor();
	}
	else if ( nSelected == 4 ) {		// Probability
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteNoteKeyScrollView->hide();
		m_pNoteVelocityScrollView->hide();
		m_pNoteProbabilityScrollView->show();

		m_pNoteProbabilityEditor->updateEditor();
	}
	/*
	else if ( nSelected == 5 ) {		// Cutoff
	}
	else if ( nSelected == 6 ) {		// Resonance
	}
	*/
	else {
		ERRORLOG( QString("unhandeled value : %1").arg(nSelected) );
	}
}


void PatternEditorPanel::recPreDeleteSelect( int index )
{
	Preferences::get_instance()->m_nRecPreDelete = index;
	if( index>=9 && index <=15 ){
		__recpostdelete->show();
	}else{
		__recpostdelete->hide();
	}
}


void PatternEditorPanel::recPostDeleteSelect( int index )
{
	Preferences::get_instance()->m_nRecPostDelete = index;
}


void PatternEditorPanel::displayorHidePrePostCB()
{
	int index = __recpredelete->currentIndex();
	if( Preferences::get_instance()->getDestructiveRecord() ){
		__recpostdelete->show();
		if( index>=8 && index <=14 ){
			__recpostdelete->show();
		}else{
			__recpostdelete->hide();
		}
		__recpredelete->show();
	}else{
		__recpostdelete->hide();
		__recpredelete->hide();
	}
}

void PatternEditorPanel::updatePianorollEditor()
{
	m_pDrumPatternEditor->updateEditor(); // force an update
}
