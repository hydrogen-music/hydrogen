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

#include <cassert>

#include "PianoRollEditor.h"
#include "PatternEditorPanel.h"
#include "../CommonStrings.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Helpers/Xml.h>
#include <core/Hydrogen.h>
#include <core/Preferences/Preferences.h>
#include <core/Preferences/Theme.h>


using namespace H2Core;

PitchLabel::PitchLabel( QWidget* pParent, const QString& sText, int nHeight )
	: QLabel( pParent )
	, m_pParent( pParent )
	, m_bEntered( false )
	, m_sText( sText )
	, m_bSelected( false )
{
	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	setFixedWidth( PatternEditor::nMarginSidebar );
	setFixedHeight( nHeight );
	setText( sText );
	setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	setIndent( 2 );

	updateFont();
	setBackgroundColor( theme.m_color.m_patternEditor_backgroundColor );
	updateStyleSheet();
}

PitchLabel::~PitchLabel() {
}

void PitchLabel::setBackgroundColor( const QColor& backgroundColor ) {
	if ( m_backgroundColor == backgroundColor ) {
		return;
	}

	m_backgroundColor =
		backgroundColor.darker( Skin::nListBackgroundColorScaling );
	update();
}


void PitchLabel::updateStyleSheet() {
	const auto colorTheme =
		H2Core::Preferences::get_instance()->getTheme().m_color;

	QColor textColor;
	if ( m_bSelected ) {
		textColor = colorTheme.m_patternEditor_selectedRowTextColor;
	} else {
		textColor = colorTheme.m_patternEditor_textColor;
	}
	const auto cursorColor = colorTheme.m_cursorColor;

	if ( m_textColor == textColor && m_cursorColor == cursorColor ) {
		return;
	}

	if ( m_textColor != textColor ) {
		m_textColor = textColor;
	}
	if ( m_cursorColor != cursorColor ) {
		m_cursorColor = cursorColor;
	}

	setStyleSheet( QString( "\
QLabel {\
   color: %1;\
   font-weight: bold;\
 }" ).arg( textColor.name() ) );
}

void PitchLabel::enterEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void PitchLabel::leaveEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void PitchLabel::mousePressEvent( QMouseEvent* pEvent ) {
	auto pSidebarRow = dynamic_cast<PitchSidebar*>( m_pParent );
	if ( pSidebarRow != nullptr ) {
		pSidebarRow->rowPressed( pEvent, this );
	}
}

void PitchLabel::paintEvent( QPaintEvent* ev )
{
	auto p = QPainter( this );

	Skin::drawListBackground( &p, QRect( 0, 0, width(), height() ),
							  m_backgroundColor, m_bEntered );

	if ( m_bSelected && ! HydrogenApp::get_instance()->hideKeyboardCursor() ) {
		QPen pen;

		pen.setColor( m_cursorColor );

		pen.setWidth( 2 );
		p.setPen( pen );
		p.setRenderHint( QPainter::Antialiasing );
		p.drawRoundedRect( QRect( 1, 1, width() - 2, height() - 2 ), 2, 2 );
	}

	QLabel::paintEvent( ev );
}

void PitchLabel::updateFont() {

	const auto theme = H2Core::Preferences::get_instance()->getTheme();

	float fScalingFactor = 1.0;
    switch ( theme.m_font.m_fontSize ) {
    case H2Core::FontTheme::FontSize::Small:
		fScalingFactor = 0.8;
		break;
    case H2Core::FontTheme::FontSize::Medium:
		fScalingFactor = 1.0;
		break;
    case H2Core::FontTheme::FontSize::Large:
		fScalingFactor = 1.0;
		break;
	}

	const int nMargin = 1;
	int nPixelSize = std::round( ( height() - nMargin ) * fScalingFactor );

	QFont font( theme.m_font.m_sLevel2FontFamily );
	font.setBold( true );
	font.setPixelSize( nPixelSize );

	// Check whether the width of the text fits the available frame width of the
	// button.
	while ( QFontMetrics( font ).size( Qt::TextSingleLine, text() ).width() >
			( width() - indent() ) && nPixelSize > 1 ) {
		nPixelSize--;
		font.setPixelSize( nPixelSize );
	}

	// This method must not be called more than once in this routine. Otherwise,
	// a repaint of the widget is triggered, which calls `updateFont()` again
	// and we are trapped in an infinite loop.
	setFont( font );
}

