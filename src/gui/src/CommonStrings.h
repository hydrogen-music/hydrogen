/*
 * Hydrogen
 * Copyright (C) 2021 The hydrogen development team <hydrogen-devel@lists.sourceforge.net>
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

#ifndef COMMON_STRINGS_H
#define COMMON_STRINGS_H

#include <QString>
#include <core/Object.h>

/** A container class to collect all translatable strings at one place
	in order to allow for the reusage. This will (some day) make
	translation work faster.

	Initially I wanted the whole thing as well as the getter methods
	to be static. But Qt requires the class to be initialized for the
	translation engine to take effect.
	
	\ingroup docGUI
*/
class CommonStrings : public H2Core::Object<CommonStrings> {
	H2_OBJECT(CommonStrings)
	Q_DECLARE_TR_FUNCTIONS(CommonStrings)
	
	public:
	CommonStrings();
	~CommonStrings();
 
	const QString& getSmallSoloButton() const { return m_sSmallSoloButton; }
	const QString& getSmallMuteButton() const { return m_sSmallMuteButton; }
	const QString& getBigMuteButton() const { return m_sBigMuteButton; }
	const QString& getBigSoloButton() const { return m_sBigSoloButton; }
	const QString& getBypassButton() const { return m_sBypassButton; }
	const QString& getEditButton() const { return m_sEditButton; }
	const QString& getClearButton() const { return m_sClearButton; }
	const QString& getPlaybackTrackButton() const { return m_sPlaybackTrackButton; }
	const QString& getTimelineButton() const { return m_sTimelineButton; }
	const QString& getTimelineBigButton() const { return m_sTimelineBigButton; }
	const QString& getFXButton() const { return m_sFXButton; }
	const QString& getPeakButton() const { return m_sPeakButton; }
	const QString& getGeneralButton() const { return m_sGeneralButton; }
	const QString& getInstrumentButton() const { return m_sInstrumentButton; }
	const QString& getSoundLibraryButton() const { return m_sSoundLibraryButton; }
	const QString& getComponentsButton() const { return m_sComponentsButton; }
	const QString& getLoadLayerButton() const { return m_sLoadLayerButton; }
	const QString& getDeleteLayerButton() const { return m_sDeleteLayerButton; }
	const QString& getEditLayerButton() const { return m_sEditLayerButton; }
	const QString& getBeatCounterButton() const { return m_sBeatCounterButton; }
	const QString& getBeatCounterSetPlayButtonOff() const { return m_sBeatCounterSetPlayButtonOff; }
	const QString& getBeatCounterSetPlayButtonOn() const { return m_sBeatCounterSetPlayButtonOn; }
	const QString& getRubberbandButton() const { return m_sRubberbandButton; }
	const QString& getJackTransportButton() const { return m_sJackTransportButton; }
	const QString& getJackTimebaseButton() const { return m_sJackTimebaseButton; }
	const QString& getMixerButton() const { return m_sMixerButton; }
	const QString& getInstrumentRackButton() const { return m_sInstrumentRackButton; }
	const QString& getPatternModeButton() const { return m_sPatternModeButton; }
	const QString& getSongModeButton() const { return m_sSongModeButton; }
	const QString& getAttackLabel() const { return m_sAttackLabel; }
	const QString& getDecayLabel() const { return m_sDecayLabel; }
	const QString& getSustainLabel() const { return m_sSustainLabel; }
	const QString& getReleaseLabel() const { return m_sReleaseLabel; }
	const QString& getMidiOutChannelLabel() const { return  m_sMidiOutChannelLabel; }
	const QString& getMidiOutNoteLabel() const { return  m_sMidiOutNoteLabel; }
	const QString& getMidiOutLabel() const { return  m_sMidiOutLabel; }
	const QString& getPitchLabel() const { return  m_sPitchLabel; }
	const QString& getPitchCoarseLabel() const { return  m_sPitchCoarseLabel; }
	const QString& getPitchFineLabel() const { return  m_sPitchFineLabel; }
	const QString& getPitchRandomLabel() const { return  m_sPitchRandomLabel; }
	const QString& getGainLabel() const { return m_sGainLabel; }
	const QString& getMuteGroupLabel() const { return m_sMuteGroupLabel; }
	const QString& getIsStopNoteLabel() const { return m_sIsStopNoteLabel; }
	const QString& getApplyVelocityLabel() const { return m_sApplyVelocityLabel; }
	const QString& getHihatGroupLabel() const { return m_sHihatGroupLabel; }
	const QString& getHihatMaxRangeLabel() const { return m_sHihatMaxRangeLabel; }
	const QString& getHihatMinRangeLabel() const { return m_sHihatMinRangeLabel; }
	const QString& getCutoffLabel() const { return m_sCutoffLabel; }
	const QString& getResonanceLabel() const { return m_sResonanceLabel; }
	const QString& getLayerGainLabel() const { return m_sLayerGainLabel; }
	const QString& getComponentGainLabel() const { return m_sComponentGainLabel; }
	const QString& getSampleSelectionLabel() const { return m_sSampleSelectionLabel; }
	const QString& getPatternSizeLabel() const { return m_sPatternSizeLabel; }
	const QString& getResolutionLabel() const { return m_sResolutionLabel; }
	const QString& getHearNotesLabel() const { return m_sHearNotesLabel; }
	const QString& getQuantizeEventsLabel() const { return m_sQuantizeEventsLabel; }
	const QString& getShowPianoLabel() const { return m_sShowPianoLabel; }
	const QString& getMidiInLabel() const { return m_sMidiInLabel; }
	const QString& getCpuLabel() const { return m_sCpuLabel; }
	const QString& getBPMLabel() const { return m_sBPMLabel; }
	const QString& getTimeHoursLabel() const { return m_sTimeHoursLabel; }
	const QString& getTimeMinutesLabel() const { return m_sTimeMinutesLabel; }
	const QString& getTimeSecondsLabel() const { return m_sTimeSecondsLabel; }
	const QString& getTimeMilliSecondsLabel() const { return m_sTimeMilliSecondsLabel; }
	const QString& getHumanizeLabel() const { return m_sHumanizeLabel; }
	const QString& getSwingLabel() const { return m_sSwingLabel; }
	const QString& getTimingLabel() const { return m_sTimingLabel; }
	const QString& getVelocityLabel() const { return m_sVelocityLabel; }
	const QString& getMasterLabel() const { return m_sMasterLabel; }
	const QString& getReturnLabel() const { return m_sReturnLabel; }

