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

#include <cmath>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>

#include "DrumPatternEditor.h"
#include "NotePropertiesRuler.h"
#include "PatternEditorInstrumentList.h"
#include "PatternEditorPanel.h"
#include "PatternEditorRuler.h"
#include "PianoRollEditor.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "../SongEditor/SongEditorPanel.h"
#include "../Widgets/Button.h"
#include "../Widgets/ClickableLabel.h"
#include "../Widgets/LCDCombo.h"
#include "../Widgets/LCDSpinBox.h"
#include "../Widgets/PatchBay.h"
#include "../Widgets/PixmapWidget.h"
#include "../WidgetScrollArea.h"
#include "../UndoActions.h"

using namespace H2Core;


PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
	: QWidget( pParent )
	, m_pPattern( nullptr )
	, m_bArmPatternSizeSpinBoxes( true )
{
	setAcceptDrops(true);

	const auto pPref = Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pHydrogen = Hydrogen::get_instance();
	m_nSelectedRowDB = pHydrogen->getSelectedInstrumentNumber();
	const auto pSong = pHydrogen->getSong();
	if ( pSong != nullptr ) {
		m_nPatternNumber = pHydrogen->getSelectedPatternNumber();
		const auto pPatternList = pSong->getPatternList();
		if ( m_nPatternNumber != -1 &&
			 m_nPatternNumber < pPatternList->size() ) {
			m_pPattern = pPatternList->get( m_nPatternNumber );
		}
		else {
			m_pPattern = nullptr;
		}
	}
	else {
		m_pPattern = nullptr;
	}

	updateDB();

	QFont boldFont( pPref->getTheme().m_font.m_sApplicationFontFamily,
					getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	boldFont.setBold( true );

	m_nCursorColumn = 0;

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
	m_pDrumkitLabel = new ClickableLabel( nullptr, QSize( 0, 0 ), "",
										  ClickableLabel::Color::Bright, true );
	m_pDrumkitLabel->setFont( boldFont );
	m_pDrumkitLabel->setFixedSize( 170, 20 );
	m_pDrumkitLabel->move( 10, 3 );
	m_pDrumkitLabel->setToolTip( tr( "Drumkit used in the current song" ) );
	m_pEditorTop1_hbox->addWidget( m_pDrumkitLabel );
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		m_pDrumkitLabel->setText( pSong->getDrumkit()->getName() );
	}
	connect( m_pDrumkitLabel, &ClickableLabel::labelClicked,
			 [=]() { HydrogenApp::get_instance()->getMainForm()->
					 action_drumkit_properties(); } );

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
	m_pPatternSizeLbl = new ClickableLabel(
		m_pSizeResol, QSize( 0, 0 ), pCommonStrings->getPatternSizeLabel(),
		ClickableLabel::Color::Dark );
	m_pPatternSizeLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pPatternSizeLbl );
	
	m_pLCDSpinBoxNumerator = new LCDSpinBox(
		this, QSize( 62, 20 ), LCDSpinBox::Type::Double, 0.1, 16.0, true );
	m_pLCDSpinBoxNumerator->setKind( LCDSpinBox::Kind::PatternSizeNumerator );
	connect( m_pLCDSpinBoxNumerator, &LCDSpinBox::slashKeyPressed,
			 this, &PatternEditorPanel::switchPatternSizeFocus );
	connect( m_pLCDSpinBoxNumerator, SIGNAL( valueChanged( double ) ),
			 this, SLOT( patternSizeChanged( double ) ) );
	m_pLCDSpinBoxNumerator->setKeyboardTracking( false );
	m_pLCDSpinBoxNumerator->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pLCDSpinBoxNumerator );
			
	auto pLabel1 = new ClickableLabel(
		m_pSizeResol, QSize( 4, 13 ), "/", ClickableLabel::Color::Dark );
	pLabel1->resize( QSize( 20, 17 ) );
	pLabel1->setText( "/" );
	pLabel1->setFont( boldFont );
	pLabel1->setToolTip( tr( "You can use the '/' inside the pattern size spin boxes to switch back and forth." ) );
	pLabel1->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( pLabel1 );
	
	m_pLCDSpinBoxDenominator = new LCDSpinBox(
		m_pSizeResol, QSize( 48, 20 ), LCDSpinBox::Type::Int, 1, 192, true );
	m_pLCDSpinBoxDenominator->setKind( LCDSpinBox::Kind::PatternSizeDenominator );
	connect( m_pLCDSpinBoxDenominator, &LCDSpinBox::slashKeyPressed,
			 this, &PatternEditorPanel::switchPatternSizeFocus );
	connect( m_pLCDSpinBoxDenominator, SIGNAL( valueChanged( double ) ),
			 this, SLOT( patternSizeChanged( double ) ) );
	m_pLCDSpinBoxDenominator->setKeyboardTracking( false );
	m_pLCDSpinBoxDenominator->setSizePolicy(
		QSizePolicy::Fixed, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pLCDSpinBoxDenominator );
	pSizeResolLayout->addSpacing( nLabelSpacing );
	updatePatternSizeLCD();
	
	// GRID resolution
	m_pResolutionLbl = new ClickableLabel(
		m_pSizeResol, QSize( 0, 0 ), pCommonStrings->getResolutionLabel(),
		ClickableLabel::Color::Dark );
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

	int nIndex;
	const int nResolution = pPref->getPatternEditorGridResolution();
	const bool bUseTriplets = pPref->isPatternEditorUsingTriplets();

	if ( nResolution == MAX_NOTES ) {
		nIndex = 11;
	} else if ( ! bUseTriplets ) {
		switch ( nResolution ) {
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
		switch ( nResolution ) {
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
	connect( m_pResolutionCombo, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( gridResolutionChanged( int ) ) );
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
	m_pHearNotesLbl = new ClickableLabel(
		m_pRec, QSize( 0, 0 ), pCommonStrings->getHearNotesLabel(),
		ClickableLabel::Color::Dark );
	m_pHearNotesLbl->setAlignment( Qt::AlignRight );
	m_pHearNotesLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pHearNotesLbl );
	
	m_pHearNotesBtn = new Button(
		m_pRec, QSize( 21, 18 ), Button::Type::Toggle, "speaker.svg", "", false,
		QSize( 15, 13 ), tr( "Hear new notes" ), false, true );
	connect( m_pHearNotesBtn, SIGNAL( clicked() ),
			 this, SLOT( hearNotesBtnClick() ) );
	m_pHearNotesBtn->setChecked( pPref->getHearNewNotes() );
	m_pHearNotesBtn->setObjectName( "HearNotesBtn" );
	m_pHearNotesBtn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pHearNotesBtn );
	pRecLayout->addSpacing( nLabelSpacing );

	// quantize
	m_pQuantizeEventsLbl = new ClickableLabel(
		m_pRec, QSize( 0, 0 ), pCommonStrings->getQuantizeEventsLabel(),
		ClickableLabel::Color::Dark );
	m_pQuantizeEventsLbl->setAlignment( Qt::AlignRight );
	m_pQuantizeEventsLbl->setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pQuantizeEventsLbl );
	
	m_pQuantizeEventsBtn = new Button(
		m_pRec, QSize( 21, 18 ), Button::Type::Toggle, "quantization.svg", "",
		false, QSize( 15, 14 ), tr( "Quantize keyboard/midi events to grid" ),
		false, true );
	m_pQuantizeEventsBtn->setChecked( pPref->getQuantizeEvents() );
	m_pQuantizeEventsBtn->setObjectName( "QuantizeEventsBtn" );
	connect( m_pQuantizeEventsBtn, SIGNAL( clicked() ),
			 this, SLOT( quantizeEventsBtnClick() ) );
	m_pQuantizeEventsBtn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pQuantizeEventsBtn );
	pRecLayout->addSpacing( nLabelSpacing );

	// Editor mode
	m_pShowPianoLbl = new ClickableLabel(
		m_pRec, QSize( 0, 0 ), pCommonStrings->getShowPianoLabel(),
		ClickableLabel::Color::Dark );
	m_pShowPianoLbl->setAlignment( Qt::AlignRight );
	m_pShowPianoLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pShowPianoLbl );

	__show_drum_btn = new Button(
		m_pRec, QSize( 25, 18 ), Button::Type::Push, "drum.svg", "", false,
		QSize( 17, 13 ), pCommonStrings->getShowPianoRollEditorTooltip() );
	__show_drum_btn->setObjectName( "ShowDrumBtn" );
	connect( __show_drum_btn, SIGNAL( clicked() ),
			 this, SLOT( showDrumEditorBtnClick() ) );
	__show_drum_btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( __show_drum_btn );

	m_pEditorTop1_hbox_2->addStretch();
	
	// Since the button to activate the piano roll is shown
	// initially, both buttons get the same tooltip. Actually only the
	// last one does need a tooltip since it will be shown regardless
	// of whether it is hidden or not. But since this behavior might
	// change in future versions of Qt the tooltip will be assigned to
	// both of them.
	__show_piano_btn = new Button(
		m_pRec, QSize( 25, 18 ), Button::Type::Push, "piano.svg", "", false,
		QSize( 19, 15 ), pCommonStrings->getShowPianoRollEditorTooltip() );
	__show_piano_btn->move( 178, 1 );
	__show_piano_btn->setObjectName( "ShowPianoBtn" );
	__show_piano_btn->hide();
	connect( __show_piano_btn, SIGNAL( clicked() ),
			 this, SLOT( showDrumEditorBtnClick() ) );
	__show_piano_btn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( __show_piano_btn );

	m_pPatchBayBtn = new Button(
		m_pRec, QSize( 25, 18 ), Button::Type::Push, "patchBay.svg", "", false,
		QSize( 19, 15 ), tr( "Show PatchBay" ) );
	m_pPatchBayBtn->move( 209, 1 );
	m_pPatchBayBtn->hide();
	m_pPatchBayBtn->setObjectName( "ShowPatchBayBtn" );
	connect( m_pPatchBayBtn, SIGNAL( clicked() ),
			 this, SLOT( patchBayBtnClicked() ) );
	m_pPatchBayBtn->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pRecLayout->addWidget( m_pPatchBayBtn );

	// zoom-in btn
	Button *zoom_in_btn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Push, "plus.svg", "", false,
		QSize( 9, 9 ), tr( "Zoom in" ) );
	connect( zoom_in_btn, SIGNAL( clicked() ), this, SLOT( zoomInBtnClicked() ) );


	// zoom-out btn
	Button *zoom_out_btn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Push, "minus.svg", "", false,
		QSize( 9, 9 ), tr( "Zoom out" ) );
	connect( zoom_out_btn, SIGNAL( clicked() ), this, SLOT( zoomOutBtnClicked() ) );
