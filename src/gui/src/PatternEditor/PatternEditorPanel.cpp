/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2025 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include <core/Hydrogen.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/EventQueue.h>
using namespace H2Core;


#include "HydrogenApp.h"
#include "PatternEditorPanel.h"
#include "PatternEditorInstrumentList.h"
#include "PatternEditorRuler.h"
#include "NotePropertiesRuler.h"
#include "DrumPatternEditor.h"
#include "PianoRollEditor.h"

#include "../UndoActions.h"
#include "../MainForm.h"
#include "../Widgets/Button.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/Fader.h"
#include "../Widgets/PixmapWidget.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/LCDSpinBox.h"
#include "../WidgetScrollArea.h"

#include "../CommonStrings.h"
#include "../SongEditor/SongEditorPanel.h"

#include <cmath>


void PatternEditorPanel::updateSLnameLabel( )
{
	auto pPref = H2Core::Preferences::get_instance();
	
	QFont font( Preferences::get_instance()->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	font.setBold( true );
	m_pSLlabel->setFont( font );
	m_pSLlabel->setText( Hydrogen::get_instance()->getLastLoadedDrumkitName() );
}


PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
 : QWidget( pParent )
 , m_pPattern( nullptr )
 , m_nSelectedPatternNumber( -1 )
 , m_bArmPatternSizeSpinBoxes( true )
{
	setAcceptDrops(true);

	Preferences *pPref = Preferences::get_instance();
	
	QFont boldFont( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	boldFont.setBold( true );

	m_nCursorPosition = 0;
	m_nCursorIncrement = 0;

	// Spacing between a label and the widget to its label.
	const int nLabelSpacing = 6;
// Editor TOP
	
	m_pEditorTop1 = new QWidget( nullptr );
	m_pEditorTop1->setFixedHeight(24);
	m_pEditorTop1->setObjectName( "editor1" );

	m_pEditorTop2 = new QWidget( nullptr );
	m_pEditorTop2->setFixedHeight( 24 );
	m_pEditorTop2->setObjectName( "editor2" );

	QHBoxLayout *m_pEditorTop1_hbox = new QHBoxLayout( m_pEditorTop1 );
	m_pEditorTop1_hbox->setSpacing( 0 );
	m_pEditorTop1_hbox->setMargin( 0 );
	m_pEditorTop1_hbox->setAlignment( Qt::AlignLeft );

	QHBoxLayout *m_pEditorTop1_hbox_2 = new QHBoxLayout( m_pEditorTop2 );
	m_pEditorTop1_hbox_2->setSpacing( 2 );
	m_pEditorTop1_hbox_2->setMargin( 0 );
	m_pEditorTop1_hbox_2->setAlignment( Qt::AlignLeft );


	//soundlibrary name
	m_pSLlabel = new QLabel( nullptr );
	m_pSLlabel->setFont( boldFont );
	m_pSLlabel->setText( Hydrogen::get_instance()->getLastLoadedDrumkitName() );
	m_pSLlabel->setFixedSize( 170, 20 );
	m_pSLlabel->move( 10, 3 );
	m_pSLlabel->setToolTip( tr( "Loaded Soundlibrary" ) );
	m_pEditorTop1_hbox->addWidget( m_pSLlabel );

//wolke some background images back_size_res
	m_pSizeResol = new QWidget( nullptr );
	m_pSizeResol->setObjectName( "sizeResol" );
	m_pSizeResol->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	m_pSizeResol->move( 0, 3 );
	m_pEditorTop1_hbox_2->addWidget( m_pSizeResol );

	QHBoxLayout* pSizeResolLayout = new QHBoxLayout( m_pSizeResol );
	pSizeResolLayout->setContentsMargins( 2, 0, 2, 0 );
	pSizeResolLayout->setSpacing( 2 );

	// PATTERN size
	m_pPatternSizeLbl = new ClickableLabel( m_pSizeResol, QSize( 0, 0 ), HydrogenApp::get_instance()->getCommonStrings()->getPatternSizeLabel(), ClickableLabel::Color::Dark );
	m_pPatternSizeLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pPatternSizeLbl );
	
	m_pLCDSpinBoxNumerator = new LCDSpinBox( this, QSize( 62, 20 ), LCDSpinBox::Type::Double, 0.1, 16.0, true );
	m_pLCDSpinBoxNumerator->setKind( LCDSpinBox::Kind::PatternSizeNumerator );
	connect( m_pLCDSpinBoxNumerator, &LCDSpinBox::slashKeyPressed, this, &PatternEditorPanel::switchPatternSizeFocus );
	connect( m_pLCDSpinBoxNumerator, SIGNAL( valueChanged( double ) ), this, SLOT( patternSizeChanged( double ) ) );
	m_pLCDSpinBoxNumerator->setKeyboardTracking( false );
	m_pLCDSpinBoxNumerator->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pLCDSpinBoxNumerator );
			
	QLabel* pLabel1 = new ClickableLabel( m_pSizeResol, QSize( 4, 13 ), "/", ClickableLabel::Color::Dark );
	pLabel1->resize( QSize( 20, 17 ) );
	pLabel1->setText( "/" );
	pLabel1->setFont( boldFont );
	pLabel1->setToolTip( tr( "You can use the '/' inside the pattern size spin boxes to switch back and forth." ) );
	pLabel1->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( pLabel1 );
	
	m_pLCDSpinBoxDenominator = new LCDSpinBox( m_pSizeResol, QSize( 48, 20 ), LCDSpinBox::Type::Int, 1, 192, true );
	m_pLCDSpinBoxDenominator->setKind( LCDSpinBox::Kind::PatternSizeDenominator );
	connect( m_pLCDSpinBoxDenominator, &LCDSpinBox::slashKeyPressed, this, &PatternEditorPanel::switchPatternSizeFocus );
	connect( m_pLCDSpinBoxDenominator, SIGNAL( valueChanged( double ) ), this, SLOT( patternSizeChanged( double ) ) );
	m_pLCDSpinBoxDenominator->setKeyboardTracking( false );
	m_pLCDSpinBoxDenominator->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pLCDSpinBoxDenominator );
	pSizeResolLayout->addSpacing( nLabelSpacing );
	
	// GRID resolution
	m_pResolutionLbl = new ClickableLabel( m_pSizeResol, QSize( 0, 0 ), HydrogenApp::get_instance()->getCommonStrings()->getResolutionLabel(), ClickableLabel::Color::Dark );
	m_pResolutionLbl->setAlignment( Qt::AlignRight );
	m_pResolutionLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pResolutionLbl );
	
	m_pResolutionCombo = new LCDCombo( m_pSizeResol, QSize( 0, 0 ), true );
	// Large enough for "1/32T" to be fully visible at large font size.
	// m_pResolutionCombo->setToolTip(tr( "Select grid resolution" ));
	m_pResolutionCombo->insertItem( 0, QString( "1/4 - " )
								 .append( tr( "quarter" ) ) );
	m_pResolutionCombo->insertItem( 1, QString( "1/8 - " )
								 .append( tr( "eighth" ) ) );
	m_pResolutionCombo->insertItem( 2, QString( "1/16 - " )
								 .append( tr( "sixteenth" ) ) );
	m_pResolutionCombo->insertItem( 3, QString( "1/32 - " )
								 .append( tr( "thirty-second" ) ) );
	m_pResolutionCombo->insertItem( 4, QString( "1/64 - " )
								 .append( tr( "sixty-fourth" ) ) );
	m_pResolutionCombo->insertSeparator( 5 );
	m_pResolutionCombo->insertItem( 6, QString( "1/4T - " )
								 .append( tr( "quarter triplet" ) ) );
	m_pResolutionCombo->insertItem( 7, QString( "1/8T - " )
								 .append( tr( "eighth triplet" ) ) );
	m_pResolutionCombo->insertItem( 8, QString( "1/16T - " )
								 .append( tr( "sixteenth triplet" ) ) );
	m_pResolutionCombo->insertItem( 9, QString( "1/32T - " )
								 .append( tr( "thirty-second triplet" ) ) );
	m_pResolutionCombo->insertSeparator( 10 );
	m_pResolutionCombo->insertItem( 11, tr( "off" ) );
	m_pResolutionCombo->setMinimumSize( QSize( 24, 18 ) );
	m_pResolutionCombo->setMaximumSize( QSize( 500, 18 ) );
	m_pResolutionCombo->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	// is triggered from inside PatternEditorPanel()
	connect( m_pResolutionCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( gridResolutionChanged( int ) ) );
	pSizeResolLayout->addWidget( m_pResolutionCombo );

	m_pRec = new QWidget( nullptr );
	m_pRec->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
	m_pRec->setObjectName( "pRec" );
	m_pRec->move( 0, 3 );
	m_pEditorTop1_hbox_2->addWidget( m_pRec );
	
	QHBoxLayout* pRecLayout = new QHBoxLayout( m_pRec );
	pRecLayout->setContentsMargins( 2, 0, 2, 0 );
	pRecLayout->setSpacing( 2 );

	// Hear notes btn
	m_pHearNotesLbl = new ClickableLabel( m_pRec, QSize( 0, 0 ), HydrogenApp::get_instance()->getCommonStrings()->getHearNotesLabel(), ClickableLabel::Color::Dark );
	m_pHearNotesLbl->setAlignment( Qt::AlignRight );
	m_pHearNotesLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pHearNotesLbl );
	
	m_pHearNotesBtn = new Button( m_pRec, QSize( 21, 18 ), Button::Type::Toggle,
								  "speaker.svg", "", false, QSize( 15, 13 ),
								  tr( "Hear new notes" ), false, true );
	connect( m_pHearNotesBtn, SIGNAL( clicked() ), this, SLOT( hearNotesBtnClick() ) );
	m_pHearNotesBtn->setChecked( pPref->getHearNewNotes() );
	m_pHearNotesBtn->setObjectName( "HearNotesBtn" );
	m_pHearNotesBtn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pHearNotesBtn );
	pRecLayout->addSpacing( nLabelSpacing );

	// quantize
	m_pQuantizeEventsLbl = new ClickableLabel( m_pRec, QSize( 0, 0 ), HydrogenApp::get_instance()->getCommonStrings()->getQuantizeEventsLabel(), ClickableLabel::Color::Dark );
	m_pQuantizeEventsLbl->setAlignment( Qt::AlignRight );
	m_pQuantizeEventsLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pQuantizeEventsLbl );
	
	m_pQuantizeEventsBtn = new Button( m_pRec, QSize( 21, 18 ),
									   Button::Type::Toggle, "quantization.svg",
									   "", false, QSize( 15, 14 ),
									   tr( "Quantize keyboard/midi events to grid" ),
									   false, true );
	m_pQuantizeEventsBtn->setChecked( pPref->getQuantizeEvents() );
	m_pQuantizeEventsBtn->setObjectName( "QuantizeEventsBtn" );
	connect( m_pQuantizeEventsBtn, SIGNAL( clicked() ), this, SLOT( quantizeEventsBtnClick() ) );
	m_pQuantizeEventsBtn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pQuantizeEventsBtn );
	pRecLayout->addSpacing( nLabelSpacing );

	// Editor mode
	m_pShowPianoLbl = new ClickableLabel( m_pRec, QSize( 0, 0 ), HydrogenApp::get_instance()->getCommonStrings()->getShowPianoLabel(), ClickableLabel::Color::Dark );
	m_pShowPianoLbl->setAlignment( Qt::AlignRight );
	m_pShowPianoLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pShowPianoLbl );

	__show_drum_btn = new Button( m_pRec, QSize( 25, 18 ), Button::Type::Push, "drum.svg", "", false, QSize( 17, 13 ), HydrogenApp::get_instance()->getCommonStrings()->getShowPianoRollEditorTooltip() );
	__show_drum_btn->setObjectName( "ShowDrumBtn" );
	connect( __show_drum_btn, SIGNAL( clicked() ), this, SLOT( showDrumEditorBtnClick() ) );
	__show_drum_btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( __show_drum_btn );

	m_pEditorTop1_hbox_2->addStretch();
	
	// Since the button to activate the piano roll is shown
	// initially, both buttons get the same tooltip. Actually only the
	// last one does need a tooltip since it will be shown regardless
	// of whether it is hidden or not. But since this behavior might
	// change in future versions of Qt the tooltip will be assigned to
	// both of them.
	__show_piano_btn = new Button( m_pRec, QSize( 25, 18 ), Button::Type::Push, "piano.svg", "", false, QSize( 19, 15 ), HydrogenApp::get_instance()->getCommonStrings()->getShowPianoRollEditorTooltip() );
	__show_piano_btn->move( 178, 1 );
	__show_piano_btn->setObjectName( "ShowPianoBtn" );
	__show_piano_btn->hide();
	connect( __show_piano_btn, SIGNAL( clicked() ), this, SLOT( showDrumEditorBtnClick() ) );
	__show_piano_btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( __show_piano_btn );

	// zoom-in btn
	Button *zoom_in_btn = new Button( nullptr, QSize( 19, 15 ), Button::Type::Push, "plus.svg", "", false, QSize( 9, 9 ), tr( "Zoom in" ) );
	connect( zoom_in_btn, SIGNAL( clicked() ), this, SLOT( zoomInBtnClicked() ) );


	// zoom-out btn
	Button *zoom_out_btn = new Button( nullptr, QSize( 19, 15 ), Button::Type::Push, "minus.svg", "", false, QSize( 9, 9 ), tr( "Zoom out" ) );
	connect( zoom_out_btn, SIGNAL( clicked() ), this, SLOT( zoomOutBtnClicked() ) );
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


