/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: PatternEditorPanel.cpp,v 1.39 2005/07/06 10:36:09 comix Exp $
 *
 */


#include "PatternEditorPanel.h"
#include "gui/MainForm.h"
#include "gui/widgets/Button.h"
#include "gui/widgets/Fader.h"
#include "gui/Skin.h"
#include "gui/SongEditor/SongEditorPanel.h"

#include "lib/Preferences.h"
#include "lib/Hydrogen.h"

#include <cmath>

#include <qlabel.h>
#include <qcombobox.h>

PatternEditorPanel::PatternEditorPanel( QWidget *pParent )
// : QWidget( pParent, "PatternEditorPanel", Qt::WStyle_Tool )
 : QWidget( pParent, "PatternEditorPanel",  Qt::WStyle_DialogBorder)
 , Object("PatEditPanel")
 , m_notePropertiesMode( VELOCITY )
 , m_pPattern( NULL )
 , m_bEnablePatternResize( true )
{
	int nMaxHeight = 850;
	int nMaxWidth = 706;

	setMinimumSize( 200, 200 );
//	setMaximumSize( nMaxWidth, nMaxHeight );

	resize( nMaxWidth, 352);

	setCaption( trUtf8( "Pattern Editor" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );

	// Load background image
	string background_path = Skin::getImagePath() + string("/patternEditor/patternEditor_background.png");
	bool ok = m_backgroundPixmap.load(background_path.c_str());
	if ( ok == false ) {
		errorLog( "[PatternEditorPanel] Error loading pixmap" );
	}

	Preferences *pPref = ( Preferences::getInstance() );

// TOOLBAR

	QWidget *pPatternInfoPanel = new QWidget( this );
	pPatternInfoPanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/patternEditor/background_name.png" ).c_str() ) ) );
	pPatternInfoPanel->move( 5, 4 );
	pPatternInfoPanel->resize( 217, 43 );

	m_pPatternNumberLCD = new LCDDisplay( pPatternInfoPanel, LCDDigit::SMALL_BLUE, 4 );
	m_pPatternNumberLCD->move( 14, 21 );
	m_pPatternNumberLCD->setPaletteBackgroundColor( QColor( 58, 62, 72 ) );

	string sNext_on = Skin::getImagePath() + string("/lcd/LCDSpinBox_up_on.png");
	string sNext_off = Skin::getImagePath() + string("/lcd/LCDSpinBox_up_off.png");
	string sNext_over = Skin::getImagePath() + string("/lcd/LCDSpinBox_up_over.png");
	m_pNextPatternBtn = new Button( pPatternInfoPanel, QSize(16,8), sNext_on, sNext_off, sNext_over );
	m_pNextPatternBtn->move( 51, 20 );
	connect( m_pNextPatternBtn, SIGNAL( clicked(Button*) ), this, SLOT( nextPatternBtnClicked(Button*) ) );

	string sPrev_on = Skin::getImagePath() + string("/lcd/LCDSpinBox_down_on.png");
	string sPrev_off = Skin::getImagePath() + string("/lcd/LCDSpinBox_down_off.png");
	string sPrev_over = Skin::getImagePath() + string("/lcd/LCDSpinBox_down_over.png");
	m_pPrevPatternBtn = new Button( pPatternInfoPanel, QSize(16,8), sPrev_on, sPrev_off, sPrev_over );
	m_pPrevPatternBtn->move( 51, 29 );
	connect( m_pPrevPatternBtn, SIGNAL( clicked(Button*) ), this, SLOT( prevPatternBtnClicked(Button*) ) );



	m_pNameLCD = new LCDDisplay( pPatternInfoPanel, LCDDigit::SMALL_BLUE, 16 );
	m_pNameLCD->move( 70, 21 );
	m_pNameLCD->setPaletteBackgroundColor( QColor( 58, 62, 72 ) );


	QWidget *pPatternPropertiesPanel = new QWidget( this );
	pPatternPropertiesPanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/patternEditor/background_res.png" ).c_str() ) ) );
	pPatternPropertiesPanel->move( 222, 4 );
	pPatternPropertiesPanel->resize( 156, 43 );

	// PATTERN size
	m_pPatternSizeLCD = new LCDDisplay( pPatternPropertiesPanel, LCDDigit::SMALL_BLUE, 4 );
	m_pPatternSizeLCD->move( 14, 21 );
	m_pPatternSizeLCD->setPaletteBackgroundColor( QColor( 58, 62, 72 ) );

	string size_dropdown_on_path = Skin::getImagePath() + string("/patternEditor/btn_dropdown_on.png");
	string size_dropdown_off_path = Skin::getImagePath() + string( "/patternEditor/btn_dropdown_off.png");
	string size_dropdown_over_path = Skin::getImagePath() + string( "/patternEditor/btn_dropdown_over.png");
	sizeDropdownBtn = new Button( pPatternPropertiesPanel, QSize(13, 13), size_dropdown_on_path, size_dropdown_off_path, size_dropdown_over_path);
	sizeDropdownBtn->move( 51, 22);
	QToolTip::add( sizeDropdownBtn, trUtf8( "Select pattern size" ) );
	connect( sizeDropdownBtn, SIGNAL( clicked(Button*) ), this, SLOT( sizeDropdownBtnClicked(Button*) ) );

	m_pPatternSizePopup  = new QPopupMenu( m_pPatternSizeLCD, "m_pResolutionPopup" );
	for ( int i = 1; i <= 32; i++) {
		m_pPatternSizePopup->insertItem( QString( toString( i ).c_str() ), i );
	}
	connect( m_pPatternSizePopup, SIGNAL( activated(int) ), this, SLOT( patternSizeChanged(int) ) );

	// GRID resolution
	m_pResolutionLCD = new LCDDisplay( pPatternPropertiesPanel, LCDDigit::SMALL_BLUE, 7 );
	m_pResolutionLCD->move( 68, 21 );
	m_pResolutionLCD->setPaletteBackgroundColor( QColor( 58, 62, 72 ) );

	string res_dropdown_on_path = Skin::getImagePath() + string("/patternEditor/btn_dropdown_on.png");
	string res_dropdown_off_path = Skin::getImagePath() + string( "/patternEditor/btn_dropdown_off.png");
	string res_dropdown_over_path = Skin::getImagePath() + string( "/patternEditor/btn_dropdown_over.png");
	resDropdownBtn = new Button( pPatternPropertiesPanel, QSize(13, 13), res_dropdown_on_path, res_dropdown_off_path, res_dropdown_over_path);
	resDropdownBtn->move( 129, 22);
	QToolTip::add( resDropdownBtn, trUtf8( "Select grid resolution" ) );
	connect( resDropdownBtn, SIGNAL( clicked(Button*) ), this, SLOT( resDropdownBtnClicked(Button*) ) );

	m_pResolutionPopup  = new QPopupMenu( m_pResolutionLCD, "m_pResolutionPopup" );
	m_pResolutionPopup->insertItem( "4", 0 );
	m_pResolutionPopup->insertItem( "8", 1 );
	m_pResolutionPopup->insertItem( "16", 2 );
	m_pResolutionPopup->insertItem( "32", 3 );
	m_pResolutionPopup->insertItem( "64", 4 );
	m_pResolutionPopup->insertSeparator();
	m_pResolutionPopup->insertItem( "4T", 5 );
	m_pResolutionPopup->insertItem( "8T", 6 );
	m_pResolutionPopup->insertItem( "16T", 7 );
	m_pResolutionPopup->insertItem( "32T", 8 );
	m_pResolutionPopup->insertSeparator();
	m_pResolutionPopup->insertItem( "off", 9 );
	connect( m_pResolutionPopup, SIGNAL( activated(int) ), this, SLOT( gridResolutionChanged(int) ) );



	QWidget *pRecordingPanel = new QWidget( this );
	pRecordingPanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/patternEditor/background_rec.png" ).c_str() ) ) );
	pRecordingPanel->move( 378, 4 );
	pRecordingPanel->resize( 107, 43 );

	string hearBtn_on_path = Skin::getImagePath() + string("/patternEditor/btn_hear_on.png");
	string hearBtn_off_path = Skin::getImagePath() + string( "/patternEditor/btn_hear_off.png");
	string hearBtn_over_path = Skin::getImagePath() + string( "/patternEditor/btn_hear_over.png");
	hearNotesBtn = new ToggleButton( pRecordingPanel, QSize(21, 15), hearBtn_on_path, hearBtn_off_path, hearBtn_over_path);
	hearNotesBtn->move( 13, 17);
	QToolTip::add( hearNotesBtn, trUtf8( "Hear new notes" ) );
	connect( hearNotesBtn, SIGNAL(clicked(Button*)), this, SLOT( hearNotesBtnClick(Button*)));

	string recBtn_on_path = Skin::getImagePath() + string( "/patternEditor/btn_record_on.png");
	string recBtn_off_path = Skin::getImagePath() + string( "/patternEditor/btn_record_off.png");
	string recBtn_over_path = Skin::getImagePath() + string( "/patternEditor/btn_record_over.png");
	recordEventsBtn = new ToggleButton( pRecordingPanel, QSize(26, 17), recBtn_on_path, recBtn_off_path, recBtn_over_path);
	recordEventsBtn->move( 40, 17);
	recordEventsBtn->setPressed( pPref->getRecordEvents());
	QToolTip::add( recordEventsBtn, trUtf8( "Record keyboard/midi events" ) );
	connect( recordEventsBtn, SIGNAL(clicked(Button*)), this, SLOT( recordEventsBtnClick(Button*)));

	// quantize
	string quantBtn_on_path = Skin::getImagePath() + string( "/patternEditor/btn_quant_on.png");
	string quantBtn_off_path = Skin::getImagePath() + string( "/patternEditor/btn_quant_off.png");
	string quantBtn_over_path = Skin::getImagePath() + string( "/patternEditor/btn_quant_over.png");
	quantizeEventsBtn = new ToggleButton( pRecordingPanel, QSize(21, 15), quantBtn_on_path, quantBtn_off_path, quantBtn_over_path);
	quantizeEventsBtn->move( 72, 17);
	quantizeEventsBtn->setPressed( pPref->getQuantizeEvents());
	QToolTip::add( quantizeEventsBtn, trUtf8( "Quantize keyboard/midi events to grid" ) );
	connect( quantizeEventsBtn, SIGNAL(clicked(Button*)), this, SLOT( quantizeEventsBtnClick(Button*)));




	// ZOOM BUTTONS
	QWidget *pZoomPanel = new QWidget( this );
//	pZoomPanel ->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/patternEditor/background_rec.png" ).c_str() ) ) );
	pZoomPanel->setPaletteBackgroundColor( QColor( 100, 100, 200 ) );
	pZoomPanel ->move( 500, 4 );
	pZoomPanel ->resize( 200, 43 );
	pZoomPanel->hide();

	string sZoomIn_on_path = Skin::getImagePath() + string( "/patternEditor/quantBtn_on.png");
	string sZoomIn_off_path = Skin::getImagePath() + string( "/patternEditor/quantBtn_off.png");
	string sZoomIn_over_path = Skin::getImagePath() + string( "/patternEditor/quantBtn_over.png");
	Button *m_pZoomInBtn = new Button( pZoomPanel , QSize(20, 20), sZoomIn_on_path, sZoomIn_off_path, sZoomIn_over_path );
	QToolTip::add( m_pZoomInBtn, trUtf8( "Zoom in" ) );
	connect( m_pZoomInBtn, SIGNAL(clicked(Button*)), this, SLOT( zoomInBtnClicked(Button*) ) );

	string sZoomOut_on_path = Skin::getImagePath() + string( "/patternEditor/quantBtn_on.png");
	string sZoomOut_off_path = Skin::getImagePath() + string( "/patternEditor/quantBtn_off.png");
	string sZoomOut_over_path = Skin::getImagePath() + string( "/patternEditor/quantBtn_over.png");
	Button *m_pZoomOutBtn = new Button( pZoomPanel , QSize(20, 20), sZoomOut_on_path, sZoomOut_off_path, sZoomOut_over_path );
	QToolTip::add( m_pZoomOutBtn, trUtf8( "Zoom out" ) );
	connect( m_pZoomOutBtn, SIGNAL(clicked(Button*)), this, SLOT( zoomOutBtnClicked(Button*) ) );

	m_pZoomInBtn->move( 5, 5 );
	m_pZoomOutBtn->move( 50, 5 );
//	m_pZoomInBtn->hide();
//	m_pZoomOutBtn->hide();
	//~ ZOOM BUTTONS

//~ TOOLBAR



// instruments buttons
	QWidget *pChannelPanel = new QWidget( this );
	pChannelPanel->setPaletteBackgroundPixmap( QPixmap( QString( Skin::getImagePath().append( "/patternEditor/background_channel.png" ).c_str() ) ) );
	pChannelPanel->move( 5, 49 );
	pChannelPanel->resize( 97, 24 );

	string sMoveDown_on = Skin::getImagePath() + string( "/songEditor/btn_down_on.png" );
	string sMoveDown_off = Skin::getImagePath() + string( "/songEditor/btn_down_off.png" );
	string sMoveDown_over = Skin::getImagePath() + string( "/songEditor/btn_down_over.png" );
	m_pMoveDownBtn = new Button( pChannelPanel, QSize( 18, 13 ), sMoveDown_on, sMoveDown_off, sMoveDown_over );
	m_pMoveDownBtn->move( 30, 5 );
	QToolTip::add( m_pMoveDownBtn, trUtf8( "Move selected instrument down" ) );
	connect( m_pMoveDownBtn, SIGNAL(clicked(Button*)), this, SLOT( moveDownBtnClicked(Button*)));

	string sMoveUp_on = Skin::getImagePath() + string( "/songEditor/btn_up_on.png" );
	string sMoveUp_off = Skin::getImagePath() + string( "/songEditor/btn_up_off.png" );
	string sMoveUp_over = Skin::getImagePath() + string( "/songEditor/btn_up_over.png" );
	m_pMoveUpBtn = new Button( pChannelPanel, QSize( 18, 13 ), sMoveUp_on, sMoveUp_off, sMoveUp_over );
	m_pMoveUpBtn->move( 48, 5 );
	QToolTip::add( m_pMoveUpBtn, trUtf8( "Move selected instrument up" ) );
	connect( m_pMoveUpBtn, SIGNAL(clicked(Button*)), this, SLOT( moveUpBtnClicked(Button*)));


// RULER____________________________________

	// Ruler ScrollView
	m_pRulerScrollView = new QScrollView( this );
	m_pRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pRulerScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pRulerScrollView->setHScrollBarMode( QScrollView::AlwaysOff );

	// Ruler
	m_pPatternEditorRuler = new PatternEditorRuler( m_pRulerScrollView->viewport(), this );

	m_pRulerScrollView->addChild( m_pPatternEditorRuler );

//_________________________________________


// EDITOR _____________________________________
	// Editor scrollview
	m_pEditorScrollView = new QScrollView( this );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pEditorScrollView->setHScrollBarMode( QScrollView::AlwaysOff );

	// Editor
	m_pPatternEditor = new PatternEditor( m_pEditorScrollView->viewport(), this );

	m_pEditorScrollView->addChild( m_pPatternEditor );

	QScrollBar *pEditorVScroll = m_pEditorScrollView->verticalScrollBar();
	connect( pEditorVScroll , SIGNAL(valueChanged(int)), this, SLOT( contentsMoving(int) ) );
//_______________________________________________




	// INSTRUMENT LIST________________________________________
	// Instrument list scrollview
	m_pInstrListScrollView = new QScrollView( this );
	m_pInstrListScrollView->setFrameShape( QFrame::NoFrame );
	m_pInstrListScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pInstrListScrollView->setHScrollBarMode( QScrollView::AlwaysOff );

	// Instrument list
	m_pInstrumentList = new PatternEditorInstrumentList( m_pInstrListScrollView->viewport(), this );
	m_pInstrListScrollView->addChild( m_pInstrumentList );

	QScrollBar *pInstrListVScroll = m_pInstrListScrollView->verticalScrollBar();
	connect( pInstrListVScroll , SIGNAL(valueChanged(int)), this, SLOT( contentsMoving(int) ) );
	//____________________________________________________________





	// NOTE_VELOCITY EDITOR______________________________________________
	m_pNoteVelocityScrollView = new QScrollView( this );
	m_pNoteVelocityScrollView->setFrameShape( QFrame::NoFrame );
	m_pNoteVelocityScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pNoteVelocityScrollView->setHScrollBarMode( QScrollView::AlwaysOff );
	m_pNoteVelocityEditor = new NotePropertiesRuler( m_pNoteVelocityScrollView->viewport(), this, NotePropertiesRuler::VELOCITY );
	m_pNoteVelocityScrollView->addChild( m_pNoteVelocityEditor );
	//_____________________________________________________________

	// NOTE_PITCH EDITOR______________________________________________
	m_pNotePitchScrollView = new QScrollView( this );
	m_pNotePitchScrollView->setFrameShape( QFrame::NoFrame );
	m_pNotePitchScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pNotePitchScrollView->setHScrollBarMode( QScrollView::AlwaysOff );
	m_pNotePitchEditor = new NotePropertiesRuler( m_pNotePitchScrollView->viewport(), this, NotePropertiesRuler::PITCH );
	m_pNotePitchScrollView->addChild( m_pNotePitchEditor );
	//_____________________________________________________________





	// external horizontal scrollbar
	m_pPatternEditorHScrollBar = new QScrollBar( this, "m_pPatternEditorHScrollBar" );
	m_pPatternEditorHScrollBar->setOrientation( Qt::Horizontal );
	connect( m_pPatternEditorHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	// external vertical scrollbar
	m_pPatternEditorVScrollBar = new QScrollBar( this, "m_pPatternEditorVScrollBar" );
	m_pPatternEditorVScrollBar->setOrientation( Qt::Vertical );
	connect( m_pPatternEditorVScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalHorizontalScrollbar(int) ) );

	// restore grid resolution
	int nIndex;
	if ( pPref->isPatternEditorUsingTriplets() == false ) {
		switch ( pPref->getPatternEditorGridResolution() ) {
			case 4:
				m_pResolutionLCD->setText( "4" );
				nIndex = 0;
				break;

			case 8:
				m_pResolutionLCD->setText( "8" );
				nIndex = 1;
				break;

			case 16:
				m_pResolutionLCD->setText( "16" );
				nIndex = 2;
				break;

			case 32:
				m_pResolutionLCD->setText( "32" );
				nIndex = 3;
				break;

			case 64:
				m_pResolutionLCD->setText( "64" );
				nIndex = 4;
				break;

			default:
				warningLog( "Wrong grid resolution: " + toString( pPref->getPatternEditorGridResolution() ) );
				m_pResolutionLCD->setText( "4" );
				nIndex = 0;
		}
	}
	else {
		switch ( pPref->getPatternEditorGridResolution() ) {
			case 8:
				m_pResolutionLCD->setText( "4T" );
				nIndex = 5;
				break;

			case 16:
				m_pResolutionLCD->setText( "8T" );
				nIndex = 6;
				break;

			case 32:
				m_pResolutionLCD->setText( "16T" );
				nIndex = 7;
				break;

			case 64:
				m_pResolutionLCD->setText( "32T" );
				nIndex = 8;
				break;

			default:
				warningLog( "Wrong grid resolution: " + toString( pPref->getPatternEditorGridResolution() ) );
				m_pResolutionLCD->setText( "4T" );
				nIndex = 5;
		}
	}
	gridResolutionChanged( nIndex );

	// restore hear new notes button state
	hearNotesBtn->setPressed( pPref->getHearNewNotes() );



	// show velocity button
	string sVelocityBtn_on_path = Skin::getImagePath() + string( "/patternEditor/velBtn_on.png");
	string sVelocityBtn_off_path = Skin::getImagePath() + string( "/patternEditor/velBtn_off.png");
	string sVelocityBtn_over_path = Skin::getImagePath() + string( "/patternEditor/velBtn_over.png");
	m_pShowVelocityBtn = new ToggleButton(this, QSize(40, 20), sVelocityBtn_on_path, sVelocityBtn_off_path, sVelocityBtn_over_path);
	m_pShowVelocityBtn->move( 10, 600);
	m_pShowVelocityBtn->setPressed( true );
	QToolTip::add( m_pShowVelocityBtn, trUtf8( "Show velocity editor" ) );
	connect( m_pShowVelocityBtn, SIGNAL(clicked(Button*)), this, SLOT( showVelocityBtnClick(Button*)));

	// show pitch button
	string sPitchBtn_on_path = Skin::getImagePath() + string( "/patternEditor/pianoBtn_on.png");
	string sPitchBtn_off_path = Skin::getImagePath() + string( "/patternEditor/pianoBtn_off.png");
	string sPitchBtn_over_path = Skin::getImagePath() + string( "/patternEditor/pianoBtn_over.png");
	m_pShowPitchBtn = new ToggleButton(this, QSize(40, 20), sPitchBtn_on_path, sPitchBtn_off_path, sPitchBtn_over_path);
	m_pShowPitchBtn->move( 10, 630);
	m_pShowPitchBtn->setPressed( false );
	QToolTip::add( m_pShowPitchBtn, trUtf8( "Show pitch editor" ) );
	connect( m_pShowPitchBtn, SIGNAL(clicked(Button*)), this, SLOT( showPitchBtnClick(Button*)));

	if (!pPref->m_bUsePitchEditor) {
		m_pShowPitchBtn->hide();
		m_pShowVelocityBtn->hide();
	}


	this->setPaletteBackgroundPixmap( m_backgroundPixmap );

	setupPattern();

	HydrogenApp::getInstance()->addEventListener( this );

	selectedPatternChangedEvent(); // force an update
}




PatternEditorPanel::~PatternEditorPanel()
{
	hide();

	delete m_pPatternEditor;
	delete m_pPatternEditorRuler;
	delete m_pInstrumentList;
	delete m_pNoteVelocityEditor;
}



void PatternEditorPanel::setupPattern()
{
	if (m_pPattern) {
		string sCurrentPatternName = m_pPattern->m_sName;
		this->setCaption( ( trUtf8( "Pattern editor - %1").arg(sCurrentPatternName.c_str()) ) );
		m_pNameLCD->setText( sCurrentPatternName );
		m_pPatternNumberLCD->setText( toString( Hydrogen::getInstance()->getSelectedPatternNumber() ) );

		// update pattern size combobox
		int nPatternSize = m_pPattern->m_nSize;
		int nEighth = MAX_NOTES / 8;

		for ( int i = 1; i <= 32; i++ ) {
			if ( nPatternSize == nEighth * i ) {
				m_pPatternSizeLCD->setText( toString( i ) );
				break;
			}
		}
	}
	else {
		this->setCaption( ( trUtf8( "Pattern editor - %1").arg(QString("NULL")) ) );
		m_pNameLCD->setText( "-" );
	}
}



void PatternEditorPanel::syncToExternalHorizontalScrollbar(int)
{
	int nValue;

	// Editor
	nValue = m_pPatternEditorHScrollBar->value();
	QScrollBar *pEditorHScroll = m_pEditorScrollView->horizontalScrollBar();
	if (pEditorHScroll->value() != nValue ) {
		pEditorHScroll->setValue( nValue );
	}

	nValue = m_pPatternEditorVScrollBar->value();
	QScrollBar *pEditorVScroll = m_pEditorScrollView->verticalScrollBar();
	if (pEditorVScroll->value() != nValue) {
		pEditorVScroll->setValue( nValue );
	}


	// Ruler
	nValue = m_pPatternEditorHScrollBar->value();
	QScrollBar *pRulerHScroll = m_pRulerScrollView->horizontalScrollBar();
	if ( pRulerHScroll->value() != nValue ) {
		pRulerHScroll->setValue( nValue );
	}

	// Instrument list
	nValue = m_pPatternEditorVScrollBar->value();
	QScrollBar *pInstrListVScroll = m_pInstrListScrollView->verticalScrollBar();
	if (pInstrListVScroll->value() != nValue ) {
		pInstrListVScroll->setValue( nValue );
	}

	// Velocity ruler
	nValue = m_pPatternEditorHScrollBar->value();
	QScrollBar *pVelocityRulerHScroll = m_pNoteVelocityScrollView->horizontalScrollBar();
	if ( pVelocityRulerHScroll->value() != nValue ) {
		pVelocityRulerHScroll->setValue( nValue );
	}
}




void PatternEditorPanel::gridResolutionChanged( int nSelected )
{
	int nResolution;
	bool bUseTriplets = false;
	switch ( nSelected ) {
		case 0:
			nResolution = 4;
			m_pResolutionLCD->setText( "4" );
			break;

		case 1:
			nResolution = 8;
			m_pResolutionLCD->setText( "8" );
			break;

		case 2:
			nResolution = 16;
			m_pResolutionLCD->setText( "16" );
			break;

		case 3:
			nResolution = 32;
			m_pResolutionLCD->setText( "32" );
			break;

		case 4:
			nResolution = 64;
			m_pResolutionLCD->setText( "64" );
			break;

		case 5:
			nResolution = 8;
			bUseTriplets = true;
			m_pResolutionLCD->setText( "4T" );
			break;

		case 6:
			nResolution = 16;
			bUseTriplets = true;
			m_pResolutionLCD->setText( "8T" );
			break;

		case 7:
			nResolution = 32;
			bUseTriplets = true;
			m_pResolutionLCD->setText( "16T" );
			break;

		case 8:
			nResolution = 64;
			bUseTriplets = true;
			m_pResolutionLCD->setText( "32T" );
			break;

		case 9:
			nResolution = MAX_NOTES;
			m_pResolutionLCD->setText( "off" );
			break;

		default:
			nResolution = 4;
			m_pResolutionLCD->setText( "4" );
	}

	m_pPatternEditor->setResolution( nResolution, bUseTriplets );

	( Preferences::getInstance() )->setPatternEditorGridResolution( nResolution );
	( Preferences::getInstance() )->setPatternEditorUsingTriplets( bUseTriplets );
}



void PatternEditorPanel::updateStart( bool bStart )
{
	m_pPatternEditorRuler->updateStart( bStart );
}




/// This method is called from another thread (audio engine)
void PatternEditorPanel::patternModifiedEvent()
{
	m_pPatternEditor->updateEditor(true);
}



void PatternEditorPanel::selectedPatternChangedEvent()
{
	infoLog( "[selectedPatternChangedEvent]" );
	// full update of display
	m_pPatternEditor->updateEditor(true);
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePitchEditor->updateEditor();
	m_pPatternEditorRuler->updateEditor(true);

	Hydrogen *pEngine = Hydrogen::getInstance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->getSize() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}
	setupPattern();
}





