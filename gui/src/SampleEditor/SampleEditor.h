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

#include "config.h"
#include "ui_SampleEditor_UI.h"
#include "../InstrumentEditor/InstrumentEditor.h"

#include <QDialog>
#include <hydrogen/Object.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/Song.h>
#include <hydrogen/sample.h>
#include <hydrogen/instrument.h>


class Button;
class 	MainSampleWaveDisplay;
class	TargetWaveDisplay;
class	DetailWaveDisplay;

///
/// This dialog is used to preview audiofiles
///
class SampleEditor : public QDialog, public Ui_SampleEditor_UI, public Object

{
	Q_OBJECT
	public:
		
		SampleEditor( QWidget* pParent, int nSelectedLayer, QString nSampleFilename );
		~SampleEditor();

		void setSampleName( QString name);
		bool getCloseQuestion();
		bool m_pSampleEditorStatus;
		bool returnAllMainWaveDisplayValues();
		void returnAllTargetDisplayValues();
		void setTrue();

		//this values come from the real sample to restore a frm song loaded sample
		bool m_sample_is_modified;	///< true if sample is modified
		QString m_sample_mode;		///< loop mode
		int m_repeats;			///< repats from the loop section
		unsigned m_start_frame;		///< start frame
		unsigned m_loop_frame;		///< beginn of the loop section
		unsigned m_end_frame; 		///< sample end frame


	private slots:
		void valueChangedLoopCountSpinBox( int );
		void valueChangedProcessingTypeComboBox( const QString );
		void valueChangedrubberComboBox( const QString  );
		void valueChangedrubberbandCsettingscomboBox( const QString );
		void valueChangedpitchdoubleSpinBox( double );
		void on_ClosePushButton_clicked();
		void on_PrevChangesPushButton_clicked();
		void valueChangedStartFrameSpinBox( int );
		void valueChangedLoopFrameSpinBox( int );
		void valueChangedEndFrameSpinBox( int );
		void on_PlayPushButton_clicked();
		void on_PlayOrigPushButton_clicked();
		void on_verticalzoomSlider_valueChanged ( int value );
		void updateMainsamplePostionRuler();
		void updateTargetsamplePostionRuler();



	private:

		H2Core::Sample *m_pSamplefromFile;
		int m_pSelectedLayer;
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

		float m_pzoomfactor;
		unsigned m_pdetailframe;
		QString m_plineColor;
		bool m_ponewayStart;
		bool m_ponewayLoop;
		bool m_ponewayEnd;
		unsigned long m_prealtimeframeend;
		unsigned long m_prealtimeframeendfortarget;
		unsigned m_pslframes;
		unsigned m_pSamplerate;
		QTimer *m_pTimer;
		QTimer *m_pTargetDisplayTimer;
		unsigned *m_pPositionsRulerPath;
		bool m_pPlayButton;
		bool m_pUseRubber;
		float m_pRubberDivider;
		float m_pratio;
		float m_ppitch;
		int m_pRubberbandCsettings;

		
};


#endif
