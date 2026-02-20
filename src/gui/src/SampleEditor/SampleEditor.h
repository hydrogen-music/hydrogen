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
#include "Widgets/LCDDisplay.h"

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
class LCDDisplay;
class LCDCombo;
class LCDSpinBox;
class SampleWaveDisplay;
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

	enum class Envelope { Velocity, Pan };
	static QString EnvelopeToQString( const Envelope& envelope );

	SampleEditor(
		QWidget* pParent,
		std::shared_ptr<H2Core::InstrumentLayer> pLayer,
		std::shared_ptr<H2Core::InstrumentComponent> pComponent,
		std::shared_ptr<H2Core::Instrument> pInstrument
	);
	~SampleEditor();

	long long getFramePosition() const;
	float getZoomFactor() const;
	Slider getHoveredSlider() const;
	void setHoveredSlider( Slider slider );
	Slider getSelectedSlider() const;
	void setSelectedSlider( Slider slider );

	int getLoopStartFrame() const;
	void setLoopStartFrame( int nFrame );
	int getLoopLoopFrame() const;
	void setLoopLoopFrame( int nFrame );
	int getLoopEndFrame() const;
	void setLoopEndFrame( int nFrame );

	Envelope getEnvelope() const;
	void setSampleName( const QString& name );
	bool getCloseQuestion();
	void returnAllTargetDisplayValues();
	void setUnclean();
	void setClean();

	// this values come from the real sample to restore a frm song loaded sample
	bool m_bSampleIsModified;  ///< true if sample is modified

   private slots:
	void on_PlayPushButton_clicked();
	void on_PlayOrigPushButton_clicked();
	void updateMainsamplePositionRuler();
	void updateTargetsamplePositionRuler();

   private:
	void updateSourceWaveDisplays();
	void getAllFrameInfos();
	void setAllSampleProps();
	void createNewLayer();
	void updateTargetFrames();
	void createPositionsRulerPath();
	void testpTimer();
	void checkRubberbandSettings();

	virtual void closeEvent( QCloseEvent* event ) override;

	SampleWaveDisplay* m_pSampleWaveDisplayL;
	SampleWaveDisplay* m_pSampleWaveDisplayR;
	DetailWaveDisplay* m_pDetailWaveDisplayL;
	DetailWaveDisplay* m_pDetailWaveDisplayR;

	LCDSpinBox* m_pLoopStartFrameSpinBox;
	LCDSpinBox* m_pLoopLoopFrameSpinBox;
	LCDSpinBox* m_pLoopCountSpinBox;
	QComboBox* m_pLoopModeComboBox;
	LCDSpinBox* m_pLoopEndFrameSpinBox;

	QComboBox* m_pRubberBandLengthComboBox;
	QLabel* m_pRubberBandRatioLabel;
	LCDSpinBox* m_pRubberBandPitchSpinBox;
	LCDCombo* m_pRubberBandCrispnessComboBox;

	QPushButton* m_pApplyButton;
	QPushButton* m_pPlayButton;
	QPushButton* m_pPlayOriginalButton;
	LCDDisplay* m_pNewLengthDisplay;
	QComboBox* m_pEnvelopeComboBox;

	TargetWaveDisplay* m_pTargetSampleView;

	std::shared_ptr<H2Core::InstrumentLayer> m_pLayer;
	std::shared_ptr<H2Core::InstrumentComponent> m_pComponent;
	std::shared_ptr<H2Core::Instrument> m_pInstrument;
	std::shared_ptr<H2Core::Sample> m_pSample;

	float m_fZoomfactor;
	long long m_nFramePosition;
	Slider m_hoveredSlider;
	Slider m_selectedSlider;

	bool m_bPlayButton;
	bool m_bSampleEditorClean;

	/** Number of frames the resulting sample will have after applying both loop
	 * and rubberband settings. */
	long long m_nTargetFrames;
	long long m_nRealtimeFrameEnd;
	long long m_nRealtimeFrameEndForTarget;
	unsigned m_nSamplerate;
	QTimer* m_pTimer;
	QTimer* m_pTargetDisplayTimer;
	long long* m_pPositionsRulerPath;
	float m_fRatio;

	Envelope m_envelope;
	H2Core::Sample::Loops m_loops;
	H2Core::Sample::Rubberband m_rubberband;
};

inline long long SampleEditor::getFramePosition() const
{
	return m_nFramePosition;
}
inline float SampleEditor::getZoomFactor() const
{
	return m_fZoomfactor;
}
inline SampleEditor::Slider SampleEditor::getHoveredSlider() const
{
	return m_hoveredSlider;
}
inline SampleEditor::Slider SampleEditor::getSelectedSlider() const
{
	return m_selectedSlider;
}
inline int SampleEditor::getLoopStartFrame() const
{
	return m_loops.nStartFrame;
}
inline int SampleEditor::getLoopLoopFrame() const
{
	return m_loops.nLoopFrame;
}
inline int SampleEditor::getLoopEndFrame() const
{
	return m_loops.nEndFrame;
}
inline SampleEditor::Envelope SampleEditor::getEnvelope() const
{
	return m_envelope;
}
#endif
