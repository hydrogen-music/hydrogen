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

#include <cmath>

#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/EventQueue.h>
#include <core/Hydrogen.h>

#include "DrumPatternEditor.h"
#include "NotePropertiesRuler.h"
#include "PatternEditorSidebar.h"
#include "PatternEditorPanel.h"
#include "PatternEditorRuler.h"
#include "PianoRollEditor.h"

#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "../PatternPropertiesDialog.h"
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

DrumPatternRow::DrumPatternRow() noexcept
	: nInstrumentID( EMPTY_INSTR_ID)
	, sType( "" )
	, bAlternate( false )
	, bMappedToDrumkit( false )
	, bPlaysBackAudio( false ){
}
DrumPatternRow::DrumPatternRow( int nId, const QString& sTypeString,
								bool bAlt, bool bMapped,
								bool bPlaysAudio ) noexcept
	: nInstrumentID( nId)
	, sType( sTypeString )
	, bAlternate( bAlt )
	, bMappedToDrumkit( bMapped )
	, bPlaysBackAudio( bPlaysAudio ) {
}

QString DrumPatternRow::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[DrumPatternRow]\n" ).arg( sPrefix )
			.append( QString( "%1%2nInstrumentID: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( nInstrumentID ) )
			.append( QString( "%1%2sType: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( sType ) )
			.append( QString( "%1%2bAlternate: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( bAlternate ) )
			.append( QString( "%1%2bMappedToDrumkit: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( bMappedToDrumkit ) )
			.append( QString( "%1%2bPlaysBackAudio: %3\n" ).arg( sPrefix )
					 .arg( s ).arg( bPlaysBackAudio ) );
	}
	else {
		sOutput = QString( "[DrumPatternRow] " )
			.append( QString( "nInstrumentID: %1" ).arg( nInstrumentID ) )
			.append( QString( ", sType: %1" ).arg( sType ) )
			.append( QString( ", bAlternate: %1" ).arg( bAlternate ) )
			.append( QString( ", bMappedToDrumkit: %1" ).arg( bMappedToDrumkit ) )
			.append( QString( ", bPlaysBackAudio: %1" ).arg( bPlaysBackAudio ) );
	}

	return sOutput;
}


PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
	: QWidget( pParent )
	, m_pPattern( nullptr )
	, m_bArmPatternSizeSpinBoxes( true )
	, m_bPatternSelectedViaTab( false )
	, m_bTypeLabelsMustBeVisible( false )
{
	setAcceptDrops(true);

	const auto pPref = Preferences::get_instance();
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	const auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	m_nSelectedRowDB = pHydrogen->getSelectedInstrumentNumber();

	m_nResolution = pPref->getPatternEditorGridResolution();
	m_bIsUsingTriplets = pPref->isPatternEditorUsingTriplets();
	m_bQuantized = pPref->getQuantizeEvents();

	QFont boldFont( pPref->getTheme().m_font.m_sApplicationFontFamily,
					getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	boldFont.setBold( true );

	m_nCursorColumn = 0;

	// Spacing between a label and the widget to its label.
	const int nLabelSpacing = 6;
// Editor TOP

	m_pTabBar = new QTabBar( this );
	m_pTabBar->setFocusPolicy( Qt::ClickFocus );
	m_pTabBar->setObjectName( "patternEditorTabBar" );
	// Select a different pattern
	connect( m_pTabBar, &QTabBar::tabBarClicked, [&]( int nIndex ) {
		if ( Hydrogen::get_instance()->isPatternEditorLocked() &&
			 Hydrogen::get_instance()->getAudioEngine()->getState() ==
			 AudioEngine::State::Playing ) {
			HydrogenApp::get_instance()->getSongEditorPanel()->
				highlightPatternEditorLocked();
		}
		else {
			// Select the corresponding pattern
			m_bPatternSelectedViaTab = true;
			CoreActionController::selectPattern( m_tabPatternMap[ nIndex ] );
		}
	});
	// Open the properties dialog for a particular pattern.
	connect( m_pTabBar, &QTabBar::tabBarDoubleClicked, [&]( int nIndex ) {
		const int nPattern = m_tabPatternMap[ nIndex ];
		if ( Hydrogen::get_instance()->getSong() != nullptr ) {
			const auto pPattern =
				Hydrogen::get_instance()->getSong()->getPatternList()->get( nPattern );
			if ( pPattern != nullptr ) {
				PatternPropertiesDialog dialog( this, pPattern, nPattern, false );
				dialog.exec();
			}
		}
	});

	m_pToolBar = new QWidget( nullptr );
	m_pToolBar->setFocusPolicy( Qt::ClickFocus );
	m_pToolBar->setFont( boldFont );
	m_pToolBar->setFixedHeight( 24 );
	m_pToolBar->setObjectName( "patternEditorToolBar" );

	QHBoxLayout* pToolBarHBox = new QHBoxLayout( m_pToolBar );
	pToolBarHBox->setSpacing( 2 );
	pToolBarHBox->setMargin( 0 );
	pToolBarHBox->setAlignment( Qt::AlignLeft );


	//soundlibrary name
	m_pDrumkitLabel = new ClickableLabel( nullptr, QSize( 0, 0 ), "",
										  ClickableLabel::Color::Bright, false );
	m_pDrumkitLabel->setFocusPolicy( Qt::ClickFocus );
	m_pDrumkitLabel->setFont( boldFont );
	m_pDrumkitLabel->setIndent( PatternEditorSidebar::m_nMargin );
	m_pDrumkitLabel->setToolTip( tr( "Drumkit used in the current song" ) );
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
		m_pDrumkitLabel->setText( pSong->getDrumkit()->getName() );
	}
	connect( m_pDrumkitLabel, &ClickableLabel::labelDoubleClicked,
			 [=]() { HydrogenApp::get_instance()->getMainForm()->
					 action_drumkit_properties(); } );

//wolke some background images back_size_res
	m_pSizeResol = new QWidget( nullptr );
	m_pSizeResol->setFocusPolicy( Qt::ClickFocus );
	m_pSizeResol->setObjectName( "sizeResol" );
	m_pSizeResol->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	m_pSizeResol->move( 0, 3 );
	pToolBarHBox->addWidget( m_pSizeResol );

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
	m_pLCDSpinBoxNumerator->setFocusPolicy( Qt::ClickFocus );
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
	m_pLCDSpinBoxDenominator->setFocusPolicy( Qt::ClickFocus );
	pSizeResolLayout->addWidget( m_pLCDSpinBoxDenominator );
	pSizeResolLayout->addSpacing( nLabelSpacing );

	// GRID resolution
	m_pResolutionLbl = new ClickableLabel(
		m_pSizeResol, QSize( 0, 0 ), pCommonStrings->getResolutionLabel(),
		ClickableLabel::Color::Dark );
	m_pResolutionLbl->setAlignment( Qt::AlignRight );
	m_pResolutionLbl->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
	pSizeResolLayout->addWidget( m_pResolutionLbl );
	
	m_pResolutionCombo = new LCDCombo( m_pSizeResol, QSize( 0, 0 ), true );
	m_pResolutionCombo->setFocusPolicy( Qt::ClickFocus );
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

	if ( m_nResolution == MAX_NOTES ) {
		nIndex = 11;
	} else if ( ! m_bIsUsingTriplets ) {
		switch ( m_nResolution ) {
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
		switch ( m_nResolution ) {
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
	m_pRec->setFocusPolicy( Qt::ClickFocus );
	m_pRec->setObjectName( "pRec" );
	m_pRec->move( 0, 3 );
	pToolBarHBox->addWidget( m_pRec );
	
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
		QSize( 15, 13 ), tr( "Hear new notes" ), false, false );
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
		false, false );
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

	pToolBarHBox->addStretch();
	
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
	m_pZoomInBtn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Push, "plus.svg", "", false,
		QSize( 9, 9 ), tr( "Zoom in" ) );
	m_pZoomInBtn->setFocusPolicy( Qt::ClickFocus );
	connect( m_pZoomInBtn, SIGNAL( clicked() ), this, SLOT( zoomInBtnClicked() ) );


	// zoom-out btn
	m_pZoomOutBtn = new Button(
		nullptr, QSize( 19, 15 ), Button::Type::Push, "minus.svg", "", false,
		QSize( 9, 9 ), tr( "Zoom out" ) );
	m_pZoomOutBtn->setFocusPolicy( Qt::ClickFocus );
	connect( m_pZoomOutBtn, SIGNAL( clicked() ), this, SLOT( zoomOutBtnClicked() ) );
// End Editor TOP


	// external horizontal scrollbar
	m_pPatternEditorHScrollBar = new QScrollBar( Qt::Horizontal , nullptr  );
	m_pPatternEditorHScrollBar->setObjectName( "PatternEditorHScrollBar" );
	m_pPatternEditorHScrollBar->setFocusPolicy( Qt::ClickFocus );
	connect( m_pPatternEditorHScrollBar, SIGNAL( valueChanged( int ) ),
			 this, SLOT( syncToExternalHorizontalScrollbar( int ) ) );

	// external vertical scrollbar
	m_pPatternEditorVScrollBar = new QScrollBar( Qt::Vertical, nullptr );
	m_pPatternEditorVScrollBar->setObjectName( "PatternEditorVScrollBar" );
	m_pPatternEditorVScrollBar->setFocusPolicy( Qt::ClickFocus );
	connect( m_pPatternEditorVScrollBar, SIGNAL(valueChanged( int)),
			 this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	QHBoxLayout *pPatternEditorHScrollBarLayout = new QHBoxLayout();
	pPatternEditorHScrollBarLayout->setSpacing( 0 );
	pPatternEditorHScrollBarLayout->setMargin( 0 );
	pPatternEditorHScrollBarLayout->addWidget( m_pPatternEditorHScrollBar );
	pPatternEditorHScrollBarLayout->addWidget( m_pZoomInBtn );
	pPatternEditorHScrollBarLayout->addWidget( m_pZoomOutBtn );

	m_pPatternEditorHScrollBarContainer = new QWidget();
	m_pPatternEditorHScrollBarContainer->setLayout( pPatternEditorHScrollBarLayout );


	QPalette label_palette;
	label_palette.setColor( QPalette::WindowText, QColor( 230, 230, 230 ) );

	updatePatternInfo();
	updateDB();

	// restore grid resolution
	m_nCursorIncrement = ( m_bIsUsingTriplets ? 4 : 3 ) *
		MAX_NOTES / ( m_nResolution * 3 );

	HydrogenApp::get_instance()->addEventListener( this );

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged,
			 this, &PatternEditorPanel::onPreferencesChanged );
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

	// PianoRollEditor
	m_pPianoRollScrollView = new WidgetScrollArea( nullptr );
	m_pPianoRollScrollView->setObjectName( "PianoRollScrollView" );
	m_pPianoRollScrollView->setFocusPolicy( Qt::NoFocus );
	m_pPianoRollScrollView->setFrameShape( QFrame::NoFrame );
	m_pPianoRollScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
	m_pPianoRollScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPianoRollEditor = new PianoRollEditor(
		m_pPianoRollScrollView->viewport() );
	m_pPianoRollScrollView->setWidget( m_pPianoRollEditor );
	connect( m_pPianoRollScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 this, SLOT( on_patternEditorHScroll(int) ) );
	connect( m_pPianoRollScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ),
			 m_pPianoRollEditor, SLOT( scrolled( int ) ) );
	connect( m_pPianoRollScrollView->verticalScrollBar(), SIGNAL( valueChanged( int ) ),
			 m_pPianoRollEditor, SLOT( scrolled( int ) ) );

	m_pPianoRollScrollView->hide();
	m_pPianoRollScrollView->setFocusProxy( m_pPianoRollEditor );

	m_pPianoRollEditor->mergeSelectionGroups( m_pDrumPatternEditor );

	// Instrument list
	m_pSidebarScrollView = new WidgetScrollArea( nullptr );
	m_pSidebarScrollView->setObjectName( "SidebarScrollView" );
	m_pSidebarScrollView->setFocusPolicy( Qt::ClickFocus );
	m_pSidebarScrollView->setFrameShape( QFrame::NoFrame );
	m_pSidebarScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pSidebarScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	m_pSidebar = new PatternEditorSidebar(
		m_pSidebarScrollView->viewport() );
	m_pSidebarScrollView->setWidget( m_pSidebar );
	m_pSidebarScrollView->setFixedWidth( m_pSidebar->width() );
	m_pSidebar->setFocusPolicy( Qt::ClickFocus );
	m_pSidebar->setFocusProxy( m_pEditorScrollView );

	connect( m_pSidebarScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternEditorVScroll(int) ) );
	m_pSidebarScrollView->setFocusProxy( m_pSidebar );

	// NOTE_VELOCITY EDITOR
	m_pNoteVelocityScrollView = new WidgetScrollArea( nullptr );
	m_pNoteVelocityScrollView->setObjectName( "NoteVelocityScrollView" );
	m_pNoteVelocityScrollView->setFocusPolicy( Qt::NoFocus );
	m_pNoteVelocityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteVelocityScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pNoteVelocityEditor = new NotePropertiesRuler(
		m_pNoteVelocityScrollView->viewport(),
		NotePropertiesRuler::Property::Velocity,
		NotePropertiesRuler::Layout::Normalized );
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
		m_pNotePanScrollView->viewport(), NotePropertiesRuler::Property::Pan,
		NotePropertiesRuler::Layout::Centered );
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
		NotePropertiesRuler::Property::LeadLag,
		NotePropertiesRuler::Layout::Centered );
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
		NotePropertiesRuler::Property::KeyOctave,
		NotePropertiesRuler::Layout::KeyOctave );
	m_pNoteKeyOctaveScrollView->setWidget( m_pNoteKeyOctaveEditor );
	m_pNoteKeyOctaveScrollView->setFixedHeight(
		NotePropertiesRuler::nKeyOctaveHeight );
	connect( m_pNoteKeyOctaveScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
			 this, SLOT( on_patternEditorHScroll( int ) ) );
	connect( m_pNoteKeyOctaveScrollView->horizontalScrollBar(), SIGNAL( valueChanged( int ) ),
			 m_pNoteKeyOctaveEditor, SLOT( scrolled( int ) ) );

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
		NotePropertiesRuler::Property::Probability,
		NotePropertiesRuler::Layout::Normalized );
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
	m_pPropertiesPanel->setFixedHeight( 100 );
	m_pPropertiesPanel->setMaximumWidth( PatternEditorSidebar::m_nWidth );


	QVBoxLayout *pPropertiesVBox = new QVBoxLayout( m_pPropertiesPanel );
	pPropertiesVBox->setSpacing( 0 );
	pPropertiesVBox->setMargin( 0 );

	m_pPropertiesCombo = new LCDCombo( nullptr, QSize( 0, 0 ), false );
	m_pPropertiesCombo->setFixedHeight( 18 );
	m_pPropertiesCombo->setMaximumWidth( PatternEditorSidebar::m_nWidth );
	m_pPropertiesCombo->setFocusPolicy( Qt::ClickFocus );
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

	pGrid->addWidget( m_pTabBar, 0, 1, 1, 2 );
	pGrid->addWidget( m_pToolBar, 1, 1, 1, 2 );
	pGrid->addWidget( m_pDrumkitLabel, 2, 0 );
	pGrid->addWidget( m_pRulerScrollView, 2, 1 );

	pGrid->addWidget( m_pSidebarScrollView, 3, 0 );

	pGrid->addWidget( m_pEditorScrollView, 3, 1 );
	pGrid->addWidget( m_pPianoRollScrollView, 3, 1 );

	pGrid->addWidget( m_pPatternEditorVScrollBar, 3, 2 );
	pGrid->addWidget( m_pPatternEditorHScrollBarContainer, 10, 1 );
	pGrid->addWidget( m_pNoteVelocityScrollView, 5, 1 );
	pGrid->addWidget( m_pNotePanScrollView, 5, 1 );
	pGrid->addWidget( m_pNoteLeadLagScrollView, 5, 1 );
	pGrid->addWidget( m_pNoteKeyOctaveScrollView, 5, 1 );
	pGrid->addWidget( m_pNoteProbabilityScrollView, 5, 1 );

	pGrid->addWidget( m_pPropertiesPanel, 5, 0 );
	pGrid->setRowStretch( 3, 100 );
	pMainPanel->setLayout( pGrid );

	QVBoxLayout *pVBox = new QVBoxLayout();
	pVBox->setSpacing( 0 );
	pVBox->setMargin( 0 );
	this->setLayout( pVBox );

	pVBox->addWidget( pMainPanel );

	updateStyleSheet();
	updateTypeLabelVisibility();
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

	const int nPreviousRows = m_db.size();

	updateDB();
	updateEditors();
	m_pSidebar->updateRows();

	if ( nPreviousRows != m_db.size() ) {
		resizeEvent( nullptr );
	}
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
	m_pSidebarScrollView->verticalScrollBar()->setValue( m_pPatternEditorVScrollBar->value() );

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
	switch( nSelected ) {
	case 0:
		// 1/4
		m_nResolution = 4;
		m_bIsUsingTriplets = false;
		break;
	case 1:
		// 1/8
		m_nResolution = 8;
		m_bIsUsingTriplets = false;
		break;
	case 2:
		// 1/16
		m_nResolution = 16;
		m_bIsUsingTriplets = false;
		break;
	case 3:
		// 1/32
		m_nResolution = 32;
		m_bIsUsingTriplets = false;
		break;
	case 4:
		// 1/64
		m_nResolution = 64;
		m_bIsUsingTriplets = false;
		break;
	case 6:
		// 1/4T
		m_nResolution = 8;
		m_bIsUsingTriplets = true;
		break;
	case 7:
		// 1/8T
		m_nResolution = 16;
		m_bIsUsingTriplets = true;
		break;
	case 8:
		// 1/16T
		m_nResolution = 32;
		m_bIsUsingTriplets = true;
		break;
	case 9:
		// 1/32T
		m_nResolution = 64;
		m_bIsUsingTriplets = true;
		break;
	case 11:
		// off
		m_nResolution = MAX_NOTES;
		m_bIsUsingTriplets = false;
		break;
	default:
		ERRORLOG( QString( "Invalid resolution selection [%1]" )
				  .arg( nSelected ) );
		return;
	}

	auto pPref = Preferences::get_instance();
	pPref->setPatternEditorGridResolution( m_nResolution );
	pPref->setPatternEditorUsingTriplets( m_bIsUsingTriplets );

	m_nCursorIncrement =
		( m_bIsUsingTriplets ? 4 : 3 ) * MAX_NOTES / ( m_nResolution * 3 );
	setCursorColumn(
		m_nCursorIncrement * ( m_nCursorColumn / m_nCursorIncrement ), false );

	updateEditors();
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

	updateQuantization( nullptr );

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

/// richiamato dall'uso dello scroll del mouse
void PatternEditorPanel::contentsMoving( int dummy )
{
	UNUSED( dummy );
	//INFOLOG( "contentsMoving" );
	syncToExternalHorizontalScrollbar(0);
}



void PatternEditorPanel::selectedInstrumentChangedEvent()
{
	const int nInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	if ( nInstrument != -1 ) {
		m_nSelectedRowDB = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	}

	ensureVisible();
	updateEditors();
	resizeEvent( nullptr );	// force a scrollbar update
}

bool PatternEditorPanel::hasPatternEditorFocus() const {
	return hasFocus() ||
		m_pPatternEditorRuler->hasFocus() ||
		m_pDrumPatternEditor->hasFocus() ||
		m_pPianoRollEditor->hasFocus() ||
		m_pNoteVelocityEditor->hasFocus() ||
		m_pNotePanEditor->hasFocus() ||
		m_pNoteLeadLagEditor->hasFocus() ||
		m_pNoteKeyOctaveEditor->hasFocus() ||
		m_pNoteProbabilityEditor->hasFocus() ||
		m_pZoomInBtn->hasFocus() ||
		m_pZoomOutBtn->hasFocus() ||
		m_pPatternEditorHScrollBar->hasFocus() ||
		m_pPatternEditorVScrollBar->hasFocus() ||
		m_pPianoRollScrollView->hasFocus() ||
		m_pRec->hasFocus() ||
		m_pResolutionCombo->hasFocus() ||
		m_pSizeResol->hasFocus() ||
		m_pLCDSpinBoxNumerator->hasFocus() ||
		m_pLCDSpinBoxDenominator->hasFocus() ||
		m_pPropertiesCombo->hasFocus() ||
		m_pDrumkitLabel->hasFocus() ||
		m_pTabBar->hasFocus() ||
		m_pToolBar->hasFocus();
}

void PatternEditorPanel::showDrumEditor()
{
	__show_drum_btn->setToolTip( tr( "Show piano roll editor" ) );
	__show_drum_btn->setChecked( false );
	m_pPianoRollScrollView->hide();
	m_pEditorScrollView->show();
	m_pSidebarScrollView->show();

	m_pEditorScrollView->setFocus();
	m_pPatternEditorRuler->setFocusProxy( m_pEditorScrollView );
	m_pSidebar->setFocusProxy( m_pEditorScrollView );
	m_pSidebar->dimRows( false );

	m_pDrumPatternEditor->updateEditor(); // force an update
	ensureVisible();

	getVisiblePropertiesRuler()->syncLasso();

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
	m_pSidebarScrollView->show();

	m_pPianoRollScrollView->setFocus();
	m_pPatternEditorRuler->setFocusProxy( m_pPianoRollScrollView );
	m_pSidebar->setFocusProxy( m_pPianoRollScrollView );
	m_pSidebar->dimRows( true );

	m_pPianoRollEditor->updateEditor(); // force an update
	ensureVisible();

	getVisiblePropertiesRuler()->syncLasso();

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
	if ( m_pPianoRollScrollView->isVisible() ) {
		return m_pPianoRollEditor;
	}
	return m_pDrumPatternEditor;
}

NotePropertiesRuler* PatternEditorPanel::getVisiblePropertiesRuler() const {
	if ( m_pNoteVelocityEditor->isVisible() ) {
		return m_pNoteVelocityEditor;
	}
	else if ( m_pNotePanEditor->isVisible() ) {
		return m_pNotePanEditor;
	}
	else if ( m_pNoteLeadLagEditor->isVisible() ) {
		return m_pNoteLeadLagEditor;
	}
	else if ( m_pNoteKeyOctaveEditor->isVisible() ) {
		return m_pNoteKeyOctaveEditor;
	}
	else {
		return m_pNoteProbabilityEditor;
	}
}

std::vector<std::shared_ptr<Pattern>> PatternEditorPanel::getPatternsToShow() const
{
	Hydrogen* pHydrogen = Hydrogen::get_instance();
	std::vector<std::shared_ptr<Pattern>> patterns;
	auto pAudioEngine = pHydrogen->getAudioEngine();

	// When using song mode without the pattern editor being locked
	// only the current pattern will be shown. In every other base
	// remaining playing patterns not selected by the user are added
	// as well.
	if ( ! ( pHydrogen->getMode() == Song::Mode::Song &&
			 ! pHydrogen->isPatternEditorLocked() ) ) {
		pAudioEngine->lock( RIGHT_HERE );
		if ( pAudioEngine->getPlayingPatterns()->size() > 0 ) {
			std::set< std::shared_ptr<Pattern> > patternSet;

			std::vector< std::shared_ptr<PatternList> > patternLists;
			patternLists.push_back( pAudioEngine->getPlayingPatterns() );
			if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
				patternLists.push_back( pAudioEngine->getNextPatterns() );
			}

			for ( const auto& pPatternList : patternLists ) {
				for ( int i = 0; i <  pPatternList->size(); i++) {
					auto ppPattern = pPatternList->get( i );
					if ( ppPattern != m_pPattern ) {
						patternSet.insert( ppPattern );
					}
				}
			}
			for ( const auto& ppPattern : patternSet ) {
				patterns.push_back( ppPattern );
			}
		}
		pAudioEngine->unlock();
	}
	else if ( m_pPattern != nullptr &&
			  pHydrogen->getMode() == Song::Mode::Song &&
			  m_pPattern->getVirtualPatterns()->size() > 0 ) {
		// A virtual pattern was selected in song mode without the
		// pattern editor being locked. Virtual patterns in selected
		// pattern mode are handled using the playing pattern above.
		for ( const auto ppVirtualPattern : *m_pPattern ) {
			patterns.push_back( ppVirtualPattern );
		}
	}


	if ( m_pPattern != nullptr ) {
		patterns.push_back( m_pPattern );
	}

	return patterns;
}