void PitchLabel::setSelected( bool bSelected )
{
	if ( bSelected == m_bSelected ) {
		return;
	}

	m_bSelected = bSelected;

	updateStyleSheet();
	update();
}

////////////////////////////////////////////////////////////////////////////////

PitchSidebar::PitchSidebar( QWidget *parent, int nHeight, int nGridHeight )
	: QWidget( parent )
	, m_nHeight( nHeight )
	, m_nGridHeight( nGridHeight )
	, m_nRowClicked( 0 )
 {
	 auto pPatternEditorPanel =
		 HydrogenApp::get_instance()->getPatternEditorPanel();
	 const auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	resize( PatternEditor::nMarginSidebar, nHeight );

	m_rows.resize( OCTAVE_NUMBER * KEYS_PER_OCTAVE );

	auto pSidebarVBox = new QVBoxLayout( this );
	pSidebarVBox->setSpacing( 0 );
	pSidebarVBox->setMargin( 0 );
	pSidebarVBox->setMargin( Qt::AlignLeft );
	setLayout( pSidebarVBox );

	auto createLabel = [&]( const QString& sText, int* pIndex ) {
		if ( pIndex == nullptr ||
			 *pIndex < 0 || *pIndex >= m_rows.size() ) {
			ERRORLOG( "invalid index" );
			return;
		}

		auto pLabel = new PitchLabel( this, sText, nGridHeight );
		pSidebarVBox->addWidget( pLabel );
		m_rows[ *pIndex ] = pLabel;
		*pIndex = *pIndex + 1;
	};

	// C5. Highest pitch on top.
	int nnActualOctave = 5;
	int nnIndex = 0;
	for ( int nnOctave = 0; nnOctave < OCTAVE_NUMBER; ++nnOctave ) {
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchB() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchASharp() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchA() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchGSharp() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchG() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchFSharp() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchF() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchE() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchDSharp() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchD() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchCSharp() )
					 .arg( nnActualOctave ) , &nnIndex );
		createLabel( QString( "%1%2" ).arg( pCommonStrings->getNotePitchC() )
					 .arg( nnActualOctave ) , &nnIndex );
		--nnActualOctave;
	}

	// Popup menu
	m_pFunctionPopup = new QMenu( this );
	auto clearAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionClearAllNotesInRow() );
	connect( clearAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->clearNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			pPatternEditorPanel->getPatternNumber(),
			Note::lineToPitch( m_nRowClicked ) ); } );

	m_pFunctionPopupSub = new QMenu(
		pCommonStrings->getActionFillNotes(), m_pFunctionPopup );
	auto fillAllAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillAllNotes() );
	connect( fillAllAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::All,
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto fillEverySecondAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEverySecondNote() );
	connect( fillEverySecondAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::EverySecond,
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto fillEveryThirdAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryThirdNote() );
	connect( fillEveryThirdAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::EveryThird,
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto fillEveryFourthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryFourthNote() );
	connect( fillEveryFourthAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::EveryFourth,
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto fillEverySixthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEverySixthNote() );
	connect( fillEverySixthAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::EverySixth,
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto fillEveryEighthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryEighthNote() );
	connect( fillEveryEighthAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::EveryEighth,
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto fillEveryTwelfthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryTwelfthNote() );
	connect( fillEveryTwelfthAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::EveryTwelfth,
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto fillEverySixteenthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEverySixteenthNote() );
	connect( fillEverySixteenthAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->fillNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			PatternEditorPanel::FillNotes::EverySixteenth,
			Note::lineToPitch( m_nRowClicked ) ); } );
	m_pFunctionPopup->addMenu( m_pFunctionPopupSub );

	auto selectNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionSelectNotes() );
	connect( selectNotesAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->getVisibleEditor()->selectAllNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(),
			Note::lineToPitch( m_nRowClicked ) ); } );

	m_pFunctionPopup->addSection( pCommonStrings->getActionEditAllPatterns() );
	auto cutNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionCutAllNotes() );
	connect( cutNotesAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->cutNotesFromRowOfAllPatterns(
			pPatternEditorPanel->getSelectedRowDB(),
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto copyNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionCopyNotes() );
	connect( copyNotesAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->copyNotesFromRowOfAllPatterns(
			pPatternEditorPanel->getSelectedRowDB(),
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto pasteNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionPasteAllNotes() );
	connect( pasteNotesAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->pasteNotesToRowOfAllPatterns(
			pPatternEditorPanel->getSelectedRowDB(),
			Note::lineToPitch( m_nRowClicked ) ); } );
	auto clearAllAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionClearAllNotes() );
	connect( clearAllAction, &QAction::triggered, this, [=](){
		pPatternEditorPanel->clearNotesInRow(
			pPatternEditorPanel->getSelectedRowDB(), -1,
			Note::lineToPitch( m_nRowClicked ) ); } );

	m_pFunctionPopup->setObjectName( "PianoRollFunctionPopup" );
}