	const QString& getRangeTooltip() const { return m_sRangeTooltip; }
	const QString& getMidiTooltipHeading() const { return m_sMidiTooltipHeading; }
	const QString& getMidiTooltipBound() const { return m_sMidiTooltipBound; }
	const QString& getMidiTooltipUnbound() const { return m_sMidiTooltipUnbound; }
	
	const QString& getShowDrumkitEditorTooltip() const { return m_sShowDrumkitEditorTooltip; }
	const QString& getShowPianoRollEditorTooltip() const { return m_sShowPianoRollEditorTooltip; }
	const QString& getPatternSizeDisabledTooltip() const { return m_sPatternSizeDisabledTooltip; }
	
	const QString& getAudioDriverStartError() const { return m_sAudioDriverStartError; }
	const QString& getAudioDriverErrorHint() const { return m_sAudioDriverErrorHint; }
	const QString& getAudioDriverNotPresent() const { return m_sAudioDriverNotPresent; }
	
	const QString& getJackTimebaseTooltip() const { return m_sJackTimebaseTooltip; }
	const QString& getJackTimebaseListenerTooltip() const { return m_sJackTimebaseListenerTooltip; }
	const QString& getJackTimebaseDisabledTooltip() const { return m_sJackTimebaseDisabledTooltip; }
	
	const QString& getMidiSenseWindowTitle() const { return m_sMidiSenseWindowTitle; }
	const QString& getMidiSenseInput() const { return m_sMidiSenseInput; }
	const QString& getMidiSenseUnavailable() const { return m_sMidiSenseUnavailable; }
	
	const QString& getPatternLoadError() const { return m_sPatternLoadError; }
	const QString& getInstrumentLoadError() const { return m_sInstrumentLoadError; }
	
	const QString& getFileDialogMissingWritePermissions() const { return m_sFileDialogMissingWritePermissions; }

	const QString& getStatusOn() const { return m_sStatusOn; }
	const QString& getStatusOff() const { return m_sStatusOff; }
	const QString& getStatusEnabled() const { return m_sStatusEnabled; }
	const QString& getStatusDisabled() const { return m_sStatusDisabled; }
		
