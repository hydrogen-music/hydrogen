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
 * $Id: PatternEditor.cpp,v 1.28 2005/07/05 15:19:19 comix Exp $
 *
 */

#include "config.h"
#include "PatternEditor.h"
#include "PatternEditorPanel.h"

#include "lib/Globals.h"
#include "lib/Song.h"
#include "lib/Hydrogen.h"
#include "lib/Preferences.h"

#include "gui/HydrogenApp.h"
#include "gui/Mixer/Mixer.h"
#include "gui/Skin.h"

#include <assert.h>
#include <algorithm>
#include <qrect.h>
#include <qcursor.h>

PatternEditor::PatternEditor(QWidget* parent, PatternEditorPanel *pPanel)
 : QWidget(parent, "", Qt::WRepaintNoErase | Qt::WResizeNoErase  )
 , Object( "PatEditor" )
 , m_bChanged( true )
 , m_bNotesChanged( true )
 , m_nResolution( 8 )
 , m_bUseTriplets( false )
 , m_bRightBtnPressed( false )
 , m_pDraggedNote( NULL )
 , m_pPattern( NULL )
 , m_pPatternEditorPanel( pPanel )
{
	//infoLog("INIT");

	m_nGridWidth = (Preferences::getInstance())->getPatternEditorGridWidth();
	m_nGridHeight = (Preferences::getInstance())->getPatternEditorGridHeight();

	unsigned nEditorWidth = 5 + m_nGridWidth * ( MAX_NOTES * 4 ) + 5;
	unsigned nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;

	// Prepare pixmaps
	string background_path = Skin::getImagePath() + string( "/patternEditor/editor_background.png" );
	bool ok = m_background.load( background_path.c_str() );
	if( ok == false ){
		errorLog( "Error loading pixmap " + background_path );
	}
	createBackground( &m_background );
	setPaletteBackgroundPixmap( m_background );

	resize( nEditorWidth, nEditorHeight );
	setMinimumSize( width(), height() );
//	setMaximumSize( width(), height() );

//	m_background.resize( width(), height() );

 	m_bLasso = m_bDrag = false;
	m_bLassoRepaint = m_bDragRepaint = false;
	m_bSelect = false;

	setFocusPolicy (ClickFocus);
}



PatternEditor::~PatternEditor()
{
	//infoLog("DESTROY");
}



void PatternEditor::updateEditor(bool forceRepaint)
{
//	infoLog( "[updateEditor]" );
	if(!isVisible()) {
		return;
	}

	Hydrogen* engine = Hydrogen::getInstance();

	// check engine state
	int state = engine->getState();
	if ( (state != STATE_READY) && (state != STATE_PLAYING) ) {
		errorLog( "FIXME: skipping pattern editor update (state shoud be READY or PLAYING)" );
		return;
	}

	Hydrogen *pEngine = Hydrogen::getInstance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->getSize() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}

//	uint nEditorWidth = 5 + m_nGridWidth * MAX_NOTES + 5;
	uint nEditorWidth;
	if ( m_pPattern ) {
		nEditorWidth = 5 + m_nGridWidth * m_pPattern->m_nSize + 5;
	}
	else {
		nEditorWidth = 5 + m_nGridWidth * MAX_NOTES + 5;
	}
	m_bChanged = true;
	m_bNotesChanged = true;

	resize( nEditorWidth, height() );
	createBackground( &m_background );

	// redraw all
	update();
}


int PatternEditor::getSnapWidth()
{
	int nBase;
	if (m_bUseTriplets) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	return (m_nGridWidth * 4 * MAX_NOTES) / (nBase * m_nResolution);
}


int PatternEditor::getColumn(QMouseEvent *ev)
{
	int nBase;
	if (m_bUseTriplets) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int nWidth = (m_nGridWidth * 4 * MAX_NOTES) / (nBase * m_nResolution);

	int x = ev->x();
	int nColumn;
	nColumn = x - 10 + (nWidth / 2);
	nColumn = nColumn / nWidth;
	nColumn = (nColumn * 4 * MAX_NOTES) / (nBase * m_nResolution);
	return nColumn;
}

uint PatternEditor::getXPos(int nColumn)
{
	//infoLog( "[getXPos]" );
	return 10 + (nColumn * m_nGridWidth);// - m_nGridWidth / 2.0;
}

uint PatternEditor::getYPos(int nRow)
{
	//infoLog( "[getYPos]" );
	return (MAX_INSTRUMENTS  * m_nGridHeight) - ((nRow + 1) * m_nGridHeight) + (m_nGridHeight / 2) - 3;
}

// returns true if the current pos is inside the rectangle
bool PatternEditor::inSelect( int nColumn, int nRow, QRect area )
{
	//infoLog( "[inSelect]" );
	m_bLasso = false;

	int x_pos = getXPos(nColumn);
	int y_pos = getYPos(nRow);

	if ( y_pos >= area.top() && y_pos <= area.bottom() && x_pos >= area.left() && x_pos <= area.right() && m_bSelect)
		return true;
	else
		return false;
}

// mark notes in select region to use in paintEvent() when dragging
void PatternEditor::markupNotes()
{
	infoLog( "[markupNotes]" );

	if (m_pPattern == NULL) {
		return;
	}

	m_notePoint.clear();

	SequenceList *sequenceList = m_pPattern->m_pSequenceList;

	for (uint y = 0; y < sequenceList->getSize(); y++) 	{
		Sequence *seq = sequenceList->get(y);
		for (uint x = 0; x < MAX_NOTES; x++) {
			Note *note = seq->m_noteList[x];
			if (note != NULL && inSelect( x, y, m_srcRegion ) ) {

				uint pos = note->m_nPosition % MAX_NOTES;

				uint x_pos = getXPos(pos);
				uint y_pos = getYPos(y);

				m_notePoint.push_back( QPoint(x_pos - m_srcRegion.left(), y_pos - m_srcRegion.top() ) );

			}
		}
	}
}

// vectors used in manipulateRegion()
void PatternEditor::copyNotes()
{
	infoLog( "[copyNotes]" );

	if (m_pPattern == NULL) {
		return;
	}

	m_noteList.clear();
	m_notePoint.clear();

	SequenceList *sequenceList = m_pPattern->m_pSequenceList;

	for (uint y = 0; y < sequenceList->getSize(); y++) 	{
		Sequence *seq = sequenceList->get(y);
		for (uint x = 0; x < MAX_NOTES; x++) {
			Note *note = seq->m_noteList[x];
			if (note != NULL && inSelect( x, y, m_srcRegion ) ) {

				m_noteList.push_back( note->copy() );
				m_notePoint.push_back( QPoint(x,y) );

			}
		}
	}
}

