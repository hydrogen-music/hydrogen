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

#include "PatternEditorSidebar.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>
#include <core/Basics/Drumkit.h>
#include <core/Basics/Instrument.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Note.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include "PianoRollEditor.h"
#include "../CommonStrings.h"
#include "../Compatibility/DropEvent.h"
#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../MainForm.h"
#include "../Skin.h"
#include "../UndoActions.h"
#include "../Widgets/Button.h"

#include <QtGui>
#include <QtWidgets>
#include <QClipboard>

#include <cassert>
#include <algorithm> // for std::min
#include <memory>

using namespace H2Core;

SidebarLabel::SidebarLabel( QWidget* pParent, Type type, const QSize& size,
							const QString& sText, int nIndent )
	: QLabel( pParent )
	, m_pParent( pParent )
	, m_type( type )
	, m_nIndent( nIndent )
	, m_bShowPlusSign( false )
	, m_bEntered( false )
	, m_sText( sText )
	, m_bShowCursor( false )
	, m_bDimed( false )
{
	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();

	setFixedWidth( size.width() );
	setFixedHeight( size.height() );
	setText( sText );
	setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
	setIndent( nIndent );
	setContentsMargins( 1, 1, 1, 1 );

	updateFont();
	setColor( pColorTheme->m_patternEditor_backgroundColor,
			  pColorTheme->m_patternEditor_textColor,
			  pColorTheme->m_cursorColor );
	updateStyle();
}

SidebarLabel::~SidebarLabel() {
}

void SidebarLabel::setText( const QString& sNewText ) {
	if ( m_sText == sNewText ) {
		return;
	}

	m_sText = sNewText;

	updateFont();
	update();
}

void SidebarLabel::setShowPlusSign( bool bShowPlusSign ) {
	if ( bShowPlusSign == m_bShowPlusSign ){
		return;
	}

	m_bShowPlusSign = bShowPlusSign;

	updateStyle();
	update();
}

void SidebarLabel::setColor( const QColor& backgroundColor,
							 const QColor& textColor,
							 const QColor& cursorColor ) {
	if ( m_backgroundColor == backgroundColor &&
		 m_textBaseColor == textColor &&
		 m_cursorColor == cursorColor ) {
		return;
	}

	if ( m_backgroundColor != backgroundColor ) {
		m_backgroundColor = backgroundColor;
	}
	if ( m_textBaseColor != textColor ) {
		m_textBaseColor = textColor;
	}
	if ( m_cursorColor != cursorColor ) {
		m_cursorColor = cursorColor;
	}

	updateStyle();
}