void PatternEditorPanel::sizeDropdownBtnClicked( Button* pRef)
{
	m_pPatternSizePopup->popup( m_pPatternSizeLCD->mapToGlobal( QPoint( 1, m_pPatternSizeLCD->height() + 2 ) ) );
}


void PatternEditorPanel::resDropdownBtnClicked( Button* pRef)
{
	m_pResolutionPopup->popup( m_pResolutionLCD->mapToGlobal( QPoint( 1, m_pResolutionLCD->height() + 2 ) ) );
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
//	infoLog( "[resizeEvent]" );

	static const int nBorder = 5;
	
	int nEditorWidth = width() - 120 - nBorder;
	int nInstrument_X = nBorder;
	int nEditor_X = nInstrument_X + 100;
	int nEditor_Y = 76;

	int nNoteEditor_Y;
	if ( m_notePropertiesMode == VELOCITY ) {
		nNoteEditor_Y = height() - m_pNoteVelocityEditor->height() - 16;

		// Note Velocity Editor
		m_pNoteVelocityScrollView->move( nEditor_X, nNoteEditor_Y );
		m_pNoteVelocityScrollView->resize( nEditorWidth, m_pNoteVelocityEditor->height() );

		m_pNoteVelocityScrollView->show();
		m_pNotePitchScrollView->hide();
	}
	else {
		nNoteEditor_Y = height() - m_pNotePitchEditor->height() - 16;

		// Note Pitch Editor
		m_pNotePitchScrollView->move( nEditor_X, nNoteEditor_Y );
		m_pNotePitchScrollView->resize( nEditorWidth, m_pNotePitchEditor->height() );

		m_pNoteVelocityScrollView->hide();
		m_pNotePitchScrollView->show();
	}


	m_pEditorScrollView->resize( nEditorWidth, nNoteEditor_Y - nEditor_Y );
	m_pEditorScrollView->move( nEditor_X, nEditor_Y );

	m_pRulerScrollView->resize( nEditorWidth, m_pRulerScrollView->height() );
	m_pRulerScrollView->move( nEditor_X, 49 );


	// Instrument list
	m_pInstrListScrollView->move( nInstrument_X, nEditor_Y );
	m_pInstrListScrollView->resize( m_pInstrListScrollView->width(), m_pEditorScrollView->height() );



	//
	m_pShowVelocityBtn->move( 10, nNoteEditor_Y + 10 );
	m_pShowPitchBtn->move( 10, nNoteEditor_Y + 10 + 30);


	// pattern editor Horizontal scroll bar
	m_pPatternEditorHScrollBar->resize( nEditorWidth, 16 );
	m_pPatternEditorHScrollBar->move( nEditor_X, height() - 16 );
	m_pPatternEditorHScrollBar->setMinValue( 0 );
	m_pPatternEditorHScrollBar->setMaxValue( m_pPatternEditor->width() - m_pEditorScrollView->viewport()->width() );
	m_pPatternEditorHScrollBar->setLineStep( 20 );
	m_pPatternEditorHScrollBar->setPageStep( 300 );

	// pattern editor Vertical scroll bar
	m_pPatternEditorVScrollBar->resize( 16, m_pEditorScrollView->height() );
	m_pPatternEditorVScrollBar->move( width() - 16 - nBorder, m_pEditorScrollView->y() );
	m_pPatternEditorVScrollBar->setMinValue( 0 );
	m_pPatternEditorVScrollBar->setMaxValue( m_pPatternEditor->height() - m_pEditorScrollView->height() );
	m_pPatternEditorVScrollBar->setLineStep( 20 );
	m_pPatternEditorVScrollBar->setPageStep( 300 );
}




