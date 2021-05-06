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

#include <core/Preferences.h>
#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/AudioEngine.h>
#include <core/EventQueue.h>
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
#include "../Widgets/LCD.h"
#include "../WidgetScrollArea.h"

#include "../Skin.h"
#include "../SongEditor/SongEditorPanel.h"

#include <cmath>


void PatternEditorPanel::updateSLnameLabel( )
{
	QFont font;
	font.setBold( true );
	pSLlabel->setFont( font );
	pSLlabel->setText( Hydrogen::get_instance()->getCurrentDrumkitName() );
}

const char* PatternEditorPanel::__class_name = "PatternEditorPanel";


PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
 : QWidget( pParent )
 , Object( __class_name )
 , m_pPattern( nullptr )
{
	setAcceptDrops(true);

	Preferences *pPref = Preferences::get_instance();

	m_nCursorPosition = 0;
	m_nCursorIncrement = 0;

// Editor TOP
	PixmapWidget *editor_top = new PixmapWidget( nullptr );
	editor_top->setPixmap( "/patternEditor/editor_top.png", true );
	editor_top->setFixedHeight(24);

	PixmapWidget *editor_top_2 = new PixmapWidget( nullptr );
	editor_top_2->setPixmap( "/patternEditor/editor_top.png", true );
	editor_top_2->setFixedHeight( 24 );

	QHBoxLayout *editor_top_hbox = new QHBoxLayout( editor_top );
	editor_top_hbox->setSpacing( 0 );
	editor_top_hbox->setMargin( 0 );
	editor_top_hbox->setAlignment( Qt::AlignLeft );

	QHBoxLayout *editor_top_hbox_2 = new QHBoxLayout( editor_top_2 );
	editor_top_hbox_2->setSpacing( 0 );
	editor_top_hbox_2->setMargin( 0 );
	editor_top_hbox_2->setAlignment( Qt::AlignLeft );


	//soundlibrary name
	pSLlabel = new QLabel( nullptr );
	pSLlabel->setText( Hydrogen::get_instance()->getCurrentDrumkitName() );
	pSLlabel->setFixedSize( 170, 20 );
	pSLlabel->move( 10, 3 );
	pSLlabel->setToolTip( tr( "Loaded Soundlibrary" ) );
	editor_top_hbox->addWidget( pSLlabel );

//wolke some background images back_size_res
	PixmapWidget *pSizeResol = new PixmapWidget( nullptr );
	pSizeResol->setFixedSize( 216, 20 );
	pSizeResol->setPixmap( "/patternEditor/background_res-new.png" );
	pSizeResol->move( 0, 3 );
	editor_top_hbox_2->addWidget( pSizeResol );

	// PATTERN size
	__pattern_size_LCD = new LCDDisplay( pSizeResol, LCDDigit::SMALL_BLUE, 10 );
	__pattern_size_LCD->move( 31, 2 );
	__pattern_size_LCD->setToolTip( tr( "Select pattern size" ) );

	connect( __pattern_size_LCD, SIGNAL( displayClicked( LCDDisplay* ) ), this, SLOT( patternSizeLCDClicked() ) );
	
	m_pDenominatorWarning = new Button(
			pSizeResol,
			"/patternEditor/icn_warning.png",
			"/patternEditor/icn_warning.png",
			"/patternEditor/icn_warning.png",
			QSize( 15, 13 ) );
	m_pDenominatorWarning->move( 114, 2 );
	m_pDenominatorWarning->hide();
	m_pDenominatorWarning->setToolTip( tr( "Unsupported note denominator. Click for more information." ) );
	connect( m_pDenominatorWarning, SIGNAL( clicked( Button* ) ), this, SLOT( denominatorWarningClicked() ) );
	
	// GRID resolution
	__resolution_combo = new LCDCombo( pSizeResol , 5, true );
	__resolution_combo->setToolTip(tr( "Select grid resolution" ));
	__resolution_combo->addItem( QString( "  1/4   - " )
								 .append( tr( "quarter" ) ) );
	__resolution_combo->addItem( QString( "  1/8   - " )
								 .append( tr( "eighth" ) ) );
	__resolution_combo->addItem( QString( " 1/16  - " )
								 .append( tr( "sixteenth" ) ) );
	__resolution_combo->addItem( QString( " 1/32  - " )
								 .append( tr( "thirty-second" ) ) );
	__resolution_combo->addItem( QString( " 1/64  - " )
								 .append( tr( "sixty-fourth" ) ) );
	__resolution_combo->addSeparator();
	__resolution_combo->addItem( QString( " 1/4T  - " )
								 .append( tr( "quarter triplet" ) ) );
	__resolution_combo->addItem( QString( " 1/8T  - " )
								 .append( tr( "eighth triplet" ) ) );
	__resolution_combo->addItem( QString( "1/16T - " )
								 .append( tr( "sixteenth triplet" ) ) );
	__resolution_combo->addItem( QString( "1/32T - " )
								 .append( tr( "thirty-second triplet" ) ) );
	__resolution_combo->addSeparator();
	__resolution_combo->addItem( tr( "off" ) );
	__resolution_combo->move( 154, 2 );
	// is triggered from inside PatternEditorPanel()
	connect( __resolution_combo, SIGNAL( valueChanged( int ) ), this, SLOT( gridResolutionChanged( int ) ) );


	PixmapWidget *pRec = new PixmapWidget( nullptr );
	pRec->setFixedSize( 300, 20 );
	pRec->setPixmap( "/patternEditor/background_rec-new.png" );
	pRec->move( 0, 3 );
	editor_top_hbox_2->addWidget( pRec );


	// Hear notes btn
	ToggleButton *hearNotesBtn = new ToggleButton(
			pRec,
			"/patternEditor/btn_hear_on.png",
			"/patternEditor/btn_hear_off.png",
			"/patternEditor/btn_hear_off.png",
			QSize( 15, 13 )
	);
	hearNotesBtn->move( 34, 3 );
	hearNotesBtn->setToolTip( tr( "Hear new notes" ) );
	connect( hearNotesBtn, SIGNAL( clicked( Button* ) ), this, SLOT( hearNotesBtnClick( Button* ) ) );
	hearNotesBtn->setPressed( pPref->getHearNewNotes() );
	hearNotesBtn->setObjectName( "HearNotesBtn" );


	// quantize
	ToggleButton* quantizeEventsBtn = new ToggleButton(
			pRec,
			"/patternEditor/btn_quant_on.png",
			"/patternEditor/btn_quant_off.png",
			"/patternEditor/btn_quant_off.png",
			QSize(15, 13)
	);
	quantizeEventsBtn->move( 90, 3 );
	quantizeEventsBtn->setPressed( pPref->getQuantizeEvents() );
	quantizeEventsBtn->setToolTip( tr( "Quantize keyboard/midi events to grid" ) );
	quantizeEventsBtn->setObjectName( "QuantizeEventsBtn" );
	connect( quantizeEventsBtn, SIGNAL( clicked( Button* ) ), this, SLOT( quantizeEventsBtnClick( Button* ) ) );

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
	__show_drum_btn->setToolTip( tr( "Show piano roll editor" ) );
	__show_drum_btn->setObjectName( "ShowDrumBtn" );
	connect( __show_drum_btn, SIGNAL( clicked( Button* ) ), this, SLOT( showDrumEditorBtnClick( Button* ) ) );

	// zoom-in btn
	Button *zoom_in_btn = new Button(
			nullptr,
			"/songEditor/btn_new_on.png",
			"/songEditor/btn_new_off.png",
			"/songEditor/btn_new_over.png",
			QSize( 19, 13 )
	);
	zoom_in_btn->setToolTip( tr( "Zoom in" ) );
	connect( zoom_in_btn, SIGNAL( clicked( Button* ) ), this, SLOT( zoomInBtnClicked( Button* ) ) );


	// zoom-out btn
	Button *zoom_out_btn = new Button(
			nullptr,
			"/songEditor/btn_minus_on.png",
			"/songEditor/btn_minus_off.png",
			"/songEditor/btn_minus_over.png",
			QSize(19, 13)
	);
	zoom_out_btn->setToolTip( tr( "Zoom out" ) );
	connect( zoom_out_btn, SIGNAL( clicked( Button* ) ), this, SLOT( zoomOutBtnClicked(Button*) ) );


// End Editor TOP


// RULER____________________________________

	// Ruler ScrollView
	m_pRulerScrollView = new WidgetScrollArea( nullptr );
	m_pRulerScrollView->setFocusPolicy( Qt::NoFocus );
	m_pRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pRulerScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pRulerScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pRulerScrollView->setFixedHeight( 25 );
	// Ruler
	m_pPatternEditorRuler = new PatternEditorRuler( m_pRulerScrollView->viewport() );
	m_pPatternEditorRuler->setFocusPolicy( Qt::ClickFocus );

	m_pRulerScrollView->setWidget( m_pPatternEditorRuler );
	connect( m_pRulerScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ), this,
																			SLOT( on_patternEditorHScroll( int ) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pPatternEditorRuler, &PatternEditorRuler::onPreferencesChanged );


//~ RULER


// EDITOR _____________________________________
	// Editor scrollview
	m_pEditorScrollView = new WidgetScrollArea( nullptr );
	m_pEditorScrollView->setObjectName( "EditorScrollView" );
	m_pEditorScrollView->setFocusPolicy( Qt::NoFocus );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pEditorScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );


	// Editor
	m_pDrumPatternEditor = new DrumPatternEditor( m_pEditorScrollView->viewport(), this );

	m_pEditorScrollView->setWidget( m_pDrumPatternEditor );
	m_pEditorScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pEditorScrollView->setFocusProxy( m_pDrumPatternEditor );

	m_pPatternEditorRuler->setFocusProxy( m_pEditorScrollView );

	connect( m_pEditorScrollView->verticalScrollBar(), SIGNAL( valueChanged( int ) ), this,
																			SLOT( on_patternEditorVScroll( int ) ) );
	connect( m_pEditorScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ), this, 
																			SLOT( on_patternEditorHScroll( int ) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pDrumPatternEditor, &DrumPatternEditor::onPreferencesChanged );

