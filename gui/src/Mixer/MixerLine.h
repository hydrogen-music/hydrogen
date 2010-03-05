/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
#ifndef MIXERLINE_H
#define MIXERLINE_H

#include "config.h"

#include <QtGui>

#include <hydrogen/Object.h>
#include <hydrogen/globals.h>

class Fader;
class MasterFader;
class PanFader;
class Knob;
class Button;
class ToggleButton;
class InstrumentPropertiesDialog;
class InstrumentNameWidget;
class LCDDisplay;
class Rotary;

#include "../widgets/PixmapWidget.h"


class InstrumentNameWidget : public PixmapWidget
{
	Q_OBJECT
	public:
		InstrumentNameWidget(QWidget* parent);
		~InstrumentNameWidget();

		void setText(QString text);
		QString text();

		void mousePressEvent( QMouseEvent * e );
		void mouseDoubleClickEvent( QMouseEvent * e );

	signals:
		void clicked();
		void doubleClicked();

	protected:
		virtual void paintEvent(QPaintEvent *ev);

	private:
		int m_nWidgetWidth;
		int m_nWidgetHeight;
		QString m_sInstrName;
		QFont m_mixerFont;
};




///
/// A mixer strip
///
class MixerLine: public PixmapWidget
{
	Q_OBJECT
	public:
		MixerLine(QWidget* parent, int nInstr);
		~MixerLine();

		void updateMixerLine();

		bool isMuteClicked();
		void setMuteClicked(bool isClicked);

		bool isSoloClicked();
		void setSoloClicked(bool isClicked);

		float getVolume();
		void setVolume(float value);

		void setPeak_L( float peak );
		float getPeak_L();

		void setPeak_R( float peak );
		float getPeak_R();

		void setName(QString name) {     m_pNameWidget->setText( name );        }
		QString getName() {      return m_pNameWidget->text();        }

		float getPan();
		void setPan(float value);

		int getActivity() {	return m_nActivity;	}
		void setActivity( uint value ) {	m_nActivity = value;	}

		void setPlayClicked( bool clicked );

		void setFXLevel( uint nFX, float fValue );
		float getFXLevel( uint nFX );

		void setSelected( bool bIsSelected );

	signals:
		void muteBtnClicked(MixerLine *ref);
		void soloBtnClicked(MixerLine *ref);
		void volumeChanged(MixerLine *ref);
		void instrumentNameClicked(MixerLine *ref);
		void instrumentNameSelected(MixerLine *ref);
		void noteOnClicked(MixerLine *ref);
		void noteOffClicked(MixerLine *ref);
		void panChanged(MixerLine *ref);
		void knobChanged(MixerLine *ref, int nKnob);

	public slots:
		void click(Button *ref);
		void rightClick(Button *ref);
		void faderChanged(Fader *ref);
		void panChanged(Rotary *ref);
		void knobChanged(Knob *ref);
		void nameClicked();
		void nameSelected();

	private:
		uint m_nWidth;
		uint m_nHeight;
		bool m_bIsSelected;

		uint m_nActivity;
		uint m_nPeakTimer;
		float m_fMaxPeak;
		float m_nFalloff;
		Fader *m_pFader;
		Rotary *m_pPanRotary;
		InstrumentNameWidget *m_pNameWidget;
		ToggleButton *m_pMuteBtn;
		ToggleButton *m_pSoloBtn;
		Button *m_pPlaySampleBtn;
		Button *m_pTriggerSampleLED;
		Knob *m_pKnob[MAX_FX];

		LCDDisplay *m_pPeakLCD;
};




class MasterMixerLine: public PixmapWidget
{
	Q_OBJECT
	public:
		MasterMixerLine(QWidget* parent);
		~MasterMixerLine();

		void updateMixerLine();

		float getVolume();
		void setVolume(float value);

		void setPeak_L(float peak);
		float getPeak_L();

		void setPeak_R(float peak);
		float getPeak_R();


	signals:
		void volumeChanged(MasterMixerLine *ref);


	public slots:
		void faderChanged(MasterFader * ref);
		void rotaryChanged( Rotary *pRef );
		void muteClicked(Button*);

	private:
		uint m_nWidth;
		uint m_nHeight;

		uint m_nPeakTimer;
		float m_fMaxPeak;
		float m_nFalloff;
		Fader *m_pFader;
		MasterFader *m_pMasterFader;

		LCDDisplay *m_pPeakLCD;

		Rotary *m_pSwingRotary;
		Rotary *m_pHumanizeTimeRotary;
		Rotary *m_pHumanizeVelocityRotary;

		ToggleButton *m_pMuteBtn;
};




///
/// Mixer strip for FX
///
class FxMixerLine: public PixmapWidget
{
	Q_OBJECT
	public:
		FxMixerLine(QWidget* parent);
		~FxMixerLine();

		float getVolume();
		void setVolume(float value);

		void setPeak_L(float peak);
		float getPeak_L();

		void setPeak_R(float peak);
		float getPeak_R();

		void setName(QString name) {     m_pNameWidget->setText( name );        }
		QString getName() {      return m_pNameWidget->text();        }

		bool isFxActive();
		void setFxActive( bool active );

	signals:
		void volumeChanged( FxMixerLine *ref );
		void instrumentNameClicked( FxMixerLine *ref );
		void activeBtnClicked( FxMixerLine *ref );

	public slots:
		void click(Button *ref);
		void faderChanged(Fader * ref);

	private:
		uint m_nWidth;
		uint m_nHeight;

		float m_fMaxPeak;
		Fader *m_pFader;
		InstrumentNameWidget *m_pNameWidget;
		ToggleButton *activeBtn;

		LCDDisplay *m_pPeakLCD;
};




class LadspaFXMixerLine : public PixmapWidget
{
	Q_OBJECT
	public:
		LadspaFXMixerLine(QWidget* parent);
		~LadspaFXMixerLine();

		bool isFxActive();
		void setFxActive( bool active );
		void setPeaks( float fPeak_L, float fPeak_R );
		void getPeaks( float *fPeak_L, float *fPeak_R );
		void setName( QString name );
		float getVolume();
		void setVolume( float value );

	public slots:
		void click(Button *ref);
		void rotaryChanged(Rotary * ref);

	signals:
		void activeBtnClicked( LadspaFXMixerLine *ref );
		void editBtnClicked( LadspaFXMixerLine *ref );
		void volumeChanged( LadspaFXMixerLine *ref);

	private:
		float m_fMaxPeak;
		ToggleButton *m_pActiveBtn;
		Button *m_pEditBtn;
		Rotary *m_pRotary;
		LCDDisplay *m_pNameLCD;
};


#endif
