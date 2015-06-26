#ifndef AUTOMATION_PATH_VIEW_H
#define AUTOMATION_PATH_VIEW_H

#include <hydrogen/object.h>
#include <hydrogen/basics/automation_path.h>

#include <QtGui>

class AutomationPathView : public QWidget, public H2Core::Object
{
	Q_OBJECT
	H2_OBJECT

	H2Core::AutomationPath *_path;
	unsigned m_nGridWidth;   /** < Width of song grid cell size - in order to properly align AutomationPathView and SongEditor */
	unsigned m_nMarginWidth; /** < Width of an empty space on the left side */
	unsigned m_nMarginHeight;/** < Height of top and bottom margins */
	unsigned m_nMaxPatternSequence;

	bool m_bIsHolding; /** < Whether any points are being dragged */
	H2Core::AutomationPath::iterator _selectedPoint; /** < Point that is being dragged */

public:
	AutomationPathView(QWidget *parent = 0);

	H2Core::AutomationPath *getAutomationPath() const noexcept { return _path; }
	void setAutomationPath(H2Core::AutomationPath *path);

	int  getGridWidth() const noexcept { return m_nGridWidth; }
	void setGridWidth(int width);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;


	QPoint translatePoint(float x, float y) const;
	QPoint translatePoint(const std::pair<float,float> &p) const;
	bool checkBounds(QMouseEvent *event) const;
	std::pair<const float, float> locate(QMouseEvent *) const;

	void autoResize();
};

#endif