void PatternEditorPanel::zoomInBtnClicked()
{
	const float fOldGridWidth = m_pPatternEditorRuler->getGridWidth();

	if ( fOldGridWidth >= 24 ){
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

	getVisiblePropertiesRuler()->zoomLasso( fOldGridWidth );
	getVisibleEditor()->zoomLasso( fOldGridWidth );

	ensureVisible();

	updateEditors();
	resizeEvent( nullptr );
}

void PatternEditorPanel::zoomOutBtnClicked()
{
	const float fOldGridWidth = m_pPatternEditorRuler->getGridWidth();

	m_pPatternEditorRuler->zoomOut();
	m_pDrumPatternEditor->zoomOut();
	m_pNoteVelocityEditor->zoomOut();
	m_pNoteLeadLagEditor->zoomOut();
	m_pNoteKeyOctaveEditor->zoomOut();
	m_pNoteProbabilityEditor->zoomOut();
	m_pNotePanEditor->zoomOut();
	m_pPianoRollEditor->zoomOut();

	auto pPref = Preferences::get_instance();
	pPref->setPatternEditorGridWidth( m_pPatternEditorRuler->getGridWidth() );
	pPref->setPatternEditorGridHeight( m_pDrumPatternEditor->getGridHeight() );

	getVisiblePropertiesRuler()->zoomLasso( fOldGridWidth );
	getVisibleEditor()->zoomLasso( fOldGridWidth );

	ensureVisible();

	updateEditors();
	resizeEvent( nullptr );
}

void PatternEditorPanel::updatePatternInfo() {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	m_hoveredNotes.clear();

	m_pPattern = nullptr;
	if ( pSong != nullptr ) {
		m_nPatternNumber = pHydrogen->getSelectedPatternNumber();
		const auto pPatternList = pSong->getPatternList();
		if ( m_nPatternNumber != -1 &&
			 m_nPatternNumber < pPatternList->size() ) {
			m_pPattern = pPatternList->get( m_nPatternNumber );
		}
	}

	if ( m_pPattern == nullptr ) {
		this->setWindowTitle( tr( "Pattern editor - No pattern selected" ) );

		for ( int ii = m_pTabBar->count(); ii >= 0; --ii ) {
			m_pTabBar->removeTab( ii );
		}
		m_tabPatternMap.clear();

		m_pTabBar->addTab( tr( "No pattern selected" ) );
		m_pTabBar->setTabEnabled( 0, false );

		m_pLCDSpinBoxDenominator->setIsActive( false );
		m_pLCDSpinBoxNumerator->setIsActive( false );

		return;
	}
	else {
		m_pLCDSpinBoxDenominator->setIsActive( true );
		m_pLCDSpinBoxNumerator->setIsActive( true );
	}

	if ( ! m_bPatternSelectedViaTab ) {
		// Reset the tab bar
		for ( int ii = m_pTabBar->count(); ii >= 0; --ii ) {
			m_pTabBar->removeTab( ii );
		}
		m_tabPatternMap.clear();
	}
	else {
		// But not if triggered via the tab bar. Then, we just switch the
		// selected tab.
		int nTabIndex = -1;
		const auto pPatternList = pSong->getPatternList();
		const int nPatternIndex = pPatternList->index( m_pPattern );
		for ( const auto& [ nnTab, nnPattern ] : m_tabPatternMap ) {
			// We also need to assure the pattern name is still valid, since it
			// might have changed in a previous action.
			const auto ppPattern = pPatternList->get( nnPattern );
			if ( ppPattern != nullptr &&
				 ppPattern->getName() != m_pTabBar->tabText( nnTab ) ) {
				m_pTabBar->setTabText( nnTab, ppPattern->getName() );
			}

			if ( nnPattern == nPatternIndex ) {
				nTabIndex = nnTab;
			}
		}

		if ( nTabIndex != -1 ) {
			m_pTabBar->setCurrentIndex( nTabIndex );
		}
		else {
			ERRORLOG( "Unable to find pattern" );
		}
	}

	// update pattern size LCD
	m_bArmPatternSizeSpinBoxes = false;

	const double fNewDenominator =
		static_cast<double>( m_pPattern->getDenominator() );
	if ( fNewDenominator != m_pLCDSpinBoxDenominator->value() &&
		 ! m_pLCDSpinBoxDenominator->hasFocus() ) {
		m_pLCDSpinBoxDenominator->setValue( fNewDenominator );

		// Update numerator to allow only for a maximum pattern length of four
		// measures.
		m_pLCDSpinBoxNumerator->setMaximum(
			4 * m_pLCDSpinBoxDenominator->value() );
	}

	const double fNewNumerator = static_cast<double>(
		m_pPattern->getLength() * m_pPattern->getDenominator() ) /
		static_cast<double>( MAX_NOTES );
	if ( fNewNumerator != m_pLCDSpinBoxNumerator->value() &&
		 ! m_pLCDSpinBoxNumerator->hasFocus() ) {
		m_pLCDSpinBoxNumerator->setValue( fNewNumerator );
	}

	m_bArmPatternSizeSpinBoxes = true;

	if ( ! m_bPatternSelectedViaTab ) {
		// Update pattern tabs
		m_pTabBar->addTab( m_pPattern->getName() );
		m_tabPatternMap[ 0 ] = pSong->getPatternList()->index( m_pPattern );

		auto patterns = getPatternsToShow();
		int nnCount = 1;
		const bool bTabsEnabled = ! ( pHydrogen->isPatternEditorLocked() &&
									  pHydrogen->getAudioEngine()->getState() ==
									  AudioEngine::State::Playing &&
									  pHydrogen->getMode() == Song::Mode::Song );
		for ( const auto& ppPattern : patterns ) {
			if ( ppPattern != nullptr && ppPattern != m_pPattern ) {
				m_tabPatternMap[ nnCount ] =
					pSong->getPatternList()->index( ppPattern );
				m_pTabBar->addTab( ppPattern->getName() );
				m_pTabBar->setTabEnabled( nnCount, bTabsEnabled );
				++nnCount;
			}
		}
	}

	if ( m_bPatternSelectedViaTab ) {
		m_bPatternSelectedViaTab = false;
	}
}

void PatternEditorPanel::updateEditors( bool bPatternOnly ) {

	// Changes of pattern may leave the cursor out of bounds.
	setCursorColumn( getCursorColumn(), false );

	m_pPatternEditorRuler->updateEditor( true );
	m_pNoteVelocityEditor->updateEditor( bPatternOnly );
	m_pNotePanEditor->updateEditor( bPatternOnly );
	m_pNoteLeadLagEditor->updateEditor( bPatternOnly );
	m_pNoteKeyOctaveEditor->updateEditor( bPatternOnly );
	m_pNoteProbabilityEditor->updateEditor( bPatternOnly );
	m_pPianoRollEditor->updateEditor( bPatternOnly );
	m_pDrumPatternEditor->updateEditor( bPatternOnly );
	m_pSidebar->updateEditor();
	updateTypeLabelVisibility();
}

void PatternEditorPanel::patternModifiedEvent() {
	updatePatternInfo();
	updateEditors();
	resizeEvent( nullptr );
}

void PatternEditorPanel::playingPatternsChangedEvent() {
	if ( PatternEditorPanel::isUsingAdditionalPatterns( m_pPattern ) ) {
		updatePatternInfo();
		updateEditors( true );
	}
}

void PatternEditorPanel::songModeActivationEvent() {
	updatePatternInfo();
	updateDB();
	updateEditors( true );

	resizeEvent( nullptr );
}

void PatternEditorPanel::stackedModeActivationEvent( int ) {
	updatePatternInfo();
	updateDB();
	updateEditors( true );

	resizeEvent( nullptr );
}

void PatternEditorPanel::songSizeChangedEvent() {
	if ( PatternEditorPanel::isUsingAdditionalPatterns( m_pPattern ) ) {
		updateEditors( true );
	}
}

void PatternEditorPanel::patternEditorLockedEvent() {
	updatePatternInfo();
	updateDB();
	updateEditors( true );
}

void PatternEditorPanel::stateChangedEvent( const H2Core::AudioEngine::State& state ) {
	auto pHydrogen = Hydrogen::get_instance();
	const bool bLocked = pHydrogen->isPatternEditorLocked();
	if ( bLocked ) {
		const bool bEnable =
			! ( bLocked &&
				pHydrogen->getAudioEngine()->getState() ==
				AudioEngine::State::Playing &&
				pHydrogen->getMode() == Song::Mode::Song );
		for ( int ii = 0; ii < m_pTabBar->count(); ++ii ) {
			m_pTabBar->setTabEnabled( ii, bEnable );
		}
	}
}

void PatternEditorPanel::relocationEvent() {
	if ( H2Core::Hydrogen::get_instance()->isPatternEditorLocked() ) {
		updateEditors( true );
	}
}

void PatternEditorPanel::instrumentMuteSoloChangedEvent( int ) {
	updateDB();
	updateEditors( true );
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

	auto pHydrogenApp = HydrogenApp::get_instance();
	pHydrogenApp->beginUndoMacro( tr( "Change pattern size to %1/%2" )
								  .arg( fNewNumerator ).arg( fNewDenominator ) );

	pHydrogenApp->pushUndoCommand(
		new SE_patternSizeChangedAction(
			nNewLength,
			m_pPattern->getLength(),
			fNewDenominator,
			m_pPattern->getDenominator(),
			m_nPatternNumber ) );

	pHydrogenApp->endUndoMacro();
}

void PatternEditorPanel::patternSizeChangedAction( int nLength, double fDenominator,
												   int nSelectedPatternNumber ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pPatternList = pSong->getPatternList();
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

	// Ensure the cursor stays within the accessible region of the current
	// pattern.
	if ( pPattern == m_pPattern && m_nCursorColumn >= nLength ) {
		int nNewColumn = std::floor( m_pPattern->getLength() /
									 m_nCursorIncrement ) * m_nCursorIncrement;
		if ( m_pPattern->getLength() % m_nCursorIncrement == 0 ) {
			nNewColumn -= m_nCursorIncrement;
		}
		setCursorColumn( nNewColumn );
	}
	
	EventQueue::get_instance()->pushEvent( Event::Type::PatternModified, -1 );
}

void PatternEditorPanel::dragEnterEvent( QDragEnterEvent *event )
{
	m_pSidebar->dragEnterEvent( event );
}



void PatternEditorPanel::dropEvent( QDropEvent *event )
{
	m_pSidebar->dropEvent( event );
}

void PatternEditorPanel::updateSongEvent( int nValue ) {
	// A new song got loaded
	if ( nValue == 0 ) {
		// Performs an editor update with updateEditor() (and no argument).
		updateDrumkitLabel();
		updatePatternInfo();
		updateDB();
		updateEditors();
		m_pPatternEditorRuler->updatePosition();
		m_pSidebar->updateRows();
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

void PatternEditorPanel::ensureVisible()
{
	if ( m_pEditorScrollView->isVisible() ) {
		const auto pos = m_pDrumPatternEditor->getCursorPosition();
		m_pEditorScrollView->ensureVisible( pos.x(), pos.y() );
	}
	else {
		const auto pos = m_pPianoRollEditor->getCursorPosition();
		m_pPianoRollScrollView->ensureVisible( pos.x(), pos.y() );
	}
}

void PatternEditorPanel::setCursorColumn( int nCursorColumn,
										  bool bUpdateEditors ) {
	if ( nCursorColumn < 0 ) {
		nCursorColumn = 0;
	}
	else if ( m_pPattern != nullptr &&
			  nCursorColumn >= m_pPattern->getLength() ) {
		return;
	}

	if ( nCursorColumn == m_nCursorColumn ) {
		return;
	}

	m_nCursorColumn = nCursorColumn;

	if ( bUpdateEditors && ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		ensureVisible();
		m_pSidebar->updateEditor();
		m_pPatternEditorRuler->update();
		getVisibleEditor()->update();
		getVisiblePropertiesRuler()->update();
	}
}

// Ensure updateModifiers() was called on the provided event first.
void PatternEditorPanel::moveCursorLeft( QKeyEvent* pEvent, int n ) {
	int nNewColumn;
	if ( ! m_bQuantized ) {
		nNewColumn = m_nCursorColumn - 1;
	}
	else {
		if ( m_nCursorColumn % m_nCursorIncrement == 0 ) {
			nNewColumn = m_nCursorColumn - m_nCursorIncrement * n;
		}
		else {
			nNewColumn = m_nCursorColumn - m_nCursorColumn % m_nCursorIncrement -
				m_nCursorIncrement * ( n - 1 );
		}
	}

	setCursorColumn( std::max( nNewColumn, 0 ) );
}

// Ensure updateModifiers() was called on the provided event first.
void PatternEditorPanel::moveCursorRight( QKeyEvent* pEvent, int n ) {
	if ( m_pPattern == nullptr ) {
		return;
	}

	int nNewColumn, nIncrement;
	// By pressing the Alt button the user can bypass quantization.
	if ( ! m_bQuantized ) {
		nNewColumn = m_nCursorColumn + 1;
		nIncrement = 1;
	}
	else {
		nIncrement = m_nCursorIncrement;
		if ( m_nCursorColumn % m_nCursorIncrement == 0 ) {
			nNewColumn = m_nCursorColumn + m_nCursorIncrement * n;
		}
		else {
			nNewColumn = m_nCursorColumn + m_nCursorIncrement -
				m_nCursorColumn % m_nCursorIncrement +
				m_nCursorIncrement * ( n - 1 );
		}

		// If a jump would be positioned beyond the end of the pattern, we move
		// to the last possible position instead.
		if ( n > 1 && nNewColumn >= m_pPattern->getLength() ) {
			nNewColumn = std::floor( m_pPattern->getLength() /
									 m_nCursorIncrement ) * m_nCursorIncrement;
			if ( m_pPattern->getLength() % m_nCursorIncrement == 0 ) {
				nNewColumn -= m_nCursorIncrement;
			}
		}
	}

	setCursorColumn( nNewColumn );
}

void PatternEditorPanel::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	const auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Font ) {
		
		// It's sufficient to check the properties of just one label
		// because they will always carry the same.
		QFont boldFont( pPref->getTheme().m_font.m_sApplicationFontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );
		boldFont.setBold( true );
		m_pDrumkitLabel->setFont( boldFont );
		m_pTabBar->setFont( boldFont );

		m_pPianoRollEditor->updateFont();
		m_pNoteKeyOctaveEditor->updateFont();
		m_pSidebar->updateFont();
		updateEditors();
	}

	if ( changes & ( H2Core::Preferences::Changes::Colors ) ) {
		updateStyleSheet();
		updateEditors();
	}
	else if ( changes & H2Core::Preferences::Changes::AppearanceTab ) {
		updateEditors( true );
	}
}

void PatternEditorPanel::updateStyleSheet() {

	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	const QColor colorDrumkit =
		colorTheme.m_patternEditor_instrumentAlternateRowColor.darker( 120 );
	const QColor colorDrumkitText =
		colorTheme.m_patternEditor_instrumentRowTextColor;
	const QColor colorPattern =
		colorTheme.m_patternEditor_alternateRowColor.darker( 120 );
	const QColor colorPatternLighter =
		colorTheme.m_patternEditor_selectedRowColor.darker( 114 );
	const QColor colorPatternText = colorTheme.m_patternEditor_textColor;

	m_pToolBar->setStyleSheet( QString( "\
QWidget#patternEditorToolBar {\
     background-color: %1; \
     color: %2; \
     border: 1px solid #000;\
}")
		.arg( colorPatternLighter.name() ).arg( colorPatternText.name() ) );
	m_pTabBar->setStyleSheet( QString( "\
QWidget#patternEditorTabBar {\
     background-color: %1; \
     color: %2; \
     font-weight: bold; \
}")
		.arg( colorPattern.name() ).arg( colorPatternText.name() ) );

	m_pDrumkitLabel->setStyleSheet( QString( "\
QLabel {\
     background-color: %1; \
     color: %2; \
     border: 1px solid #000;\
}" ).arg( colorDrumkit.name() ).arg( colorDrumkitText.name() ) );

	const QString sWidgetTopStyleSheet = QString( "\
QWidget#sizeResol {\
    background-color: %1;\
    color: %2;\
} \
QWidget#pRec {\
    background-color: %1;\
    color: %2;\
}" )
		.arg( colorPatternLighter.name() ).arg( colorPatternText.name() );

	m_pSizeResol->setStyleSheet( sWidgetTopStyleSheet );
	m_pRec->setStyleSheet( sWidgetTopStyleSheet );
	m_pPianoRollEditor->updateStyleSheet();
	m_pNoteKeyOctaveEditor->updateColors();
	m_pSidebar->updateStyleSheet();
}

void PatternEditorPanel::switchPatternSizeFocus() {
	if ( ! m_pLCDSpinBoxDenominator->hasFocus() ) {
		m_pLCDSpinBoxDenominator->setFocus();
	} else {
		m_pLCDSpinBoxNumerator->setFocus();
	}
}

NotePropertiesRuler::Property PatternEditorPanel::getSelectedNoteProperty() const
{
	NotePropertiesRuler::Property property;

	switch ( m_pPropertiesCombo->currentIndex() ) {
	case 0:
		property = NotePropertiesRuler::Property::Velocity;
		break;
	case 1:
		property = NotePropertiesRuler::Property::Pan;
		break;
	case 2:
		property = NotePropertiesRuler::Property::LeadLag;
		break;
	case 3:
		property = NotePropertiesRuler::Property::KeyOctave;
		break;
	case 4:
		property = NotePropertiesRuler::Property::Probability;
		break;
	default:
		ERRORLOG( QString( "Unsupported m_pPropertiesCombo index [%1]" )
				  .arg( m_pPropertiesCombo->currentIndex() ) );
	}

	return property;
}

void PatternEditorPanel::patchBayBtnClicked() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pPatchBay = new PatchBay(
		nullptr, pSong->getPatternList(), pSong->getDrumkit() );
	pPatchBay->exec();
	delete pPatchBay;
}

