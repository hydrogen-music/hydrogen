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
#ifndef MASTER_LINE_H
#define MASTER_LINE_H

#include <QtGui>
#include <QtWidgets>

#include <core/EventQueue.h>
#include <core/Object.h>

class Button;
class ClickableLabel;
class Fader;
class LCDDisplay;
class Rotary;
class WidgetWithInput;

#include "../Widgets/PixmapWidget.h"

/** \ingroup docGUI*/
class MasterLine: public PixmapWidget, public H2Core::Object<MasterLine>
{
	H2_OBJECT(MasterLine)
	Q_OBJECT

public:
		static constexpr int nWidth = 126;
		static constexpr int nHeight = 284;

	explicit MasterLine(QWidget* parent);
	~MasterLine();

	void	updateMixerLine();

	bool	isMuteChecked() const;
	void	setMuteChecked( bool bIsChecked );

	float	getVolume() const;
	void	setVolume( float fValue,
					   H2Core::Event::Trigger trigger =
					      H2Core::Event::Trigger::Default );

	float	getHumanizeTime() const;
	void	setHumanizeTime( float fValue,
							 H2Core::Event::Trigger trigger =
							    H2Core::Event::Trigger::Default );

	float	getHumanizeVelocity() const;
	void	setHumanizeVelocity( float fValue,
								 H2Core::Event::Trigger trigger =
								    H2Core::Event::Trigger::Default );

	float	getSwing() const;
	void	setSwing( float fValue,
					  H2Core::Event::Trigger trigger =
					     H2Core::Event::Trigger::Default );

	void	setPeak_L( float fPeak );
	float	getPeak_L() const;

	void	setPeak_R( float fPeak );
	float	getPeak_R() const;


signals:
	void	volumeChanged(MasterLine *ref);

public slots:
	void	faderChanged( WidgetWithInput* pRef);
	void	rotaryChanged( WidgetWithInput *pRef );
	void	muteClicked();

private:
	int			m_nPeakTimer;
	float			m_fMaxPeak;
	float			m_nFalloffSpeed;
		
	Fader*			m_pMasterFader;

	ClickableLabel* m_pMasterLbl;
	ClickableLabel* m_pHumanizeLbl;
	ClickableLabel* m_pSwingLbl;
	ClickableLabel* m_pTimingLbl;
	ClickableLabel* m_pVelocityLbl;

	LCDDisplay *	m_pPeakLCD;

	Rotary *		m_pSwingRotary;
	Rotary *		m_pHumanizeTimeRotary;
	Rotary *		m_pHumanizeVelocityRotary;

	Button *		m_pMuteBtn;
};

#endif
