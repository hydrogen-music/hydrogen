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

#include "SongEditorPositionRuler.h"

#include <algorithm>

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/TransportPosition.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/CoreActionController.h>
#include <core/Hydrogen.h>

#include "SongEditor.h"
#include "SongEditorPanel.h"
#include "SongEditorPanelBpmWidget.h"
#include "SongEditorPanelTagWidget.h"
#include "PlaybackTrackWaveDisplay.h"
#include "../HydrogenApp.h"
#include "../Skin.h"
#include "../Widgets/AutomationPathView.h"

using namespace H2Core;

SongEditorPositionRuler::SongEditorPositionRuler( QWidget *parent )
 : QWidget( parent )
 , m_bRightBtnPressed( false )
 , m_nActiveBpmWidgetColumn( -1 )
 , m_nHoveredColumn( -1 )
 , m_hoveredRow( HoveredRow::None )
 , m_nTagHeight( 6 )
 , m_fTick( 0 )
 , m_nColumn( 0 )
{

	const auto pPref = H2Core::Preferences::get_instance();

	HydrogenApp::get_instance()->addEventListener( this );
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();

	setAttribute(Qt::WA_OpaquePaintEvent);
	setMouseTracking( true );

	m_nGridWidth = pPref->getSongEditorGridWidth();

	int nInitialWidth = SongEditor::nMargin + pPref->getMaxBars() * m_nGridWidth;

	if ( pSong != nullptr ) {
		m_nActiveColumns = pSong->getPatternGroupVector()->size();
	}
	else {
		m_nActiveColumns = 0;
	}

	resize( nInitialWidth, m_nMinimumHeight );
	setFixedHeight( m_nMinimumHeight );

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( nInitialWidth * pixelRatio, m_nMinimumHeight * pixelRatio );	// initialize the pixmap
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );

	createBackground();	// create m_backgroundPixmap pixmap
	update();

	m_pTimer = new QTimer(this);
	connect( m_pTimer, &QTimer::timeout, [=]() {
		if ( pHydrogen->getAudioEngine()->getState() ==
			 H2Core::AudioEngine::State::Playing ) {
			updatePosition();
		}
	});
	m_pTimer->start(200);
}



SongEditorPositionRuler::~SongEditorPositionRuler() {
	m_pTimer->stop();
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
}

void SongEditorPositionRuler::relocationEvent() {
	updatePosition();
}

void SongEditorPositionRuler::songSizeChangedEvent() {
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong == nullptr ) {
		m_nActiveColumns = 0;
	}
	else {
		m_nActiveColumns = pSong->getPatternGroupVector()->size();
	}
	invalidateBackground();
	update();
}

int SongEditorPositionRuler::getGridWidth()
{
	return m_nGridWidth;
}

void SongEditorPositionRuler::setGridWidth( int width )
{
	if ( SongEditor::nMinGridWidth <= width &&
		 SongEditor::nMaxGridWidth >= width ) {
		m_nGridWidth = width;
		resize( columnToX( Preferences::get_instance()->getMaxBars() ), height() );
		invalidateBackground();
		update();
	}
}

void SongEditorPositionRuler::invalidateBackground() {
	m_bBackgroundInvalid = true;
}

