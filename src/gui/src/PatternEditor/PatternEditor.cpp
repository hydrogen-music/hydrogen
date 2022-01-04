/*
 * Hydrogen
 * Copyright(c) 2002-2008 by the Hydrogen Team
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

#include "PatternEditor.h"

#include <core/Globals.h>
#include <core/Basics/Song.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/EventQueue.h>
#include <core/Basics/DrumkitComponent.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/InstrumentComponent.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Adsr.h>
#include <core/Basics/Note.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/Helpers/Xml.h>

#include "../HydrogenApp.h"
#include "../EventListener.h"
#include "PatternEditorPanel.h"
#include "UndoActions.h"


using namespace std;
using namespace H2Core;


PatternEditor::PatternEditor( QWidget *pParent,
							  PatternEditorPanel *panel )
	: Object()
	, QWidget( pParent )
	, m_selection( this )
	, m_bEntered( false )
	, m_nResolution( 8 )
	, m_bUseTriplets( false )
	, m_pDraggedNote( nullptr )
	, m_pPatternEditorPanel( panel )
	, m_pPattern( nullptr )
	, m_bSelectNewNotes( false )
	, m_bFineGrained( false )
	, m_bCopyNotMove( false ) {

	auto pPref = H2Core::Preferences::get_instance();
	
	m_fGridWidth = pPref->getPatternEditorGridWidth();
	m_nEditorWidth = m_nMargin + m_fGridWidth * ( MAX_NOTES * 4 );

	setFocusPolicy(Qt::StrongFocus);

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &PatternEditor::onPreferencesChanged );
	
	m_pAudioEngine = Hydrogen::get_instance()->getAudioEngine();

	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "&Cut" ), this, &PatternEditor::cut );
	m_pPopupMenu->addAction( tr( "&Copy" ), this, &PatternEditor::copy );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, &PatternEditor::paste );
	m_pPopupMenu->addAction( tr( "&Delete" ), this, &PatternEditor::deleteSelection );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, &PatternEditor::selectAll );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, &PatternEditor::selectNone );


}

void PatternEditor::onPreferencesChanged( H2Core::Preferences::Changes changes ) {
	auto pPref = H2Core::Preferences::get_instance();

	if ( changes & H2Core::Preferences::Changes::Colors ) {
		
		update( 0, 0, width(), height() );
		m_pPatternEditorPanel->updateEditors();
	}
}

void PatternEditor::setResolution(uint res, bool bUseTriplets)
{
	this->m_nResolution = res;
	this->m_bUseTriplets = bUseTriplets;

	// redraw all
	update( 0, 0, width(), height() );
	m_pPatternEditorPanel->updateEditors();
}

void PatternEditor::zoomIn()
{
	if (m_fGridWidth >= 3) {
		m_fGridWidth *= 2;
	} else {
		m_fGridWidth *= 1.5;
	}
	updateEditor();
}

void PatternEditor::zoomOut()
{
	if ( m_fGridWidth > 1.5 ) {
		if (m_fGridWidth > 3) {
			m_fGridWidth /= 2;
		} else {
			m_fGridWidth /= 1.5;
		}
		updateEditor();
	}
}

QColor PatternEditor::computeNoteColor( float velocity ) {
	int red;
	int green;
	int blue;


	/*
	The note gets painted black if it has the default velocity (0.8).
	The color changes if you alter the velocity..
	*/

	//qDebug() << "x: " << x;
	//qDebug() << "x2: " << x*x;


	if( velocity < 0.8){
		red = fabs(-( velocity - 0.8))*255;
		green =  fabs(-( velocity - 0.8))*255;
		blue =  green * 1.25;
	} else {
		green = blue = 0;
		red = (velocity-0.8)*5*255;
	}

	//qDebug() << "R " << red << "G " << green << "blue " << blue;
	return QColor( red, green, blue );
}


