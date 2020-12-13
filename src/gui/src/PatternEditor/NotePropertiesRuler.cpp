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
#include <core/Basics/Note.h>
using namespace H2Core;

#include <cassert>

#include "../HydrogenApp.h"

#include "UndoActions.h"
#include "NotePropertiesRuler.h"
#include "PatternEditorPanel.h"
#include "DrumPatternEditor.h"
#include "PianoRollEditor.h"

const char* NotePropertiesRuler::__class_name = "NotePropertiesRuler";

NotePropertiesRuler::NotePropertiesRuler( QWidget *parent, PatternEditorPanel *pPatternEditorPanel, NotePropertiesMode mode )
	: PatternEditor( parent, __class_name, pPatternEditorPanel )
{
	//infoLog("INIT");
	//setAttribute(Qt::WA_NoBackground);

	m_Mode = mode;

	m_nGridWidth = (Preferences::get_instance())->getPatternEditorGridWidth();
	m_nEditorWidth = m_nMargin + m_nGridWidth * ( MAX_NOTES * 4 );

	m_fLastSetValue = 0.0;
	m_bValueHasBeenSet = false;

	if (m_Mode == VELOCITY ) {
		m_nEditorHeight = 100;
	}
	else if ( m_Mode == PAN ) {
		m_nEditorHeight = 100;
	}
	else if ( m_Mode == LEADLAG ) {
		m_nEditorHeight = 100;
	}
	else if ( m_Mode == NOTEKEY ) {
		m_nEditorHeight = 210;
	}
	if (m_Mode == PROBABILITY ) {
		m_nEditorHeight = 100;
	}

	resize( m_nEditorWidth, m_nEditorHeight );
	setMinimumSize( m_nEditorWidth, m_nEditorHeight );

	m_pBackground = new QPixmap( m_nEditorWidth, m_nEditorHeight );

	updateEditor();
	show();

	HydrogenApp::get_instance()->addEventListener( this );

	setFocusPolicy( Qt::StrongFocus );

	// Generic pattern editor menu contains some operations that don't apply here, and we will want to add
	// menu options specific to this later.
	delete m_pPopupMenu;
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, &PatternEditor::selectAll );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, &PatternEditor::selectNone );

}




NotePropertiesRuler::~NotePropertiesRuler()
{
	//infoLog("DESTROY");
}