void SongEditorPositionRuler::createBackground()
{
	const auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pTimeline = pHydrogen->getTimeline();
	auto tagVector = pTimeline->getAllTags();
	
	QColor textColor( pPref->getTheme().m_color.m_songEditor_textColor );
	QColor textColorAlpha( textColor );
	textColorAlpha.setAlpha( 45 );

	QColor backgroundColor = pPref->getTheme().m_color.m_songEditor_alternateRowColor.darker( 115 );
	QColor backgroundInactiveColor = pPref->getTheme().m_color.m_midLightColor;
	QColor backgroundColorTempoMarkers = backgroundColor.darker( 120 );

	QColor colorHighlight = pPref->getTheme().m_color.m_highlightColor;

	QColor lineColor = pPref->getTheme().m_color.m_songEditor_lineColor;
	QColor lineColorAlpha( lineColor );
	lineColorAlpha.setAlpha( 45 );
		
	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ||
		 m_pBackgroundPixmap->width() != width() ||
		 m_pBackgroundPixmap->height() != height() ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width()  * pixelRatio , height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );

	QPainter p( m_pBackgroundPixmap );
	p.setFont( font );

	int nActiveWidth = columnToX( m_nActiveColumns ) + 1;
	p.fillRect( 0, 0, width(), height(), backgroundColorTempoMarkers );
	p.fillRect( 0, 25, nActiveWidth, height() - 25, backgroundColor );
	p.fillRect( nActiveWidth, 25, width() - nActiveWidth, height() - 25,
				backgroundInactiveColor );

	int nMaxPatternSequence = pPref->getMaxBars();
	
	QColor textColorGrid( textColor );
	textColorGrid.setAlpha( 200 );
	p.setPen( QPen( textColorGrid, 1, Qt::SolidLine ) );
	for ( int ii = 0; ii < nMaxPatternSequence + 1; ii++) {
		int x = columnToX( ii );

		if ( ( ii % 4 ) == 0 ) {
			p.drawLine( x, height() - 14, x, height() - 1);
		}
		else {
			p.drawLine( x, height() - 6, x, height() - 1);
		}
	}

	// Add every 4th number to the grid
	p.setPen( textColor );
	for ( int i = 0; i < nMaxPatternSequence + 1; i += 4) {
		int x = columnToX( i );

		if ( i < 10 ) {
			p.drawText( x, height() / 2 + 3, m_nGridWidth, height() / 2 - 7,
						Qt::AlignHCenter, QString::number( i + 1 ) );
		} else {
			p.drawText( x + 2, height() / 2 + 3, m_nGridWidth * 3.5, height() / 2 - 7,
						Qt::AlignLeft, QString::number( i + 1 ) );
		}
	}
	
	// draw tags
	p.setPen( pPref->getTheme().m_color.m_accentTextColor );
	
	QFont font2( pPref->getTheme().m_font.m_sApplicationFontFamily, 5 );
	p.setFont( font2 );
		
	for ( const auto& ttag : tagVector ){
		int x = columnToX( ttag->nColumn ) + 4;
		QRect rect( x, height() / 2 - 1 - m_nTagHeight,
					m_nGridWidth - 6, m_nTagHeight );

		p.fillRect( rect, pPref->getTheme().m_color.m_highlightColor.darker( 135 ) );
		p.drawText( rect, Qt::AlignCenter, "T");
	}
	p.setFont( font );

	// draw tempo content
	
	// Draw tempo marker grid.
	if ( ! pHydrogen->isTimelineEnabled() ) {
		p.setPen( textColorAlpha );
	} else {
		QColor tempoMarkerGridColor( textColor );
		tempoMarkerGridColor.setAlpha( 170 );
		p.setPen( tempoMarkerGridColor );
	}
	for ( int ii = 0; ii < nMaxPatternSequence + 1; ii++) {
		int x = columnToX( ii );

		p.drawLine( x, 1, x, 4 );
		p.drawLine( x, height() / 2 - 5, x, height() / 2 );
	}

	// Draw tempo markers
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	for ( const auto& ttempoMarker : tempoMarkerVector ){
		drawTempoMarker( ttempoMarker, false, p );				
	}

	p.setPen( QColor(35, 39, 51) );
	p.drawLine( 0, 0, width(), 0 );
	p.drawLine( 0, height() - 25, width(), height() - 25 );
	p.drawLine( 0, height(), width(), height() );

	m_bBackgroundInvalid = false;
}

void SongEditorPositionRuler::tempoChangedEvent( int ) {
	auto pTimeline = Hydrogen::get_instance()->getTimeline();
	if ( ! pTimeline->isFirstTempoMarkerSpecial() ) {
		return;
	}

	// There is just the special tempo marker -> no tempo markers set
	// by the user. In this case the special marker isn't drawn and
	// doesn't need to be update.
	if ( pTimeline->getAllTempoMarkers().size() == 1 ) {
		return;
	}

	invalidateBackground();
	update();
}

void SongEditorPositionRuler::patternModifiedEvent() {
	// This can change the size of the song and affect the position of
	// the playhead.
	update();
}

void SongEditorPositionRuler::playingPatternsChangedEvent() {
	// Triggered every time the column of the SongEditor grid
	// changed. Either by rolling transport or by relocation.
	update();
}