void PatternEditor::drawNoteSymbol( QPainter &p, QPoint pos, H2Core::Note *pNote, bool bIsForeground ) const
{
	auto pPref = H2Core::Preferences::get_instance();
	
	static const QColor noteColor( pPref->getColorTheme()->m_patternEditor_noteColor );
	static const QColor noteoffColor( pPref->getColorTheme()->m_patternEditor_noteoffColor );

	p.setRenderHint( QPainter::Antialiasing );

	QColor color = computeNoteColor( pNote->get_velocity() );

	uint w = 8, h =  8;
	uint x_pos = pos.x(), y_pos = pos.y();

	bool bSelected = m_selection.isSelected( pNote );

	if ( bSelected ) {
		QPen selectedPen( selectedNoteColor() );
		selectedPen.setWidth( 2 );
		p.setPen( selectedPen );
		p.setBrush( Qt::NoBrush );
	}

	bool bMoving = bSelected && m_selection.isMoving();
	QPen movingPen( noteColor );
	QPoint movingOffset;

	if ( bMoving ) {
		movingPen.setStyle( Qt::DotLine );
		movingPen.setWidth( 2 );
		QPoint delta = movingGridOffset();
		movingOffset = QPoint( delta.x() * m_fGridWidth,
							   delta.y() * m_nGridHeight );
	}

	if ( pNote->get_note_off() == false ) {	// trigger note
		int width = w;

		QBrush noteBrush( color );
		QPen notePen( noteColor );
		if ( !bIsForeground ) {
			noteBrush.setStyle( Qt::Dense4Pattern );
			notePen.setStyle( Qt::DotLine );
		}

		if ( bSelected ) {
			p.drawEllipse( x_pos -4 -2, y_pos-2, w+4, h+4 );
		}

		// Draw tail
		if ( pNote->get_length() != -1 ) {
			float fNotePitch = pNote->get_octave() * 12 + pNote->get_key();
			float fStep = Note::pitchToFrequency( ( double )fNotePitch );
			width = m_fGridWidth * pNote->get_length() / fStep;
			width = width - 1;	// lascio un piccolo spazio tra una nota ed un altra

			if ( bSelected ) {
				p.drawRoundedRect( x_pos-2, y_pos, width+4, 3+4, 4, 4 );
			}
			p.setPen( notePen );
			p.setBrush( noteBrush );
			p.fillRect( x_pos, y_pos +2, width, 3, color );	/// \todo: definire questo colore nelle preferenze
			p.drawRect( x_pos, y_pos +2, width, 3 );
			p.drawLine( x_pos+width, y_pos, x_pos+width, y_pos + h );
		}

		p.setPen( notePen );
		p.setBrush( noteBrush );
		p.drawEllipse( x_pos -4 , y_pos, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + x_pos -4 -2, movingOffset.y() + y_pos -2 , w + 4, h + 4 );
			// Moving tail
			if ( pNote->get_length() != -1 ) {
				p.setPen( movingPen );
				p.setBrush( Qt::NoBrush );
				p.drawRoundedRect( movingOffset.x() + x_pos-2, movingOffset.y() + y_pos, width+4, 3+4, 4, 4 );
			}
		}
	}
	else if ( pNote->get_length() == 1 && pNote->get_note_off() == true ) {

		if ( bSelected ) {
			p.drawEllipse( x_pos -4 -2, y_pos-2, w+4, h+4 );
		}

		QBrush noteOffBrush( noteoffColor );
		if ( !bIsForeground ) {
			noteOffBrush.setStyle( Qt::Dense4Pattern );
		}

		p.setPen( Qt::NoPen );
		p.setBrush( noteOffBrush );
		p.drawEllipse( x_pos -4 , y_pos, w, h );

		if ( bMoving ) {
			p.setPen( movingPen );
			p.setBrush( Qt::NoBrush );
			p.drawEllipse( movingOffset.x() + x_pos -4 -2, movingOffset.y() + y_pos -2, w + 4, h + 4 );
		}
	}
}


int PatternEditor::getColumn( int x, bool bUseFineGrained ) const
{
	int nGranularity = 1;
	if ( !( bUseFineGrained && m_bFineGrained ) ) {
		nGranularity = granularity();
	}
	int nWidth = m_fGridWidth * nGranularity;
	int nColumn = ( x - m_nMargin + (nWidth / 2) ) / nWidth;
	nColumn = nColumn * nGranularity;
	if ( nColumn < 0 ) {
		return 0;
	} else {
		return nColumn;
	}
}

