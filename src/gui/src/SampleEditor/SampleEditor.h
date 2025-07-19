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

#ifndef SAMPLEEDITOR_H
#define SAMPLEEDITOR_H

#include "ui_SampleEditor_UI.h"
#include "../InstrumentEditor/InstrumentEditor.h"

#include <QDialog>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>
#include <core/Basics/Song.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Instrument.h>


class 	MainSampleWaveDisplay;
class	TargetWaveDisplay;
class	DetailWaveDisplay;

///
/// This dialog is used to preview audiofiles
///
/** \ingroup docGUI*/
class SampleEditor :  public QDialog, public Ui_SampleEditor_UI,  public H2Core::Object<SampleEditor>
{
	H2_OBJECT(SampleEditor)
	Q_OBJECT
	public:
		
		SampleEditor( QWidget* pParent, int nSelectedComponent,
					  int nSelectedLayer, const QString& nSampleFilename );
		~SampleEditor();

		void setSampleName( const QString& name );
		bool getCloseQuestion();
		bool returnAllMainWaveDisplayValues();
		void returnAllTargetDisplayValues();
		void setUnclean();
		void setClean();

		//this values come from the real sample to restore a frm song loaded sample
		bool m_bSampleIsModified;	///< true if sample is modified

	private slots:
		void valueChangedLoopCountSpinBox( int );
		void valueChangedProcessingTypeComboBox( int );
		void valueChangedrubberComboBox( int );
		void valueChangedrubberbandCsettingscomboBox( int );
		void valueChangedpitchdoubleSpinBox( double );
		void on_ClosePushButton_clicked();
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

	std::shared_ptr<H2Core::Sample> retrieveSample() const;

		void openDisplays();
		void getAllFrameInfos();
		void getAllLocalFrameInfos();
		void setAllSampleProps();
		void testPositionsSpinBoxes();
		void createNewLayer();
		void setSamplelengthFrames();
		void createPositionsRulerPath();
		void testpTimer();
		void checkRatioSettings();

		virtual void closeEvent(QCloseEvent *event) override;
		virtual void mouseReleaseEvent(QMouseEvent *ev) override;
	
		MainSampleWaveDisplay *m_pMainSampleWaveDisplay;
		TargetWaveDisplay *m_pTargetSampleView;
		DetailWaveDisplay *m_pSampleAdjustView;
	
		std::shared_ptr<H2Core::Sample> m_pSampleFromFile;
		int m_nSelectedLayer;
		int m_nSelectedComponent;
		QString m_sSampleName;
	
		double m_divider;
		float m_fZoomfactor;
		unsigned m_pDetailFrame;
		QString m_sLineColor;

		bool m_bOnewayStart;
		bool m_bOnewayLoop;
		bool m_bOnewayEnd;
		bool m_bPlayButton;
		bool m_bAdjusting;
		bool m_bSampleEditorClean;
		
		unsigned long m_nRealtimeFrameEnd;
		unsigned long m_nRealtimeFrameEndForTarget;
		unsigned m_nSlframes;
		unsigned m_nSamplerate;
		QTimer *m_pTimer;
		QTimer *m_pTargetDisplayTimer;
		unsigned *m_pPositionsRulerPath;
		float m_fRatio;
		H2Core::Sample::Loops __loops;
		H2Core::Sample::Rubberband __rubberband;
};


#endif
