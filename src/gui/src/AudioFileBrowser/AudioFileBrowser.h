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

#ifndef AUDIOFILEBROWSER_H
#define AUDIOFILEBROWSER_H

#include "ui_AudioFileBrowser_UI.h"

#include <QDialog>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {
class Instrument;
class Sample;
}  // namespace H2Core

///
/// This dialog is used to preview audiofiles
///
/** \ingroup docGUI*/
class AudioFileBrowser : public QDialog,
						 public Ui_AudioFileBrowser_UI,
						 public H2Core::Object<AudioFileBrowser>

{
	H2_OBJECT( AudioFileBrowser )
	Q_OBJECT
   public:
	static constexpr int nPlayheadUpdateTimeout = 20;

	AudioFileBrowser(
		QWidget* pParent,
		bool bAllowMultiSelect,
		bool bShowInstrumentManipulationControls,
		const QString& sDefaultPath = "",
		const QString& sFileName = ""
	);
	~AudioFileBrowser();

	QStringList getSelectedFiles() const;
	QString getSelectedDirectory() const;
	bool useFileNameAsInstrumentName() const;
	bool useFileNameAsComponentName() const;
	bool useVelocityAdjustment() const;

   private slots:
	void on_cancelBTN_clicked();
	void on_openBTN_clicked();
	void clicked( const QModelIndex& index );
	void doubleClicked( const QModelIndex& index );
	void on_m_pPlayBtn_clicked();
	void updateModelIndex();
	void on_m_pPathHometoolButton_clicked();
	void on_m_pPathUptoolButton_clicked();
	void on_playSamplescheckBox_clicked();
	void on_hiddenCB_clicked();

	virtual void keyPressEvent( QKeyEvent* ev ) override;
	virtual void keyReleaseEvent( QKeyEvent* ev ) override;

   private:
	enum class Playback { Rolling, Stopped };

	void startPlayback();
	void stopPlayback();
	/** We rely on the realtime frame of the audio engine. This means neither
	 * count in nor playback (of the audio engine) itself must be started during
	 * rendering of a sample. But this should not be a big problem since the
	 * whole #SampleEditor is a modal and the buttons for starting playback are
	 * not accessible. */
	void updateTransport();

	void browseTree( const QModelIndex& index );

	void getEnvironment();
	bool isFileSupported( const QString& sFileName );

	void updateIcons();

	Playback m_playback;

	QString m_sSampleFileName;
	QStringList m_selectedFiles;
	QString m_sSelectedDirectory;

	bool m_bSingleClick;
	QFileSystemModel* m_pDirModel;

	QModelIndex m_ModelIndex;

	QString m_sEmptySampleFileName;
	QStringList m_filters;

	bool m_bAllowMultiSelect;
	bool m_bShowInstrumentManipulationControls;

	QString m_sFileName;

	std::shared_ptr<H2Core::Instrument> m_pPreviewInstrument;
	std::shared_ptr<H2Core::Sample> m_pSample;

	float m_fPlayheadPos;
	long long m_nRealtimeFrameEnd;
	long long m_nLastRealtimeFrame;
	long long m_nSampleLength;

	/** Updates the playhead within the wave displays while a sample is playing.
	 */
	QTimer* m_pPlayheadUpdateTimer;
};

#endif