void PatternEditor::selectNone()
{
	m_selection.clearSelection();
	m_selection.updateWidgetGroup();
}

///
/// Copy selection to clipboard in XML
///
void PatternEditor::copy()
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	InstrumentList *pInstrumentList = pHydrogen->getSong()->getInstrumentList();
	XMLDoc doc;
	XMLNode selection = doc.set_root( "noteSelection" );
	XMLNode noteList = selection.createNode( "noteList");
	XMLNode positionNode = selection.createNode( "sourcePosition" );
	bool bWroteNote = false;
	// "Top left" of selection, in the three dimensional time*instrument*pitch space.
	int nLowestPos, nLowestInstrument, nHighestPitch;

	for ( Note *pNote : m_selection ) {
		int nPitch = pNote->get_notekey_pitch() + 12*OCTAVE_OFFSET;
		int nPos = pNote->get_position();
		int nInstrument = pInstrumentList->index( pNote->get_instrument() );
		if ( bWroteNote ) {
			nLowestPos = std::min( nPos, nLowestPos );
			nLowestInstrument = std::min( nInstrument, nLowestInstrument );
			nHighestPitch = std::max( nPitch, nHighestPitch );
		} else {
			nLowestPos = nPos;
			nLowestInstrument = nInstrument;
			nHighestPitch = nPitch;
			bWroteNote = true;
		}
		XMLNode note_node = noteList.createNode( "note" );
		pNote->save_to( &note_node );
	}

	if ( bWroteNote ) {
		positionNode.write_int( "position", nLowestPos );
		positionNode.write_int( "instrument", nLowestInstrument );
		positionNode.write_int( "note", nHighestPitch );
	} else {
		positionNode.write_int( "position", m_pPatternEditorPanel->getCursorPosition() );
		positionNode.write_int( "instrument", pHydrogen->getSelectedInstrumentNumber() );
	}

	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText( doc.toString() );

	// This selection will probably be pasted at some point. So show the keyboard cursor as this is the place
	// where the selection will be pasted.
	HydrogenApp::get_instance()->setHideKeyboardCursor( false );
}


void PatternEditor::cut()
{
	copy();
	deleteSelection();
}


void PatternEditor::selectInstrumentNotes( int nInstrument )
{
	InstrumentList *pInstrumentList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	auto pInstrument = pInstrumentList->get( nInstrument );

	m_selection.clearSelection();
	FOREACH_NOTE_CST_IT_BEGIN_END(m_pPattern->get_notes(), it) {
		if ( it->second->get_instrument() == pInstrument ) {
			m_selection.addToSelection( it->second );
		}
	}
	m_selection.updateWidgetGroup();
}

void PatternEditor::setCurrentInstrument( int nInstrument ) {
	Hydrogen::get_instance()->setSelectedInstrumentNumber( nInstrument );
	m_pPatternEditorPanel->updateEditors();
}

void PatternEditor::mousePressEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mousePressEvent( ev );
}

void PatternEditor::mouseMoveEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	if ( m_selection.isMoving() ) {
		updateEditor( true );
	}
	m_selection.mouseMoveEvent( ev );
}

void PatternEditor::mouseReleaseEvent( QMouseEvent *ev )
{
	updateModifiers( ev );
	m_selection.mouseReleaseEvent( ev );
}

void PatternEditor::updateModifiers( QInputEvent *ev ) {
	// Key: Alt + drag: move notes with fine-grained positioning
	m_bFineGrained = ev->modifiers() & Qt::AltModifier;
	// Key: Ctrl + drag: copy notes rather than moving
	m_bCopyNotMove = ev->modifiers() & Qt::ControlModifier;

	if ( QKeyEvent *pEv = dynamic_cast<QKeyEvent*>( ev ) ) {
		// Keyboard events for press and release of modifier keys don't have those keys in the modifiers set,
		// so explicitly update these.
		bool bPressed = ev->type() == QEvent::KeyPress;
		if ( pEv->key() == Qt::Key_Control ) {
			m_bCopyNotMove = bPressed;
		} else if ( pEv->key() == Qt::Key_Alt ) {
			m_bFineGrained = bPressed;
		}
	}

	if ( m_selection.isMouseGesture() && m_selection.isMoving() ) {
		// If a selection is currently being moved, change the cursor
		// appropriately. Selection will change it back after the move
		// is complete (or abandoned)
		if ( m_bCopyNotMove &&  cursor().shape() != Qt::DragCopyCursor ) {
			setCursor( QCursor( Qt::DragCopyCursor ) );
		} else if ( !m_bCopyNotMove && cursor().shape() != Qt::DragMoveCursor ) {
			setCursor( QCursor( Qt::DragMoveCursor ) );
		}
	}
}