void NotePropertiesRuler::wheelEvent(QWheelEvent *ev )
{

	if (m_pPattern == nullptr) return;

	prepareUndoAction( ev->x() ); //get all old values

	float delta;
	if (ev->modifiers() == Qt::ControlModifier) {
		delta = 0.01; // fine control
	} else {
		delta = 0.05; // course control
	}
		
	if ( ev->delta() < 0 ) {
		delta = (delta * -1.0);
	}

	int width = m_nGridWidth * granularity();
	int x_pos = ev->x();
	int column;
	column = (x_pos - m_nMargin) + (width / 2);
	column = (column / width) * granularity();

	m_pPatternEditorPanel->setCursorPosition( column );
	m_pPatternEditorPanel->setCursorHidden( true );

	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = Hydrogen::get_instance()->getSong();

	const Pattern::notes_t* notes = m_pPattern->get_notes();
	FOREACH_NOTE_CST_IT_BOUND(notes,it,column) {
		Note *pNote = it->second;
		assert( pNote );
		assert( (int)pNote->get_position() == column );
		if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
			continue;
		}
		if ( m_Mode == VELOCITY && !pNote->get_note_off() ) {
			float val = pNote->get_velocity() + delta;
			if (val > 1.0) {
				val = 1.0;
			}
			else if (val < 0.0) {
				val = 0.0;
			}

			pNote->set_velocity(val);
			__velocity = val;
			m_fLastSetValue = val;
			m_bValueHasBeenSet = true;

			char valueChar[100];
			sprintf( valueChar, "%#.2f",  val);
			( HydrogenApp::get_instance() )->setStatusBarMessage( QString("Set note velocity [%1]").arg( valueChar ), 2000 );
		}
		else if ( m_Mode == PAN && !pNote->get_note_off() ){
			float pan_L, pan_R;

			float val = (pNote->get_pan_r() - pNote->get_pan_l() + 0.5) + delta;
			if (val > 1.0) {
				val = 1.0;
			}
			else if (val < 0.0) {
				val = 0.0;
			}
			if ( val > 0.5 ) {
				pan_L = 1.0 - val;
				pan_R = 0.5;
			}
			else {
				pan_L = 0.5;
				pan_R = val;
			}
			m_fLastSetValue = val;
			m_bValueHasBeenSet = true;
			pNote->set_pan_l(pan_L);
			pNote->set_pan_r(pan_R);
		}
		else if ( m_Mode == LEADLAG ){
			float val = (pNote->get_lead_lag() - 1.0)/-2.0 + delta;
			if (val > 1.0) {
				val = 1.0;
			}
			else if (val < 0.0) {
				val = 0.0;
			}
			m_fLastSetValue = val * -2.0 + 1.0;
			m_bValueHasBeenSet = true;
			pNote->set_lead_lag((val * -2.0) + 1.0);
			char valueChar[100];
			if (pNote->get_lead_lag() < 0.0) {
				sprintf( valueChar, "%.2f",  ( pNote->get_lead_lag() * -5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
				HydrogenApp::get_instance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
			} else if (pNote->get_lead_lag() > 0.0) {
				sprintf( valueChar, "%.2f",  ( pNote->get_lead_lag() * 5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
				HydrogenApp::get_instance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
			} else {
				HydrogenApp::get_instance()->setStatusBarMessage( QString("Note on beat"), 2000 );
			}
		}
		else if ( m_Mode == PROBABILITY && !pNote->get_note_off() ) {
			float val = pNote->get_probability() + delta;
			if (val > 1.0) {
				val = 1.0;
			}
			else if (val < 0.0) {
				val = 0.0;
			}

			m_fLastSetValue = val;
			m_bValueHasBeenSet = true;
			pNote->set_probability(val);
			__probability = val;

		}
		pSong->set_is_modified( true );
		addUndoAction();
		updateEditor();
		break;
	}
}


void NotePropertiesRuler::mouseClickEvent( QMouseEvent *ev ) {
	if ( ev->button() == Qt::RightButton ) {
		m_pPopupMenu->popup( ev->globalPos() );

	} else {
		propertyAdjustStart( ev );
		propertyAdjustUpdate( ev );
		propertyAdjustEnd( ev );
	}
}

void NotePropertiesRuler::mouseDragStartEvent( QMouseEvent *ev ) {
	propertyAdjustStart( ev );
}

void NotePropertiesRuler::mouseDragUpdateEvent( QMouseEvent *ev ) {
	propertyAdjustUpdate( ev );
}

void NotePropertiesRuler::mouseDragEndEvent( QMouseEvent *ev ) {
	propertyAdjustEnd( ev );
}

void NotePropertiesRuler::propertyAdjustStart( QMouseEvent *ev )
{
	prepareUndoAction( ev->x() );
	mouseMoveEvent( ev );
	updateEditor();
}


// Preserve current note properties at position x for use in later UndoAction.
void NotePropertiesRuler::prepareUndoAction( int x )
{

	//create all needed old vars for undo
	if (m_pPattern == nullptr) {
		return;
	}

	__oldVelocity = 0.8f;
	__oldPan_L = 0.5f;
	__oldPan_R = 0.5f;
	__oldLeadLag = 0.0f;
	__oldNoteKeyVal = 10;

	int width = m_nGridWidth * granularity();
	int x_pos = x;
	int column;
	column = (x_pos - m_nMargin) + (width / 2);
	column = (column / width) * granularity();

	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = (Hydrogen::get_instance())->getSong();

	__nSelectedInstrument = nSelectedInstrument;
	__undoColumn = 	column;

	const Pattern::notes_t* notes = m_pPattern->get_notes();
	FOREACH_NOTE_CST_IT_BOUND(notes,it,column) {
		Note *pNote = it->second;
		assert( pNote );
		assert( (int)pNote->get_position() == column );
		if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
			continue;
		}

		if ( m_Mode == VELOCITY && !pNote->get_note_off() ) {
			__oldVelocity = pNote->get_velocity();
			__mode = "VELOCITY";
		}
		else if ( m_Mode == PAN && !pNote->get_note_off() ){

			__oldPan_L = pNote->get_pan_l();
			__oldPan_R = pNote->get_pan_r();
			__mode = "PAN";
		}
		else if ( m_Mode == LEADLAG ){
			
			__oldLeadLag = pNote->get_lead_lag();
			__mode = "LEADLAG";
		}

		else if ( m_Mode == NOTEKEY ){
			__mode = "NOTEKEY";
		__oldOctaveKeyVal = pNote->get_octave();
		__oldNoteKeyVal = pNote->get_key();
		}
		else if ( m_Mode == PROBABILITY && !pNote->get_note_off() ) {
			__oldProbability = pNote->get_probability();
			__mode = "PROBABILITY";

		}

	}
}

void NotePropertiesRuler::propertyAdjustUpdate( QMouseEvent *ev )
{
	__velocity = 0.8f;
	__pan_L = 0.5f;
	__pan_R = 0.5f;
	__leadLag = 0.0f ;
	__noteKeyVal = 10;

	if (m_pPattern == nullptr) return;

	int width = m_nGridWidth * granularity();
	int x_pos = ev->x();
	int column;
	column = (x_pos - m_nMargin) + (width / 2);
	column = column / width;
	column = column * granularity();

	m_pPatternEditorPanel->setCursorPosition( column );
	m_pPatternEditorPanel->setCursorHidden( true );

	bool columnChange = false;
	if( __columnCheckOnXmouseMouve != column ){
		__undoColumn = column;
		columnChange = true;
	}

	float val = height() - ev->y();
	if (val > height()) {
		val = height();
	}
	else if (val < 0.0) {
		val = 0.0;
	}
	int keyval = val;
	val = val / height();

	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Song *pSong = (Hydrogen::get_instance())->getSong();

	const Pattern::notes_t* notes = m_pPattern->get_notes();
	FOREACH_NOTE_CST_IT_BOUND(notes,it,column) {
		Note *pNote = it->second;
		assert( pNote );
		assert( (int)pNote->get_position() == column );
		if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
			continue;
		}
		if ( m_Mode == VELOCITY && !pNote->get_note_off() ) {
			if( columnChange ){
				__oldVelocity = pNote->get_velocity();
			}
			pNote->set_velocity( val );
			m_fLastSetValue = val;
			m_bValueHasBeenSet = true;
			__velocity = val;
			char valueChar[100];
			sprintf( valueChar, "%#.2f",  val);
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Set note velocity [%1]").arg( valueChar ), 2000 );
		}
		else if ( m_Mode == PAN && !pNote->get_note_off() ){
			float pan_L, pan_R;
			if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				val = 0.5;
			}
			if ( val > 0.5 ) {
				pan_L = 1.0 - val;
				pan_R = 0.5;
			}
			else {
				pan_L = 0.5;
				pan_R = val;
			}
			
			if( columnChange ){
				__oldPan_L = pNote->get_pan_l();
				__oldPan_R = pNote->get_pan_r();
			}
			m_fLastSetValue = val;
			m_bValueHasBeenSet = true;
			pNote->set_pan_l( pan_L );
			pNote->set_pan_r( pan_R );
			__pan_L = pan_L;
			__pan_R = pan_R;
		}
		else if ( m_Mode == LEADLAG ){
			if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				pNote->set_lead_lag(0.0);
				__leadLag = 0.0;
			} else {
				if( columnChange ){
					__oldLeadLag = pNote->get_lead_lag();
				}
				
				m_fLastSetValue = val * -2.0 + 1.0;
				m_bValueHasBeenSet = true;
				pNote->set_lead_lag((val * -2.0) + 1.0);
				__leadLag = (val * -2.0) + 1.0;
				char valueChar[100];
				if (pNote->get_lead_lag() < 0.0) {
					sprintf( valueChar, "%.2f",  ( pNote->get_lead_lag() * -5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
					HydrogenApp::get_instance()->setStatusBarMessage( QString("Leading beat by: %1 ticks").arg( valueChar ), 2000 );
				} else if (pNote->get_lead_lag() > 0.0) {
					sprintf( valueChar, "%.2f",  ( pNote->get_lead_lag() * 5)); // FIXME: '5' taken from fLeadLagFactor calculation in hydrogen.cpp
					HydrogenApp::get_instance()->setStatusBarMessage( QString("Lagging beat by: %1 ticks").arg( valueChar ), 2000 );
				} else {
					HydrogenApp::get_instance()->setStatusBarMessage( QString("Note on beat"), 2000 );
				}
				
			}
		}
		
		else if ( m_Mode == NOTEKEY ){
			if ( (ev->button() == Qt::MidButton) || (ev->modifiers() == Qt::ControlModifier && ev->button() == Qt::LeftButton) ) {
				;
			} else {
				//set the note height
				//QMessageBox::information ( this, "Hydrogen", tr( "val: %1" ).arg(keyval)  );
				int k = 666;
				int o = 666;
				if(keyval >=6 && keyval<=125) {
					k = (keyval-6)/10;
				} else if(keyval>=135 && keyval<=205) {
					o = (keyval-166)/10;
					if(o==-4) o=-3; // 135
				}
				m_fLastSetValue = o * 12 + k;
				m_bValueHasBeenSet = true;
				pNote->set_key_octave((Note::Key)k,(Note::Octave)o); // won't set wrong values see Note::set_key_octave
				__octaveKeyVal = pNote->get_octave();
				__noteKeyVal = pNote->get_key();
			}
		}
		else if ( m_Mode == PROBABILITY && !pNote->get_note_off() ) {
			if( columnChange ){
				__oldProbability = pNote->get_probability();
			}
			m_fLastSetValue = val;
			m_bValueHasBeenSet = true;
			pNote->set_probability( val );
			__probability = val;
			char valueChar[100];
			sprintf( valueChar, "%#.2f",  val);
			HydrogenApp::get_instance()->setStatusBarMessage( QString("Set note probability [%1]").arg( valueChar ), 2000 );
		}

	
		if( columnChange ){
			__columnCheckOnXmouseMouve = column;
			addUndoAction();
			return;
		}
				
		__columnCheckOnXmouseMouve = column;
	
		pSong->set_is_modified( true );
		updateEditor();
		break;
	}
	m_pPatternEditorPanel->getPianoRollEditor()->updateEditor();
	m_pPatternEditorPanel->getDrumPatternEditor()->updateEditor();
}

void NotePropertiesRuler::propertyAdjustEnd(QMouseEvent *ev)
{
	addUndoAction();
	updateEditor();
}

void NotePropertiesRuler::keyPressEvent( QKeyEvent *ev )
{
	bool bIsSelectionKey = m_selection.keyPressEvent( ev );

	m_pPatternEditorPanel->setCursorHidden( false );

	if ( bIsSelectionKey ) {
		// Key was claimed by selection
	} else if ( ev->matches( QKeySequence::MoveToNextChar ) || ev->matches( QKeySequence::SelectNextChar ) ) {
		// ->
		m_pPatternEditorPanel->moveCursorRight();

	} else if ( ev->matches( QKeySequence::MoveToEndOfLine ) || ev->matches( QKeySequence::SelectEndOfLine ) ) {
		// -->|
		m_pPatternEditorPanel->setCursorPosition( m_pPattern->get_length() );

	} else if ( ev->matches( QKeySequence::MoveToPreviousChar ) || ev->matches( QKeySequence::SelectPreviousChar ) ) {
		// <-
		m_pPatternEditorPanel->moveCursorLeft();

	} else if ( ev->matches( QKeySequence::MoveToStartOfLine ) || ev->matches( QKeySequence::SelectStartOfLine ) ) {
		// |<--
		m_pPatternEditorPanel->setCursorPosition(0);

	} else {
		// Value adjustments
		double delta = 0.0;
		bool bRepeatLastValue = false;

		if ( ev->matches( QKeySequence::MoveToPreviousLine ) ) {
			// Key: Up: increase note parameter value
			delta = 0.1;

		} else if ( ev->matches( QKeySequence::MoveToNextLine ) ) {
			// Key: Down: decrease note parameter value
			delta = -0.1;

		} else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ) {
			// Key: MoveToStartOfDocument: increase parameter to maximum value
			delta = 1.0;

		} else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ) {
			// Key: MoveEndOfDocument: decrease parameter to minimum value
			delta = -1.0;

		} else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
			// Key: Enter/Return: repeat last parameter value set.
			if (m_bValueHasBeenSet) {
				bRepeatLastValue = true;
			}

		} else if ( ev->matches( QKeySequence::SelectAll ) ) {
			// Key: Ctrl + A: Select all
			selectAll();

		} else if ( ev->matches( QKeySequence::Deselect ) ) {
			// Key: Shift + Ctrl + A: clear selection
			selectNone();

		}

		if ( delta != 0.0 || bRepeatLastValue ) {
			int column = m_pPatternEditorPanel->getCursorPosition();
			int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
			Song *pSong = (Hydrogen::get_instance())->getSong();


			prepareUndoAction( m_nMargin + column * m_nGridWidth );

			const Pattern::notes_t* notes = m_pPattern->get_notes();
			FOREACH_NOTE_CST_IT_BOUND(notes,it,column) {
				Note *pNote = it->second;
				assert( pNote );
				assert( (int)pNote->get_position() == column );
				if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
					continue;
				}

				switch (m_Mode) {
				case VELOCITY:
					if ( !pNote->get_note_off() ) {
						if ( bRepeatLastValue ) {
							__velocity = m_fLastSetValue;
						} else {
							__velocity = pNote->get_velocity() + delta;
						}
						__velocity = qBound( __velocity, VELOCITY_MIN, VELOCITY_MAX );
						m_fLastSetValue = __velocity;
						m_bValueHasBeenSet = true;
					}
					break;
				case PAN:
					if ( !pNote->get_note_off() ) {
						double val;
						if ( bRepeatLastValue ) {
							val = m_fLastSetValue;
						} else {
							 val = (pNote->get_pan_r() - pNote->get_pan_l() + 0.5) + delta;
						}
						if ( val > PAN_MAX ) {
							__pan_L = 2*PAN_MAX - val;
							__pan_R = PAN_MAX;
						} else {
							__pan_L = PAN_MAX;
							__pan_R = val;
						}
						m_fLastSetValue = val;
						m_bValueHasBeenSet = true;
						break;
					}
				case LEADLAG:
					{
						if ( bRepeatLastValue ) {
							__leadLag = m_fLastSetValue;
						} else {
							__leadLag = pNote->get_lead_lag() - delta;
						}
						__leadLag = qBound( __leadLag, LEAD_LAG_MIN, LEAD_LAG_MAX );
						m_fLastSetValue = __leadLag;
						m_bValueHasBeenSet = true;
						break;
					}
				case PROBABILITY:
					if ( !pNote->get_note_off() ) {
						if ( bRepeatLastValue ) {
							__probability = m_fLastSetValue;
						} else {
							__probability = pNote->get_probability() + delta;
						}
						__probability = qBound( __probability, 0.0f, 1.0f );
						m_fLastSetValue = __probability;
						m_bValueHasBeenSet = true;
					}
					break;
				case NOTEKEY:
					if ( bRepeatLastValue ) {
						__octaveKeyVal = (int)m_fLastSetValue / 12;
						__noteKeyVal = (int)m_fLastSetValue % 12;
					} else {
						__octaveKeyVal = pNote->get_octave();
						__noteKeyVal = pNote->get_key();
						if (delta > 0) {
							if (__noteKeyVal < 11) {
								__noteKeyVal += 1;
							} else if (__octaveKeyVal < 3) {
								__octaveKeyVal += 1;
								__noteKeyVal = 0;
							}
						} else {
							if (__noteKeyVal > 0) {
								__noteKeyVal -= 1;
							} else if (__octaveKeyVal > -3) {
								__octaveKeyVal -= 1;
								__noteKeyVal = 11;
							}
						}
					}
					m_fLastSetValue = 12 * __octaveKeyVal + __noteKeyVal;
					m_bValueHasBeenSet = true;
					break;
				}
			}
			addUndoAction();
		} else {
			m_pPatternEditorPanel->setCursorHidden( true );
			ev->ignore();
			return;
		}
	}

	m_selection.updateKeyboardCursorPosition( getKeyboardCursorRect() );
	updateEditor();
	ev->accept();

}


