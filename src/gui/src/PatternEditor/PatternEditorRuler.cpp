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

#include <core/Hydrogen.h>
#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>


using namespace H2Core;

#include <QTimer>
#include <QPainter>

#include "DrumPatternEditor.h"
#include "PatternEditorRuler.h"
#include "PatternEditorPanel.h"
#include "NotePropertiesRuler.h"
#include "../Compatibility/MouseEvent.h"
#include "../HydrogenApp.h"
#include "../Skin.h"


PatternEditorRuler::PatternEditorRuler( QWidget* parent )
	: QWidget( parent )
	, m_nHoveredColumn( -1 )
	, m_nTick( -1 )
 {
	setAttribute(Qt::WA_OpaquePaintEvent);
	setMouseTracking( true );

	//infoLog( "INIT" );

	Preferences *pPref = Preferences::get_instance();

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_backgroundColor );

	m_pPattern = nullptr;
	m_fGridWidth = Preferences::get_instance()->getPatternEditorGridWidth();

	m_nRulerWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );
	m_nRulerHeight = 25;

	resize( m_nRulerWidth, m_nRulerHeight );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( m_nRulerWidth * pixelRatio,
									   m_nRulerHeight * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );

	m_pTimer = new QTimer(this);
	connect(m_pTimer, &QTimer::timeout, [=]() {
		if ( H2Core::Hydrogen::get_instance()->getAudioEngine()->getState() ==
			 H2Core::AudioEngine::State::Playing ) {
			updatePosition();
		}
	});

	// Will set the active width and calls createBackground.
	updateActiveRange();
	invalidateBackground();
	update();

	HydrogenApp::get_instance()->addEventListener( this );
}



PatternEditorRuler::~PatternEditorRuler() {
	//infoLog( "DESTROY");
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
}

void PatternEditorRuler::updatePosition( bool bForce ) {
	
	auto pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();
	
	bool bIsSelectedPatternPlaying = false;	// is the pattern playing now?

	if ( pHydrogen->getMode() == Song::Mode::Song &&
		 pHydrogen->isPatternEditorLocked() ) {
		// In case the pattern editor is locked we will always display
		// the position tick. Even if no pattern is set at all.
		bIsSelectedPatternPlaying = true;
	} else {
		/* 
		 * Lock audio engine to make sure pattern list does not get
		 * modified / cleared during iteration 
		 */
		pAudioEngine->lock( RIGHT_HERE );

		auto pList = pAudioEngine->getPlayingPatterns();
		for (uint i = 0; i < pList->size(); i++) {
			if ( m_pPattern == pList->get(i) ) {
				bIsSelectedPatternPlaying = true;
				break;
			}
		}

		pAudioEngine->unlock();
	}

	int nTick = pAudioEngine->getTransportPosition()->getPatternTickPosition();

	if ( nTick != m_nTick || bForce ) {
		int nDiff = m_fGridWidth * (nTick - m_nTick);
		m_nTick = nTick;
		int nX = static_cast<int>( static_cast<float>(PatternEditor::nMargin) + 1 +
								   m_nTick * static_cast<float>(m_fGridWidth) -
								   static_cast<float>(Skin::nPlayheadWidth) / 2 );
		QRect updateRect( nX -2, 0, 4 + Skin::nPlayheadWidth, height() );
		update( updateRect );
		if ( nDiff > 1 || nDiff < -1 ) {
			// New cursor is far enough away from the old one that the single update rect won't cover both. So
			// update at the old location as well.
			updateRect.translate( -nDiff, 0 );
			update( updateRect );
		}

		if ( ! bIsSelectedPatternPlaying ) {
			nTick = -1;
		}

		auto pPatternEditorPanel = HydrogenApp::get_instance()->getPatternEditorPanel();
		if ( pPatternEditorPanel != nullptr ) {
			pPatternEditorPanel->getDrumPatternEditor()->updatePosition( nTick );
			pPatternEditorPanel->getPianoRollEditor()->updatePosition( nTick );
			pPatternEditorPanel->getVelocityEditor()->updatePosition( nTick );
			pPatternEditorPanel->getPanEditor()->updatePosition( nTick );
			pPatternEditorPanel->getLeadLagEditor()->updatePosition( nTick );
			pPatternEditorPanel->getNoteKeyEditor()->updatePosition( nTick );
			pPatternEditorPanel->getProbabilityEditor()->updatePosition( nTick );
		}
	}
}