const DrumPatternRow PatternEditorPanel::getRowDB( int nRow ) const {
	if ( nRow < 0 || nRow >= m_db.size() ) {
		return DrumPatternRow();
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

	m_nSelectedRowDB = nNewRow;

	auto pHydrogen = Hydrogen::get_instance();
	const auto pSong = pHydrogen->getSong();
	if ( pSong != nullptr && pSong->getDrumkit() != nullptr &&
		 nNewRow < pSong->getDrumkit()->getInstruments()->size() ) {
		// Within the kit, rows/ids are unique.
		pHydrogen->setSelectedInstrumentNumber(
			nNewRow, Event::Trigger::Default );
	}
	else {
		// For all other lines the cached instrument number does not change. But
		// we still want to handle the update using the same event.
		pHydrogen->setSelectedInstrumentNumber(
			-1, Event::Trigger::Force );
	}
}

int PatternEditorPanel::getRowIndexDB( const DrumPatternRow& row ) {
	for ( int ii = 0; ii <= m_db.size(); ++ii ) {
		if ( m_db[ ii ].nInstrumentID == row.nInstrumentID &&
			 m_db[ ii ].sType == row.sType ) {
			return ii;
		}
	}

	ERRORLOG( QString( "Row [instrument id: %1, instrument type: %2] could not be found in DB" )
			  .arg( row.nInstrumentID ).arg( row.sType ) );
	printDB();

	return 0;
}

int PatternEditorPanel::getRowNumberDB() const {
	return m_db.size();
}

int PatternEditorPanel::findRowDB( std::shared_ptr<Note> pNote,
								   bool bSilent ) const {
	if ( pNote != nullptr ) {
		for ( int ii = 0; ii < m_db.size(); ++ii ) {
			// Both instrument ID and type are unique within a drumkit. But
			// since notes live in patterns and are independent of our kit,
			// their id/type combination does not have to match the one in the
			// kit.
			//
			// Instrument ID always takes precedence over type since the former
			// is used to associate a note to an instrument and the latter is
			// more a means of portability between different kits.
			if ( pNote->getInstrumentId() != EMPTY_INSTR_ID &&
				 pNote->getInstrumentId() == m_db[ ii ].nInstrumentID ) {
				return ii;
			}
			else if ( ! pNote->getType().isEmpty() &&
					  pNote->getType() == m_db[ ii ].sType ) {
				return ii;
			}
		}

		if ( ! bSilent ) {
			ERRORLOG( QString( "Note [%1] is not contained in DB" )
					  .arg( pNote->toQString() ) );
			printDB();
		}
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

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "song not ready yet" );
		return;
	}

	const auto pInstrumentList = pSong->getDrumkit()->getInstruments();

	int nnRow = 0;

	std::set<int> kitIds;
	// First we add all instruments of the current drumkit in the order author
	// of the kit intended.
	for ( const auto& ppInstrument : *pInstrumentList ) {
		if ( ppInstrument != nullptr ) {
			const bool bNoPlayback = ppInstrument->isMuted() ||
				( pInstrumentList->isAnyInstrumentSoloed() &&
				  ! ppInstrument->isSoloed() );

			m_db.push_back(
				DrumPatternRow( ppInstrument->getId(), ppInstrument->getType(),
								nnRow % 2 != 0, true, ! bNoPlayback ) );
			kitIds.insert( ppInstrument->getId() );
			++nnRow;
		}
	}

	// Next we add rows for all notes in the selected pattern not covered by any
	// of the instruments above.
	const auto kitTypes = pSong->getDrumkit()->getAllTypes();
	QStringList additionalTypes, additionalIds;

	for ( const auto& ppPattern : getPatternsToShow() ) {
		for ( const auto& [ _, ppNote ] : *ppPattern->getNotes() ) {
			if ( ppNote != nullptr && ! ppNote->getType().isEmpty() &&
				 kitTypes.find( ppNote->getType() ) == kitTypes.end() &&
				 ppNote->getInstrumentId() == EMPTY_INSTR_ID ) {
				// We just have an instrument type. The note was created
				// with a recent kit for an instrument type not contained in
				// the current drumkit.
				if ( ! additionalTypes.contains( ppNote->getType() ) ) {
					additionalTypes << ppNote->getType();
				}
			}
			else if ( ppNote != nullptr && ppNote->getType().isEmpty() &&
					  kitIds.find( ppNote->getInstrumentId() ) == kitIds.end() &&
					  ppNote->getInstrumentId() != EMPTY_INSTR_ID ) {
				// We just have an instrument id. The note was created with a
				// legacy kit or a newly created custom one not featuring (all)
				// instrument types and the id of the instrument the note was
				// created for is not used in the current drumkit.
				if ( ! additionalIds.contains(
						 QString::number( ppNote->getInstrumentId() ) ) ) {
					additionalIds << QString::number( ppNote->getInstrumentId() );
				}
			}
		}
	}

	// First we will insert all type-only rows. They are more likely to occur
	// than unmapped id-only ones.
	additionalTypes.sort();
	for ( const auto& ssType : additionalTypes ) {
		m_db.push_back( DrumPatternRow( EMPTY_INSTR_ID, ssType, nnRow % 2 != 0,
										false, false ) );
		++nnRow;
	}

	additionalIds.sort();
	for ( const auto& ssId : additionalIds ) {
		m_db.push_back( DrumPatternRow( ssId.toInt(), "", nnRow % 2 != 0,
										false, false ) );
		++nnRow;
	}

	const int nSelectedInstrument =
		Hydrogen::get_instance()->getSelectedInstrumentNumber();
	if ( nSelectedInstrument != -1 ) {
		m_nSelectedRowDB = nSelectedInstrument;
	}
	else if ( m_nSelectedRowDB >= m_db.size() ) {
		// Previously, a type-only row was selected. But we seem to have jumped
		// to a pattern in which there are no notes not associated to a
		// instrument -> no type-only rows. We selected the bottom-most
		// instrument instead.
		setSelectedRowDB( m_db.size() - 1 );
	}

	if ( additionalIds.size() > 0 || additionalTypes.size() > 0 ) {
		m_bTypeLabelsMustBeVisible = true;
	} else {
		m_bTypeLabelsMustBeVisible = false;
	}
}

