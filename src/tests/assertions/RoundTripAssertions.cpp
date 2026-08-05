/*
 * Hydrogen
 * Copyright(c) 2008-2026 The hydrogen development team
 * [hydrogen-devel@lists.sourceforge.net]
 *
 * http://www.hydrogen-music.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses/
 *
 */

#include "RoundTripAssertions.h"

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>

#include <core/Basics/AutomationPath.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Sample.h>
#include <core/Timeline.h>
#include <core/License.h>

#include <cmath>
#include <cstdint>
#include <cstring>

using namespace H2Core;

// ── Private helpers ──────────────────────────────────────────────────

void RoundTripAssertions::assertStringEqual( const QString& sLabel,
											 const QString& a, const QString& b )
{
	CPPUNIT_ASSERT_ASSERTION_PASS(
		CPPUNIT_ASSERT_EQUAL_MESSAGE( sLabel.toStdString(),
									  a.toStdString(), b.toStdString() ) );
}

void RoundTripAssertions::assertFloatEqual( const QString& sLabel,
											float a, float b )
{
	// Under -ffast-math (which implies -ffinite-math-only) the compiler is
	// allowed to assume that NaN never occurs. As a result std::isnan() is
	// unconditionally folded to false at compile time and this guard would
	// be silently eliminated — causing CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE
	// to fail because |NaN - NaN| < delta is always false.
	//
	// We inspect the IEEE 754 bit pattern directly instead. Integer
	// operations are not subject to -ffinite-math-only, so the optimizer
	// cannot reason them away.
	static_assert( sizeof( float ) == 4,
				   "float must be 32-bit IEEE 754" );
	uint32_t nBitsA, nBitsB;
	std::memcpy( &nBitsA, &a, sizeof( nBitsA ) );
	std::memcpy( &nBitsB, &b, sizeof( nBitsB ) );
	// IEEE 754 binary32: NaN iff all exponent bits are set and mantissa != 0.
	constexpr uint32_t nExponentMask = 0x7F800000u;
	constexpr uint32_t nMantissaMask  = 0x007FFFFFu;
	const bool bIsNanA = ( nBitsA & nExponentMask ) == nExponentMask &&
		( nBitsA & nMantissaMask ) != 0;
	const bool bIsNanB = ( nBitsB & nExponentMask ) == nExponentMask &&
		( nBitsB & nMantissaMask ) != 0;
	if ( bIsNanA && bIsNanB ) {
		return;
	}
	CPPUNIT_ASSERT_ASSERTION_PASS(
		CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(
			sLabel.toStdString(),
			static_cast<double>( a ), static_cast<double>( b ), 0.001 ) );
}

void RoundTripAssertions::assertIntEqual( const QString& sLabel,
										  int a, int b )
{
	CPPUNIT_ASSERT_ASSERTION_PASS(
		CPPUNIT_ASSERT_EQUAL_MESSAGE( sLabel.toStdString(), a, b ) );
}

void RoundTripAssertions::assertUuidEqual( const QString& sLabel,
										   H2Core::Uuid a, H2Core::Uuid b )
{
	CPPUNIT_ASSERT_ASSERTION_PASS(
		CPPUNIT_ASSERT_EQUAL_MESSAGE( sLabel.toStdString(), a, b ) );
}

void RoundTripAssertions::assertLongLongEqual( const QString& sLabel,
											   long long a, long long b )
{
	CPPUNIT_ASSERT_ASSERTION_PASS(
		CPPUNIT_ASSERT_EQUAL_MESSAGE( sLabel.toStdString(), a, b ) );
}

void RoundTripAssertions::assertBoolEqual( const QString& sLabel,
										   bool a, bool b )
{
	CPPUNIT_ASSERT_ASSERTION_PASS(
		CPPUNIT_ASSERT_EQUAL_MESSAGE( sLabel.toStdString(), a, b ) );
}

// ── License ──────────────────────────────────────────────────────────

void RoundTripAssertions::assertLicenseEqual( const License& a, const License& b )
{
	assertIntEqual( "License::m_license",
					static_cast<int>( a.getType() ),
					static_cast<int>( b.getType() ) );
	// We do not compare copyright holders. This is not yet serialized, not yet
	// supported on a per-sample level but right now it is only used to provide
	// a summary in the SongPropertiesDialog by combining the license and
	// authors of drumkits.
	// assertStringEqual( "License::m_sCopyrightHolder",
	//				   a.getCopyrightHolder(), b.getCopyrightHolder() );
	if ( a.getType() == License::Other ) {
		assertStringEqual( "License::m_sLicenseString",
						   a.getLicenseString(), b.getLicenseString() );
	}
}

