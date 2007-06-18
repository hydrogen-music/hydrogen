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
 * $Id: SongEditorPanel.cpp,v 1.32 2005/05/27 00:42:20 wsippel Exp $
 *
 */
#include "SongEditorPanel.h"

#include "config.h"
#include "gui/Skin.h"
#include "gui/HydrogenApp.h"
#include "gui/PatternPropertiesDialog.h"
#include "gui/SongPropertiesDialog.h"
#include "gui/widgets/Button.h"

#include "lib/Hydrogen.h"
#include "lib/Preferences.h"

#include <qtooltip.h>
#include <qmessagebox.h>

SongEditorPanel::SongEditorPanel(QWidget *pParent)
 : QWidget( pParent, "SongEditorPanel", Qt::WStyle_DialogBorder )
 , Object( "SongEditPanel" )
 , m_actionMode( DRAW_ACTION )
{

	m_nInitialWidth = 600;
	m_nInitialHeight = 250;

	const uint nMinWidth = 100;
	const uint nMinHeight = 50;

	resize( QSize(m_nInitialWidth, m_nInitialHeight) );
	setMinimumSize( nMinWidth, nMinHeight );
//	setMaximumSize( width, m_nHeight );

	setCaption( trUtf8( "Song Editor" ) );
	setIcon( QPixmap( Skin::getImagePath().c_str() + QString( "/icon32.png") ) );


	// background
	QWidget *pBackPanel = new QWidget( this );
	pBackPanel->resize( 196, 24 );
	pBackPanel->move( 6, 4 );
	pBackPanel->setPaletteBackgroundPixmap( QPixmap( Skin::getImagePath().append( "/songEditor/bg_topPanel.png" ).c_str() ) );

	// clear sequence button
	string m_pClearSeqBtn_on_path = Skin::getImagePath() + string( "/songEditor/btn_clear_on.png" );
	string m_pClearSeqBtn_off_path = Skin::getImagePath() + string( "/songEditor/btn_clear_off.png" );
	string m_pClearSeqBtn_over_path = Skin::getImagePath() + string( "/songEditor/btn_clear_over.png" );
	m_pClearPatternSeqBtn = new Button(pBackPanel, QSize(53,13), m_pClearSeqBtn_on_path, m_pClearSeqBtn_off_path, m_pClearSeqBtn_over_path);
	m_pClearPatternSeqBtn->move( 13, 5 );
	QToolTip::add( m_pClearPatternSeqBtn, trUtf8("Clear pattern sequence") );
	connect( m_pClearPatternSeqBtn, SIGNAL( clicked( Button* ) ), this, SLOT( clearSequence(Button*) ) );

	// new pattern button
	string newPat_on_path = Skin::getImagePath() + string( "/songEditor/btn_new_on.png" );
	string newPat_off_path = Skin::getImagePath() + string( "/songEditor/btn_new_off.png" );
	string newPat_over_path = Skin::getImagePath() + string( "/songEditor/btn_new_over.png" );
	Button *newPatBtn = new Button(pBackPanel, QSize(19, 13), newPat_on_path, newPat_off_path, newPat_over_path);
	newPatBtn->move( 75, 5 );
	QToolTip::add( newPatBtn, trUtf8("Create new pattern") );
	connect( newPatBtn, SIGNAL( clicked( Button* ) ), this, SLOT( newPatBtnClicked( Button* ) ) );

	// down button
	string m_pDownBtn_on_path = Skin::getImagePath() + string( "/songEditor/btn_down_on.png" );
	string m_pDownBtn_off_path = Skin::getImagePath() + string( "/songEditor/btn_down_off.png" );
	string m_pDownBtn_over_path = Skin::getImagePath() + string( "/songEditor/btn_down_over.png" );
	m_pDownBtn = new Button(pBackPanel, QSize(18,13), m_pDownBtn_on_path, m_pDownBtn_off_path, m_pDownBtn_over_path);
	m_pDownBtn->move( 102, 5 );
	QToolTip::add( m_pDownBtn, trUtf8("Move the selected pattern down") );
	connect( m_pDownBtn, SIGNAL( clicked( Button* ) ), this, SLOT( downBtnClicked( Button* ) ) );

	// up button
	string m_pUpBtn_on_path = Skin::getImagePath() + string( "/songEditor/btn_up_on.png" );
	string m_pUpBtn_off_path = Skin::getImagePath() + string( "/songEditor/btn_up_off.png" );
	string m_pUpBtn_over_path = Skin::getImagePath() + string( "/songEditor/btn_up_over.png" );
	m_pUpBtn = new Button(pBackPanel, QSize(18,13), m_pUpBtn_on_path, m_pUpBtn_off_path, m_pUpBtn_over_path);
	m_pUpBtn->move( 120, 5 );
	QToolTip::add( m_pUpBtn, trUtf8("Move the selected pattern up") );
	connect( m_pUpBtn, SIGNAL( clicked( Button* ) ), this, SLOT( upBtnClicked( Button* ) ) );

	// select toggle button
	string sPointerBtn_on = Skin::getImagePath() + string( "/songEditor/btn_select_on.png" );
	string sPointerBtn_off = Skin::getImagePath() + string( "/songEditor/btn_select_off.png" );
	string sPointerBtn_over = Skin::getImagePath() + string( "/songEditor/btn_select_over.png" );
	m_pPointerActionBtn = new ToggleButton( pBackPanel, QSize( 18, 13 ), sPointerBtn_on, sPointerBtn_off, sPointerBtn_over );
	m_pPointerActionBtn->move( 146, 5 );
	QToolTip::add( m_pPointerActionBtn, trUtf8( "Select mode" ) );
	connect( m_pPointerActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( pointerActionBtnPressed(Button*) ) );

	// draw toggle button
	string sDrawBtn_on = Skin::getImagePath() + string( "/songEditor/btn_draw_on.png" );
	string sDrawBtn_off = Skin::getImagePath() + string( "/songEditor/btn_draw_off.png" );
	string sDrawBtn_over = Skin::getImagePath() + string( "/songEditor/btn_draw_over.png" );
	m_pDrawActionBtn = new ToggleButton( pBackPanel, QSize( 18, 13 ), sDrawBtn_on, sDrawBtn_off, sDrawBtn_over );
	m_pDrawActionBtn->move( 164, 5 );
	QToolTip::add( m_pDrawActionBtn, trUtf8( "Draw mode") );
	connect( m_pDrawActionBtn, SIGNAL( clicked( Button* ) ), this, SLOT( drawActionBtnPressed(Button* ) ) );
	m_pDrawActionBtn->setPressed( true );




	// PATTERN LIST
	m_pPatternListScrollView = new QScrollView( this );
	m_pPatternListScrollView->setFrameShape( QFrame::NoFrame );
	m_pPatternListScrollView->move( 5, 30 );
	m_pPatternListScrollView->resize( m_nPatternListWidth, m_nInitialHeight - 45 - 5 );
	m_pPatternListScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pPatternListScrollView->setHScrollBarMode( QScrollView::AlwaysOn );

	m_pPatternList = new SongEditorPatternList( m_pPatternListScrollView->viewport() );
	m_pPatternList->move( 0, 0 );
	m_pPatternList->show();
	m_pPatternListScrollView->addChild( m_pPatternList );


	// EDITOR
	m_pEditorScrollView = new QScrollView( this );
	m_pEditorScrollView->setFrameShape( QFrame::NoFrame );
	m_pEditorScrollView->move( m_nPatternListWidth + 5, 30 );
	m_pEditorScrollView->resize( m_nInitialWidth - 110, m_nInitialHeight - 45 - 5 );

	m_pSongEditor = new SongEditor( m_pEditorScrollView->viewport() );
	m_pSongEditor->move( 0, 0 );
	m_pSongEditor->show();
	m_pEditorScrollView->addChild( m_pSongEditor );
	connect( m_pEditorScrollView, SIGNAL( contentsMoving ( int , int ) ), this, SLOT( contentsMove( int, int ) ) );


	// POSITION
	m_pPositionRulerScrollView = new QScrollView( this );
	m_pPositionRulerScrollView->setFrameShape( QFrame::NoFrame );
	m_pPositionRulerScrollView->move( m_nPatternListWidth + 5, 5 );
	m_pPositionRulerScrollView->resize( m_nInitialWidth - 110, 25 );
	m_pPositionRulerScrollView->setVScrollBarMode( QScrollView::AlwaysOff );
	m_pPositionRulerScrollView->setHScrollBarMode( QScrollView::AlwaysOff );

	m_pPositionRuler = new SongEditorPositionRuler( m_pPositionRulerScrollView->viewport() );
	m_pPositionRuler->move( 0, 0 );
	m_pPositionRulerScrollView->addChild( m_pPositionRuler );



	QPixmap patternIcon;
	string patternIcon_path = Skin::getImagePath() + string( "/songEditor/patternIcon.png" );
	patternIcon.load( patternIcon_path.c_str() );

	// Background image
	string background_path = Skin::getImagePath() + string( "/patternEditor/patternEditor_background.png" );
	QPixmap m_backgroundPixmap;
	bool ok = m_backgroundPixmap.load( background_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap " + background_path );
	}
	setPaletteBackgroundPixmap( m_backgroundPixmap );

	updateAll();

	HydrogenApp::getInstance()->addEventListener( this );

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
	Song *pSong = Hydrogen::getInstance()->getSong();

	if ( Preferences::getInstance()->m_bFollowPlayhead && pSong->getMode() == Song::SONG_MODE) {
		if ( Hydrogen::getInstance()->getState() != STATE_PLAYING ) {
			return;
		}

		int x = m_pPositionRulerScrollView->contentsX();
		int w = m_pPositionRulerScrollView->visibleWidth();
		int nPlayHeadPosition = Hydrogen::getInstance()->getPatternPos() * SONG_EDITOR_GRID_WIDTH;
//		infoLog( "[updatePlayHeadPosition] ph: " + toString( nPlayHeadPosition ) );

		if ( nPlayHeadPosition > ( x + w - 50 ) ) {
			m_pEditorScrollView->horizontalScrollBar()->setValue( m_pEditorScrollView->horizontalScrollBar()->value() + 100 );
			contentsMove( -1, -1 );	// force a re-sync
		}
		else if ( nPlayHeadPosition < x ) {
			m_pEditorScrollView->horizontalScrollBar()->setValue( m_pEditorScrollView->horizontalScrollBar()->value() - 100 );
			contentsMove( -1, -1 );	// force a re-sync
		}
	}
}