void PatternEditorPanel::updateQuantization( QInputEvent* pEvent ) {
	bool bQuantized = Preferences::get_instance()->getQuantizeEvents();

	if ( pEvent != nullptr ) {
		if ( QKeyEvent* pKeyEvent = dynamic_cast<QKeyEvent*>( pEvent ) ) {
			// Keyboard events for press and release of modifier keys don't have
			// those keys in the modifiers set, so explicitly update these.
			if ( ( pEvent->type() == QEvent::KeyPress &&
				   pKeyEvent->key() == Qt::Key_Alt ) ||
				 pEvent->modifiers() & Qt::AltModifier ) {
				bQuantized = false;
			}
		}
		else if ( pEvent->modifiers() & Qt::AltModifier ) {
			bQuantized = false;
		}
	}

	if ( bQuantized != m_bQuantized ) {
		m_bQuantized = bQuantized;

		getVisibleEditor()->updateEditor();
		getVisiblePropertiesRuler()->updateEditor();
	}
}

void PatternEditorPanel::updateTypeLabelVisibility() {
	if ( m_pSidebar == nullptr ) {
		return;
	}

	// Update visibility
	bool bVisible;
	if ( Preferences::get_instance()->getPatternEditorAlwaysShowTypeLabels() ) {
		bVisible = true;
	}
	else {
		bVisible = m_bTypeLabelsMustBeVisible;
	}

	// Update the width of the sidebar.
	int nWidth;
	if ( ! bVisible ) {
		nWidth = PatternEditorSidebar::m_nWidth - SidebarRow::m_nTypeLblWidth;
	}
	else {
		nWidth = PatternEditorSidebar::m_nWidth;
	}

	m_pDrumkitLabel->setFixedWidth( nWidth );
	m_pSidebar->setFixedWidth( nWidth );
	m_pSidebarScrollView->setFixedWidth( nWidth );
	m_pPropertiesPanel->setFixedWidth( nWidth );
	m_pPropertiesCombo->setFixedWidth( nWidth );

	m_pSidebar->updateTypeLabelVisibility( bVisible );
}

