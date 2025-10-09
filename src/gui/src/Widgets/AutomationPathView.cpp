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

#include "AutomationPathView.h"

#include "../Compatibility/MouseEvent.h"
#include "../SongEditor/SongEditor.h"
#include "../SongEditor/SongEditorPanel.h"
#include "../HydrogenApp.h"
#include "../Skin.h"

#include <core/Basics/Song.h>
#include <core/Hydrogen.h>

using namespace H2Core;

AutomationPathView::AutomationPathView(QWidget *parent)
	: QWidget(parent),
	  H2Core::Object<AutomationPathView>(),
	  m_nGridWidth(16),
	  m_nMarginHeight(4),
	  m_bIsHolding(false),
	  m_fTick( 0 )
{
	setFocusPolicy( Qt::ClickFocus );
	m_nMaxPatternSequence = Preferences::get_instance()->getMaxBars();

	connect( HydrogenApp::get_instance(), &HydrogenApp::preferencesChanged, this, &AutomationPathView::onPreferencesChanged );
	
	_path = nullptr;
	autoResize();

	qreal pixelRatio = devicePixelRatio();
	m_pBackgroundPixmap = new QPixmap( width() * pixelRatio,
									   height() * pixelRatio );
	m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	createBackground();
}

AutomationPathView::~AutomationPathView()
{
	if ( m_pBackgroundPixmap ) {
		delete m_pBackgroundPixmap;
	}
}

void AutomationPathView::onPreferencesChanged( const H2Core::Preferences::Changes& changes ) {
	if ( changes & H2Core::Preferences::Changes::Colors ) {
		createBackground();
		update();
	}
}


void AutomationPathView::setAutomationPath( AutomationPath *path, bool bUpdate )
{
	if ( path == _path ) {
		return;
	}
	_path = path;

	if( _path ) {
		_selectedPoint = path->end();
	}

	if ( bUpdate ) {
		createBackground();
		update();
	}
}

// Make sure we have the current automation path
void AutomationPathView::updateAutomationPath()
{
	auto pSong = Hydrogen::get_instance()->getSong();
	if ( pSong != nullptr ) {
		setAutomationPath( pSong->getVelocityAutomationPath(), false );
	} else {
		setAutomationPath( nullptr, false );
	}
}

void AutomationPathView::setGridWidth( int width )
{
	if ( SongEditor::nMinGridWidth <= width &&
		 SongEditor::nMaxGridWidth >= width ) {
		m_nGridWidth = width;
		autoResize();
		createBackground();
		update();
	}
}


/**
 * \brief Locate path point on a wdiget surface
 **/
QPoint AutomationPathView::translatePoint(float x,float y) const
{
	return translatePoint(std::make_pair(x, y));
}


/**
 * \brief Locate path point on a wdiget surface
 **/
QPoint AutomationPathView::translatePoint(const std::pair<float,float> &p) const
{
	int contentHeight = height() - 2* m_nMarginHeight;

	return QPoint(
		SongEditor::nMargin + p.first * m_nGridWidth,
		m_nMarginHeight + contentHeight * ((_path->get_max()-p.second)/(_path->get_max()-_path->get_min()))
	);
}


/**
 * \brief Check if user clicked within area inside margins
 */
bool AutomationPathView::checkBounds(QMouseEvent *event) const
{
	auto pEv = static_cast<MouseEvent*>( event );

	return pEv->position().x() > SongEditor::nMargin
		&& pEv->position().y() > m_nMarginHeight
		&& pEv->position().y() < height()-m_nMarginHeight;
}


/**
 * \brief Locate clicked point on a path
 */
std::pair<const float, float> AutomationPathView::locate(QMouseEvent *event) const
{
	int contentHeight = height() - 2* m_nMarginHeight;

	auto pEv = static_cast<MouseEvent*>( event );

	float x = (pEv->position().x() - SongEditor::nMargin) / (float)m_nGridWidth;
	float y = ((contentHeight - pEv->position().y() + m_nMarginHeight)/
			   (float)contentHeight)
		* (_path->get_max() - _path->get_min()) + _path->get_min();

	return std::pair<const float,float>(x,y);
}