//PianoRollEditor
	m_pPianoRollScrollView = new WidgetScrollArea( nullptr );
	m_pPianoRollScrollView->setObjectName( "PianoRollScrollView" );
	m_pPianoRollScrollView->setFocusPolicy( Qt::NoFocus );
	m_pPianoRollScrollView->setFrameShape( QFrame::NoFrame );
	m_pPianoRollScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pPianoRollScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPianoRollEditor = new PianoRollEditor( m_pPianoRollScrollView->viewport(), this, m_pPianoRollScrollView );
	m_pPianoRollScrollView->setWidget( m_pPianoRollEditor );
	connect( m_pPianoRollScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorHScroll(int) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pPianoRollEditor, &PianoRollEditor::onPreferencesChanged );

	m_pPianoRollScrollView->hide();
	m_pPianoRollScrollView->setFocusProxy( m_pPianoRollEditor );

	m_pPianoRollEditor->mergeSelectionGroups( m_pDrumPatternEditor );

//~ EDITOR






// INSTRUMENT LIST
	// Instrument list scrollview
	m_pInstrListScrollView = new WidgetScrollArea( nullptr );
	m_pInstrListScrollView->setObjectName( "InstrListScrollView" );
	m_pInstrListScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pInstrListScrollView->setFrameShape( QFrame::NoFrame );
	m_pInstrListScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pInstrListScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	// Instrument list
	m_pInstrumentList = new PatternEditorInstrumentList( m_pInstrListScrollView->viewport(), this );
	m_pInstrListScrollView->setWidget( m_pInstrumentList );
	m_pInstrListScrollView->setFixedWidth( m_pInstrumentList->width() );
	m_pInstrumentList->setFocusPolicy( Qt::ClickFocus );
	m_pInstrumentList->setFocusProxy( m_pEditorScrollView );

	connect( m_pInstrListScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorVScroll(int) ) );
	m_pInstrListScrollView->setFocusProxy( m_pInstrumentList );