// ~ RULER


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
	connect( m_pPianoRollScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pPianoRollScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pPianoRollEditor, SLOT( scrolled( int ) ) );
	connect( m_pPianoRollScrollView->verticalScrollBar(), SIGNAL( valueChanged( int ) ),
			 m_pPianoRollEditor, SLOT( scrolled( int ) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pPianoRollEditor, &PianoRollEditor::onPreferencesChanged );

	m_pPianoRollScrollView->hide();
	m_pPianoRollScrollView->setFocusProxy( m_pPianoRollEditor );

	m_pPianoRollEditor->mergeSelectionGroups( m_pDrumPatternEditor );

// ~ EDITOR






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

// ~ INSTRUMENT LIST




// NOTE_VELOCITY EDITOR
	m_pNoteVelocityScrollView = new WidgetScrollArea( nullptr );
	m_pNoteVelocityScrollView->setObjectName( "NoteVelocityScrollView" );
	m_pNoteVelocityScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteVelocityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteVelocityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityEditor = new NotePropertiesRuler( m_pNoteVelocityScrollView->viewport(), this,
													 NotePropertiesRuler::Mode::Velocity );
	m_pNoteVelocityScrollView->setWidget( m_pNoteVelocityEditor );
	m_pNoteVelocityScrollView->setFixedHeight( 100 );
	connect( m_pNoteVelocityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNoteVelocityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNoteVelocityEditor, SLOT( scrolled( int ) ) );

	m_pNoteVelocityEditor->mergeSelectionGroups( m_pDrumPatternEditor );

// ~ NOTE_VELOCITY EDITOR


// NOTE_PAN EDITOR
	m_pNotePanScrollView = new WidgetScrollArea( nullptr );
	m_pNotePanScrollView->setObjectName( "NotePanScrollView" );
	m_pNotePanScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNotePanScrollView->setFrameShape( QFrame::NoFrame );
	m_pNotePanScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanEditor = new NotePropertiesRuler( m_pNotePanScrollView->viewport(), this,
												NotePropertiesRuler::Mode::Pan );
	m_pNotePanScrollView->setWidget( m_pNotePanEditor );
	m_pNotePanScrollView->setFixedHeight( 100 );

	connect( m_pNotePanScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNotePanScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNotePanEditor, SLOT( scrolled( int ) ) );

	m_pNotePanEditor->mergeSelectionGroups( m_pDrumPatternEditor );

// ~ NOTE_PAN EDITOR


// NOTE_LEADLAG EDITOR
	m_pNoteLeadLagScrollView = new WidgetScrollArea( nullptr );
	m_pNoteLeadLagScrollView->setObjectName( "NoteLeadLagScrollView" );
	m_pNoteLeadLagScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteLeadLagScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteLeadLagScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagEditor = new NotePropertiesRuler( m_pNoteLeadLagScrollView->viewport(), this,
													NotePropertiesRuler::Mode::LeadLag );
	m_pNoteLeadLagScrollView->setWidget( m_pNoteLeadLagEditor );
	m_pNoteLeadLagScrollView->setFixedHeight( 100 );

	connect( m_pNoteLeadLagScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNoteLeadLagScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNoteLeadLagEditor, SLOT( scrolled( int ) ) );

	m_pNoteLeadLagEditor->mergeSelectionGroups( m_pDrumPatternEditor );

// ~ NOTE_LEADLAG EDITOR


// NOTE_NOTEKEY EDITOR


	m_pNoteNoteKeyScrollView = new WidgetScrollArea( nullptr );
	m_pNoteNoteKeyScrollView->setObjectName( "NoteNoteKeyScrollView" );
	m_pNoteNoteKeyScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteNoteKeyScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteNoteKeyScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteNoteKeyScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteNoteKeyEditor = new NotePropertiesRuler( m_pNoteNoteKeyScrollView->viewport(), this,
													NotePropertiesRuler::Mode::NoteKey );
	m_pNoteNoteKeyScrollView->setWidget( m_pNoteNoteKeyEditor );
	m_pNoteNoteKeyScrollView->setFixedHeight( 210 );
	connect( m_pNoteNoteKeyScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
			 this, SLOT( on_patternEditorHScroll( int ) ) );
	connect( m_pNoteNoteKeyScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
			 m_pNoteNoteKeyEditor, SLOT( scrolled( int ) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pNoteNoteKeyEditor, &NotePropertiesRuler::onPreferencesChanged );
	
	m_pNoteNoteKeyEditor->mergeSelectionGroups( m_pDrumPatternEditor );

// ~ NOTE_NOTEKEY EDITOR

// NOTE_PROBABILITY EDITOR
	m_pNoteProbabilityScrollView = new WidgetScrollArea( nullptr );
	m_pNoteProbabilityScrollView->setObjectName( "NoteProbabilityScrollView" );
	m_pNoteProbabilityScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteProbabilityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteProbabilityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityEditor = new NotePropertiesRuler( m_pNoteProbabilityScrollView->viewport(), this,
														NotePropertiesRuler::Mode::Probability );
	m_pNoteProbabilityScrollView->setWidget( m_pNoteProbabilityEditor );
	m_pNoteProbabilityScrollView->setFixedHeight( 100 );
	connect( m_pNoteProbabilityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNoteProbabilityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNoteProbabilityEditor, SLOT( scrolled( int ) ) );
	
	m_pNoteProbabilityEditor->mergeSelectionGroups( m_pDrumPatternEditor );

// ~ NOTE_PROBABILITY EDITOR



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
	connect( m_pPatternEditorVScrollBar, SIGNAL( valueChanged( int ) ),
			 m_pDrumPatternEditor, SLOT( scrolled( int ) ) );
	connect( m_pPatternEditorHScrollBar, SIGNAL( valueChanged( int ) ),
			 m_pDrumPatternEditor, SLOT( scrolled( int ) ) );
	
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


	m_pPropertiesCombo =
		new LCDCombo( nullptr, QSize( m_pInstrumentList->width(), 18 ), false );
	m_pPropertiesCombo->setToolTip( tr( "Select note properties" ) );
	m_pPropertiesCombo->addItem( tr("Velocity") );
	m_pPropertiesCombo->addItem( tr("Pan") );
	m_pPropertiesCombo->addItem( tr("Lead and Lag") );
	m_pPropertiesCombo->addItem( tr("NoteKey") );
	m_pPropertiesCombo->addItem( tr("Probability") );
	/* m_pPropertiesCombo->addItem( tr("Cutoff") ); */
	/* m_pPropertiesCombo->addItem( tr("Resonance") ); */
	// is triggered here below
	m_pPropertiesCombo->setObjectName( "PropertiesCombo" );
	connect( m_pPropertiesCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( propertiesComboChanged( int ) ) );

	pPropertiesVBox->addWidget( m_pPropertiesCombo );

// ~ NOTE_PROPERTIES BUTTONS


// LAYOUT
	QWidget *pMainPanel = new QWidget();

	QGridLayout *pGrid = new QGridLayout();
	pGrid->setSpacing( 0 );
	pGrid->setMargin( 0 );

	pGrid->addWidget( m_pEditorTop1, 0, 0 );
	pGrid->addWidget( m_pEditorTop2, 0, 1, 1, 2 );
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
	m_pResolutionCombo->setCurrentIndex( nIndex );

	// LAYOUT
	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );
	this->setLayout( pVBox );

	pVBox->addWidget( pMainPanel );

	HydrogenApp::get_instance()->addEventListener( this );

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &PatternEditorPanel::onPreferencesChanged );

	// update
	m_pPropertiesCombo->setCurrentIndex( 0 );
	propertiesComboChanged( 0 );
	selectedPatternChangedEvent();
	updateStyleSheet();
}




