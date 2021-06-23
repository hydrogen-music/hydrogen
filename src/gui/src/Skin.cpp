/*
 * Hydrogen
 * Copyright(c) 2008-2021 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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

#include "Skin.h"

QString Skin::getGlobalStyleSheet() {
	return QString( "\
QToolTip { \
    padding: 1px; \
    border: 1px solid rgb(199, 202, 204); \
    background-color: rgb(227, 243, 252); \
    color: rgb(64, 64, 66); \
} \
QPushButton { \
    color: %2; \
    border: 1px solid %2; \
    border-radius: 2px; \
    padding: 5px; \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #dae0f2, stop: 1 #9298aa); \
} \
QPushButton:hover { \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #e1e7fa, stop: 1 #9ba1b4); \
} \
QPushButton:checked { \
    background-color: qlineargradient(x1: 0.1, y1: 0.1, x2: 1, y2: 1, \
                                      stop: 0 #a2cdff, stop: 1 #69a2e5); \
} \
QComboBox { \
    color: %2; \
    background-color: %3; \
} \
QComboBox QAbstractItemView { \
    background-color: #babfcf; \
} \
QLineEdit { \
    color: #ffffff; \
    background-color: %1; \
} \
QDoubleSpinBox, QSpinBox { \
    color: #ffffff; \
    background-color: %4; \
    selection-color: #f0f0f0; \
    selection-background-color: #334a64; \
}"
					)
		.arg( getWindowColor().name() )
		.arg( getCustomButtonTextColor().name() )
		.arg( getCustomButtonColor().name() )
		.arg( getBlueAccentColor().name() );
}