//~ INSTRUMENT LIST




// NOTE_VELOCITY EDITOR
	m_pNoteVelocityScrollView = new WidgetScrollArea( nullptr );
	m_pNoteVelocityScrollView->setObjectName( "NoteVelocityScrollView" );
	m_pNoteVelocityScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteVelocityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteVelocityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityEditor = new NotePropertiesRuler( m_pNoteVelocityScrollView->viewport(), this,
																					NotePropertiesRuler::VELOCITY );
	m_pNoteVelocityScrollView->setWidget( m_pNoteVelocityEditor );
	m_pNoteVelocityScrollView->setFixedHeight( 100 );
	connect( m_pNoteVelocityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorHScroll(int) ) );

	m_pNoteVelocityEditor->mergeSelectionGroups( m_pDrumPatternEditor );

//~ NOTE_VELOCITY EDITOR


// NOTE_PAN EDITOR
	m_pNotePanScrollView = new WidgetScrollArea( nullptr );
	m_pNotePanScrollView->setObjectName( "NotePanScrollView" );
	m_pNotePanScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNotePanScrollView->setFrameShape( QFrame::NoFrame );
	m_pNotePanScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanEditor = new NotePropertiesRuler( m_pNotePanScrollView->viewport(), this, NotePropertiesRuler::PAN );
	m_pNotePanScrollView->setWidget( m_pNotePanEditor );
	m_pNotePanScrollView->setFixedHeight( 100 );

	connect( m_pNotePanScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );

	m_pNotePanEditor->mergeSelectionGroups( m_pDrumPatternEditor );

//~ NOTE_PAN EDITOR


// NOTE_LEADLAG EDITOR
	m_pNoteLeadLagScrollView = new WidgetScrollArea( nullptr );
	m_pNoteLeadLagScrollView->setObjectName( "NoteLeadLagScrollView" );
	m_pNoteLeadLagScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteLeadLagScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteLeadLagScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagEditor = new NotePropertiesRuler( m_pNoteLeadLagScrollView->viewport(), this,
																						NotePropertiesRuler::LEADLAG );
	m_pNoteLeadLagScrollView->setWidget( m_pNoteLeadLagEditor );
	m_pNoteLeadLagScrollView->setFixedHeight( 100 );

	connect( m_pNoteLeadLagScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );

	m_pNoteLeadLagEditor->mergeSelectionGroups( m_pDrumPatternEditor );

//~ NOTE_LEADLAG EDITOR


// NOTE_NOTEKEY EDITOR


	m_pNoteNoteKeyScrollView = new WidgetScrollArea( nullptr );
	m_pNoteNoteKeyScrollView->setObjectName( "NoteNoteKeyScrollView" );
	m_pNoteNoteKeyScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteNoteKeyScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteNoteKeyScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteNoteKeyScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteNoteKeyEditor = new NotePropertiesRuler( m_pNoteNoteKeyScrollView->viewport(), this,
																						NotePropertiesRuler::NOTEKEY );
	m_pNoteNoteKeyScrollView->setWidget( m_pNoteNoteKeyEditor );
	m_pNoteNoteKeyScrollView->setFixedHeight( 210 );
	connect( m_pNoteNoteKeyScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ), this,
																			SLOT( on_patternEditorHScroll( int ) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pNoteNoteKeyEditor, &NotePropertiesRuler::onPreferencesChanged );
	
	m_pNoteNoteKeyEditor->mergeSelectionGroups( m_pDrumPatternEditor );