// End Editor TOP


	// external horizontal scrollbar
	m_pPatternEditorHScrollBar = new QScrollBar( Qt::Horizontal , nullptr  );
	m_pPatternEditorHScrollBar->setObjectName( "PatternEditorHScrollBar" );
	connect( m_pPatternEditorHScrollBar, SIGNAL( valueChanged( int ) ),
			 this, SLOT( syncToExternalHorizontalScrollbar( int ) ) );

	// external vertical scrollbar
	m_pPatternEditorVScrollBar = new QScrollBar( Qt::Vertical, nullptr );
	m_pPatternEditorVScrollBar->setObjectName( "PatternEditorVScrollBar" );
	connect( m_pPatternEditorVScrollBar, SIGNAL(valueChanged( int)),
			 this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	QHBoxLayout *pPatternEditorHScrollBarLayout = new QHBoxLayout();
	pPatternEditorHScrollBarLayout->setSpacing( 0 );
	pPatternEditorHScrollBarLayout->setMargin( 0 );
	pPatternEditorHScrollBarLayout->addWidget( m_pPatternEditorHScrollBar );
	pPatternEditorHScrollBarLayout->addWidget( zoom_in_btn );
	pPatternEditorHScrollBarLayout->addWidget( zoom_out_btn );

	m_pPatternEditorHScrollBarContainer = new QWidget();
	m_pPatternEditorHScrollBarContainer->setLayout( pPatternEditorHScrollBarLayout );


	QPalette label_palette;
	label_palette.setColor( QPalette::WindowText, QColor( 230, 230, 230 ) );

	m_pPatternNameLbl = new ClickableLabel( nullptr, QSize( 0, 0 ), "",
											ClickableLabel::Color::Bright, true );
	m_pPatternNameLbl->setFont( boldFont );
	m_pPatternNameLbl->setPalette( label_palette );
	connect( m_pPatternNameLbl, &ClickableLabel::labelClicked,
			 [=]() { HydrogenApp::get_instance()->getSongEditorPanel()->
					 getSongEditorPatternList()->patternPopup_properties(); } );
	updatePatternName();

	// restore grid resolution
	m_nCursorIncrement = ( bUseTriplets ? 4 : 3 ) * MAX_NOTES / ( nResolution * 3 );

	HydrogenApp::get_instance()->addEventListener( this );

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &PatternEditorPanel::onPreferencesChanged );

	updateStyleSheet();
}