// manipulate region, ie delete/copy/move a set of cells
// it copies/moves from m_srcRegion to m_dstRegion
// or deletes in m_srcRegion
void PatternEditor::manipulateRegion(RegionManipulationMode mode)
{
	infoLog( "[manipulateRegion]" );

	if (m_pPattern == NULL || !m_bSelect) {
		return;
	}

	// calculate grid distance between src and dst region
	QPoint dist( ( (signed)m_dstRegion.left() - (signed)m_srcRegion.left() ) / (signed)m_nGridWidth ,
               ( (signed)m_srcRegion.top()  - (signed)m_dstRegion.top()  ) / (signed)m_nGridHeight );

	QPoint point, newPoint;

	#define VALID(p) !( p.x() < 0 || p.y() < 0 || p.y() >= MAX_INSTRUMENTS || p.x() >= MAX_NOTES )

	if (mode == DELETE_MODE) {
		dist.setX(0);
		dist.setY(0);
	}
	else if (dist.x() == 0 && dist.y() == 0) {
		return;
	}

	copyNotes();

	( Hydrogen::getInstance() )->lockEngine("PatternEditor::manipulateRegion");	// lock the audio engine
	Song *pSong = (Hydrogen::getInstance())->getSong();
	vector <QPoint> savedNotes;

	savedNotes.clear();

	for (uint i = 0; i < m_noteList.size(); i++) {
		point = newPoint = m_notePoint[i];
		newPoint += dist;

		if ( !VALID(point) ) { // this shouldn't be possible really
			delete m_noteList[i];
			continue;
		}

		Sequence *seqSrc = m_pPattern->m_pSequenceList->get( point.y() );
		Sequence *seqDst;
		Note *note;
		Instrument *instr;

		switch (mode) {

			case COPY_MODE:
			case MOVE_MODE:
				if ( VALID(newPoint) ) { // make sure the new point isn't out of bounds

					seqDst = m_pPattern->m_pSequenceList->get( newPoint.y() );
					note = m_noteList[i]->copy();
					instr = pSong->getInstrumentList()->get(newPoint.y());
					note->setInstrument(instr);
					note->m_nPosition = newPoint.x();

					if ( seqDst->m_noteList[ newPoint.x() ] != NULL ) {
						delete seqDst->m_noteList[ newPoint.x() ];
					}

					seqDst->m_noteList[ newPoint.x() ] = note;
				}

				if (mode == COPY_MODE) // don't delete if copying
					break;

			case DELETE_MODE:
				if (mode == DELETE_MODE || (std::find(savedNotes.begin(), savedNotes.end(), point) == savedNotes.end() ) ) {
					note = seqSrc->m_noteList[ point.x() ];
					if (note != NULL) {
						delete note;
						seqSrc->m_noteList[ point.x() ] = NULL;
					}
				}
				if (mode == MOVE_MODE)
					savedNotes.push_back(newPoint); //save this note position so we don't delete it again when moving

				break;
		}

		delete m_noteList[i];

	}

	pSong->m_bIsModified = true;
	( Hydrogen::getInstance() )->unlockEngine(); // unlock the audio engine

	m_bSelect = false;

}


void PatternEditor::mousePressEvent(QMouseEvent *ev) {
	if (m_pPattern == NULL) {
		return;
	}

	int row = MAX_INSTRUMENTS - 1 - (ev->y()  / (int)m_nGridHeight);
	if (row >= MAX_INSTRUMENTS) {
		return;
	}

	int nColumn = getColumn( ev );

//	if (nColumn >= MAX_NOTES) {
	if ( nColumn >= m_pPattern->m_nSize ) {
		update();
		return;
	}

	if (ev->button() == MidButton) {
		m_bLasso = true;
		m_srcRegion.setRect( ev->x(), ev->y(), 0, 0 );
		return;
	}
	else if ( ev->button() == LeftButton && inSelect( nColumn, row, m_srcRegion ) ) {
		m_bDragCopy = ev->state() & ControlButton; //use copy if ctrl-clicking
		m_bDrag = m_bDragRepaint = true;
		m_dstRegion = m_srcRegion;
		m_mousePos = ev->pos();

		update();
		return;
	}
	else if (ev->button() == LeftButton ) {
		m_bRightBtnPressed = m_bSelect = false;
		( Hydrogen::getInstance() )->lockEngine("PatternEditor::mousePressEvent");	// lock the audio engine
		Song *pSong = (Hydrogen::getInstance())->getSong();
		Sequence *pSeq = m_pPattern->m_pSequenceList->get(row);

		if (pSeq->m_noteList[ nColumn ] != NULL) {
			Note *note = pSeq->m_noteList[ nColumn ];
			delete note;
			pSeq->m_noteList[ nColumn ] = NULL;
			m_bNotesChanged = true;
		}
		else {
			// create the new note
			const unsigned nPosition = nColumn;
			const float fVelocity = 0.8f;
			const float fPan_L = 1.0f;
			const float fPan_R = 1.0f;
			const int nLength = -1;
			const float fPitch = 0.0f;
			Instrument *pInstr = pSong->getInstrumentList()->get(row);
			Note *note = new Note( pInstr, nPosition, fVelocity, fPan_L, fPan_R, nLength, fPitch);
			pSeq->m_noteList[ nColumn ] = note;
			m_bNotesChanged = true;

			// hear note
			Preferences *pref = Preferences::getInstance();
			if ( pref->getHearNewNotes() ) {
				Note *note2 = new Note( pInstr, 0, fVelocity, fPan_L, fPan_R, nLength, fPitch);
				Hydrogen::getInstance()->noteOn( note2 );
			}
		}
		pSong->m_bIsModified = true;
		( Hydrogen::getInstance() )->unlockEngine(); // unlock the audio engine
	}
	else if (ev->button() == RightButton ) {
		m_bRightBtnPressed = true;
		m_bSelect = false;
		m_pDraggedNote = NULL;

		int nRealColumn = (ev->x() - 10) /  (int)m_nGridWidth;

		( Hydrogen::getInstance() )->lockEngine("PatternEditor::mousePressEvent");	// lock the audio engine
		Sequence *pSeq = m_pPattern->m_pSequenceList->get(row);

		if (pSeq->m_noteList[ nColumn ] != NULL) {
			m_pDraggedNote = pSeq->m_noteList[ nColumn ];
		}
		else if (pSeq->m_noteList[ nRealColumn ] != NULL) {
			m_pDraggedNote = pSeq->m_noteList[ nRealColumn ];
		}
		else {
			// potrei essere sulla coda di una nota precedente..
			for ( int i = 0; i < nRealColumn; i++) {
				if ( pSeq->m_noteList[ i ] ) {
					int nEndColumn = i + ( pSeq->m_noteList[ i ] )->m_nLength;
					if (nEndColumn > nRealColumn ) {
						m_pDraggedNote = pSeq->m_noteList[ i ];
					}
				}
			}
		}
		( Hydrogen::getInstance() )->unlockEngine(); // unlock the audio engine
	}

	// update the selected line
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	if (nSelectedInstrument != row) {
		m_pPatternEditorPanel->setSelectedInstrument(row);
	}
	else {
		m_bChanged = true;
		m_bNotesChanged = true;
		createBackground( &m_background );
		update();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPitchEditor()->updateEditor();
	}
}

void PatternEditor::mouseReleaseEvent(QMouseEvent *ev)
{
	setCursor( QCursor( Qt::ArrowCursor ) );

	if (m_pPattern == NULL) {
		return;
	}

	if (m_bLasso) { // user is done with selecting notes

		m_bSelect = true;
		m_bLasso = m_bLassoRepaint = false;

		m_srcRegion = m_srcRegion.normalize();

		markupNotes();

		m_bChanged = m_bNotesChanged = true;

		update();
		m_oldRegion.setRect( 0, 0, 0, 0 );

	}	else if (m_bDrag) { // user is done with the draggin, copy/move the notes

		m_bDrag = m_bDragRepaint = false;

		if (m_bDragCopy) {
			manipulateRegion( COPY_MODE );
		}
		else {
			manipulateRegion( MOVE_MODE );
		}

		m_bChanged = m_bNotesChanged = true;
		m_bSelect = false;
		createBackground( &m_background );

		update();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPitchEditor()->updateEditor();
		m_oldRegion.setRect( 0, 0, 0, 0 );
	}
}