#ifdef H2CORE_HAVE_QT6
void SidebarLabel::enterEvent( QEnterEvent *ev ) {
#else
void SidebarLabel::enterEvent( QEvent *ev ) {
#endif
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void SidebarLabel::leaveEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void SidebarLabel::mousePressEvent( QMouseEvent* pEvent ) {

	auto pSidebarRow = dynamic_cast<SidebarRow*>( m_pParent );
	if ( pSidebarRow != nullptr ) {
		pSidebarRow->mousePressEvent( pEvent );
	}

	emit labelClicked( pEvent );
}

void SidebarLabel::mouseDoubleClickEvent( QMouseEvent* pEvent ) {
	emit labelDoubleClicked( pEvent );
}

void SidebarLabel::paintEvent( QPaintEvent* ev )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pPatternEditorPanel = pHydrogenApp->getPatternEditorPanel();
	auto p = QPainter( this );

	QColor backgroundColor( m_backgroundColor );
	if ( m_bDimed ) {
		backgroundColor = backgroundColor.darker( SidebarLabel::nDimScaling );
	}

	Skin::drawListBackground( &p, QRect( 0, 0, width(), height() ),
							  backgroundColor, m_bEntered );

	if ( m_bShowPlusSign ) {
		const auto pPref = Preferences::get_instance();

		int nLineWidth, nHeight;
		switch ( pPref->getFontTheme()->m_fontSize ) {
		case H2Core::FontTheme::FontSize::Small:
			nHeight = height() - 11;
            nLineWidth = 2;
            break;
		case H2Core::FontTheme::FontSize::Medium:
            nHeight = height() - 10;
            nLineWidth = 3;
            break;
        case H2Core::FontTheme::FontSize::Large:
            nHeight = height() - 7;
            nLineWidth = 4;
            break;
        default:
            ERRORLOG( "Unknown font size" );
            return;
		}

		QColor color = m_bEntered ? pPref->getColorTheme()->m_highlightColor :
			m_textBaseColor;

		if ( m_bDimed ) {
			color = color.darker( SidebarLabel::nDimScaling );
		}

		int nMidX = std::round( width() / 2 );

		if ( ! text().isEmpty() && m_type == Type::Type ) {
			// If no instrument type was provided, we display the instrument id
			// as fallback to nevertheless allow interacting with the
			// corresponding notes.

			// We use a fixed space with the width of roughly the amount digits
			// of the largest instrument id rendered in the current font.
			int nIdMax = EMPTY_INSTR_ID;
			for ( const auto rrow : pPatternEditorPanel->getDB() ) {
				if ( rrow.nInstrumentID > nIdMax ) {
					nIdMax = rrow.nInstrumentID;
				}
			}
			const QString sReferenceText = QString( "0" ).repeated(
				QString::number( nIdMax ).length() );

			const int nTextWidth = margin() * 2 + indent() +
				QFontMetrics( font() ).size( Qt::TextSingleLine,
											 sReferenceText ).width();
			nMidX = std::round(( width() - nTextWidth ) / 2 ) + nTextWidth;

			// Border between fallback id and plus sign.
			QColor borderColor = backgroundColor.darker(
				Skin::nListBackgroundDarkBorderScaling );
			borderColor.setAlpha( 200 );

			p.setPen( borderColor );
			p.drawLine( nTextWidth, 1, nTextWidth, height() );
			p.setPen( Qt::NoPen );
		}

		// horizontal
		p.fillRect( QRect( nMidX - nHeight / 2,
						   height() / 2 - nLineWidth / 2, nHeight, nLineWidth ),
					color );

		// vertical
		p.fillRect( QRect( nMidX - nLineWidth / 2,
						   height() / 2 - nHeight / 2, nLineWidth, nHeight ),
					color );
	}

	if ( pPatternEditorPanel->hasPatternEditorFocus() &&
		 m_bShowCursor && ! pHydrogenApp->hideKeyboardCursor() ) {
		QPen pen;

		// Only within the drum pattern editor we are able to change the sidebar
		// column using keyboard events.
		QColor cursorColor( m_cursorColor );
		if ( ! pPatternEditorPanel->getDrumPatternEditor()->hasFocus() ) {
			cursorColor.setAlpha( Skin::nInactiveCursorAlpha );
		}

		pen.setColor( cursorColor );

		pen.setWidth( 2 );
		p.setPen( pen );
		p.setRenderHint( QPainter::Antialiasing );
		p.drawRoundedRect( QRect( 1, 1, width() - 2, height() - 2 ), 4, 4 );
	}

	QLabel::paintEvent( ev );
}

void SidebarLabel::updateFont() {
	const auto pFontTheme = H2Core::Preferences::get_instance()->getFontTheme();

	const QString sFontFamily = pFontTheme->m_sLevel2FontFamily;
	const auto fontSize = pFontTheme->m_fontSize;

	int nShrinkage = 7;
	switch ( fontSize ) {
	case H2Core::FontTheme::FontSize::Small:
		nShrinkage = 10;
		break;
	case H2Core::FontTheme::FontSize::Medium:
		nShrinkage = 7;
		break;
	case H2Core::FontTheme::FontSize::Large:
		nShrinkage = 2;
		break;
	default:
		ERRORLOG( QString( "Unknown font size [%1]" )
				  .arg( static_cast<int>(fontSize) ) );
		return;
	}

	const int nPixelSize = height() - nShrinkage;

	QFont font( sFontFamily );

	font.setPixelSize( nPixelSize );
	font.setBold( true );

	// This method must not be called more than once in this routine. Otherwise,
	// a repaint of the widget is triggered, which calls `updateFont()` again
	// and we are trapped in an infinite loop.
	setFont( font );

	const QString sEllipsis = QString::fromUtf8("\u2026");
	QString sText = m_sText;
	// Check whether the width of the text fits the available frame
	// width of the label
	while ( QFontMetrics( font ).size( Qt::TextSingleLine, sText ).width() >
			width() - m_nIndent && sText.size() > 3 ) {
		if ( sText.at( sText.size() - 2 ) != sEllipsis ) {
			// First trim action
			sText.replace( sText.size() - 2, 1, sEllipsis );
		}
		else {
			sText = sText.remove( sText.size() - 3, 1 );
		}
	}

	if ( sText != text() ) {
		QLabel::setText( sText );
	}
}

void SidebarLabel::setShowCursor( bool bShowCursor ) {
	if ( bShowCursor != m_bShowCursor ) {
		m_bShowCursor = bShowCursor;
	}
	update();
}

void SidebarLabel::setDimed( bool bDimed ) {
	if ( bDimed == m_bDimed ) {
		return;
	}

	m_bDimed = bDimed;

	updateStyle();
	update();
}

void SidebarLabel::updateStyle() {
	m_textColor = m_textBaseColor;
	if ( m_bDimed && m_type == Type::Type ) {
		m_textColor = m_textColor.darker( SidebarLabel::nDimScaling );
	}
	if ( m_bShowPlusSign && m_type == Type::Type ) {
		m_textColor.setAlpha( 200 );
	}

	setStyleSheet( QString( "\
QLabel {\
   color: %1;\
   font-weight: bold;\
 }" ).arg( m_textColor.name( QColor::HexArgb ) ) );
}

SidebarRow::SidebarRow( QWidget* pParent, const DrumPatternRow& row )
	: PixmapWidget(pParent)
	, m_row( row )
	, m_bIsSelected( false )
	, m_bEntered( false )
	, m_bDimed( false )
{
	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	const auto pPref = H2Core::Preferences::get_instance();
	auto pCommonStrings = HydrogenApp::get_instance()->getCommonStrings();

	const int nHeight = pPref->getPatternEditorGridHeight();
	resize( PatternEditorSidebar::m_nWidth, nHeight );

	auto pHBox = new QHBoxLayout();
	pHBox->setSpacing( 0 );
	pHBox->setContentsMargins( 0, 0, 0, 0 );

	QFont nameFont( pPref->getFontTheme()->m_sLevel2FontFamily,
					getPointSize( pPref->getFontTheme()->m_fontSize ) );

	m_pInstrumentNameLbl = new SidebarLabel(
		this, SidebarLabel::Type::Instrument,
		QSize( PatternEditorSidebar::m_nWidth - 2 * SidebarRow::m_nButtonWidth -
			   SidebarRow::m_nTypeLblWidth, nHeight ),
		"", PatternEditorSidebar::m_nMargin );
	m_pInstrumentNameLbl->setFont( nameFont );
	m_pInstrumentNameLbl->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	pHBox->addWidget( m_pInstrumentNameLbl );

	// Play back a sample of specific velocity based on the horizontal position
	// of the click event. We will do so just for the instrument label.
	connect(
		m_pInstrumentNameLbl, &SidebarLabel::labelClicked, [=]( QMouseEvent* pEvent ){
			auto pEv = static_cast<MouseEvent*>( pEvent );
			if ( pEvent->button() == Qt::LeftButton &&
				 ! m_pInstrumentNameLbl->isShowingPlusSign() ) {
				// Play a sound
				auto pSong = Hydrogen::get_instance()->getSong();
				if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
					return;
				}
				auto pInstr = pSong->getDrumkit()->getInstruments()->
					find( m_row.nInstrumentID );
				if ( m_pMuteBtn != nullptr &&
					 pInstr != nullptr && pInstr->hasSamples() &&
					 pPref->getHearNewNotes() ) {

					const int nWidth = m_pMuteBtn->x() - 5; // clickable field width
					const float fVelocity = std::min(
						(float)pEv->position().x()/(float)nWidth, VELOCITY_MAX );
					auto pNote = std::make_shared<Note>( pInstr, 0, fVelocity);
					Hydrogen::get_instance()->getAudioEngine()->getSampler()->
						noteOn( pNote );
				}
			}
			else if ( pEvent->button() == Qt::LeftButton &&
					  m_pInstrumentNameLbl->isShowingPlusSign() ) {
				SidebarRow::mousePressEvent( pEvent );
				// Add a new instrument to the current row
				bool bIsOkPressed;
				const QString sNewName = QInputDialog::getText(
					nullptr, "Hydrogen", pCommonStrings->getActionAddInstrument(),
					QLineEdit::Normal, "", &bIsOkPressed );
				if ( bIsOkPressed ) {
					auto pNewInstrument = std::make_shared<Instrument>();
					pNewInstrument->setType( m_row.sType );
					pNewInstrument->setName( sNewName );

					MainForm::action_drumkit_addInstrument( pNewInstrument );
				}
			}
	} );
	connect( m_pInstrumentNameLbl, &SidebarLabel::labelDoubleClicked,
			 [=]( QMouseEvent* pEvent ) {
				 if ( pEvent->button() == Qt::LeftButton &&
					  m_row.nInstrumentID != EMPTY_INSTR_ID ) {
					 MainForm::action_drumkit_renameInstrument(
						 m_pPatternEditorPanel->getRowIndexDB( m_row ) );
				 }
			 } );

	m_pSampleWarning = new Button(
		this, QSize( 15, 13 ), Button::Type::Icon, "warning.svg", "", QSize(),
		tr( "Some samples for this instrument failed to load." ), true );
	m_pSampleWarning->hide();
	pHBox->addWidget( m_pSampleWarning );
	connect(m_pSampleWarning, SIGNAL( clicked() ),
			this, SLOT( sampleWarningClicked() ));

	m_pMuteBtn = new Button(
		this, QSize( SidebarRow::m_nButtonWidth, height() ), Button::Type::Toggle,
		"", pCommonStrings->getSmallMuteButton(), QSize(), tr("Mute instrument"),
		true );
	m_pMuteBtn->setChecked( false );
	m_pMuteBtn->setObjectName( "SidebarRowMuteButton" );
	pHBox->addWidget( m_pMuteBtn );
	connect(m_pMuteBtn, SIGNAL( clicked() ), this, SLOT( muteClicked() ));

	m_pSoloBtn = new Button(
		this, QSize( SidebarRow::m_nButtonWidth, height() ), Button::Type::Toggle,
		"", pCommonStrings->getSmallSoloButton(), QSize(),
		pCommonStrings->getBigSoloButton(), true );
	m_pSoloBtn->setChecked( false );
	m_pSoloBtn->setObjectName( "SidebarRowSoloButton" );
	pHBox->addWidget( m_pSoloBtn );
	connect(m_pSoloBtn, SIGNAL( clicked() ), this, SLOT(soloClicked()));

	if ( row.nInstrumentID == EMPTY_INSTR_ID ) {
		m_pMuteBtn->hide();
		m_pSoloBtn->hide();
		m_pSampleWarning->hide();
	}

	pHBox->addStretch();

	m_pTypeLbl = new SidebarLabel(
		this, SidebarLabel::Type::Type,
		QSize( SidebarRow::m_nTypeLblWidth, nHeight ), m_row.sType, 3 );
	pHBox->addWidget( m_pTypeLbl );
	connect( m_pTypeLbl, &SidebarLabel::labelClicked, [=]( QMouseEvent* pEvent ){
		if ( pEvent->button() == Qt::LeftButton &&
			 m_pTypeLbl->isShowingPlusSign() ) {
			if ( m_row.bMappedToDrumkit ) {
				MainForm::editDrumkitProperties(
					false, false, m_row.nInstrumentID );
			}
			else {
				m_pPatternEditorPanel->setTypeInRow(
					m_pPatternEditorPanel->getRowIndexDB( m_row ) );
			}
		}
	} );
	connect( m_pTypeLbl, &SidebarLabel::labelDoubleClicked,
			 [=]( QMouseEvent* pEvent ){
				 if ( pEvent->button() == Qt::LeftButton ) {
					if ( m_row.bMappedToDrumkit ) {
						MainForm::editDrumkitProperties(
							false, false, m_row.nInstrumentID );
					}
					else {
						m_pPatternEditorPanel->setTypeInRow(
							m_pPatternEditorPanel->getRowIndexDB( m_row ) );
					}
				 }
			 } );
	m_pTypeLbl->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

	if ( ! pPref->getPatternEditorAlwaysShowTypeLabels() ) {
		m_pTypeLbl->hide();
	}

	// Popup menu
	m_pFunctionPopup = new QMenu( this );
	auto clearAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionClearAllNotesInRow() );
	connect( clearAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->clearNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			m_pPatternEditorPanel->getPatternNumber() ); } );

	m_pFunctionPopupSub = new QMenu(
		pCommonStrings->getActionFillNotes(), m_pFunctionPopup );
	auto fillAllAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillAllNotes() );
	connect( fillAllAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::All ); } );
	auto fillEverySecondAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEverySecondNote() );
	connect( fillEverySecondAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::EverySecond ); } );
	auto fillEveryThirdAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryThirdNote() );
	connect( fillEveryThirdAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::EveryThird ); } );
	auto fillEveryFourthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryFourthNote() );
	connect( fillEveryFourthAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::EveryFourth ); } );
	auto fillEverySixthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEverySixthNote() );
	connect( fillEverySixthAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::EverySixth ); } );
	auto fillEveryEighthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryEighthNote() );
	connect( fillEveryEighthAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::EveryEighth ); } );
	auto fillEveryTwelfthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEveryTwelfthNote() );
	connect( fillEveryTwelfthAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::EveryTwelfth ); } );
	auto fillEverySixteenthAction = m_pFunctionPopupSub->addAction(
		pCommonStrings->getActionFillEverySixteenthNote() );
	connect( fillEverySixteenthAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->fillNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ),
			PatternEditorPanel::FillNotes::EverySixteenth ); } );
	m_pFunctionPopup->addMenu( m_pFunctionPopupSub );

	auto selectNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionSelectNotes() );
	connect( selectNotesAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->getVisibleEditor()->selectAllNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ) ); } );

	m_pFunctionPopup->addSection( pCommonStrings->getActionEditAllPatterns() );
	auto cutNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionCutAllNotes() );
	connect( cutNotesAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->cutNotesFromRowOfAllPatterns(
			m_pPatternEditorPanel->getRowIndexDB( m_row ) ); } );
	auto copyNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionCopyNotes() );
	connect( copyNotesAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->copyNotesFromRowOfAllPatterns(
			m_pPatternEditorPanel->getRowIndexDB( m_row ) ); } );
	auto pasteNotesAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionPasteAllNotes() );
	connect( pasteNotesAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->pasteNotesToRowOfAllPatterns(
			m_pPatternEditorPanel->getRowIndexDB( m_row ) ); } );
	auto clearAllAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionClearAllNotes() );
	connect( clearAllAction, &QAction::triggered, this, [=](){
		m_pPatternEditorPanel->clearNotesInRow(
			m_pPatternEditorPanel->getRowIndexDB( m_row ), -1 ); } );

	m_pFunctionPopup->addSection( tr( "Instrument" ) );
	m_pFunctionPopup->addAction( pCommonStrings->getActionAddInstrument(),
								 HydrogenApp::get_instance()->getMainForm(),
								 SLOT( action_drumkit_addInstrument() ) );
	m_pRenameInstrumentAction = m_pFunctionPopup->addAction(
		pCommonStrings->getActionRenameInstrument() );
	connect( m_pRenameInstrumentAction, &QAction::triggered, this, [=](){
		MainForm::action_drumkit_renameInstrument(
			m_pPatternEditorPanel->getRowIndexDB( m_row ) );} );
	m_pDuplicateInstrumentAction =
		m_pFunctionPopup->addAction( pCommonStrings->getActionDuplicateInstrument() );
	connect( m_pDuplicateInstrumentAction, &QAction::triggered, this, [=](){
		MainForm::action_drumkit_duplicateInstrument(
			m_pPatternEditorPanel->getRowIndexDB( m_row ) );} );
	m_pDeleteInstrumentAction =
		m_pFunctionPopup->addAction( pCommonStrings->getActionDeleteInstrument() );
	connect( m_pDeleteInstrumentAction, &QAction::triggered, this, [=](){
		MainForm::action_drumkit_deleteInstrument(
			m_pPatternEditorPanel->getRowIndexDB( m_row ) );} );
	if ( m_row.nInstrumentID == EMPTY_INSTR_ID ) {
		m_pRenameInstrumentAction->setEnabled( false );
		m_pDeleteInstrumentAction->setEnabled( false );
	}

	m_pFunctionPopup->addSection( pCommonStrings->getSettings() );
	m_pTypeLabelVisibilityAction = m_pFunctionPopup->addAction(
		"Always show type labels" );
	m_pTypeLabelVisibilityAction->setCheckable( true );
	m_pTypeLabelVisibilityAction->setChecked( true );
	connect( m_pTypeLabelVisibilityAction, &QAction::triggered, this, [=](){
		Preferences::get_instance()->setPatternEditorAlwaysShowTypeLabels(
			m_pTypeLabelVisibilityAction->isChecked() );
		m_pPatternEditorPanel->updateTypeLabelVisibility();
	} );

	m_pFunctionPopup->setObjectName( "PatternEditorFunctionPopup" );

	updateColors();

	setLayout( pHBox );

	set( row );
}

