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


#ifndef AUDIO_ENGINE_INFO_FORM_H
#define AUDIO_ENGINE_INFO_FORM_H

#include <core/Object.h>
#include <core/AudioEngine/AudioEngine.h>

#include "EventListener.h"
#include "ui_AudioEngineInfoForm_UI.h"

#include <QtGui>
#include <QtWidgets>

/**
 * Audio Engine information form
 */
/** \ingroup docGUI docAudioEngine docDebugging*/
class AudioEngineInfoForm :  public QWidget, public Ui_AudioEngineInfoForm_UI, public EventListener,  public H2Core::Object<AudioEngineInfoForm>
{
    H2_OBJECT(AudioEngineInfoForm)
	Q_OBJECT
	private:
		QTimer* m_pTimer;

		// EventListener implementation
	virtual void stateChangedEvent( const H2Core::AudioEngine::State& state) override;
	virtual void playingPatternsChangedEvent() override;
	virtual void updateSongEvent(int) override;
		// ~ EventListener implementation

	public:
		explicit AudioEngineInfoForm(QWidget* parent);
		~AudioEngineInfoForm();

		void showEvent ( QShowEvent *ev ) override;
		void hideEvent ( QHideEvent *ev ) override;

	public slots:
		void updateInfo();

	private:
		void updateAudioEngineState();
};

#endif

