/*
 * Hydrogen
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

#ifndef H2C_CONNECT_VIA_IPC_MODE_TEST_H
#define H2C_CONNECT_VIA_IPC_MODE_TEST_H

#include <cppunit/extensions/HelperMacros.h>

#include <core/Object.h>

class ConnectViaIpcModeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( ConnectViaIpcModeTest );
	CPPUNIT_TEST( testAttachesToEngineEndpoint );
	CPPUNIT_TEST( testFailedConnectionReported );
	CPPUNIT_TEST( testMirrorUsesHeadlessDriver );
	CPPUNIT_TEST( testIssuesCommands );
	CPPUNIT_TEST( testEngineSurvivesEditorDisconnect );
	CPPUNIT_TEST( testEngineBuildsTransportSnapshot );
	CPPUNIT_TEST( testEngineBuildsFullSnapshot );
	CPPUNIT_TEST( testEngineSnapshotCapsInstrumentPeaks );
	CPPUNIT_TEST( testMirrorFollowsTransportTelemetry );
	CPPUNIT_TEST( testMirrorAppliesMeterTelemetry );
	CPPUNIT_TEST( testTelemetryMetersFlowEngineToEditor );
	CPPUNIT_TEST( testTelemetryLateAttach );
	CPPUNIT_TEST( testAudioDriverInfoCarriesSampleRate );
	CPPUNIT_TEST( testMirrorSyncsSampleRate );
	CPPUNIT_TEST( testMirrorAppliesSelectionEvents );
	CPPUNIT_TEST( testSelectionEventsCarryStoredState );
	CPPUNIT_TEST( testEngineSelectionChangesReachMirror );
	CPPUNIT_TEST( testEditorSelectionReachesEngine );
	CPPUNIT_TEST( testEditorChannelDeliversViaSignalNotQueue );
	CPPUNIT_TEST( testEngineAccessFallsBackToLocal );
	CPPUNIT_TEST( testSyncViaIpc );
	CPPUNIT_TEST( testMidiDriverReadsRoundTrip );
	CPPUNIT_TEST( testHandledMidiLogClearOrdering );
	CPPUNIT_TEST( testSetPreferencesSkipsMirrorRestarts );
	CPPUNIT_TEST( testSetPreferencesRestartsEngineDrivers );
	CPPUNIT_TEST( testAudioDeviceQueriesRoundTrip );
	CPPUNIT_TEST( testMirrorHoldsNoOscOrNsmObjects );
	CPPUNIT_TEST( testMirrorSetSongDoesNotPinPathUnderNsm );
	CPPUNIT_TEST( testMirrorSetSongModifiedWithoutNsmClient );
	CPPUNIT_TEST( testOscTemporaryPortQueryRoundTrip );
	CPPUNIT_TEST( testRecreateOscServerForwardsToEngine );
	CPPUNIT_TEST( testSongModifiedForwardsToEngine );
	CPPUNIT_TEST( testSessionFolderQueryRoundTrip );
	CPPUNIT_TEST( testPulledSongPreservesModifiedFlag );
	CPPUNIT_TEST( testEngineFlipEventCrosses );
	CPPUNIT_TEST( testEditorFlipEchoSuppressed );
	CPPUNIT_TEST( testForwardedEditDoesNotEchoSongModified );
	CPPUNIT_TEST( testSongModifiedForceFiresWhenUnchanged );
	CPPUNIT_TEST( testEngineWrapperFlipWhileDirtyEchoes );
	CPPUNIT_TEST( testRuntimeErrorForwardsToEditor );
	CPPUNIT_TEST( testBootErrorReplayedOnConnect );
	CPPUNIT_TEST( testAdhocInstrumentPreviewForwardsToEngine );
	CPPUNIT_TEST( testUpdateBeatCounterSettingsForwardsToEngine );
	CPPUNIT_TEST( testHandleBeatCounterForwardsToEngine );
	CPPUNIT_TEST( testOnTapTempoAccelEventForwardsToEngine );
	CPPUNIT_TEST( testSetIsTimelineActivatedForwardsToEngine );
	CPPUNIT_TEST( testSetPatternModeForwardsToEngine );
	CPPUNIT_TEST( testLoadPlaybackTrackForwardsToEngine );
	CPPUNIT_TEST( testSetDrumkitModifiedForwardsToEngine );
	CPPUNIT_TEST( testSetPatternModifiedForwardsToEngine );
	CPPUNIT_TEST( testSetIsPatternEditorLockedForwardsToEngine );
	CPPUNIT_TEST_SUITE_END();

public:
	void testAttachesToEngineEndpoint();
	void testFailedConnectionReported();
	void testMirrorUsesHeadlessDriver();
	void testIssuesCommands();
	void testEngineSurvivesEditorDisconnect();
	void testEngineBuildsTransportSnapshot();
	void testEngineBuildsFullSnapshot();
	void testEngineSnapshotCapsInstrumentPeaks();
	void testMirrorFollowsTransportTelemetry();
	void testMirrorAppliesMeterTelemetry();
	void testTelemetryMetersFlowEngineToEditor();
	void testTelemetryLateAttach();
	void testAudioDriverInfoCarriesSampleRate();
	void testMirrorSyncsSampleRate();
	void testMirrorAppliesSelectionEvents();
	void testSelectionEventsCarryStoredState();
	void testEngineSelectionChangesReachMirror();
	void testEditorSelectionReachesEngine();
	void testEditorChannelDeliversViaSignalNotQueue();
	void testEngineAccessFallsBackToLocal();
	void testSyncViaIpc();
	void testMidiDriverReadsRoundTrip();
	void testHandledMidiLogClearOrdering();
	void testSetPreferencesSkipsMirrorRestarts();
	void testSetPreferencesRestartsEngineDrivers();
	void testAudioDeviceQueriesRoundTrip();
	void testMirrorHoldsNoOscOrNsmObjects();
	void testMirrorSetSongDoesNotPinPathUnderNsm();
	void testMirrorSetSongModifiedWithoutNsmClient();
	void testOscTemporaryPortQueryRoundTrip();
	void testRecreateOscServerForwardsToEngine();
	void testSongModifiedForwardsToEngine();
	void testSessionFolderQueryRoundTrip();
	void testPulledSongPreservesModifiedFlag();
	void testEngineFlipEventCrosses();
	void testEditorFlipEchoSuppressed();
	void testForwardedEditDoesNotEchoSongModified();
	void testSongModifiedForceFiresWhenUnchanged();
	void testEngineWrapperFlipWhileDirtyEchoes();
	void testRuntimeErrorForwardsToEditor();
	void testBootErrorReplayedOnConnect();
	void testAdhocInstrumentPreviewForwardsToEngine();
	void testUpdateBeatCounterSettingsForwardsToEngine();
	void testHandleBeatCounterForwardsToEngine();
	void testOnTapTempoAccelEventForwardsToEngine();
	void testSetIsTimelineActivatedForwardsToEngine();
	void testSetPatternModeForwardsToEngine();
	void testLoadPlaybackTrackForwardsToEngine();
	void testSetDrumkitModifiedForwardsToEngine();
	void testSetPatternModifiedForwardsToEngine();
	void testSetIsPatternEditorLockedForwardsToEngine();
};

#endif
