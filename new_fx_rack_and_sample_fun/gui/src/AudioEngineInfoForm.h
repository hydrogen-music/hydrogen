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


#ifndef AUDIO_ENGINE_INFO_FORM_H
#define AUDIO_ENGINE_INFO_FORM_H

#include "config.h"
#include <hydrogen/Object.h>

#include "EventListener.h"
#include "ui_AudioEngineInfoForm_UI.h"

#include <QtGui>

/**
 * Audio Engine information form
 */
class AudioEngineInfoForm : public QWidget, public Ui_AudioEngineInfoForm_UI, public EventListener, public Object
{
	Q_OBJECT
	private:
		QTimer *timer;

		virtual void updateAudioEngineState();

		// EventListener implementation
		virtual void stateChangedEvent(int nState);
		virtual void patternChangedEvent();
		//~ EventListener implementation

	public:
		AudioEngineInfoForm(QWidget* parent);
		~AudioEngineInfoForm();

		void showEvent ( QShowEvent *ev );
		void hideEvent ( QHideEvent *ev );

	public slots:
		void updateInfo();
};

#endif