void SidebarRow::set( const DrumPatternRow& row )
{
	auto pHydrogen = Hydrogen::get_instance();
	QString sToolTip;
	bool bIsSoloed = false, bIsMuted = false;
	m_row = row;

	std::shared_ptr<Instrument> pInstrument = nullptr;
	if ( row.nInstrumentID != EMPTY_INSTR_ID && row.bMappedToDrumkit ) {
		auto pSong = pHydrogen->getSong();
		if ( pSong != nullptr && pSong->getDrumkit() != nullptr ) {
			pInstrument =
				pSong->getDrumkit()->getInstruments()->find( row.nInstrumentID );
			if ( pInstrument != nullptr ) {
				const QString sInstrumentName = pInstrument->getName();
				m_pInstrumentNameLbl->setText( sInstrumentName );
				m_pInstrumentNameLbl->setShowPlusSign( false );
				m_pInstrumentNameLbl->setDimed( false );

				setMuted( pInstrument->isMuted() );
				setSoloed( pInstrument->isSoloed() );
				setSamplesMissing( pInstrument->hasMissingSamples() );

				m_pMuteBtn->show();
				m_pSoloBtn->show();
				m_pRenameInstrumentAction->setEnabled( true );
				m_pDuplicateInstrumentAction->setEnabled( true );
				m_pDeleteInstrumentAction->setEnabled( true );

				if ( ! pInstrument->getDrumkitPath().isEmpty() ) {
					// Instrument belongs to a kit in the SoundLibrary (and was
					// not created anew).
					QString sKit = pHydrogen->getSoundLibraryDatabase()->
						getUniqueLabel( pInstrument->getDrumkitPath() );
					if ( sKit.isEmpty() ) {
						// This should not happen. But drumkit.xml files can be
						// created by hand and we should account for it.
						sKit = pInstrument->getDrumkitPath();
					}

					/*: Shown in a tooltop and indicating the drumkit (to the right of this string) an instrument (to the left of this string) is loaded from. */
					sToolTip = QString( "%1 (" ).arg( sInstrumentName )
						.append( tr( "imported from" ) )
						.append( QString( " [%1])" ).arg( sKit ) );
				}
			}
		}
	}

	if ( pInstrument == nullptr ) {
		m_pInstrumentNameLbl->setText( "" );
		m_pInstrumentNameLbl->setShowPlusSign( true );
		m_pInstrumentNameLbl->setDimed( true );
		m_pMuteBtn->hide();
		m_pSoloBtn->hide();
		m_pSampleWarning->hide();
		m_pRenameInstrumentAction->setEnabled( false );
		m_pDuplicateInstrumentAction->setEnabled( false );
		m_pDeleteInstrumentAction->setEnabled( false );
	}

	setSelected( m_pPatternEditorPanel->getSelectedRowDB() ==
				 m_pPatternEditorPanel->getRowIndexDB( row ) );

	if ( ! row.sType.isEmpty() && m_pTypeLbl->text() != row.sType ) {
		m_pTypeLbl->setText( row.sType );
		m_pTypeLbl->setToolTip( row.sType );
		m_pTypeLbl->setShowPlusSign( false );
	}
	else if ( row.sType.isEmpty() && row.nInstrumentID != EMPTY_INSTR_ID ) {
		m_pTypeLbl->setShowPlusSign( true );
		m_pTypeLbl->setText( QString::number( row.nInstrumentID ) );
	}

	if ( m_pInstrumentNameLbl->toolTip() != sToolTip ){
		m_pInstrumentNameLbl->setToolTip( sToolTip );
	}

	updateStyleSheet();
	update();
}