	const QString& getTimelineEnabled() const { return m_sTimelineEnabled; }
	const QString& getTimelineDisabledPatternMode() const { return m_sTimelineDisabledPatternMode; }
	const QString& getTimelineDisabledTimebaseListener() const { return m_sTimelineDisabledTimebaseListener; }
	const QString& getPatternEditorLocked() const { return m_sPatternEditorLocked; }

	const QString& getPreferencesNotCompiled() const { return m_sPreferencesNotCompiled; }
	const QString& getPreferencesNone() const { return m_sPreferencesNone; }
	const QString& getPreferencesJackTooltip() const { return m_sPreferencesJackTooltip; }
	const QString& getPreferencesShortcutCapture() const { return m_sPreferencesShortcutCapture; }

	const QString& getButtonOk() const { return m_sButtonOk; }
	const QString& getButtonApply() const { return m_sButtonApply; }
	const QString& getButtonSave() const { return m_sButtonSave; }
	const QString& getButtonCancel() const { return m_sButtonCancel; }
	const QString& getButtonDiscard() const { return m_sButtonDiscard; }
	const QString& getButtonPlay() const { return m_sButtonPlay; }
	const QString& getButtonPlayOriginalSample() const { return m_sButtonPlayOriginalSample; }
	const QString& getUnsavedChanges() const { return m_sUnsavedChanges; }
	const QString& getSavingChanges() const { return m_sSavingChanges; }

	const QString& getMutableDialog() const { return m_sMutableDialog; }
	const QString& getTabGeneralDialog() const { return m_sTabGeneralDialog; }
	const QString& getTabLicensesDialog() const { return m_sTabLicensesDialog; }
	const QString& getNameDialog() const { return m_sNameDialog; }
	const QString& getVersionDialog() const { return m_sVersionDialog; }
	const QString& getLicenseDialog() const { return m_sLicenseDialog; }
	const QString& getAuthorDialog() const { return m_sAuthorDialog; }
	const QString& getNotesDialog() const { return m_sNotesDialog; }

	// const QString& getDialogSongLoadError() const { return m_sDialogSongLoadError; }
	// const QString& getDialogUnsavedChangesH1() const { return m_sDialogUnsavedChangedH1; }
	// const QString& getDialogUnsavedChangesH2() const { return m_sDialogUnsavedChangedH2; }

	const QString& getLicenseStringLbl() const { return m_sLicenseStringLbl; }
	const QString& getLicenseComboToolTip() const { return m_sLicenseComboToolTip; }
	const QString& getLicenseStringToolTip() const { return m_sLicenseStringToolTip; }
	
	const QString& getLicenseCopyleftWarning() const { return m_sLicenseCopyleftWarning; }
	const QString& getLicenseAttributionWarning() const { return m_sLicenseAttributionWarning; }
	const QString& getLicenseWarningWindowTitle() const { return m_sLicenseWarningWindowTitle; }
	const QString& getLicenseMismatchingUserInput() const { return m_sLicenseMismatchingUserInput; }

	const QString& getInputCaptureBpm() const { return m_sInputCaptureBpm; }
	const QString& getInputCaptureVolume() const { return m_sInputCaptureVolume; }
	const QString& getInputCaptureColumn() const { return m_sInputCaptureColumn; }
	const QString& getInputCapturePattern() const { return m_sInputCapturePattern; }
	const QString& getInputCaptureSong() const { return m_sInputCaptureSong; }
	const QString& getInputCaptureInstrument() const { return m_sInputCaptureInstrument; }
	const QString& getInputCaptureComponent() const { return m_sInputCaptureComponent; }
	const QString& getInputCaptureLayer() const { return m_sInputCaptureLayer; }
	const QString& getInputCaptureFXLevel() const { return m_sInputCaptureFXLevel; }
	const QString& getInputCaptureFXNumber() const { return m_sInputCaptureFXNumber; }
	const QString& getInputCaptureFilterCutoff() const { return m_sInputCaptureFilterCutoff; }
	const QString& getInputCaptureTag() const { return m_sInputCaptureTag; }

	const QString& getSoundLibraryFailedPreDrumkitLoad() const { return m_sSoundLibraryFailedPreDrumkitLoad; }
	const QString& getSoundLibrarySystemSuffix() const { return m_sSoundLibrarySystemSuffix; }
	const QString& getSoundLibrarySessionSuffix() const { return m_sSoundLibrarySessionSuffix; }

