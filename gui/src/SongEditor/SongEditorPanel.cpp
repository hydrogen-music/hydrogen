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
#include "SongEditorPanel.h"

#include "../HydrogenApp.h"
#include "../PatternPropertiesDialog.h"
#include "../SongPropertiesDialog.h"
#include "../widgets/Button.h"
#include "../widgets/PixmapWidget.h"
#include "../Skin.h"

#include "SongEditor.h"

#include <hydrogen/hydrogen.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/audio_engine.h>
#include <hydrogen/Pattern.h>
using namespace H2Core;
using namespace std;

SongEditorPanel::SongEditorPanel(QWidget *pParent)
 : QWidget( pParent )
 , Object( "SongEditorPanel" )
 , m_actionMode( DRAW_ACTION )
{
	m_nInitialWidth = 600;
	m_nInitialHeight = 250;

	setWindowTitle( trUtf8( "Song Editor" ) );

	// background
	PixmapWidget *pBackPanel = new PixmapWidget( NULL );
	pBackPanel->setFixedSize( 196, 49 );
	pBackPanel->setPixmap( "/songEditor/bg_topPanel.png" );

	// time line toggle button
	m_pTimeLineToggleBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_bpm_on.png",
			"/songEditor/btn_bpm_off.png",
			"/songEditor/btn_bpm_over.png",
			QSize( 54, 13 )
	);
	m_pTimeLineToggleBtn->move( 133, 6 );
	m_pTimeLineToggleBtn->setToolTip( trUtf8( "Enable time line edit") );
	connect( m_pTimeLineToggleBtn, SIGNAL( clicked( Button* ) ), this, SLOT( timeLineBtnPressed(Button* ) ) );
	m_pTimeLineToggleBtn->setPressed( Preferences::get_instance()->__usetimeline );


	// clear sequence button
	m_pClearPatternSeqBtn = new Button(
			pBackPanel,
			"/songEditor/btn_clear_on.png",
			"/songEditor/btn_clear_off.png",
			"/songEditor/btn_clear_over.png",
			QSize(53,13)
	);
	m_pClearPatternSeqBtn->move( 6, 5 + 25 );
	m_pClearPatternSeqBtn->setToolTip( trUtf8("Clear pattern sequence") );
	connect( m_pClearPatternSeqBtn, SIGNAL( clicked( Button* ) ), this, SLOT( clearSequence(Button*) ) );

	// new pattern button
	Button *newPatBtn = new Button(
			pBackPanel,
			"/songEditor/btn_new_on.png",
			"/songEditor/btn_new_off.png",
			"/songEditor/btn_new_over.png",
			QSize(19, 13)
	);
	newPatBtn->move( 64, 5 + 25);
	newPatBtn->setToolTip( trUtf8("Create new pattern") );
	connect( newPatBtn, SIGNAL( clicked( Button* ) ), this, SLOT( newPatBtnClicked( Button* ) ) );

	// down button
	m_pDownBtn = new Button(
			pBackPanel,
			"/songEditor/btn_down_on.png",
			"/songEditor/btn_down_off.png",
			"/songEditor/btn_down_over.png",
			QSize(18,13)
	);
	m_pDownBtn->move( 89, 5 + 25);
	m_pDownBtn->setToolTip( trUtf8("Move the selected pattern down") );
	connect( m_pDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT( downBtnClicked( Button* ) ) );

	// up button
	m_pUpBtn = new Button(
			pBackPanel,
			"/songEditor/btn_up_on.png",
			"/songEditor/btn_up_off.png",
			"/songEditor/btn_up_over.png",
			QSize(18,13)
	);
	m_pUpBtn->move( 106, 5 + 25 );
	m_pUpBtn->setToolTip( trUtf8("Move the selected pattern up") );
	connect( m_pUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT( upBtnClicked( Button* ) ) );

	// select toggle button
	m_pPointerActionBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_select_on.png",
			"/songEditor/btn_select_off.png",
			"/songEditor/btn_select_over.png",
			QSize( 18, 13 )
	);
	m_pPointerActionBtn->move( 128, 5 + 25 );
	m_pPointerActionBtn->setToolTip( trUtf8( "Select mode" ) );
	connect( m_pPointerActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( pointerActionBtnPressed(Button*) ) );

	// draw toggle button
	m_pDrawActionBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_draw_on.png",
			"/songEditor/btn_draw_off.png",
			"/songEditor/btn_draw_over.png",
			QSize( 18, 13 )
	);
	m_pDrawActionBtn->move( 147, 5 + 25 );
	m_pDrawActionBtn->setToolTip( trUtf8( "Draw mode") );
	connect( m_pDrawActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( drawActionBtnPressed(Button* ) ) );
	m_pDrawActionBtn->setPressed( true );

	m_pModeActionBtn = new ToggleButton(
			pBackPanel,
			"/songEditor/btn_mode_on.png",
			"/songEditor/btn_mode_off.png",
			"/songEditor/btn_mode_over.png",
			QSize( 18, 13 )
	);
	m_pModeActionBtn->move( 169, 5 + 25 );
	m_pModeActionBtn->setToolTip( trUtf8( "stacked mode") );
	m_pModeActionBtn->setPressed(  Preferences::get_instance()->patternModePlaysSelected() );
	connect( m_pModeActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( modeActionBtnPressed() ) );

