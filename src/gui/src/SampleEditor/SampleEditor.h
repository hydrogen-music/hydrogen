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

#ifndef SAMPLEEDITOR_H
#define SAMPLEEDITOR_H

#include "ui_SampleEditor_UI.h"
#include "../InstrumentEditor/InstrumentEditor.h"

#include <QDialog>
#include <QProcess>
#include <hydrogen/object.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/basics/instrument.h>


class Button;
class 	MainSampleWaveDisplay;
class	TargetWaveDisplay;
class	DetailWaveDisplay;

///
/// This dialog is used to preview audiofiles
///
class SampleEditor : public QDialog, public Ui_SampleEditor_UI, public H2Core::Object
{
	H2_OBJECT
	Q_OBJECT
	public:
		
		SampleEditor( QWidget* pParent, int nSelectedComponent, int nSelectedLayer, QString nSampleFilename );
		~SampleEditor();

		void setSampleName( QString name);
		bool getCloseQuestion();
		bool m_pSampleEditorStatus;
		bool returnAllMainWaveDisplayValues();
		void returnAllTargetDisplayValues();
		void setTrue();

		//this values come from the real sample to restore a frm song loaded sample
		bool m_sample_is_modified;	///< true if sample is modified

	private slots:
		void valueChangedLoopCountSpinBox( int );
		void valueChangedProcessingTypeComboBox( const QString );
		void valueChangedrubberComboBox( const QString  );
		void valueChangedrubberbandCsettingscomboBox( const QString );
		void valueChangedpitchdoubleSpinBox( double );
		void on_ClosePushButton_clicked();
		void on_reloadToolButton_clicked();
		void on_externalEditorPushButton_clicked();
		void on_PrevChangesPushButton_clicked();
		void valueChangedStartFrameSpinBox( int );
		void valueChangedLoopFrameSpinBox( int );
		void valueChangedEndFrameSpinBox( int );
		void on_PlayPushButton_clicked();
		void on_PlayOrigPushButton_clicked();
		void on_verticalzoomSlider_valueChanged ( int value );
		void updateMainsamplePositionRuler();
		void updateTargetsamplePositionRuler();



	private:

		H2Core::Sample *m_pSampleFromFile;
		int m_pSelectedLayer;
		int m_pSelectedComponent;
		QString m_samplename;
	
		double m_divider;

		void openDisplays();
		void getAllFrameInfos();
		void getAllLocalFrameInfos();
		void setAllSampleProps();
		void testPositionsSpinBoxes();
		void createNewLayer();
		void setSamplelengthFrames();
		void createPositionsRulerPath();
		void testpTimer();
		void closeEvent(QCloseEvent *event);
		void checkRatioSettings();

		virtual void mouseReleaseEvent(QMouseEvent *ev);
	
		MainSampleWaveDisplay *m_pMainSampleWaveDisplay;
		TargetWaveDisplay *m_pTargetSampleView;
		DetailWaveDisplay *m_pSampleAdjustView; 

		float m_pZoomfactor;
		unsigned m_pDetailFrame;
		QString m_pLineColor;
		bool m_pOnewayStart;
		bool m_pOnewayLoop;
		bool m_pOnewayEnd;
		unsigned long m_pRealtimeFrameEnd;
		unsigned long m_prealtimeframeendfortarget;
		unsigned m_pslframes;
		unsigned m_pSamplerate;
		QTimer *m_pTimer;
		QTimer *m_pTargetDisplayTimer;
		unsigned *m_pPositionsRulerPath;
		bool m_pPlayButton;
		float m_pRatio;
		QString m_externalEditor;
		H2Core::Sample::Loops __loops;
		H2Core::Sample::Rubberband __rubberband;
		
};


#endif