void NotePropertiesRuler::focusInEvent( QFocusEvent * ev )
{
	if ( ev->reason() != Qt::MouseFocusReason && ev->reason() != Qt::OtherFocusReason ) {
		m_pPatternEditorPanel->setCursorHidden( false );
	}
	updateEditor();
}


void NotePropertiesRuler::focusOutEvent( QFocusEvent * ev )
{
	updateEditor();
}


void NotePropertiesRuler::addUndoAction()
{

	SE_editNotePropertiesVolumeAction *action = new SE_editNotePropertiesVolumeAction( __undoColumn,
											   __mode,
											   __nSelectedPatternNumber,
											   __nSelectedInstrument,
											   __velocity,
											   __oldVelocity,
											   __pan_L,
											   __oldPan_L,
											   __pan_R,
											   __oldPan_R,
											   __leadLag,
											   __oldLeadLag,
											   __probability,
											   __oldProbability,
											   __noteKeyVal,
											   __oldNoteKeyVal,
											   __octaveKeyVal,
											   __oldOctaveKeyVal );

	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}

void NotePropertiesRuler::paintEvent( QPaintEvent *ev)
{
	QPainter painter(this);
	if ( m_bNeedsUpdate ) {
		finishUpdateEditor();
	}
	painter.drawPixmap( ev->rect(), *m_pBackground, ev->rect() );
	m_selection.paintSelection( &painter );
}



