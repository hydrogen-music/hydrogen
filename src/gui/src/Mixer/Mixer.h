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


#ifndef MIXER_H
#define MIXER_H


#include <QtGui>
#if QT_VERSION >= 0x050000
#  include <QtWidgets>
#endif

#include <hydrogen/object.h>
#include <hydrogen/globals.h>
#include "../EventListener.h"

class Button;
class ToggleButton;
class MixerLine;
class ComponentMixerLine;
class FxMixerLine;
class MasterMixerLine;
class LadspaFXMixerLine;
class PixmapWidget;

class Mixer : public QWidget, public EventListener, public H2Core::Object
{
	Q_OBJECT
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
		Mixer(QWidget* parent);
		~Mixer();

		void showEvent ( QShowEvent *ev );
		void hideEvent ( QHideEvent *ev );
		void resizeEvent ( QResizeEvent *ev );
		void soloClicked(uint nLine);
		bool isSoloClicked(uint nLine);

		void getPeaksInMixerLine( uint nMixerLine, float& fPeak_L, float& fPeak_R );

	public slots:
		void noteOnClicked(MixerLine* ref);
		void noteOffClicked(MixerLine* ref);
		void muteClicked(MixerLine* ref);
		void muteClicked(ComponentMixerLine* ref);
		void unmuteAll( bool findSelectedInstr);
		void unmuteAll( int selectedInstr);
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
		void showFXPanelClicked(Button* ref);
		void showPeaksBtnClicked(Button* ref);
		void ladspaActiveBtnClicked( LadspaFXMixerLine* ref );
		void ladspaEditBtnClicked( LadspaFXMixerLine *ref );
		void ladspaVolumeChanged( LadspaFXMixerLine* ref);
		void closeEvent(QCloseEvent *event);

	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
		QHBoxLayout *			m_pFaderHBox;
		LadspaFXMixerLine *		m_pLadspaFXLine[MAX_FX];

		QScrollArea*			m_pFaderScrollArea;
		ToggleButton *			m_pShowFXPanelBtn;
		ToggleButton *			m_pShowPeaksBtn;
		MasterMixerLine *		m_pMasterLine;

		QWidget *				m_pFaderPanel;
		MixerLine *				m_pMixerLine[MAX_INSTRUMENTS];
		std::map<int, ComponentMixerLine*> m_pComponentMixerLine;

		PixmapWidget *			m_pFXFrame;

		QTimer *				m_pUpdateTimer;

		uint findMixerLineByRef(MixerLine* ref);
		uint findCompoMixerLineByRef(ComponentMixerLine* ref);
		MixerLine* createMixerLine( int );
		ComponentMixerLine* createComponentMixerLine( int );

		// Implements EventListener interface
		virtual void noteOnEvent( int nInstrument );
		//~ Implements EventListener interface

};



#endif