PatternEditorPanel::~PatternEditorPanel()
{
}

void PatternEditorPanel::createEditors() {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

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
	connect( m_pRulerScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pPatternEditorRuler, &PatternEditorRuler::onPreferencesChanged );

	// Drum Pattern
	m_pEditorScrollView = new WidgetScrollArea( nullptr );
	m_pEditorScrollView->setObjectName( "EditorScrollView" );
	m_pEditorScrollView->setFocusPolicy( Qt::NoFocus );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pEditorScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	m_pDrumPatternEditor = new DrumPatternEditor(
		m_pEditorScrollView->viewport() );

	m_pEditorScrollView->setWidget( m_pDrumPatternEditor );
	m_pEditorScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pEditorScrollView->setFocusProxy( m_pDrumPatternEditor );

	m_pPatternEditorRuler->setFocusProxy( m_pEditorScrollView );

	connect( m_pPatternEditorVScrollBar, SIGNAL( valueChanged( int ) ),
			 m_pDrumPatternEditor, SLOT( scrolled( int ) ) );
	connect( m_pPatternEditorHScrollBar, SIGNAL( valueChanged( int ) ),
			 m_pDrumPatternEditor, SLOT( scrolled( int ) ) );
	connect( m_pEditorScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorVScroll(int) ) );
	connect( m_pEditorScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pDrumPatternEditor, &DrumPatternEditor::onPreferencesChanged );

	// PianoRollEditor
	m_pPianoRollScrollView = new WidgetScrollArea( nullptr );
	m_pPianoRollScrollView->setObjectName( "PianoRollScrollView" );
	m_pPianoRollScrollView->setFocusPolicy( Qt::NoFocus );
	m_pPianoRollScrollView->setFrameShape( QFrame::NoFrame );
	m_pPianoRollScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pPianoRollScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPianoRollEditor = new PianoRollEditor(
		m_pPianoRollScrollView->viewport(), m_pPianoRollScrollView );
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

	// Instrument list
	m_pInstrListScrollView = new WidgetScrollArea( nullptr );
	m_pInstrListScrollView->setObjectName( "InstrListScrollView" );
	m_pInstrListScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pInstrListScrollView->setFrameShape( QFrame::NoFrame );
	m_pInstrListScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pInstrListScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	m_pInstrumentList = new PatternEditorInstrumentList(
		m_pInstrListScrollView->viewport() );
	m_pInstrListScrollView->setWidget( m_pInstrumentList );
	m_pInstrListScrollView->setFixedWidth( m_pInstrumentList->width() );
	m_pInstrumentList->setFocusPolicy( Qt::ClickFocus );
	m_pInstrumentList->setFocusProxy( m_pEditorScrollView );

	connect( m_pInstrListScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorVScroll(int) ) );
	m_pInstrListScrollView->setFocusProxy( m_pInstrumentList );

	// NOTE_VELOCITY EDITOR
	m_pNoteVelocityScrollView = new WidgetScrollArea( nullptr );
	m_pNoteVelocityScrollView->setObjectName( "NoteVelocityScrollView" );
	m_pNoteVelocityScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteVelocityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteVelocityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityEditor = new NotePropertiesRuler(
		m_pNoteVelocityScrollView->viewport(),
		NotePropertiesRuler::Mode::Velocity );
	m_pNoteVelocityScrollView->setWidget( m_pNoteVelocityEditor );
	m_pNoteVelocityScrollView->setFixedHeight( 100 );
	connect( m_pNoteVelocityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNoteVelocityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNoteVelocityEditor, SLOT( scrolled( int ) ) );

	m_pNoteVelocityEditor->mergeSelectionGroups( m_pDrumPatternEditor );

	// NOTE_PAN EDITOR
	m_pNotePanScrollView = new WidgetScrollArea( nullptr );
	m_pNotePanScrollView->setObjectName( "NotePanScrollView" );
	m_pNotePanScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNotePanScrollView->setFrameShape( QFrame::NoFrame );
	m_pNotePanScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNotePanEditor = new NotePropertiesRuler(
		m_pNotePanScrollView->viewport(), NotePropertiesRuler::Mode::Pan );
	m_pNotePanScrollView->setWidget( m_pNotePanEditor );
	m_pNotePanScrollView->setFixedHeight( 100 );

	connect( m_pNotePanScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNotePanScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNotePanEditor, SLOT( scrolled( int ) ) );

	m_pNotePanEditor->mergeSelectionGroups( m_pDrumPatternEditor );

	// NOTE_LEADLAG EDITOR
	m_pNoteLeadLagScrollView = new WidgetScrollArea( nullptr );
	m_pNoteLeadLagScrollView->setObjectName( "NoteLeadLagScrollView" );
	m_pNoteLeadLagScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteLeadLagScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteLeadLagScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteLeadLagEditor = new NotePropertiesRuler(
		m_pNoteLeadLagScrollView->viewport(),
		NotePropertiesRuler::Mode::LeadLag );
	m_pNoteLeadLagScrollView->setWidget( m_pNoteLeadLagEditor );
	m_pNoteLeadLagScrollView->setFixedHeight( 100 );

	connect( m_pNoteLeadLagScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNoteLeadLagScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNoteLeadLagEditor, SLOT( scrolled( int ) ) );

	m_pNoteLeadLagEditor->mergeSelectionGroups( m_pDrumPatternEditor );

	// NOTE_NOTEKEY EDITOR
	m_pNoteKeyOctaveScrollView = new WidgetScrollArea( nullptr );
	m_pNoteKeyOctaveScrollView->setObjectName( "NoteNoteKeyScrollView" );
	m_pNoteKeyOctaveScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteKeyOctaveScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteKeyOctaveScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteKeyOctaveScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteKeyOctaveEditor = new NotePropertiesRuler(
		m_pNoteKeyOctaveScrollView->viewport(),
		NotePropertiesRuler::Mode::KeyOctave );
	m_pNoteKeyOctaveScrollView->setWidget( m_pNoteKeyOctaveEditor );
	m_pNoteKeyOctaveScrollView->setFixedHeight( 210 );
	connect( m_pNoteKeyOctaveScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
			 this, SLOT( on_patternEditorHScroll( int ) ) );
	connect( m_pNoteKeyOctaveScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
			 m_pNoteKeyOctaveEditor, SLOT( scrolled( int ) ) );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 m_pNoteKeyOctaveEditor, &NotePropertiesRuler::onPreferencesChanged );

	m_pNoteKeyOctaveEditor->mergeSelectionGroups( m_pDrumPatternEditor );

	// NOTE_PROBABILITY EDITOR
	m_pNoteProbabilityScrollView = new WidgetScrollArea( nullptr );
	m_pNoteProbabilityScrollView->setObjectName( "NoteProbabilityScrollView" );
	m_pNoteProbabilityScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteProbabilityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteProbabilityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteProbabilityEditor = new NotePropertiesRuler(
		m_pNoteProbabilityScrollView->viewport(),
		NotePropertiesRuler::Mode::Probability );
	m_pNoteProbabilityScrollView->setWidget( m_pNoteProbabilityEditor );
	m_pNoteProbabilityScrollView->setFixedHeight( 100 );
	connect( m_pNoteProbabilityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pNoteProbabilityScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pNoteProbabilityEditor, SLOT( scrolled( int ) ) );

	m_pNoteProbabilityEditor->mergeSelectionGroups( m_pDrumPatternEditor );

	m_pPropertiesPanel = new PixmapWidget( nullptr );
	m_pPropertiesPanel->setObjectName( "PropertiesPanel" );
	m_pPropertiesPanel->setColor( QColor( 58, 62, 72 ) );

	m_pPropertiesPanel->setFixedSize( 181, 100 );

	QVBoxLayout *pPropertiesVBox = new QVBoxLayout( m_pPropertiesPanel );
	pPropertiesVBox->setSpacing( 0 );
	pPropertiesVBox->setMargin( 0 );

	m_pPropertiesCombo =
		new LCDCombo( nullptr, QSize( m_pInstrumentList->width(), 18 ), false );
	m_pPropertiesCombo->setToolTip( tr( "Select note properties" ) );
	m_pPropertiesCombo->addItem( pCommonStrings->getNotePropertyVelocity() );
	m_pPropertiesCombo->addItem( pCommonStrings->getNotePropertyPan() );
	m_pPropertiesCombo->addItem( pCommonStrings->getNotePropertyLeadLag() );
	m_pPropertiesCombo->addItem( pCommonStrings->getNotePropertyKeyOctave() );
	m_pPropertiesCombo->addItem( pCommonStrings->getNotePropertyProbability() );
	// is triggered here below
	m_pPropertiesCombo->setObjectName( "PropertiesCombo" );
	connect( m_pPropertiesCombo, SIGNAL( currentIndexChanged( int ) ),
			 this, SLOT( propertiesComboChanged( int ) ) );
	m_pPropertiesCombo->setCurrentIndex( 0 );
	propertiesComboChanged( 0 );

	pPropertiesVBox->addWidget( m_pPropertiesCombo );

	// Layout
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
	pGrid->addWidget( m_pPatternEditorHScrollBarContainer, 10, 1 );
	pGrid->addWidget( m_pNoteVelocityScrollView, 4, 1 );
	pGrid->addWidget( m_pNotePanScrollView, 4, 1 );
	pGrid->addWidget( m_pNoteLeadLagScrollView, 4, 1 );
	pGrid->addWidget( m_pNoteKeyOctaveScrollView, 4, 1 );
	pGrid->addWidget( m_pNoteProbabilityScrollView, 4, 1 );

	pGrid->addWidget( m_pPropertiesPanel, 4, 0 );
	pGrid->setRowStretch( 2, 100 );
	pMainPanel->setLayout( pGrid );

	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );
	this->setLayout( pVBox );

	pVBox->addWidget( pMainPanel );
}