void NotePropertiesRuler::createVelocityBackground(QPixmap *pixmap)
{
	if ( !isVisible() ) {
		return;
	}

	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(),
				  pStyle->m_patternEditor_line1Color.getGreen(),
				  pStyle->m_patternEditor_line1Color.getBlue() );

	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(),
							pStyle->m_patternEditor_backgroundColor.getGreen(),
							pStyle->m_patternEditor_backgroundColor.getBlue() );

	QColor horizLinesColor( pStyle->m_patternEditor_backgroundColor.getRed() - 20,
							pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
							pStyle->m_patternEditor_backgroundColor.getBlue() - 20 );

	unsigned nNotes = MAX_NOTES;
	if ( m_pPattern ) {
		nNotes = m_pPattern->get_length();
	}

	QPainter p( pixmap );

	p.fillRect( 0, 0, m_nMargin + nNotes * m_nGridWidth, height(), backgroundColor );

	drawGridLines( p, Qt::DotLine );

	// Horizontal lines at 10% intervals
	p.setPen( horizLinesColor );
	for (unsigned y = 0; y < m_nEditorHeight; y = y + (m_nEditorHeight / 10)) {
		p.drawLine( m_nMargin, y, 20 + nNotes * m_nGridWidth, y );
	}

	// draw velocity lines
	if (m_pPattern != nullptr) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();

		QPen selectedPen( selectedNoteColor( pStyle ) );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND(notes,coit,pos) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
					continue;
				}
				uint x_pos = m_nMargin + pos * m_nGridWidth;
				uint line_end = height();


				uint value = 0;
				if ( m_Mode == VELOCITY ) {
					value = (uint)(pNote->get_velocity() * height());
				}
				else if ( m_Mode == PROBABILITY ) {
					value = (uint)(pNote->get_probability() * height());
				}
				uint line_start = line_end - value;
				QColor centerColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );
				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 + xoffset, line_start - 2,
									   nLineWidth + 4,  line_end - line_start + 4 ,
									   4, 4 );
				}

				p.fillRect( x_pos - 1 + xoffset, line_start, nLineWidth,  line_end - line_start , centerColor );
				xoffset++;
			}
		}
	}
	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}