void PatternEditorPanel::showEvent ( QShowEvent *ev )
{
	m_pPatternEditorVScrollBar->setValue( m_pPatternEditorVScrollBar->maxValue() );
}


/// richiamato dall'uso dello scroll del mouse
void PatternEditorPanel::contentsMoving(int dummy)
{
	syncToExternalHorizontalScrollbar(0);
}




void PatternEditorPanel::setSelectedInstrument( int nInstr )
{
	//infoLog( "[setSelectedInstrument]" + toString( nInstr ) );
	Hydrogen::getInstance()->setSelectedInstrumentNumber( nInstr );

	m_pPatternEditor->updateEditor(true);	// force repaint
	m_pInstrumentList->updateEditor();
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePitchEditor->updateEditor();
}



void PatternEditorPanel::selectedInstrumentChangedEvent()
{
	// aggiorno tutti i componenti
	m_pPatternEditor->updateEditor(true);	// force repaint
	m_pInstrumentList->updateEditor();
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePitchEditor->updateEditor();
}



void PatternEditorPanel::showVelocityBtnClick(Button *ref)
{
	m_notePropertiesMode = VELOCITY;
	m_pShowVelocityBtn->setPressed( true );
	m_pShowPitchBtn->setPressed( false );

	resizeEvent( NULL );
}