void PatternEditorRuler::relocationEvent() {
	updatePosition();
}

void PatternEditorRuler::updateStart(bool start) {
	if (start) {
		m_pTimer->start(50);	// update ruler at 20 fps
	}
	else {
		m_pTimer->stop();
	}
}



void PatternEditorRuler::showEvent( QShowEvent *ev )
{
	UNUSED( ev );
	updatePosition();
	updateStart(true);
}


void PatternEditorRuler::leaveEvent( QEvent* ev ){
	m_nHoveredColumn = -1;
	update();

	QWidget::leaveEvent( ev );
}

void PatternEditorRuler::hideEvent ( QHideEvent *ev )
{
	UNUSED( ev );
	updateStart(false);
}

void PatternEditorRuler::mousePressEvent( QMouseEvent* ev ) {

	auto pEv = static_cast<MouseEvent*>( ev );

	if ( ev->button() == Qt::LeftButton &&
		 pEv->position().x() < m_nWidthActive ) {
		auto pHydrogen = Hydrogen::get_instance();
		auto pCoreActionController = pHydrogen->getCoreActionController();
		auto pHydrogenApp = HydrogenApp::get_instance();
		DrumPatternEditor* pDrumPatternEditor;
		if ( pHydrogenApp->getPatternEditorPanel() != nullptr ) {
			pDrumPatternEditor = pHydrogenApp->getPatternEditorPanel()->getDrumPatternEditor();
		} else {
			pDrumPatternEditor = nullptr;
		}

		// Fall back to default values in case the GUI is starting and the
		// pattern editor is not ready yet.
		float fResolution;
		bool bIsUsingTriplets;
		if ( pDrumPatternEditor != nullptr ) {
			fResolution = static_cast<float>(pDrumPatternEditor->getResolution());
			bIsUsingTriplets = pDrumPatternEditor->isUsingTriplets();
		} else {
			fResolution = 8;
			bIsUsingTriplets = false;
		}

		float fTripletFactor;
		if ( bIsUsingTriplets ) {
			fTripletFactor = 3;
		} else {
			fTripletFactor = 4;
		}

		long nNewTick = std::floor( static_cast<float>(m_nHoveredColumn) *
									4 * static_cast<float>(MAX_NOTES) /
									( fTripletFactor * fResolution ) );

		if ( pHydrogen->getMode() != Song::Mode::Pattern ) {
			pCoreActionController->activateSongMode( false );
			pHydrogen->setIsModified( true );
		}

		pCoreActionController->locateToTick( nNewTick );
	}
}

void PatternEditorRuler::mouseMoveEvent( QMouseEvent* ev ) {

	auto pEv = static_cast<MouseEvent*>( ev );
	if ( pEv->position().x() < m_nWidthActive ) {
	
		auto pHydrogenApp = HydrogenApp::get_instance();
		DrumPatternEditor* pDrumPatternEditor;
		if ( pHydrogenApp->getPatternEditorPanel() != nullptr ) {
			pDrumPatternEditor = pHydrogenApp->getPatternEditorPanel()->getDrumPatternEditor();
		} else {
			pDrumPatternEditor = nullptr;
		}

		// Fall back to default values in case the GUI is starting and the
		// pattern editor is not ready yet.
		float fResolution;
		bool bIsUsingTriplets;
		if ( pDrumPatternEditor != nullptr ) {
			fResolution = static_cast<float>(pDrumPatternEditor->getResolution());
			bIsUsingTriplets = pDrumPatternEditor->isUsingTriplets();
		} else {
			fResolution = 8;
			bIsUsingTriplets = false;
		}

		float fTripletFactor;
		if ( bIsUsingTriplets ) {
			fTripletFactor = 3;
		} else {
			fTripletFactor = 4;
		}

		float fColumnWidth = fTripletFactor * fResolution /
			( 4 * static_cast<float>(MAX_NOTES) * m_fGridWidth );

		int nHoveredColumn =
			static_cast<int>(std::floor( static_cast<float>(
				std::max( static_cast<int>(pEv->position().x()) -
						  PatternEditor::nMargin +
						  static_cast<int>(std::round(1 / fColumnWidth / 2) ), 0 )) *
										 fColumnWidth ));
		
		if ( nHoveredColumn != m_nHoveredColumn ) {
			m_nHoveredColumn = nHoveredColumn;
			update();
		}
	}
}

