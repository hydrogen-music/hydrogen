/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
 * Copyright(c) 2008-2026 The hydrogen development team [hydrogen-devel@lists.sourceforge.net]
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


#ifndef EXPORT_SONG_DIALOG_H
#define EXPORT_SONG_DIALOG_H

#include <memory>
#include <map>

#include "ui_ExportSongDialog_UI.h"
#include "EventListener.h"
#include <core/Helpers/Filesystem.h>
#include <core/Object.h>
#include <core/Sampler/Sampler.h>

using InterpolateMode = H2Core::Interpolation::InterpolateMode;

namespace H2Core {
	class Instrument;
}

///
/// Dialog for exporting song
///
/** \ingroup docGUI*/
class ExportSongDialog :  public QDialog, public Ui_ExportSongDialog_UI, public EventListener,  public H2Core::Object<ExportSongDialog>
{
	H2_OBJECT(ExportSongDialog)
	Q_OBJECT

	public:
		explicit ExportSongDialog(QWidget* parent);
		~ExportSongDialog();

		virtual void audioExportProgressEvent( int nValue ) override;
		void closeEvent( QCloseEvent* event ) override;


private slots:
	void		on_browseBtn_clicked();
	void		on_closeBtn_clicked();
	void		on_okBtn_clicked();
	void		on_exportNameTxt_textChanged(const QString& text);
	void		formatComboIndexChanged(int index);
	void		toggleTimeLineBPMMode(bool toggled);

private:

	bool		checkUseOfRubberband();

	bool		instrumentHasNotes( int nInstrumentIndex );
	QString		findUniqueExportFileNameForInstrument( std::shared_ptr<H2Core::Instrument> pInstrument );

	bool 		validateUserInput();
	QString		createDefaultFileName();

	void		closeExport();
	
	bool					m_bExporting;
	bool					m_bOverwriteFiles;
	QString					m_sExtension;
	int						m_nOldRubberbandBatchMode;
	bool					m_bOldTimeLineBPMMode;
	bool					m_bQfileDialog;

	// Bookkeeping of the one-shot export plan (batch 2l): how many
	// files it holds and how many of them reported completion.
	int						m_nPlanDone;
	int						m_nPlanTotal;

		std::map<int, H2Core::Filesystem::AudioFormat> m_formatMap;
	static QString 			sLastFileName;
};


#endif