// ── AutomationPath ────────────────────────────────────────────────────

void RoundTripAssertions::assertAutomationPathEqual(
	const AutomationPath& a, const AutomationPath& b )
{
	assertFloatEqual( "AutomationPath::m_fMin", a.getMin(), b.getMin() );
	assertFloatEqual( "AutomationPath::m_fMax", a.getMax(), b.getMax() );
	assertFloatEqual( "AutomationPath::m_fDef", a.getDefault(), b.getDefault() );
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "AutomationPath point count",
								  a.empty(), b.empty() );
	auto itA = a.begin();
	auto itB = b.begin();
	std::size_t ii = 0;
	while ( itA != a.end() && itB != b.end() ) {
		assertFloatEqual( QString( "AutomationPath point[%1] x" ).arg( ii ),
						  itA->first, itB->first );
		assertFloatEqual( QString( "AutomationPath point[%1] y" ).arg( ii ),
						  itA->second, itB->second );
		++itA; ++itB; ++ii;
	}
}

// ── ADSR ─────────────────────────────────────────────────────────────

void RoundTripAssertions::assertAdsrEqual( const ADSR& a, const ADSR& b )
{
	assertIntEqual( "Adsr::m_nAttack", a.getAttack(), b.getAttack() );
	assertIntEqual( "Adsr::m_nDecay", a.getDecay(), b.getDecay() );
	assertFloatEqual( "Adsr::m_fSustain", a.getSustain(), b.getSustain() );
	assertIntEqual( "Adsr::m_nRelease", a.getRelease(), b.getRelease() );
	assertIntEqual( "Adsr::m_state",
					static_cast<int>( a.getState() ),
					static_cast<int>( b.getState() ) );
}

// ── Note ─────────────────────────────────────────────────────────────

void RoundTripAssertions::assertNoteEqual( const Note& a, const Note& b )
{
	assertUuidEqual( "Note::m_uuid", a.getUuid(), b.getUuid() );
	assertIntEqual( "Note::m_nPosition", a.getPosition(), b.getPosition() );
	assertFloatEqual( "Note::m_fLeadLag", a.getLeadLag(), b.getLeadLag() );
	assertFloatEqual( "Note::m_fVelocity", a.getVelocity(), b.getVelocity() );
	assertFloatEqual( "Note::m_fPan", a.getPan(), b.getPan() );
	assertIntEqual( "Note::m_nLength", a.getLength(), b.getLength() );
	assertIntEqual( "Note::m_instrumentId",
					static_cast<int>( a.getInstrumentId() ),
					static_cast<int>( b.getInstrumentId() ) );
	assertIntEqual( "Note::m_key",
					static_cast<int>( a.getKey() ),
					static_cast<int>( b.getKey() ) );
	assertIntEqual( "Note::m_octave",
					static_cast<int>( a.getOctave() ),
					static_cast<int>( b.getOctave() ) );
	assertBoolEqual( "Note::m_bNoteOff", a.getNoteOff(), b.getNoteOff() );
	assertFloatEqual( "Note::m_fProbability",
					  a.getProbability(), b.getProbability() );
	assertLongLongEqual( "Note::m_nNoteStart",
						 a.getNoteStart(), b.getNoteStart() );
	assertFloatEqual( "Note::m_fUsedTickSize",
					  a.getUsedTickSize(), b.getUsedTickSize() );
	assertFloatEqual( "Note::m_fPitchHumanization",
					  a.getPitchHumanization(), b.getPitchHumanization() );
	// m_sType is a QString
	assertStringEqual( "Note::m_sType", a.getType(), b.getType() );
	if ( ( a.getAdsr() == nullptr && b.getAdsr() != nullptr ) ||
		 ( a.getAdsr() != nullptr && b.getAdsr() == nullptr ) ) {
		CPPUNIT_FAIL( "Note::m_pAdsr presence" );
	}
	else if ( ( a.getAdsr() != nullptr && b.getAdsr() != nullptr ) ) {
		assertAdsrEqual( a.getAdsr(), b.getAdsr() );
	}
	if ( ( a.getInstrument() == nullptr && b.getInstrument() != nullptr ) ||
		 ( a.getInstrument() != nullptr && b.getInstrument() == nullptr ) ) {
		CPPUNIT_FAIL( "Note::m_pInstrument presence" );
	}
	else if ( ( a.getInstrument() != nullptr && b.getInstrument() != nullptr
			  ) ) {
		assertInstrumentEqual( a.getInstrument(), b.getInstrument() );
	}
}

// ── InstrumentLayer ──────────────────────────────────────────────────

