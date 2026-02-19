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


#include <QtGui>
#include <QtWidgets>
#include <memory>

#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {
class Instrument;
class InstrumentComponent;
class InstrumentLayer;
}  // namespace H2Core

class DetailWaveDisplay;
class MainSampleWaveDisplay;
class TargetWaveDisplay;

///
/// This dialog is used to preview audiofiles
///
/** \ingroup docGUI*/
class SampleEditor : public QDialog, public H2Core::Object<SampleEditor> {
	H2_OBJECT( SampleEditor )
	Q_OBJECT
   public:
	static constexpr int nHeight = 510;
	static constexpr int nWidth = 863;

	enum class Slider { None, Start, Loop, End };
	static QString SliderToQString( const Slider& slider );

	SampleEditor(
		QWidget* pParent,
		std::shared_ptr<H2Core::InstrumentLayer> pLayer,
		std::shared_ptr<H2Core::InstrumentComponent> pComponent,
		std::shared_ptr<H2Core::Instrument> pInstrument
	);
	~SampleEditor();

	int getFramePosition() const;
	float getZoomFactor() const;
	Slider getSelectedSlider() const;

	int getEnvelopeIndex() const;
	void setSampleName( const QString& name );
	bool getCloseQuestion();
	bool returnAllMainWaveDisplayValues();
	void returnAllTargetDisplayValues();
	void setUnclean();
	void setClean();

	// this values come from the real sample to restore a frm song loaded sample
	bool m_bSampleIsModified;  ///< true if sample is modified

   private slots:
	void valueChangedLoopCountSpinBox( int );
	void valueChangedProcessingTypeComboBox( int );
	void valueChangedrubberComboBox( int );
	void valueChangedrubberbandCsettingscomboBox( int );
	void valueChangedpitchdoubleSpinBox( double );
	void valueChangedStartFrameSpinBox( int );
	void valueChangedLoopFrameSpinBox( int );
	void valueChangedEndFrameSpinBox( int );
	void on_PlayPushButton_clicked();
	void on_PlayOrigPushButton_clicked();
	void updateMainsamplePositionRuler();
	void updateTargetsamplePositionRuler();

   private:
	void updateWaveDisplays();
	void getAllFrameInfos();
	void getAllLocalFrameInfos();
	void setAllSampleProps();
	void testPositionsSpinBoxes();
	void createNewLayer();
	void setSamplelengthFrames();
	void createPositionsRulerPath();
	void testpTimer();
	void checkRatioSettings();

	virtual void closeEvent( QCloseEvent* event ) override;
	virtual void mouseReleaseEvent( QMouseEvent* ev ) override;

	MainSampleWaveDisplay* m_pMainSampleWaveDisplay;
	DetailWaveDisplay* m_pDetailWaveDisplayL;
	DetailWaveDisplay* m_pDetailWaveDisplayR;

	QSpinBox* m_pStartFrameSpinBox;
	QSpinBox* m_pLoopFrameSpinBox;
	QSpinBox* m_pLoopCountSpinBox;
	QComboBox* m_pLoopModeComboBox;
	QSpinBox* m_pEndFrameSpinBox;

	QComboBox* m_pRubberBandLengthComboBox;
	QLabel* m_pRubberBandRatioLabel;
	QDoubleSpinBox* m_pRubberBandPitchSpinBox;
	QComboBox* m_pRubberBandCrispnessComboBox;

	QPushButton* m_pApplyButton;
	QPushButton* m_pPlayButton;
	QPushButton* m_pPlayOriginalButton;
	QLabel* m_pNewLengthLabel;
	QComboBox* m_pEnvelopeComboBox;

	TargetWaveDisplay* m_pTargetSampleView;

	std::shared_ptr<H2Core::InstrumentLayer> m_pLayer;
	std::shared_ptr<H2Core::InstrumentComponent> m_pComponent;
	std::shared_ptr<H2Core::Instrument> m_pInstrument;
	std::shared_ptr<H2Core::Sample> m_pSample;

	double m_fDivider;

	float m_fZoomfactor;
	int m_nFramePosition;
	Slider m_selectedSlider;

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
	QTimer* m_pTimer;
	QTimer* m_pTargetDisplayTimer;
	unsigned* m_pPositionsRulerPath;
	float m_fRatio;
	H2Core::Sample::Loops __loops;
	H2Core::Sample::Rubberband __rubberband;
};

inline int SampleEditor::getFramePosition() const
{
	return m_nFramePosition;
}
inline float SampleEditor::getZoomFactor() const
{
	return m_fZoomfactor;
}
inline SampleEditor::Slider SampleEditor::getSelectedSlider() const
{
	return m_selectedSlider;
}

#endif