void PatternEditorPanel::showPitchBtnClick(Button *ref)
{
	m_notePropertiesMode = PITCH;
	m_pShowPitchBtn->setPressed( true );
	m_pShowVelocityBtn->setPressed( false );

	resizeEvent( NULL );
}



void PatternEditorPanel::zoomInBtnClicked(Button *ref)
{
	m_pPatternEditorRuler->zoomIn();
	m_pPatternEditor->zoomIn();
	m_pNoteVelocityEditor->zoomIn();
	m_pNotePitchEditor->zoomIn();

	resizeEvent( NULL );
}



void PatternEditorPanel::zoomOutBtnClicked(Button *ref)
{
	m_pPatternEditorRuler->zoomOut();
	m_pPatternEditor->zoomOut();
	m_pNoteVelocityEditor->zoomOut();
	m_pNotePitchEditor->zoomOut();

	resizeEvent( NULL );
}



void PatternEditorPanel::patternSizeChanged( int nSelected )
{
	if ( !m_bEnablePatternResize ) {
		QMessageBox::information( this, "Hydrogen", trUtf8( "Is not possible to change the pattern size when playing." ) );
		return;
	}

	if ( !m_pPattern ) {
		return;
	}

	uint nEighth = MAX_NOTES / 8;

	if ( nSelected > 0 && nSelected <= 32 ) {
		m_pPattern->m_nSize = nEighth * nSelected;
		m_pPatternSizeLCD->setText( toString( nSelected ) );
	}
	else {
		errorLog( "[patternSizeChanged] Unhandled case " + toString( nSelected ) );
	}

	resizeEvent( NULL ); // force a resize
	m_pPatternEditorRuler->updateEditor( true );	// redraw all
	m_pPatternEditor->updateEditor( true );
	m_pNoteVelocityEditor->updateEditor();
	m_pNotePitchEditor->updateEditor();
}