void PatternEditorRuler::updateEditor( bool bRedrawAll )
{
	Hydrogen *pHydrogen = Hydrogen::get_instance();
	auto pAudioEngine = pHydrogen->getAudioEngine();

	//Do not redraw anything if Export is active.
	//https://github.com/hydrogen-music/hydrogen/issues/857	
	if( pHydrogen->getIsExportSessionActive() ) {
		return;
	}
	
	PatternList *pPatternList = pHydrogen->getSong()->getPatternList();
	int nSelectedPatternNumber = pHydrogen->getSelectedPatternNumber();
	if ( (nSelectedPatternNumber != -1) && ( (uint)nSelectedPatternNumber < pPatternList->size() )  ) {
		m_pPattern = pPatternList->get( nSelectedPatternNumber );
	}
	else {
		m_pPattern = nullptr;
	}

	const bool bActiveRangeUpdated = updateActiveRange();

	updatePosition();
	
	if ( bRedrawAll || bActiveRangeUpdated ) {
		invalidateBackground();
		update( 0, 0, width(), height() );
	}
}


void PatternEditorRuler::invalidateBackground()
{
	m_bBackgroundInvalid = true;
}

void PatternEditorRuler::createBackground()
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	DrumPatternEditor* pDrumPatternEditor;
	if ( pHydrogenApp->getPatternEditorPanel() != nullptr ) {
		pDrumPatternEditor = pHydrogenApp->getPatternEditorPanel()->getDrumPatternEditor();
	} else {
		pDrumPatternEditor = nullptr;
	}
	auto pPref = H2Core::Preferences::get_instance();

	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->width() != m_nRulerWidth ||
		 m_pBackgroundPixmap->height() != m_nRulerHeight ||
		 m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width()  * pixelRatio , height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	QColor backgroundColor( pPref->getColorTheme()->m_patternEditor_alternateRowColor.darker( 120 ) );
	QColor textColor = pPref->getColorTheme()->m_patternEditor_textColor;
	textColor.setAlpha( 220 );
	
	QColor lineColor = pPref->getColorTheme()->m_patternEditor_lineColor;

	QPainter painter( m_pBackgroundPixmap );
	
	painter.fillRect( QRect( 0, 0, width(), height() ), backgroundColor );

	// gray background for unusable section of pattern
	if ( m_nRulerWidth - m_nWidthActive != 0 ) {
		painter.fillRect( m_nWidthActive, 0, m_nRulerWidth - m_nWidthActive,
						  m_nRulerHeight,
						  pPref->getColorTheme()->m_midLightColor );
	}

	// numbers

	QFont font( pPref->getApplicationFontFamily(), getPointSize( pPref->getFontSize() ) );
	painter.setFont(font);

	uint nQuarter = 48;

	// Fall back to default values in case the GUI is starting and the
	// pattern editor is not ready yet.
	int nResolution;
	bool bIsUsingTriplets;
	if ( pDrumPatternEditor != nullptr ) {
		nResolution = pDrumPatternEditor->getResolution();
		bIsUsingTriplets = pDrumPatternEditor->isUsingTriplets();
	} else {
		nResolution = 8;
		bIsUsingTriplets = false;
	}

	// Draw numbers and quarter ticks
	painter.setPen( textColor );
	for ( int ii = 0; ii < 64 ; ii += 4 ) {
		int nText_x = PatternEditor::nMargin + nQuarter / 4 * ii * m_fGridWidth;
		painter.drawLine( nText_x, height() - 13, nText_x, height() - 1 );
		painter.drawText( nText_x + 3, 0, 60, m_nRulerHeight,
						  Qt::AlignVCenter | Qt::AlignLeft,
						  QString("%1").arg(ii / 4 + 1) );
	}

	// Draw remaining ticks
	float fStep;
	if ( bIsUsingTriplets ) {
		fStep = 4 * MAX_NOTES / ( 3 * nResolution ) * m_fGridWidth;
	} else {
		fStep = 4 * MAX_NOTES / ( 4 * nResolution ) * m_fGridWidth;
	}
	for ( float xx = PatternEditor::nMargin; xx < m_nWidthActive; xx += fStep ) {
		painter.drawLine( xx, height() - 6, xx, height() - 1 );
	}

	painter.setPen( QPen( lineColor, 2, Qt::SolidLine ) );
	painter.drawLine( 0, m_nRulerHeight, m_nRulerWidth, m_nRulerHeight);
	painter.drawLine( m_nRulerWidth, 0, m_nRulerWidth, m_nRulerHeight );

	m_bBackgroundInvalid = false;
}


