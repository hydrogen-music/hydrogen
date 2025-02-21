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
class LadspaFXLine;
class MasterLine;
class MixerLine;
class PixmapWidget;

/** \ingroup docGUI*/
class Mixer : public QWidget, public EventListener, public H2Core::Object<Mixer>
{
	H2_OBJECT(Mixer)
	Q_OBJECT
	public:

		/** Defines the rate using which the peaks in the Mixer are update. */
		static constexpr int nPeakTimeoutMs = 50;

		explicit Mixer(QWidget* parent);
		~Mixer();

		void updateMixer();

		void showEvent( QShowEvent *ev ) override;
		void hideEvent( QHideEvent *ev ) override;
		void resizeEvent( QResizeEvent *ev ) override;

	public slots:
		void updatePeaks();
		void showFXPanelClicked();
		void showPeaksBtnClicked();
		void openMixerSettingsDialog();
		void ladspaBypassBtnClicked( LadspaFXLine* ref );
		void ladspaEditBtnClicked( LadspaFXLine *ref );
		void ladspaVolumeChanged( LadspaFXLine *ref );
		void closeEvent(QCloseEvent *event) override;
		void onPreferencesChanged( const H2Core::Preferences::Changes& changes );

	private:
		QHBoxLayout *			m_pFaderHBox;
		std::vector<LadspaFXLine*>	m_ladspaFXLines;

		QScrollArea*			m_pFaderScrollArea;
		Button *				m_pShowFXPanelBtn;
		Button *				m_pShowPeaksBtn;
		Button *				m_pOpenMixerSettingsBtn;
		MasterLine *		m_pMasterLine;

		QWidget *				m_pFaderPanel;
		std::vector<MixerLine*>	m_mixerLines;

		PixmapWidget *			m_pFXFrame;

		QTimer *				m_pUpdateTimer;

		int					findMixerLineByRef(MixerLine* ref);

		// Implements EventListener interface
		virtual void mixerSettingsChangedEvent() override;
		virtual void noteOnEvent( int nInstrument ) override;
		// ~ Implements EventListener interface

};



#endif