void AutomationPathView::updatePosition( float fTick ) {
	m_fTick = fTick;
	update();
}

/**
 * \brief Repaint widget
 **/
void AutomationPathView::paintEvent(QPaintEvent *ev)
{
	
	if (!isVisible()) {
		return;
	}
	
	qreal pixelRatio = devicePixelRatio();
	if ( pixelRatio != m_pBackgroundPixmap->devicePixelRatio() ||
		 width() * pixelRatio != m_pBackgroundPixmap->width() ||
		 height() * pixelRatio != m_pBackgroundPixmap->height() ) {
		createBackground();
	}
	
	QPainter painter( this );
	painter.drawPixmap( ev->rect(), *m_pBackgroundPixmap,
						QRectF( pixelRatio * ev->rect().x(),
								pixelRatio * ev->rect().y(),
								pixelRatio * ev->rect().width(),
								pixelRatio * ev->rect().height() ) );

	// Draw playhead
	//
	// Using the grid width of the song editor over class' own one is
	// crucial in order to keep the full-height playhead in sync.
	auto pSongEditorPanel = HydrogenApp::get_instance()->getSongEditorPanel();
	if ( m_fTick != -1 && pSongEditorPanel != nullptr ) {
		int nOffset = Skin::getPlayheadShaftOffset();
		int nX = static_cast<int>( static_cast<float>(SongEditor::nMargin) + 1 +
								   m_fTick *
								   static_cast<float>(pSongEditorPanel->getSongEditor()->
													  getGridWidth()) -
								   static_cast<float>(Skin::nPlayheadWidth) / 2 );
		Skin::setPlayheadPen( &painter, false );
		painter.drawLine( nX + nOffset, 0, nX + nOffset, height() );
	}

}

void AutomationPathView::createBackground() {
	
	const auto pColorTheme = H2Core::Preferences::get_instance()->getColorTheme();
	updateAutomationPath();

	QColor backgroundColor =
		pColorTheme->m_songEditor_automationBackgroundColor;
	QColor automationLineColor =
		pColorTheme->m_songEditor_automationLineColor;
	QColor nodeColor = pColorTheme->m_songEditor_automationNodeColor;
	QColor textColor = pColorTheme->m_songEditor_textColor;

	// Resize pixmap if pixel ratio has changed
	qreal pixelRatio = devicePixelRatio();
	if ( m_pBackgroundPixmap->devicePixelRatio() != pixelRatio ||
		 width() != m_pBackgroundPixmap->width() ||
		 height() != m_pBackgroundPixmap->height() ) {
		delete m_pBackgroundPixmap;
		m_pBackgroundPixmap = new QPixmap( width()  * pixelRatio , height() * pixelRatio );
		m_pBackgroundPixmap->setDevicePixelRatio( pixelRatio );
	}

	m_pBackgroundPixmap->fill( backgroundColor );

	QPainter painter( m_pBackgroundPixmap );
	painter.setRenderHint(QPainter::Antialiasing);

	// Border
	painter.setPen( Qt::black );
	painter.drawLine( 0, 0, width(), 0 );

	QPen rulerPen(Qt::DotLine);
	rulerPen.setColor( textColor );
	painter.setPen(rulerPen);

	/* Paint min, max  */
	painter.drawLine(0, m_nMarginHeight, width(), m_nMarginHeight);
	painter.drawLine(0, height()-m_nMarginHeight, width(), height()-m_nMarginHeight);

	if (!_path) {
		return;
	}

	/* Paint default */
	QPoint def = translatePoint(0, _path->get_default());
	painter.drawLine(0, def.y(), width(), def.y());

	QPen linePen( automationLineColor );
	linePen.setWidth(2);
	painter.setPen(linePen);

	if (_path->empty()) {
		QPoint p = translatePoint(0,_path->get_default());

		painter.drawLine(0, p.y(), width(), p.y());
	} else {
		std::pair<float, float> firstPoint = *_path->begin();
		QPoint lastPoint = translatePoint(0,firstPoint.second);
		lastPoint.setX(0);
		
		for ( const auto& point : *_path) {
			QPoint current = translatePoint(point);
			painter.drawLine(lastPoint, current);
			lastPoint = current;
		}
		QPoint last(width(), lastPoint.y());
		painter.drawLine(lastPoint, last);
	}


	QPen circlePen( nodeColor );
	circlePen.setWidth(1);
	painter.setPen(circlePen);
	painter.setBrush(QBrush( pColorTheme->m_windowColor ));

	for ( const auto& point : *_path) {

		QPoint center = translatePoint(point);
		painter.drawEllipse(center, 3, 3);

	}
}