//~ NOTE_NOTEKEY EDITOR

// NOTE_PROBABILITY EDITOR
	m_pNoteProbabilityScrollView = new WidgetScrollArea( nullptr );
	m_pNoteProbabilityScrollView->setObjectName( "NoteProbabilityScrollView" );
	m_pNoteProbabilityScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteProbabilityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteProbabilityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityEditor = new NotePropertiesRuler( m_pNoteProbabilityScrollView->viewport(), this,
																					NotePropertiesRuler::PROBABILITY );
	m_pNoteProbabilityScrollView->setWidget( m_pNoteProbabilityEditor );
	m_pNoteProbabilityScrollView->setFixedHeight( 100 );
	connect( m_pNoteProbabilityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	
	m_pNoteProbabilityEditor->mergeSelectionGroups( m_pDrumPatternEditor );

//~ NOTE_PROBABILITY EDITOR



	// external horizontal scrollbar
	m_pPatternEditorHScrollBar = new QScrollBar( Qt::Horizontal , nullptr  );
	m_pPatternEditorHScrollBar->setObjectName( "PatternEditorHScrollBar" );
	connect( m_pPatternEditorHScrollBar, SIGNAL( valueChanged( int ) ), this,
																	SLOT( syncToExternalHorizontalScrollbar( int ) ) );

	// external vertical scrollbar
	m_pPatternEditorVScrollBar = new QScrollBar( Qt::Vertical, nullptr );
	m_pPatternEditorVScrollBar->setObjectName( "PatternEditorVScrollBar" );
	connect( m_pPatternEditorVScrollBar, SIGNAL(valueChanged( int)), this,
																	SLOT( syncToExternalHorizontalScrollbar(int) ) );

	QHBoxLayout *pPatternEditorHScrollBarLayout = new QHBoxLayout();
	pPatternEditorHScrollBarLayout->setSpacing( 0 );
	pPatternEditorHScrollBarLayout->setMargin( 0 );
	pPatternEditorHScrollBarLayout->addWidget( m_pPatternEditorHScrollBar );
	pPatternEditorHScrollBarLayout->addWidget( zoom_in_btn );
	pPatternEditorHScrollBarLayout->addWidget( zoom_out_btn );

	QWidget *pPatternEditorHScrollBarContainer = new QWidget();
	pPatternEditorHScrollBarContainer->setLayout( pPatternEditorHScrollBarLayout );


	QPalette label_palette;
	label_palette.setColor( QPalette::WindowText, QColor( 230, 230, 230 ) );

	QFont boldFont;
	boldFont.setBold( true );
	m_pPatternNameLbl = new QLabel( nullptr );
	m_pPatternNameLbl->setFont( boldFont );
	m_pPatternNameLbl->setText( "pattern name label" );
	m_pPatternNameLbl->setPalette(label_palette);






// NOTE_PROPERTIES BUTTONS
	PixmapWidget *pPropertiesPanel = new PixmapWidget( nullptr );
	pPropertiesPanel->setObjectName( "PropertiesPanel" );
	pPropertiesPanel->setColor( QColor( 58, 62, 72 ) );

	pPropertiesPanel->setFixedSize( 181, 100 );

	QVBoxLayout *pPropertiesVBox = new QVBoxLayout( pPropertiesPanel );
	pPropertiesVBox->setSpacing( 0 );
	pPropertiesVBox->setMargin( 0 );


	__pPropertiesCombo = new LCDCombo( nullptr, 20 );
	__pPropertiesCombo->setToolTip( tr( "Select note properties" ) );
	__pPropertiesCombo->addItem( tr("Velocity") );
	__pPropertiesCombo->addItem( tr("Pan") );
	__pPropertiesCombo->addItem( tr("Lead and Lag") );
	__pPropertiesCombo->addItem( tr("NoteKey") );
	__pPropertiesCombo->addItem( tr("Probability") );
	/* __pPropertiesCombo->addItem( tr("Cutoff") ); */
	/* __pPropertiesCombo->addItem( tr("Resonance") ); */
	// is triggered here below
	__pPropertiesCombo->setObjectName( "PropertiesCombo" );
	connect( __pPropertiesCombo, SIGNAL( valueChanged( int ) ), this, SLOT( propertiesComboChanged( int ) ) );

	pPropertiesVBox->addWidget( __pPropertiesCombo );

//~ NOTE_PROPERTIES BUTTONS


// LAYOUT
	QWidget *pMainPanel = new QWidget();

	QGridLayout *pGrid = new QGridLayout();
	pGrid->setSpacing( 0 );
	pGrid->setMargin( 0 );

	pGrid->addWidget( editor_top, 0, 0 );
	pGrid->addWidget( editor_top_2, 0, 1, 1, 3 );
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
	if ( nRes == MAX_NOTES ) {
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
				ERRORLOG( QString( "Wrong grid resolution: %1" ).arg( pPref->getPatternEditorGridResolution() ) );
		}
	} else {
		switch ( nRes ) {
			case  8: nIndex = 6; break;
			case 16: nIndex = 7; break;
			case 32: nIndex = 8; break;
			case 64: nIndex = 9; break;
			default:
				nIndex = 6;
				ERRORLOG( QString( "Wrong grid resolution: %1" ).arg( pPref->getPatternEditorGridResolution() ) );
		}
	}
	__resolution_combo->select( nIndex );

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



