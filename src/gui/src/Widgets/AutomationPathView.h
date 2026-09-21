/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

public:
	AutomationPathView(QWidget *parent = nullptr);
	~AutomationPathView();

	std::shared_ptr<H2Core::AutomationPath> getAutomationPath() const noexcept
	{
		return m_pPath;
	}
	void setAutomationPath(
		std::shared_ptr<H2Core::AutomationPath> pPath,
		bool bUpdate = true
	);

	int  getGridWidth() const noexcept { return m_nGridWidth; }
	void setGridWidth(int width);

	void updatePosition( float fTick );
	void updateAutomationPath();
	void updateView();

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
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

signals:
	void valueChanged();

private:
	void createBackground();

	std::shared_ptr<H2Core::AutomationPath> m_pPath;
	int m_nGridWidth;   /** < Width of song grid cell size - in order to properly align AutomationPathView and SongEditor */
	int m_nMarginHeight;/** < Height of top and bottom margins */
	int m_nMaxPatternSequence;

	bool m_bIsHolding; /** < Whether any points are being dragged */
	H2Core::AutomationPath::iterator _selectedPoint; /** < Point that is being dragged */

	float m_fTick;
	QPixmap* m_pBackgroundPixmap;


};

#endif