PitchSidebar::~PitchSidebar() {
}

void PitchSidebar::updateRows(){
	for ( auto& rrow : m_rows ) {
		rrow->update();
	}
}

void PitchSidebar::setRowColor( int nRowIndex, const QColor& backgroundColor ) {
	if ( nRowIndex < 0 || nRowIndex >= m_rows.size() ) {
		ERRORLOG( QString( "Provided index [%1] out of bound [0,%2]" )
				  .arg( nRowIndex ).arg( m_rows.size() - 1 ) );
		return;
	}

	m_rows[ nRowIndex ]->setBackgroundColor( backgroundColor );
}

void PitchSidebar::updateStyleSheet() {
	for ( auto& rrow : m_rows ) {
		rrow->updateStyleSheet();
	}
}

void PitchSidebar::updateFont() {
	for ( auto& rrow : m_rows ) {
		rrow->updateFont();
	}
}

void PitchSidebar::selectedRow( int nRowIndex ) {
	if ( nRowIndex < 0 || nRowIndex >= m_rows.size() ) {
		ERRORLOG( QString( "Provided index [%1] out of bound [0,%2]" )
				  .arg( nRowIndex ).arg( m_rows.size() - 1 ) );
		return;
	}

	for ( int nnIndex = 0; nnIndex < m_rows.size(); ++nnIndex ) {
		m_rows[ nnIndex ]->setSelected( nnIndex == nRowIndex );
	}
}

void PitchSidebar::rowPressed( QMouseEvent* pEvent, PitchLabel* pLabel ) {
	if ( pLabel == nullptr ) {
		return;
	}
	int nRowClicked = -1;
	for ( int nnRow = 0; nnRow < m_rows.size(); ++nnRow ) {
		if ( m_rows[ nnRow ] == pLabel ) {
			nRowClicked = nnRow;
			break;
		}
	}

	if ( nRowClicked == -1 ) {
		ERRORLOG( "row not found" );
		return;
	}

	m_nRowClicked = nRowClicked;

	auto pPatternEditorPanel =
		HydrogenApp::get_instance()->getPatternEditorPanel();
	pPatternEditorPanel->getVisibleEditor()->setCursorPitch(
		Note::lineToPitch( nRowClicked ) );

	if ( pEvent->button() == Qt::RightButton ) {
		m_pFunctionPopup->popup( QPoint( pEvent->globalX(), pEvent->globalY() ) );
	}

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	pPatternEditorPanel->getVisibleEditor()->handleKeyboardCursor( false );
}

////////////////////////////////////////////////////////////////////////////////

PianoRollEditor::PianoRollEditor( QWidget *pParent )
	: PatternEditor( pParent )
{
	m_editor = PatternEditor::Editor::PianoRoll;

	const auto pPref = H2Core::Preferences::get_instance();
	QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
				getPointSize( pPref->getTheme().m_font.m_fontSize ) );
	setFont( font );
	
	m_nGridHeight = 13;

	setAttribute(Qt::WA_OpaquePaintEvent);

	m_nEditorHeight = OCTAVE_NUMBER * KEYS_PER_OCTAVE * m_nGridHeight;

	resize( m_nEditorWidth, m_nEditorHeight );

	// Create the sidebar of labels
	m_pPitchSidebar = new PitchSidebar( this, m_nEditorHeight, m_nGridHeight );
}

PianoRollEditor::~PianoRollEditor() {
}

QPoint PianoRollEditor::noteToPoint( std::shared_ptr<H2Core::Note> pNote ) const {
	if ( pNote == nullptr ) {
		return QPoint();
	}

	return QPoint(
		PatternEditor::nMarginSidebar + pNote->getPosition() * m_fGridWidth,
		m_nGridHeight *
		Note::pitchToLine( pNote->getPitchFromKeyOctave() ) + 1 );
}

