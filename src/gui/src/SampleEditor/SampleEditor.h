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

#include "../EventListener.h"
#include "../Widgets/EditorDefs.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/Basics/Sample.h>
#include <core/Basics/Song.h>
#include <core/Object.h>
#include <core/Preferences/Preferences.h>

namespace H2Core {
class Instrument;
class InstrumentComponent;
class InstrumentLayer;
}  // namespace H2Core

class Button;
class DetailWaveDisplay;
class LCDDisplay;
class LCDCombo;
class LCDSpinBox;
class SampleWaveDisplay;
class TargetSection;

///
/// This dialog is used to preview audiofiles
///
/** \ingroup docGUI*/
class SampleEditor : public QDialog,
					 public EventListener,
					 public H2Core::Object<SampleEditor> {
	H2_OBJECT( SampleEditor )
	Q_OBJECT
   public:
	static constexpr int nButtonWidth = 120;
	static constexpr int nButtonHeight = 24;
	/** All overlays, like envelopes or sliders, will be map slightly
	 * transparent. */
	static constexpr int nColorAlpha = 200;
	static constexpr int nHeight = 657;
	static constexpr int nWidth = 863;
	/** Visual separation between two label-widget pairs. */
	static constexpr int nSpacerWidth = 24;
	static constexpr int nSpacerHeight = 20;
	static constexpr int nSpacing = 6;
	/** Equivalent to 50 fps.*/
	static constexpr int nWaveDisplayUpdateInterval = 20;
	static constexpr int nSampleUpdateTimeout = 20;

	enum class Slider { None, Start, Loop, End };
	static QString SliderToQString( const Slider& slider );

	enum class EnvelopeType { Velocity, Pan };
	static QString EnvelopeTypeToQString( const EnvelopeType& envelopeType );

	SampleEditor(
		QWidget* pParent,
		std::shared_ptr<H2Core::InstrumentLayer> pLayer,
		std::shared_ptr<H2Core::InstrumentComponent> pComponent,
		std::shared_ptr<H2Core::Instrument> pInstrument
	);
	~SampleEditor();

	long long getPlayheadMain() const;
	long long getPlayheadTarget() const;
	long long getTotalPlaybackFrames() const;
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
	void setLoops( H2Core::Sample::Loops newLoops );
	void setRubberband( H2Core::Sample::Rubberband newRubberband );

	EnvelopeType getEnvelopeType() const;
	const std::vector<H2Core::EnvelopePoint>& getCurrentEnvelope() const;
	const H2Core::Sample::PanEnvelope& getPanEnvelope() const;
	const H2Core::Sample::VelocityEnvelope& getVelocityEnvelope() const;
	void editEnvelopePoint(
		H2Core::EnvelopePoint point,
		SampleEditor::EnvelopeType envelopeType,
		Editor::Action action
	);
	void moveEnvelopePoint(
		H2Core::EnvelopePoint oldPoint,
		H2Core::EnvelopePoint newPoint,
		SampleEditor::EnvelopeType envelopeType
	);

	/** EventListener interface */
	void drumkitLoadedEvent() override;

   private:
	enum class Playback { None, Target, Original };

	void startPlayback( Playback playback );
	void stopPlayback();
	/** We rely on the realtime frame of the audio engine. This means neither
	 * count in nor playback (of the audio engine) itself must be started during
	 * rendering of a sample. But this should not be a big problem since the
	 * whole #SampleEditor is a modal and the buttons for starting playback are
	 * not accessible. */
	void updateTransport();

	void lockWidgets( bool bLock );
	void setUnclean();
	void setClean();
	void updateSourceWaveDisplays();
	void triggerSampleUpdate();
	void updateSample();
	void reloadLayer();
	void checkRubberbandSettings();
	/** Since this dialog is a modal, we do not have to care about updating the
	 * style sheet in case the theme is changed in the preferences dialog. */
	void updateStyleSheet();

	void closeEvent( QCloseEvent* event ) override;
	void keyPressEvent( QKeyEvent* e ) override;

	SampleWaveDisplay* m_pSampleWaveDisplayL;
	SampleWaveDisplay* m_pSampleWaveDisplayR;
	DetailWaveDisplay* m_pDetailWaveDisplayL;
	DetailWaveDisplay* m_pDetailWaveDisplayR;

	LCDSpinBox* m_pLoopStartFrameSpinBox;
	LCDSpinBox* m_pLoopLoopFrameSpinBox;
	LCDSpinBox* m_pLoopCountSpinBox;
	LCDCombo* m_pLoopModeComboBox;
	LCDSpinBox* m_pLoopEndFrameSpinBox;

	LCDCombo* m_pRubberBandLengthComboBox;
	QLabel* m_pRubberBandRatioLabel;
	LCDSpinBox* m_pRubberBandPitchSpinBox;
	LCDCombo* m_pRubberBandCrispnessComboBox;

	Button* m_pApplyButton;
	Button* m_pPlayButton;
	Button* m_pPlayOriginalButton;
	LCDDisplay* m_pNewLengthDisplay;
	QComboBox* m_pEnvelopeComboBox;

	TargetSection* m_pTargetSection;

	/** Original instrument used to write back changes.
	 *
	 * @{ */
	std::shared_ptr<H2Core::Instrument> m_pInstrument;
	std::shared_ptr<H2Core::InstrumentComponent> m_pComponent;
	std::shared_ptr<H2Core::InstrumentLayer> m_pLayer;
	/** @} */

	/** Contains the latest user changes (modulo #m_pSampleUpdateTimer)
	 * _without_ the need to apply them. */
	std::shared_ptr<H2Core::Sample> m_pSample;
	/** Unmodified version loaded from disk */
	std::shared_ptr<H2Core::Sample> m_pSampleOriginal;
	/** Instrument used for playback. This way none of the
	 * instrument/component/layer settings leaks into the SampleEditor. */
	std::shared_ptr<H2Core::Instrument> m_pPreviewInstrument;
	std::shared_ptr<H2Core::Instrument> m_pPreviewInstrumentOriginal;

	Playback m_playback;
	/** Since we rely on the real-time frames when rendering samples, we
	 * have to check whether they have been reset within the audio engine.
	 * This will occur on state changes. */
	H2Core::AudioEngine::State m_previousState;
	long long m_nPlayheadSample;
	long long m_nPlayheadTarget;
	long long m_nRealtimeFrameEnd;
	/** Cache all all things required to efficiently calculate the playhead
	 * position in the main section.
	 *
	 * @{*/
	enum class Looped { NotYet, Forward, Reverse };
	long long m_nPreLoopFrames;
	long long m_nLoopFrames;
	long long m_nLastRealtimeFrame;
	long long m_nTotalPlaybackFrames;
	Looped m_looped;
	bool m_bLastLoopForward;
	/** @} */

	float m_fZoomfactor;
	Slider m_hoveredSlider;
	Slider m_selectedSlider;

	bool m_bPlayButton;
	bool m_bSampleEditorClean;

	/** Updates the playhead within the wave displays while a sample is playing.
	 */
	QTimer* m_pWaveDisplayUpdateTimer;
	/** We provide an almost immediate update of the target sample. But in order
	 * to not create too much unnecessary load when the user is doing a lot of
	 * changes at once, we wait till there are no changes for at last
	 * #SampleUpdateTimeout ms before performing the update. */
	QTimer* m_pSampleUpdateTimer;
	bool m_bRetriggerRequired;
	bool m_bLayerReloadRequired;
	double m_fIncrementScaling;

	H2Core::Sample::Loops m_loops;
	H2Core::Sample::Rubberband m_rubberband;

	EnvelopeType m_envelopeType;
	H2Core::Sample::PanEnvelope m_panEnvelope;
	H2Core::Sample::VelocityEnvelope m_velocityEnvelope;
};

inline long long SampleEditor::getPlayheadMain() const
{
	return m_nPlayheadSample;
}
inline long long SampleEditor::getPlayheadTarget() const
{
	return m_nPlayheadTarget;
}
inline long long SampleEditor::getTotalPlaybackFrames() const
{
	return m_nTotalPlaybackFrames;
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
inline SampleEditor::EnvelopeType SampleEditor::getEnvelopeType() const
{
	return m_envelopeType;
}
inline const std::vector<H2Core::EnvelopePoint>&
SampleEditor::getCurrentEnvelope() const
{
	if ( m_envelopeType == EnvelopeType::Velocity ) {
		return m_velocityEnvelope;
	}
	else {
		return m_panEnvelope;
	}
}
inline const H2Core::Sample::PanEnvelope& SampleEditor::getPanEnvelope() const
{
	return m_panEnvelope;
}
inline const H2Core::Sample::VelocityEnvelope&
SampleEditor::getVelocityEnvelope() const
{
	return m_velocityEnvelope;
}
#endif
