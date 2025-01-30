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


#ifndef MIXER_H
#define MIXER_H

#include <QtGui>
#include <QtWidgets>

#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Globals.h>
#include "../EventListener.h"

class Button;
class MixerLine;
class ComponentMixerLine;
class MasterMixerLine;
class LadspaFXMixerLine;
class PixmapWidget;

/** \ingroup docGUI*/
class Mixer :  public QWidget, public EventListener,  public H2Core::Object<Mixer>
{
	H2_OBJECT(Mixer)
	Q_OBJECT
	public:
		explicit Mixer(QWidget* parent);
		~Mixer();

		void showEvent ( QShowEvent *ev ) override;
		void hideEvent ( QHideEvent *ev ) override;
		void resizeEvent ( QResizeEvent *ev ) override;
		void soloClicked(uint nLine);
		bool isSoloClicked(uint nLine);

		void getPeaksInMixerLine( uint nMixerLine, float& fPeak_L, float& fPeak_R );

	public slots:
		void noteOnClicked(MixerLine* ref);
		void noteOffClicked(MixerLine* ref);
		void muteClicked(MixerLine* ref);
		void muteClicked(ComponentMixerLine* ref);
		void soloClicked(MixerLine* ref);
		void soloClicked(ComponentMixerLine* ref);
		void volumeChanged(MixerLine* ref);
		void volumeChanged(ComponentMixerLine* ref);
		void panChanged(MixerLine* ref);
		void knobChanged(MixerLine* ref, int nKnob);
		void masterVolumeChanged(MasterMixerLine*);
		void nameClicked(MixerLine* ref);
		void nameSelected(MixerLine* ref);
		void updateMixer();
		void showFXPanelClicked();
		void showPeaksBtnClicked();
		void openMixerSettingsDialog();
		void ladspaBypassBtnClicked( LadspaFXMixerLine* ref );
		void ladspaEditBtnClicked( LadspaFXMixerLine *ref );
		void ladspaVolumeChanged( LadspaFXMixerLine *ref );
		void closeEvent(QCloseEvent *event) override;
		void onPreferencesChanged( H2Core::Preferences::Changes changes );

	private:
		QHBoxLayout *			m_pFaderHBox;
		LadspaFXMixerLine *		m_pLadspaFXLine[MAX_FX];

		QScrollArea*			m_pFaderScrollArea;
		Button *				m_pShowFXPanelBtn;
		Button *				m_pShowPeaksBtn;
		Button *				m_pOpenMixerSettingsBtn;
		MasterMixerLine *		m_pMasterLine;

		QWidget *				m_pFaderPanel;
		MixerLine *				m_pMixerLine[MAX_INSTRUMENTS];
		std::map<int, ComponentMixerLine*> m_pComponentMixerLine;

		PixmapWidget *			m_pFXFrame;

		QTimer *				m_pUpdateTimer;

		uint					findMixerLineByRef(MixerLine* ref);
		uint					findCompoMixerLineByRef(ComponentMixerLine* ref);
		MixerLine*				createMixerLine( int );
		ComponentMixerLine*		createComponentMixerLine( int );

		// Implements EventListener interface
		virtual void noteOnEvent( int nInstrument ) override;
		// ~ Implements EventListener interface

};



#endif