void PatternEditor::mouseMoveEvent(QMouseEvent *ev)
{
	if (m_pPattern == NULL) {
		return;
	}

	int row = MAX_INSTRUMENTS - 1 - (ev->y()  / (int)m_nGridHeight);
	if (row >= MAX_INSTRUMENTS) {
		return;
	}

	if (m_bLasso) {	// user is resizing lasso (ie marking notes)
		//m_srcRegion.setBottomRight(QPoint( ev->x(), ev->y() ) );
		m_srcRegion.setRect( m_srcRegion.x(), m_srcRegion.y(), ev->x() - m_srcRegion.x(), ev->y() - m_srcRegion.y() );

		m_bLassoRepaint = true;
		update();
	}
	else if (m_bDrag) {	// user is dragging previously created lasso to a new position (ie copying)
		setCursor( QCursor( Qt::SizeAllCursor ) );

		int nWidth = getSnapWidth();
		int xDiff = (ev->x() / (signed)nWidth)  - (m_mousePos.x() / (signed)nWidth);
		int yDiff = (ev->y() / (signed)m_nGridHeight) - (m_mousePos.y() / (signed)m_nGridHeight);

		if (xDiff != 0 || yDiff != 0) {
			m_dstRegion.moveBy( xDiff * nWidth, yDiff * m_nGridHeight );
			QPoint newPos( xDiff * nWidth, yDiff * m_nGridHeight );

			m_mousePos += newPos;

			m_bDragRepaint = true;
			update();
		}
	}
	else	if (m_bRightBtnPressed && m_pDraggedNote ) {
		int nTickColumn = getColumn( ev );

		( Hydrogen::getInstance() )->lockEngine("PatternEditor::mouseMoveEvent");	// lock the audio engine
		int nLen = nTickColumn - (int)m_pDraggedNote->m_nPosition;

		if (nLen <= 0) {
			nLen = -1;
		}
		m_pDraggedNote->m_nLength = nLen;

		Hydrogen::getInstance()->getSong()->m_bIsModified = true;
		( Hydrogen::getInstance() )->unlockEngine(); // unlock the audio engine

		m_bChanged = true;
		m_bNotesChanged = true;
		createBackground( &m_background );
		update();
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPitchEditor()->updateEditor();

	}
// 	else {
// 	}

}

void PatternEditor::keyPressEvent (QKeyEvent *ev)
{
	if (ev->key() == Key_Delete && m_bSelect) { 	// if pressing Delete and having a selected region

		manipulateRegion(DELETE_MODE);

		m_bChanged = true;
		m_bNotesChanged = true;
		m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
		m_pPatternEditorPanel->getPitchEditor()->updateEditor();
		createBackground( &m_background );
		update();

		m_oldRegion.setRect(0,0,0,0);
	}
	else {
		ev->ignore();
	}
}


/// Draw a note
void PatternEditor::drawNote(Note *note, uint nSequence, QPixmap *pixmap)
{
	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	QColor noteColor( pStyle->m_patternEditor_noteColor.getRed(), pStyle->m_patternEditor_noteColor.getGreen(), pStyle->m_patternEditor_noteColor.getBlue() );


	QPainter p( pixmap );
//	p.setRasterOp( Qt::AndROP );

//	uint pos = note->getPosition() % MAX_NOTES;
	uint pos = note->m_nPosition;

	if (inSelect(pos, nSequence, m_srcRegion))
		p.setPen( QColor(150,150,150)); // color for selected note
	else
		p.setPen( noteColor );

	if ( note->m_nLength == -1 ) {	// trigger note
		uint x_pos = 10 + (pos * m_nGridWidth);// - m_nGridWidth / 2.0;
		uint y_pos = (MAX_INSTRUMENTS  * m_nGridHeight) - ((nSequence + 1) * m_nGridHeight) + (m_nGridHeight / 2) - 3;

		// draw the "dot"
		p.drawLine(x_pos, y_pos, x_pos + 3, y_pos + 3);
		p.drawLine(x_pos, y_pos, x_pos - 3, y_pos + 3);
		p.drawLine(x_pos, y_pos + 6, x_pos + 3, y_pos + 3);
		p.drawLine(x_pos - 3, y_pos + 3, x_pos, y_pos + 6);

		p.drawLine(x_pos, y_pos + 1, x_pos + 2, y_pos + 3);
		p.drawLine(x_pos, y_pos + 1, x_pos - 2, y_pos + 3);
		p.drawLine(x_pos, y_pos + 5, x_pos + 2, y_pos + 3);
		p.drawLine(x_pos - 2, y_pos + 3, x_pos, y_pos + 5);

		p.drawLine(x_pos, y_pos + 2, x_pos + 1, y_pos + 3);
		p.drawLine(x_pos, y_pos + 2, x_pos - 1, y_pos + 3);
		p.drawLine(x_pos, y_pos + 4, x_pos + 1, y_pos + 3);
		p.drawLine(x_pos - 1, y_pos + 3, x_pos, y_pos + 4);
	}
	else {
		uint x = 10 + (pos * m_nGridWidth);
		int w = m_nGridWidth * note->m_nLength;
		w = w - 1;	// lascio un piccolo spazio tra una nota ed un altra

/*		uint y = (int) ( (MAX_INSTRUMENTS  * m_nGridHeight) - ((nSequence + 1) * m_nGridHeight) + (m_nGridHeight / 100.0 * 30.0) );
		int h = (int) (m_nGridHeight - ((m_nGridHeight / 100.0 * 30.0) * 2.0) );*/
		int y = (int) ( (MAX_INSTRUMENTS  * m_nGridHeight) - ((nSequence + 1) * m_nGridHeight) + (m_nGridHeight / 100.0 * 30.0) );
		int h = (int) (m_nGridHeight - ((m_nGridHeight / 100.0 * 30.0) * 2.0) );

		p.fillRect( x, y + 1, w, h + 1, QColor(100, 100, 200) );	/// \todo: definire questo colore nelle preferenze
		p.drawRect( x, y + 1, w, h + 1 );
	}




}



void PatternEditor::createBackground(QPixmap *pixmap)
{
	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->m_nSize;
	}

	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	QColor alternateRowColor( pStyle->m_patternEditor_alternateRowColor.getRed(), pStyle->m_patternEditor_alternateRowColor.getGreen(), pStyle->m_patternEditor_alternateRowColor.getBlue() );
	QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );
	QColor selectedMuteRowColor( pStyle->m_patternEditor_selectedMuteRowColor.getRed(), pStyle->m_patternEditor_selectedMuteRowColor.getGreen(), pStyle->m_patternEditor_selectedMuteRowColor.getBlue() );
	QColor muteRowColor( pStyle->m_patternEditor_muteRowColor.getRed(), pStyle->m_patternEditor_muteRowColor.getGreen(), pStyle->m_patternEditor_muteRowColor.getBlue() );
	QColor selectedLockRowColor( pStyle->m_patternEditor_selectedLockRowColor.getRed(), pStyle->m_patternEditor_selectedLockRowColor.getGreen(), pStyle->m_patternEditor_selectedLockRowColor.getBlue() );
	QColor selectedMuteLockRowColor( pStyle->m_patternEditor_selectedMuteLockRowColor.getRed(), pStyle->m_patternEditor_selectedMuteLockRowColor.getGreen(), pStyle->m_patternEditor_selectedMuteLockRowColor.getBlue() );
	QColor muteLockRowColor( pStyle->m_patternEditor_muteLockRowColor.getRed(), pStyle->m_patternEditor_muteLockRowColor.getGreen(), pStyle->m_patternEditor_muteLockRowColor.getBlue() );
	QColor lockRowColor( pStyle->m_patternEditor_lockRowColor.getRed(), pStyle->m_patternEditor_lockRowColor.getGreen(), pStyle->m_patternEditor_lockRowColor.getBlue() );
	QColor lineColor( pStyle->m_patternEditor_lineColor.getRed(), pStyle->m_patternEditor_lineColor.getGreen(), pStyle->m_patternEditor_lineColor.getBlue() );

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	pixmap->resize( width(), height() );
	pixmap->fill( backgroundColor );
	QPainter p(pixmap);