	const QString& getEncodingError() const { return m_sEncodingError; }

	const QString& getExportSongFailure() const { return m_sExportSongFailure; }
	const QString& getExportDrumkitFailure() const { return m_sExportDrumkitFailure; }
	const QString& getImportDrumkitSuccess() const { return m_sImportDrumkitSuccess; }
	const QString& getImportDrumkitFailure() const { return m_sImportDrumkitFailure; }
	const QString& getImportDrumkitEncodingFailure() const { return m_sImportDrumkitEncodingFailure; }
	const QString& getPlaylistSaveFailure() const { return m_sPlaylistSaveFailure; }
	const QString& getIsModified() const { return m_sIsModified; }
	const QString& getReadOnlyAdvice() const { return m_sReadOnlyAdvice; }

	const QString& getInstrumentId() const { return m_sInstrumentId; }
	const QString& getInstrumentType() const { return m_sInstrumentType; }
	const QString& getComponent() const { return m_sComponent; }
	const QString& getSample() const { return m_sSample; }
	const QString& getLicense() const { return m_sLicense; }

	const QString& getMenuActionAdd() const { return m_sMenuActionAdd; }
	const QString& getMenuActionDelete() const { return m_sMenuActionDelete; }
	const QString& getMenuActionRename() const { return m_sMenuActionRename; }
	const QString& getMenuActionLoad() const { return m_sMenuActionLoad; }
	const QString& getMenuActionExport() const { return m_sMenuActionExport; }
	const QString& getMenuActionProperties() const { return m_sMenuActionProperties; }
	const QString& getMenuActionDuplicate() const { return m_sMenuActionDuplicate; }
	const QString& getMenuActionImport() const { return m_sMenuActionImport; }
	const QString& getMenuActionOnlineImport() const { return m_sMenuActionOnlineImport; }

	const QString& getUndoMenuUndo() const { return m_sUndoMenuUndo; }
	const QString& getUndoMenuRedo() const { return m_sUndoMenuRedo; }
	const QString& getUndoMenuHistory() const { return m_sUndoMenuHistory; }
	const QString& getUndoHistoryTitle() const { return m_sUndoHistoryTitle; }

		const QString& getActionEditCurrentDrumkitProperties() const {
			return m_sActionEditCurrentDrumkitProperties; }
		const QString& getActionEditDrumkitProperties() const { return m_sActionEditDrumkitProperties; }
		const QString& getActionIrreversible() const { return m_sActionIrreversible; }

		const QString& getActionAddInstrument() const { return m_sActionAddInstrument; }
		const QString& getActionDeleteInstrument() const { return m_sActionDeleteInstrument; }
		const QString& getActionDropInstrument() const { return m_sActionDropInstrument; }
		const QString& getActionRenameInstrument() const { return m_sActionRenameInstrument; }
		const QString& getActionMoveInstrument() const { return m_sActionMoveInstrument; }
		const QString& getActionSwitchDrumkit() const { return m_sActionSwitchDrumkit; }
		const QString& getActionNewDrumkit() const { return m_sActionNewDrumkit; }
		const QString& getActionLoadDrumkit() const {
			return m_sActionLoadDrumkit; }
		const QString& getActionSaveDrumkit() const {
			return m_sActionSaveDrumkit; }
		const QString& getActionSaveSong() const { return m_sActionSaveSong; }
		const QString& getActionSaveCurrentDrumkit() const {
			return m_sActionSaveCurrentDrumkit; }
		const QString& getActionAddComponent() const { return m_sActionAddComponent; }
		const QString& getActionDeleteComponent() const { return m_sActionDeleteComponent; }
		const QString& getActionRenameComponent() const { return m_sActionRenameComponent; }