void PatternEditorPanel::setHoveredNotesMouse(
	std::vector< std::pair< std::shared_ptr<H2Core::Pattern>,
							std::vector< std::shared_ptr<H2Core::Note> > >
			   > hoveredNotes,
	bool bUpdateEditors )
{
	if ( hoveredNotes == m_hoveredNotesMouse ) {
		return;
	}

	m_hoveredNotesMouse = hoveredNotes;

	updateHoveredNotes();

	if ( bUpdateEditors ) {
		getVisibleEditor()->update();
		getVisiblePropertiesRuler()->update();
	}
}

void PatternEditorPanel::setHoveredNotesKeyboard(
	std::vector< std::pair< std::shared_ptr<H2Core::Pattern>,
							std::vector< std::shared_ptr<H2Core::Note> > >
			   > hoveredNotes,
	bool bUpdateEditors )
{
	if ( hoveredNotes == m_hoveredNotesKeyboard ) {
		return;
	}

	m_hoveredNotesKeyboard = hoveredNotes;

	updateHoveredNotes();

	if ( bUpdateEditors ) {
		getVisibleEditor()->updateEditor( true );
		getVisiblePropertiesRuler()->updateEditor( true );
	}
}

void PatternEditorPanel::updateHoveredNotes() {
	m_hoveredNotes.clear();

	std::map< std::shared_ptr<Pattern>, std::vector< std::shared_ptr<Note> > >
		hoveredMap;

	// We collect notes of the current pattern separately in order to ensure
	// they are added last (and painted on top of the background ones).
	std::vector< std::shared_ptr<H2Core::Note> > notesForeground;
	for ( const auto& [ ppPattern, nnotes ] : m_hoveredNotesKeyboard ) {
		if ( ppPattern == m_pPattern ) {
			notesForeground = nnotes;
		}
		else {
			hoveredMap[ ppPattern ] = nnotes;
		}
	}

	for ( const auto& [ ppPattern, nnotes ] : m_hoveredNotesMouse ) {
		if ( ppPattern == m_pPattern ) {
			for ( const auto& ppNoteMouse : nnotes ) {
				notesForeground.push_back( ppNoteMouse );
			}
		}
		else if ( hoveredMap.find( ppPattern ) != hoveredMap.end() ) {
			// Pattern is already present. Merge it.
			for ( const auto& ppNoteMouse : nnotes ) {
				bool bPresent = false;
				for ( const auto& ppNoteHovered : hoveredMap[ ppPattern ] ) {
					if ( ppNoteHovered == ppNoteMouse ) {
						bPresent = true;
						break;
					}
				}

				if ( ! bPresent ) {
					hoveredMap[ ppPattern ].push_back( ppNoteMouse );
				}
			}
		}
		else {
			// Pattern is not present yet.
			hoveredMap[ ppPattern ] = nnotes;
		}
	}

	for ( const auto& [ ppPattern, nnotes ] : hoveredMap ) {
		m_hoveredNotes.push_back( std::make_pair( ppPattern, nnotes ) );
	}

	if ( notesForeground.size() > 0 ) {
		m_hoveredNotes.push_back( std::make_pair( m_pPattern, notesForeground ) );
	}

	for ( auto& [ _, nnotes ] : m_hoveredNotes ) {
		std::sort( nnotes.begin(), nnotes.end(), Note::compare );
	}
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

void PatternEditorPanel::addOrRemoveNotes( int nPosition, int nRow, int nKey,
										   int nOctave, bool bDoAdd,
										   bool bDoDelete, bool bIsNoteOff,
										   PatternEditor::AddNoteAction action ) {
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();
	if ( m_pPattern == nullptr ) {
		// No pattern selected.
		return;
	}

	if ( nPosition >= m_pPattern->getLength() ) {
		// Note would be beyond the active region of the current pattern.
		return;
	}

	auto row = getRowDB( nRow );
	if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
		DEBUGLOG( QString( "Empty row [%1]" ).arg( nRow ) );
		return;
	}

	std::vector< std::shared_ptr<Note> > oldNotes;
	int nNewKey = nKey;
	int nNewOctave = nOctave;
	if ( nKey == KEY_INVALID || nOctave == OCTAVE_INVALID ) {
		oldNotes =
			m_pPattern->findNotes( nPosition, row.nInstrumentID, row.sType );
		nNewKey = KEY_MIN;
		nNewOctave = OCTAVE_DEFAULT;
	}
	else {
		auto pOldNote = m_pPattern->findNote(
			nPosition, row.nInstrumentID, row.sType,
			static_cast<Note::Key>(nKey), static_cast<Note::Octave>(nOctave) );
		if ( pOldNote != nullptr ) {
			oldNotes.push_back( pOldNote );
		}
	}

	if ( oldNotes.size() > 0 && ! bDoDelete ) {
		// Found an old note, but we don't want to delete, so just return.
		return;
	} else if ( oldNotes.size() == 0 && ! bDoAdd ) {
		// No note there, but we don't want to add a new one, so return.
		return;
	}

	if ( oldNotes.size() == 0 ) {
		// Play back added notes.
		if ( Preferences::get_instance()->getHearNewNotes() &&
			  row.bMappedToDrumkit ) {
			auto pSelectedInstrument = getSelectedInstrument();
			if ( pSelectedInstrument != nullptr &&
				 pSelectedInstrument->hasSamples() ) {
				auto pNote2 = std::make_shared<Note>( pSelectedInstrument );
				pNote2->setKeyOctave( static_cast<Note::Key>(nKey),
									  static_cast<Note::Octave>(nOctave) );
				pNote2->setNoteOff( bIsNoteOff );
				Hydrogen::get_instance()->getAudioEngine()->getSampler()->
					noteOn( pNote2 );
			}
		}

		pHydrogenApp->pushUndoCommand(
			new SE_addOrRemoveNoteAction(
				nPosition,
				row.nInstrumentID,
				row.sType,
				m_nPatternNumber,
				LENGTH_ENTIRE_SAMPLE,
				VELOCITY_DEFAULT,
				PAN_DEFAULT,
				LEAD_LAG_DEFAULT,
				nNewKey,
				nNewOctave,
				PROBABILITY_DEFAULT,
				/* bIsDelete */ false,
				bIsNoteOff,
				action ) );
	}
	else {
		// delete notes
		pHydrogenApp->beginUndoMacro(
			pCommonStrings->getActionDeleteNotes() );
		for ( const auto& ppNote : oldNotes ) {
			pHydrogenApp->pushUndoCommand(
				new SE_addOrRemoveNoteAction(
					nPosition,
					row.nInstrumentID,
					row.sType,
					m_nPatternNumber,
					ppNote->getLength(),
					ppNote->getVelocity(),
					ppNote->getPan(),
					ppNote->getLeadLag(),
					ppNote->getKey(),
					ppNote->getOctave(),
					ppNote->getProbability(),
					/* bIsDelete */ true,
					ppNote->getNoteOff(),
					action ) );
		}
		pHydrogenApp->endUndoMacro();
	}
}