void SongEditorPositionRuler::leaveEvent( QEvent* ev ){
	m_nHoveredColumn = -1;
	m_hoveredRow = HoveredRow::None;
	update();

	QWidget::leaveEvent( ev );
}

void SongEditorPositionRuler::mouseMoveEvent(QMouseEvent *ev)
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	
	int nColumn = std::max( xToColumn( ev->x() ), 0 );

	HoveredRow row;
	if ( ev->y() > 22 ) {
		row = HoveredRow::Ruler;
	} else if ( ev->y() > 22 - 1 - m_nTagHeight ) {
		row = HoveredRow::Tag;
	} else {
		row = HoveredRow::TempoMarker;
	}
	
	if ( nColumn != m_nHoveredColumn ||
		 row != m_hoveredRow ) {
		// Cursor has moved into a region where the above caching
		// became invalid.
		m_hoveredRow = row;
		m_nHoveredColumn = nColumn;

		update();
	}
	
	if ( !m_bRightBtnPressed && ev->buttons() & Qt::LeftButton ) {
		// Click+drag triggers same action as clicking at new position
		mousePressEvent( ev );
	}
	else if ( ev->buttons() & Qt::RightButton ) {
		// Right-click+drag
		auto pPref = Preferences::get_instance();
		
		if ( nColumn > pSong->getPatternGroupVector()->size() ) {
			pPref->setPunchOutPos(-1);
			return;
		}
		if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
			return;
		}
		pPref->setPunchOutPos( nColumn - 1 );
		update();
	}
}

bool SongEditorPositionRuler::event( QEvent* ev ) {
	if ( ev->type() == QEvent::ToolTip ) {
		const auto helpEv = dynamic_cast<QHelpEvent*>(ev);
		showToolTip( helpEv->pos(), helpEv->globalPos() );
		return 0;
	}

	return QWidget::event( ev );
}

void SongEditorPositionRuler::songModeActivationEvent() {
	updatePosition();
	invalidateBackground();
	update();
}

void SongEditorPositionRuler::timelineActivationEvent() {
	invalidateBackground();
	update();
}

void SongEditorPositionRuler::jackTimebaseStateChangedEvent( int nState ) {
	invalidateBackground();
	update();
}

void SongEditorPositionRuler::updateSongEvent( int nValue ) {

	if ( nValue == 0 ) { // different song opened
		updatePosition();
		songSizeChangedEvent();
	}
}

void SongEditorPositionRuler::showToolTip( const QPoint& pos, const QPoint& globalPos ) {
	auto pHydrogen = Hydrogen::get_instance();
	auto pTimeline = pHydrogen->getTimeline();

	const int nColumn = std::max( xToColumn( pos.x() ), 0 );
	
	if ( pHydrogen->isTimelineEnabled() &&
		 pTimeline->isFirstTempoMarkerSpecial() &&
		 m_hoveredRow == HoveredRow::TempoMarker &&
		 pos.x() < columnToX( 1 ) ) { // first tempo marker 
			const QString sBpm =
				pTimeline->getTempoMarkerAtColumn( nColumn )->getPrettyString( -1 );
		QToolTip::showText( globalPos, QString( "%1: %2" )
							.arg( tr( "The tempo set in the BPM widget will be used as a default for the beginning of the song. Left-click to overwrite it." ) )
							.arg( sBpm ), this );
		
	}
	else if ( m_hoveredRow == HoveredRow::TempoMarker ) {
		if ( pTimeline->hasColumnTempoMarker( nColumn ) ) {
			const QString sBpm =
				pTimeline->getTempoMarkerAtColumn( nColumn )->getPrettyString( -1 );
			QToolTip::showText( globalPos, sBpm, this );
		}
	}
	else if ( m_hoveredRow == HoveredRow::Tag ) {
		// Row containing the tags
		if ( pTimeline->hasColumnTag( nColumn ) ) {
			QToolTip::showText( globalPos,
								pTimeline->getTagAtColumn( nColumn ), this );
		}
	}
}

void SongEditorPositionRuler::showTagWidget( int nColumn )
{
	SongEditorPanelTagWidget dialog( this , nColumn );
	dialog.exec();
}