void PianoRollEditor::paintEvent(QPaintEvent *ev)
{
	if (!isVisible()) {
		return;
	}

	PatternEditor::paintEvent( ev );

	QPainter painter( this );

	const auto row = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );

	// Draw hovered note
	const auto pPattern = m_pPatternEditorPanel->getPattern();
	for ( const auto& [ ppPattern, nnotes ] :
			  m_pPatternEditorPanel->getHoveredNotes() ) {
		const auto baseStyle = static_cast<NoteStyle>(
			( ppPattern == pPattern ? NoteStyle::Foreground :
			  NoteStyle::Background ) | NoteStyle::Hovered);
		for ( const auto& ppNote : nnotes ) {
			if ( ppNote != nullptr && ppNote->getType() == row.sType &&
				 ppNote->getInstrumentId() == row.nInstrumentID ) {
				const auto style = static_cast<NoteStyle>(
					m_selection.isSelected( ppNote ) ?
					NoteStyle::Selected | baseStyle : baseStyle );
				drawNote( painter, ppNote, style );
			}
		}
	}

	// Draw moved notes
	if ( ! m_selection.isEmpty() && m_selection.isMoving() ) {
		for ( const auto& ppNote : m_selection ) {
			if ( ppNote != nullptr && ppNote->getType() == row.sType &&
				 ppNote->getInstrumentId() == row.nInstrumentID ) {
				drawNote( painter, ppNote, NoteStyle::Moved );
			}
		}
	}
}

void PianoRollEditor::createBackground()
{
	const auto pPref = H2Core::Preferences::get_instance();

	auto pPattern = m_pPatternEditorPanel->getPattern();
	
	QColor backgroundColor(
		pPref->getTheme().m_color.m_patternEditor_backgroundColor );
	const QColor backgroundInactiveColor(
		pPref->getTheme().m_color.m_windowColor );
	QColor alternateRowColor(
		pPref->getTheme().m_color.m_patternEditor_alternateRowColor );
	QColor octaveColor = pPref->getTheme().m_color.m_patternEditor_octaveRowColor;
	QColor lineColor( pPref->getTheme().m_color.m_patternEditor_lineColor );
	// Row clicked by the user.
	QColor selectedRowColor(
		pPref->getTheme().m_color.m_patternEditor_selectedRowColor );
	const QColor lineInactiveColor(
		pPref->getTheme().m_color.m_windowTextColor.darker( 170 ) );

	if ( ! hasFocus() ) {
		lineColor = lineColor.darker( PatternEditor::nOutOfFocusDim );
		backgroundColor = backgroundColor.darker( PatternEditor::nOutOfFocusDim );
		alternateRowColor =
			alternateRowColor.darker( PatternEditor::nOutOfFocusDim );
		octaveColor = octaveColor.darker( PatternEditor::nOutOfFocusDim );
		selectedRowColor = selectedRowColor.darker( PatternEditor::nOutOfFocusDim );
	}

	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nEditorWidth ||
		 m_pBackgroundPixmap->height() != m_nEditorHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( m_nEditorWidth * pixelRatio,
										   m_nEditorHeight * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
		delete m_pPatternPixmap;
		m_pPatternPixmap = new QPixmap( m_nEditorWidth  * pixelRatio,
										m_nEditorHeight * pixelRatio );
		m_pPatternPixmap->setDevicePixelRatio( pixelRatio );
	}

	m_pBackgroundPixmap->fill( backgroundInactiveColor );

	QPainter p( m_pBackgroundPixmap );

	auto fillGridLines = [&]( int nLineHeight, bool bRenderInactive,
							  bool bUpdateSidebar ) {

		const int nSelectedRow = Note::pitchToLine( m_nCursorPitch );
		if ( bUpdateSidebar ) {
			m_pPitchSidebar->selectedRow( nSelectedRow );
		}

		int nnRow = 0;
		QColor color;
		for ( int nnOctave = 0; nnOctave < OCTAVE_NUMBER; ++nnOctave ) {
			const int nStartY = nnOctave * KEYS_PER_OCTAVE * m_nGridHeight;

			for ( int nnKey = 0; nnKey < KEYS_PER_OCTAVE; ++nnKey ) {
				if ( nnRow == nSelectedRow ) {
					color = selectedRowColor;
				}
				else if ( nnKey == 0 || nnKey == 2 || nnKey == 4 || nnKey == 6 ||
						  nnKey == 7 || nnKey == 9 || nnKey == 11 ) {
					if ( nnOctave % 2 != 0 ) {
						color = octaveColor;
					} else {
						color = backgroundColor;
					}
				}
				else {
					color = alternateRowColor;
				}

				p.fillRect( 0, nStartY + nnKey * m_nGridHeight,
							m_nActiveWidth - 0, nLineHeight, color );

				if ( bUpdateSidebar ) {
					m_pPitchSidebar->setRowColor( nnRow, color );
				}

				if ( bRenderInactive && m_nActiveWidth + 1 < m_nEditorWidth ) {
					p.fillRect( m_nActiveWidth,
								nStartY + nnKey * m_nGridHeight,
								m_nEditorWidth - m_nActiveWidth, nLineHeight,
								backgroundInactiveColor );
				}
				++nnRow;
			}
		}
	};

	// basic background
	fillGridLines( m_nGridHeight, false, true );

	// grid markers
	if ( pPattern != nullptr ) {
		drawGridLines( p, Qt::SolidLine );

		// Erase part of the grid lines to create grid markers
		fillGridLines( m_nGridHeight * 0.6, true, false );
	}

	// horiz lines
	p.setPen( QPen( lineColor, 1, Qt::DotLine ) );
	for ( uint row = 0; row < ( KEYS_PER_OCTAVE * OCTAVE_NUMBER ); ++row ) {
		unsigned y = row * m_nGridHeight;
		p.drawLine( PatternEditor::nMarginSidebar, y, m_nActiveWidth, y );
	}

	if ( m_nActiveWidth + 1 < m_nEditorWidth ) {
		p.setPen( QPen( lineInactiveColor, 1, Qt::DotLine ) );
		for ( uint row = 0; row < ( KEYS_PER_OCTAVE * OCTAVE_NUMBER ); ++row ) {
			unsigned y = row * m_nGridHeight;
			p.drawLine( m_nActiveWidth, y, m_nEditorWidth, y );
		}
	}

	// borders
	drawBorders( p );

	p.setPen( lineColor );
	p.drawLine( PatternEditor::nMarginSidebar, 0,
				PatternEditor::nMarginSidebar, m_nEditorHeight );
}

