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
#ifndef LADSPA_FX_LINE_H
#define LADSPA_FX_LINE_H

#include <QtGui>
#include <QtWidgets>

#include <memory>

#include <core/Object.h>

class Button;
class LCDDisplay;
class Rotary;
class ClickableLabel;

namespace H2Core {
	class LadspaFX;
}

#include "../Widgets/PixmapWidget.h"

/** \ingroup docGUI*/
class LadspaFXLine : public PixmapWidget,
					 public H2Core::Object<LadspaFXLine>
{
	H2_OBJECT(LadspaFXLine)
	Q_OBJECT

public:
		static constexpr int nWidth = 194;
		static constexpr int nHeight = 43;

		explicit LadspaFXLine( QWidget* pParent,
							   std::shared_ptr<H2Core::LadspaFX> pFX );
		~LadspaFXLine();

		void updateLine();

		std::shared_ptr<H2Core::LadspaFX> getFX() const;
		void setFX( std::shared_ptr<H2Core::LadspaFX> pFX );

private:
		std::shared_ptr<H2Core::LadspaFX> m_pFX;

	Button *		m_pBypassBtn;
	Button *		m_pEditBtn;
	Rotary *		m_pVolumeRotary;
	LCDDisplay *	m_pNameLCD;
	ClickableLabel* m_pReturnLbl;
};

inline std::shared_ptr<H2Core::LadspaFX> LadspaFXLine::getFX() const {
	return m_pFX;
}
#endif
