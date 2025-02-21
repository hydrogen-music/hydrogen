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
#ifndef MIXER_LINE_H
#define MIXER_LINE_H

#include <QtGui>
#include <QtWidgets>
#include <vector>

#include "../Widgets/PixmapWidget.h"

#include <core/EventQueue.h>
#include <core/Globals.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

class Button;
class Fader;
class InstrumentNameWidget;
class LCDDisplay;
class LED;
class Rotary;
class WidgetWithInput;

///
/// A mixer strip
///
/** \ingroup docGUI*/
class MixerLine: public PixmapWidget, public H2Core::Object<MixerLine>
{
	H2_OBJECT(MixerLine)
	Q_OBJECT

public:
		static constexpr int nWidth = 56;
		static constexpr int nHeight = 254;

	MixerLine( QWidget* pParent, int nInstrument );
	~MixerLine();

	void	updateMixerLine();

	bool	isMuteClicked() const;
	void	setMuteClicked( bool bIsClicked );

	bool	isSoloClicked() const;
	void	setSoloClicked( bool bIsClicked );

	float	getVolume() const;
	void	setVolume( float fValue,
					   H2Core::Event::Trigger trigger =
					      H2Core::Event::Trigger::Default );

	void	setPeak_L( float fPeak );
	float	getPeak_L() const;

	void	setPeak_R( float fPeak );
	float	getPeak_R() const;

	void	setName( const QString& sName );
	const QString& getName() const;

	float	getPan() const;
	void	setPan( float fValue,
					H2Core::Event::Trigger trigger =
				       H2Core::Event::Trigger::Default );

	int		getActivity() const {	return m_nActivity;	}
	void	setActivity( int nValue ) {	m_nActivity = nValue; }

	void	setPlayClicked( bool bCicked );

	void	setFXLevel( int nFX, float fValue,
						H2Core::Event::Trigger trigger =
						   H2Core::Event::Trigger::Default );
	float	getFXLevel( int nFX ) const;

	void	setSelected( bool bIsSelected );

signals:
	void	muteBtnClicked(MixerLine *ref);
	void	soloBtnClicked(MixerLine *ref);
	void	volumeChanged(MixerLine *ref);
	void	instrumentNameClicked(MixerLine *ref);
	void	instrumentNameSelected(MixerLine *ref);
	void	noteOnClicked(MixerLine *ref);
	void	noteOffClicked(MixerLine *ref);
	void	panChanged(MixerLine *ref);
	void	knobChanged(MixerLine *ref, int nKnob);

public slots:
	void	muteBtnClicked();
	void	soloBtnClicked();
	void	faderChanged(WidgetWithInput *ref);
	void	panChanged(WidgetWithInput *ref);
	void	knobChanged(WidgetWithInput *ref);
	void	nameClicked();
	void	nameSelected();

private:
	bool	m_bIsSelected;

	int	m_nActivity;
	int	m_nPeakTimer;
	float	m_fMaxPeak;
	float	m_nFalloffSpeed;
		
	Fader *					m_pFader;
	Rotary*					m_pPanRotary;
	InstrumentNameWidget *	m_pNameWidget;
	Button *			m_pMuteBtn;
	Button *			m_pSoloBtn;
	Button *				m_pPlaySampleBtn;
	LED*				m_pTriggerSampleLED;
	LED*				m_pSelectionLED;
	std::vector<Rotary*> m_fxRotaries;

	LCDDisplay *			m_pPeakLCD;
};

#endif