// ZOOM
	m_pHScrollBar = new QScrollBar( Qt::Horizontal,NULL );
	connect( m_pHScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalScrollBar() ) );

	// zoom-in btn
	Button* pZoomInBtn = new Button(
			NULL,
			"/songEditor/btn_new_on.png",
			"/songEditor/btn_new_off.png",
			"/songEditor/btn_new_over.png",
			QSize( 19, 13 )
	);
	connect( pZoomInBtn, SIGNAL( clicked( Button* ) ), this, SLOT( zoomOutBtnPressed(Button* ) ) );



	// zoom-out btn
	Button* pZoomOutBtn = new Button(
			NULL,
			"/songEditor/btn_minus_on.png",
			"/songEditor/btn_minus_off.png",
			"/songEditor/btn_minus_over.png",
			QSize( 19, 13 )
	);
	connect( pZoomOutBtn, SIGNAL( clicked( Button* ) ), this, SLOT( zoomInBtnPressed(Button* ) ) );

	QHBoxLayout *pHZoomLayout = new QHBoxLayout();
	pHZoomLayout->setSpacing( 0 );
	pHZoomLayout->setMargin( 0 );
	pHZoomLayout->addWidget( m_pHScrollBar );
	pHZoomLayout->addWidget( pZoomInBtn );
	pHZoomLayout->addWidget( pZoomOutBtn );

	QWidget *pHScrollbarPanel = new QWidget();
	pHScrollbarPanel->setLayout( pHZoomLayout );