bool PatternEditorPanel::isUsingAdditionalPatterns(
	const std::shared_ptr<H2Core::Pattern> pPattern ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();

	if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ||
		 ( pPattern != nullptr && pPattern->isVirtual() ) ||
		 ( pHydrogen->getMode() == Song::Mode::Song &&
		   pHydrogen->isPatternEditorLocked() ) ) {
		return true;
	}

	return false;
}

void PatternEditorPanel::clearNotesInRow( int nRow, int nPattern, int nPitch,
										  bool bCut ) {
	if ( m_pPattern == nullptr ) {
		return;
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	std::shared_ptr<PatternList> pPatternList = nullptr;
	if ( nPattern != -1 ) {
		auto pPattern = pSong->getPatternList()->get( nPattern );
		if ( pPattern == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve pattern [%1]" )
					  .arg( nPattern ) );
			return;
		}
		pPatternList = std::make_shared<PatternList>();
		pPatternList->add( pPattern );
	}
	else {
		pPatternList = pSong->getPatternList();
	}

	const auto row = getRowDB( nRow );

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	if ( bCut ) {
		pHydrogenApp->beginUndoMacro( pCommonStrings->getActionCutAllNotes() );
	}
	else if ( nRow != -1 ) {
		pHydrogenApp->beginUndoMacro(
			QString( "%1 [%2]" )
			.arg( pCommonStrings->getActionClearAllNotesInRow() )
			.arg( nRow ) );
	}
	else {
		pHydrogenApp->beginUndoMacro( pCommonStrings->getActionClearAllNotes() );
	}

	for ( const auto& ppPattern : *pPatternList ) {
		if ( ppPattern != nullptr ) {
			std::vector< std::shared_ptr<Note> > notes;
			for ( const auto& [ _, ppNote ] : *ppPattern->getNotes() ) {
				if ( ppNote != nullptr &&
					 ppNote->getInstrumentId() == row.nInstrumentID &&
					 ppNote->getType() == row.sType &&
					 ( nPitch == PITCH_INVALID ||
					   ppNote->getTotalPitch() == nPitch ) ) {
					notes.push_back( ppNote );
				}
			}

			for ( const auto& ppNote : notes ) {
				pHydrogenApp->pushUndoCommand(
					new SE_addOrRemoveNoteAction(
						ppNote->getPosition(),
						ppNote->getInstrumentId(),
						ppNote->getType(),
						pSong->getPatternList()->index( ppPattern ),
						ppNote->getLength(),
						ppNote->getVelocity(),
						ppNote->getPan(),
						ppNote->getLeadLag(),
						ppNote->getKey(),
						ppNote->getOctave(),
						ppNote->getProbability(),
						/* bIsDelete */ true,
						ppNote->getNoteOff() ) );
			}
		}
	}
	pHydrogenApp->endUndoMacro();
}