void PatternEditorPanel::updateDrumkitLabel( )
{
	const auto pTheme = H2Core::Preferences::get_instance()->getTheme();

	QFont font( pTheme.m_font.m_sApplicationFontFamily,
				getPointSize( pTheme.m_font.m_fontSize ) );
	font.setBold( true );
	m_pDrumkitLabel->setFont( font );

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		m_pDrumkitLabel->setText( pSong->getDrumkit()->getName() );
	}
}

void PatternEditorPanel::drumkitLoadedEvent() {
	updateDrumkitLabel();
	updateDB();
	updateEditors();
	m_pInstrumentList->updateInstrumentLines();
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
	m_pNoteKeyOctaveScrollView->horizontalScrollBar()->setValue( m_pPatternEditorHScrollBar->value() );

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
	m_pNoteKeyOctaveEditor->setResolution( nResolution, bUseTriplets );
	m_pNoteProbabilityEditor->setResolution( nResolution, bUseTriplets );
	m_pNotePanEditor->setResolution( nResolution, bUseTriplets );

	m_nCursorIncrement = ( bUseTriplets ? 4 : 3 ) * MAX_NOTES / ( nResolution * 3 );
	m_nCursorColumn = m_nCursorIncrement * ( m_nCursorColumn / m_nCursorIncrement );

	auto pPref = Preferences::get_instance();
	pPref->setPatternEditorGridResolution( nResolution );
	pPref->setPatternEditorUsingTriplets( bUseTriplets );
}



