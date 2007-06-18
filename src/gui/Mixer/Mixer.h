/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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
 * $Id: Mixer.h,v 1.8 2005/05/09 18:11:49 comix Exp $
 *
 */


#ifndef MIXER_H
#define MIXER_H

#include "qpixmap.h"
#include "qwidget.h"
#include "qtimer.h"
#include "qscrollview.h"
#include "qhbox.h"

#include "config.h"
#include "lib/Object.h"
#include "lib/Globals.h"
#include "gui/EventListener.h"

class Button;
class ToggleButton;
class MixerLine;
class FxMixerLine;
class MasterMixerLine;
class LadspaFXMixerLine;

class Mixer : public QWidget, public EventListener, public Object
{
	Q_OBJECT
	public:
		Mixer(QWidget* parent);
		~Mixer();

		void showEvent ( QShowEvent *ev );
		void hideEvent ( QHideEvent *ev );
		void resizeEvent ( QResizeEvent *ev );
		void soloClicked(uint nLine);

		void getPeaksInMixerLine( uint nMixerLine, float& fPeak_L, float& fPeak_R );

	public slots:
		void noteOnClicked(MixerLine* ref);
		void noteOffClicked(MixerLine* ref);
		void muteClicked(MixerLine* ref);
		void soloClicked(MixerLine* ref);
		void volumeChanged(MixerLine* ref);
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

	private:
		unsigned m_nMixerWidth;
		unsigned m_nMixerHeight;

		LadspaFXMixerLine *m_pLadspaFXLine[MAX_FX];

		QScrollView* m_pFaderScrollView;
		ToggleButton *m_pShowFXPanelBtn;
		ToggleButton *m_pShowPeaksBtn;
		MasterMixerLine *m_pMasterLine;

		QFrame *m_pFaderFrame;
		MixerLine *m_pMixerLine[MAX_INSTRUMENTS];

		QFrame *m_pFXFrame;

		QPixmap m_backgroundPixmap;
		QPixmap m_fxBackgroundPixmap;

		void setupMixer();
		uint findMixerLineByRef(MixerLine* ref);

		// Implements EventListener interface
		virtual void noteOnEvent( int nInstrument );
		//~ Implements EventListener interface

};



#endif