void SongEditorPositionRuler::showBpmWidget( int nColumn )
{
	bool bTempoMarkerPresent =
		Hydrogen::get_instance()->getTimeline()->hasColumnTempoMarker( nColumn );
	m_nActiveBpmWidgetColumn = nColumn;
	update();
	
	SongEditorPanelBpmWidget dialog( this , nColumn, bTempoMarkerPresent );
	dialog.exec();
	
	m_nActiveBpmWidgetColumn = -1;
	update();
}


void SongEditorPositionRuler::mousePressEvent( QMouseEvent *ev )
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}

	int nColumn = std::max( xToColumn( ev->x() ), 0 );
	
	if (ev->button() == Qt::LeftButton ) {
		if ( ev->y() > 22 ) {
			// Relocate transport using position ruler
			m_bRightBtnPressed = false;

			if ( nColumn > pSong->getPatternGroupVector()->size() ) {
				return;
			}

			if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
				CoreActionController::activateSongMode( true );
				pHydrogen->setIsModified( true );
			}

			CoreActionController::locateToColumn( nColumn );
			update();
		}
		else if ( ev->y() > 22 - 1 - m_nTagHeight ) {
			showTagWidget( nColumn );
		}	
		else if ( pHydrogen->isTimelineEnabled() ){
			showBpmWidget( nColumn );
		}
	}
	else if ( ev->button() == Qt::MiddleButton ) {
		showTagWidget( nColumn );
	}
	else if (ev->button() == Qt::RightButton && ev->y() >= 26) {
		auto pPref = Preferences::get_instance();
		if ( nColumn >= pSong->getPatternGroupVector()->size() ) {
			pPref->unsetPunchArea();
			return;
		}
		if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
			return;
		}
		m_bRightBtnPressed = true;
		// Disable until mouse is moved
		pPref->setPunchInPos( nColumn );
		pPref->setPunchOutPos(-1);
		update();
	}

}




void SongEditorPositionRuler::mouseReleaseEvent(QMouseEvent *ev)
{
	UNUSED( ev );
	m_bRightBtnPressed = false;
}


