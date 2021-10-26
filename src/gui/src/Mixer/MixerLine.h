/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */
#ifndef MIXERLINE_H
#define MIXERLINE_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Globals.h>
#include <core/Preferences.h>

class Fader;
class MasterFader;
class PanFader;
//class Knob;
class Button;
class ToggleButton;
class InstrumentPropertiesDialog;
class InstrumentNameWidget;
class LCDDisplay;
class Rotary;

#include "../Widgets/PixmapWidget.h"
#include "../Widgets/WidgetWithScalableFont.h"

/** \ingroup docGUI*/
class InstrumentNameWidget : public PixmapWidget, public H2Core::Object<InstrumentNameWidget>, protected WidgetWithScalableFont<8, 10, 12>
{
	H2_OBJECT(InstrumentNameWidget)
	Q_OBJECT
	public:
		explicit InstrumentNameWidget(QWidget* parent);
		~InstrumentNameWidget();

		void	setText(QString text);
		QString text();

		void	mousePressEvent( QMouseEvent * e ) override;
		void	mouseDoubleClickEvent( QMouseEvent * e ) override;

public slots:
	void onPreferencesChanged( bool bAppearanceOnly );
	
	signals:
		void	clicked();
		void	doubleClicked();

	protected:
		virtual void paintEvent(QPaintEvent *ev) override;

	private:
		int			m_nWidgetWidth;
		int			m_nWidgetHeight;
		QString		m_sInstrName;
		/** Used to detect changed in the font*/
		QString m_sLastUsedFontFamily;
		/** Used to detect changed in the font*/
		H2Core::Preferences::FontSize m_lastUsedFontSize;
};




///
/// A mixer strip
///
/** \ingroup docGUI*/
class MixerLine: public PixmapWidget, public H2Core::Object<MixerLine>
{
	H2_OBJECT(MixerLine)
	Q_OBJECT
	public:
		MixerLine(QWidget* parent, int nInstr);
		~MixerLine();

		void	updateMixerLine();

		bool	isMuteClicked();
		void	setMuteClicked(bool isClicked);

		bool	isSoloClicked();
		void	setSoloClicked(bool isClicked);

		float	getVolume();
		void	setVolume(float value);

		void	setPeak_L( float peak );
		float	getPeak_L();

		void	setPeak_R( float peak );
		float	getPeak_R();

		void	setName(QString name) {     m_pNameWidget->setText( name );        }
		QString getName() {      return m_pNameWidget->text();        }

		float	getPan();
		void	setPan(float value);

		int		getActivity() {	return m_nActivity;	}
		void	setActivity( uint value ) {	m_nActivity = value;	}

		void	setPlayClicked( bool clicked );

		void	setFXLevel( uint nFX, float fValue );
		float	getFXLevel( uint nFX );

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
		void	click(Button *ref);
		void	rightClick(Button *ref);
		void	faderChanged(Fader *ref);
		void	panChanged(Rotary *ref);
		void	knobChanged(Rotary *ref);
		void	nameClicked();
		void	nameSelected();

	private:
		uint	m_nWidth;
		uint	m_nHeight;
		bool	m_bIsSelected;

		uint	m_nActivity;
		uint	m_nPeakTimer;
		float	m_fMaxPeak;
		float	m_nFalloff;
		
		Fader *					m_pFader;
		Rotary*					m_pPanRotary;
		InstrumentNameWidget *	m_pNameWidget;
		ToggleButton *			m_pMuteBtn;
		ToggleButton *			m_pSoloBtn;
		Button *				m_pPlaySampleBtn;
		Button *				m_pTriggerSampleLED;
		Rotary *				m_pFxRotary[MAX_FX];

		LCDDisplay *			m_pPeakLCD;
};

/** \ingroup docGUI*/
class ComponentMixerLine: public PixmapWidget, public H2Core::Object<ComponentMixerLine>
{
	H2_OBJECT(ComponentMixerLine)
	Q_OBJECT
	public:
		ComponentMixerLine(QWidget* parent, int CompoID);
		~ComponentMixerLine();

		void	updateMixerLine();

		bool	isMuteClicked();
		void	setMuteClicked(bool isClicked);

		bool	isSoloClicked();
		void	setSoloClicked(bool isClicked);

		float	getVolume();
		void	setVolume(float value);

		void	setPeak_L( float peak );
		float	getPeak_L();

		void	setPeak_R( float peak );
		float	getPeak_R();

		void	setName(QString name) {     m_pNameWidget->setText( name );        }
		QString getName() {      return m_pNameWidget->text();        }