/**
 * \brief Handle mouse click
 *
 * This function locates point within click proximity, moves it
 * along Y axis. If threre's no point to move, new point is created.
 * That point is marked for move by mouseMoveEvent().
 */
void AutomationPathView::mousePressEvent(QMouseEvent *event)
{
	updateAutomationPath();

	if (! checkBounds(event) || !_path) {
		return;
	}

	auto p = locate(event);
	float x = p.first;
	float y = p.second;

	_selectedPoint = _path->find(x);
	if (_selectedPoint == _path->end()) {
		_path->add_point(x, y);	
		_selectedPoint = _path->find(x);

		m_bPointAdded = true;
	} else {
		_selectedPoint = _path->move(_selectedPoint, x, y);
		m_fOriginX = x;
		m_fOriginY = y;
		m_bPointAdded = false;
	}
	H2Core::Hydrogen::get_instance()->setIsModified( true );

	createBackground();
	update();

	m_bIsHolding = true;

	emit valueChanged();
}


/**
 * \brief Handler for releasing mouse button
 *
 * Ends any point drags
 **/
void AutomationPathView::mouseReleaseEvent(QMouseEvent *event)
{
	updateAutomationPath();
	m_bIsHolding = false;

	if (! checkBounds(event) || !_path) {
		return;
	}

	auto p = locate(event);
	float x = p.first;
	float y = p.second;
	if (m_bPointAdded) {
		emit pointAdded(x, y);
	} else {
		emit pointMoved(m_fOriginX, m_fOriginY, x, y);
	}

	emit valueChanged();
}


/**
 * \brief Handler for mouse moves
 *
 * Moves selected point.
 */
void AutomationPathView::mouseMoveEvent(QMouseEvent *event)
{
	updateAutomationPath();
	if (! checkBounds(event) || !_path) {
		return;
	}

	auto p = locate(event);
	float x = p.first;
	float y = p.second;

	if ( m_bIsHolding && _path && _selectedPoint != _path->end() ) {
		_selectedPoint = _path->move(_selectedPoint, x, y);
		H2Core::Hydrogen::get_instance()->setIsModified( true );
	}

	createBackground();
	update();
}


/**
 * \brief Handler for key presses
 *
 * Removed selected point
 */
void AutomationPathView::keyPressEvent(QKeyEvent *event)
{
	updateAutomationPath();
	if ( event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace ) {
		if ( _path && _selectedPoint != _path->end() ) {
			float x = _selectedPoint->first;
			float y = _selectedPoint->second;
			_path->remove_point(_selectedPoint->first);
			_selectedPoint = _path->end();
			
			H2Core::Hydrogen::get_instance()->setIsModified( true );

			emit pointRemoved( x, y );
			createBackground();
			update();
			emit valueChanged();

			event->accept();
			return;
		}
	}

	event->ignore();
}


/**
 * \brief Resize widget to fit everything
 **/
void AutomationPathView::autoResize()
{
	resize( SongEditor::nMargin + m_nMaxPatternSequence * m_nGridWidth,
			m_nMinimumHeight );
}