		const QString& getActionClearAllNotesInRow() const {
			return m_sActionClearAllNotesInRow; }
		const QString& getActionClearAllNotes() const {
			return m_sActionClearAllNotes; }
		const QString& getActionCutAllNotes() const {
			return m_sActionCutAllNotes; }
		const QString& getActionPasteAllNotes() const {
			return m_sActionPasteAllNotes; }
		const QString& getActionFillNotes() const {
			return m_sActionFillNotes; }
		const QString& getActionFillAllNotes() const {
			return m_sActionFillAllNotes; }
		const QString& getActionFillEverySecondNote() const {
			return m_sActionFillEverySecondNote; }
		const QString& getActionFillEveryThirdNote() const {
			return m_sActionFillEveryThirdNote; }
		const QString& getActionFillEveryFourthNote() const {
			return m_sActionFillEveryFourthNote; }
		const QString& getActionFillEverySixthNote() const {
			return m_sActionFillEverySixthNote; }
		const QString& getActionFillEveryEighthNote() const {
			return m_sActionFillEveryEighthNote; }
		const QString& getActionFillEveryTwelfthNote() const {
			return m_sActionFillEveryTwelfthNote; }
		const QString& getActionFillEverySixteenthNote() const {
			return m_sActionFillEverySixteenthNote; }
		const QString& getActionDeleteNotes() const {
			return m_sActionDeleteNotes; }
		const QString& getActionSelectNotes() const {
			return m_sActionSelectNotes; }
		const QString& getActionEditAllPatterns() const {
			return m_sActionEditAllPatterns; }
		const QString& getActionCopyNotes() const {
			return m_sActionCopyNotes; }
		const QString& getActionEditTypes() const {
			return m_sActionEditTypes; }

		const QString& getNotePropertyVelocity() const {
			return m_sNotePropertyVelocity; }
		const QString& getNotePropertyPan() const { return m_sNotePropertyPan; }
		const QString& getNotePropertyLeadLag() const {
			return m_sNotePropertyLeadLag; }
		const QString& getNotePropertyKeyOctave() const {
			return m_sNotePropertyKeyOctave; }
		const QString& getNotePropertyProbability() const {
			return m_sNotePropertyProbability; }
		const QString& getNotePropertyLength() const {
			return m_sNotePropertyLength; }

		const QString& getNotePitchC() const { return m_sNotePitchC; }
		const QString& getNotePitchCSharp() const { return m_sNotePitchCSharp; }
		const QString& getNotePitchD() const { return m_sNotePitchD; }
		const QString& getNotePitchDSharp() const { return m_sNotePitchDSharp; }
		const QString& getNotePitchE() const { return m_sNotePitchE; }
		const QString& getNotePitchF() const { return m_sNotePitchF; }
		const QString& getNotePitchFSharp() const { return m_sNotePitchFSharp; }
		const QString& getNotePitchG() const { return m_sNotePitchG; }
		const QString& getNotePitchGSharp() const { return m_sNotePitchGSharp; }
		const QString& getNotePitchA() const { return m_sNotePitchA; }
		const QString& getNotePitchASharp() const { return m_sNotePitchASharp; }
		const QString& getNotePitchB() const { return m_sNotePitchB; }

		const QString& getErrorNotFound() const { return m_sErrorNotFound; }
		const QString& getErrorNotFoundShort() const {
			return m_sErrorNotFoundShort; }
		const QString& getErrorEmptyType() const { return m_sErrorEmptyType; }
		const QString& getErrorUniqueTypes() const {return m_sErrorUniqueTypes; }

