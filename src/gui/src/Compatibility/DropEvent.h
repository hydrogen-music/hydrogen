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

#ifndef DROPEVENT_H_
#define DROPEVENT_H_

#include <QDropEvent>

/** Compatibility class to support QDropEvent more esily in Qt5 and Qt6.
 *
 * At some point, when we dropped Qt5 support, we can drop this class too and
 * use the plain `QDropEvent` again. Therefore, it is important to _not_ add
 * any members incompatible with the Qt6 interface of QDropEvent. */
/** \ingroup docGUI docWidgets*/
class DropEvent : public QDropEvent
{

public:
	DropEvent( QDropEvent* );
	~DropEvent();

	QPointF	position() const;
};

#endif // DROPEVENT_H_