void SongEditorPositionRuler::paintEvent( QPaintEvent *ev )
{
	auto pHydrogenApp = HydrogenApp::get_instance();
	auto pSongEditor = pHydrogenApp->getSongEditorPanel()->getSongEditor();
	auto pHydrogen = Hydrogen::get_instance();
	auto pTimeline = pHydrogen->getTimeline();
	const auto pPref = Preferences::get_instance();
	const auto theme = pPref->getTheme();
	auto tempoMarkerVector = pTimeline->getAllTempoMarkers();

	if ( m_bBackgroundInvalid ) {
		createBackground();
	}
	
	if (!isVisible()) {
		return;
	}
	
	QColor textColor( theme.m_color.m_songEditor_textColor );
	QColor textColorAlpha( textColor );
	textColorAlpha.setAlpha( 45 );
	QColor highlightColor = theme.m_color.m_highlightColor;
	QColor colorHovered( highlightColor );
	colorHovered.setAlpha( 200 );
	QColor backgroundColor = theme.m_color.m_songEditor_alternateRowColor.darker( 115 );
	QColor backgroundColorTempoMarkers = backgroundColor.darker( 120 );

	int nPunchInPos = pPref->getPunchInPos();
	int nPunchOutPos = pPref->getPunchOutPos();

	QPainter painter(this);
	QFont font( theme.m_font.m_sApplicationFontFamily, getPointSize( theme.m_font.m_fontSize ) );
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ) {
		createBackground();
	}
	QRectF srcRect(
			pixelRatio * ev->rect().x(),
			pixelRatio * ev->rect().y(),
			pixelRatio * ev->rect().width(),
			pixelRatio * ev->rect().height()
	);
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap, srcRect );
	
	// Which tempo marker is the currently used one?
	int nCurrentTempoMarkerColumn = -1;
	for ( const auto& tempoMarker : tempoMarkerVector ) {
		if ( tempoMarker->nColumn > m_nColumn ) {
			break;
		}
		nCurrentTempoMarkerColumn = tempoMarker->nColumn;
	}
	if ( nCurrentTempoMarkerColumn == -1 &&
		 tempoMarkerVector.size() != 0 ) {
		auto pTempoMarker = tempoMarkerVector[ tempoMarkerVector.size() - 1 ];
		if ( pTempoMarker != nullptr ) {
			nCurrentTempoMarkerColumn = pTempoMarker->nColumn;
		}
	}
	if ( nCurrentTempoMarkerColumn != -1 ) {
		auto pTempoMarker = pTimeline->getTempoMarkerAtColumn( nCurrentTempoMarkerColumn );
		if ( pTempoMarker != nullptr ) {
			// Reset the region and overwrite the marker's versio
			// using normal weight.
			const QRect rect = calcTempoMarkerRect( pTempoMarker, true );
			painter.fillRect( rect, backgroundColorTempoMarkers );
			drawTempoMarker( pTempoMarker, true, painter );
		}
	}

	// Draw playhead
	if ( m_fTick != -1 ) {
		int nX = tickToColumn( m_fTick, m_nGridWidth );
		int nShaftOffset = Skin::getPlayheadShaftOffset();
		Skin::drawPlayhead( &painter, nX, height() / 2 + 2, false );
		painter.drawLine( nX + nShaftOffset, 0, nX + nShaftOffset, height() );
	}
			
	// Highlight hovered tick of the Timeline
	if ( m_hoveredRow == HoveredRow::Tag ||
		 ( m_hoveredRow == HoveredRow::TempoMarker &&
		   pHydrogen->isTimelineEnabled() ) ||
		 m_nActiveBpmWidgetColumn != -1 ) {

		int nColumn;
		if ( m_nActiveBpmWidgetColumn != -1 ) {
			nColumn = m_nActiveBpmWidgetColumn;
		} else {
			nColumn = m_nHoveredColumn;
		}
		const int x = columnToX( nColumn );

		// Erase background tick lines to not get any interference.
		painter.fillRect( x, 1, 1, 4,
						  backgroundColorTempoMarkers );
		painter.fillRect( x, height() / 2 - 5, 1, 4,
						  backgroundColorTempoMarkers );

		if ( m_hoveredRow == HoveredRow::TempoMarker ) {
			// Erase the area of horizontal line above a tempo
			// marker too. The associated highlight will be drawn later on.
			painter.fillRect( x - 1, 1, m_nGridWidth - 1, 4,
							  backgroundColorTempoMarkers );
		}

		painter.setPen( QPen( highlightColor, 1 ) );

		painter.drawLine( x, 1, x, 4 );
		painter.drawLine( x, height() / 2 - 5, x, height() / 2 - 1 );
	}

	// Highlight tag
	bool bTagPresent = false;
	if ( m_hoveredRow == HoveredRow::Tag &&
		 pTimeline->hasColumnTag( m_nHoveredColumn ) ) {

		int x = columnToX( m_nHoveredColumn ) + 4;
		QRect rect( x, height() / 2 - 1 - m_nTagHeight,
					m_nGridWidth - 6, m_nTagHeight );
	
		QFont font2( theme.m_font.m_sApplicationFontFamily, 5 );
		painter.setFont( font2 );
		
		painter.fillRect( rect, theme.m_color.m_highlightColor );
		painter.setPen( theme.m_color.m_highlightedTextColor );
		painter.drawText( rect, Qt::AlignCenter, "T");

		painter.setFont( font );
		bTagPresent = true;
	}

	// Draw a slight highlight around the tempo marker hovered using
	// mouse or touch events. This will also redraw the
	// tempo marker to ensure it's visible (they can overlap with
	// neighboring ones and be hardly readable).
	bool bTempoMarkerPresent = false;
	if ( ! bTagPresent &&
		 ( ( pHydrogen->isTimelineEnabled() &&
			 m_hoveredRow == HoveredRow::TempoMarker ) ||
		   m_nActiveBpmWidgetColumn != -1 ) ) {

		int nColumn;
		if ( m_nActiveBpmWidgetColumn != -1 ) {
			nColumn = m_nActiveBpmWidgetColumn;
		} else {
			nColumn = m_nHoveredColumn;
		}

		if ( pTimeline->hasColumnTempoMarker( nColumn ) ||
			 ( pTimeline->isFirstTempoMarkerSpecial() &&
			   nColumn == 0 ) ) {

			auto pTempoMarker = pTimeline->getTempoMarkerAtColumn( nColumn );
			if ( pTempoMarker != nullptr ) {

				const bool bEmphasize = pTempoMarker->nColumn == nCurrentTempoMarkerColumn;
				const QRect rect = calcTempoMarkerRect( pTempoMarker, bEmphasize );
		
				painter.fillRect( rect, backgroundColorTempoMarkers );
				drawTempoMarker( pTempoMarker, bEmphasize, painter );
				
				if ( m_nActiveBpmWidgetColumn == -1 ) {
					painter.setPen( QPen( colorHovered, 1 ) );
				} else {
					painter.setPen( QPen( highlightColor, 1 ) );
				}
				painter.drawRect( rect );

				painter.drawLine( rect.x(), 2, rect.x() + m_nGridWidth / 2, 2 );

				bTempoMarkerPresent = true;
			}
		}
	}

	// Draw hovering highlights in tempo marker row
	if ( ( m_nHoveredColumn > -1 &&
		   ( ( m_hoveredRow == HoveredRow::Tag && !bTagPresent ) ||
			 ( m_hoveredRow == HoveredRow::TempoMarker &&
			   pHydrogen->isTimelineEnabled() &&
			   ! bTempoMarkerPresent ) ) ) ||
		 ( m_nActiveBpmWidgetColumn != -1 &&
		   ! bTempoMarkerPresent ) ) {

		QColor color;
		if ( m_nActiveBpmWidgetColumn == -1 ) {
			color = colorHovered;
		} else {
			color = highlightColor;
		}
		QPen p( color );
		p.setWidth( 1 );
		painter.setPen( p );

		int nCursorX;
		if ( m_nActiveBpmWidgetColumn != -1 ) {
			nCursorX = columnToX( m_nActiveBpmWidgetColumn ) + 3;
		} else {
			nCursorX = columnToX( m_nHoveredColumn ) + 3;
		}

		if ( m_hoveredRow == HoveredRow::TempoMarker ||
			 m_nActiveBpmWidgetColumn != -1 ) {
			// Reset the background during highlight in order to
			// indicate that no tempo marker is present in this
			// column.
			QRect hoveringRect( nCursorX, 6, m_nGridWidth - 5, 12 );
			painter.fillRect( hoveringRect, backgroundColorTempoMarkers );
			painter.drawRect( hoveringRect );
		} else {
			painter.drawRect( nCursorX, height() / 2 - 1 - m_nTagHeight,
							  m_nGridWidth - 5, m_nTagHeight - 1 );
		}
	}

	// Draw cursor
	if ( ! pHydrogenApp->hideKeyboardCursor() && pSongEditor->hasFocus() ) {
		int nCursorX = columnToX( pSongEditor->getCursorColumn() ) + 2;

		QColor cursorColor = theme.m_color.m_cursorColor;

		QPen p( cursorColor );
		p.setWidth( 2 );
		painter.setPen( p );
		painter.setRenderHint( QPainter::Antialiasing );
		// Aim to leave a visible gap between the border of the
		// pattern cell, and the cursor line, for consistency and
		// visibility.
		painter.drawLine( nCursorX, height() / 2 + 3,
						  nCursorX + m_nGridWidth - 3, height() / 2 + 3 );
		painter.drawLine( nCursorX, height() / 2 + 4,
						  nCursorX, height() / 2 + 5 );
		painter.drawLine( nCursorX + m_nGridWidth - 3, height() / 2 + 4,
						  nCursorX + m_nGridWidth - 3, height() / 2 + 5 );
		painter.drawLine( nCursorX, height() - 4,
						  nCursorX + m_nGridWidth - 3, height() - 4 );
		painter.drawLine( nCursorX, height() - 6,
						  nCursorX, height() - 5 );
		painter.drawLine( nCursorX + m_nGridWidth - 3, height() - 6,
						  nCursorX + m_nGridWidth - 3, height() - 5 );
	}

	// Faint playhead over hovered position marker.
	if ( m_nHoveredColumn > -1  &&
		 m_hoveredRow == HoveredRow::Ruler &&
		 m_nHoveredColumn <= m_nActiveColumns ) {

		int x = tickToColumn( m_nHoveredColumn, m_nGridWidth );
		int nShaftOffset = Skin::getPlayheadShaftOffset();
		Skin::drawPlayhead( &painter, x, height() / 2 + 2, true );
		painter.drawLine( x + nShaftOffset, 0, x + nShaftOffset, height() / 2 + 1 );
		painter.drawLine( x + nShaftOffset, height() / 2 + 2 + Skin::nPlayheadHeight,
						  x + nShaftOffset, height() );
	}

	if ( nPunchInPos <= nPunchOutPos ) {
		const int xIn = columnToX( nPunchInPos );
		const int xOut = columnToX( nPunchOutPos + 1 );
		painter.fillRect( xIn, 30, xOut-xIn+1, 12, QColor(200, 100, 100, 100) );
		QPen pen(QColor(200, 100, 100));
		painter.setPen(pen);
		painter.drawRect( xIn, 30, xOut-xIn+1, 12 );
	}
}