void RoundTripAssertions::assertLayerEqual( const InstrumentLayer& a,
											const InstrumentLayer& b )
{
	assertUuidEqual( "Layer::m_uuid", a.getUuid(), b.getUuid() );
	assertFloatEqual( "Layer::m_fStartVelocity",
					  a.getStartVelocity(), b.getStartVelocity() );
	assertFloatEqual( "Layer::m_fEndVelocity",
					  a.getEndVelocity(), b.getEndVelocity() );
	assertFloatEqual( "Layer::m_fPitchOffset",
					  a.getPitchOffset(), b.getPitchOffset() );
	assertFloatEqual( "Layer::m_fGain", a.getGain(), b.getGain() );
	assertBoolEqual( "Layer::m_bIsMuted",
					 a.getIsMuted(), b.getIsMuted() );
	assertBoolEqual( "Layer::m_bIsSoloed",
					 a.getIsSoloed(), b.getIsSoloed() );
	assertStringEqual(
		"Layer::m_sFallbackSampleFileName", a.getFallbackSampleFileName(),
		b.getFallbackSampleFileName()
	);

	// Sample — compare the file path (the serialized form)
	const auto pSampleA = a.getSample();
	const auto pSampleB = b.getSample();
	if ( ( pSampleA == nullptr && pSampleB != nullptr ) ||
		 ( pSampleA != nullptr && pSampleB == nullptr ) ) {
		CPPUNIT_FAIL( "Layer::m_pSample presence" );
	}
	else if ( pSampleA != nullptr && pSampleB != nullptr ) {
		assertStringEqual( "Layer::Sample::m_sFilePath",
						   pSampleA->getFilePath(),
						   pSampleB->getFilePath() );
		assertBoolEqual( "Layer::Sample::m_bIsLoaded", pSampleA->isLoaded(),
						 pSampleB->isLoaded() );
		assertLongLongEqual( "Layer::Sample::m_nFrames", pSampleA->getFrames(), pSampleB->getFrames() );
		assertIntEqual( "Layer::Sample::m_nSampleRate", pSampleA->getSampleRate(), pSampleB->getSampleRate() );
		assertBoolEqual( "Layer::Sample::m_bIsModified", pSampleA->getIsModified(),
						 pSampleB->getIsModified() );
		assertLicenseEqual( pSampleA->getLicense(), pSampleB->getLicense() );

		// Loops
		const auto& loopsA = pSampleA->getLoops();
		const auto& loopsB = pSampleB->getLoops();
		assertIntEqual( "Sample::Loops::mode",
						static_cast<int>( loopsA.mode ),
						static_cast<int>( loopsB.mode ) );
		assertIntEqual( "Sample::Loops::nStartFrame",
						static_cast<int>( loopsA.nStartFrame ),
						static_cast<int>( loopsB.nStartFrame ) );
		assertIntEqual( "Sample::Loops::nLoopFrame",
						static_cast<int>( loopsA.nLoopFrame ),
						static_cast<int>( loopsB.nLoopFrame ) );
		assertIntEqual( "Sample::Loops::nEndFrame",
						static_cast<int>( loopsA.nEndFrame ),
						static_cast<int>( loopsB.nEndFrame ) );

		// Rubberband
		const auto& rbA = pSampleA->getRubberband();
		const auto& rbB = pSampleB->getRubberband();
		// This bit is reset in InstrumentLayer whenever Hydrogen is not
		// compiled with Rubberband support or the Rubberband CLI is not
		// installed on the system.
		// assertBoolEqual( "Sample::Rubberband::bUse",
		// 				 rbA.bUse, rbB.bUse );
		assertFloatEqual( "Sample::Rubberband::fLengthInBeats",
						  rbA.fLengthInBeats, rbB.fLengthInBeats );
		assertIntEqual( "Sample::Rubberband::nCrispness",
						rbA.nCrispness, rbB.nCrispness );
		assertFloatEqual( "Sample::Rubberband::fSemitonesToShift",
						  rbA.fSemitonesToShift, rbB.fSemitonesToShift );

		// Envelopes
		const auto& panA = pSampleA->getPanEnvelope();
		const auto& panB = pSampleB->getPanEnvelope();
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
			"Sample::PanEnvelope size", panA.size(), panB.size()
		);
		for ( int ii = 0; ii < panA.size(); ++ii ) {
			assertIntEqual(
				"Sample::PanEnvelope::nFrame", panA[ii].nFrame, panB[ii].nFrame
			);
			assertIntEqual(
				"Sample::PanEnvelope::nValue", panA[ii].nValue, panB[ii].nValue
			);
		}
		const auto& velocityA = pSampleA->getVelocityEnvelope();
		const auto& velocityB = pSampleB->getVelocityEnvelope();
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
			"Sample::VelocityEnvelope size", velocityA.size(), velocityB.size()
		);
		for ( int ii = 0; ii < velocityA.size(); ++ii ) {
			assertIntEqual(
				"Sample::VelocityEnvelope::nFrame", velocityA[ii].nFrame,
				velocityB[ii].nFrame
			);
			assertIntEqual(
				"Sample::VelocityEnvelope::nValue", velocityA[ii].nValue,
				velocityB[ii].nValue
			);
		}
	}
}

