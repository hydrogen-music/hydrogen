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


#ifndef EXPORT_SONG_DIALOG_H
#define EXPORT_SONG_DIALOG_H


#include "ui_ExportSongDialog_UI.h"
#include "EventListener.h"
#include <core/Object.h>
#include <core/Sampler/Sampler.h>

using InterpolateMode = H2Core::Interpolation::InterpolateMode;

namespace H2Core {
	class Instrument;
	class Hydrogen;
	class Preferences;
}

///
/// Dialog for exporting song
///
class ExportSongDialog : public QDialog, public Ui_ExportSongDialog_UI, public EventListener, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT

	public:
		explicit ExportSongDialog(QWidget* parent);
		~ExportSongDialog();

		virtual void progressEvent( int nValue ) override;
		void closeEvent( QCloseEvent* event ) override;


private slots:
	void		on_browseBtn_clicked();
	void		on_closeBtn_clicked();
	void		on_okBtn_clicked();
	void		on_exportNameTxt_textChanged(const QString& text);
	void		on_templateCombo_currentIndexChanged(int index);
	void		toggleRubberbandBatchMode(bool toggled);
	void		toggleTimeLineBPMMode(bool toggled);
	void		resampleComboBoIndexChanged(int index);

private:

	void		setResamplerMode(int index);
	void		calculateRubberbandTime();
	bool		checkUseOfRubberband();
	
	void		saveSettingsToPreferences();
	void		restoreSettingsFromPreferences();
	
	bool		currentInstrumentHasNotes();
	QString		findUniqueExportFilenameForInstrument(H2Core::Instrument* pInstrument);

	void		exportTracks();
	bool 		validateUserInput();
	QString		createDefaultFilename();

	void		closeExport();
	
	bool					m_bExporting;
	bool					m_bExportTrackouts;
	bool					m_bOverwriteFiles;
	uint					m_nInstrument;
	QString					m_sExtension;
	bool					m_bOldRubberbandBatchMode;
	bool					m_bOldTimeLineBPMMode;
	InterpolateMode			m_OldInterpolationMode;
	bool					m_bQfileDialog;
	H2Core::Hydrogen *		m_pEngine;
	H2Core::Preferences*	m_pPreferences;
	
	static QString 			sLastFilename;
};


#endif