void PatternEditorPanel::nextPatternBtnClicked(Button*)
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	int nPattern = pEngine->getSelectedPatternNumber() + 1;

	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	if ( nPattern >= (int)pPatternList->getSize() ) {
		return;
	}
	pEngine->setSelectedPatternNumber( nPattern );

	if ( pSong->getMode() == Song::PATTERN_MODE ) {
		PatternList *pCurrentPatternList = pEngine->getCurrentPatternList();
		if ( pCurrentPatternList->getSize() == 0 ) {
			// nessun pattern e' attivo. seleziono subito questo.
			pCurrentPatternList->add( pPatternList->get( nPattern ) );
		}
		else {
			pEngine->setNextPattern( nPattern );
		}
	}
	HydrogenApp::getInstance()->getSongEditorPanel()->updateAll();
}

void PatternEditorPanel::prevPatternBtnClicked(Button*)
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	int nPattern = pEngine->getSelectedPatternNumber() - 1;
	if ( nPattern < 0 ) {
		return;
	}

	Song *pSong = pEngine->getSong();
	PatternList *pPatternList = pSong->getPatternList();

	if ( nPattern >= (int)pPatternList->getSize() ) {
		return;
	}
	pEngine->setSelectedPatternNumber( nPattern );

	if ( pSong->getMode() == Song::PATTERN_MODE ) {
		PatternList *pCurrentPatternList = pEngine->getCurrentPatternList();
		if ( pCurrentPatternList->getSize() == 0 ) {
			// nessun pattern e' attivo. seleziono subito questo.
			pCurrentPatternList->add( pPatternList->get( nPattern ) );
		}
		else {
			pEngine->setNextPattern( nPattern );
		}
	}
	HydrogenApp::getInstance()->getSongEditorPanel()->updateAll();
}