void PatternEditorPanel::selectedPatternChangedEvent()
{
	updatePatternInfo();
	updateDB();
	updateEditors();

	resizeEvent( nullptr ); // force an update of the scrollbars
}

void PatternEditorPanel::hearNotesBtnClick()
{
	Preferences::get_instance()->setHearNewNotes( m_pHearNotesBtn->isChecked() );

	if ( m_pHearNotesBtn->isChecked() ) {
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Hear new notes = On" ) );
	} else {
		( HydrogenApp::get_instance() )->showStatusBarMessage( tr( "Hear new notes = Off" ) );
	}
}

void PatternEditorPanel::quantizeEventsBtnClick()
{
	Preferences::get_instance()->setQuantizeEvents(
		m_pQuantizeEventsBtn->isChecked() );

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
	syncScrollBarSize( m_pNoteKeyOctaveScrollView->horizontalScrollBar(), pScrollArea->horizontalScrollBar() );
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
	m_nSelectedRowDB = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	updateEditors();
	resizeEvent( nullptr );	// force a scrollbar update
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

	m_pDrumPatternEditor->updateEditor(); // force an update

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

PatternEditor* PatternEditorPanel::getVisibleEditor() const {
	if ( m_pEditorScrollView->isVisible() ) {
		return m_pDrumPatternEditor;
	}
	return m_pPianoRollEditor;
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
	m_pNoteKeyOctaveEditor->zoomIn();
	m_pNoteProbabilityEditor->zoomIn();
	m_pNotePanEditor->zoomIn();
	m_pPianoRollEditor->zoomIn();

	auto pPref = Preferences::get_instance();
	pPref->setPatternEditorGridWidth( m_pPatternEditorRuler->getGridWidth() );
	pPref->setPatternEditorGridHeight( m_pDrumPatternEditor->getGridHeight() );

	resizeEvent( nullptr );
}

void PatternEditorPanel::zoomOutBtnClicked()
{
	m_pPatternEditorRuler->zoomOut();
	m_pDrumPatternEditor->zoomOut();
	m_pNoteVelocityEditor->zoomOut();
	m_pNoteLeadLagEditor->zoomOut();
	m_pNoteKeyOctaveEditor->zoomOut();
	m_pNoteProbabilityEditor->zoomOut();
	m_pNotePanEditor->zoomOut();
	m_pPianoRollEditor->zoomOut();

	resizeEvent( nullptr );

	auto pPref = Preferences::get_instance();
	pPref->setPatternEditorGridWidth( m_pPatternEditorRuler->getGridWidth() );
	pPref->setPatternEditorGridHeight( m_pDrumPatternEditor->getGridHeight() );
}

void PatternEditorPanel::updatePatternInfo() {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();

	m_pPattern = nullptr;
	if ( pSong != nullptr ) {
		m_nPatternNumber = pHydrogen->getSelectedPatternNumber();
		const auto pPatternList = pSong->getPatternList();
		if ( m_nPatternNumber != -1 &&
			 m_nPatternNumber < pPatternList->size() ) {
			m_pPattern = pPatternList->get( m_nPatternNumber );
		}
	}

	updatePatternName();
	updatePatternSizeLCD();
}

void PatternEditorPanel::updatePatternName() {
	if ( m_pPattern != nullptr ) {
		// update pattern name text
		QString sCurrentPatternName = m_pPattern->getName();
		this->setWindowTitle( ( tr( "Pattern editor - %1" ).arg( sCurrentPatternName ) ) );
		m_pPatternNameLbl->setText( sCurrentPatternName );

	}
	else {
		this->setWindowTitle( tr( "Pattern editor - No pattern selected" ) );
		m_pPatternNameLbl->setText( tr( "No pattern selected" ) );
	}
}

void PatternEditorPanel::updateEditors( bool bPatternOnly ) {

	// Changes of pattern may leave the cursor out of bounds.
	setCursorColumn( getCursorColumn() );

	m_pPatternEditorRuler->updateEditor( true );
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePanEditor->updateEditor();
	m_pNoteLeadLagEditor->updateEditor();
	m_pNoteKeyOctaveEditor->updateEditor();
	m_pNoteProbabilityEditor->updateEditor();
	m_pPianoRollEditor->updateEditor( bPatternOnly );
	m_pDrumPatternEditor->updateEditor();
}

void PatternEditorPanel::patternModifiedEvent() {
	updatePatternInfo();
	updateEditors();
	resizeEvent( nullptr );
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

	double fNewDenominator = static_cast<double>( m_pPattern->getDenominator() );
	if ( fNewDenominator != m_pLCDSpinBoxDenominator->value() &&
		 ! m_pLCDSpinBoxDenominator->hasFocus() ) {
		m_pLCDSpinBoxDenominator->setValue( fNewDenominator );

		// Update numerator to allow only for a maximum pattern length of
		// four measures.
		m_pLCDSpinBoxNumerator->setMaximum( 4 * m_pLCDSpinBoxDenominator->value() );
	}

	double fNewNumerator = static_cast<double>( m_pPattern->getLength() * m_pPattern->getDenominator() ) / static_cast<double>( MAX_NOTES );
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
	auto pInstrumentList = pHydrogen->getSong()->getDrumkit()->getInstruments();

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

	if ( nNewLength == m_pPattern->getLength() ) {
		return;
	}

	QUndoStack* pUndoStack = HydrogenApp::get_instance()->m_pUndoStack;
	pUndoStack->beginMacro( tr( "Change pattern size to %1/%2" )
							.arg( fNewNumerator ).arg( fNewDenominator ) );

	pUndoStack->push( new SE_patternSizeChangedAction(
						  nNewLength,
						  m_pPattern->getLength(),
						  fNewDenominator,
						  m_pPattern->getDenominator(),
						  m_nPatternNumber ) );
	pUndoStack->endMacro();
}

void PatternEditorPanel::patternSizeChangedAction( int nLength, double fDenominator,
												   int nSelectedPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pPatternList = pHydrogen->getSong()->getPatternList();
	std::shared_ptr<H2Core::Pattern> pPattern = nullptr;

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
	pPattern->setLength( nLength );
	pPattern->setDenominator( static_cast<int>( fDenominator ) );
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
		updateDrumkitLabel();
		updatePatternInfo();
		updateDB();
		updateEditors( true );
		m_pPatternEditorRuler->updatePosition();
		m_pInstrumentList->updateInstrumentLines();
		resizeEvent( nullptr );
	}
}