void SidebarRow::setSelected( bool bSelected )
{
	if ( bSelected == m_bIsSelected ) {
		return;
	}

	m_bIsSelected = bSelected;

	if ( m_pTypeLbl->isVisible() ) {
		m_pTypeLbl->setShowCursor( bSelected );
		m_pInstrumentNameLbl->setShowCursor( false );
	}
	else {
		m_pInstrumentNameLbl->setShowCursor( bSelected );
		m_pTypeLbl->setShowCursor( false );
	}

	updateStyleSheet();

	m_pTypeLbl->setDimed( m_bDimed && ! bSelected );
}

void SidebarRow::setDimed( bool bDimed ) {
	if ( bDimed == m_bDimed ) {
		return;
	}

	m_bDimed = bDimed;

	m_pTypeLbl->setDimed( bDimed && ! m_bIsSelected );
}

void SidebarRow::updateStyleSheet() {
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	QColor textColor, textPatternColor, backgroundPatternColor, backgroundColor;
	if ( m_bIsSelected ) {
		backgroundPatternColor =
			pColorTheme->m_patternEditor_selectedRowColor.darker(
				Skin::nListBackgroundColorScaling );
		backgroundColor =
			pColorTheme->m_patternEditor_instrumentSelectedRowColor;
		textPatternColor = pColorTheme->m_patternEditor_selectedRowTextColor;
		textColor = pColorTheme->m_patternEditor_instrumentSelectedRowTextColor;
	}
	else if ( m_row.bAlternate ) {
		backgroundPatternColor =
			pColorTheme->m_patternEditor_alternateRowColor.darker(
				Skin::nListBackgroundColorScaling );
		backgroundColor =
			pColorTheme->m_patternEditor_instrumentAlternateRowColor;
		textPatternColor = pColorTheme->m_patternEditor_textColor;
		textColor = pColorTheme->m_patternEditor_instrumentRowTextColor;
	}
	else {
		backgroundPatternColor =
			pColorTheme->m_patternEditor_backgroundColor.darker(
				Skin::nListBackgroundColorScaling );
		backgroundColor =
			pColorTheme->m_patternEditor_instrumentRowColor;
		textPatternColor = pColorTheme->m_patternEditor_textColor;
		textColor = pColorTheme->m_patternEditor_instrumentRowTextColor;
	}

	// Indicate chosen editor mode.
	QColor backgroundInactiveColor;
	if ( Hydrogen::get_instance()->getMode() == Song::Mode::Pattern ) {
		backgroundInactiveColor = pColorTheme->m_windowColor.lighter(
			Skin::nEditorActiveScaling );
	}
	else {
		backgroundInactiveColor = pColorTheme->m_windowColor;
	}

	setColor( backgroundInactiveColor );

	m_pInstrumentNameLbl->setColor(
		backgroundColor, textColor, pColorTheme->m_cursorColor );
	m_pTypeLbl->setColor(
		backgroundPatternColor, textPatternColor, pColorTheme->m_cursorColor );
}