// ── InstrumentComponent ──────────────────────────────────────────────

void RoundTripAssertions::assertComponentEqual(
	const InstrumentComponent& a, const InstrumentComponent& b )
{
	assertUuidEqual( "Component::m_uuid", a.getUuid(), b.getUuid() );
	assertStringEqual( "Component::m_sName", a.getName(), b.getName() );
	assertFloatEqual( "Component::m_fGain", a.getGain(), b.getGain() );
	assertBoolEqual( "Component::m_bIsMuted",
					 a.getIsMuted(), b.getIsMuted() );
	assertBoolEqual( "Component::m_bIsSoloed",
					 a.getIsSoloed(), b.getIsSoloed() );
	assertIntEqual( "Component::m_selection",
					static_cast<int>( a.getSelection() ),
					static_cast<int>( b.getSelection() ) );

	const auto& layersA = a.getLayers();
	const auto& layersB = b.getLayers();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Component layer count",
								  layersA.size(), layersB.size() );
	for ( std::size_t ii = 0; ii < layersA.size(); ++ii ) {
		if ( layersA[ ii ] != nullptr && layersB[ ii ] != nullptr ) {
			assertLayerEqual( *layersA[ ii ], *layersB[ ii ] );
		}
	}
}

// ── Instrument ───────────────────────────────────────────────────────

void RoundTripAssertions::assertInstrumentEqual( const Instrument& a,
												 const Instrument& b )
{
	assertUuidEqual( "Instrument::m_uuid", a.getUuid(), b.getUuid() );
	assertIntEqual( "Instrument::m_id",
					static_cast<int>( a.getId() ),
					static_cast<int>( b.getId() ) );
	assertStringEqual( "Instrument::m_sName", a.getName(), b.getName() );
	assertStringEqual( "Instrument::m_type", a.getType(), b.getType() );
	assertFloatEqual( "Instrument::m_fVolume", a.getVolume(), b.getVolume() );
	assertBoolEqual( "Instrument::m_bMuted", a.isMuted(), b.isMuted() );
	assertBoolEqual( "Instrument::m_bSoloed", a.isSoloed(), b.isSoloed() );
	assertFloatEqual( "Instrument::m_fPan", a.getPan(), b.getPan() );
	assertFloatEqual( "Instrument::m_fPitchOffset",
					  a.getPitchOffset(), b.getPitchOffset() );
	assertFloatEqual( "Instrument::m_fRandomPitchFactor",
					  a.getRandomPitchFactor(), b.getRandomPitchFactor() );
	assertFloatEqual( "Instrument::m_fGain", a.getGain(), b.getGain() );
	assertBoolEqual( "Instrument::m_bApplyVelocity",
					 a.getApplyVelocity(), b.getApplyVelocity() );
	assertBoolEqual( "Instrument::m_bFilterActive",
					 a.isFilterActive(), b.isFilterActive() );
	assertFloatEqual( "Instrument::m_fFilterCutoff",
					  a.getFilterCutoff(), b.getFilterCutoff() );
	assertFloatEqual( "Instrument::m_fFilterResonance",
					  a.getFilterResonance(), b.getFilterResonance() );

	// ADSR
	const auto pAdsrA = a.getAdsr();
	const auto pAdsrB = b.getAdsr();
	if ( ( pAdsrA == nullptr && pAdsrB != nullptr ) ||
		 ( pAdsrA != nullptr && pAdsrB == nullptr ) ) {
		CPPUNIT_FAIL( "Instrument::m_pAdsr presence" );
	}
	if ( pAdsrA != nullptr && pAdsrB != nullptr ) {
		assertAdsrEqual( pAdsrA, pAdsrB );
	}

	assertIntEqual( "Instrument::m_nMuteGroup",
					a.getMuteGroup(), b.getMuteGroup() );
	assertIntEqual( "Instrument::m_midiOutChannel",
					static_cast<int>( a.getMidiOutChannel() ),
					static_cast<int>( b.getMidiOutChannel() ) );
	assertIntEqual( "Instrument::m_midiOutNote",
					static_cast<int>( a.getMidiOutNote() ),
					static_cast<int>( b.getMidiOutNote() ) );
	assertBoolEqual( "Instrument::m_bStopNotes",
					 a.isStopNotes(), b.isStopNotes() );
	assertIntEqual( "Instrument::m_nHihatGrp",
					a.getHihatGrp(), b.getHihatGrp() );
	assertIntEqual( "Instrument::m_lowerCc",
					static_cast<int>( a.getLowerCc() ),
					static_cast<int>( b.getLowerCc() ) );
	assertIntEqual( "Instrument::m_higherCc",
					static_cast<int>( a.getHigherCc() ),
					static_cast<int>( b.getHigherCc() ) );
	assertBoolEqual( "Instrument::m_bIsPreviewInstrument",
					 a.isPreviewInstrument(), b.isPreviewInstrument() );

	// Components
	const auto compsA = a.getComponents();
	const auto compsB = b.getComponents();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Instrument component count",
								  compsA->size(), compsB->size() );
	for ( std::size_t ii = 0; ii < compsA->size(); ++ii ) {
		if ( compsA->at( ii ) != nullptr && compsB->at( ii ) != nullptr ) {
			assertComponentEqual( *compsA->at( ii ), *compsB->at( ii ) );
		}
	}

	// Song-kit context fields (only serialized when bSongKit=true)
	assertStringEqual( "Instrument::m_sDrumkitPath",
					   a.getDrumkitPath(), b.getDrumkitPath() );
	assertStringEqual( "Instrument::m_sDrumkitName",
					   a.getDrumkitName(), b.getDrumkitName() );
}