void PatternEditorPanel::propertiesComboChanged( int nSelected )
{
	if ( nSelected == 0 ) {				// Velocity
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteKeyOctaveScrollView->hide();
		m_pNoteVelocityScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNoteVelocityEditor->updateEditor();
	}
	else if ( nSelected == 1 ) {		// Pan
		m_pNoteVelocityScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteKeyOctaveScrollView->hide();
		m_pNotePanScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNotePanEditor->updateEditor();
	}
	else if ( nSelected == 2 ) {		// Lead and Lag
		m_pNoteVelocityScrollView->hide();
		m_pNotePanScrollView->hide();
		m_pNoteKeyOctaveScrollView->hide();
		m_pNoteLeadLagScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNoteLeadLagEditor->updateEditor();
	}
	else if ( nSelected == 3 ) {		// KeyOctave
		m_pNoteVelocityScrollView->hide();
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteKeyOctaveScrollView->show();
		m_pNoteProbabilityScrollView->hide();

		m_pNoteKeyOctaveEditor->updateEditor();
	}
	else if ( nSelected == 4 ) {		// Probability
		m_pNotePanScrollView->hide();
		m_pNoteLeadLagScrollView->hide();
		m_pNoteKeyOctaveScrollView->hide();
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

int PatternEditorPanel::getCursorColumn()
{
	return m_nCursorColumn;
}

void PatternEditorPanel::ensureCursorVisible()
{
	uint y = m_nSelectedRowDB *
		Preferences::get_instance()->getPatternEditorGridHeight();
	m_pEditorScrollView->ensureVisible(
		m_nCursorColumn * m_pPatternEditorRuler->getGridWidth(), y );
}

void PatternEditorPanel::setCursorColumn(int nCursorPosition)
{
	if ( nCursorPosition < 0 ) {
		m_nCursorColumn = 0;
	} else if ( m_pPattern != nullptr && nCursorPosition >= m_pPattern->getLength() ) {
		m_nCursorColumn = m_pPattern->getLength() - m_nCursorIncrement;
	} else {
		m_nCursorColumn = nCursorPosition;
	}
}

int PatternEditorPanel::moveCursorLeft( int n )
{
	m_nCursorColumn = std::max( m_nCursorColumn - m_nCursorIncrement * n,
								  0 );

	ensureCursorVisible();

	return m_nCursorColumn;
}

int PatternEditorPanel::moveCursorRight( int n )
{
	if ( m_pPattern == nullptr ) {
		return 0;
	}
	
	m_nCursorColumn = std::min( m_nCursorColumn + m_nCursorIncrement * n,
								  m_pPattern->getLength() - m_nCursorIncrement );

	ensureCursorVisible();

	return m_nCursorColumn;
}

void PatternEditorPanel::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		// It's sufficient to check the properties of just one label
		// because they will always carry the same.
		QFont boldFont( pPref->getTheme().m_font.m_sApplicationFontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );
		boldFont.setBold( true );
		m_pDrumkitLabel->setFont( boldFont );
		m_pPatternNameLbl->setFont( boldFont );

		updateStyleSheet();
	}

	if ( changes & ( H2Core::Preferences::Changes::Colors ) ) {
		updateStyleSheet();
	}
}