void SidebarRow::updateTypeLabelVisibility( bool bVisible ) {
	if ( bVisible ) {
		m_pTypeLbl->show();
	} else {
		m_pTypeLbl->hide();
	}

	// Update label on which the cursor is shown
	if ( m_bIsSelected ) {
		if ( bVisible ) {
			m_pInstrumentNameLbl->setShowCursor( false );
			m_pTypeLbl->setShowCursor( true );
		}
		else {
			m_pInstrumentNameLbl->setShowCursor( true );
			m_pTypeLbl->setShowCursor( false );
		}
	}
}

#ifdef H2CORE_HAVE_QT6
void SidebarRow::enterEvent( QEnterEvent *ev ) {
#else
void SidebarRow::enterEvent( QEvent *ev ) {
#endif
	UNUSED( ev );
	m_bEntered = true;
	update();
}

void SidebarRow::leaveEvent( QEvent* ev ) {
	UNUSED( ev );
	m_bEntered = false;
	update();
}

void SidebarRow::setMuted(bool isMuted)
{
	if ( ! m_pMuteBtn->isDown() &&
		 m_pMuteBtn->isChecked() != isMuted ) {
		m_pMuteBtn->setChecked(isMuted);
	}
}


void SidebarRow::setSoloed( bool soloed )
{
	if ( ! m_pSoloBtn->isDown() &&
		 m_pSoloBtn->isChecked() != soloed ) {
		m_pSoloBtn->setChecked( soloed );
	}
}


void SidebarRow::setSamplesMissing( bool bSamplesMissing )
{
	if ( m_pSampleWarning != nullptr ) {
		if ( bSamplesMissing ) {
			m_pSampleWarning->show();
		} else {
			m_pSampleWarning->hide();
		}
	}
}

void SidebarRow::muteClicked()
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}


	const int nRow = m_pPatternEditorPanel->getRowIndexDB( m_row );
	m_pPatternEditorPanel->setSelectedRowDB( nRow );

	if ( m_row.nInstrumentID != EMPTY_INSTR_ID ) {
		auto pInstr =
			pSong->getDrumkit()->getInstruments()->find( m_row.nInstrumentID );
		if ( pInstr == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve instrument of ID [%1]" )
					  .arg( m_row.nInstrumentID ) );
			return;
		}

		H2Core::CoreActionController::setStripIsMuted(
			nRow, ! pInstr->isMuted(), false );
	}
}