		int		getComponentID(){ return m_nComponentID; }

	signals:
		void	muteBtnClicked(ComponentMixerLine *ref);
		void	soloBtnClicked(ComponentMixerLine *ref);
		void	volumeChanged(ComponentMixerLine *ref);

	public slots:
		void	click(Button *ref);
		void	faderChanged(Fader *ref);


	private:
		int		m_nComponentID;
		uint	m_nWidth;
		uint	m_nHeight;
		bool	m_bIsSelected;

		uint	m_nActivity;
		uint	m_nPeakTimer;
		float	m_fMaxPeak;
		float	m_nFalloff;
		
		Fader *					m_pFader;
		InstrumentNameWidget *	m_pNameWidget;
		ToggleButton *			m_pMuteBtn;
		ToggleButton *			m_pSoloBtn;
		LCDDisplay *			m_pPeakLCD;
};






/** \ingroup docGUI*/
class MasterMixerLine: public PixmapWidget, public H2Core::Object<MasterMixerLine>
{
	H2_OBJECT(MasterMixerLine)
	Q_OBJECT
	public:
		explicit MasterMixerLine(QWidget* parent);
		~MasterMixerLine();

		void	updateMixerLine();

		float	getVolume();
		void	setVolume(float value);

		void	setPeak_L(float peak);
		float	getPeak_L();

		void	setPeak_R(float peak);
		float	getPeak_R();


	signals:
		void	volumeChanged(MasterMixerLine *ref);


	public slots:
		void	faderChanged(MasterFader * ref);
		void	rotaryChanged( Rotary *pRef );
		void	muteClicked(Button*);

	private:
		uint	m_nWidth;
		uint	m_nHeight;

		uint	m_nPeakTimer;
		float	m_fMaxPeak;
		float	m_nFalloff;
		
		Fader *			m_pFader;
		MasterFader *	m_pMasterFader;

		LCDDisplay *	m_pPeakLCD;

		Rotary *		m_pSwingRotary;
		Rotary *		m_pHumanizeTimeRotary;
		Rotary *		m_pHumanizeVelocityRotary;

		ToggleButton *	m_pMuteBtn;
};




///
/// Mixer strip for FX
///
/** \ingroup docGUI*/
class FxMixerLine: public PixmapWidget, public H2Core::Object<FxMixerLine>
{
	H2_OBJECT(FxMixerLine)
	Q_OBJECT
	public:
		explicit FxMixerLine(QWidget* parent);
		~FxMixerLine();

		float	getVolume();
		void	setVolume(float value);

		void	setPeak_L(float peak);
		float	getPeak_L();

		void	setPeak_R(float peak);
		float	getPeak_R();

		void	setName(QString name) {     m_pNameWidget->setText( name );        }
		QString getName() {      return m_pNameWidget->text();        }

		bool	isFxActive();
		void	setFxActive( bool active );

	signals:
		void	volumeChanged( FxMixerLine *ref );
		void	instrumentNameClicked( FxMixerLine *ref );
		void	activeBtnClicked( FxMixerLine *ref );

	public slots:
		void	click(Button *ref);
		void	faderChanged(Fader * ref);

	private:
		uint	m_nWidth;
		uint	m_nHeight;
		float	m_fMaxPeak;
		
		Fader *					m_pFader;
		InstrumentNameWidget *	m_pNameWidget;
		ToggleButton *			activeBtn;
		LCDDisplay *			m_pPeakLCD;
};




/** \ingroup docGUI*/
class LadspaFXMixerLine : public PixmapWidget, public H2Core::Object<LadspaFXMixerLine>
{
	H2_OBJECT(LadspaFXMixerLine)
	Q_OBJECT
	public:
		explicit LadspaFXMixerLine(QWidget* parent);
		~LadspaFXMixerLine();

		bool	isFxActive();
		void	setFxActive( bool active );
		
		void	setPeaks( float fPeak_L, float fPeak_R );
		void	getPeaks( float *fPeak_L, float *fPeak_R );
		void	setName( QString name );
		
		float	getVolume();
		void	setVolume( float value );

	public slots:
		void click(Button *ref);
		void rotaryChanged(Rotary * ref);

	signals:
		void activeBtnClicked( LadspaFXMixerLine *ref );
		void editBtnClicked( LadspaFXMixerLine *ref );
		void volumeChanged( LadspaFXMixerLine *ref);

	private:
		float			m_fMaxPeak;
		ToggleButton *	m_pActiveBtn;
		Button *		m_pEditBtn;
		Rotary *		m_pRotary;
		LCDDisplay *	m_pNameLCD;
};


#endif
