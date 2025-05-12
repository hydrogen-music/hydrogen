/*
 * Hydrogen
 * Copyright(c) 2002-2008 by the Hydrogen Team
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

#include "PatternEditor.h"
#include "PatternEditorRuler.h"
#include "PatternEditorInstrumentList.h"
#include "PatternEditorPanel.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../EventListener.h"
#include "../UndoActions.h"
#include "../Skin.h"

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


using namespace std;
using namespace H2Core;


PatternEditor::PatternEditor( QWidget *pParent,
							  PatternEditorPanel *panel )
	: Object()
	, QWidget( pParent )
	, m_selection( this )
	, m_bEntered( false )
	, m_pDraggedNote( nullptr )
	, m_pPatternEditorPanel( panel )
	, m_pPattern( nullptr )
	, m_bSelectNewNotes( false )
	, m_bFineGrained( false )
	, m_bCopyNotMove( false )
	, m_nTick( -1 )
	, m_editor( Editor::None )
	, m_mode( Mode::None )
{

	const auto pPref = H2Core::Preferences::get_instance();

	m_nResolution = pPref->getPatternEditorGridResolution();
	m_bUseTriplets = pPref->isPatternEditorUsingTriplets();
	m_fGridWidth = pPref->getPatternEditorGridWidth();
	m_nEditorWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );
	m_nActiveWidth = m_nEditorWidth;

	setFocusPolicy(Qt::StrongFocus);

	HydrogenApp::get_instance()->addEventListener( this );
	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &PatternEditor::onPreferencesChanged );
	
	m_pAudioEngine = Hydrogen::get_instance()->getAudioEngine();

	// Popup context menu
	m_pPopupMenu = new QMenu( this );
	m_pPopupMenu->addAction( tr( "&Cut" ), this, SLOT( cut() ) );
	m_pPopupMenu->addAction( tr( "&Copy" ), this, SLOT( copy() ) );
	m_pPopupMenu->addAction( tr( "&Paste" ), this, SLOT( paste() ) );
	m_pPopupMenu->addAction( tr( "&Delete" ), this, SLOT( deleteSelection() ) );
	m_pPopupMenu->addAction( tr( "Select &all" ), this, SLOT( selectAll() ) );
	m_pPopupMenu->addAction( tr( "Clear selection" ), this, SLOT( selectNone() ) );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
									   height() * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	m_bBackgroundInvalid = true;
}

PatternEditor::~PatternEditor()
{
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
}

void PatternEditor::onPreferencesChanged( H2Core::Preferences::Changes changes )
{
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

QColor PatternEditor::computeNoteColor( float fVelocity ) {
	float fRed, fGreen, fBlue;

	auto pPref = H2Core::Preferences::get_instance();

	QColor fullColor = pPref->getColorTheme()->m_patternEditor_noteVelocityFullColor;
	QColor defaultColor = pPref->getColorTheme()->m_patternEditor_noteVelocityDefaultColor;
	QColor halfColor = pPref->getColorTheme()->m_patternEditor_noteVelocityHalfColor;
	QColor zeroColor = pPref->getColorTheme()->m_patternEditor_noteVelocityZeroColor;

	// The colors defined in the Preferences correspond to fixed
	// velocity values. In case the velocity lies between two of those
	// the corresponding colors will be interpolated.
	float fWeightFull = 0;
	float fWeightDefault = 0;
	float fWeightHalf = 0;
	float fWeightZero = 0;

	if ( fVelocity >= 1.0 ) {
		fWeightFull = 1.0;
	} else if ( fVelocity >= 0.8 ) {
		fWeightDefault = ( 1.0 - fVelocity )/ ( 1.0 - 0.8 );
		fWeightFull = 1.0 - fWeightDefault;
	} else if ( fVelocity >= 0.5 ) {
		fWeightHalf = ( 0.8 - fVelocity )/ ( 0.8 - 0.5 );
		fWeightDefault = 1.0 - fWeightHalf;
	} else {
		fWeightZero = ( 0.5 - fVelocity )/ ( 0.5 );
		fWeightHalf = 1.0 - fWeightZero;
	}

	fRed = fWeightFull * fullColor.redF() +
		fWeightDefault * defaultColor.redF() +
		fWeightHalf * halfColor.redF() + fWeightZero * zeroColor.redF();
	fGreen = fWeightFull * fullColor.greenF() +
		fWeightDefault * defaultColor.greenF() +
		fWeightHalf * halfColor.greenF() + fWeightZero * zeroColor.greenF();
	fBlue = fWeightFull * fullColor.blueF() +
		fWeightDefault * defaultColor.blueF() +
		fWeightHalf * halfColor.blueF() + fWeightZero * zeroColor.blueF();

	QColor color;
	color.setRedF( fRed );
	color.setGreenF( fGreen );
	color.setBlueF( fBlue );
	
	return color;
}


void PatternEditor::drawNoteSymbol( QPainter &p, QPoint pos, H2Core::Note *pNote, bool bIsForeground ) const
{
	if ( m_pPattern == nullptr ) {
		return;
	}

	auto pPref = H2Core::Preferences::get_instance();
	
	const QColor noteColor( pPref->getColorTheme()->m_patternEditor_noteVelocityDefaultColor );
	const QColor noteInactiveColor( pPref->getColorTheme()->m_windowTextColor.darker( 150 ) );
	const QColor noteoffColor( pPref->getColorTheme()->m_patternEditor_noteOffColor );
	const QColor noteoffInactiveColor( pPref->getColorTheme()->m_windowTextColor );

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

			if ( x_pos >= m_nActiveWidth ) {
				noteBrush.setColor( noteInactiveColor );
				notePen.setColor( noteInactiveColor );
			}
			
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

			// if there is a stop-note to the right of this note, only draw-
			// its length till there.
			int nLength = pNote->get_length();
			auto notes = m_pPattern->get_notes();
			for ( const auto& [ _, ppNote ] : *m_pPattern->get_notes() ) {
				if ( ppNote != nullptr &&
					 // noteOff note
					 ppNote->get_note_off() &&
					 // located in the same row
					 ppNote->get_instrument() == pNote->get_instrument() &&
					 // left of the NoteOff
					 pNote->get_position() < ppNote->get_position() &&
					 // custom length reaches beyond NoteOff
					 pNote->get_position() + pNote->get_length() >
					 ppNote->get_position() ) {
					// In case there are multiple stop-notes present, take the
					// shortest distance.
					nLength = std::min( ppNote->get_position() - pNote->get_position(), nLength );
				}
			}

			width = m_fGridWidth * nLength / fStep;
			width = width - 1;	// lascio un piccolo spazio tra una nota ed un altra

			if ( bSelected ) {
				p.drawRoundedRect( x_pos-2, y_pos, width+4, 3+4, 4, 4 );
			}
			p.setPen( notePen );
			p.setBrush( noteBrush );

			// Since the note body is transparent for an inactive note, we try
			// to start the tail at its boundary. For regular notes we do not
			// care about an overlap, as it ensures that there are no white
			// artifacts between tail and note body regardless of the scale
			// factor.
			int nRectOnsetX = x_pos;
			int nRectWidth = width;
			if ( ! bIsForeground ) {
				nRectOnsetX = nRectOnsetX + w/2;
				nRectWidth = nRectWidth - w/2;
			}

			p.drawRect( nRectOnsetX, y_pos +2, nRectWidth, 3 );
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

		QBrush noteOffBrush( noteoffColor );
		if ( !bIsForeground ) {
			noteOffBrush.setStyle( Qt::Dense4Pattern );

			if ( x_pos >= m_nActiveWidth ) {
				noteOffBrush.setColor( noteoffInactiveColor );
			}
		}

		if ( bSelected ) {
			p.drawEllipse( x_pos -4 -2, y_pos-2, w+4, h+4 );
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
	int nColumn = ( x - PatternEditor::nMargin + (nWidth / 2) ) / nWidth;
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
	auto pInstrumentList = pHydrogen->getSong()->getInstrumentList();
	XMLDoc doc;
	XMLNode selection = doc.set_root( "noteSelection" );
	XMLNode noteList = selection.createNode( "noteList");
	XMLNode positionNode = selection.createNode( "sourcePosition" );
	bool bWroteNote = false;
	// "Top left" of selection, in the three dimensional time*instrument*pitch space.
	int nMinColumn, nMinRow, nMaxPitch;

	for ( Note *pNote : m_selection ) {
		const int nPitch = pNote->get_notekey_pitch();
		const int nColumn = pNote->get_position();
		const int nRow = pInstrumentList->index( pNote->get_instrument() );
		if ( nRow == -1 ) {
			// In versions prior to v2.0 all notes not belonging to any
			// instrument will just be ignored.
			continue;
		}
		if ( bWroteNote ) {
			nMinColumn = std::min( nColumn, nMinColumn );
			nMinRow = std::min( nRow, nMinRow );
			nMaxPitch = std::max( nPitch, nMaxPitch );
		} else {
			nMinColumn = nColumn;
			nMinRow = nRow;
			nMaxPitch = nPitch;
			bWroteNote = true;
		}
		XMLNode note_node = noteList.createNode( "note" );
		pNote->save_to( &note_node );
	}

	if ( bWroteNote ) {
		positionNode.write_int( "minColumn", nMinColumn );
		positionNode.write_int( "minRow", nMinRow );
		positionNode.write_int( "maxPitch", nMaxPitch );
	} else {
		positionNode.write_int( "minColumn",
								m_pPatternEditorPanel->getCursorPosition() );
		positionNode.write_int( "minRow",
								pHydrogen->getSelectedInstrumentNumber() );
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
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	auto pInstrumentList = Hydrogen::get_instance()->getSong()->getInstrumentList();
	auto pInstrument = pInstrumentList->get( nInstrument );

	m_selection.clearSelection();
	FOREACH_NOTE_CST_IT_BEGIN_LENGTH(m_pPattern->get_notes(), it, m_pPattern) {
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
	if ( m_pPattern == nullptr ) {
		return false;
	}

	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();
	
	//	Hydrogen *pH = Hydrogen::get_instance();
	std::set< Note *> duplicates;
	for ( Note *pNote : elements ) {
		if ( duplicates.find( pNote ) != duplicates.end() ) {
			// Already marked pNote as a duplicate of some other pNote. Skip it.
			continue;
		}
		FOREACH_NOTE_CST_IT_BOUND_END( m_pPattern->get_notes(), it, pNote->get_position() ) {
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
			messageBox.setCheckBox( new QCheckBox( pCommonStrings->getMutableDialog() ) );
			messageBox.checkBox()->setChecked( false );
			bOk = messageBox.exec() == QMessageBox::Ok;
			if ( messageBox.checkBox()->isChecked() ) {
				pPreferences->setShowNoteOverwriteWarning( false );
			}
		}

		if ( bOk ) {
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
	if ( m_pPattern == nullptr ) {
		return;
	}
	
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
				delete pNote;
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
	if ( m_pPattern == nullptr ) {
		return;
	}
	
	// Restore previously-overwritten notes, and select notes that were selected before.
	m_selection.clearSelection( /* bCheck=*/false );
	m_pAudioEngine->lock( RIGHT_HERE );
	for ( auto pNote : overwritten ) {
		Note *pNewNote = new Note( pNote );
		m_pPattern->insert_note( pNewNote );
	}
	// Select the previously-selected notes
	for ( auto pNote : selected ) {
		FOREACH_NOTE_CST_IT_BOUND_END( m_pPattern->get_notes(), it, pNote->get_position() ) {
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

	if ( pSong != nullptr ) {
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
	const std::vector<QColor> colorsActive = {
		QColor( pPref->getColorTheme()->m_patternEditor_line1Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line2Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line3Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line4Color ),
		QColor( pPref->getColorTheme()->m_patternEditor_line5Color ),
	};
	const std::vector<QColor> colorsInactive = {
		QColor( pPref->getColorTheme()->m_windowTextColor.darker( 170 ) ),
		QColor( pPref->getColorTheme()->m_windowTextColor.darker( 190 ) ),
		QColor( pPref->getColorTheme()->m_windowTextColor.darker( 210 ) ),
		QColor( pPref->getColorTheme()->m_windowTextColor.darker( 230 ) ),
		QColor( pPref->getColorTheme()->m_windowTextColor.darker( 250 ) ),
	};

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

		// First, quarter note markers. All the quarter note markers must be
		// drawn. These will be drawn on all resolutions.
		const int nRes = 4;
		float fStep = MAX_NOTES / nRes * m_fGridWidth;
		float x = PatternEditor::nMargin;
		p.setPen( QPen( colorsActive[ 0 ], 1, style ) );
		while ( x < m_nActiveWidth ) {
			p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			x += fStep;
		}
			
		p.setPen( QPen( colorsInactive[ 0 ], 1, style ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine( x, 1, x, m_nEditorHeight - 1 );
			x += fStep;
		}

		// Resolution 4 was already taken into account above;
		std::vector<int> availableResolutions = { 8, 16, 32, 64, MAX_NOTES };

		// For each successive set of finer-spaced lines, the even
		// lines will have already been drawn at the previous coarser
		// pitch, so only the odd numbered lines need to be drawn.
		int nColour = 1;
		for ( int nnRes : availableResolutions ) {
			if ( nnRes > m_nResolution ) {
				break;
			}

			fStep = MAX_NOTES / nnRes * m_fGridWidth;
			float x = PatternEditor::nMargin + fStep;
			p.setPen( QPen( colorsActive[ std::min( nColour, static_cast<int>(colorsActive.size()) - 1 ) ],
							1, style ) );

			if ( nnRes != MAX_NOTES ) {
				// With each increase of resolution 1/4 -> 1/8 -> 1/16 -> 1/32
				// -> 1/64 the number of available notes doubles and all we need
				// to do is to draw another grid line right between two existing
				// ones.
				while ( x < m_nActiveWidth + fStep ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}
			else {
				// When turning resolution off, things get a bit more tricky.
				// Between 1/64 -> 1/192 (1/MAX_NOTES) the space between
				// existing grid line will be filled by two instead of one new
				// line.
				while ( x < m_nActiveWidth + fStep ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep;
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}

			p.setPen( QPen( colorsInactive[ std::min( nColour, static_cast<int>(colorsInactive.size()) - 1 ) ],
							1, style ) );
			if ( nnRes != MAX_NOTES ) {
				while ( x < m_nEditorWidth ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}
			else {
				while ( x < m_nEditorWidth ) {
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep;
					p.drawLine( x, 1, x, m_nEditorHeight - 1 );
					x += fStep * 2;
				}
			}

			nColour++;
		}

	} else {

		// Triplet style markers, we only differentiate colours on the
		// first of every triplet.
		float fStep = granularity() * m_fGridWidth;
		float x = PatternEditor::nMargin;
		p.setPen(  QPen( colorsActive[ 0 ], 1, style ) );
		while ( x < m_nActiveWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			x += fStep * 3;
		}
		
		p.setPen(  QPen( colorsInactive[ 0 ], 1, style ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			x += fStep * 3;
		}
		
		// Second and third marks
		x = PatternEditor::nMargin + fStep;
		p.setPen(  QPen( colorsActive[ 2 ], 1, style ) );
		while ( x < m_nActiveWidth + fStep ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + fStep, 1, x + fStep, m_nEditorHeight - 1);
			x += fStep * 3;
		}
		
		p.setPen( QPen( colorsInactive[ 2 ], 1, style ) );
		while ( x < m_nEditorWidth ) {
			p.drawLine(x, 1, x, m_nEditorHeight - 1);
			p.drawLine(x + fStep, 1, x + fStep, m_nEditorHeight - 1);
			x += fStep * 3;
		}
	}

}


QColor PatternEditor::selectedNoteColor() const {
	
	auto pPref = H2Core::Preferences::get_instance();
	
	if ( hasFocus() ) {
		const QColor selectHighlightColor( pPref->getColorTheme()->m_selectionHighlightColor );
		return selectHighlightColor;
	} else {
		const QColor selectInactiveColor( pPref->getColorTheme()->m_selectionInactiveColor );
		return selectInactiveColor;
	}
}


///
/// Ensure selection only refers to valid notes, and does not contain any stale references to deleted notes.
///
void PatternEditor::validateSelection()
{
	if ( m_pPattern == nullptr ) {
		return;
	}
	
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

#ifdef H2CORE_HAVE_QT6
void PatternEditor::enterEvent( QEnterEvent *ev ) {
#else
void PatternEditor::enterEvent( QEvent *ev ) {
#endif
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void PatternEditor::leaveEvent( QEvent *ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void PatternEditor::focusInEvent( QFocusEvent *ev ) {
	UNUSED( ev );
	if ( ev->reason() == Qt::TabFocusReason || ev->reason() == Qt::BacktabFocusReason ) {
		HydrogenApp::get_instance()->setHideKeyboardCursor( false );
		m_pPatternEditorPanel->ensureCursorVisible();
	}
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getInstrumentList()->update();
	}

	// Update to show the focus border highlight
	update();
}

void PatternEditor::focusOutEvent( QFocusEvent *ev ) {
	UNUSED( ev );
	if ( ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
		m_pPatternEditorPanel->getInstrumentList()->update();
	}
	
	// Update to remove the focus border highlight
	update();
}

void PatternEditor::invalidateBackground() {
	m_bBackgroundInvalid = true;
}

void PatternEditor::createBackground() {
}

//! Get notes to show in pattern editor.
//! This may include "background" notes that are in currently-playing patterns
//! rather than the current pattern.
std::vector< Pattern *> PatternEditor::getPatternsToShow( void )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	std::vector<Pattern *> patterns;

	// When using song mode without the pattern editor being locked
	// only the current pattern will be shown. In every other base
	// remaining playing patterns not selected by the user are added
	// as well.
	if ( ! ( pHydrogen->getMode() == Song::Mode::Song &&
			 ! pHydrogen->isPatternEditorLocked() ) ) {
		m_pAudioEngine->lock( RIGHT_HERE );
		if ( m_pAudioEngine->getPlayingPatterns()->size() > 0 ) {
			std::set< Pattern *> patternSet;

			std::vector<const PatternList*> patternLists;
			patternLists.push_back( m_pAudioEngine->getPlayingPatterns() );
			if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ) {
				patternLists.push_back( m_pAudioEngine->getNextPatterns() );
			}
		
			for ( const PatternList *pPatternList : patternLists ) {
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
		}
		m_pAudioEngine->unlock();
	}
	else if ( m_pPattern != nullptr &&
			  pHydrogen->getMode() == Song::Mode::Song &&
			  m_pPattern->get_virtual_patterns()->size() > 0 ) {
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

bool PatternEditor::isUsingAdditionalPatterns( const H2Core::Pattern* pPattern ) {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	if ( pHydrogen->getPatternMode() == Song::PatternMode::Stacked ||
		 ( pPattern != nullptr && pPattern->isVirtual() ) ||
		 ( pHydrogen->getMode() == Song::Mode::Song &&
		   pHydrogen->isPatternEditorLocked() ) ) {
		return true;
	}
	
	return false;
}

void PatternEditor::updateWidth() {
	auto pHydrogen = H2Core::Hydrogen::get_instance();
	
	if ( m_pPattern != nullptr ) {
		m_nActiveWidth = PatternEditor::nMargin + m_fGridWidth *
			m_pPattern->get_length();
		
		// In case there are other patterns playing which are longer
		// than the selected one, their notes will be placed using a
		// different color set between m_nActiveWidth and
		// m_nEditorWidth.
		if ( pHydrogen->getMode() == Song::Mode::Song &&
			 m_pPattern != nullptr && m_pPattern->isVirtual() &&
			 ! pHydrogen->isPatternEditorLocked() ) {
			m_nEditorWidth = 
				std::max( PatternEditor::nMargin + m_fGridWidth *
						  m_pPattern->longestVirtualPatternLength() + 1,
						  static_cast<float>(m_nActiveWidth) );
		}
		else if ( isUsingAdditionalPatterns( m_pPattern ) ) {
			m_nEditorWidth =
				std::max( PatternEditor::nMargin + m_fGridWidth *
						  pHydrogen->getAudioEngine()->getPlayingPatterns()->longest_pattern_length( false ) + 1,
						  static_cast<float>(m_nActiveWidth) );
		}
		else {
			m_nEditorWidth = m_nActiveWidth;
		}
	}
	else {
		m_nEditorWidth = PatternEditor::nMargin + MAX_NOTES * m_fGridWidth;
		m_nActiveWidth = m_nEditorWidth;
	}
}

void PatternEditor::songModeActivationEvent()
{
	// May need to draw (or hide) other background patterns
	update();
}

void PatternEditor::stackedModeActivationEvent( int nValue )
{
	UNUSED( nValue );
	// May need to draw (or hide) other background patterns
	update();
}

void PatternEditor::updatePosition( float fTick ) {
	if ( m_nTick == (int)fTick ) {
		return;
	}

	float fDiff = m_fGridWidth * (fTick - m_nTick);

	m_nTick = fTick;

	int nOffset = Skin::getPlayheadShaftOffset();
	int nX = static_cast<int>(static_cast<float>(PatternEditor::nMargin) +
							  static_cast<float>(m_nTick) *
							  m_fGridWidth );

	QRect updateRect( nX -2, 0, 4 + Skin::nPlayheadWidth, height() );
	update( updateRect );
	if ( fDiff > 1.0 || fDiff < -1.0 ) {
		// New cursor is far enough away from the old one that the single update rect won't cover both. So
		// update at the old location as well.
		updateRect.translate( -fDiff, 0 );
		update( updateRect );
	}
}

void PatternEditor::storeNoteProperties( const Note* pNote ) {
	if( pNote != nullptr ){
		m_nOldLength = pNote->get_length();
		//needed to undo note properties
		m_fOldVelocity = pNote->get_velocity();
		m_fOldPan = pNote->getPan();

		m_fOldLeadLag = pNote->get_lead_lag();

		m_fVelocity = m_fOldVelocity;
		m_fPan = m_fOldPan;
		m_fLeadLag = m_fOldLeadLag;
	}
	else {
		m_nOldLength = -1;
	}
}

void PatternEditor::mouseDragStartEvent( QMouseEvent *ev ) {

	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pHydrogen = Hydrogen::get_instance();

	// Move cursor.
	int nColumn = getColumn( ev->x() );
	m_pPatternEditorPanel->setCursorPosition( nColumn );

	// Hide cursor.
	bool bOldCursorHidden = pHydrogenApp->hideKeyboardCursor();
	
	pHydrogenApp->setHideKeyboardCursor( true );
	
	// Cursor either just got hidden or was moved.
	if ( bOldCursorHidden != pHydrogenApp->hideKeyboardCursor() ) {
		// Immediate update to prevent visual delay.
		m_pPatternEditorPanel->getInstrumentList()->repaintInstrumentLines();
		m_pPatternEditorPanel->getPatternEditorRuler()->update();
	}

	int nRealColumn = 0;

	if ( ev->button() == Qt::RightButton ) {

		int nPressedLine =
			std::floor(static_cast<float>(ev->y()) / static_cast<float>(m_nGridHeight));
		int nSelectedInstrumentNumber = pHydrogen->getSelectedInstrumentNumber();
		
		if( ev->x() > PatternEditor::nMargin ) {
			nRealColumn =
				static_cast<int>(std::floor(
					static_cast<float>((ev->x() - PatternEditor::nMargin)) /
					m_fGridWidth));
		}

		// Needed for undo changes in the note length
		m_nOldPoint = ev->y();
		m_nRealColumn = nRealColumn;
		m_nColumn = nColumn;
		m_nPressedLine = nPressedLine;
		m_nSelectedInstrumentNumber = pHydrogen->getSelectedInstrumentNumber();
	}

}

void PatternEditor::mouseDragUpdateEvent( QMouseEvent *ev) {

	if ( m_pPattern == nullptr || m_pDraggedNote == nullptr ) {
		return;
	}

	if ( m_pDraggedNote->get_note_off() ) {
		return;
	}

	int nTickColumn = getColumn( ev->x() );

	m_pAudioEngine->lock( RIGHT_HERE );
	int nLen = nTickColumn - m_pDraggedNote->get_position();

	if ( nLen <= 0 ) {
		nLen = -1;
	}

	float fNotePitch = m_pDraggedNote->get_notekey_pitch();
	float fStep = 0;
	if ( nLen > -1 ){
		fStep = Note::pitchToFrequency( ( double )fNotePitch );
	} else {
		fStep = 1.0;
	}
	m_pDraggedNote->set_length( nLen * fStep);

	m_mode = m_pPatternEditorPanel->getNotePropertiesMode();

	// edit note property. We do not support the note key property.
	if ( m_mode != Mode::NoteKey ) {

		float fValue = 0.0;
		if ( m_mode == Mode::Velocity ) {
			fValue = m_pDraggedNote->get_velocity();
		}
		else if ( m_mode == Mode::Pan ) {
			fValue = m_pDraggedNote->getPanWithRangeFrom0To1();
		}
		else if ( m_mode == Mode::LeadLag ) {
			fValue = ( m_pDraggedNote->get_lead_lag() - 1.0 ) / -2.0 ;
		}
		else if ( m_mode == Mode::Probability ) {
			fValue = m_pDraggedNote->get_probability();
		}
		
		float fMoveY = m_nOldPoint - ev->y();
		fValue = fValue  + (fMoveY / 100);
		if ( fValue > 1 ) {
			fValue = 1;
		}
		else if ( fValue < 0.0 ) {
			fValue = 0.0;
		}

		if ( m_mode == Mode::Velocity ) {
			m_pDraggedNote->set_velocity( fValue );
			m_fVelocity = fValue;
		}
		else if ( m_mode == Mode::Pan ) {
			m_pDraggedNote->setPanWithRangeFrom0To1( fValue );
			m_fPan = m_pDraggedNote->getPan();
		}
		else if ( m_mode == Mode::LeadLag ) {
			m_pDraggedNote->set_lead_lag( ( fValue * -2.0 ) + 1.0 );
			m_fLeadLag = ( fValue * -2.0 ) + 1.0;
		}
		else if ( m_mode == Mode::Probability ) {
			m_pDraggedNote->set_probability( fValue );
			m_fProbability = fValue;
		}

		PatternEditor::triggerStatusMessage( m_pDraggedNote, m_mode );
		
		m_nOldPoint = ev->y();
	}

	m_pAudioEngine->unlock(); // unlock the audio engine
	Hydrogen::get_instance()->setIsModified( true );

	if ( m_pPatternEditorPanel != nullptr ) {
		m_pPatternEditorPanel->updateEditors( true );
	}
}

void PatternEditor::mouseDragEndEvent( QMouseEvent* ev ) {

	UNUSED( ev );
	unsetCursor();
	
	if ( m_pPattern == nullptr || m_nSelectedPatternNumber == -1 ) {
		return;
	}

	if ( m_pDraggedNote == nullptr || m_pDraggedNote->get_note_off() ) {
		return;
	}

	if ( m_pDraggedNote->get_length() != m_nOldLength ) {
		SE_editNoteLengthAction *action =
			new SE_editNoteLengthAction( m_pDraggedNote->get_position(),
										 m_pDraggedNote->get_position(),
										 m_nRow,
										 m_pDraggedNote->get_length(),
										 m_nOldLength,
										 m_nSelectedPatternNumber,
										 m_nSelectedInstrumentNumber,
										 m_editor );
		HydrogenApp::get_instance()->m_pUndoStack->push( action );
	}


	if( m_fVelocity == m_fOldVelocity &&
		m_fOldPan == m_fPan &&
		m_fOldLeadLag == m_fLeadLag &&
		m_fOldProbability == m_fProbability ) {
		return;
	}
		
	SE_editNotePropertiesAction *action =
		new SE_editNotePropertiesAction( m_pDraggedNote->get_position(),
										 m_pDraggedNote->get_position(),
										 m_nRow,
										 m_nSelectedPatternNumber,
										 m_nSelectedInstrumentNumber,
										 m_mode,
										 m_editor,
										 m_fVelocity,
										 m_fOldVelocity,
										 m_fPan,
										 m_fOldPan,
										 m_fLeadLag,
										 m_fOldLeadLag,
										 m_fProbability,
										 m_fOldProbability );
	HydrogenApp::get_instance()->m_pUndoStack->push( action );
}

void PatternEditor::editNoteLengthAction( int nColumn,
										  int nRealColumn,
										  int nRow,
										  int nLength,
										  int nSelectedPatternNumber,
										  int nSelectedInstrumentnumber,
										  Editor editor)
{

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPatternList = pSong->getPatternList();

	H2Core::Pattern* pPattern = nullptr;
	if ( nSelectedPatternNumber != -1 &&
		 nSelectedPatternNumber < pPatternList->size() ) {
		pPattern = pPatternList->get( nSelectedPatternNumber );
	}

	if ( pPattern == nullptr ) {
		return;
	}

	m_pAudioEngine->lock( RIGHT_HERE );

	// Find the note to edit
	Note* pDraggedNote = nullptr;
	if ( editor == Editor::PianoRoll ) {
		auto pSelectedInstrument =
			pSong->getInstrumentList()->get( nSelectedInstrumentnumber );
		if ( pSelectedInstrument == nullptr ) {
			ERRORLOG( "No instrument selected" );
			m_pAudioEngine->unlock();
			return;
		}
		
		Note::Octave pressedOctave = Note::pitchToOctave( lineToPitch( nRow ) );
		Note::Key pressedNoteKey = Note::pitchToKey( lineToPitch( nRow ) );

		auto pDraggedNote = pPattern->find_note( nColumn, nRealColumn,
												 pSelectedInstrument,
												 pressedNoteKey, pressedOctave,
												 false );
	}
	else if ( editor == Editor::DrumPattern ) {
		auto pSelectedInstrument = pSong->getInstrumentList()->get( nRow );
		if ( pSelectedInstrument == nullptr ) {
			ERRORLOG( "No instrument selected" );
			m_pAudioEngine->unlock();
			return;
		}
		pDraggedNote = pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, false );
	}
	else {
		ERRORLOG( QString( "Unsupported editor [%1]" )
				  .arg( static_cast<int>(editor) ) );
		m_pAudioEngine->unlock();
		return;
	}	
		
	if ( pDraggedNote != nullptr ){
		pDraggedNote->set_length( nLength );
	}

	m_pAudioEngine->unlock();
	
	pHydrogen->setIsModified( true );

	if ( m_pPatternEditorPanel != nullptr ) {
		m_pPatternEditorPanel->updateEditors( true );
	}
}


void PatternEditor::editNotePropertiesAction( int nColumn,
											  int nRealColumn,
											  int nRow,
											  int nSelectedPatternNumber,
											  int nSelectedInstrumentNumber,
											  Mode mode,
											  Editor editor,
											  float fVelocity,
											  float fPan,
											  float fLeadLag,
											  float fProbability )
{

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	auto pPatternList = pSong->getPatternList();

	H2Core::Pattern* pPattern = nullptr;
	if ( nSelectedPatternNumber != -1 &&
		 nSelectedPatternNumber < pPatternList->size() ) {
		pPattern = pPatternList->get( nSelectedPatternNumber );
	}

	if ( pPattern == nullptr ) {
		return;
	}

	m_pAudioEngine->lock( RIGHT_HERE );

	// Find the note to edit
	Note* pDraggedNote = nullptr;
	if ( editor == Editor::PianoRoll ) {
		
		auto pSelectedInstrument =
			pSong->getInstrumentList()->get( nSelectedInstrumentNumber );
		if ( pSelectedInstrument == nullptr ) {
			ERRORLOG( "No instrument selected" );
			m_pAudioEngine->unlock();
			return;
		}
		
		Note::Octave pressedOctave = Note::pitchToOctave( lineToPitch( nRow ) );
		Note::Key pressedNoteKey = Note::pitchToKey( lineToPitch( nRow ) );

		pDraggedNote = pPattern->find_note( nColumn, nRealColumn,
												 pSelectedInstrument,
												 pressedNoteKey, pressedOctave,
												 false );
	}
	else if ( editor == Editor::DrumPattern ) {
		auto pSelectedInstrument = pSong->getInstrumentList()->get( nRow );
		if ( pSelectedInstrument == nullptr ) {
			ERRORLOG( "No instrument selected" );
			m_pAudioEngine->unlock();
			return;
		}
		pDraggedNote = pPattern->find_note( nColumn, nRealColumn, pSelectedInstrument, false );
	}
	else {
		ERRORLOG( QString( "Unsupported editor [%1]" )
				  .arg( static_cast<int>(editor) ) );
		m_pAudioEngine->unlock();
		return;
	}

	bool bValueChanged = true;

	if ( pDraggedNote != nullptr ){
		switch ( mode ) {
		case Mode::Velocity:
			pDraggedNote->set_velocity( fVelocity );
			break;
		case Mode::Pan:
			pDraggedNote->setPan( fPan );
			break;
		case Mode::LeadLag:
			pDraggedNote->set_lead_lag( fLeadLag );
			break;
		case Mode::Probability:
			pDraggedNote->set_probability( fProbability );
			break;
		}			
		bValueChanged = true;
		PatternEditor::triggerStatusMessage( pDraggedNote, mode );
	} else {
		ERRORLOG("note could not be found");
	}

	m_pAudioEngine->unlock();

	if ( bValueChanged &&
		 m_pPatternEditorPanel != nullptr ) {
		pHydrogen->setIsModified( true );
		m_pPatternEditorPanel->updateEditors( true );
	}
}

					
QString PatternEditor::modeToQString( Mode mode ) {
	QString s;
	
	switch ( mode ) {
	case PatternEditor::Mode::Velocity:
		s = "Velocity";
		break;
	case PatternEditor::Mode::Pan:
		s = "Pan";
		break;
	case PatternEditor::Mode::LeadLag:
		s = "LeadLag";
		break;
	case PatternEditor::Mode::NoteKey:
		s = "NoteKey";
		break;
	case PatternEditor::Mode::Probability:
		s = "Probability";
		break;
	default:
		s = QString( "Unknown mode [%1]" ).arg( static_cast<int>(mode) ) ;
		break;
	}

	return s;
}

void PatternEditor::triggerStatusMessage( Note* pNote, Mode mode ) {
	QString s;
	QString sCaller( _class_name() );
	QString sUnit( tr( "ticks" ) );
	float fValue;
	
	switch ( mode ) {
	case PatternEditor::Mode::Velocity:
		if ( ! pNote->get_note_off() ) {
			s = QString( tr( "Set note velocity" ) )
				.append( QString( ": [%1]")
						 .arg( pNote->get_velocity(), 2, 'f', 2 ) );
			sCaller.append( ":Velocity" );
		}
		break;
		
	case PatternEditor::Mode::Pan:
		if ( ! pNote->get_note_off() ) {

			// Round the pan to not miss the center due to fluctuations
			fValue = pNote->getPan() * 100;
			fValue = std::round( fValue );
			fValue = fValue / 100;
			
			if ( fValue > 0.0 ) {
				s = QString( tr( "Note panned to the right by" ) ).
					append( QString( ": [%1]" ).arg( fValue / 2, 2, 'f', 2 ) );
			} else if ( fValue < 0.0 ) {
				s = QString( tr( "Note panned to the left by" ) ).
					append( QString( ": [%1]" ).arg( -1 * fValue / 2, 2, 'f', 2 ) );
			} else {
				s = QString( tr( "Note centered" ) );
			}
			sCaller.append( ":Pan" );
		}
		break;
		
	case PatternEditor::Mode::LeadLag:
		// Round the pan to not miss the center due to fluctuations
		fValue = pNote->get_lead_lag() * 100;
		fValue = std::round( fValue );
		fValue = fValue / 100;
		if ( fValue < 0.0 ) {
			s = QString( tr( "Leading beat by" ) )
				.append( QString( ": [%1] " )
						 .arg( fValue * -1 *
							   AudioEngine::getLeadLagInTicks() , 2, 'f', 2 ) )
				.append( sUnit );
		}
		else if ( fValue > 0.0 ) {
			s = QString( tr( "Lagging beat by" ) )
				.append( QString( ": [%1] " )
						 .arg( fValue *
							   AudioEngine::getLeadLagInTicks() , 2, 'f', 2 ) )
				.append( sUnit );
		}
		else {
			s = tr( "Note on beat" );
		}
		sCaller.append( ":LeadLag" );
		break;

	case PatternEditor::Mode::NoteKey:
		s = QString( tr( "Set pitch" ) ).append( ": " ).append( tr( "key" ) )
			.append( QString( " [%1], " ).arg( Note::KeyToQString( pNote->get_key() ) ) )
			.append( tr( "octave" ) )
			.append( QString( ": [%1]" ).arg( pNote->get_octave() ) );
		sCaller.append( ":NoteKey" );
		break;

	case PatternEditor::Mode::Probability:
		if ( ! pNote->get_note_off() ) {
			s = tr( "Set note probability to" )
				.append( QString( ": [%1]" ).arg( pNote->get_probability(), 2, 'f', 2 ) );
		}
		sCaller.append( ":Probability" );
		break;

	default:
		ERRORLOG( PatternEditor::modeToQString( mode ) );
	}

	if ( ! s.isEmpty() ) {
		HydrogenApp::get_instance()->showStatusBarMessage( s, sCaller );
	}
}