void NotePropertiesRuler::createPanBackground(QPixmap *pixmap)
{
	if ( !isVisible() ) {
		return;
	}

	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();

	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(),
							pStyle->m_patternEditor_backgroundColor.getGreen(),
							pStyle->m_patternEditor_backgroundColor.getBlue() );

	QColor horizLinesColor( pStyle->m_patternEditor_backgroundColor.getRed() - 20,
							pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
							pStyle->m_patternEditor_backgroundColor.getBlue() - 20 );

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(),
				  pStyle->m_patternEditor_line1Color.getGreen(),
				  pStyle->m_patternEditor_line1Color.getBlue() );

	QPainter p( pixmap );

	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->get_length();
	}
	p.fillRect( 0, 0, m_nMargin + nNotes * m_nGridWidth, height(), backgroundColor );

	// central line
	p.setPen( horizLinesColor );
	p.drawLine(0, height() / 2.0, m_nEditorWidth, height() / 2.0);

	// vertical lines
	drawGridLines( p, Qt::DotLine );

	if ( m_pPattern ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor( pStyle ) );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND(notes,coit,pos) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_note_off() || (pNote->get_instrument()
											   != pSong->get_instrument_list()->get( nSelectedInstrument ) ) ) {
					continue;
				}
				uint x_pos = m_nMargin + pNote->get_position() * m_nGridWidth;
				QColor centerColor = DrumPatternEditor::computeNoteColor( pNote->get_velocity() );
				if (pNote->get_pan_r() == pNote->get_pan_l()) {
					// pan value is centered - draw circle
					int y_pos = (int)( height() * 0.5 );
					p.setBrush(QColor( centerColor ));
					p.drawEllipse( x_pos-4 + xoffset, y_pos-4, 8, 8);
				} else {
					int y_start = (int)( pNote->get_pan_l() * height() );
					int y_end = (int)( height() - pNote->get_pan_r() * height() );
					int nLineWidth = 3;
					p.fillRect( x_pos - 1 + xoffset, y_start, nLineWidth, y_end - y_start, QColor(  centerColor) );
					p.fillRect( x_pos - 1 + xoffset, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor(  centerColor ) );
				}

				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setBrush( Qt::NoBrush );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 + xoffset, 0,
									   nLineWidth + 4,  height() ,
									   4, 4 );
				}
				xoffset++;
			}
		}
	}

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}

