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

#ifndef HYDROGEN_APP_H
#define HYDROGEN_APP_H

#include "config.h"

#include <hydrogen/Object.h>
#include <hydrogen/globals.h>

#include "EventListener.h"

#include <iostream>
#include <vector>
#include <QtGui>

namespace H2Core
{
	class Song;
}

class SongEditorPanel;
class MainForm;
class PlayerControl;
class PatternEditorPanel;
class InstrumentEditorPanel;
class SongEditor;
class Mixer;
class AudioEngineInfoForm;
class SimpleHTMLBrowser;
class LadspaFXProperties;
class LadspaFXInfo;
class LadspaFXGroup;
class InstrumentRack;
class PlaylistDialog;
class SampleEditor;
class Director;

class HydrogenApp : public QObject, public Object
{
	Q_OBJECT
	public:
		HydrogenApp( MainForm* pMainForm, H2Core::Song *pFirstSong );

		/// Returns the instance of HydrogenApp class
		static HydrogenApp* get_instance();

		virtual ~HydrogenApp();

		void setSong( H2Core::Song* pSong );

		void showPreferencesDialog();
		void showMixer(bool bShow);
		void showAudioEngineInfoForm();
		void showPlaylistDialog();
		void showDirector();
		void showSampleEditor( QString name, int mSelectedLayer );

		Mixer* getMixer() {	return m_pMixer;	}
		MainForm* getMainForm() {	return m_pMainForm;	}
		SongEditorPanel* getSongEditorPanel() {	return m_pSongEditorPanel;	}
		AudioEngineInfoForm* getAudioEngineInfoForm() {	return m_pAudioEngineInfoForm;	}
		PlaylistDialog* getPlayListDialog() {	return m_pPlaylistDialog;	}
		Director* getDirector() { return m_pDirector; }
		SampleEditor* getSampleEditor() {  return m_pSampleEditor;	}
		SimpleHTMLBrowser* getHelpBrowser() {	return m_pHelpBrowser;	}
		PatternEditorPanel* getPatternEditorPanel() {	return m_pPatternEditorPanel;	}
		PlayerControl* getPlayerControl() {	return m_pPlayerControl;	}
		InstrumentRack* getInstrumentRack(){	return m_pInstrumentRack;	}


		void setStatusBarMessage( const QString& msg, int msec = 0 );
		void setScrollStatusBarMessage( const QString& msg, int msec = 0, bool test = true );
                void setWindowTitle( const QString& title);

#ifdef LADSPA_SUPPORT
		LadspaFXProperties* getLadspaFXProperties(uint nFX) {	return m_pLadspaFXProperties[nFX];	}
#endif
		void addEventListener( EventListener* pListener );
		void removeEventListener( EventListener* pListener );
		void closeFXProperties();

		void onDrumkitLoad( QString name );
		void enableDestructiveRecMode();

	public slots:
		void onEventQueueTimer();

	private:
		static HydrogenApp *m_pInstance;	///< HydrogenApp instance

#ifdef LADSPA_SUPPORT
		LadspaFXProperties *m_pLadspaFXProperties[MAX_FX];
#endif
		MainForm *m_pMainForm;
		Mixer *m_pMixer;
		PatternEditorPanel* m_pPatternEditorPanel;
		AudioEngineInfoForm *m_pAudioEngineInfoForm;
		SongEditorPanel *m_pSongEditorPanel;
		SimpleHTMLBrowser *m_pHelpBrowser;
		SimpleHTMLBrowser *m_pFirstTimeInfo;
		InstrumentRack* m_pInstrumentRack;
		PlayerControl *m_pPlayerControl;
		PlaylistDialog *m_pPlaylistDialog;
		SampleEditor *m_pSampleEditor;
		Director *m_pDirector;
		QTimer *m_pEventQueueTimer;
		std::vector<EventListener*> m_eventListeners;

		// implement EngineListener interface
		void engineError(uint nErrorCode);

		//void setupTopLevelInterface();
		void setupSinglePanedInterface();
		void showInfoSplash();
};


#endif