//~ ZOOM

	// PATTERN LIST
	m_pPatternListScrollView = new QScrollArea( NULL );
	m_pPatternListScrollView->setFrameShape( QFrame::NoFrame );
	m_pPatternListScrollView->setFixedWidth( m_nPatternListWidth );
	m_pPatternListScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPatternListScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	connect( m_pPatternListScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_patternListScroll() ) );

	m_pPatternList = new SongEditorPatternList( m_pPatternListScrollView->viewport() );
	m_pPatternListScrollView->setWidget( m_pPatternList );


	// EDITOR
	m_pEditorScrollView = new QScrollArea( NULL );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pEditorScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pSongEditor = new SongEditor( m_pEditorScrollView->viewport() );
	m_pEditorScrollView->setWidget( m_pSongEditor );

	connect( m_pEditorScrollView->horizontalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_EditorScroll() ) );
	connect( m_pEditorScrollView->verticalScrollBar(), SIGNAL( valueChanged(int) ), this, SLOT( on_EditorScroll() ) );


	// POSITION RULER
	m_pPositionRulerScrollView = new QScrollArea( NULL );
	m_pPositionRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pPositionRulerScrollView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRulerScrollView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
	m_pPositionRuler = new SongEditorPositionRuler( m_pPositionRulerScrollView->viewport() );
	m_pPositionRulerScrollView->setWidget( m_pPositionRuler );
	m_pPositionRulerScrollView->setFixedHeight( 50 );

	m_pVScrollBar = new QScrollBar( Qt::Vertical, NULL );
	connect( m_pVScrollBar, SIGNAL(valueChanged(int)), this, SLOT( syncToExternalScrollBar() ) );



	// ok...let's build the layout
	QGridLayout *pGridLayout = new QGridLayout();
	pGridLayout->setSpacing( 0 );
	pGridLayout->setMargin( 0 );

	pGridLayout->addWidget( pBackPanel, 0, 0 );
	pGridLayout->addWidget( m_pPositionRulerScrollView, 0, 1 );
	pGridLayout->addWidget( m_pPatternListScrollView, 1, 0 );
	pGridLayout->addWidget( m_pEditorScrollView, 1, 1 );
	pGridLayout->addWidget( m_pVScrollBar, 1, 2 );
	//pGridLayout->addWidget( m_pHScrollBar, 2, 1 );
	pGridLayout->addWidget( pHScrollbarPanel, 2, 1 );



	this->setLayout( pGridLayout );
	QPalette defaultPalette;
	defaultPalette.setColor( QPalette::Background, QColor( 58, 62, 72 ) );
	this->setPalette( defaultPalette );


	show();

	updateAll();

	HydrogenApp::get_instance()->addEventListener( this );

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT( updatePlayHeadPosition() ) );
	m_pTimer->start(100);
}



SongEditorPanel::~SongEditorPanel()
{
	m_pTimer->stop();
}



void SongEditorPanel::updatePlayHeadPosition()
{
	Song *pSong = Hydrogen::get_instance()->getSong();

	if ( Preferences::get_instance()->m_bFollowPlayhead && pSong->get_mode() == Song::SONG_MODE) {
		if ( Hydrogen::get_instance()->getState() != STATE_PLAYING ) {
			return;
		}

		QPoint pos = m_pPositionRuler->pos();
		int x = -pos.x();
		int w = m_pPositionRulerScrollView->viewport()->width();

//		int x = m_pPositionRulerScrollView->contentsX();
//		int w = m_pPositionRulerScrollView->visibleWidth();
		int nPlayHeadPosition = Hydrogen::get_instance()->getPatternPos() * m_pSongEditor->getGridWidth();

		if ( nPlayHeadPosition > ( x + w - 50 ) ) {
			m_pEditorScrollView->horizontalScrollBar()->setValue( m_pEditorScrollView->horizontalScrollBar()->value() + 100 );
			on_EditorScroll();	// force a re-sync
		}
		else if ( nPlayHeadPosition < x ) {
			m_pEditorScrollView->horizontalScrollBar()->setValue( m_pEditorScrollView->horizontalScrollBar()->value() - 100 );
			on_EditorScroll();	// force a re-sync
		}
	}
}



void SongEditorPanel::on_patternListScroll()
{
	m_pEditorScrollView->verticalScrollBar()->setValue( m_pPatternListScrollView->verticalScrollBar()->value() );
}



///
/// Synchronize the patternlist with the patternsequence
///
void SongEditorPanel::on_EditorScroll()
{
	resyncExternalScrollBar();
	m_pPatternListScrollView->verticalScrollBar()->setValue( m_pEditorScrollView->verticalScrollBar()->value() );
	m_pPositionRulerScrollView->horizontalScrollBar()->setValue( m_pEditorScrollView->horizontalScrollBar()->value() );
}



void SongEditorPanel::syncToExternalScrollBar()
{
	m_pEditorScrollView->horizontalScrollBar()->setValue( m_pHScrollBar->value() );
	m_pEditorScrollView->verticalScrollBar()->setValue( m_pVScrollBar->value() );
}