void PatternEditorPanel::moveDownBtnClicked(Button *)
{
	Hydrogen *engine = Hydrogen::getInstance();
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	engine->lockEngine("PatternEditorPanel::moveDownBtnClicked");
	Song *pSong = engine->getSong();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();

	if ( ( nSelectedInstrument - 1 ) >= 0 ) {
		Instrument *pTemp = pInstrumentList->get( nSelectedInstrument - 1 );
		pInstrumentList->replace( pInstrumentList->get( nSelectedInstrument ), nSelectedInstrument - 1 );
		pInstrumentList->replace( pTemp, nSelectedInstrument );

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

		engine->unlockEngine();
		engine->setSelectedInstrumentNumber( nSelectedInstrument - 1 );

		pSong->m_bIsModified = true;
	}
	else {
		engine->unlockEngine();
	}
}


void PatternEditorPanel::moveUpBtnClicked(Button *)
{
	Hydrogen *engine = Hydrogen::getInstance();
	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	engine->lockEngine("PatternEditorPanel::moveUpBtnClicked");
	Song *pSong = engine->getSong();
	InstrumentList *pInstrumentList = pSong->getInstrumentList();

	if ( ( nSelectedInstrument + 1 ) < pInstrumentList->getSize() ) {
		Instrument *pTemp = pInstrumentList->get( nSelectedInstrument + 1 );
		pInstrumentList->replace( pInstrumentList->get( nSelectedInstrument ), nSelectedInstrument + 1 );
		pInstrumentList->replace( pTemp, nSelectedInstrument );

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

		engine->unlockEngine();
		engine->setSelectedInstrumentNumber( nSelectedInstrument + 1 );

		pSong->m_bIsModified = true;
	}
	else {
		engine->unlockEngine();
	}
}