		const QString& getSettings() const { return m_sSettings; }
private:
	QString m_sSmallSoloButton;
	QString m_sSmallMuteButton;
	QString m_sBigMuteButton;
	QString m_sBigSoloButton;
	QString m_sBypassButton;
	QString m_sEditButton;
	QString m_sClearButton;
	QString m_sPlaybackTrackButton;
	QString m_sTimelineButton;
	QString m_sTimelineBigButton;
	QString m_sFXButton;
	QString m_sPeakButton;
	QString m_sGeneralButton;
	QString m_sInstrumentButton;
	QString m_sSoundLibraryButton;
	QString m_sComponentsButton;
	QString m_sLoadLayerButton;
	QString m_sEditLayerButton;
	QString m_sDeleteLayerButton;
	QString m_sBeatCounterButton;
	QString m_sBeatCounterSetPlayButtonOff;
	QString m_sBeatCounterSetPlayButtonOn;
	QString m_sRubberbandButton;
	QString m_sJackTransportButton;
	QString m_sJackTimebaseButton;
	QString m_sMixerButton;
	QString m_sInstrumentRackButton;
	QString m_sPatternModeButton;
	QString m_sSongModeButton;
	QString m_sAttackLabel;
	QString m_sDecayLabel;
	QString m_sSustainLabel;
	QString m_sReleaseLabel;
	QString m_sMidiOutChannelLabel;
	QString m_sMidiOutNoteLabel;
	QString m_sMidiOutLabel;
	QString m_sPitchLabel;
	QString m_sPitchCoarseLabel;
	QString m_sPitchFineLabel;
	QString m_sPitchRandomLabel;
	QString m_sGainLabel;
	QString m_sMuteGroupLabel;
	QString m_sIsStopNoteLabel;
	QString m_sApplyVelocityLabel;
	QString m_sHihatGroupLabel;
	QString m_sHihatMaxRangeLabel;
	QString m_sHihatMinRangeLabel;
	QString m_sCutoffLabel;
	QString m_sResonanceLabel;
	QString m_sLayerGainLabel;
	QString m_sComponentGainLabel;
	QString m_sSampleSelectionLabel;
	QString m_sPatternSizeLabel;
	QString m_sResolutionLabel;
	QString m_sHearNotesLabel;
	QString m_sQuantizeEventsLabel;
	QString m_sShowPianoLabel;
	QString m_sMidiInLabel;
	QString m_sCpuLabel;
	QString m_sBPMLabel;
	QString m_sTimeHoursLabel;
	QString m_sTimeMinutesLabel;
	QString m_sTimeSecondsLabel;
	QString m_sTimeMilliSecondsLabel;
	QString m_sHumanizeLabel;
	QString m_sSwingLabel;
	QString m_sTimingLabel;
	QString m_sVelocityLabel;
	QString m_sMasterLabel;
	QString m_sReturnLabel;
	
	QString m_sRangeTooltip;
	QString m_sMidiTooltipHeading;
	QString m_sMidiTooltipBound;
	QString m_sMidiTooltipUnbound;
	QString m_sPatternSizeDisabledTooltip;
	
	QString m_sShowDrumkitEditorTooltip;
	QString m_sShowPianoRollEditorTooltip;
	
	QString m_sAudioDriverStartError;
	QString m_sAudioDriverErrorHint;
	QString m_sAudioDriverNotPresent;

	QString m_sJackTimebaseTooltip;
	QString m_sJackTimebaseListenerTooltip;
	QString m_sJackTimebaseDisabledTooltip;
	
	QString m_sMidiSenseWindowTitle;
	QString m_sMidiSenseInput;
	QString m_sMidiSenseUnavailable;

	QString m_sPatternLoadError;
	QString m_sInstrumentLoadError;

	QString m_sFileDialogMissingWritePermissions;
	
	QString m_sStatusOn;
	QString m_sStatusOff;
	QString m_sStatusEnabled;
	QString m_sStatusDisabled;
	QString m_sTimelineEnabled;
	QString m_sTimelineDisabledPatternMode;
	QString m_sTimelineDisabledTimebaseListener;
	QString m_sPatternEditorLocked;
	
	QString m_sPreferencesNotCompiled;
	QString m_sPreferencesNone;
	QString m_sPreferencesJackTooltip;
	QString m_sPreferencesShortcutCapture;

	QString m_sButtonOk;
	QString m_sButtonApply;
	QString m_sButtonSave;
	QString m_sButtonCancel;
	QString m_sButtonDiscard;
	QString m_sButtonPlay;
	QString m_sButtonPlayOriginalSample;
	QString m_sUnsavedChanges;
	QString m_sSavingChanges;
	
	QString m_sMutableDialog;
	QString m_sTabGeneralDialog;
	QString m_sTabLicensesDialog;
	QString m_sNameDialog;
	QString m_sVersionDialog;
	QString m_sLicenseDialog;
	QString m_sAuthorDialog;
	QString m_sNotesDialog;
	
	// Not used yet. A redesign of the GUI startup is required first
	// since these strings are required _before_ HydrogenApp was
	// created.
	// QString m_sDialogSongLoadError;
	// QString m_sDialogUnsavedChangedH1;
	// QString m_sDialogUnsavedChangedH2;
	
	QString m_sLicenseStringLbl;
	QString m_sLicenseComboToolTip;
	QString m_sLicenseStringToolTip;

	QString m_sLicenseCopyleftWarning;
	QString m_sLicenseAttributionWarning;
	QString m_sLicenseWarningWindowTitle;
	QString m_sLicenseMismatchingUserInput;
	