QRect SongEditorPositionRuler::calcTempoMarkerRect( std::shared_ptr<const Timeline::TempoMarker> pTempoMarker, bool bEmphasize ) const {
	assert( pTempoMarker );

	auto pTimeline = Hydrogen::get_instance()->getTimeline();
	const auto pPref = Preferences::get_instance();
	auto weight = QFont::Normal;
	if ( bEmphasize ) {
		weight = QFont::Bold;
	}
	
	const QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily,
					  getPointSize( pPref->getTheme().m_font.m_fontSize ), weight );

	const int x = columnToX( pTempoMarker->nColumn );
	int nWidth = QFontMetrics( font ).size(
		Qt::TextSingleLine, pTempoMarker->getPrettyString( 2 ) ).width();

	// Check whether the full width would overlap with an adjacent
	// tempo marker and trim it if necessary
	const int nCoveredNeighborColumns = 
		static_cast<int>(std::floor( static_cast<float>(nWidth) /
									 static_cast<float>(m_nGridWidth) ) );
	for ( int ii = 1; ii <= nCoveredNeighborColumns; ++ii ) {
		if ( pTimeline->hasColumnTempoMarker( pTempoMarker->nColumn + ii ) ) {
			nWidth = m_nGridWidth * ii;
			break;
		}
	}
	
	QRect rect( x, 6, nWidth, 12 );

	return std::move( rect );
}

