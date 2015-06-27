#include "AutomationPathView.h"
#include <hydrogen/Preferences.h>
#include "../SongEditor/SongEditor.h"
#include <iostream>

const char* AutomationPathView::__class_name = "AutomationPathView";

using namespace H2Core;

AutomationPathView::AutomationPathView(QWidget *parent)
	: QWidget(parent),
	  H2Core::Object(__class_name),
	  m_nGridWidth(16),
	  m_nMarginWidth(10),
	  m_nMarginHeight(4),
	  m_bIsHolding(false)
{
	Preferences *pref = Preferences::get_instance();
	m_nMaxPatternSequence = pref->getMaxBars();

	_path = nullptr;
	autoResize();
}


void AutomationPathView::setAutomationPath(AutomationPath *path)
{
	_path = path;
	if(_path) {
		_selectedPoint = _path->end();
	}

	update();
}


void AutomationPathView::setGridWidth( int width )
{
	if ( ( SONG_EDITOR_MIN_GRID_WIDTH <= width ) && ( SONG_EDITOR_MAX_GRID_WIDTH >= width ) ) {
		m_nGridWidth = width;
		autoResize();
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
		m_nMarginWidth + p.first * m_nGridWidth,
		m_nMarginHeight + contentHeight * ((_path->get_max()-p.second)/(_path->get_max()-_path->get_min()))
	);
}


/**
 * \brief Check if user clicked within area inside margins
 */
bool AutomationPathView::checkBounds(QMouseEvent *event) const
{
	return event->x() > m_nMarginWidth
		&& event->y() > m_nMarginHeight
		&& event->y() < height()-m_nMarginHeight;
}


/**
 * \brief Locate clicked point on a path
 */
std::pair<const float, float> AutomationPathView::locate(QMouseEvent *event) const
{
	int contentHeight = height() - 2* m_nMarginHeight;

	float x = (event->x() - m_nMarginWidth) / (float)m_nGridWidth;
	float y = ((contentHeight-event->y()+m_nMarginHeight)/(float)contentHeight)
		* (_path->get_max() - _path->get_min()) + _path->get_min();

	return std::pair<const float,float>(x,y);
}


/**
 * \brief Repaint widget
 **/
void AutomationPathView::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	QPen rulerPen(Qt::DotLine);
	rulerPen.setColor(QColor(127, 133, 153));
	painter.setPen(rulerPen);

	/* Paint min, max  */
	int contentHeight = height() - 2* m_nMarginHeight;

	painter.drawLine(0, m_nMarginHeight, width(), m_nMarginHeight);
	painter.drawLine(0, height()-m_nMarginHeight, width(), height()-m_nMarginHeight);

	if (!_path) {
		return;
	}

	/* Paint default */
	QPoint def = translatePoint(0, _path->get_default());
	painter.drawLine(0, def.y(), width(), def.y());

	int slotWidth = 22;

	QPen linePen(QColor(99, 165, 255));
	linePen.setWidth(2);
	painter.setPen(linePen);

	std::pair<float, float> firstPoint = *_path->begin();
	QPoint lastPoint = translatePoint(0,firstPoint.second);
	lastPoint.setX(0);
		
	for (auto point : *_path) {
		QPoint current = translatePoint(point);
		painter.drawLine(lastPoint, current);
		lastPoint = current;
	}
	QPoint last(width(), lastPoint.y());
	painter.drawLine(lastPoint, last);


	painter.setBrush(QBrush(QColor(58,62,72)));

	for (auto point : *_path) {

		QPoint center = translatePoint(point);
		painter.drawEllipse(center, 4, 4);

	}

	/*
	int x = m_nMarginWidth;
	while(x < width()) {

		painter.drawLine(x, m_nMarginHeight, x, contentHeight);

		x += m_nGridWidth;
	}
	*/
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
	if (! checkBounds(event) || !_path)
		return;

	auto p = locate(event);
	float x = p.first;
	float y = p.second;

	_selectedPoint = _path->find(x);
	if (_selectedPoint == _path->end()) {
		_path->add_point(x, y);	
		_selectedPoint = _path->find(x);
	} else {
		_selectedPoint = _path->move(_selectedPoint, x, y);
	}

	update();

	m_bIsHolding = true;
}


/**
 * \brief Handler for relasing mouse button
 *
 * Ends any point drags
 **/
void AutomationPathView::mouseReleaseEvent(QMouseEvent *event)
{
	m_bIsHolding = false;
}


/**
 * \brief Handler for mouse moves
 *
 * Moves selected point.
 */
void AutomationPathView::mouseMoveEvent(QMouseEvent *event)
{
	if (! checkBounds(event) || !_path)
		return;

	auto p = locate(event);
	float x = p.first;
	float y = p.second;

	if(m_bIsHolding) {
		_selectedPoint = _path->move(_selectedPoint, x, y);
	}

	update();
}


/**
 * \brief Resize widget to fit everything
 **/
void AutomationPathView::autoResize()
{
	resize ( 10 + m_nMaxPatternSequence * m_nGridWidth, 64 );
}
