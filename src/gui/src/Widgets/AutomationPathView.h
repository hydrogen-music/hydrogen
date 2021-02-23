/*
 * Hydrogen
 * Copyright(c) 2015-2016 by Przemysław Sitek
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
#ifndef AUTOMATION_PATH_VIEW_H
#define AUTOMATION_PATH_VIEW_H

#include <core/Object.h>
#include <core/Basics/AutomationPath.h>

#include <QtGui>
#include <QtWidgets>

class AutomationPathView : public QWidget, public H2Core::Object
{
	Q_OBJECT
	H2_OBJECT

	H2Core::AutomationPath *_path;
	int m_nGridWidth;   /** < Width of song grid cell size - in order to properly align AutomationPathView and SongEditor */
	int m_nMarginWidth; /** < Width of an empty space on the left side */
	int m_nMarginHeight;/** < Height of top and bottom margins */
	int m_nMaxPatternSequence;

	bool m_bIsHolding; /** < Whether any points are being dragged */
	bool m_bPointAdded;/** < Whether a new point was added during mouse move */
	float m_fOriginX;  /** < Original position of selected point */
	float m_fOriginY;  /** < Original position of selected point */
	H2Core::AutomationPath::iterator _selectedPoint; /** < Point that is being dragged */

public:
	AutomationPathView(QWidget *parent = nullptr);

	H2Core::AutomationPath *getAutomationPath() const noexcept { return _path; }
	void setAutomationPath(H2Core::AutomationPath *path);

	int  getGridWidth() const noexcept { return m_nGridWidth; }
	void setGridWidth(int width);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;


	QPoint translatePoint(float x, float y) const;
	QPoint translatePoint(const std::pair<float,float> &p) const;
	bool checkBounds(QMouseEvent *event) const;
	std::pair<const float, float> locate(QMouseEvent *) const;

	void autoResize();

signals:
	void valueChanged();

	void pointAdded(float x, float y);
	void pointRemoved(float x, float y);
	void pointMoved(float ox, float oy, float tx, float ty);
};

#endif