void SidebarRow::soloClicked()
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	const int nRow = m_pPatternEditorPanel->getRowIndexDB( m_row );
	m_pPatternEditorPanel->setSelectedRowDB( nRow );

	if ( m_row.nInstrumentID != EMPTY_INSTR_ID ) {
		auto pInstr =
			pSong->getDrumkit()->getInstruments()->find( m_row.nInstrumentID );
		if ( pInstr == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve instrument of ID [%1]" )
					  .arg( m_row.nInstrumentID ) );
			return;
		}

		H2Core::CoreActionController::setStripIsSoloed(
			nRow, ! pInstr->isSoloed(), false );
	}
}

void SidebarRow::sampleWarningClicked()
{
	QMessageBox::information( this, "Hydrogen",
							  tr( "One or more samples for this instrument failed to load. This may be because the"
								  " songfile uses an older default drumkit. This might be fixed by opening a new "
								  "drumkit." ) );
}

void SidebarRow::mousePressEvent(QMouseEvent *ev)
{
	auto pEv = static_cast<MouseEvent*>( ev );

	const auto pPref = Preferences::get_instance();

	m_pPatternEditorPanel->setSelectedRowDB(
		m_pPatternEditorPanel->getRowIndexDB( m_row ) );

	if ( ev->button() == Qt::RightButton ) {
		if ( m_pTypeLabelVisibilityAction->isChecked() !=
			 pPref->getPatternEditorAlwaysShowTypeLabels() ) {
			m_pTypeLabelVisibilityAction->setChecked(
				pPref->getPatternEditorAlwaysShowTypeLabels() );
		}

		m_pFunctionPopup->popup( pEv->globalPosition().toPoint() );
	}

	// Hide cursor in case this behavior was selected in the
	// Preferences.
	m_pPatternEditorPanel->getVisibleEditor()->handleKeyboardCursor( false );

	// propago l'evento al parent: serve per il drag&drop
	PixmapWidget::mousePressEvent(ev);
}

void SidebarRow::updateColors() {
	const auto pColorTheme = Preferences::get_instance()->getColorTheme();

	m_pMuteBtn->setCheckedBackgroundColor( pColorTheme->m_muteColor );
	m_pMuteBtn->setCheckedBackgroundTextColor( pColorTheme->m_muteTextColor );
	m_pSoloBtn->setCheckedBackgroundColor( pColorTheme->m_soloColor );
	m_pSoloBtn->setCheckedBackgroundTextColor( pColorTheme->m_soloTextColor );
}

void SidebarRow::updateFont() {
	m_pInstrumentNameLbl->updateFont();
	m_pTypeLbl->updateFont();
}

void SidebarRow::update() {
	PixmapWidget::update();

	m_pInstrumentNameLbl->update();
	m_pTypeLbl->update();
}


//////