// ── Drumkit ──────────────────────────────────────────────────────────

void RoundTripAssertions::assertDrumkitEqual( const Drumkit& a,
											  const Drumkit& b )
{
	assertUuidEqual( "Drumkit::m_uuid", a.getUuid(), b.getUuid() );
	assertIntEqual(
		"Drumkit::m_context", static_cast<int>( a.getContext() ),
		static_cast<int>( b.getContext() )
	);
	assertStringEqual( "Drumkit::m_sPath", a.getPath(), b.getPath() );
	assertStringEqual( "Drumkit::m_sName", a.getName(), b.getName() );
	assertIntEqual( "Drumkit::m_nVersion", a.getVersion(), b.getVersion() );
	assertStringEqual( "Drumkit::m_sAuthor", a.getAuthor(), b.getAuthor() );
	assertStringEqual( "Drumkit::m_sInfo", a.getInfo(), b.getInfo() );
	assertLicenseEqual( a.getLicense(), b.getLicense() );

	// Tags
	const auto& tagsA = a.getTags();
	const auto& tagsB = b.getTags();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Drumkit tag count",
								  tagsA.size(), tagsB.size() );
	for ( int ii = 0; ii < tagsA.size(); ++ii ) {
		assertStringEqual( QString( "Drumkit::m_tags[%1]" ).arg( ii ),
						   tagsA[ ii ], tagsB[ ii ] );
	}

	assertStringEqual( "Drumkit::m_sImage", a.getImage(), b.getImage() );
	assertLicenseEqual( a.getImageLicense(), b.getImageLicense() );
	assertBoolEqual( "Drumkit::m_bIsModified",
					 a.getIsModified(), b.getIsModified() );

	// Instruments
	const auto pInstrsA = a.getInstruments();
	const auto pInstrsB = b.getInstruments();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Drumkit instrument count",
								  pInstrsA->size(), pInstrsB->size() );
	for ( int ii = 0; ii < pInstrsA->size(); ++ii ) {
		const auto pInstrA = pInstrsA->get( ii );
		const auto pInstrB = pInstrsB->get( ii );
		if ( pInstrA != nullptr && pInstrB != nullptr ) {
			assertInstrumentEqual( *pInstrA, *pInstrB );
		}
	}
}

// ── Pattern ──────────────────────────────────────────────────────────