///
/// Update and redraw all...
///
void SongEditorPanel::updateAll()
{
	m_pPatternList->createBackground();
	m_pPatternList->update();

	m_pSongEditor->cleanUp();

	m_pSongEditor->createBackground();
	m_pSongEditor->update();

	resyncExternalScrollBar();
}


void SongEditorPanel::updatePositionRuler()
{
	m_pPositionRuler->createBackground();
}

///
/// Create a new pattern
///
void SongEditorPanel::newPatBtnClicked( Button* btn)
{
	UNUSED( btn );
	Hydrogen *engine = Hydrogen::get_instance();
	Song *song = engine->getSong();
	PatternList *patternList = song->get_pattern_list();
	int emptyPatternNo = patternList->get_size() + 1;

	Pattern *emptyPattern = Pattern::get_empty_pattern();
	emptyPattern->set_name( trUtf8("Pattern %1").arg(emptyPatternNo) );
	emptyPattern->set_category( trUtf8("not_categorized") );

	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, emptyPattern, true );
	if ( dialog->exec() == QDialog::Accepted ) {
		patternList->add( emptyPattern );
		song->__is_modified = true;
		updateAll();
	}
	else {
		patternList->del( emptyPattern );
		delete emptyPattern;
		emptyPattern = NULL;
	}
	delete dialog;
	dialog = NULL;
}



///
/// Move up a pattern in the patternList
///
void SongEditorPanel::upBtnClicked( Button* btn )
{
	UNUSED( btn );
	Hydrogen *pEngine = Hydrogen::get_instance();
	int nSelectedPatternPos = pEngine->getSelectedPatternNumber();

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	Song *pSong = pEngine->getSong();
	PatternList *pList = pSong->get_pattern_list();

	if ( ( nSelectedPatternPos - 1 ) >= 0 ) {
		Pattern *pTemp = pList->get( nSelectedPatternPos - 1 );
		pList->replace( pList->get( nSelectedPatternPos ), nSelectedPatternPos - 1 );
		pList->replace( pTemp, nSelectedPatternPos );
		AudioEngine::get_instance()->unlock();
		pEngine->setSelectedPatternNumber( nSelectedPatternPos - 1 );

		updateAll();
		pSong->__is_modified = true;
	}
	else {
		AudioEngine::get_instance()->unlock();
	}
}



///
/// Move down a pattern in the patternList
///
void SongEditorPanel::downBtnClicked( Button* btn )
{
	UNUSED( btn );
	Hydrogen *pEngine = Hydrogen::get_instance();
	int nSelectedPatternPos = pEngine->getSelectedPatternNumber();

	AudioEngine::get_instance()->lock( RIGHT_HERE );
	Song *pSong = pEngine->getSong();
	PatternList *pList = pSong->get_pattern_list();

	if ( ( nSelectedPatternPos + 1 ) < (int)pList->get_size() ) {
		Pattern *pTemp = pList->get( nSelectedPatternPos + 1 );
		pList->replace( pList->get( nSelectedPatternPos ), nSelectedPatternPos + 1 );
		pList->replace( pTemp, nSelectedPatternPos );

		AudioEngine::get_instance()->unlock();
		pEngine->setSelectedPatternNumber( nSelectedPatternPos + 1 );

		updateAll();
		pSong->__is_modified = true;
	}
	else {
		AudioEngine::get_instance()->unlock();
	}
}