void PatternEditorRuler::paintEvent( QPaintEvent *ev)
{
	auto pPref = H2Core::Preferences::get_instance();
	auto pHydrogenApp = HydrogenApp::get_instance();
	DrumPatternEditor* pDrumPatternEditor;
	if ( pHydrogenApp->getPatternEditorPanel() != nullptr ) {
		pDrumPatternEditor = pHydrogenApp->getPatternEditorPanel()->getDrumPatternEditor();
	} else {
		pDrumPatternEditor = nullptr;
	}

	if (!isVisible()) {
		return;
	}

	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() || m_bBackgroundInvalid ) {
		createBackground();
	}

	QPainter painter(this);

	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, QRectF( pixelRatio * ev->rect().x(),
															pixelRatio * ev->rect().y(),
															pixelRatio * ev->rect().width(),
															pixelRatio * ev->rect().height() ) );

	// draw cursor
	if ( pHydrogenApp->getPatternEditorPanel() != nullptr &&
		 ( pDrumPatternEditor->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getVelocityEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getPanEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getLeadLagEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getNoteKeyEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getProbabilityEditor()->hasFocus() ||
		   pHydrogenApp->getPatternEditorPanel()->getPianoRollEditor()->hasFocus() ) &&
		! pHydrogenApp->hideKeyboardCursor() ) {

		int nCursorX = m_fGridWidth *
			pHydrogenApp->getPatternEditorPanel()->getCursorPosition() +
			PatternEditor::nMargin - 4 -
			m_fGridWidth * 5;

		// Middle line to indicate the selected tick
		painter.setPen( QPen( pPref->getColorTheme()->m_cursorColor, 2 ) );
		painter.setRenderHint( QPainter::Antialiasing );
		painter.drawLine( nCursorX + m_fGridWidth * 5 + 4, height() - 6,
						  nCursorX + m_fGridWidth * 5 + 4, height() - 13 );
		painter.drawLine( nCursorX, 3, nCursorX + m_fGridWidth * 10 + 8, 3 );
		painter.drawLine( nCursorX, 4, nCursorX, 5 );
		painter.drawLine( nCursorX + m_fGridWidth * 10 + 8, 4,
						  nCursorX + m_fGridWidth * 10 + 8, 5 );
		painter.drawLine( nCursorX, height() - 5,
						  nCursorX + m_fGridWidth * 10 + 8, height() - 5 );
		painter.drawLine( nCursorX, height() - 7,
						  nCursorX, height() - 6 );
		painter.drawLine( nCursorX + m_fGridWidth * 10 + 8, height() - 6,
						  nCursorX + m_fGridWidth * 10 + 8, height() - 7 );
	}
	
	// Fall back to default values in case the GUI is starting and the
	// pattern editor is not ready yet.
	float fResolution;
	bool bIsUsingTriplets;
	if ( pDrumPatternEditor != nullptr ) {
		fResolution = static_cast<float>(pDrumPatternEditor->getResolution());
		bIsUsingTriplets = pDrumPatternEditor->isUsingTriplets();
	} else {
		fResolution = 8;
		bIsUsingTriplets = false;
	}

	float fTripletFactor;
	if ( bIsUsingTriplets ) {
		fTripletFactor = 3;
	} else {
		fTripletFactor = 4;
	}

	// draw playhead
	if ( m_nTick != -1 ) {
		int nOffset = Skin::getPlayheadShaftOffset();
		int x = static_cast<int>(static_cast<float>(PatternEditor::nMargin) +
								  static_cast<float>(m_nTick) *
								  m_fGridWidth);

		Skin::drawPlayhead( &painter, x - nOffset, 3, false );
		painter.drawLine( x, 8, x, height() - 1 );
	}

	// Display playhead on hovering
	if ( m_nHoveredColumn > -1 ) {
		int x = PatternEditor::nMargin +
			static_cast<int>(m_nHoveredColumn * 4 * static_cast<float>(MAX_NOTES) /
							 ( fTripletFactor * fResolution ) * m_fGridWidth);

		if ( x < m_nWidthActive ) {
			int nOffset = Skin::getPlayheadShaftOffset();
			Skin::drawPlayhead( &painter, x - nOffset, 3, true );
			painter.drawLine( x, 8, x, height() - 1 );
		}
	}
}