//	p.drawLine( 0, 0, width(), height() );

	// start y of velocity ruler
	uint velRuler_y_start = m_nGridHeight * MAX_INSTRUMENTS;

	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	// horizontal lines (only fill color)
	for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
		uint y = m_nGridHeight * i;
		Song *song = (Hydrogen::getInstance())->getSong();
		Instrument *pInstr = (song->getInstrumentList())->get( (MAX_INSTRUMENTS -1) - i );

		if ( ((MAX_INSTRUMENTS -1) - i) == (uint)nSelectedInstrument) {	// selected instrument row
			if ( pInstr->m_bIsMuted ) {
				if ( pInstr->m_bIsLocked ){
					p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, selectedMuteLockRowColor ); // muted instrument is also locked
				}
				else {
					p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, selectedMuteRowColor ); // instrument is just locked
				}
			}
			else if ( pInstr->m_bIsLocked ) {
				p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, selectedLockRowColor ); // instrument locked
			}
			else {
				p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, selectedRowColor ); // instrument unlocked and unmuted
			}
		}
		else if ( pInstr->m_bIsMuted ) {	// unselected instrument row
			if ( pInstr->m_bIsLocked ){
					p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, muteLockRowColor ); // muted instrument is also locked
			}
			else {
				p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, muteRowColor );
			}
		}
		else if ( pInstr->m_bIsLocked ) {
			p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, lockRowColor ); // instrument is locked
		}
		else {	// fill line
			if ( ( i % 2) == 0) {
				p.fillRect( 0, y, (10 + nNotes * m_nGridWidth), m_nGridHeight, alternateRowColor ); // instrument is not locked, muted, or selected
			}
		}
	}

	// vertical lines
//	p.setRasterOp( Qt::AndROP );
//	p.setRasterOp( Qt::NotOrROP );
	p.setPen( QPen( res_1, 0, Qt::DotLine ) );

	int nBase;
	if (m_bUseTriplets) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}

	int n4th = 4 * MAX_NOTES / (nBase * 4);
	int n8th = 4 * MAX_NOTES / (nBase * 8);
	int n16th = 4 * MAX_NOTES / (nBase * 16);
	int n32th = 4 * MAX_NOTES / (nBase * 32);
	int n64th = 4 * MAX_NOTES / (nBase * 64);

	if (!m_bUseTriplets) {
		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 10 + i * m_nGridWidth;

			if ( (i % n4th) == 0 ) {
				if (m_nResolution >= 4) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, velRuler_y_start );
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (m_nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, velRuler_y_start);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (m_nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, velRuler_y_start );
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (m_nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, velRuler_y_start );
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (m_nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, velRuler_y_start );
				}
			}
		}
	}
	else {	// Triplets
		uint nCounter = 0;
		int nSize = 4 * MAX_NOTES / (nBase * m_nResolution);

		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 10 + i * m_nGridWidth;

			if ( (i % nSize) == 0) {
				if ((nCounter % 3) == 0) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
				}
				p.drawLine(x, 0, x, velRuler_y_start );
				nCounter++;
			}
		}
	}

	// horizontal lines
	p.setPen( lineColor );
	for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
		uint y = m_nGridHeight * i;
		p.drawLine( 0, y, (10 + nNotes * m_nGridWidth), y);
	}
	// end of instruments

	p.drawLine( 0, m_nGridHeight * MAX_INSTRUMENTS, (10 + nNotes * m_nGridWidth), m_nGridHeight * MAX_INSTRUMENTS);
}



void PatternEditor::paintEvent( QPaintEvent*)
{
	if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;

		if (m_bNotesChanged) {	// ridisegno tutto solo se sono cambiate le note
			drawPattern( &m_background );
			m_bNotesChanged = false;
		}
	}
	bitBlt(this, 0, 0, &m_background, 0, 0, width(), height(), CopyROP);

	if (m_bLassoRepaint || m_bDragRepaint) {
 		QRect currentRegion;

		if (m_bLassoRepaint)
			currentRegion = m_srcRegion.normalize();
		else
			currentRegion = m_dstRegion.normalize();

		QPainter p( this );
		p.setRasterOp( XorROP );

		QPen pen( gray );
		pen.setStyle( Qt::DotLine );
		//pen.setStyle( Qt::DashLine );
		p.setPen( pen );

		if ( m_bLassoRepaint ) {
			p.drawRect( m_oldRegion );	// remove old rectangle, xor:ing will take care of that
			p.drawRect( currentRegion );	// add the new one
		}

		// if dragging, also draw each note
		if (m_bDragRepaint) {
			QPoint notePoint;
			QRect tempRegion;
			QPointArray notePoly(4);
			/*QBrush brush( gray, SolidPattern );
			p.setBrush( brush );*/

			pen.setStyle( Qt::SolidLine );
			p.setPen(pen);

			// loop twice, to erase old notes and draw the new ones
			for (int j = 0; j < 2; j++) {
				if (j == 0)
					tempRegion = m_oldRegion;
				else
					tempRegion = currentRegion;

				if (j > 0 || !tempRegion.isNull()) {
					for (uint i = 0; i < m_notePoint.size(); i ++) {

						notePoint = m_notePoint[i];
						notePoint += QPoint(tempRegion.left(), tempRegion.top() );
						int x = notePoint.x();
						int y = notePoint.y();

						notePoly.setPoints(4, x, y, x + 3, y + 3, x, y + 6, x - 3, y + 3);
						p.drawPolygon(notePoly);
					}
				}
			}
		}
		m_bLassoRepaint = m_bDragRepaint = false;
		m_oldRegion = currentRegion;
	}
}



void PatternEditor::drawPattern(QPixmap *pixmap)
{
	if (m_pPattern == NULL) {
		return;
	}

	SequenceList *sequenceList = m_pPattern->m_pSequenceList;

	for (uint i = 0; i < sequenceList->getSize(); i++) 	{
		Sequence *seq = sequenceList->get(i);
		for (uint j = 0; j < m_pPattern->m_nSize; j++) {
			Note *note = seq->m_noteList[j];
			if (note != NULL) {
				drawNote(note, i, pixmap);
			}
		}
	}
}



void PatternEditor::showEvent ( QShowEvent *ev ) {
	updateEditor();
//	updateStart(true);
}



void PatternEditor::hideEvent ( QHideEvent *ev ) {
//	updateStart(false);
}



void PatternEditor::setResolution(uint res, bool bUseTriplets) {
	this->m_nResolution = res;
	this->m_bUseTriplets = bUseTriplets;

	// redraw all
	createBackground( &m_background );
	m_bChanged = true;
	m_bNotesChanged = true;
	update();
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPitchEditor()->updateEditor();
	/// \todo [PatternEditor::setResolution] aggiornare la risoluzione del Ruler in alto."
}


void PatternEditor::zoomIn()
{
	m_nGridWidth = m_nGridWidth * 2;
	updateEditor( true );
}

void PatternEditor::zoomOut()
{
	m_nGridWidth = m_nGridWidth / 2;
	if (m_nGridWidth < 3) {
		m_nGridWidth = 3;
	}
	updateEditor( true );
}



///////////////////////////////////



PatternEditorRuler::PatternEditorRuler(QWidget* parent, PatternEditorPanel *pPatternEditorPanel)
 : QWidget( parent , "PatternEditorRuler", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "PatEditRuler" )
{
	//infoLog( "INIT" );

	Preferences *pPref = Preferences::getInstance();

	//QColor backgroundColor(230, 230, 230);
	UIStyle *pStyle = pPref->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );


	m_pPattern = NULL;
	m_nGridWidth = Preferences::getInstance()->getPatternEditorGridWidth();

	m_nRulerWidth = 5 + m_nGridWidth * ( MAX_NOTES * 4 ) + 5;
	m_nRulerHeight = 25;

	resize( m_nRulerWidth, m_nRulerHeight );

	setMinimumSize( width(), height() );
	setMaximumSize( width(), height() );

	m_temp.resize( m_nRulerWidth, m_nRulerHeight );

	string tickPosition_path = Skin::getImagePath() + string( "/patternEditor/tickPosition.png" );
	bool ok = m_tickPosition.load(tickPosition_path.c_str());
	if( ok == false ){
		errorLog( "Error loading pixmap " + tickPosition_path );
	}

	m_background.resize( m_nRulerWidth, m_nRulerHeight );
	m_background.fill( backgroundColor );
	setPaletteBackgroundPixmap( m_background );

	m_bChanged = true;

	m_pTimer = new QTimer(this);
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(updateEditor()));
}