void NotePropertiesRuler::createLeadLagBackground(QPixmap *pixmap)
{
	if ( !isVisible() ) {
		return;
	}

	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();
	
	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(),
							pStyle->m_patternEditor_backgroundColor.getGreen(),
							pStyle->m_patternEditor_backgroundColor.getBlue() );

	QColor horizLinesColor( pStyle->m_patternEditor_backgroundColor.getRed() - 20,
							pStyle->m_patternEditor_backgroundColor.getGreen() - 20,
							pStyle->m_patternEditor_backgroundColor.getBlue() - 20 );

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(),
				  pStyle->m_patternEditor_line1Color.getGreen(),
				  pStyle->m_patternEditor_line1Color.getBlue() );

	QPainter p( pixmap );

	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->get_length();
	}
	p.fillRect( 0, 0, m_nMargin + nNotes * m_nGridWidth, height(), backgroundColor );

	// central line
	p.setPen( horizLinesColor );
	p.drawLine(0, height() / 2.0, m_nEditorWidth, height() / 2.0);

	// vertical lines
	drawGridLines( p, Qt::DotLine );

	if ( m_pPattern ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor( pStyle ) );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pposNote = it->second;
			assert( pposNote );
			uint pos = pposNote->get_position();
			int xoffset = 0;
			FOREACH_NOTE_CST_IT_BOUND(notes,coit,pos) {
				Note *pNote = coit->second;
				assert( pNote );
				if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
					continue;
				}

				uint x_pos = m_nMargin + pNote->get_position() * m_nGridWidth;

				int red1 = (int) (pNote->get_velocity() * 255);
				int green1;
				int blue1;
				blue1 = ( 255 - (int) red1 )* .33;
				green1 =  ( 255 - (int) red1 );
	
				if (pNote->get_lead_lag() == 0) {
				
					// leadlag value is centered - draw circle
					int y_pos = (int)( height() * 0.5 );
					p.setBrush(QColor( 0 , 0 , 0 ));
					p.drawEllipse( x_pos-4 + xoffset, y_pos-4, 8, 8);
				} else {
					int y_start = (int)( height() * 0.5 );
					int y_end = y_start + ((pNote->get_lead_lag()/2) * height());
		
					int nLineWidth = 3;
					int red;
					int green;
					int blue = (int) (pNote->get_lead_lag() * 255);
					if (blue < 0)  {
						red = blue *-1;
						blue = (int) red * .33;
						green = (int) red * .33;
					} else {
						red = (int) blue * .33;
						green = (int) blue * .33;
					}
					p.fillRect( x_pos - 1 + xoffset, y_start, nLineWidth, y_end - y_start, QColor( red, green ,blue ) );
		
					p.fillRect( x_pos - 1 + xoffset, ( height() / 2.0 ) - 2 , nLineWidth, 5, QColor( red1, green1 ,blue1 ) );
				}

				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setBrush( Qt::NoBrush );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 + xoffset, 0,
									   nLineWidth + 4,  height() ,
									   4, 4 );
				}

				xoffset++;
 			}
		}
	}

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);
}



