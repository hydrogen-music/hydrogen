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
#ifndef INSTRUMENT_NAME_WIDGET_H
#define INSTRUMENT_NAME_WIDGET_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

#include "../Widgets/PixmapWidget.h"
#include "../Widgets/WidgetWithScalableFont.h"

/** \ingroup docGUI*/
class InstrumentNameWidget : public PixmapWidget,
							 public H2Core::Object<InstrumentNameWidget>,
							 protected WidgetWithScalableFont<8, 10, 12>
{
	H2_OBJECT(InstrumentNameWidget)
	Q_OBJECT

public:
		static constexpr int nWidth = 17;
		static constexpr int nHeight = 116;

	explicit InstrumentNameWidget(QWidget* parent);
	~InstrumentNameWidget();

	void	setText( const QString& sText );
	const QString& text() const;

	void	mousePressEvent( QMouseEvent * e ) override;
	void	mouseDoubleClickEvent( QMouseEvent * e ) override;

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );
	
signals:
	void	clicked();
	void	doubleClicked();

protected:
	virtual void paintEvent(QPaintEvent *ev) override;

private:
	int			m_nWidgetWidth;
	int			m_nWidgetHeight;
	QString		m_sInstrName;
};

#endif