PatternEditorSidebar::PatternEditorSidebar( QWidget *parent )
	: QWidget( parent )
	, m_nDragStartY( -1 )
 {

	HydrogenApp::get_instance()->addEventListener( this );
	const auto pPref = H2Core::Preferences::get_instance();

	m_pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();

	auto pVBoxLayout = new QVBoxLayout( this );
	pVBoxLayout->setSpacing( 0 );
	pVBoxLayout->setContentsMargins( 0, 0, 0, 0 );

	setLayout( pVBoxLayout );

	m_nEditorHeight = pPref->getPatternEditorGridHeight() *
		m_pPatternEditorPanel->getRowNumberDB();

	if ( pPref->getPatternEditorAlwaysShowTypeLabels() ) {
		resize( PatternEditorSidebar::m_nWidth, m_nEditorHeight );
	}
	else {
		resize( PatternEditorSidebar::m_nWidth -
				SidebarRow::m_nTypeLblWidth, m_nEditorHeight );
	}

	setAcceptDrops(true);

	updateRows();

	QScrollArea *pScrollArea = dynamic_cast< QScrollArea *>( parentWidget()->parentWidget() );
	assert( pScrollArea );
	m_pDragScroller = new DragScroller( pScrollArea );
}



PatternEditorSidebar::~PatternEditorSidebar()
{
	//INFOLOG( "DESTROY" );
	delete m_pDragScroller;
}

void PatternEditorSidebar::updateColors() {
	for ( auto& rrow : m_rows ) {
		rrow->updateColors();
	}
}

void PatternEditorSidebar::updateEditor() {
	updateRows();

	for ( auto& rrow : m_rows ) {
		rrow->update();
	}

	update();
}

void PatternEditorSidebar::updateFont() {
	for ( auto& rrow : m_rows ) {
		rrow->updateFont();
	}
}

void PatternEditorSidebar::updateStyleSheet() {
	for ( auto& rrow : m_rows ) {
		rrow->updateStyleSheet();
	}
}

void PatternEditorSidebar::updateTypeLabelVisibility( bool bVisible ) {
	for ( auto& rrow : m_rows ) {
		rrow->updateTypeLabelVisibility( bVisible );
	}
}

void PatternEditorSidebar::dimRows( bool bDim ) {
	for ( auto& rrow : m_rows ) {
		rrow->setDimed( bDim );
	}
}

///
/// Update every SidebarRow, create or destroy lines if necessary.
///
void PatternEditorSidebar::updateRows()
{
	const auto pPref = H2Core::Preferences::get_instance();
	if ( m_nEditorHeight != pPref->getPatternEditorGridHeight() *
		 m_pPatternEditorPanel->getRowNumberDB() ) {
		m_nEditorHeight = pPref->getPatternEditorGridHeight() *
			m_pPatternEditorPanel->getRowNumberDB();
		resize( width(), m_nEditorHeight );
	}

	bool bPianoRollShown = false;
	if ( dynamic_cast<PianoRollEditor*>(
			 m_pPatternEditorPanel->getVisibleEditor()) != nullptr ) {
		bPianoRollShown = true;
	}

	int nnIndex = 0;
	for ( const auto& rrow : m_pPatternEditorPanel->getDB() ) {
		if ( nnIndex < m_rows.size() ) {
			// row already exists do a lazy update instead of recreating it.
			m_rows[ nnIndex ]->set( rrow );
			m_rows[ nnIndex ]->setDimed( bPianoRollShown );
		}
		else {
			// row in DB does not has its counterpart in the sidebar yet. Create
			// it.
			auto pRow = new SidebarRow( this, rrow );
			layout()->addWidget( pRow );
			pRow->setDimed( bPianoRollShown );
			m_rows.push_back( pRow );
		}
		++nnIndex;
	}

	const int nRows = m_pPatternEditorPanel->getRowNumberDB();
	while ( nRows < m_rows.size() && m_rows.size() > 0 ) {
		// There are rows not required anymore
		auto pRow = *( m_rows.end() - 1 );
		layout()->removeWidget( pRow );
		m_rows.pop_back();
		delete pRow;
	}
}

void PatternEditorSidebar::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
}