void RoundTripAssertions::assertPatternEqual( const Pattern& a,
											  const Pattern& b )
{
	assertUuidEqual( "Pattern::m_uuid", a.getUuid(), b.getUuid() );
	assertStringEqual( "Pattern::m_sPath", a.getPath(), b.getPath() );
	assertIntEqual( "Pattern::m_nVersion", a.getVersion(), b.getVersion() );
	assertStringEqual( "Pattern::m_sDrumkitName",
					   a.getDrumkitName(), b.getDrumkitName() );
	assertStringEqual( "Pattern::m_sName", a.getName(), b.getName() );
	assertStringEqual( "Pattern::m_sAuthor", a.getAuthor(), b.getAuthor() );
	assertStringEqual( "Pattern::m_sInfo", a.getInfo(), b.getInfo() );
	assertLicenseEqual( a.getLicense(), b.getLicense() );
	assertIntEqual( "Pattern::m_nLength", a.getLength(), b.getLength() );
	assertIntEqual( "Pattern::m_nDenominator",
					a.getDenominator(), b.getDenominator() );
	assertBoolEqual( "Pattern::m_bIsModified",
					 a.getIsModified(), b.getIsModified() );

	// Tags
	const auto& tagsA = a.getTags();
	const auto& tagsB = b.getTags();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Pattern tag count",
								  tagsA.size(), tagsB.size() );
	for ( int ii = 0; ii < tagsA.size(); ++ii ) {
		assertStringEqual( QString( "Pattern::m_tags[%1]" ).arg( ii ),
						   tagsA[ ii ], tagsB[ ii ] );
	}

	// Notes
	const auto pNotesA = a.getNotes();
	const auto pNotesB = b.getNotes();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Pattern note count",
								  pNotesA->size(), pNotesB->size() );

	// Notes are stored in a multimap keyed by position. Iterate both in
	// the same order (ascending position).
	auto itA = pNotesA->begin();
	auto itB = pNotesB->begin();
	int nNoteIdx = 0;
	while ( itA != pNotesA->end() && itB != pNotesB->end() ) {
		assertIntEqual(
			QString( "Pattern::note[%1] position" ).arg( nNoteIdx ),
			itA->first, itB->first );
		if ( itA->second != nullptr && itB->second != nullptr ) {
			assertNoteEqual( *itA->second, *itB->second );
		}
		++itA; ++itB; ++nNoteIdx;
	}

	// Virtual patterns
	const auto pVirtualPatternsA = a.getVirtualPatterns();
	const auto pVirtualPatternsB = b.getVirtualPatterns();
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Virtual pattern count", pVirtualPatternsA->size(),
		pVirtualPatternsB->size()
	);

	auto itVPA = pVirtualPatternsA->begin();
	auto itVPB = pVirtualPatternsB->begin();
	while ( itVPA != pVirtualPatternsA->end() && itVPB != pVirtualPatternsB->end() ) {
		assertPatternEqual( *itVPA, *itVPB );
		++itVPA; ++itVPB;
	}

	const auto pFlattenedVirtualPatternsA = a.getFlattenedVirtualPatterns();
	const auto pFlattenedVirtualPatternsB = b.getFlattenedVirtualPatterns();
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"FlattenedVirtual pattern count", pFlattenedVirtualPatternsA->size(),
		pFlattenedVirtualPatternsB->size()
	);

	auto itFVPA = pFlattenedVirtualPatternsA->begin();
	auto itFVPB = pFlattenedVirtualPatternsB->begin();
	while ( itFVPA != pFlattenedVirtualPatternsA->end() &&
			itFVPB != pFlattenedVirtualPatternsB->end() ) {
		assertPatternEqual( *itFVPA, *itFVPB );
		++itFVPA;
		++itFVPB;
	}
}

// ── PlaylistEntry ────────────────────────────────────────────────────

void RoundTripAssertions::assertPlaylistEntryEqual(
	const PlaylistEntry& a, const PlaylistEntry& b )
{
	auto normalizePath = []( const QString& sPath ) {
		QString sNormalized( sPath );
		if ( sNormalized.startsWith( "C:" ) ) {
			sNormalized.replace( "C:", "" );
		}
		return sNormalized;
	};
	assertStringEqual(
		"PlaylistEntry::m_sSongPath", normalizePath( a.getSongPath() ),
		normalizePath( b.getSongPath() )
	);
	assertStringEqual(
		"PlaylistEntry::m_sScriptPath", normalizePath( a.getScriptPath() ),
		normalizePath( b.getScriptPath() )
	);
	assertBoolEqual(
		"PlaylistEntry::m_bScriptEnabled", a.getScriptEnabled(),
		b.getScriptEnabled()
	);
}

// ── Playlist ─────────────────────────────────────────────────────────