void PatternEditorPanel::syncToExternalHorizontalScrollbar( int )
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


void PatternEditorPanel::on_patternEditorVScroll( int nValue )
{
	//INFOLOG( "[on_patternEditorVScroll] " + QString::number(nValue)  );
	m_pPatternEditorVScrollBar->setValue( nValue );	
	resizeEvent( nullptr );
}

void PatternEditorPanel::on_patternEditorHScroll( int nValue )
{
	//INFOLOG( "[on_patternEditorHScroll] " + QString::number(nValue)  );
	m_pPatternEditorHScrollBar->setValue( nValue );	
	resizeEvent( nullptr );
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
		nResolution = 0x1 << ( nSelected - 3 );
	}
	else {
		nResolution = 0x1 << ( nSelected + 2 );
	}

	// INFOLOG( QString("idx %1 -> %2 resolution").arg( nSelected ).arg( nResolution ) );

	m_pDrumPatternEditor->setResolution( nResolution, bUseTriplets );
	m_pPianoRollEditor->setResolution( nResolution, bUseTriplets );
	m_pNoteVelocityEditor->setResolution( nResolution, bUseTriplets );
	m_pNoteLeadLagEditor->setResolution( nResolution, bUseTriplets );
	m_pNoteNoteKeyEditor->setResolution( nResolution, bUseTriplets );
	m_pNoteProbabilityEditor->setResolution( nResolution, bUseTriplets );
	m_pNotePanEditor->setResolution( nResolution, bUseTriplets );

	m_nCursorIncrement = ( bUseTriplets ? 4 : 3 ) * MAX_NOTES / ( nResolution * 3 );
	m_nCursorPosition = m_nCursorIncrement * ( m_nCursorPosition / m_nCursorIncrement );

	Preferences::get_instance()->setPatternEditorGridResolution( nResolution );
	Preferences::get_instance()->setPatternEditorUsingTriplets( bUseTriplets );
}



void PatternEditorPanel::selectedPatternChangedEvent()
{
	PatternList *pPatternList = Hydrogen::get_instance()->getSong()->getPatternList();
	int nSelectedPatternNumber = Hydrogen::get_instance()->getSelectedPatternNumber();

	if ( ( nSelectedPatternNumber != -1 ) && ( (uint) nSelectedPatternNumber < pPatternList->size() ) ) {
		// update pattern name text
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
		QString sCurrentPatternName = m_pPattern->get_name();
		this->setWindowTitle( ( tr( "Pattern editor - %1" ).arg( sCurrentPatternName ) ) );
		m_pPatternNameLbl->setText( sCurrentPatternName );

		// update pattern size LCD
		updatePatternSizeLCD();
		
	}
	else {
		m_pPattern = nullptr;

		this->setWindowTitle( ( tr( "Pattern editor - %1" ).arg(QString( "No pattern selected." ) ) ) );
		m_pPatternNameLbl->setText( tr( "No pattern selected" ) );
	}

	resizeEvent( nullptr ); // force an update of the scrollbars
}



void PatternEditorPanel::hearNotesBtnClick(Button *ref)
{
	Preferences *pref = ( Preferences::get_instance() );
	pref->setHearNewNotes( ref->isPressed() );

	if (ref->isPressed() ) {
		( HydrogenApp::get_instance() )->setStatusBarMessage( tr( "Hear new notes = On" ), 2000 );
	}
	else {
		( HydrogenApp::get_instance() )->setStatusBarMessage( tr( "Hear new notes = Off" ), 2000 );
	}

}

void PatternEditorPanel::quantizeEventsBtnClick( Button *ref )
{
	Preferences *pref = ( Preferences::get_instance() );
	pref->setQuantizeEvents( ref->isPressed() );

	if (ref->isPressed() ) {
		( HydrogenApp::get_instance() )->setStatusBarMessage( tr( "Quantize incoming keyboard/midi events = On" ),
																												2000 );
	}
	else {
		( HydrogenApp::get_instance() )->setStatusBarMessage( tr( "Quantize incoming keyboard/midi events = Off" ),
																												2000 );
	}
}

static void syncScrollBarSize( QScrollBar *pDest, QScrollBar *pSrc )
{
	pDest->setMinimum( pSrc->minimum() );
	pDest->setMaximum( pSrc->maximum() );
	pDest->setSingleStep( pSrc->singleStep() );
	pDest->setPageStep( pSrc->pageStep() );
}