void PatternEditorSidebar::dropEvent(QDropEvent *event)
{
	if ( ! event->mimeData()->hasFormat("text/plain") ) {
		event->ignore();
		return;
	}

	auto pEv = static_cast<DropEvent*>( event );

	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	auto pInstrumentList = pSong->getDrumkit()->getInstruments();
	const auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogenApp = HydrogenApp::get_instance();
	const auto pCommonStrings = pHydrogenApp->getCommonStrings();

	QString sText = event->mimeData()->text();

	if ( sText.startsWith("Songs:") ||
		 sText.startsWith("Patterns:") ||
		 sText.startsWith("move pattern:") ||
		 sText.startsWith("drag pattern:") ) {
		return;
	}

	// Starting point for instument list is 50 lower than on the drum pattern
	// editor
	int nPosY;
	if ( pEv->position().x() >= PatternEditorSidebar::m_nWidth ) {
		nPosY = pEv->position().y() - 50;
	}
	else {
		nPosY = pEv->position().y();
	}

	int nTargetRow = nPosY / pPref->getPatternEditorGridHeight();

	// There might be rows in the pattern editor not corresponding to the
	// current kit. Since we only support rearranging rows corresponding to
	// valid instruments we will move the dragged one to the end of the
	// instrument list in case it was dragged beyond it.
	if ( nTargetRow >= pInstrumentList->size() ) {
		nTargetRow = pInstrumentList->size() - 1;
	}

	if ( sText.startsWith( "move instrument:" ) ) {

		sText.remove( 0, QString( "move instrument:" ).length() );

		bool bOk = false;
		const int nSourceRow = sText.toInt( &bOk, 10 );

		if ( nSourceRow == nTargetRow ) {
			event->acceptProposedAction();
			return;
		}

		pHydrogenApp->pushUndoCommand(
			new SE_moveInstrumentAction( nSourceRow, nTargetRow ) );
		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2] -> [%3]" )
			.arg( pCommonStrings->getActionMoveInstrument() )
			.arg( nSourceRow ).arg( nTargetRow ) );

		event->acceptProposedAction();
	}
	else if ( sText.startsWith( "importInstrument:" ) ) {
		// an instrument was dragged from the soundlibrary browser to the
		// pattern editor
		sText.remove( 0, QString( "importInstrument:" ).length() );

		QStringList tokens = sText.split( "::" );
		const QString sDrumkitPath = tokens.at( 0 );
		const QString sInstrumentName = tokens.at( 1 );

		// Load Instrument
		const auto pNewDrumkit =
			pHydrogen->getSoundLibraryDatabase()->getDrumkit( sDrumkitPath );
		if ( pNewDrumkit == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve kit [%1] for instrument [%2]" )
					  .arg( sDrumkitPath ).arg( sInstrumentName ) );
			QMessageBox::critical( this, "Hydrogen",
								   pCommonStrings->getInstrumentLoadError() );
			return;
		}
		const auto pTargetInstrument =
			pNewDrumkit->getInstruments()->find( sInstrumentName );
		if ( pTargetInstrument == nullptr ) {
			ERRORLOG( QString( "Unable to retrieve instrument [%1] from kit [%2]" )
					  .arg( sInstrumentName ).arg( sDrumkitPath ) );
			QMessageBox::critical( this, "Hydrogen",
								   pCommonStrings->getInstrumentLoadError() );
			return;
		}

		// Appending in this action is done by setting the target row to -1.
		int nTargetRowSE = nTargetRow;
		if ( nTargetRow == pInstrumentList->size() - 1 ) {
			nTargetRowSE = -1;
			// Select the row after the current "end" of the drumkit.
			++nTargetRow;
		}
		// We provide a copy of the instrument in order to not leak any changes
		// into the original kit.
		pHydrogenApp->pushUndoCommand(
			new SE_addInstrumentAction(
				std::make_shared<Instrument>(pTargetInstrument), nTargetRowSE,
				SE_addInstrumentAction::Type::DropInstrument ) );
		pHydrogenApp->showStatusBarMessage(
			QString( "%1 [%2]" ) .arg( pCommonStrings->getActionDropInstrument() )
			.arg( pTargetInstrument->getName() ) );

		event->acceptProposedAction();
	}
	else {
		// Unknown drop action
		return;
	}

	m_pPatternEditorPanel->setSelectedRowDB( nTargetRow );
}


void PatternEditorSidebar::mousePressEvent( QMouseEvent *event ) {
	if ( event->button() != Qt::LeftButton ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( event );

	if ( m_pPatternEditorPanel->getRowDB(
			 m_pPatternEditorPanel->getSelectedRowDB() ).nInstrumentID !=
		 EMPTY_INSTR_ID ) {
		// Drag started at a line corresponding to an instrument of the current
		// drumkit.
		m_nDragStartY = pEv->position().y();
	}
	else {
		m_nDragStartY = -1;
	}
}

void PatternEditorSidebar::mouseMoveEvent(QMouseEvent *event)
{
	// Button needs to stay pressed.
	if ( ! ( event->buttons() & Qt::LeftButton ) ) {
		return;
	}

	auto pEv = static_cast<MouseEvent*>( event );

	// No valid drag. Maybe it was started using a instrument type only row.
	if ( m_nDragStartY == -1 ) {
		return;
	}

	const auto pPref = H2Core::Preferences::get_instance();
	if ( abs( pEv->position().y() - m_nDragStartY ) <
		 pPref->getPatternEditorGridHeight() ) {
		// Still within the same row.
		return;
	}

	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr || pSong->getDrumkit() == nullptr ) {
		return;
	}

	// Instrument corresponding to the selected line in the pattern editor.
	const int nSelectedRow = m_pPatternEditorPanel->getSelectedRowDB();
	auto pInstrument = pSong->getDrumkit()->getInstruments()->find(
		m_pPatternEditorPanel->getRowDB( nSelectedRow ).nInstrumentID );
	if ( pInstrument == nullptr ) {
		ERRORLOG( QString( "No instrument selected found for row [%1]" )
				  .arg( nSelectedRow ) );
		return;
	}

	const QString sText = QString( "move instrument:%1" ).arg( nSelectedRow );

	QDrag *pDrag = new QDrag(this);
	QMimeData *pMimeData = new QMimeData;

	pMimeData->setText( sText );
	pDrag->setMimeData( pMimeData );

	m_pDragScroller->startDrag();
	pDrag->exec( Qt::CopyAction | Qt::MoveAction );
	m_pDragScroller->endDrag();

	QWidget::mouseMoveEvent(event);
}


void PatternEditorSidebar::instrumentMuteSoloChangedEvent( int nInstrumentIndex ) {

	if ( nInstrumentIndex == -1 ) {
		updateRows();
	}
	else {
		// Update a specific line
		const auto row = m_pPatternEditorPanel->getRowDB( nInstrumentIndex );
		if ( row.nInstrumentID == EMPTY_INSTR_ID && row.sType.isEmpty() ) {
			ERRORLOG( QString( "Invalid row [%1]" ).arg( nInstrumentIndex ) );
			return;
		}

		if ( nInstrumentIndex >= m_rows.size() ) {
			// This should not happen
			updateRows();
		}
		else {
			m_rows[ nInstrumentIndex ]->set( row );
		}
	}
}