///
/// Synchronize the patternlist with the patternsequence
///
void SongEditorPanel::contentsMove( int x, int y) {
	m_pPatternListScrollView->verticalScrollBar()->setValue( m_pEditorScrollView->verticalScrollBar()->value() );
	m_pPositionRulerScrollView->horizontalScrollBar()->setValue( m_pEditorScrollView->horizontalScrollBar()->value() );
}



///
/// Update and redraw all...
///
void SongEditorPanel::updateAll() {
	m_pPatternList->createBackground();
	m_pPatternList->update();

	m_pSongEditor->createBackground();
	m_pSongEditor->update();
}



///
/// Create a new pattern
///
void SongEditorPanel::newPatBtnClicked( Button* btn) {
	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	PatternList *patternList = song->getPatternList();

	Pattern *emptyPattern = Pattern::getEmptyPattern();

	PatternPropertiesDialog *dialog = new PatternPropertiesDialog( this, emptyPattern );
	if ( dialog->exec() == QDialog::Accepted ) {
		patternList->add( emptyPattern );
		song->m_bIsModified = true;
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
	Hydrogen *pEngine = Hydrogen::getInstance();
	int nSelectedPatternPos = pEngine->getSelectedPatternNumber();

	pEngine->lockEngine("SongEditorPanel::m_pUpBtnClicked");
	Song *pSong = pEngine->getSong();
	PatternList *pList = pSong->getPatternList();

	if ( ( nSelectedPatternPos - 1 ) >= 0 ) {
		Pattern *pTemp = pList->get( nSelectedPatternPos - 1 );
		pList->replace( pList->get( nSelectedPatternPos ), nSelectedPatternPos - 1 );
		pList->replace( pTemp, nSelectedPatternPos );
		pEngine->unlockEngine();
		pEngine->setSelectedPatternNumber( nSelectedPatternPos - 1 );

		updateAll();
		pSong->m_bIsModified = true;
	}
	else {
		pEngine->unlockEngine();
	}
}



///
/// Move down a pattern in the patternList
///
void SongEditorPanel::downBtnClicked( Button* btn )
{
	Hydrogen *pEngine = Hydrogen::getInstance();
	int nSelectedPatternPos = pEngine->getSelectedPatternNumber();

	pEngine->lockEngine("SongEditorPanel::m_pDownBtnClicked");
	Song *pSong = pEngine->getSong();
	PatternList *pList = pSong->getPatternList();

	if ( ( nSelectedPatternPos + 1 ) < (int)pList->getSize() ) {
		Pattern *pTemp = pList->get( nSelectedPatternPos + 1 );
		pList->replace( pList->get( nSelectedPatternPos ), nSelectedPatternPos + 1 );
		pList->replace( pTemp, nSelectedPatternPos );

		pEngine->unlockEngine();
		pEngine->setSelectedPatternNumber( nSelectedPatternPos + 1 );

		updateAll();
		pSong->m_bIsModified = true;
	}
	else {
		pEngine->unlockEngine();
	}
}




void SongEditorPanel::clearSequence( Button* btn)
{
	int res = QMessageBox::information( this, "Hydrogen", tr( "Warning, this will erase your pattern sequence.\nAre you sure?"), tr("&Ok"), tr("&Cancel"), 0, 1 );
	if ( res == 1 ) {
		return;
	}

	Hydrogen *engine = Hydrogen::getInstance();

	engine->lockEngine("SongEditorPanel::clearSequence");

	Song *song = engine->getSong();
	vector<PatternList*> *pPatternGroupsVect = song->getPatternGroupVector();
	for (uint i = 0; i < pPatternGroupsVect->size(); i++) {
		PatternList *pPatternList = (*pPatternGroupsVect)[i];
		pPatternList->clear();
		delete pPatternList;
	}
	pPatternGroupsVect->clear();

	engine->unlockEngine();

	updateAll();
	song->m_bIsModified = true;
}



void SongEditorPanel::patternChangedEvent()
{
	m_pPatternList->createBackground();	// update the pattern highlight
	m_pPatternList->update();
	m_pPositionRuler->update();
}



void SongEditorPanel::resizeEvent ( QResizeEvent *ev )
{
	int nHeight = height() - 30 - 5 ;
	m_pEditorScrollView->resize( width() - (m_nPatternListWidth + 10), nHeight );

	// scrollview 2
	m_pPatternListScrollView->resize( m_nPatternListWidth, nHeight );

	// scrollview 3
	m_pPositionRulerScrollView->resize( width() - (m_nPatternListWidth + 10), 25 );
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