void PatternEditorPanel::resizeEvent( QResizeEvent *ev )
{
	UNUSED( ev );
	QScrollArea *pScrollArea = m_pEditorScrollView;

	syncScrollBarSize( m_pPatternEditorHScrollBar, pScrollArea->horizontalScrollBar() );
	syncScrollBarSize( m_pPatternEditorVScrollBar, pScrollArea->verticalScrollBar() );

	syncScrollBarSize( m_pRulerScrollView->horizontalScrollBar(), pScrollArea->horizontalScrollBar() );
	syncScrollBarSize( m_pNoteVelocityScrollView->horizontalScrollBar(), pScrollArea->horizontalScrollBar() );
	syncScrollBarSize( m_pNotePanScrollView->horizontalScrollBar(), pScrollArea->horizontalScrollBar() );
	syncScrollBarSize( m_pNoteLeadLagScrollView->horizontalScrollBar(), pScrollArea->horizontalScrollBar() ) ;
	syncScrollBarSize( m_pNoteNoteKeyScrollView->horizontalScrollBar(), pScrollArea->horizontalScrollBar() );
	syncScrollBarSize( m_pNoteProbabilityScrollView->horizontalScrollBar(), pScrollArea->horizontalScrollBar() );
}

void PatternEditorPanel::showEvent ( QShowEvent *ev )
{
	UNUSED( ev );
}


/// richiamato dall'uso dello scroll del mouse
void PatternEditorPanel::contentsMoving( int dummy )
{
	UNUSED( dummy );
	//INFOLOG( "contentsMoving" );
	syncToExternalHorizontalScrollbar(0);
}



void PatternEditorPanel::selectedInstrumentChangedEvent()
{
	resizeEvent( nullptr );	// force a scrollbar update
}

void PatternEditorPanel::selectInstrumentNotes( int nInstrument )
{
	if ( __show_drum_btn->isPressed() ) {
		m_pPianoRollEditor->selectInstrumentNotes( nInstrument );
	} else {
		m_pDrumPatternEditor->selectInstrumentNotes( nInstrument );
	}
}

void PatternEditorPanel::showDrumEditor()
{
	__show_drum_btn->setToolTip( tr( "Show piano roll editor" ) );
	__show_drum_btn->setPressed( false );
	m_pPianoRollScrollView->hide();
	m_pEditorScrollView->show();
	m_pInstrListScrollView->show();

	m_pEditorScrollView->setFocus();
	m_pPatternEditorRuler->setFocusProxy( m_pEditorScrollView );
	m_pInstrumentList->setFocusProxy( m_pEditorScrollView );

	m_pDrumPatternEditor->selectedInstrumentChangedEvent(); // force an update

	m_pDrumPatternEditor->selectNone();
	m_pPianoRollEditor->selectNone();

	// force a re-sync of extern scrollbars
	resizeEvent( nullptr );

}

void PatternEditorPanel::showPianoRollEditor()
{
	__show_drum_btn->setToolTip( tr( "Show drum editor" ) );
	__show_drum_btn->setPressed( true );
	m_pPianoRollScrollView->show();
	m_pPianoRollScrollView->verticalScrollBar()->setValue( 250 );
	m_pEditorScrollView->hide();
	m_pInstrListScrollView->show();

	m_pPianoRollScrollView->setFocus();
	m_pPatternEditorRuler->setFocusProxy( m_pPianoRollScrollView );
	m_pInstrumentList->setFocusProxy( m_pPianoRollScrollView );

	m_pDrumPatternEditor->selectNone();
	m_pPianoRollEditor->selectNone();

	m_pPianoRollEditor->selectedPatternChangedEvent();
	m_pPianoRollEditor->updateEditor(); // force an update
	// force a re-sync of extern scrollbars
	resizeEvent( nullptr );
}

void PatternEditorPanel::showDrumEditorBtnClick( Button *ref )
{
	UNUSED( ref );
	if ( !__show_drum_btn->isPressed() ){
		showDrumEditor();
	}
	else
	{
		showPianoRollEditor();
	}
}


void PatternEditorPanel::zoomInBtnClicked( Button *ref )
{
	if( m_pPatternEditorRuler->getGridWidth() >= 24 ){
		return;
	}
	UNUSED( ref );
	m_pPatternEditorRuler->zoomIn();
	m_pDrumPatternEditor->zoomIn();
	m_pNoteVelocityEditor->zoomIn();
	m_pNoteLeadLagEditor->zoomIn();
	m_pNoteNoteKeyEditor->zoomIn();
	m_pNoteProbabilityEditor->zoomIn();
	m_pNotePanEditor->zoomIn();
	m_pPianoRollEditor->zoomIn();
	
	Preferences::get_instance()->setPatternEditorGridWidth( m_pPatternEditorRuler->getGridWidth() );
	Preferences::get_instance()->setPatternEditorGridHeight( m_pDrumPatternEditor->getGridHeight() );

	resizeEvent( nullptr );
}



void PatternEditorPanel::zoomOutBtnClicked( Button *ref )
{
	UNUSED( ref );
	m_pPatternEditorRuler->zoomOut();
	m_pDrumPatternEditor->zoomOut();
	m_pNoteVelocityEditor->zoomOut();
	m_pNoteLeadLagEditor->zoomOut();
	m_pNoteNoteKeyEditor->zoomOut();
	m_pNoteProbabilityEditor->zoomOut();
	m_pNotePanEditor->zoomOut();
	m_pPianoRollEditor->zoomOut();

	resizeEvent( nullptr );
	
	Preferences::get_instance()->setPatternEditorGridWidth( m_pPatternEditorRuler->getGridWidth() );
	Preferences::get_instance()->setPatternEditorGridHeight( m_pDrumPatternEditor->getGridHeight() );
}


