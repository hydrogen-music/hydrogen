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

#include <core/EventQueue.h>
#include <core/Object.h>

class Button;
class LCDDisplay;
class Rotary;
class WidgetWithInput;
class ClickableLabel;

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

	explicit LadspaFXLine(QWidget* parent);
	~LadspaFXLine();

	bool	isFxBypassed() const;
	void	setFxBypassed( bool bActive );
		
	void	setPeaks( float fPeak_L, float fPeak_R );
	void	getPeaks( float *fPeak_L, float *fPeak_R ) const;
	void	setName( const QString& sName );
		
	float	getVolume() const;
	void	setVolume( float fValue,
					   H2Core::Event::Trigger trigger =
					      H2Core::Event::Trigger::Default );

public slots:
	void bypassBtnClicked();
	void editBtnClicked();
	void rotaryChanged( WidgetWithInput* ref);

signals:
	void bypassBtnClicked( LadspaFXLine *ref );
	void editBtnClicked( LadspaFXLine *ref );
	void volumeChanged( LadspaFXLine *ref );

private:
	Button *		m_pBypassBtn;
	Button *		m_pEditBtn;
	Rotary *		m_pRotary;
	LCDDisplay *	m_pNameLCD;
	ClickableLabel* m_pReturnLbl;
};

#endif