void NotePropertiesRuler::createNoteKeyBackground(QPixmap *pixmap)
{
	if ( !isVisible() ) {
		return;
	}

	UIStyle *pStyle = Preferences::get_instance()->getDefaultUIStyle();

	QColor res_1( pStyle->m_patternEditor_line1Color.getRed(),
				  pStyle->m_patternEditor_line1Color.getGreen(),
				  pStyle->m_patternEditor_line1Color.getBlue() );

	QColor backgroundColor( pStyle->m_patternEditor_backgroundColor.getRed(),
							pStyle->m_patternEditor_backgroundColor.getGreen(),
							pStyle->m_patternEditor_backgroundColor.getBlue() );

	QColor horizLinesColor( pStyle->m_patternEditor_backgroundColor.getRed() - 100,
							pStyle->m_patternEditor_backgroundColor.getGreen() - 100,
							pStyle->m_patternEditor_backgroundColor.getBlue() - 100 );

	unsigned nNotes = MAX_NOTES;
	if (m_pPattern) {
		nNotes = m_pPattern->get_length();
	}
	QPainter p( pixmap );

	p.fillRect( 0, 0, m_nMargin + nNotes * m_nGridWidth, height(), backgroundColor );

	p.setPen( horizLinesColor );
	for (unsigned y = 10; y < 80; y = y + 10 ) {
		p.setPen( QPen( res_1, 1, Qt::DashLine ) );
		if (y == 40) p.setPen( QPen( QColor(0,0,0), 1, Qt::SolidLine ) );
		p.drawLine( m_nMargin, y, m_nMargin + nNotes * m_nGridWidth, y );
	}

	for (unsigned y = 90; y < 210; y = y + 10 ) {
		p.setPen( QPen( QColor( 255, 255, 255 ), 9, Qt::SolidLine, Qt::FlatCap) );
		if ( y == 100 ||y == 120 ||y == 140 ||y == 170 ||y == 190) {
			p.setPen( QPen( QColor( 128, 128, 128 ), 9, Qt::SolidLine, Qt::FlatCap ) );
		}
		p.drawLine( m_nMargin, y, m_nMargin + nNotes * m_nGridWidth, y );
	}

	// Annotate with note class names
	static QString noteNames[] = { tr( "B" ), tr( "A#" ), tr( "A" ), tr( "G#" ), tr( "G" ), tr( "F#" ),
								   tr( "F" ), tr( "E" ), tr( "D#" ), tr( "D" ), tr( "C#" ), tr( "C" ) };
	QFont font;
	font.setPointSize( 9 );
	p.setFont( font );
	p.setPen( QColor( 0, 0, 0 ) );
	for ( int n = 0; n < 12; n++ ) {
		p.drawText( 5, 90 + 10 * n +3, noteNames[n] );
	}

	// vertical lines
	drawGridLines( p, Qt::DotLine );

	p.setPen(res_1);
	p.drawLine(0, 0, m_nEditorWidth, 0);
	p.drawLine(0, m_nEditorHeight - 1, m_nEditorWidth, m_nEditorHeight - 1);


	// Black outline each key
	for (unsigned y = 90; y <= 210; y = y + 10 ) {
		p.setPen( QPen( QColor( 0, 0, 0 ), 1, Qt::SolidLine));
		p.drawLine( m_nMargin, y-5, m_nMargin + nNotes * m_nGridWidth, y-5);
	}

	//paint the octave
	if ( m_pPattern ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor( pStyle ) );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
				continue;
			}
			if ( !pNote->get_note_off() ) {
				uint x_pos = 17 + pNote->get_position() * m_nGridWidth;
				uint y_pos = (4-pNote->get_octave())*10-3;
				p.setBrush(QColor( 99, 160, 233 ));
				p.drawEllipse( x_pos, y_pos, 6, 6);
			}
		}
	}

	//paint the note
	if ( m_pPattern ) {
		int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
		Song *pSong = Hydrogen::get_instance()->getSong();
		QPen selectedPen( selectedNoteColor( pStyle ) );
		selectedPen.setWidth( 2 );

		const Pattern::notes_t* notes = m_pPattern->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );
			if ( pNote->get_instrument() != pSong->get_instrument_list()->get( nSelectedInstrument ) ) {
				continue;
			}

			if ( !pNote->get_note_off() ) {
				int d = 8;
				int k = pNote->get_key();
				uint x_pos = 16 + pNote->get_position() * m_nGridWidth;
				uint y_pos = 200-(k*10)-4;

				x_pos -= 1;
				y_pos -= 1;
				d += 2;
				p.setPen( Qt::NoPen );
				p.setBrush(QColor( 0, 0, 0));
				p.drawEllipse( x_pos, y_pos, d, d);

				// Paint selection outlines
				int nLineWidth = 3;
				if ( m_selection.isSelected( pNote ) ) {
					p.setPen( selectedPen );
					p.setBrush( Qt::NoBrush );
					p.setRenderHint( QPainter::Antialiasing );
					p.drawRoundedRect( x_pos - 1 -2 +3, 0,
									   nLineWidth + 4 + 4,  height() ,
									   4, 4 );
				}
			}
		}
	}
}