void PatternEditorPanel::updateStyleSheet() {

	const auto pPref = H2Core::Preferences::get_instance();
	int nFactorTop = 112;
	
	QColor topColorLight = pPref->getTheme().m_color.m_midColor.lighter( nFactorTop );
	QColor topColorDark = pPref->getTheme().m_color.m_midColor.darker( nFactorTop );

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
		.arg( pPref->getTheme().m_color.m_midLightColor.name() );

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
		mode = NotePropertiesRuler::Mode::KeyOctave;
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

void PatternEditorPanel::patchBayBtnClicked() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		ERRORLOG( "No Song set" );
		return;
	}

	auto pPatchBay = new PatchBay(
		nullptr, pSong->getPatternList(), pSong->getDrumkit() );
	pPatchBay->exec();
	delete pPatchBay;
}

const DrumPatternRow PatternEditorPanel::getRowDB( int nRow ) const {
	if ( nRow < 0 || nRow >= m_db.size() ) {
		return { EMPTY_INSTR_ID, "" };
	}
	else {
		return m_db.at( nRow );
	}
}

void PatternEditorPanel::setSelectedRowDB( int nNewRow ) {
	if ( nNewRow == m_nSelectedRowDB ) {
		return;
	}

	if ( nNewRow < 0 || nNewRow >= m_db.size() ) {
		ERRORLOG( QString( "Provided row [%1] is out of DB bound [0,%2]" )
				  .arg( nNewRow ).arg( m_db.size() ) );
		return;
	}

	auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		if ( nNewRow < pSong->getDrumkit()->getInstruments()->size() ) {
			pHydrogen->setSelectedInstrumentNumber( nNewRow );
		}
		else {
			pHydrogen->setSelectedInstrumentNumber( EMPTY_INSTR_ID );
		}
	}
	else {
		pHydrogen->setSelectedInstrumentNumber( EMPTY_INSTR_ID );
	}

	m_nSelectedRowDB = nNewRow;
}

