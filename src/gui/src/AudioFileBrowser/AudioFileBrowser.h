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

#ifndef AUDIOFILEBROWSER_H
#define AUDIOFILEBROWSER_H

#include "ui_AudioFileBrowser_UI.h"
#include "InstrumentEditor/InstrumentEditor.h"

#include <QDialog>
#include <core/Object.h>
#include <core/Preferences.h>


class Button;
class SampleWaveDisplay;

///
/// This dialog is used to preview audiofiles
///
class AudioFileBrowser : public QDialog, public Ui_AudioFileBrowser_UI, public H2Core::Object

{
	H2_OBJECT
	Q_OBJECT
	public:
		
		AudioFileBrowser( QWidget* pParent, bool bAllowMultiSelect, bool bShowInstrumentManipulationControls);
		~AudioFileBrowser();
	
		QStringList getSelectedFiles();
		QString		setDir( QString dir );

	private slots:
		void on_cancelBTN_clicked();
		void on_openBTN_clicked();
		void clicked( const QModelIndex& index );
		void doubleClicked( const QModelIndex& index );
		void on_m_pPlayBtn_clicked();
		void on_m_pStopBtn_clicked();
		void updateModelIndex();
		void on_m_pPathHometoolButton_clicked();
		void on_m_pPathUptoolButton_clicked();
		void on_playSamplescheckBox_clicked();
		void on_hiddenCB_clicked();

		virtual void keyPressEvent (QKeyEvent *ev);
		virtual void keyReleaseEvent (QKeyEvent *ev);


	private:
		void browseTree( const QModelIndex& index );

		void getEnvironment();
		bool isFileSupported( QString filename );
		
		InstrumentEditor*	m_pInstrumentEditor;
		SampleWaveDisplay *	m_pSampleWaveDisplay;
		
		QString				m_pSampleFilename;
		QStringList			m_pSelectedFile;

		bool				m_SingleClick;
		QDirModel *			m_pDirModel;
		QTreeView *			m_pTree;

		QModelIndex			m_ModelIndex;
		
		QString				m_sEmptySampleFilename;
		QStringList			m_Filters;
		
		bool				m_bAllowMultiSelect;
		bool				m_bShowInstrumentManipulationControls;


};


#endif
