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

#ifndef ROTARY_H
#define ROTARY_H

#include <QtGui>
#include <QtWidgets>
#include <QSvgRenderer>

#include "WidgetWithInput.h"

#include <core/Object.h>
#include <core/Preferences/Preferences.h>

/** Custom rotary widget.
 *
 * The background image - for Rotary::Type::Normal and
 * Rotary::Type::Center - as well as the knob is loaded via a SVG
 * image. The arc indicating the current value will the painted within
 * Qt.
 *
 */
/** \ingroup docGUI docWidgets*/
class Rotary : public WidgetWithInput, public H2Core::Object<Rotary>
{
    H2_OBJECT(Rotary)
	
public:
	enum class Type {
		/** The arc is of solid red color.*/
		Normal,
		/** The arc features a point at its upmost position. If set
		 * by the user, it shows a green color. If another value is
		 * used, a grey and smaller dot will be displayed instead.*/
		Center,
		/** No arc will be drawn*/
		Small
	};

		static constexpr int nWidthSmall = 18;
		static constexpr int nHeightSmall = 18;
		static constexpr int nWidth = 44;
		static constexpr int nHeight = 26;

	Rotary(const Rotary&) = delete;
	Rotary& operator=( const Rotary& rhs ) = delete;
	
	Rotary( QWidget* parent, const Type& type, const QString& sBaseTooltip,
			bool bUseIntSteps, float fMin = 0.0, float fMax = 1.0,
			bool bModifyOnChange = true );
	~Rotary();

public slots:
	void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

private:
	Type m_type;
	QSvgRenderer* m_background;
	QSvgRenderer* m_knob;

	virtual void paintEvent(QPaintEvent *ev) override;
};
#endif