int PatternEditorPanel::getRowNumberDB() const {
	return m_db.size();
}

int PatternEditorPanel::findRowDB( Note* pNote ) const {
	if ( pNote != nullptr ) {
		for ( int ii = 0; ii < m_db.size(); ++ii ) {
			if ( pNote->get_instrument_id() == m_db[ ii ].nInstrumentID &&
				 pNote->getType() == m_db[ ii ].sType ) {
				return ii;
			}
		}

		ERRORLOG( QString( "Note [%1] is not contained in DB" )
				  .arg( pNote->toQString() ) );
		printDB();
	}

	return -1;
}

std::shared_ptr<H2Core::Instrument> PatternEditorPanel::getSelectedInstrument() const {
	if ( m_nSelectedRowDB < 0 || m_nSelectedRowDB >= m_db.size() ) {
		return nullptr;
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return nullptr;
	}

	auto row = m_db.at( m_nSelectedRowDB );
	if ( row.nInstrumentID == EMPTY_INSTR_ID ) {
		// Row is associated with a type but not an instrument of the current
		// kit.
		return nullptr;
	}

	return pSong->getDrumkit()->getInstruments()->find( row.nInstrumentID );
}

void PatternEditorPanel::updateDB() {
	m_db.clear();

	if ( m_pPattern == nullptr ) {
		return;
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "song not ready yet" );
		return;
	}

	// First we add all instruments of the current drumkit in the order author
	// of the kit intended.
	for ( const auto& ppInstrument : *pSong->getDrumkit()->getInstruments() ) {
		if ( ppInstrument != nullptr ) {
			m_db.push_back( { ppInstrument->get_id(), ppInstrument->getType() } );
		}
	}

	// Next we add rows for all notes in the selected pattern not covered by any
	// of the instruments above.
	const auto kitTypes = pSong->getDrumkit()->getAllTypes();
	std::set<DrumkitMap::Type> additionalTypes;
	for ( const auto& [ _, ppNote ] : *m_pPattern->getNotes() ) {
		if ( ppNote != nullptr && ! ppNote->getType().isEmpty() &&
			 kitTypes.find( ppNote->getType() ) == kitTypes.end() ) {
			// Note is not associated with current kit.
			if ( additionalTypes.find( ppNote->getType() ) ==
				 additionalTypes.end() ) {
				additionalTypes.insert( ppNote->getType() );
				m_db.push_back( { EMPTY_INSTR_ID, ppNote->getType() } );
			}
		}
	}

	m_nSelectedRowDB = Hydrogen::get_instance()->getSelectedInstrumentNumber();

	printDB();
}

void PatternEditorPanel::printDB() const {
	QString sMsg = "PatternEditorPanel database:";
	for ( int ii = 0; ii < m_db.size(); ++ii ) {
		sMsg.append( QString( "\n\t[%1] ID: %2, Type: %3" )
					 .arg( ii ).arg( m_db[ ii ].nInstrumentID )
					 .arg( m_db[ ii ].sType ) );
	}

	DEBUGLOG( sMsg );
}