void PatternEditorPanel::updateEditors( bool bPatternOnly ) {
	m_pPatternEditorRuler->updateEditor( true );
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePanEditor->updateEditor();
	m_pNoteLeadLagEditor->updateEditor();
	m_pNoteNoteKeyEditor->updateEditor();
	m_pNoteProbabilityEditor->updateEditor();
	m_pPianoRollEditor->updateEditor( bPatternOnly );
	m_pDrumPatternEditor->updateEditor();
}


void PatternEditorPanel::patternLengthChanged()
{
	// INFOLOG( QString("idx %1 -> %2 eighth").arg( nSelected ).arg( ( MAX_NOTES / 8 ) * ( nSelected + 1 ) ) );

	if ( !m_pPattern ) {
		return;
	}

	updatePatternSizeLCD();

	updateEditors();
	resizeEvent( nullptr );

	EventQueue::get_instance()->push_event( EVENT_SELECTED_PATTERN_CHANGED, -1 );
}

void PatternEditorPanel::updatePatternSizeLCD(){
	// update pattern size
	int nPatternSize = m_pPattern->get_length(); // in ticks
	int nDen = m_pPattern->get_denominator();
	QString qtmp;
	
	// Note: numerator = ( nPatternSize * nDen ) / MAX_NOTES
	if ( ( nPatternSize * nDen ) % MAX_NOTES == 0 ) { // numerator is integer. Print with no decimal digits.
		qtmp = QString( "%1/%2" ).arg(( nPatternSize * nDen ) / MAX_NOTES ).arg( nDen );
	} 
	else { // numerator is not integer
				/* Note: print numerator using 3 decimal digits: enough for the resolution = 192 ticks/whole note.
					In fact the minimum representable note value is 1/192 of a whole note = 0.00520333 whole notes 
					or alternatively 1/48 of a quarter note = 0.02083333 quarter notes. */
		QLocale loc = QLocale::system(); // to use locale decimal separator
		qtmp = QString( "%1/%2" ).arg( loc.toString( ( nPatternSize * nDen ) / (float) MAX_NOTES, 'f', 3 ) ).arg( nDen );
	}
	__pattern_size_LCD->setText( qtmp );

	// hide or show warning icon if denominator doesn't divide MAX_NOTES
		/* Note: warning even if ( nPatternSize * nDen ) % MAX_NOTES == 0 . In that case the displayed numerator is
			integer and correct (e.g. size = 5/5), but the user may think that denominator is fully supported. */
	if( MAX_NOTES % nDen != 0 )	{
		m_pDenominatorWarning->show();
	}
	else {
		m_pDenominatorWarning->hide();
	}
}

void PatternEditorPanel::denominatorWarningClicked()
{
	QMessageBox::information( this, "Hydrogen",
							  tr( "Hydrogen can only represent notes as small as 1/%1 of a whole note, "
							  	  "so note values must be multiple of this.\nSupported values are: "
							  	  "1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/48, 1/64, 1/96, 1/192" 
							  	  ).arg( MAX_NOTES ) );
							  	  // Note: the previous values are valid if and only if MAX_NOTES = 192
}