QString PatternEditorPanel::FillNotesToQString( const FillNotes& fillNotes ) {
	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	switch ( fillNotes ) {
		case FillNotes::All:
			return pCommonStrings->getActionFillAllNotes();
		case FillNotes::EverySecond:
			return pCommonStrings->getActionFillEverySecondNote();
		case FillNotes::EveryThird:
			return pCommonStrings->getActionFillEveryThirdNote();
		case FillNotes::EveryFourth:
			return pCommonStrings->getActionFillEveryFourthNote();
		case FillNotes::EverySixth:
			return pCommonStrings->getActionFillEverySixthNote();
		case FillNotes::EveryEighth:
			return pCommonStrings->getActionFillEveryEighthNote();
		case FillNotes::EveryTwelfth:
			return pCommonStrings->getActionFillEveryTwelfthNote();
		case FillNotes::EverySixteenth:
			return pCommonStrings->getActionFillEverySixteenthNote();
		default:
			return QString( "Unknown fill option [%1]" )
				.arg( static_cast<int>(fillNotes) );
	}
}

void PatternEditorPanel::fillNotesInRow( int nRow, FillNotes every, int nPitch ) {
	if ( m_pPattern == nullptr ) {
		return;
	}

	int nBase;
	if ( m_bIsUsingTriplets ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	const int nResolution = 4 * MAX_NOTES * static_cast<int>(every) /
		( nBase * m_nResolution );

	const auto row = getRowDB( nRow );

	std::vector<int> notePositions;
	const auto notes = m_pPattern->getNotes();
	for ( int ii = 0; ii < m_pPattern->getLength(); ii += nResolution ) {
		bool bNoteAlreadyPresent = false;
		FOREACH_NOTE_CST_IT_BOUND_LENGTH( notes, it, ii, m_pPattern ) {
			auto ppNote = it->second;
			if ( ppNote != nullptr &&
				 ppNote->getInstrumentId() == row.nInstrumentID &&
				 ppNote->getType() == row.sType &&
				 ( nPitch == PITCH_INVALID ||
				   ppNote->getTotalPitch() == nPitch ) ) {
				bNoteAlreadyPresent = true;
				break;
			}
		}

		if ( ! bNoteAlreadyPresent ) {
			notePositions.push_back( ii );
		}
	}

	if ( notePositions.size() > 0 ) {
		auto pHydrogenApp = HydrogenApp::get_instance();
		const auto pCommonStrings = pHydrogenApp->getCommonStrings();

		int nKey = KEY_MIN;
		int nOctave = OCTAVE_DEFAULT;
		if ( nPitch != PITCH_INVALID ) {
			nKey = Note::pitchToKey( nPitch );
			nOctave = Note::pitchToOctave( nPitch );
		}

		pHydrogenApp->beginUndoMacro( FillNotesToQString( every ) );
		for ( int nnPosition : notePositions ) {
			addOrRemoveNotes( nnPosition, nRow, nKey, nOctave,
							  true /* bDoAdd */, false /* bDoDelete */,
							  false /* bIsNoteOff */ );
		}
		pHydrogenApp->endUndoMacro();
	}
}

void PatternEditorPanel::setTypeInRow( int nRow ) {
	const auto row = getRowDB( nRow );
	if ( row.bMappedToDrumkit ) {
		ERRORLOG( QString( "Row [%1] is mapped to the current drumkit. Please edit the drumkit to change types instead!" )
				  .arg( nRow ) );
		return;
	}

	const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	// Get all notes in line nRow.
	std::vector< std::shared_ptr<Note> > notes;
	for ( const auto& [ _, ppNote ] : *m_pPattern->getNotes() ) {
		if ( ppNote != nullptr && ppNote->getType() == row.sType &&
			 ppNote->getInstrumentId() == row.nInstrumentID ) {
			notes.push_back( ppNote );
		}
	}

	if ( notes.size() == 0 ) {
		// Nothing to do. All notes seem to have been deleted before triggering
		// this action.
		updateDB();
		updateEditors();
		return;
	}

	bool bIsOkPressed;
	const QString sNewType = QInputDialog::getText(
		nullptr, "Hydrogen", QString( "%1 [%2]" )
		.arg( pCommonStrings->getActionEditTypes() ).arg( nRow ),
		QLineEdit::Normal, row.sType, &bIsOkPressed );
	if ( ! bIsOkPressed ) {
		// Cancelled by the user.
		return;
	}

	if ( sNewType.isEmpty() ) {
		QMessageBox::critical( this, "Hydrogen",
							   pCommonStrings->getErrorEmptyType() );
		return;
	}

	// Changing a type is effectively moving the note to another row of the
	// DrumPatternEditor. This could result in overlapping notes at the same
	// position. To guard against this, select all adjusted notes to harness the
	// checkDeselectElements capabilities.
	getVisibleEditor()->clearSelection();

	auto pHydrogenApp = HydrogenApp::get_instance();
	pHydrogenApp->beginUndoMacro(
		QString( "%1 [%2]" )
		.arg( pHydrogenApp->getCommonStrings()->getActionEditTypes() )
		.arg( nRow ) );

	for ( const auto& ppNote : notes ) {
		pHydrogenApp->pushUndoCommand(
			new SE_editNotePropertiesAction(
				PatternEditor::Property::Type,
				getPatternNumber(),
				ppNote->getPosition(),
				EMPTY_INSTR_ID,
				ppNote->getInstrumentId(),
				sNewType,
				ppNote->getType(),
				ppNote->getVelocity(),
				ppNote->getVelocity(),
				ppNote->getPan(),
				ppNote->getPan(),
				ppNote->getLeadLag(),
				ppNote->getLeadLag(),
				ppNote->getProbability(),
				ppNote->getProbability(),
				ppNote->getLength(),
				ppNote->getLength(),
				ppNote->getKey(),
				ppNote->getKey(),
				ppNote->getOctave(),
				ppNote->getOctave() ) );
	}

	pHydrogenApp->endUndoMacro();

	updateDB();
	updateEditors();

	getVisibleEditor()->triggerStatusMessage(
		notes, PatternEditor::Property::Type );
}

void PatternEditorPanel::copyNotesFromRowOfAllPatterns( int nRow, int nPitch ) {
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		ERRORLOG( "Song not ready" );
		return;
	}

	const auto row = getRowDB( nRow );

	// Serialize & put to clipboard
	H2Core::XMLDoc doc;
	auto rootNode = doc.set_root( "serializedPatternList" );
	pSong->getPatternList()->saveTo(
		rootNode, row.nInstrumentID, row.sType, nPitch );

	const QString sSerialized = doc.toString();
	if ( sSerialized.isEmpty() ) {
		ERRORLOG( QString( "Unable to serialize pattern editor line [%1]" )
				  .arg( nRow ) );
		return;
	}

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText( sSerialized );
}