PatternEditorPanel::~PatternEditorPanel()
{
}

void PatternEditorPanel::drumkitLoadedEvent() {
	updateSLnameLabel();
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

	switch( nSelected ) {
	case 0:
		// 1/4
		nResolution = 4;
		bUseTriplets = false;
		break;
	case 1:
		// 1/8
		nResolution = 8;
		bUseTriplets = false;
		break;
	case 2:
		// 1/16
		nResolution = 16;
		bUseTriplets = false;
		break;
	case 3:
		// 1/32
		nResolution = 32;
		bUseTriplets = false;
		break;
	case 4:
		// 1/64
		nResolution = 64;
		bUseTriplets = false;
		break;
	case 6:
		// 1/4T
		nResolution = 8;
		bUseTriplets = true;
		break;
	case 7:
		// 1/8T
		nResolution = 16;
		bUseTriplets = true;
		break;
	case 8:
		// 1/16T
		nResolution = 32;
		bUseTriplets = true;
		break;
	case 9:
		// 1/32T
		nResolution = 64;
		bUseTriplets = true;
		break;
	case 11:
		// off
		nResolution = MAX_NOTES;
		bUseTriplets = false;
		break;
	default:
		ERRORLOG( QString( "Invalid resolution selection [%1]" )
				  .arg( nSelected ) );
		return;
	}

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
	m_nSelectedPatternNumber = Hydrogen::get_instance()->getSelectedPatternNumber();

	if ( ( m_nSelectedPatternNumber != -1 ) &&
		 ( m_nSelectedPatternNumber < pPatternList->size() ) ) {
		// update pattern name text
		m_pPattern = pPatternList->get( m_nSelectedPatternNumber );
		QString sCurrentPatternName = m_pPattern->get_name();
		this->setWindowTitle( ( tr( "Pattern editor - %1" ).arg( sCurrentPatternName ) ) );
		m_pPatternNameLbl->setText( sCurrentPatternName );

		// update pattern size LCD
		updatePatternSizeLCD();
		updateEditors();
		
	}
	else {
		m_pPattern = nullptr;

		this->setWindowTitle( tr( "Pattern editor - No pattern selected" ) );
		m_pPatternNameLbl->setText( tr( "No pattern selected" ) );
	}

	resizeEvent( nullptr ); // force an update of the scrollbars
}