void PatternEditorPanel::patternSizeLCDClicked()
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	if ( pEngine->getState() != STATE_READY ) {	
		QMessageBox::information( this, "Hydrogen", tr( "Is not possible to change the pattern size when playing." ) );
		return;
	} // TODO is it really impossible to change the pattern size when playing?

	bool bIsOkPressed;
	int denominator;
	
	QString qtmp = QInputDialog::getText( this, "Hydrogen", tr( "New Pattern length (beats/note value)" ),
													QLineEdit::Normal, __pattern_size_LCD->getText(), &bIsOkPressed );
													//Note: actually is (beats * note value) but looks less clear
	
	if ( bIsOkPressed ) {
	    if	(__pattern_size_LCD->getText() == qtmp ) { // text unchanged
	    	return;
	    }
	    
		QStringList parts = qtmp.split( '/' );
		parts[0].replace( ",", "." ); // allowing both point or comma decimal separator
		int nDenominator;
		if ( parts.size() == 1 || parts.size() == 2 ) { // must reject if parts.size > 2 or null
		    bool bOk;
		    double fNumerator = parts[0].toDouble( &bOk );
		    if ( bOk && parts.size() == 2 ) { // user entered both numerator and denominator
				nDenominator = parts[1].toInt( &bOk );
				if ( bOk && ( nDenominator <= 0 || nDenominator > MAX_NOTES ) ) {
			   		QMessageBox::information( this, "Hydrogen", tr( "Denominator value rejected.\nLimits: (0, %1]"
			   														 ).arg(MAX_NOTES) );
			   		return;
				}
		    }
		    else { // user entered numerator only. keep the current pattern denominator
		    	nDenominator = m_pPattern->get_denominator();
		    } 
		    if ( bOk && fNumerator > 0 ) {
				if ( fNumerator / nDenominator > 4. ) { 
					 // pattern size is limited because the pattern editor ruler goes up to 16/4. Limit might be extended
					QMessageBox::information( this, "Hydrogen", tr( "Pattern size too big.\nMaximum = 16/4" ) );
					return;
				}
				else {	 
					if ( MAX_NOTES % nDenominator != 0 ) {
						QMessageBox::information( this, "Hydrogen", tr( "Pattern length in 1/%1 notes is not supported. "
																	"Length may be approximated.").arg( nDenominator ) );
						/* Note: such denominators are not rejected even if not supported:
						in fact user can input a non integer numerator and this feature is very powerful
						because it allows to set really any possible pattern size (in ticks) using ANY arbitrary denominator.
						e.g. pattern size of 38 ticks will result from both inputs 1/5 (quintuplet) and 0.79/4 of a whole note,
						since both are rounded and BOTH are UNSUPPORTED, but the first notation looks more meaningful */
					}

					int nLength = round( MAX_NOTES / (double) nDenominator * fNumerator );

					// set length and denominator				
					m_pPattern->set_length( nLength);
					m_pPattern->set_denominator( nDenominator );
					patternLengthChanged();

					/* Message to tell the user why the pattern display won't respect the input value. 
					Note: constant = 1000 since the displayed numerator has 3 decimal digits*/
					int displayedNum_x1000 = round( (double) nLength / MAX_NOTES * nDenominator * 1000 );
					int roundInputNum_x1000 = round( fNumerator * 1000);
					if ( displayedNum_x1000 != roundInputNum_x1000 ) {
						QMessageBox::information( this, "Hydrogen",
							tr( "Pattern size was approximated.\n(resolution = %1 ticks/quarter note)" ).arg( MAX_NOTES / 4 ));
					}
				}
		    }
		    else { // user entered invalid text
		    	QMessageBox::information( this, "Hydrogen", tr( "Text rejected" ) );
		    	return;
		    }
		}
		else { // last case: user entered more than 2 slashes
		    	QMessageBox::information( this, "Hydrogen", tr( "Text rejected" ) );
		    	return;
		}
	}
}


void PatternEditorPanel::moveUpBtnClicked( Button * )
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	AudioEngine* pAudioEngine = pHydrogen->getAudioEngine();

	int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();

	pAudioEngine->lock( RIGHT_HERE );

	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();

	if ( ( nSelectedInstrument - 1 ) >= 0 ) {
		pInstrumentList->swap( nSelectedInstrument -1, nSelectedInstrument );

		pAudioEngine->unlock();
		pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument - 1 );

		pSong->setIsModified( true );
	}
	else {
		pAudioEngine->unlock();
	}
}



void PatternEditorPanel::moveDownBtnClicked( Button * )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	AudioEngine* pAudioEngine = pHydrogen->getAudioEngine();

	int nSelectedInstrument = pHydrogen->getSelectedInstrumentNumber();

	pAudioEngine->lock( RIGHT_HERE );

	Song *pSong = pHydrogen->getSong();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();

	if ( ( nSelectedInstrument + 1 ) < (int)pInstrumentList->size() ) {
		pInstrumentList->swap( nSelectedInstrument, nSelectedInstrument + 1 );

		pAudioEngine->unlock();
		pHydrogen->setSelectedInstrumentNumber( nSelectedInstrument + 1 );

		pSong->setIsModified( true );
	}
	else {
		pAudioEngine->unlock();
	}

}




void PatternEditorPanel::dragEnterEvent( QDragEnterEvent *event )
{
	m_pInstrumentList->dragEnterEvent( event );
}



void PatternEditorPanel::dropEvent( QDropEvent *event )
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
		ERRORLOG( QString( "unhandled value : %1" ).arg( nSelected ) );
	}
}

void PatternEditorPanel::updatePianorollEditor()
{
	m_pDrumPatternEditor->updateEditor(); // force an update
}

int PatternEditorPanel::getCursorPosition()
{
	return m_nCursorPosition;
}

void PatternEditorPanel::ensureCursorVisible()
{
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	uint y = nSelectedInstrument * Preferences::get_instance()->getPatternEditorGridHeight();
	m_pEditorScrollView->ensureVisible( m_nCursorPosition * m_pPatternEditorRuler->getGridWidth(), y );
}

void PatternEditorPanel::setCursorPosition(int nCursorPosition)
{
	if ( nCursorPosition < 0 ) {
		m_nCursorPosition = 0;
	} else if ( nCursorPosition >= m_pPattern->get_length() ) {
		m_nCursorPosition = m_pPattern->get_length() - m_nCursorIncrement;
	} else {
		m_nCursorPosition = nCursorPosition;
	}
}

int PatternEditorPanel::moveCursorLeft( int n )
{
	m_nCursorPosition = std::max( m_nCursorPosition - m_nCursorIncrement * n,
								  0 );

	ensureCursorVisible();

	return m_nCursorPosition;
}

int PatternEditorPanel::moveCursorRight( int n )
{
	m_nCursorPosition = std::min( m_nCursorPosition + m_nCursorIncrement * n,
								  m_pPattern->get_length() - m_nCursorIncrement );

	ensureCursorVisible();

	return m_nCursorPosition;
}