PatternEditorRuler::~PatternEditorRuler() {
	//infoLog( "DESTROY");
}



void PatternEditorRuler::updateStart(bool start) {
	if (start) {
		m_pTimer->start(50);	// update ruler at 20 fps
	}
	else {
		m_pTimer->stop();
	}
}



void PatternEditorRuler::showEvent ( QShowEvent *ev ) {
	updateEditor();
	updateStart(true);
}



void PatternEditorRuler::hideEvent ( QHideEvent *ev ) {
	updateStart(false);
}



void PatternEditorRuler::updateEditor( bool bRedrawAll ) {
	static int oldNTicks = 0;

	Hydrogen *pEngine = Hydrogen::getInstance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->getSize() )  ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}


	bool bActive = false;
	PatternList *pList = pEngine->getCurrentPatternList();
	for (uint i = 0; i < pList->getSize(); i++) {
		if ( m_pPattern == pList->get(i) ) {
			bActive = true;
		}
	}

	int state = pEngine->getState();
	if ( ( state == STATE_PLAYING ) && (bActive) ) {
		m_nTicks = pEngine->getTickPosition();
	}
	else {
		m_nTicks = -1;	// hide the tickPosition
	}


	if (oldNTicks != m_nTicks) {
		// redraw all
		m_bChanged = true;
		update();
	}
	oldNTicks = m_nTicks;

	if (bRedrawAll) {
		m_bChanged = true;
		update();
	}
}



void PatternEditorRuler::paintEvent( QPaintEvent*) {
  if (!isVisible()) {
		return;
	}

	if (m_bChanged) {
		m_bChanged = false;
		bitBlt( &m_temp, 0, 0, &m_background, 0, 0, m_nRulerWidth, m_nRulerHeight, CopyROP );

		// gray background for unusable section of pattern
		QPainter p( &m_temp );
		if (m_pPattern) {
			int nXStart = 10 + m_pPattern->m_nSize * m_nGridWidth;
			if ( (m_nRulerWidth - nXStart) != 0 ) {
				p.fillRect( nXStart, 0, m_nRulerWidth - nXStart, m_nRulerHeight, QColor(170,170,170) );
			}
		}

		// numbers
		QColor textColor( 100, 100, 100 );
		QColor lineColor( 170, 170, 170 );

		Preferences *pref = Preferences::getInstance();
		QString family = pref->getApplicationFontFamily().c_str();
		int size = pref->getApplicationFontPointSize();
		QFont font( family, size );
		p.setFont(font);
		p.drawLine( 0, 0, m_nRulerWidth, 0 );
		p.drawLine( 0, m_nRulerHeight - 1, m_nRulerWidth - 1, m_nRulerHeight - 1);

		uint nQuarter = 48;

		for ( int i = 0; i < 64; i++ ) {
			int nText_x = 10 + nQuarter / 4 * i * m_nGridWidth;
			if ( ( i % 4 ) == 0 ) {
				p.setPen( textColor );
				p.drawText( nText_x - 30, 0, 60, m_nRulerHeight, Qt::AlignCenter, QString( toString( i / 4 + 1 ).c_str() ) );
			}
			else {
				p.setPen( lineColor );
				p.drawLine( nText_x, ( m_nRulerHeight - 5 ) / 2, nText_x, m_nRulerHeight - ( (m_nRulerHeight - 5 ) / 2 ) );
			}
		}

		// draw tickPosition
		if (m_nTicks != -1) {
			uint x = 10 + m_nTicks * m_nGridWidth - 5 - 11 / 2.0;
			bitBlt( &m_temp, x, height() / 2, &m_tickPosition, 0, 0, 11, m_nRulerHeight, CopyROP );
		}
	}
	bitBlt(this, 0, 0, &m_temp, 0, 0, width(), height(), CopyROP);
}


void PatternEditorRuler::zoomIn()
{
	errorLog( "[zoomIn] not implemented yet" );
}

void PatternEditorRuler::zoomOut()
{
	errorLog( "[zoomOut] not implemented yet" );
}



// :::::::::::::::::::



PatternEditorInstrumentList::PatternEditorInstrumentList( QWidget *parent, PatternEditorPanel *pPatternEditorPanel )
 : QWidget( parent , "PatternEditorInstrumentList", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "PatEditInstrList" )
{

	//infoLog("INIT");
	m_pPattern = NULL;
 	m_pPatternEditorPanel = pPatternEditorPanel;
	m_nGridHeight = (Preferences::getInstance())->getPatternEditorGridHeight();

	m_nEditorWidth = 100;
	m_nEditorHeight = m_nGridHeight * MAX_INSTRUMENTS;
	m_bChanged = true;

	m_background.resize(m_nEditorWidth, m_nEditorHeight );
	m_temp.resize(m_nEditorWidth, m_nEditorHeight );

	createBackground( &m_background );

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumSize( m_nEditorWidth, m_nEditorHeight );
	setMaximumSize( m_nEditorWidth, m_nEditorHeight );


	string genericIcon_path = Skin::getImagePath() + string( "/songEditor/patternIcon.png" );
	m_genericIcon.load( genericIcon_path.c_str() );

	// Popup menu
	m_pFunctionPopup  = new QPopupMenu( this, "patternSequence_patternPopupMenu" );
	m_pFunctionPopup->insertItem( m_genericIcon, trUtf8( "Mute" ), this, SLOT( functionMute() ) );
	m_pFunctionPopup->insertItem( m_genericIcon, trUtf8( "Lock" ), this, SLOT( functionLock() ) );
	m_pFunctionPopup->insertItem( m_genericIcon, trUtf8( "Solo" ), this, SLOT( functionSolo() ) );
	m_pFunctionPopup->insertItem( m_genericIcon, trUtf8( "Clear notes" ), this, SLOT( functionClearNotes() ) );
	m_pFunctionPopup->insertItem( m_genericIcon, trUtf8( "Fill notes" ), this, SLOT( functionFillNotes() ) );
	m_pFunctionPopup->insertItem( m_genericIcon, trUtf8( "Randomize velocity" ), this, SLOT( functionRandomizeVelocity() ) );

}



PatternEditorInstrumentList::~PatternEditorInstrumentList()
{
	//infoLog( "DESTROY" );
}



void PatternEditorInstrumentList::paintEvent( QPaintEvent*) {
	if (!isVisible()) {
		return;
	}

	bitBlt(this, 0, 0, &m_background, 0, 0, m_nEditorWidth, m_nEditorHeight, CopyROP);
}