void PatternEditorPanel::cutNotesFromRowOfAllPatterns( int nRow, int nPitch ) {
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	copyNotesFromRowOfAllPatterns( nRow, nPitch );

	clearNotesInRow( nRow, -1, nPitch, true );
}

void PatternEditorPanel::pasteNotesToRowOfAllPatterns( int nRow, int nPitch ) {
	const auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	const auto row = getRowDB( nRow );
	if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
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

	const auto pPatternList = PatternList::loadFrom(
		rootNode, pSong->getDrumkit()->getExportName() );
	if ( pPatternList == nullptr ) {
		ERRORLOG( QString( "Unable to deserialized pattern list [%1]" )
				  .arg( sSerialized ) );
		return;
	}

	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	// Those patterns contain only notes of a single row.
	pHydrogenApp->beginUndoMacro( pCommonStrings->getActionPasteAllNotes() );
	for ( auto& ppPattern : *pPatternList ) {
		if ( ppPattern != nullptr ) {
			for ( auto& [ _, ppNote ] : *ppPattern->getNotes() ) {
				if ( ppNote != nullptr ) {
					pHydrogenApp->pushUndoCommand(
						new SE_addOrRemoveNoteAction(
							ppNote->getPosition(),
							row.nInstrumentID,
							row.sType,
							pPatternList->index( ppPattern ),
							ppNote->getLength(),
							ppNote->getVelocity(),
							ppNote->getPan(),
							ppNote->getLeadLag(),
							nPitch == PITCH_INVALID ? ppNote->getKey() :
							  Note::pitchToKey( nPitch ),
							nPitch == PITCH_INVALID ? ppNote->getOctave() :
							  Note::pitchToOctave( nPitch ),
							ppNote->getProbability(),
							/* bIsDelete */ false,
							ppNote->getNoteOff() ) );
				}
			}
		}
	}
	pHydrogenApp->endUndoMacro();
}