	QString m_sInputCaptureBpm;
	QString m_sInputCaptureVolume;
	QString m_sInputCaptureColumn;
	QString m_sInputCapturePattern;
	QString m_sInputCaptureSong;
	QString m_sInputCaptureInstrument;
	QString m_sInputCaptureComponent;
	QString m_sInputCaptureLayer;
	QString m_sInputCaptureFXLevel;
	QString m_sInputCaptureFXNumber;
	QString m_sInputCaptureFilterCutoff;
	QString m_sInputCaptureTag;

	QString m_sExportSongFailure;
	QString m_sExportDrumkitFailure;
	QString m_sImportDrumkitSuccess;
	QString m_sImportDrumkitFailure;
	QString m_sImportDrumkitEncodingFailure;
	QString m_sPlaylistSaveFailure;
	QString m_sIsModified;
	QString m_sReadOnlyAdvice;

	QString m_sInstrumentId;
	QString m_sInstrumentType;
	QString m_sComponent;
	QString m_sSample;
	QString m_sLicense;

	QString m_sMenuActionAdd;
	QString m_sMenuActionDelete;
	QString m_sMenuActionRename;
	QString m_sMenuActionLoad;
	QString m_sMenuActionExport;
	QString m_sMenuActionProperties;
	QString m_sMenuActionDuplicate;
	QString m_sMenuActionImport;
	QString m_sMenuActionOnlineImport;

		QString m_sUndoMenuUndo;
		QString m_sUndoMenuRedo;
		QString m_sUndoMenuHistory;
		QString m_sUndoHistoryTitle;

		QString m_sActionEditCurrentDrumkitProperties;
		QString m_sActionEditDrumkitProperties;
		QString m_sActionIrreversible;

		QString m_sActionAddInstrument;
		QString m_sActionDeleteInstrument;
		QString m_sActionDropInstrument;
		QString m_sActionRenameInstrument;
		QString m_sActionMoveInstrument;
		QString m_sActionSwitchDrumkit;
		QString m_sActionNewDrumkit;
		QString m_sActionLoadDrumkit;
		QString m_sActionSaveDrumkit;
		QString m_sActionSaveSong;
		QString m_sActionSaveCurrentDrumkit;
		QString m_sActionAddComponent;
		QString m_sActionDeleteComponent;
		QString m_sActionRenameComponent;

		QString m_sActionClearAllNotesInRow;
		QString m_sActionClearAllNotes;
		QString m_sActionCutAllNotes;
		QString m_sActionPasteAllNotes;
		QString m_sActionFillNotes;
		QString m_sActionFillAllNotes;
		QString m_sActionFillEverySecondNote;
		QString m_sActionFillEveryThirdNote;
		QString m_sActionFillEveryFourthNote;
		QString m_sActionFillEverySixthNote;
		QString m_sActionFillEveryEighthNote;
		QString m_sActionFillEveryTwelfthNote;
		QString m_sActionFillEverySixteenthNote;
		QString m_sActionDeleteNotes;
		QString m_sActionSelectNotes;
		QString m_sActionEditAllPatterns;
		QString m_sActionCopyNotes;
		QString m_sActionEditTypes;

		QString m_sErrorNotFound;
		QString m_sErrorNotFoundShort;
		QString m_sErrorEmptyType;
		QString m_sErrorUniqueTypes;

	QString m_sSoundLibraryFailedPreDrumkitLoad;
	QString m_sSoundLibrarySystemSuffix;
	QString m_sSoundLibrarySessionSuffix;

		QString m_sNotePropertyVelocity;
		QString m_sNotePropertyPan;
		QString m_sNotePropertyLeadLag;
		QString m_sNotePropertyKeyOctave;
		QString m_sNotePropertyProbability;
		QString m_sNotePropertyLength;

		QString m_sNotePitchC;
		QString m_sNotePitchCSharp;
		QString m_sNotePitchD;
		QString m_sNotePitchDSharp;
		QString m_sNotePitchE;
		QString m_sNotePitchF;
		QString m_sNotePitchFSharp;
		QString m_sNotePitchG;
		QString m_sNotePitchGSharp;
		QString m_sNotePitchA;
		QString m_sNotePitchASharp;
		QString m_sNotePitchB;

	QString m_sEncodingError;

		QString m_sSettings;
};
#endif