void RoundTripAssertions::assertPlaylistEqual( const Playlist& a,
											   const Playlist& b )
{
	assertUuidEqual( "Playlist::m_uuid", a.getUuid(), b.getUuid() );
	assertStringEqual( "Playlist::m_sPath", a.getPath(), b.getPath() );
	assertBoolEqual( "Playlist::m_bIsModified",
					 a.getIsModified(), b.getIsModified() );
	assertBoolEqual( "Playlist::m_nActiveSongNumber",
					 a.getActiveSongNumber(), b.getActiveSongNumber() );
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Playlist entry count", a.size(), b.size() );
	for ( int ii = 0; ii < a.size(); ++ii ) {
		const auto pEntryA = a.get( ii );
		const auto pEntryB = b.get( ii );
		if ( pEntryA != nullptr && pEntryB != nullptr ) {
			assertPlaylistEntryEqual( *pEntryA, *pEntryB );
		}
	}
}

// ── Song ─────────────────────────────────────────────────────────────

void RoundTripAssertions::assertSongEqual( const Song& a, const Song& b )
{
	assertUuidEqual( "Song::m_uuid", a.getUuid(), b.getUuid() );
	assertFloatEqual( "Song::m_fBpm", a.getBpm(), b.getBpm() );
	assertFloatEqual( "Song::m_fVolume", a.getVolume(), b.getVolume() );
	assertBoolEqual( "Song::m_bIsMuted", a.getIsMuted(), b.getIsMuted() );
	assertIntEqual( "Song::m_nVersion", a.getVersion(), b.getVersion() );
	assertStringEqual( "Song::m_sPath", a.getPath(), b.getPath() );
	assertStringEqual( "Song::m_sName", a.getName(), b.getName() );
	assertStringEqual( "Song::m_sAuthor", a.getAuthor(), b.getAuthor() );
	assertStringEqual( "Song::m_sNotes", a.getNotes(), b.getNotes() );
	assertLicenseEqual( a.getLicense(), b.getLicense() );

	// Tags
	const auto& tagsA = a.getTags();
	const auto& tagsB = b.getTags();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Song tag count",
								  tagsA.size(), tagsB.size() );
	for ( int ii = 0; ii < tagsA.size(); ++ii ) {
		assertStringEqual( QString( "Song::m_tags[%1]" ).arg( ii ),
						   tagsA[ ii ], tagsB[ ii ] );
	}

	// Loop mode
	assertIntEqual( "Song::m_loopMode",
					static_cast<int>( a.getLoopMode() ),
					static_cast<int>( b.getLoopMode() ) );
	// Pattern mode
	assertIntEqual( "Song::m_patternMode",
					static_cast<int>( a.getPatternMode() ),
					static_cast<int>( b.getPatternMode() ) );
	// Mode
	assertIntEqual( "Song::m_mode",
					static_cast<int>( a.getMode() ),
					static_cast<int>( b.getMode() ) );
	// Action mode
	assertIntEqual( "Song::m_actionMode",
					static_cast<int>( a.getActionMode() ),
					static_cast<int>( b.getActionMode() ) );

	assertBoolEqual( "Song::m_bIsPatternEditorLocked",
					 a.getIsPatternEditorLocked(), b.getIsPatternEditorLocked() );
	assertBoolEqual( "Song::m_bIsTimelineActivated",
					 a.getIsTimelineActivated(), b.getIsTimelineActivated() );
	// Since the methods involved do also set bIsModified themselves, we ignore
	// this member for now.
	// assertBoolEqual( "Song::m_bIsModified",
	// 				 a.getIsModified(), b.getIsModified() );

	assertIntEqual( "Song::m_nPanLawType",
					a.getPanLawType(), b.getPanLawType() );
	assertFloatEqual( "Song::m_fPanLawKNorm",
					  a.getPanLawKNorm(), b.getPanLawKNorm() );
	assertFloatEqual( "Song::m_fHumanizeTimeValue",
					  a.getHumanizeTimeValue(), b.getHumanizeTimeValue() );
	assertFloatEqual( "Song::m_fHumanizeVelocityValue",
					  a.getHumanizeVelocityValue(),
					  b.getHumanizeVelocityValue() );
	assertFloatEqual( "Song::m_fSwingFactor",
					  a.getSwingFactor(), b.getSwingFactor() );

	assertStringEqual( "Song::m_sLastLoadedDrumkitPath",
					   a.getLastLoadedDrumkitPath(),
					   b.getLastLoadedDrumkitPath() );
	assertBoolEqual( "Song::m_bWasAskedAboutMissingSamples",
					 a.getWasAskedAboutMissingSamples(),
					 b.getWasAskedAboutMissingSamples() );

	// Drumkit
	const auto pKitA = a.getDrumkit();
	const auto pKitB = b.getDrumkit();
	if ( pKitA != nullptr && pKitB != nullptr ) {
		assertDrumkitEqual( *pKitA, *pKitB );
	}
	else {
		CPPUNIT_ASSERT_EQUAL_MESSAGE( "Song drumkit null-ness",
									  pKitA == nullptr, pKitB == nullptr );
	}

	// Pattern list
	const auto pPatternsA = a.getPatternList();
	const auto pPatternsB = b.getPatternList();
	CPPUNIT_ASSERT_EQUAL_MESSAGE( "Song pattern count",
								  pPatternsA->size(), pPatternsB->size() );
	for ( int ii = 0; ii < pPatternsA->size(); ++ii ) {
		const auto pPatA = pPatternsA->get( ii );
		const auto pPatB = pPatternsB->get( ii );
		if ( pPatA != nullptr && pPatB != nullptr ) {
			assertPatternEqual( *pPatA, *pPatB );
		}
	}

	// Pattern group vector
	const auto& pgvA = a.getPatternGroupVector();
	const auto& pgvB = b.getPatternGroupVector();
	CPPUNIT_ASSERT_EQUAL_MESSAGE(
		"Song patternGroupVector size", pgvA->size(), pgvB->size()
	);
	for ( std::size_t ii = 0; ii < pgvA->size(); ++ii ) {
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
			QString( "Song patternGroupVector[%1] size" )
				.arg( ii )
				.toStdString(),
			pgvA->at( ii )->size(), pgvB->at( ii )->size()
		);
		for ( int jj = 0; jj < pgvA->at( ii )->size(); ++jj ) {
			assertPatternEqual(
				pgvA->at( ii )->get( jj ), pgvB->at( ii )->get( jj )
			);
		}
	}

	// Automation path
	const auto pAutoA = a.getAutomationPath();
	const auto pAutoB = b.getAutomationPath();
	if ( pAutoA != nullptr && pAutoB != nullptr ) {
		assertAutomationPathEqual( *pAutoA, *pAutoB );
	}

	// Timeline — tempo markers and tags
	const auto pTimelineA = a.getTimeline();
	const auto pTimelineB = b.getTimeline();
	if ( pTimelineA != nullptr && pTimelineB != nullptr ) {
		const auto& markersA = pTimelineA->getAllTempoMarkers();
		const auto& markersB = pTimelineB->getAllTempoMarkers();
		CPPUNIT_ASSERT_EQUAL_MESSAGE( "Timeline tempo marker count",
									  markersA.size(), markersB.size() );
		for ( std::size_t ii = 0; ii < markersA.size(); ++ii ) {
			assertIntEqual( QString( "Timeline::marker[%1] bar" ).arg( ii ),
							markersA[ ii ]->nColumn, markersB[ ii ]->nColumn );
			assertFloatEqual( QString( "Timeline::marker[%1] bpm" ).arg( ii ),
							  markersA[ ii ]->fBpm, markersB[ ii ]->fBpm );
		}
		const auto& tagsA = pTimelineA->getAllTags();
		const auto& tagsB = pTimelineB->getAllTags();
		CPPUNIT_ASSERT_EQUAL_MESSAGE( "Timeline tag count",
									  tagsA.size(), tagsB.size() );
		for ( std::size_t ii = 0; ii < tagsA.size(); ++ii ) {
			assertIntEqual( QString( "Timeline::tag[%1] bar" ).arg( ii ),
							tagsA[ ii ]->nColumn, tagsB[ ii ]->nColumn );
			assertStringEqual( QString( "Timeline::tag[%1] text" ).arg( ii ),
							   tagsA[ ii ]->sTag, tagsB[ ii ]->sTag );
		}
		assertFloatEqual(
			"Timeline::m_fDefaultBpm", pTimelineA->getDefaultBpm(),
			pTimelineB->getDefaultBpm()
		);
	}
	else {
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
			"Song timeline null-ness", pTimelineA == nullptr,
			pTimelineB == nullptr
		);
	}

	// Playback track instrument
	const auto pPlaybackA = a.getPlaybackTrackInstrument();
	const auto pPlaybackB = b.getPlaybackTrackInstrument();
	if ( pPlaybackA != nullptr && pPlaybackB != nullptr ) {
		assertInstrumentEqual( *pPlaybackA, *pPlaybackB );
	}
	else {
		CPPUNIT_ASSERT_EQUAL_MESSAGE( "Song playback track null-ness",
									  pPlaybackA == nullptr,
									  pPlaybackB == nullptr );
	}
}

// ── Preferences (core props subset) ─────────────────────────────────

void RoundTripAssertions::assertCorePreferencesEqual(
	const Preferences& a, const Preferences& b )
{
	assertStringEqual( "Preferences::coreProps",
					a.corePropsToXml(), b.corePropsToXml() );
}