void PatternEditorInstrumentList::createBackground(QPixmap *pixmap)
{
	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	QColor alternateRowColor( pStyle->m_patternEditor_alternateRowColor.getRed(), pStyle->m_patternEditor_alternateRowColor.getGreen(), pStyle->m_patternEditor_alternateRowColor.getBlue() );
	QColor selectedRowColor( pStyle->m_patternEditor_selectedRowColor.getRed(), pStyle->m_patternEditor_selectedRowColor.getGreen(), pStyle->m_patternEditor_selectedRowColor.getBlue() );
	QColor textColor( pStyle->m_patternEditor_textColor.getRed(), pStyle->m_patternEditor_textColor.getGreen(), pStyle->m_patternEditor_textColor.getBlue() );
	QColor lineColor( pStyle->m_patternEditor_lineColor.getRed(), pStyle->m_patternEditor_lineColor.getGreen(), pStyle->m_patternEditor_lineColor.getBlue() );

	pixmap->fill( backgroundColor );

	Preferences *pref = Preferences::getInstance();
	QString family = pref->getApplicationFontFamily().c_str();
	int size = pref->getApplicationFontPointSize();
	QFont font( family, size );

	QPainter p(pixmap);
	p.setFont(font);

	Song *song = (Hydrogen::getInstance())->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	//int nSelectedInstrument = m_pPatternEditorPanel->getSelectedInstrument();
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();

	// horizontal lines (only fill color)
	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		uint y = m_nGridHeight * i;
		if ( ((MAX_INSTRUMENTS -1) - i ) == nSelectedInstrument ) {	// selected instrument line
			p.fillRect( 0, y, m_nEditorWidth, m_nGridHeight, selectedRowColor );
		}
		else {
			if ( (i % 2) == 0) {	// fill line
				p.fillRect( 0, y, m_nEditorWidth, m_nGridHeight, alternateRowColor );
			}
		}
	}

	p.setPen( lineColor );
	p.drawLine(0, 0, 0, m_nEditorHeight);

	// horizontal lines
	for (uint i = 0; i < MAX_INSTRUMENTS; i++) {
		uint y = m_nGridHeight * i;
		p.setPen( lineColor );
		p.drawLine( 0, y, (100 + m_nEditorWidth), y);

		uint text_y = (m_nGridHeight * MAX_INSTRUMENTS) - (i * m_nGridHeight);
		string trackName = "";

		Instrument *instr = instrList->get(i);
		trackName = instr->m_sName;
		p.setPen( textColor );
		p.drawText( 5, text_y - 1 - m_nGridHeight, 100, m_nGridHeight + 2, Qt::AlignVCenter, trackName.c_str() );
	}

	p.setPen( lineColor );
	p.drawLine( 0, m_nGridHeight * MAX_INSTRUMENTS, m_nEditorWidth, m_nGridHeight * MAX_INSTRUMENTS);
	p.drawLine(m_nEditorWidth - 1 , 0, m_nEditorWidth - 1 , m_nEditorHeight);
}



void PatternEditorInstrumentList::updateEditor() {
	if(!isVisible()) {
		return;
	}

	Hydrogen* engine = Hydrogen::getInstance();

	// check engine state
	int state = engine->getState();
	if ( (state != STATE_READY) && (state != STATE_PLAYING) ) {
		errorLog( "[PatternEditorInstrumentList::updateEditor] FIXME: skip pattern editor update (state should be READY or PLAYING" );
		return;
	}

	createBackground( &m_background );
	m_bChanged = true;
	update();
}



void PatternEditorInstrumentList::mousePressEvent(QMouseEvent *ev) {
	int row = MAX_INSTRUMENTS - 1 - (ev->y()  / (int)m_nGridHeight);
	if (row >= MAX_INSTRUMENTS) {
		return;
	}

	Hydrogen::getInstance()->setSelectedInstrumentNumber( row );
	m_pPatternEditorPanel->setSelectedInstrument( row );
	updateEditor();
	m_pPatternEditorPanel->getPatternEditor()->updateEditor();
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPitchEditor()->updateEditor();

	if (ev->button() == LeftButton ) {
		const float velocity = 0.8f;
		const float pan_L = 1.0f;
		const float pan_R = 1.0f;
		const int nLength = -1;
		const float fPitch = 0.0f;
		Song *song = (Hydrogen::getInstance())->getSong();
		Instrument *instrRef = (song->getInstrumentList())->get(row);
		Note *pNote = new Note( instrRef, 0, velocity, pan_L, pan_R, nLength, fPitch);
		Hydrogen::getInstance()->noteOn( pNote );
	}
	else if (ev->button() == RightButton ) {
		m_pFunctionPopup->popup( QPoint( ev->globalX(), ev->globalY() ) );
	}
	return;

}



void PatternEditorInstrumentList::functionClearNotes()
{
	Hydrogen *engine = (Hydrogen::getInstance());
	engine->lockEngine("PatternEditorInstrumentList::functionClearNotes");	// lock the audio engine

	Pattern *pCurrentPattern = getCurrentPattern();
	SequenceList *pSequenceList = pCurrentPattern->m_pSequenceList;

	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	Sequence *seq = pSequenceList->get( nSelectedInstrument );

	int nPatternSize = pCurrentPattern->m_nSize;

	for (uint i = 0; i < nPatternSize; i++) {
		if (seq->m_noteList[i] != NULL) {
			Note *note = seq->m_noteList[i];
			delete note;
			seq->m_noteList[i] = NULL;
		}
	}
	engine->unlockEngine();	// unlock the audio engine

	m_bChanged = true;
	update();
	m_pPatternEditorPanel->getPatternEditor()->updateEditor(true);
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPitchEditor()->updateEditor();
}



void PatternEditorInstrumentList::functionFillNotes()
{
	Hydrogen *pEngine = (Hydrogen::getInstance());

	const float velocity = 0.8f;
	const float pan_L = 1.0f;
	const float pan_R = 1.0f;
	const float fPitch = 0.0f;
	const int nLength = -1;

	PatternEditor *pPatternEditor = m_pPatternEditorPanel->getPatternEditor();
	int nBase;
	if ( pPatternEditor->isUsingTriplets() ) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int nResolution = 4 * MAX_NOTES / ( nBase * pPatternEditor->getResolution() );


	pEngine->lockEngine("PatternEditorInstrumentList::functionFillNotes");	// lock the audio engine


	Song *pSong = pEngine->getSong();

	Pattern* pCurrentPattern = getCurrentPattern();
	if (pCurrentPattern != NULL) {

		SequenceList *pSequenceList = pCurrentPattern->m_pSequenceList;
		int nPatternSize = pCurrentPattern->m_nSize;
		int nSelectedInstrument = pEngine->getSelectedInstrumentNumber();

		if (nSelectedInstrument != -1) {
			Sequence *pSeq = pSequenceList->get( nSelectedInstrument );
			Instrument *instrRef = (pSong->getInstrumentList())->get( nSelectedInstrument );

			for ( uint i = 0; i < nPatternSize; i += nResolution ) {
				if ( pSeq->m_noteList[i] == NULL ) {
					
					// create the new note
					Note *pNote = new Note( instrRef, i, velocity, pan_L, pan_R, nLength, fPitch );
					pNote->setInstrument(instrRef);
					pSeq->m_noteList[i] = pNote;
				}
			}
		}
	}
	pEngine->unlockEngine();	// unlock the audio engine

	m_bChanged = true;
	update();
	m_pPatternEditorPanel->getPatternEditor()->updateEditor(true);
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPitchEditor()->updateEditor();
}



Pattern* PatternEditorInstrumentList::getCurrentPattern()
{
	Hydrogen *pEngine = (Hydrogen::getInstance());
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	assert( pPatternList != NULL );

	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( nSelectedPatternNumber != -1 ) {
		Pattern* pCurrentPattern = pPatternList->get( nSelectedPatternNumber );
		return pCurrentPattern;
	}
	return NULL;
}



void PatternEditorInstrumentList::functionMute()
{
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	Instrument *pInstr = instrList->get(nSelectedInstrument);
	pInstr->m_bIsMuted = !pInstr->m_bIsMuted;

	m_pPatternEditorPanel->getPatternEditor()->updateEditor( true );
}


void PatternEditorInstrumentList::functionLock()
{
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();

	Hydrogen *engine = Hydrogen::getInstance();
	Song *song = engine->getSong();
	InstrumentList *instrList = song->getInstrumentList();

	Instrument *pInstr = instrList->get(nSelectedInstrument);
	pInstr->m_bIsLocked = !pInstr->m_bIsLocked;

	m_pPatternEditorPanel->getPatternEditor()->updateEditor( true );
}