bool PatternEditor::notesMatchExactly( Note *pNoteA, Note *pNoteB ) const {
	return ( pNoteA->match( pNoteB->get_instrument(), pNoteB->get_key(), pNoteB->get_octave() )
			 && pNoteA->get_position() == pNoteB->get_position()
			 && pNoteA->get_velocity() == pNoteB->get_velocity()
			 && pNoteA->getPan() == pNoteB->getPan()
			 && pNoteA->get_lead_lag() == pNoteB->get_lead_lag()
			 && pNoteA->get_probability() == pNoteB->get_probability() );
}

bool PatternEditor::checkDeselectElements( std::vector<SelectionIndex> &elements )
{
	//	Hydrogen *pH = Hydrogen::get_instance();
	std::set< Note *> duplicates;
	for ( Note *pNote : elements ) {
		if ( duplicates.find( pNote ) != duplicates.end() ) {
			// Already marked pNote as a duplicate of some other pNote. Skip it.
			continue;
		}
		FOREACH_NOTE_CST_IT_BOUND( m_pPattern->get_notes(), it, pNote->get_position() ) {
			// Duplicate note of a selected note is anything occupying the same position. Multiple notes
			// sharing the same location might be selected; we count these as duplicates too. They will appear
			// in both the duplicates and selection lists.
			if ( it->second != pNote && pNote->match( it->second ) ) {
				duplicates.insert( it->second );
			}
		}
	}
	if ( !duplicates.empty() ) {
		Preferences *pPreferences = Preferences::get_instance();
		bool bOk = true;

		if ( pPreferences->getShowNoteOverwriteWarning() ) {
			m_selection.cancelGesture();
			QString sMsg ( tr( "Placing these notes here will overwrite %1 duplicate notes." ) );
			QMessageBox messageBox ( QMessageBox::Warning, "Hydrogen", sMsg.arg( duplicates.size() ),
									 QMessageBox::Cancel | QMessageBox::Ok, this );
			messageBox.setCheckBox( new QCheckBox( tr( "Don't show this message again" ) ) );
			messageBox.checkBox()->setChecked( false );
			bOk = messageBox.exec() == QMessageBox::Ok;
			if ( messageBox.checkBox()->isChecked() ) {
				pPreferences->setShowNoteOverwriteWarning( false );
			}
		}

		if ( bOk ) {
			Hydrogen *pHydrogen = Hydrogen::get_instance();
			InstrumentList *pInstrumentList = pHydrogen->getSong()->getInstrumentList();
			QUndoStack *pUndo = HydrogenApp::get_instance()->m_pUndoStack;

			std::vector< Note *>overwritten;
			for ( Note *pNote : duplicates ) {
				overwritten.push_back( pNote );
			}
			pUndo->push( new SE_deselectAndOverwriteNotesAction( elements, overwritten ) );

		} else {
			return false;
		}
	}
	return true;
}


void PatternEditor::deselectAndOverwriteNotes( std::vector< H2Core::Note *> &selected,
											   std::vector< H2Core::Note *> &overwritten )
{
	// Iterate over all the notes in 'selected' and 'overwrite' by erasing any *other* notes occupying the
	// same position.
	m_pAudioEngine->lock( RIGHT_HERE );
	Pattern::notes_t *pNotes = const_cast< Pattern::notes_t *>( m_pPattern->get_notes() );
	for ( auto pSelectedNote : selected ) {
		m_selection.removeFromSelection( pSelectedNote, /* bCheck=*/false );
		bool bFoundExact = false;
		int nPosition = pSelectedNote->get_position();
		for ( auto it = pNotes->lower_bound( nPosition ); it != pNotes->end() && it->first == nPosition; ) {
			Note *pNote = it->second;
			if ( !bFoundExact && notesMatchExactly( pNote, pSelectedNote ) ) {
				// Found an exact match. We keep this.
				bFoundExact = true;
				++it;
			} else if ( pSelectedNote->match( pNote ) && pNote->get_position() == pSelectedNote->get_position() ) {
				// Something else occupying the same position (which may or may not be an exact duplicate)
				it = pNotes->erase( it );
			} else {
				// Any other note
				++it;
			}
		}
	}
	Hydrogen::get_instance()->setIsModified( true );
	m_pAudioEngine->unlock();
}