void PianoRollEditor::updateFont() {
	m_pPitchSidebar->updateFont();
}

void PianoRollEditor::updateStyleSheet() {
	m_pPitchSidebar->updateStyleSheet();
}

void PianoRollEditor::selectAll()
{
	selectAllNotesInRow( m_pPatternEditorPanel->getSelectedRowDB() );
}

void PianoRollEditor::keyPressEvent( QKeyEvent * ev )
{
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return;
	}

	auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row [%1]" );
		return;
	}

	const int nBlockSize = 5;
	bool bIsSelectionKey = m_selection.keyPressEvent( ev );
	bool bUnhideCursor = true;
	bool bEventUsed = true;
	updateModifiers( ev );

	if ( bIsSelectionKey ) {
		// Selection key, nothing more to do (other than update editor)
	}
	else if ( ev->matches( QKeySequence::MoveToNextLine ) ||
			  ev->matches( QKeySequence::SelectNextLine ) ) {
		if ( m_nCursorPitch > Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
													(Note::Key)KEY_MIN ) ) {
			setCursorPitch( m_nCursorPitch - 1 );
		}
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfBlock ) ||
			  ev->matches( QKeySequence::SelectEndOfBlock ) ) {
		setCursorPitch( std::max( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
														(Note::Key)KEY_MIN ),
								m_nCursorPitch - nBlockSize ) );
	}
	else if ( ev->matches( QKeySequence::MoveToNextPage ) ||
			  ev->matches( QKeySequence::SelectNextPage ) ) {
		// Page down -- move down by a whole octave
		const int nMinPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
													  (Note::Key)KEY_MIN );
		setCursorPitch( std::max( m_nCursorPitch - KEYS_PER_OCTAVE, nMinPitch ) );
	}
	else if ( ev->matches( QKeySequence::MoveToEndOfDocument ) ||
			  ev->matches( QKeySequence::SelectEndOfDocument ) ) {
		setCursorPitch( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MIN,
											  (Note::Key)KEY_MIN ) );
	}
	else if ( ev->matches( QKeySequence::MoveToPreviousLine ) ||
			  ev->matches( QKeySequence::SelectPreviousLine ) ) {
		if ( m_nCursorPitch < Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
													(Note::Key)KEY_MAX ) ) {
			setCursorPitch( m_nCursorPitch + 1 );
		}
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfBlock ) ||
			  ev->matches( QKeySequence::SelectStartOfBlock ) ) {
		setCursorPitch( std::min( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
														(Note::Key)KEY_MAX ),
								m_nCursorPitch + nBlockSize ) );
	}
	else if ( ev->matches( QKeySequence::MoveToPreviousPage ) ||
			  ev->matches( QKeySequence::SelectPreviousPage ) ) {
		const int nMaxPitch = Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
													  (Note::Key)KEY_MAX );
		setCursorPitch( std::min( m_nCursorPitch + KEYS_PER_OCTAVE, nMaxPitch ) );
	}
	else if ( ev->matches( QKeySequence::MoveToStartOfDocument ) ||
			  ev->matches( QKeySequence::SelectStartOfDocument ) ) {
		setCursorPitch( Note::octaveKeyToPitch( (Note::Octave)OCTAVE_MAX,
											  (Note::Key)KEY_MAX ) );
	}
	else if ( ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return ) {
		// Key: Enter/Return : Place or remove note at current position
		m_selection.clearSelection();
		int pressedline = Note::pitchToLine( m_nCursorPitch );
		int nPitch = Note::lineToPitch( pressedline );
		m_pPatternEditorPanel->addOrRemoveNotes(
			m_pPatternEditorPanel->getCursorColumn(),
			m_pPatternEditorPanel->getSelectedRowDB(),
			Note::pitchToKey( nPitch ), Note::pitchToOctave( nPitch ),
			/* bDoAdd */ true, /* bDoDelete */ true,
			/* bIsNoteOff */ false,
			PatternEditor::AddNoteAction::Playback );
	}
	else if ( ev->key() == Qt::Key_Delete ) {
		// Key: Delete: delete selection or note at keyboard cursor
		if ( m_selection.begin() != m_selection.end() ) {
			deleteSelection();
		}
		else {
			// Delete a note under the keyboard cursor
			int pressedline = Note::pitchToLine( m_nCursorPitch );
			int nPitch = Note::lineToPitch( pressedline );
			m_pPatternEditorPanel->addOrRemoveNotes(
				m_pPatternEditorPanel->getCursorColumn(),
				m_pPatternEditorPanel->getSelectedRowDB(),
				Note::pitchToKey( nPitch ), Note::pitchToOctave( nPitch ),
				/* bDoAdd */ false, /* bDoDelete */ true,
				/* bIsNoteOff */ false,
				PatternEditor::AddNoteAction::None );
		}
	}
	else {
		bEventUsed = false;
	}

	if ( ! bEventUsed ) {
		ev->setAccepted( false );
	}


	PatternEditor::keyPressEvent( ev );
}