void PatternEditorPanel::hearNotesBtnClick()
{
	Preferences *pref = ( Preferences::get_instance() );
	pref->setHearNewNotes( m_pHearNotesBtn->isChecked() );

	if ( m_pHearNotesBtn->isChecked() ) {
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Hear new notes = On" ) );
	} else {
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Hear new notes = Off" ) );
	}
}

void PatternEditorPanel::quantizeEventsBtnClick()
{
	Preferences *pref = ( Preferences::get_instance() );
	pref->setQuantizeEvents( m_pQuantizeEventsBtn->isChecked() );

	if ( m_pQuantizeEventsBtn->isChecked() ) {
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Quantize incoming keyboard/midi events = On" ) );
	} else {
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Quantize incoming keyboard/midi events = Off" ) );
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
	if ( __show_drum_btn->isChecked() ) {
		m_pPianoRollEditor->selectInstrumentNotes( nInstrument );
	} else {
		m_pDrumPatternEditor->selectInstrumentNotes( nInstrument );
	}
}

void PatternEditorPanel::showDrumEditor()
{
	__show_drum_btn->setToolTip( tr( "Show piano roll editor" ) );
	__show_drum_btn->setChecked( false );
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
	__show_drum_btn->setChecked( true );
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

void PatternEditorPanel::showDrumEditorBtnClick()
{
	if ( __show_drum_btn->isVisible() ){
		showPianoRollEditor();
		__show_drum_btn->hide();
		__show_piano_btn->show();
		__show_drum_btn->setBaseToolTip( HydrogenApp::get_instance()->getCommonStrings()->getShowDrumkitEditorTooltip() );
		__show_piano_btn->setBaseToolTip( HydrogenApp::get_instance()->getCommonStrings()->getShowDrumkitEditorTooltip() );
	} else {
		showDrumEditor();
		__show_drum_btn->show();
		__show_piano_btn->hide();
		__show_drum_btn->setBaseToolTip( HydrogenApp::get_instance()->getCommonStrings()->getShowPianoRollEditorTooltip() );
		__show_piano_btn->setBaseToolTip( HydrogenApp::get_instance()->getCommonStrings()->getShowPianoRollEditorTooltip() );
	}
}


void PatternEditorPanel::zoomInBtnClicked()
{
	if( m_pPatternEditorRuler->getGridWidth() >= 24 ){
		return;
	}

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

void PatternEditorPanel::zoomOutBtnClicked()
{
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

void PatternEditorPanel::updatePatternInfo() {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	m_pPattern = nullptr;
	m_nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();

	if ( pSong != nullptr ) {
		PatternList *pPatternList = pSong->getPatternList();
		if ( ( m_nSelectedPatternNumber != -1 ) && ( m_nSelectedPatternNumber < pPatternList->size() ) ) {
			m_pPattern = pPatternList->get( m_nSelectedPatternNumber );
		}
	}
}

void PatternEditorPanel::updateEditors( bool bPatternOnly ) {

	updatePatternInfo();

	// Changes of pattern may leave the cursor out of bounds.
	setCursorPosition( getCursorPosition() );

	m_pPatternEditorRuler->updateEditor( true );
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePanEditor->updateEditor();
	m_pNoteLeadLagEditor->updateEditor();
	m_pNoteNoteKeyEditor->updateEditor();
	m_pNoteProbabilityEditor->updateEditor();
	m_pPianoRollEditor->updateEditor( bPatternOnly );
	m_pDrumPatternEditor->updateEditor();
}

void PatternEditorPanel::patternModifiedEvent() {
	selectedPatternChangedEvent();
}

void PatternEditorPanel::playingPatternsChangedEvent() {
	if ( PatternEditor::isUsingAdditionalPatterns( m_pPattern ) ) {
		updateEditors( true );
	}
}

void PatternEditorPanel::songModeActivationEvent() {
	updateEditors( true );
}

void PatternEditorPanel::stackedModeActivationEvent( int ) {
	updateEditors( true );
}

void PatternEditorPanel::songSizeChangedEvent() {
	if ( PatternEditor::isUsingAdditionalPatterns( m_pPattern ) ) {
		updateEditors( true );
	}
}

void PatternEditorPanel::patternEditorLockedEvent() {
	updateEditors( true );
}

void PatternEditorPanel::relocationEvent() {
	if ( H2Core::Hydrogen::get_instance()->isPatternEditorLocked() ) {
		updateEditors( true );
	}
}

void PatternEditorPanel::updatePatternSizeLCD() {
	if ( m_pPattern == nullptr ) {
		return;
	}

	m_bArmPatternSizeSpinBoxes = false;

	double fNewDenominator = static_cast<double>( m_pPattern->get_denominator() );
	if ( fNewDenominator != m_pLCDSpinBoxDenominator->value() &&
		 ! m_pLCDSpinBoxDenominator->hasFocus() ) {
		m_pLCDSpinBoxDenominator->setValue( fNewDenominator );

		// Update numerator to allow only for a maximum pattern length of
		// four measures.
		m_pLCDSpinBoxNumerator->setMaximum( 4 * m_pLCDSpinBoxDenominator->value() );
	}

	double fNewNumerator = static_cast<double>( m_pPattern->get_length() * m_pPattern->get_denominator() ) / static_cast<double>( MAX_NOTES );
	if ( fNewNumerator != m_pLCDSpinBoxNumerator->value() && ! m_pLCDSpinBoxNumerator->hasFocus() ) {
		m_pLCDSpinBoxNumerator->setValue( fNewNumerator );
	}
	
	m_bArmPatternSizeSpinBoxes = true;
}

void PatternEditorPanel::patternSizeChanged( double fValue ){
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	if ( ! m_bArmPatternSizeSpinBoxes ) {
		// Don't execute this function if the values of the spin boxes
		// have been set by Hydrogen instead of by the user.
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pInstrumentList = pHydrogen->getSong()->getInstrumentList();

	// Update numerator to allow only for a maximum pattern length of
	// four measures.
	m_pLCDSpinBoxNumerator->setMaximum( 4 * m_pLCDSpinBoxDenominator->value() );

	double fNewNumerator = m_pLCDSpinBoxNumerator->value();
	double fNewDenominator = m_pLCDSpinBoxDenominator->value();

	/* Note: user can input a non integer numerator and this feature
	   is very powerful because it allows to set really any possible
	   pattern size (in ticks) using ANY arbitrary denominator.
	   e.g. pattern size of 38 ticks will result from both inputs 1/5
	   (quintuplet) and 0.79/4 of a whole note, since both are rounded
	   and BOTH are UNSUPPORTED, but the first notation looks more
	   meaningful */

	int nNewLength =
		std::round( static_cast<double>( MAX_NOTES ) / fNewDenominator * fNewNumerator );

	if ( nNewLength == m_pPattern->get_length() ) {
		return;
	}

	QUndoStack* pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;
	pUndoStack->beginMacro( QString( "Change pattern size to %1/%2" )
							.arg( fNewNumerator ).arg( fNewDenominator ) );

	pUndoStack->push( new SE_patternSizeChangedAction( nNewLength,
													   m_pPattern->get_length(),
													   fNewDenominator,
													   m_pPattern->get_denominator(),
													   m_nSelectedPatternNumber ) );
	pUndoStack->endMacro();
}

void PatternEditorPanel::patternSizeChangedAction( int nLength, double fDenominator,
												   int nSelectedPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pPatternList = pHydrogen->getSong()->getPatternList();
	H2Core::Pattern* pPattern = nullptr;

	if ( ( nSelectedPatternNumber != -1 ) &&
		 ( nSelectedPatternNumber < pPatternList->size() ) ) {
		pPattern = pPatternList->get( nSelectedPatternNumber );
	}

	if ( pPattern == nullptr ) {
		ERRORLOG( QString( "Pattern corresponding to pattern number [%1] could not be retrieved" )
				  .arg( nSelectedPatternNumber ) );
		return;
	}

	pAudioEngine->lock( RIGHT_HERE );
	// set length and denominator				
	pPattern->set_length( nLength );
	pPattern->set_denominator( static_cast<int>( fDenominator ) );
	pHydrogen->updateSongSize();
	pAudioEngine->unlock();
	
	pHydrogen->setIsModified( true );
	
	EventQueue::get_instance()->push_event( EVENT_PATTERN_MODIFIED, -1 );
}

void PatternEditorPanel::dragEnterEvent( QDragEnterEvent *event )
{
	m_pInstrumentList->dragEnterEvent( event );
}



void PatternEditorPanel::dropEvent( QDropEvent *event )
{
	m_pInstrumentList->dropEvent( event );
}

void PatternEditorPanel::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		// Performs an editor update with updateEditor() (and no argument).
		selectedPatternChangedEvent();
		selectedInstrumentChangedEvent();
		updateSLnameLabel();
		updateEditors( true );
		m_pPatternEditorRuler->updatePosition();
	}
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
	} else if ( m_pPattern != nullptr && nCursorPosition >= m_pPattern->get_length() ) {
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
	if ( m_pPattern == nullptr ) {
		return 0;
	}
	
	m_nCursorPosition = std::min( m_nCursorPosition + m_nCursorIncrement * n,
								  m_pPattern->get_length() - m_nCursorIncrement );

	ensureCursorVisible();

	return m_nCursorPosition;
}

void PatternEditorPanel::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		// It's sufficient to check the properties of just one label
		// because they will always carry the same.
		QFont boldFont( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
		boldFont.setBold( true );
		m_pSLlabel->setFont( boldFont );
		m_pPatternNameLbl->setFont( boldFont );

		updateStyleSheet();
	}

	if ( changes & ( H2Core::Preferences::Changes::Colors ) ) {
		updateStyleSheet();
	}
}

void PatternEditorPanel::updateStyleSheet() {

	auto pPref = H2Core::Preferences::get_instance();
	int nFactorTop = 112;
	
	QColor topColorLight = pPref->getColorTheme()->m_midColor.lighter( nFactorTop );
	QColor topColorDark = pPref->getColorTheme()->m_midColor.darker( nFactorTop );

	QString sEditorTopStyleSheet = QString( "\
QWidget#editor1 {\
     background-color: qlineargradient(x1: 0.5, y1: 0.1, x2: 0.5, y2: 0.9, \
                                      stop: 0 %1, stop: 1 %2); \
} \
QWidget#editor2 {\
     background-color: qlineargradient(x1: 0.5, y1: 0.1, x2: 0.5, y2: 0.9, \
                                      stop: 0 %1, stop: 1 %2); \
}")
		.arg( topColorLight.name() ).arg( topColorDark.name() );
	QString sWidgetTopStyleSheet = QString( "\
QWidget#sizeResol {\
    background-color: %1;\
} \
QWidget#pRec {\
    background-color: %1;\
}" )
		.arg( pPref->getColorTheme()->m_midLightColor.name() );

	m_pEditorTop1->setStyleSheet( sEditorTopStyleSheet );
	m_pEditorTop2->setStyleSheet( sEditorTopStyleSheet );
		
	m_pSizeResol->setStyleSheet( sWidgetTopStyleSheet );
	m_pRec->setStyleSheet( sWidgetTopStyleSheet );
									
}

void PatternEditorPanel::switchPatternSizeFocus() {
	if ( ! m_pLCDSpinBoxDenominator->hasFocus() ) {
		m_pLCDSpinBoxDenominator->setFocus();
	} else {
		m_pLCDSpinBoxNumerator->setFocus();
	}
}

NotePropertiesRuler::Mode PatternEditorPanel::getNotePropertiesMode() const
{
	NotePropertiesRuler::Mode mode;

	switch ( m_pPropertiesCombo->currentIndex() ) {
	case 0:
		mode = NotePropertiesRuler::Mode::Velocity;
		break;
	case 1:
		mode = NotePropertiesRuler::Mode::Pan;
		break;
	case 2:
		mode = NotePropertiesRuler::Mode::LeadLag;
		break;
	case 3:
		mode = NotePropertiesRuler::Mode::NoteKey;
		break;
	case 4:
		mode = NotePropertiesRuler::Mode::Probability;
		break;
	default:
		ERRORLOG( QString( "Unsupported m_pPropertiesCombo index [%1]" )
				  .arg( m_pPropertiesCombo->currentIndex() ) );
	}

	return mode;
}