void PatternEditor::undoDeselectAndOverwriteNotes( std::vector< H2Core::Note *> &selected,
												   std::vector< H2Core::Note *> &overwritten )
{
	// Restore previously-overwritten notes, and select notes that were selected before.
	m_selection.clearSelection( /* bCheck=*/false );
	m_pAudioEngine->lock( RIGHT_HERE );
	for ( auto pNote : overwritten ) {
		Note *pNewNote = new Note( pNote );
		m_pPattern->insert_note( pNewNote );
	}
	// Select the previously-selected notes
	for ( auto pNote : selected ) {
		FOREACH_NOTE_CST_IT_BOUND( m_pPattern->get_notes(), it, pNote->get_position() ) {
			if ( notesMatchExactly( it->second, pNote ) ) {
				m_selection.addToSelection( it->second );
				break;
			}
		}
	}
	Hydrogen::get_instance()->setIsModified( true );
	m_pAudioEngine->unlock();
	m_pPatternEditorPanel->updateEditors();
}


void PatternEditor::updatePatternInfo() {
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::shared_ptr<Song> pSong = pHydrogen->getSong();

	m_pPattern = nullptr;
	m_nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();

	if ( pSong ) {
		PatternList *pPatternList = pSong->getPatternList();
		if ( ( m_nSelectedPatternNumber != -1 ) && ( m_nSelectedPatternNumber < pPatternList->size() ) ) {
			m_pPattern = pPatternList->get( m_nSelectedPatternNumber );
		}
	}
}


QPoint PatternEditor::movingGridOffset( ) const {
	QPoint rawOffset = m_selection.movingOffset();
	// Quantize offset to multiples of m_nGrid{Width,Height}
	int nQuantX = m_fGridWidth, nQuantY = m_nGridHeight;
	float nFactor = 1;
	if ( ! m_bFineGrained ) {
		nFactor = granularity();
		nQuantX = m_fGridWidth * nFactor;
	}
	int x_bias = nQuantX / 2, y_bias = nQuantY / 2;
	if ( rawOffset.y() < 0 ) {
		y_bias = -y_bias;
	}
	if ( rawOffset.x() < 0 ) {
		x_bias = -x_bias;
	}
	int x_off = (rawOffset.x() + x_bias) / nQuantX;
	int y_off = (rawOffset.y() + y_bias) / nQuantY;
	return QPoint( nFactor * x_off, y_off);
}