void SongEditorPositionRuler::drawTempoMarker( std::shared_ptr<const Timeline::TempoMarker> pTempoMarker, bool bEmphasize, QPainter& painter ) {
	assert( pTempoMarker );

	const auto pPref = Preferences::get_instance();
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pTimeline = pHydrogen->getTimeline();

	// Only paint the special tempo marker in case Timeline is
	// activated.
	if ( pTempoMarker->nColumn == 0 && pTimeline->isFirstTempoMarkerSpecial() &&
		 ! pHydrogen->isTimelineEnabled() ) {
		return;
	}
		
	QFont font( pPref->getTheme().m_font.m_sApplicationFontFamily, getPointSize( pPref->getTheme().m_font.m_fontSize ) );
		
	QRect rect = calcTempoMarkerRect( pTempoMarker, bEmphasize );

	// Draw an additional small horizontal line at the top of the
	// current column to better indicate the position of the tempo
	// marker (for larger float values e.g. 130.67).
	QColor textColor( pPref->getTheme().m_color.m_songEditor_textColor );

	if ( pTempoMarker->nColumn == 0 && pTimeline->isFirstTempoMarkerSpecial() ) {
		textColor = textColor.darker( 150 );
	}
			
	if ( ! pHydrogen->isTimelineEnabled() ) {
		QColor textColorAlpha( textColor );
		textColorAlpha.setAlpha( 45 );
		painter.setPen( textColorAlpha );
	} else {
		QColor tempoMarkerGridColor( textColor );
		tempoMarkerGridColor.setAlpha( 170 );
		painter.setPen( tempoMarkerGridColor );
	}

	painter.drawLine( rect.x(), 2, rect.x() + m_nGridWidth / 2, 2 );

	QColor tempoMarkerColor( textColor );
	if ( ! pHydrogen->isTimelineEnabled() ) {
		tempoMarkerColor.setAlpha( 45 );
	}
	painter.setPen( tempoMarkerColor );
				
	if ( bEmphasize ) {
		font.setBold( true );
	}
	painter.setFont( font );
	painter.drawText( rect, Qt::AlignLeft | Qt::AlignVCenter,
					  pTempoMarker->getPrettyString( 2 ) );

	if ( bEmphasize ) {
		font.setBold( false );
	}
	painter.setFont( font );
}