void PatternEditorInstrumentList::functionSolo()
{
	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	( (HydrogenApp::getInstance())->getMixer() )->soloClicked( nSelectedInstrument );
	m_pPatternEditorPanel->getPatternEditor()->updateEditor(true);
}




void PatternEditorInstrumentList::functionRandomizeVelocity()
{
	Hydrogen *engine = Hydrogen::getInstance();
	engine->lockEngine("PatternEditorInstrumentList::functionRandomizeVelocity");

	int nSelectedInstrument = engine->getSelectedInstrumentNumber();

	int nPattern = engine->getSelectedPatternNumber();
	Pattern *pPattern = engine->getSong()->getPatternList()->get( nPattern );

	Sequence *pSequence = pPattern->m_pSequenceList->get( nSelectedInstrument );
	for ( int i = 0; i < pPattern->m_nSize; i++ ){
		Note *pNote = pSequence->m_noteList[ i ];
		if ( pNote ) {
			float fVal = ( rand() % 100 ) / 100.0;
			fVal = pNote->m_fVelocity + ( ( fVal - 0.50 ) / 2 );
			if ( fVal < 0  ) {
				fVal = 0;
			}
			if ( fVal > 1 ) {
				fVal = 1;
			}
			pNote->m_fVelocity = fVal;
		}
	}
	engine->unlockEngine();

	engine->getSong()->m_bIsModified = true;

	m_pPatternEditorPanel->getPatternEditor()->updateEditor(true);
	m_pPatternEditorPanel->getVelocityEditor()->updateEditor();
	m_pPatternEditorPanel->getPitchEditor()->updateEditor();
}





// ::::::::::::::::::::::::::::




NotePropertiesRuler::NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode )
 : QWidget( parent , "NotePropertiesRuler", Qt::WRepaintNoErase | Qt::WResizeNoErase )
 , Object( "NotePropertiesRuler" )
 , m_mode( mode )
 , m_pPatternEditorPanel( pPatternEditorPanel )
 , m_pPattern( NULL )
 , m_bChanged( true )
{
	//infoLog("INIT");
	m_nGridWidth = (Preferences::getInstance())->getPatternEditorGridWidth();
	m_nEditorWidth = 5 + m_nGridWidth * ( MAX_NOTES * 4 ) + 5;

	if (m_mode == VELOCITY ) {
		m_nEditorHeight = 100;
	}
	else if ( m_mode == PITCH ) {
		m_nEditorHeight = 10 * m_nKeys;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumSize( m_nEditorWidth, m_nEditorHeight );
//	setMaximumSize( m_nEditorWidth, m_nEditorHeight );

	m_background.resize( m_nEditorWidth, m_nEditorHeight );

	updateEditor();
}



NotePropertiesRuler::~NotePropertiesRuler()
{
	//infoLog("DESTROY");
}



void NotePropertiesRuler::mousePressEvent(QMouseEvent *ev)
{
//	infoLog( "mousePressEvent()" );
	if (m_pPattern == NULL) return;

	PatternEditor *pPatternEditor = m_pPatternEditorPanel->getPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}
	int width = (m_nGridWidth * 4 *  MAX_NOTES) / ( nBase * pPatternEditor->getResolution());
	int x_pos = ev->x();
	int column;
	column = (x_pos - 10) + (width / 2);
	column = column / width;
	column = (column * 4 * MAX_NOTES) / ( nBase * pPatternEditor->getResolution() );
	float val = m_nEditorHeight - ev->y();
	if (val > height()) {
		val = height();
	}
	else if (val < 0.0) {
		val = 0.0;
	}
	val = val / height();

	int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();
	SequenceList *sequenceList = m_pPattern->m_pSequenceList;
	Sequence *seq = sequenceList->get( nSelectedInstrument );

	for (uint j = 0; j < m_pPattern->m_nSize; j++) {
		Note *note = seq->m_noteList[j];
		if ( (note != NULL) && ((int)(note->m_nPosition) == column) ) {

			if ( m_mode == VELOCITY ) {
				note->m_fVelocity = val;

				char valueChar[100];
				sprintf( valueChar, "%#.2f",  val);
				( HydrogenApp::getInstance() )->setStatusBarMessage( QString("Set note velocity [%1]").arg( valueChar ), 2000 );
			}
			else if ( m_mode == PITCH ){
				int nPitch = (int)( m_nKeys * val ) - m_nBasePitch;
				if ( note->m_fPitch == nPitch ) {
					return;
				}
				note->m_fPitch = nPitch;
			}

			Song *song = (Hydrogen::getInstance())->getSong();
			song->m_bIsModified = true;
			m_bChanged = true;
			updateEditor();
			break;
		}
	}
}



 void NotePropertiesRuler::mouseMoveEvent( QMouseEvent *ev )
{
//	infoLog( "mouse move" );
	mousePressEvent( ev );
}



void NotePropertiesRuler::paintEvent( QPaintEvent*) {
	if (!isVisible()) {
		return;
	}
	if (m_bChanged) {
		m_bChanged = false;
	}
	bitBlt(this, 0, 0, &m_background, 0, 0, width(), height(), CopyROP);
}



void NotePropertiesRuler::createVelocityBackground(QPixmap *pixmap)
{
	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();

	H2RGBColor valueColor(
			(int)( pStyle->m_patternEditor_backgroundColor.getRed() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getGreen() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getBlue() * ( 1 - 0.3 ) )
	);

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(), pStyle->m_patternEditor_backgroundColor.getGreen(), pStyle->m_patternEditor_backgroundColor.getBlue() );
	QColor horizLinesColor(
			pStyle->m_patternEditor_backgroundColor.getRed() - 20,
			pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
			pStyle->m_patternEditor_backgroundColor.getBlue() - 20
	);

	pixmap->fill( backgroundColor );
	QPainter p( pixmap );

	// vertical lines

	PatternEditor *pPatternEditor = m_pPatternEditorPanel->getPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}

	int n4th = 4 * MAX_NOTES / (nBase * 4);
	int n8th = 4 * MAX_NOTES / (nBase * 8);
	int n16th = 4 * MAX_NOTES / (nBase * 16);
	int n32th = 4 * MAX_NOTES / (nBase * 32);
	int n64th = 4 * MAX_NOTES / (nBase * 64);

	int nResolution = pPatternEditor->getResolution();

	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->m_nSize;
	}

	if ( !pPatternEditor->isUsingTriplets() ) {

		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 10 + i * m_nGridWidth;

			if ( (i % n4th) == 0 ) {
				if (nResolution >= 4) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
		}
	}
	else {	// Triplets
		uint nCounter = 0;
		int nSize = 4 * MAX_NOTES / (nBase * nResolution);

		for (uint i = 0; i < nNotes + 1; i++) {
			uint x = 10 + i * m_nGridWidth;

			if ( (i % nSize) == 0) {
				if ((nCounter % 3) == 0) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
				}
				p.drawLine(x, 0, x, m_nEditorHeight);
				nCounter++;
			}
		}
	}

	p.setPen( horizLinesColor );
	for (unsigned y = 0; y < m_nEditorHeight; y = y + (m_nEditorHeight / 10)) {
		p.drawLine(10, y, 10 + nNotes * m_nGridWidth, y);
	}


	if (m_pPattern != NULL) {
		SequenceList *sequenceList = m_pPattern->m_pSequenceList;
		int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();

		for (uint i = 0; i < sequenceList->getSize(); i++) {
			if ( (int)i == nSelectedInstrument ) {	// draw the velocity

				Sequence *seq = sequenceList->get(i);
				for (uint j = 0; j < nNotes; j++) {
					Note *note = seq->m_noteList[j];
					if (note != NULL) {
//						uint pos = note->m_nPosition % MAX_NOTES;
						uint pos = note->m_nPosition;
						uint x_pos = 10 + pos * m_nGridWidth;

						uint line_end = height();

						uint velocity = (uint)(note->m_fVelocity * height());
						uint line_start = line_end - velocity;

						QColor sideColor(
							(int)( valueColor.getRed() * ( 1 - note->m_fVelocity ) ),
							(int)( valueColor.getGreen() * ( 1 - note->m_fVelocity ) ),
							(int)( valueColor.getBlue() * ( 1 - note->m_fVelocity ) )
						);
//						p.setPen( sideColor );
						p.fillRect( (int)( x_pos - m_nGridWidth / 2.0 ) + 1, line_start, m_nGridWidth, line_end - line_start, sideColor );

						QColor centerColor(
							(int)( valueColor.getRed() * ( 1 - note->m_fVelocity ) ),
							(int)( valueColor.getGreen() * ( 1 - note->m_fVelocity ) ),
							(int)( valueColor.getBlue() * ( 1 - note->m_fVelocity ) )
						);
						int nLineWidth = (int)( m_nGridWidth / 2.0 );
						int nSpace = (int)( ( m_nGridWidth -nLineWidth ) / 2.0 );
						p.fillRect( (int)( x_pos - nSpace ) + 1, line_start, nLineWidth, line_end - line_start, centerColor );
					}
				}
			}
		}
	}
	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);


	m_bChanged = true;
}



