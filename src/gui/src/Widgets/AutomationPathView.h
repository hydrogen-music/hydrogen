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

#ifndef AUTOMATION_PATH_VIEW_H
#define AUTOMATION_PATH_VIEW_H

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/AutomationPath.h>

#include <QtGui>
#include <QtWidgets>

/** \ingroup docGUI docAutomation docWidgets*/
class AutomationPathView :  public QWidget,  public H2Core::Object<AutomationPathView>
{
	Q_OBJECT
	H2_OBJECT(AutomationPathView)

	void createBackground();

	H2Core::AutomationPath *_path;
	int m_nGridWidth;   /** < Width of song grid cell size - in order to properly align AutomationPathView and SongEditor */
	int m_nMarginHeight;/** < Height of top and bottom margins */
	int m_nMaxPatternSequence;

	bool m_bIsHolding; /** < Whether any points are being dragged */
	bool m_bPointAdded;/** < Whether a new point was added during mouse move */
	float m_fOriginX;  /** < Original position of selected point */
	float m_fOriginY;  /** < Original position of selected point */
	H2Core::AutomationPath::iterator _selectedPoint; /** < Point that is being dragged */

	float m_fTick;
	QPixmap* m_pBackgroundPixmap;

public:
	AutomationPathView(QWidget *parent = nullptr);
	~AutomationPathView();

	H2Core::AutomationPath *getAutomationPath() const noexcept { return _path; }
	void setAutomationPath(H2Core::AutomationPath *path, bool bUpdate = true);

	int  getGridWidth() const noexcept { return m_nGridWidth; }
	void setGridWidth(int width);

	void updatePosition( float fTick );
	void updateAutomationPath();

		static constexpr int m_nMinimumHeight = 64;

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

public slots:
	void onPreferencesChanged( H2Core::Preferences::Changes changes );

signals:
	void valueChanged();

	void pointAdded(float x, float y);
	void pointRemoved(float x, float y);
	void pointMoved(float ox, float oy, float tx, float ty);
};

#endif