void NotePropertiesRuler::updateEditor( bool bPatternOnly )
{
	Hydrogen *pEngine = Hydrogen::get_instance();
	PatternList *pPatternList = pEngine->getSong()->get_pattern_list();
	int nSelectedPatternNumber = pEngine->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() ) ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = nullptr;
	}
	__nSelectedPatternNumber = nSelectedPatternNumber;

	// update editor width
	if ( m_pPattern ) {
		m_nEditorWidth = m_nMargin + m_pPattern->get_length() * m_nGridWidth;
	}
	else {
		m_nEditorWidth =  m_nMargin + MAX_NOTES * m_nGridWidth;
	}

	if ( !m_bNeedsUpdate ) {
		m_bNeedsUpdate = true;
		update();
	}
}

void NotePropertiesRuler::finishUpdateEditor()
{
	assert( m_bNeedsUpdate );
	resize( m_nEditorWidth, height() );
		
	delete m_pBackground;
	m_pBackground = new QPixmap( m_nEditorWidth, m_nEditorHeight );

	if ( m_Mode == VELOCITY || m_Mode == PROBABILITY ) {
		createVelocityBackground( m_pBackground );
	}
	else if ( m_Mode == PAN ) {
		createPanBackground( m_pBackground );
	}
	else if ( m_Mode == LEADLAG ) {
		createLeadLagBackground( m_pBackground );
	}
	else if ( m_Mode == NOTEKEY ) {
		createNoteKeyBackground( m_pBackground );
	}

	if ( hasFocus() && !m_pPatternEditorPanel->cursorHidden() ) {
		QPainter p( m_pBackground );

		uint x = m_nMargin + m_pPatternEditorPanel->getCursorPosition() * m_nGridWidth;

		p.setPen( QColor( 0,0,0 ) );
		p.setRenderHint( QPainter::Antialiasing );
		p.drawRoundedRect( QRect( x-m_nGridWidth*3, 0+1, m_nGridWidth*6, height()-2 ), 4, 4 );
	}

	// redraw all
	m_bNeedsUpdate = false;
	update();
}


void NotePropertiesRuler::selectedPatternChangedEvent()
{
	updateEditor();
}



void NotePropertiesRuler::selectedInstrumentChangedEvent()
{
	updateEditor();
}


std::vector<NotePropertiesRuler::SelectionIndex> NotePropertiesRuler::elementsIntersecting( QRect r ) {
	std::vector<SelectionIndex> result;
	const Pattern::notes_t* notes = m_pPattern->get_notes();
	Song *pSong = Hydrogen::get_instance()->getSong();
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	Instrument *pInstrument = pSong->get_instrument_list()->get( nSelectedInstrument );

	// Account for the notional active area of the slider. We allow a
	// width of 8 as this is the size of the circle used for the zero
	// position on the lead/lag editor.
	r = r.normalized();
	if ( r.top() == r.bottom() && r.left() == r.right() ) {
		r += QMargins( 2, 2, 2, 2 );
	}
	r += QMargins( 4, 4, 4, 4 );

	FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
		if ( it->second->get_instrument() !=  pInstrument ) {
			continue;
		}

		int pos = it->first;
		uint x_pos = m_nMargin + pos * m_nGridWidth;
		if ( r.intersects( QRect( x_pos, 0, 1, height() ) ) ) {
			result.push_back( it->second );
		}
	}

	// Updating selection, we may need to repaint the whole widget. 
	updateEditor();
	return std::move(result);
}

///
/// The screen area occupied by the keyboard cursor
///
QRect NotePropertiesRuler::getKeyboardCursorRect()
{
	uint x = m_nMargin + m_pPatternEditorPanel->getCursorPosition() * m_nGridWidth;
	int nSelectedInstrument = Hydrogen::get_instance()->getSelectedInstrumentNumber();
	uint y = nSelectedInstrument * m_nGridHeight;
	return QRect( x-m_nGridWidth*3, 0+1, m_nGridWidth*6, height()-2 );
}

void NotePropertiesRuler::selectAll() {
	m_selection.clearSelection();
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	Song *pSong = pHydrogen->getSong();
	Pattern *pPattern = pSong->get_pattern_list()->get( pHydrogen->getSelectedPatternNumber() );
	Instrument *pInstrument =  pSong->get_instrument_list()->get( pHydrogen->getSelectedInstrumentNumber() );
	FOREACH_NOTE_CST_IT_BEGIN_END( pPattern->get_notes(), it )
		{
			if ( it->second->get_instrument() == pInstrument ) {
				m_selection.addToSelection( it->second );
			}
		}
	m_selection.updateWidgetGroup();
}
