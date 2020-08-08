/*
 * Hydrogen
 * Copyright(c) 2002-2020 by the Hydrogen Team
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

#ifndef WIDGETSCROLLAREA_H
#define WIDGETSCROLLAREA_H

#include <QWidget>

/* Qt's QScrollArea widget has some behaviour that's quite unpleasant
 * for Hydrogen. When tabbing to the next or previous widget, a
 * QScrollArea will ensure that the most recently (or currently)
 * focused child widget of the QScrollArea is visible.
 *
 * In the Hydrogen GUI, ScrollAreas typically are occupied by one
 * single large widget, whose extent is larger than that visible to
 * the ScrollArea. In this case, QScrollArea will center the viewport
 * on the centre of the child widget.
 *
 * The movement is distracting and confusing and will likely remove
 * the area the user is *actually* focusing from view.
 *
 * This class overrides this behaviour.
 */
class WidgetScrollArea : public QScrollArea
{
public:
	WidgetScrollArea( QWidget *parent ) : QScrollArea( parent ) {}

	bool focusNextPrevChild( bool next ) override
	{
		return QWidget::focusNextPrevChild( next );
	}
};

#endif