//! Draw lines for note grid.
void PatternEditor::drawGridLines( QPainter &p, Qt::PenStyle style ) const
{
	auto pPref = H2Core::Preferences::get_instance();
	static const QColor res[5] = {
		QColor( pPref->getColorTheme()->m_patternEditor_line1Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line2Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line3Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line4Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line5Color ),
	};

	int nGranularity = granularity() * m_nResolution;
	int nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	}
	int nMaxX = m_fGridWidth * nNotes + m_nMargin;

	if ( !m_bUseTriplets ) {

		// Draw vertical lines. To minimise pen colour changes (and
		// avoid unnecessary division operations), we draw them in
		// multiple passes, of successively finer spacing (and
		// advancing the colour selection at each refinement) until
		// we've drawn enough to satisfy the resolution setting.
		//
		// The drawing sequence looks something like:
		// |       |       |       |         - first pass, all 1/4 notes
		// |   :   |   :   |   :   |   :     - second pass, odd 1/8th notes
		// | . : . | . : . | . : . | . : .   - third pass, odd 1/16th notes

		uint nRes = 4;
		float fStep = nGranularity / nRes * m_fGridWidth;

		// First, quarter note markers. All the quarter note markers must be drawn.
		if ( m_nResolution >= nRes ) {
			p.setPen( QPen( res[ 0 ], 0, style ) );
			for ( float x = m_nMargin ; x < nMaxX; x += fStep ) {
				p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			}
		}
		nRes *= 2;
		fStep /= 2;

		// For each successive set of finer-spaced lines, the even
		// lines will have already been drawn at the previous coarser
		// pitch, so only the odd numbered lines need to be drawn.
		int nColour = 1;
		while ( m_nResolution >= nRes ) {
			p.setPen( QPen( res[ nColour++ ], 0, style ) );
			for ( float x = m_nMargin + fStep; x < nMaxX; x += fStep * 2) {
				p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			}
			nRes *= 2;
			fStep /= 2;
		}

	} else {

		// Triplet style markers, we only differentiate colours on the
		// first of every triplet.
		float fStep = granularity() * m_fGridWidth;
		p.setPen(  QPen( res[ 0 ], 0, style ) );
		for ( float x = m_nMargin; x < nMaxX; x += fStep * 3 ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
		}
		// Second and third marks
		p.setPen(  QPen( res[ 2 ], 0, style ) );
		for ( float x = m_nMargin + fStep; x < nMaxX; x += fStep * 3 ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + fStep, 1, x + fStep, m_nEditorHeight - 1);
		}
	}

}


QColor PatternEditor::selectedNoteColor() const {
	
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( hasFocus() ) {
		static const QColor selectHilightColor( pPref->getColorTheme()->m_selectionHighlightColor );
		return selectHilightColor;
	} else {
		static const QColor selectInactiveColor( pPref->getColorTheme()->m_selectionInactiveColor );
		return selectInactiveColor;
	}
}


///
/// Ensure selection only refers to valid notes, and does not contain any stale references to deleted notes.
///
void PatternEditor::validateSelection()
{
	// Rebuild selection from valid notes.
	std::set<Note *> valid;
	std::vector< Note *> invalidated;
	FOREACH_NOTE_CST_IT_BEGIN_END(m_pPattern->get_notes(), it) {
		if ( m_selection.isSelected( it->second ) ) {
			valid.insert( it->second );
		}
	}
	for (auto i : m_selection ) {
		if ( valid.find(i) == valid.end()) {
			// Keep the note to invalidate, but don't remove from the selection while walking the selection
			// set.
			invalidated.push_back( i );
		}
	}
	for ( auto i : invalidated ) {
		m_selection.removeFromSelection( i, /* bCheck=*/false );
	}
}

void PatternEditor::scrolled( int nValue ) {
	UNUSED( nValue );
	update();
}

void PatternEditor::enterEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void PatternEditor::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;
}


//! Get notes to show in pattern editor.
//! This may include "background" notes that are in currently-playing patterns
//! rather than the current pattern.
std::vector< Pattern *> PatternEditor::getPatternsToShow( void )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::vector<Pattern *> patterns;

	// Add stacked-mode patterns
	if ( pHydrogen->getSong()->getMode() == Song::Mode::Pattern ) {
		if ( !Preferences::get_instance()->patternModePlaysSelected() ) {
			m_pAudioEngine->lock( RIGHT_HERE );
			std::set< Pattern *> patternSet;
			for ( PatternList *pPatternList : { m_pAudioEngine->getPlayingPatterns(),
						                        m_pAudioEngine->getNextPatterns() } ) {
				for ( int i = 0; i <  pPatternList->size(); i++) {
					Pattern *pPattern = pPatternList->get( i );
					if ( pPattern != m_pPattern ) {
						patternSet.insert( pPattern );
					}
				}
			}
			for ( Pattern *pPattern : patternSet ) {
				patterns.push_back( pPattern );
			}
			m_pAudioEngine->unlock();
		}
	}

	if ( m_pPattern ) {
		patterns.push_back( m_pPattern );
	}

	return patterns;
}


void PatternEditor::songModeActivationEvent( int nValue )
{
	UNUSED( nValue );
	// May need to draw (or hide) other background patterns
	update();
}

void PatternEditor::stackedModeActivationEvent( int nValue )
{
	UNUSED( nValue );
	// May need to draw (or hide) other background patterns
	update();
}