void SongEditorPanel::clearSequence( Button* btn)
{
	UNUSED( btn );

	int res = QMessageBox::information( this, "Hydrogen", tr( "Warning, this will erase your pattern sequence.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( res == 1 ) {
		return;
	}

	Hydrogen *engine = Hydrogen::get_instance();

	AudioEngine::get_instance()->lock( RIGHT_HERE );

	Song *song = engine->getSong();
	vector<PatternList*> *pPatternGroupsVect = song->get_pattern_group_vector();
	for (uint i = 0; i < pPatternGroupsVect->size(); i++) {
		PatternList *pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
		delete pPatternList;
	}
	pPatternGroupsVect->clear();

	AudioEngine::get_instance()->unlock();

	updateAll();
	song->__is_modified = true;
}



void SongEditorPanel::resyncExternalScrollBar()
{
	m_pHScrollBar->setMinimum( m_pEditorScrollView->horizontalScrollBar()->minimum() );
	m_pHScrollBar->setMaximum( m_pEditorScrollView->horizontalScrollBar()->maximum() );
	m_pHScrollBar->setSingleStep( m_pEditorScrollView->horizontalScrollBar()->singleStep() );
	m_pHScrollBar->setPageStep( m_pEditorScrollView->horizontalScrollBar()->pageStep() );
	m_pHScrollBar->setValue( m_pEditorScrollView->horizontalScrollBar()->value() );

	m_pVScrollBar->setMinimum( m_pEditorScrollView->verticalScrollBar()->minimum() );
	m_pVScrollBar->setMaximum( m_pEditorScrollView->verticalScrollBar()->maximum() );
	m_pVScrollBar->setSingleStep( m_pEditorScrollView->verticalScrollBar()->singleStep() );
	m_pVScrollBar->setPageStep( m_pEditorScrollView->verticalScrollBar()->pageStep() );
	m_pVScrollBar->setValue( m_pEditorScrollView->verticalScrollBar()->value() );
}


void SongEditorPanel::resizeEvent( QResizeEvent *ev )
{
	UNUSED( ev );
	resyncExternalScrollBar();
}

void SongEditorPanel::pointerActionBtnPressed( Button* pBtn )
{
	pBtn->setPressed( true );
	m_pDrawActionBtn->setPressed( false );
	m_actionMode = SELECT_ACTION;
}



void SongEditorPanel::drawActionBtnPressed( Button* pBtn )
{
	pBtn->setPressed( true );
	m_pPointerActionBtn->setPressed( false );
	m_actionMode = DRAW_ACTION;

}



void SongEditorPanel::timeLineBtnPressed( Button* pBtn )
{
	if( m_pTimeLineToggleBtn->isPressed() ){
		Preferences::get_instance()->__usetimeline = true;
		Hydrogen::get_instance()->setTimelineBpm();
	}
	else
	{
		Preferences::get_instance()->__usetimeline = false;
	}
	m_pPositionRuler->createBackground();
}


void SongEditorPanel::modeActionBtnPressed( )
{
	if( m_pModeActionBtn->isPressed() ){
		m_pModeActionBtn->setToolTip( trUtf8( "stacked pattern mode") );
	} else {
		m_pModeActionBtn->setToolTip( trUtf8( "single pattern mode") );
	}
	Hydrogen::get_instance()->togglePlaysSelected();
	updateAll();
}

void SongEditorPanel::setModeActionBtn( bool mode )
{
	if( mode ){
		m_pModeActionBtn->setPressed( true );
		m_pModeActionBtn->setToolTip( trUtf8( "stacked pattern mode") );
	} else {
		m_pModeActionBtn->setPressed( false );
		m_pModeActionBtn->setToolTip( trUtf8( "single pattern mode") );
	}
}

void SongEditorPanel::zoomInBtnPressed( Button* pBtn )
{
	UNUSED( pBtn );
	unsigned width = m_pSongEditor->getGridWidth ();
	--width;
	m_pSongEditor->setGridWidth (width);
	m_pPositionRuler->setGridWidth (width);

	updateAll();
}


void SongEditorPanel::zoomOutBtnPressed( Button* pBtn )
{
	UNUSED( pBtn );
	unsigned width = m_pSongEditor->getGridWidth ();
	++width;
	m_pSongEditor->setGridWidth (width);
	m_pPositionRuler->setGridWidth (width);
	updateAll();
}


void SongEditorPanel::selectedPatternChangedEvent()
{
  resyncExternalScrollBar();
}