std::vector<PianoRollEditor::SelectionIndex> PianoRollEditor::elementsIntersecting( const QRect& r )
{
	std::vector<SelectionIndex> result;
	auto pPattern = m_pPatternEditorPanel->getPattern();
	if ( pPattern == nullptr ) {
		return std::move( result );
	}

	const auto selectedRow = m_pPatternEditorPanel->getRowDB(
		m_pPatternEditorPanel->getSelectedRowDB() );
	if ( selectedRow.nInstrumentID == EMPTY_INSTR_ID &&
		 selectedRow.sType.isEmpty() ) {
		DEBUGLOG( "Empty row" );
		return std::move( result );
	}
	
	int w = 8;
	int h = m_nGridHeight - 2;

	auto rNormalized = r.normalized();
	if ( rNormalized.top() == rNormalized.bottom() &&
		 rNormalized.left() == rNormalized.right() ) {
		rNormalized += QMargins( 2, 2, 2, 2 );
	}

	// Calculate the first and last position values that this rect will
	// intersect with
	int x_min = (rNormalized.left() - w - PatternEditor::nMargin) / m_fGridWidth;
	int x_max = (std::min( rNormalized.right(), m_nActiveWidth ) + w -
				 PatternEditor::nMargin) / m_fGridWidth;

	const Pattern::notes_t* pNotes = pPattern->getNotes();

	for ( auto it = pNotes->lower_bound( x_min ); it != pNotes->end() && it->first <= x_max; ++it ) {
		auto pNote = it->second;
		if ( pNote->getInstrumentId() == selectedRow.nInstrumentID ||
			 pNote->getType() == selectedRow.sType ) {
			const auto notePoint = noteToPoint( pNote );

			if ( rNormalized.intersects( QRect( notePoint.x() -4 , notePoint.y(),
												w, h ) ) ) {
				result.push_back( pNote );
			}
		}
	}
	updateEditor( true );
	return std::move( result );
}