void SongEditorPositionRuler::updatePosition()
{
	auto pHydrogen = Hydrogen::get_instance();
	auto pSong = pHydrogen->getSong();
	if ( pSong == nullptr ) {
		return;
	}
	auto pAudioEngine = pHydrogen->getAudioEngine();
	const auto pTimeline = pHydrogen->getTimeline();
	const auto pPref = Preferences::get_instance();
	const auto tempoMarkerVector = pTimeline->getAllTempoMarkers();
	
	pAudioEngine->lock( RIGHT_HERE );

	const auto pTransportPos = pAudioEngine->getTransportPosition();
	const auto pPatternGroupVector = pSong->getPatternGroupVector();
	m_nColumn = std::max( pTransportPos->getColumn(), 0 );

	float fTick = static_cast<float>(m_nColumn);

	if ( pHydrogen->getMode() == Song::Mode::Pattern ) {
		fTick = -1;
	}
	else {
		// Song mode
		if ( pTransportPos->getColumn() == -1 ) {
			// Transport reached end of song. This can mean we switched from
			// Pattern Mode and transport wasn't started yet -> playhead at
			// beginning or we reached the end during playback -> playhead stays
			// at the end.
			if ( pAudioEngine->isEndOfSongReached( pTransportPos ) ) {
				fTick = pPatternGroupVector->size();
			}
			else {
				fTick = 0;
			}
		}
		else if ( pPatternGroupVector->size() > m_nColumn &&
				  pPatternGroupVector->at( m_nColumn )->size() > 0 ) {
			int nLength = pPatternGroupVector->at( m_nColumn )->longest_pattern_length();
			fTick += (float)pTransportPos->getPatternTickPosition() /
				(float)nLength;
		}
		else {
			// Empty column. Use the default length.
			fTick += (float)pTransportPos->getPatternTickPosition() /
				(float)MAX_NOTES;
		}

		if ( fTick < 0 ) {
			// As some variables of the audio engine are initialized as or
			// reset to -1 we ensure this does not affect the position of
			// the playhead in the SongEditor.
			fTick = 0;
		}
	}

	pAudioEngine->unlock();

	if ( fTick != m_fTick ) {
		float fDiff = static_cast<float>(m_nGridWidth) * (fTick - m_fTick);

		m_fTick = fTick;
		int nX = tickToColumn( m_fTick, m_nGridWidth );

		QRect updateRect( nX -2, 0, 4 + Skin::nPlayheadWidth, height() );
		update( updateRect );
		if ( fDiff > 1.0 || fDiff < -1.0 ) {
			// New cursor is far enough away from the old one that the single update rect won't cover both. So
			// update at the old location as well.
			updateRect.translate( -fDiff, 0 );
			update( updateRect );
		}

		auto pSongEditorPanel = HydrogenApp::get_instance()->getSongEditorPanel();
		if ( pSongEditorPanel != nullptr ) {
			pSongEditorPanel->getSongEditor()->updatePosition( fTick );
			pSongEditorPanel->getPlaybackTrackWaveDisplay()->updatePosition( fTick );
			pSongEditorPanel->getAutomationPathView()->updatePosition( fTick );
		}
	}
}

int SongEditorPositionRuler::columnToX( int nColumn ) const {
	return SongEditor::nMargin + nColumn * static_cast<int>(m_nGridWidth);
}
int SongEditorPositionRuler::xToColumn( int nX ) const {
	return static_cast<int>(
		( static_cast<float>(nX) - static_cast<float>(SongEditor::nMargin)) /
		static_cast<float>(m_nGridWidth));
}
int SongEditorPositionRuler::tickToColumn( float fTick, int nGridWidth ) {
	return static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
								   fTick * static_cast<float>(nGridWidth) -
							 static_cast<float>(Skin::nPlayheadWidth) / 2 );
}


void SongEditorPositionRuler::timelineUpdateEvent( int nValue )
{
	invalidateBackground();
	update();
}

void SongEditorPositionRuler::onPreferencesChanged( const H2Core::Preferences::Changes& changes )
{
	if ( changes & ( H2Core::Preferences::Changes::Colors |
					 H2Core::Preferences::Changes::Font ) ) {
			 
		resize( SongEditor::nMargin +
				Preferences::get_instance()->getMaxBars() * m_nGridWidth, height() );
		invalidateBackground();
		update();
	}
}