void NotePropertiesRuler::createPitchBackground(QPixmap *pixmap) {
	UIStyle *pStyle = Preferences::getInstance()->getDefaultUIStyle();
	QColor backgroundColor( 255, 255, 255 );
	QColor blackKeysColor( 240, 240, 240 );
	QColor horizLinesColor(
			pStyle->m_patternEditor_backgroundColor.getRed() - 20,
			pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
			pStyle->m_patternEditor_backgroundColor.getBlue() - 20
	);
	H2RGBColor valueColor(
			(int)( pStyle->m_patternEditor_backgroundColor.getRed() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getGreen() * ( 1 - 0.3 ) ),
			(int)( pStyle->m_patternEditor_backgroundColor.getBlue() * ( 1 - 0.3 ) )
	);

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(), pStyle->m_patternEditor_line1Color.getGreen(), pStyle->m_patternEditor_line1Color.getBlue() );
	QColor res_2( pStyle->m_patternEditor_line2Color.getRed(), pStyle->m_patternEditor_line2Color.getGreen(), pStyle->m_patternEditor_line2Color.getBlue() );
	QColor res_3( pStyle->m_patternEditor_line3Color.getRed(), pStyle->m_patternEditor_line3Color.getGreen(), pStyle->m_patternEditor_line3Color.getBlue() );
	QColor res_4( pStyle->m_patternEditor_line4Color.getRed(), pStyle->m_patternEditor_line4Color.getGreen(), pStyle->m_patternEditor_line4Color.getBlue() );
	QColor res_5( pStyle->m_patternEditor_line5Color.getRed(), pStyle->m_patternEditor_line5Color.getGreen(), pStyle->m_patternEditor_line5Color.getBlue() );

	pixmap->fill( backgroundColor );

	QPainter p( pixmap );

	// keys
	int keySequence[] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

	float nKeyHeight = height() / m_nKeys;

	for ( int nKey = 0; nKey <= m_nKeys; nKey++ ) {
		int y = (int)( height() - nKeyHeight * (float)nKey );

		if ( keySequence[ nKey % 12 ] == 1 ) {
			p.fillRect( 0, y, m_nEditorWidth, -( (int)nKeyHeight), blackKeysColor );
		}

		p.setPen( horizLinesColor );
		p.drawLine(0, y, m_nEditorWidth, y);
	}


	// vertical lines
	PatternEditor *pPatternEditor = m_pPatternEditorPanel->getPatternEditor();
	int nBase;
	if (pPatternEditor->isUsingTriplets()) {
		nBase = 3;
	}
	else {
		nBase = 4;
	}

	int n4th = 4 * MAX_NOTES / (nBase * 4);
	int n8th = 4 * MAX_NOTES / (nBase * 8);
	int n16th = 4 * MAX_NOTES / (nBase * 16);
	int n32th = 4 * MAX_NOTES / (nBase * 32);
	int n64th = 4 * MAX_NOTES / (nBase * 64);

	int nResolution = pPatternEditor->getResolution();

	if ( !pPatternEditor->isUsingTriplets() ) {

		for (uint i = 0; i < MAX_NOTES + 1; i++) {
			uint x = 10 + i * m_nGridWidth;

			if ( (i % n4th) == 0 ) {
				if (nResolution >= 4) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n8th) == 0 ) {
				if (nResolution >= 8) {
					p.setPen( QPen( res_2, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n16th) == 0 ) {
				if (nResolution >= 16) {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n32th) == 0 ) {
				if (nResolution >= 32) {
					p.setPen( QPen( res_4, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
			else if ( (i % n64th) == 0 ) {
				if (nResolution >= 64) {
					p.setPen( QPen( res_5, 0, Qt::DotLine ) );
					p.drawLine(x, 0, x, m_nEditorHeight);
				}
			}
		}
	}
	else {	// Triplets
		uint nCounter = 0;
		int nSize = 4 * MAX_NOTES / (nBase * nResolution);

		for (uint i = 0; i < MAX_NOTES + 1; i++) {
			uint x = 10 + i * m_nGridWidth;

			if ( (i % nSize) == 0) {
				if ((nCounter % 3) == 0) {
					p.setPen( QPen( res_1, 0, Qt::DotLine ) );
				}
				else {
					p.setPen( QPen( res_3, 0, Qt::DotLine ) );
				}
				p.drawLine(x, 0, x, m_nEditorHeight);
				nCounter++;
			}
		}
	}



	if ( m_pPattern ) {
		SequenceList *sequenceList = m_pPattern->m_pSequenceList;
		int nSelectedInstrument = Hydrogen::getInstance()->getSelectedInstrumentNumber();

		for (uint i = 0; i < sequenceList->getSize(); i++) {
			if ( (int)i == nSelectedInstrument ) {	// draw the velocity

				Sequence *seq = sequenceList->get(i);
				for (uint j = 0; j < MAX_NOTES; j++) {
					Note *note = seq->m_noteList[j];
					if (note != NULL) {
						uint x_pos = 10 + note->m_nPosition * m_nGridWidth;

						int line_start = (int)( height() - ( ( note->m_fPitch + m_nBasePitch ) * nKeyHeight ) - nKeyHeight );

						QColor centerColor(
							(int)( valueColor.getRed() * ( 1 - note->m_fVelocity ) ),
							(int)( valueColor.getGreen() * ( 1 - note->m_fVelocity ) ),
							(int)( valueColor.getBlue() * ( 1 - note->m_fVelocity ) )
						);
						int nLineWidth = 10;
						p.fillRect( x_pos, line_start, nLineWidth, (int)nKeyHeight, centerColor );
					}
				}
			}
		}
	}

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);

	m_bChanged = true;
}



void NotePropertiesRuler::updateEditor()
{
//	infoLog( "[updateEditor]" );
	Hydrogen *pEngine = Hydrogen::getInstance();
	PatternList *pPatternList = pEngine->getSong()->getPatternList();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->getSize() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = NULL;
	}

	if ( m_mode == VELOCITY ) {
		createVelocityBackground(&m_background);
	}
	else if ( m_mode == PITCH ) {
		createPitchBackground(&m_background);
	}

	// redraw all
	update();
}



void NotePropertiesRuler::zoomIn()
{
	m_nGridWidth = m_nGridWidth * 2;
	m_bChanged = true;
	updateEditor();
}

void NotePropertiesRuler::zoomOut()
{
	m_nGridWidth = m_nGridWidth / 2;
	if (m_nGridWidth < 3) {
		m_nGridWidth = 3;
	}
	m_bChanged = true;
	updateEditor();
}