bool PatternEditorRuler::updateActiveRange() {
	
	auto pAudioEngine = H2Core::Hydrogen::get_instance()->getAudioEngine();
	int nTicksInPattern = MAX_NOTES;

	auto pPlayingPatterns = pAudioEngine->getPlayingPatterns();
	if ( pPlayingPatterns->size() != 0 ) {
		// Virtual patterns are already expanded in the playing
		// patterns and must not be considered when determining the
		// longest one.
		nTicksInPattern = pPlayingPatterns->longest_pattern_length( false );
	}

	int nWidthActive = PatternEditor::nMargin + nTicksInPattern * m_fGridWidth;
	if ( m_nWidthActive != nWidthActive ) {
		m_nWidthActive = nWidthActive;
		return true;
	}

	return false;
}

void PatternEditorRuler::zoomIn()
{
	if ( m_fGridWidth >= 3 ){
		m_fGridWidth *= 2;
	} else {
		m_fGridWidth *= 1.5;
	}
	m_nRulerWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );
	resize( QSize( m_nRulerWidth, m_nRulerHeight ) );

	updateActiveRange();
	
	invalidateBackground();
	update();
}


void PatternEditorRuler::zoomOut()
{
	if ( m_fGridWidth > 1.5 ) {
		if ( m_fGridWidth > 3 ){
			m_fGridWidth /= 2;
		} else {
			m_fGridWidth /= 1.5;
		}
		m_nRulerWidth = PatternEditor::nMargin + m_fGridWidth * ( MAX_NOTES * 4 );
		resize( QSize(m_nRulerWidth, m_nRulerHeight) );
		
		updateActiveRange();
		
		invalidateBackground();
		update();
	}
}


void PatternEditorRuler::songModeActivationEvent()
{
	updateEditor( true );
}

void PatternEditorRuler::stateChangedEvent( H2Core::AudioEngine::State )
{
	updatePosition();
}
	
void PatternEditorRuler::selectedPatternChangedEvent()
{
	updateEditor( true );
}

void PatternEditorRuler::playingPatternsChangedEvent() {
	if ( ! PatternEditor::isUsingAdditionalPatterns( m_pPattern ) ) {
		if ( updateActiveRange() ) {
			invalidateBackground();
			update();
		}
	}
}

void PatternEditorRuler::onPreferencesChanged( H2Core::Preferences::Changes changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
		update( 0, 0, width(), height() );
	}
}
