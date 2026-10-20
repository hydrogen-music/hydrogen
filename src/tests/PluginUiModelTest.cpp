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
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see https://www.gnu.org/licenses
 *
 */

#include "PluginUiModelTest.h"

#include <core/AudioEngine/AudioEngine.h>
#include <core/AudioEngine/Transport.h>
#include <core/Basics/Event.h>
#include <core/Basics/Pattern.h>
#include <core/Basics/PatternList.h>
#include <core/Basics/Song.h>
#include <core/Helpers/Filesystem.h>
#include <core/Hydrogen.h>
#include <core/Midi/MidiInstrumentMap.h>
#include <core/Preferences/Preferences.h>
#include <core/SoundLibrary/SoundLibraryDatabase.h>

#include <plugin/HydrogenPlugin.h>
#include <plugin/ui/PluginUiEngineSink.h>
#include <plugin/ui/PluginUiLandingModel.h>
#include <plugin/ui/PluginUiMappingModel.h>
#include <plugin/ui/PluginUiMixerModel.h>
#include <core/CommonStrings.h>

#include <algorithm>
#include <optional>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QTranslator>

#include "TestHelper.h"

using namespace H2Core;

void PluginUiModelTest::testLandingLoadCommands()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiLandingModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiLandingModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.loadSong(
		H2TEST_FILE( "functional/test_adsr.h2song" ) ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );

	// Song: load + install — a new song object replaces the empty one and
	// setSong loads the installed kit's samples (ASSERT_SONG; the fixture's
	// broken absolute drumkitPath forces the name-based kit lookup through
	// the data folders).
	auto pSongBefore = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSongBefore != nullptr );
	CPPUNIT_ASSERT( model.loadSong(
		H2TEST_FILE( "functional/test_adsr.h2song" ) ) );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( pSong != pSongBefore );
	ASSERT_SONG( pSong );

	// An unloadable path leaves the current song untouched.
	CPPUNIT_ASSERT( ! model.loadSong( "/nonexistent/nope.h2song" ) );
	CPPUNIT_ASSERT( pHydrogen->getSong() == pSong );

	// Pattern: load + install at position 0 of the current song.
	auto pPatternList = pSong->getPatternList();
	CPPUNIT_ASSERT( pPatternList != nullptr );
	const int nPatternsBefore = pPatternList->size();
	CPPUNIT_ASSERT( model.loadPattern(
		H2TEST_FILE( "pattern/pattern.h2pattern" ), 0, false ) );
	CPPUNIT_ASSERT( pPatternList->size() == nPatternsBefore + 1 );
	CPPUNIT_ASSERT( pPatternList->get( 0 ) != nullptr );
	CPPUNIT_ASSERT( pPatternList->get( 0 )->getName() == "pat" );

	// A second load replacing position 0 swaps the pattern in place.
	CPPUNIT_ASSERT( model.loadPattern(
		H2TEST_FILE( "pattern/empty.h2pattern" ), 0, true ) );
	CPPUNIT_ASSERT( pPatternList->size() == nPatternsBefore + 1 );
	CPPUNIT_ASSERT( pPatternList->get( 0 ) != nullptr );
	CPPUNIT_ASSERT( pPatternList->get( 0 )->getName() == "Pattern" );

	// An unloadable path leaves the pattern list untouched.
	CPPUNIT_ASSERT( ! model.loadPattern( "/nonexistent/nope.h2pattern",
										 0, false ) );
	CPPUNIT_ASSERT( pPatternList->size() == nPatternsBefore + 1 );

	// Drumkit: load + install into the current song.
	CPPUNIT_ASSERT( model.loadDrumkit(
		TestHelper::get_instance()->getDataDir() + "/drumkits/GMRockKit" ) );
	auto pDrumkit = pHydrogen->getSong()->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getName() == "GMRockKit" );

	// An unloadable path leaves the current kit untouched.
	CPPUNIT_ASSERT( ! model.loadDrumkit( "/nonexistent/nope" ) );
	CPPUNIT_ASSERT( pHydrogen->getSong()->getDrumkit() == pDrumkit );

	// Editor button: forwards to the plugin's editor seam. No process
	// spawn in the unit test — the always-on session (UI-3) serves the
	// endpoint already.
	CPPUNIT_ASSERT( model.openEditor( /*bLaunchProcess=*/false ) );
	CPPUNIT_ASSERT( ! plugin.getEditorEndpoint().isEmpty() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testLandingDrumkitListing()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiLandingModel model( &plugin );

	// A model without a plugin reports an empty listing instead of
	// crashing.
	PluginUiLandingModel orphanModel( nullptr );
	CPPUNIT_ASSERT( orphanModel.drumkits().empty() );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );

	// The database populates asynchronously at startup; the picker
	// only shows stable results once the initial scan finished.
	pHydrogen->getSoundLibraryDatabase()->waitForInitialScan();

	const auto kits = model.drumkits();
	CPPUNIT_ASSERT( ! kits.empty() );

	// Sorted by name, case-insensitive.
	for ( std::size_t ii = 1; ii < kits.size(); ++ii ) {
		CPPUNIT_ASSERT( kits[ ii - 1 ].sName.compare(
			kits[ ii ].sName, Qt::CaseInsensitive ) <= 0 );
	}

	// The repo's GMRockKit is listed with its drumkit.xml metadata;
	// the info is plain text (HTML tags stripped for the tooltip).
	const auto it = std::find_if(
		kits.begin(), kits.end(),
		[]( const PluginUiLandingModel::DrumkitSummary& kit ) {
			return kit.sName == "GMRockKit";
		} );
	CPPUNIT_ASSERT( it != kits.end() );
	CPPUNIT_ASSERT( it->sAuthor == "Glen MacArthur / Sebastian Moors" );
	CPPUNIT_ASSERT( it->sLicense == "GPL" );
	CPPUNIT_ASSERT( it->nVersion == 0 );
	// The database keys kits by their canonical drumkit.xml path.
	CPPUNIT_ASSERT( it->sPath.endsWith( "drumkits/GMRockKit/drumkit.xml" ) );
	CPPUNIT_ASSERT( it->sInfo.contains(
		"A Sampled 5pc Pearl DX Series Drumkit." ) );
	CPPUNIT_ASSERT( ! it->sInfo.contains( "<" ) );

	// Paths are unique — they are the picker's selection key.
	for ( std::size_t ii = 0; ii < kits.size(); ++ii ) {
		for ( std::size_t jj = ii + 1; jj < kits.size(); ++jj ) {
			CPPUNIT_ASSERT( kits[ ii ].sPath != kits[ jj ].sPath );
		}
	}

	// Selecting a row loads the kit: a summary's path installs the kit
	// it describes.
	CPPUNIT_ASSERT( model.loadDrumkit( it->sPath ) );
	auto pDrumkit = pHydrogen->getSong()->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	CPPUNIT_ASSERT( pDrumkit->getName() == "GMRockKit" );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testLandingEditorExecutable()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiLandingModel model( &plugin );

	// A model without a plugin has no executable and refuses the
	// override.
	PluginUiLandingModel orphanModel( nullptr );
	CPPUNIT_ASSERT( orphanModel.editorExecutable().isEmpty() );
	CPPUNIT_ASSERT(
		! orphanModel.setEditorExecutable( "/nonexistent/hydrogen" ) );

	// Default: the resolved binary — never empty (the PATH fallback
	// names the bare executable).
	const QString sDefault = model.editorExecutable();
	CPPUNIT_ASSERT( ! sDefault.isEmpty() );

	// An explicit override wins; clearing it restores the default.
	CPPUNIT_ASSERT( model.setEditorExecutable( "/nonexistent/hydrogen" ) );
	CPPUNIT_ASSERT( model.editorExecutable() == "/nonexistent/hydrogen" );
	CPPUNIT_ASSERT( model.setEditorExecutable( "" ) );
	CPPUNIT_ASSERT( model.editorExecutable() == sDefault );

	// The override is what launches: opening without spawning still
	// serves the endpoint (the test acts as the editor itself).
	CPPUNIT_ASSERT( model.setEditorExecutable( "/nonexistent/hydrogen" ) );
	CPPUNIT_ASSERT( model.openEditor( /*bLaunchProcess=*/false ) );
	CPPUNIT_ASSERT( ! plugin.getEditorEndpoint().isEmpty() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testHtmlToPlainText()
{
	___INFOLOG( "" );

	// The reference battery: every expected output below was produced by
	// the QtGui template itself —
	// QTextDocumentFragment::fromHtml( html ).toPlainText() — the exact
	// path the GUI's InfoView tooltip takes with a drumkit's <info>
	// (reference Qt 6.8.2). The plugin UI links QtCore only, so the
	// model reimplements those semantics and this table pins them
	// case-by-case: tags dropped, entities decoded, block boundaries as
	// newlines, whitespace collapsed per white-space mode, tables as
	// gridded cells.
	struct Case {
		const char* szName;
		const char* szHtml;
		const char* szExpected;
	};
	const Case cases[] = {
		{ "gmrockkit-info", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\np, li { white-space: pre-wrap; }\n</style></head><body style=\" font-family:'Lucida Grande'; font-size:10pt; font-weight:400; font-style:normal;\">\n<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">A Sampled 5pc Pearl DX Series Drumkit.</p></body></html>", "A Sampled 5pc Pearl DX Series Drumkit." },
		{ "two-paragraphs", "<p>Hello</p><p>World</p>", "Hello\nWorld" },
		{ "br", "a<br>b", "a\nb" },
		{ "br-br", "a<br><br>b", "a\n\nb" },
		{ "p-br-p", "<p>a</p><br><p>b</p>", "a\n\n\nb" },
		{ "entities", "&amp; &lt; &gt; &quot; &nbsp;&#65;&apos;", "& < > \"  A'" },
		{ "nbsp-between", "a&nbsp;b", "a b" },
		{ "head-noise", "<html><head><style>p { color: red }</style><script>var x = 1;</script><title>T</title></head><body><p>text</p></body></html>", "text" },
		{ "comment", "a<!-- hidden -->b", "ab" },
		{ "space-run", "<p>a    b</p>", "a b" },
		{ "blank-lines", "a\n\nb", "a b" },
		{ "padded-block", "<p>  padded  </p>", "padded " },
		{ "inline-tags", "<p><b>bold</b> normal</p>", "bold normal" },
		{ "empty", "", "" },
		{ "plain", "just text", "just text" },
		{ "list", "<ul><li>one</li><li>two</li></ul>", "one\ntwo" },
		{ "bogus-entity", "&bogus; &#x41; &#66;", "&bogus; A B" },
		{ "table", "<table><tr><td>x</td><td>y</td></tr></table>", "\nx\ny\n" },
		{ "upper-tags", "<P>Hello</P>", "Hello" },
		{ "stray-lt", "a < b", "a " },
		{ "unterminated-tag", "a<b", "a" },
		{ "nested-blocks", "<div><p>a</p><p>b</p></div>", "a\nb" },
		{ "newline-in-block", "<p>line1\nline2</p>", "line1 line2" },
		{ "no-close", "text1<p>text2", "text1\ntext2" },
		{ "hr", "a<hr>b", "a\n\nb" },
		{ "heading", "<h1>Title</h1><p>Body</p>", "Title\nBody" },
		{ "pre", "<pre>  keep  spaces  </pre>", "  keep  spaces  " },
		{ "entity-sentence", "1 &lt; 2 &amp;&amp; 3 &gt; 2", "1 < 2 && 3 > 2" },
		{ "trailing-br", "a<br>", "a\n" },
		{ "leading-br", "<br>a", "\na" },
		{ "only-br", "<br>", "\n" },
		{ "only-br-br", "<br><br>", "\n\n" },
		{ "single-p", "<p>a</p>", "a" },
		{ "three-ps", "<p>a</p><p>b</p><p>c</p>", "a\nb\nc" },
		{ "empty-p", "<p></p>", "" },
		{ "p-empty-p", "<p>a</p><p></p><p>b</p>", "a\nb" },
		{ "text-after-pclose", "<p>a</p>b", "a\nb" },
		{ "text-before-popen", "a<p>b</p>", "a\nb" },
		{ "ws-between-blocks", "<p>a</p>   <p>b</p>", "a \nb" },
		{ "ws-both-sides", "<p>  a  </p>  <p>  b  </p>", "a \nb " },
		{ "ws-around-br", "a  <br>  b", "a \nb" },
		{ "doc-leading-ws", "   text", "text" },
		{ "doc-trailing-ws", "text   ", "text " },
		{ "div-p", "<p>a</p><div>b</div>", "a\nb" },
		{ "bare-div", "<div>a</div>", "a" },
		{ "bare-li", "<li>one</li>", "one" },
		{ "bare-h1", "<h1>a</h1>", "a" },
		{ "bare-td", "<td>x</td>", "\nx\n" },
		{ "bare-tr-td", "<tr><td>x</td></tr>", "\nx\n" },
		{ "table-single-cell", "<table><tr><td>x</td></tr></table>", "\nx\n" },
		{ "table-two-rows", "<table><tr><td>x</td><td>y</td></tr><tr><td>z</td></tr></table>", "\nx\ny\nz\n\n" },
		{ "list-then-text", "<ul><li>a</li><li>b</li></ul>after", "a\nb\nafter" },
		{ "text-then-list", "text<ul><li>a</li></ul>", "text\na" },
		{ "unclosed-ps", "<p>a<p>b", "a\nb" },
		{ "self-closing-br", "a<br/>b", "a\nb" },
		{ "attr-gt-in-quotes", "<p title=\"a>b\">text</p>", "text" },
		{ "style-then-text", "<style>x</style>text", "text" },
		{ "script-then-text", "<script>x</script>text", "text" },
		{ "title-then-text", "<title>T</title>text", "text" },
		{ "a-style-b", "a<style>x</style>b", "ab" },
		{ "a-title-b", "a<title>T</title>b", "ab" },
		{ "only-comment", "<!-- only comment -->", "" },
		{ "p-comment-p", "<p>a</p><!-- c --><p>b</p>", "a\nb" },
		{ "entity-upper", "&AMP;&NBSP;", "&&NBSP;" },
		{ "entity-no-semicolon", "a&ltb", "a&ltb" },
		{ "entity-numeric-nl", "&#10;", "" },
		{ "entity-numeric-emoji", "&#x1F600;", "\U0001f600" },
		{ "entity-numeric-overflow", "&#999999999;", "&#999999999;" },
		{ "inline-at-top", "<b>bold</b>", "bold" },
		{ "a-div-b-c", "a<div>b</div>c", "a\nbc" },
		{ "table-3rows-1cell", "<table><tr><td>a</td></tr><tr><td>b</td></tr><tr><td>c</td></tr></table>", "\na\nb\nc\n" },
		{ "table-2rows-2cells", "<table><tr><td>a</td><td>b</td></tr><tr><td>c</td><td>d</td></tr></table>", "\na\nb\nc\nd\n" },
		{ "table-text-after", "<table><tr><td>x</td></tr></table>a", "\nx\na" },
		{ "text-table", "a<table><tr><td>x</td></tr></table>", "a\nx\n" },
		{ "two-tables", "<table><tr><td>x</td></tr></table><table><tr><td>y</td></tr></table>", "\nx\n\ny\n" },
		{ "table-p", "<table><tr><td>x</td></tr></table><p>p</p>", "\nx\n\np" },
		{ "p-table", "<p>q</p><table><tr><td>x</td></tr></table>", "q\nx\n" },
		{ "empty-cell", "<table><tr><td></td><td>y</td></tr></table>", "\n\ny\n" },
		{ "hr-alone", "<hr>", "\n" },
		{ "a-hr", "a<hr>", "a\n" },
		{ "hr-a", "<hr>a", "\n\na" },
		{ "hr-hr", "a<hr><hr>b", "a\n\n\nb" },
		{ "nested-list", "<ul><li>a<ul><li>b</li></ul></li></ul>", "a\nb" },
		{ "li-br", "<li>a<br>b</li>", "a\nb" },
		{ "td-br", "<table><tr><td>x<br>z</td></tr></table>", "\nx\nz\n" },
		{ "bare-tds", "<td>x</td><td>y</td>", "\nx\ny\n" },
		{ "table-in-p", "<p>a<table><tr><td>x</td></tr></table>b</p>", "a\nx\nb" },
		{ "empty-p-then-text", "<p></p>a", "a" },
		{ "text-then-empty-p", "a<p></p>", "a\n" },
		{ "ragged-first-row", "<table><tr><td>a</td></tr><tr><td>b</td><td>c</td></tr></table>", "\na\n\nb\nc\n" },
		{ "td-multi-blocks", "<table><tr><td>a<p>b</p></td></tr></table>", "\na\nb\n" },
		{ "td-trailing-ws", "<table><tr><td>x  </td></tr></table>", "\nx \n" },
		{ "head-then-text", "<head>hidden</head>text", "text" },
		{ "style-attrs", "<style type=\"text/css\">x</style>text", "text" },
		{ "style-upper", "<STYLE>x</STYLE>text", "text" },
		{ "script-src", "<script src=\"x\">y</script>text", "text" },
		{ "ol-list", "<ol><li>a</li><li>b</li></ol>", "a\nb" },
		{ "pre-then-text", "<pre>a</pre>b", "a\nb" },
		{ "text-then-pre", "a<pre>b</pre>", "a\nb" },
		{ "pre-nl", "<pre>a\nb</pre>", "a\nb" },
		{ "pre-br", "<pre>a<br>b</pre>", "a\nb" },
		{ "pre-pre", "<pre>  a  </pre>  <pre>  b  </pre>", "  a   \n  b  " },
		{ "ws-after-last-close", "<p>a</p>  <p>b</p>  ", "a \nb " },
		{ "entity-nl-between", "a&#10;b", "a b" },
		{ "entity-nbsp-ws-run", "a &nbsp; b", "a   b" },
		{ "multiple-nbsp", "a&nbsp;&nbsp;b", "a  b" },
		{ "nbsp-leading", "&nbsp;a", " a" },
		{ "lt-digit", "a <3 b", "a " },
		{ "lt-space-gt", "a < b > c", "a c" },
		{ "dl-dt-dd", "<dl><dt>t</dt><dd>d</dd></dl>", "t\nd" },
		{ "h1-h2", "<h1>a</h1><h2>b</h2>", "a\nb" },
		{ "blockquote", "<blockquote>q</blockquote>", "q" },
		{ "tab-ws", "a\tb", "a b" },
		{ "cr-lf", "a\r\nb", "a b" },
		{ "uppercase-br", "a<BR>b", "a\nb" },
		{ "mixed-case-p", "<P>a</P>", "a" },
		{ "attr-single-quotes", "<p title='a>b'>text</p>", "text" },
		{ "attr-no-quotes", "<p title=a>b>text</p>", "b>text" },
		{ "inline-nested", "<p><b><i>x</i></b> y</p>", "x y" },
		{ "p-br-only", "<p><br></p>", "\n" },
		{ "td-empty-bare", "<td></td>", "\n\n" },
		{ "qrichtext-ws", "<meta name=\"qrichtext\" content=\"1\"><p>a  b</p>", "a  b" },
		{ "qrichtext-nl", "<meta name=\"qrichtext\" content=\"1\">a\nb", "ab" },
		{ "qrichtext-attr-order", "<meta content=\"1\" name=\"qrichtext\"><p>a  b</p>", "a b" },
		{ "qrichtext-br-ws", "<meta name=\"qrichtext\" content=\"1\">a<br> b", "a\n b" },
		{ "qrichtext-entity-nl", "<meta name=\"qrichtext\" content=\"1\">a&#10;b", "ab" },
		{ "caption", "<table><caption>C</caption><tr><td>x</td></tr></table>", "\nC\nx\n" },
		{ "thead-text", "<table><thead>h</thead><tr><td>x</td></tr></table>", "h\nx\n" },
		{ "colspan", "<table><tr><td colspan=\"2\">a</td></tr><tr><td>b</td><td>c</td></tr></table>", "\na\nb\nc\n" },
		{ "rowspan", "<table><tr><td rowspan=\"2\">a</td><td>b</td></tr><tr><td>c</td></tr></table>", "\na\nb\nc\n" },
		{ "startfragment", "x<!--StartFragment-->keep<!--EndFragment-->y", "keep" },
		{ "startfragment-qrichtext-outside", "x<meta name=\"qrichtext\" content=\"1\" /><!--StartFragment-->a\nb<!--EndFragment-->y", "ab" },
		{ "startfragment-qrichtext-inside", "x<!--StartFragment--><meta name=\"qrichtext\" content=\"1\" />a\nb<!--EndFragment-->y", "ab" },
		{ "img", "a<img src=\"x.png\" alt=\"hi\">b", "a\ufffcb" },
	};

	for ( const Case& row : cases ) {
		const QString sExpected = QString::fromUtf8( row.szExpected );
		const QString sGot = PluginUiLandingModel::htmlToPlainText(
			QString::fromUtf8( row.szHtml ) );
		CPPUNIT_ASSERT_MESSAGE(
			QString( "case [%1]: expected [%2], got [%3]" )
				.arg( row.szName )
				.arg( sExpected )
				.arg( sGot )
				.toStdString(),
			sGot == sExpected );
	}

	// A numeric entity for NUL decodes to a NUL character in the output —
	// kept out of the table because an embedded NUL ends any char*
	// literal.
	CPPUNIT_ASSERT( PluginUiLandingModel::htmlToPlainText( "&#0;" ) ==
					QStringLiteral( "\u0000" ) );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testLandingInfoSnapshot()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiLandingModel model( &plugin );

	// A model without a plugin reports the idle defaults instead of
	// crashing — and never any snapshot: there is no pipeline without
	// an engine.
	PluginUiLandingModel orphanModel( nullptr );
	const auto orphanInfo = orphanModel.infoSnapshot();
	CPPUNIT_ASSERT( orphanInfo.mode == Song::Mode::None );
	CPPUNIT_ASSERT( orphanInfo.sSongName.isEmpty() );
	CPPUNIT_ASSERT( orphanInfo.sDrumkitName.isEmpty() );
	CPPUNIT_ASSERT( orphanInfo.nPatternCount == 0 );
	CPPUNIT_ASSERT( orphanInfo.nColumnCount == 0 );
	CPPUNIT_ASSERT( orphanInfo.nCurrentColumn == -1 );
	CPPUNIT_ASSERT( orphanInfo.playingPatterns.empty() );
	CPPUNIT_ASSERT( orphanInfo.sPatternName.isEmpty() );
	CPPUNIT_ASSERT( orphanInfo.nPatternLength == 0 );
	CPPUNIT_ASSERT( orphanInfo.nPatternNoteCount == 0 );
	CPPUNIT_ASSERT( ! orphanModel.pollInfoChanged() );

	// Before the first delivery the cache holds the same idle defaults
	// (the dash rows) — a degraded first publish cycle can never blank
	// the section.
	const auto emptyInfo = model.infoSnapshot();
	CPPUNIT_ASSERT( emptyInfo.mode == Song::Mode::None );
	CPPUNIT_ASSERT( emptyInfo.sSongName.isEmpty() );
	CPPUNIT_ASSERT( emptyInfo.sDrumkitName.isEmpty() );
	CPPUNIT_ASSERT( emptyInfo.nPatternCount == 0 );
	CPPUNIT_ASSERT( emptyInfo.nColumnCount == 0 );
	CPPUNIT_ASSERT( emptyInfo.nCurrentColumn == -1 );
	CPPUNIT_ASSERT( emptyInfo.playingPatterns.empty() );
	CPPUNIT_ASSERT( emptyInfo.sPatternName.isEmpty() );
	CPPUNIT_ASSERT( emptyInfo.nPatternLength == 0 );
	CPPUNIT_ASSERT( emptyInfo.nPatternNoteCount == 0 );
	CPPUNIT_ASSERT( ! model.pollInfoChanged() );

	// The info data is pipeline-fed (ADR 0035 UI-3): the model caches
	// what the publish cycle delivers and never reads the engine
	// itself. The builder's field-for-field truth is covered by
	// EngineEventSinkTest::testInfoSnapshotFanout; here the injected
	// snapshots stand in for the bridge thread.
	EngineInfoSnapshot patternMode;
	patternMode.sSongName = "Empty song";
	patternMode.sDrumkitName = "GMRockKit";
	patternMode.mode = Song::Mode::Pattern;
	patternMode.sPatternName = "Pattern 1";
	patternMode.nPatternLength = 192;
	patternMode.nPatternNoteCount = 12;
	model.acceptInfoSnapshot( patternMode );
	CPPUNIT_ASSERT( model.infoSnapshot() == patternMode );
	// The displayed surface is still the idle defaults: one poll sees
	// the change, the next (nothing moved) does not.
	CPPUNIT_ASSERT( model.pollInfoChanged() );
	CPPUNIT_ASSERT( ! model.pollInfoChanged() );

	EngineInfoSnapshot songMode;
	songMode.sSongName = "Timeline and Time Signatures";
	songMode.sDrumkitName = "GMRockKit";
	songMode.mode = Song::Mode::Song;
	songMode.nPatternCount = 10;
	songMode.nColumnCount = 6;
	songMode.nCurrentColumn = 3;
	songMode.playingPatterns = { "Pattern A", "Pattern B" };
	model.acceptInfoSnapshot( songMode );
	CPPUNIT_ASSERT( model.infoSnapshot() == songMode );
	CPPUNIT_ASSERT( model.pollInfoChanged() );
	CPPUNIT_ASSERT( ! model.pollInfoChanged() );

	// The publish cycle repeats the identical snapshot every ~50 ms
	// while nothing changes — that is not a change.
	model.acceptInfoSnapshot( songMode );
	CPPUNIT_ASSERT( ! model.pollInfoChanged() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingModelRoundTrip()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiMappingModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.setInputMode(
		MidiInstrumentMap::Input::Custom ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );
	auto pPreferences = pHydrogen->getPreferences();
	CPPUNIT_ASSERT( pPreferences != nullptr );

	// Map-wide settings — distinctive from-default values, each applied
	// with one command and read back from the engine's live map.
	CPPUNIT_ASSERT( model.setInputMode( MidiInstrumentMap::Input::Custom ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()->getInput() ==
					MidiInstrumentMap::Input::Custom );
	CPPUNIT_ASSERT( model.setOutputMode(
		MidiInstrumentMap::Output::Constant ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()->getOutput() ==
					MidiInstrumentMap::Output::Constant );
	CPPUNIT_ASSERT( model.setUseGlobalInputChannel( true ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getUseGlobalInputChannel() );
	CPPUNIT_ASSERT( model.setGlobalInputChannel( Midi::Channel( 3 ) ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getGlobalInputChannel() == Midi::Channel( 3 ) );
	CPPUNIT_ASSERT( model.setUseGlobalOutputChannel( true ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getUseGlobalOutputChannel() );
	CPPUNIT_ASSERT( model.setGlobalOutputChannel( Midi::Channel( 7 ) ) );
	CPPUNIT_ASSERT( pPreferences->getMidiInstrumentMap()
					->getGlobalOutputChannel() == Midi::Channel( 7 ) );

	// Per-instrument custom input row — inserted through the model and
	// resolved back through the engine's live map.
	auto pDrumkit = pHydrogen->getSong()->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	auto pInstrument = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	CPPUNIT_ASSERT( model.insertCustomInputMapping(
		pInstrument, Midi::Note( 42 ), Midi::Channel( 3 ) ) );
	auto pLiveMap = pPreferences->getMidiInstrumentMap();
	CPPUNIT_ASSERT( pLiveMap->getCustomInputMappingsType().size() +
					pLiveMap->getCustomInputMappingsId().size() == 1 );
	const auto mapping = pLiveMap->getInputMapping(
		pInstrument, pDrumkit, pHydrogen );
	CPPUNIT_ASSERT( mapping.note == Midi::Note( 42 ) );
	CPPUNIT_ASSERT( mapping.channel == Midi::Channel( 3 ) );

	// Per-instrument output — lives on the instrument itself, keyed by
	// the instrument index within the current kit.
	CPPUNIT_ASSERT( model.setInstrumentMidiOutNote( 0, Midi::Note( 60 ) ) );
	CPPUNIT_ASSERT( model.setInstrumentMidiOutChannel( 0,
													   Midi::Channel( 9 ) ) );
	CPPUNIT_ASSERT( pInstrument->getMidiOutNote() == Midi::Note( 60 ) );
	CPPUNIT_ASSERT( pInstrument->getMidiOutChannel() == Midi::Channel( 9 ) );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingModelInstrumentReads()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// The default engine song carries a kit with instruments.
	CPPUNIT_ASSERT( model.instrumentCount() > 0 );
	auto pInstrument = plugin.getHydrogen()->getSong()->getDrumkit()
		->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	CPPUNIT_ASSERT( model.instrumentName( 0 ) == pInstrument->getName() );
	// Out of range — empty, not garbage.
	CPPUNIT_ASSERT( model.instrumentName( model.instrumentCount() ).isEmpty() );

	// The output note/channel reads follow the commands.
	CPPUNIT_ASSERT( model.setInstrumentMidiOutNote( 0, Midi::Note( 60 ) ) );
	CPPUNIT_ASSERT( model.instrumentMidiOutNote( 0 ) == Midi::Note( 60 ) );
	CPPUNIT_ASSERT( model.setInstrumentMidiOutChannel( 0,
													   Midi::Channel( 9 ) ) );
	CPPUNIT_ASSERT( model.instrumentMidiOutChannel( 0 ) ==
					Midi::Channel( 9 ) );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingModelInputMappingRead()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// Custom mode + a custom row for instrument 0, inserted by index —
	// the read resolves the effective incoming note/channel through
	// the engine's live map.
	CPPUNIT_ASSERT( model.setInputMode( MidiInstrumentMap::Input::Custom ) );
	CPPUNIT_ASSERT( model.instrumentCount() > 0 );
	CPPUNIT_ASSERT( model.insertCustomInputMapping(
		0, Midi::Note( 42 ), Midi::Channel( 3 ) ) );

	const auto mapping = model.inputMapping( 0 );
	CPPUNIT_ASSERT( mapping.note == Midi::Note( 42 ) );
	CPPUNIT_ASSERT( mapping.channel == Midi::Channel( 3 ) );

	// Out of range — invalid read, refused insert, no garbage.
	CPPUNIT_ASSERT( model.inputMapping( model.instrumentCount() ).isNull() );
	CPPUNIT_ASSERT( ! model.insertCustomInputMapping(
		model.instrumentCount(), Midi::Note( 42 ), Midi::Channel( 3 ) ) );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingMidiConfigNoWriteBack()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// The plugin never persists its runtime config: whatever the model
	// applies, the user config file must stay byte-identical (or stay
	// absent). Snapshot after construction so bootstrap noise cannot
	// mask the pin.
	const QString sConfigPath = Filesystem::userConfigPath();
	QByteArray sConfigBefore;
	bool bConfigExists = false;
	{
		QFile file( sConfigPath );
		if ( file.exists() ) {
			CPPUNIT_ASSERT( file.open( QIODevice::ReadOnly ) );
			sConfigBefore = file.readAll();
			file.close();
			bConfigExists = true;
		}
	}

	// Scalar MIDI control settings: applied to the plugin's engine
	// preferences (ADR 0022 plugin-instance layer). Distinctive values —
	// several differ from the defaults, so adoption cannot pass vacuously.
	CPPUNIT_ASSERT( model.applyMidiControlSettings(
		/*bNoteOffIgnore=*/false, Midi::Channel( 9 ),
		/*bEnableFeedback=*/true, /*bTransportInputHandling=*/false,
		/*bTransportOutputSend=*/true, Midi::Channel( 12 ),
		Preferences::MidiSendNoteOff::OnCustomLengths ) );
	auto pPref = plugin.getHydrogen()->getPreferences();
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( pPref->m_bMidiNoteOffIgnore == false );
	CPPUNIT_ASSERT( pPref->m_midiActionChannel == Midi::Channel( 9 ) );
	CPPUNIT_ASSERT( pPref->m_bEnableMidiFeedback == true );
	CPPUNIT_ASSERT( pPref->getMidiTransportInputHandling() == false );
	CPPUNIT_ASSERT( pPref->getMidiTransportOutputSend() == true );
	CPPUNIT_ASSERT( pPref->getMidiFeedbackChannel() == Midi::Channel( 12 ) );
	CPPUNIT_ASSERT( pPref->getMidiSendNoteOff() ==
					Preferences::MidiSendNoteOff::OnCustomLengths );

	// MIDI instrument map: a custom-mode map with one per-instrument row.
	auto pMap = std::make_shared<MidiInstrumentMap>();
	pMap->setInput( MidiInstrumentMap::Input::Custom );
	pMap->setUseGlobalInputChannel( true );
	pMap->setGlobalInputChannel( Midi::Channel( 5 ) );
	auto pInstrument = plugin.getHydrogen()->getSong()->getDrumkit()
		->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	pMap->insertCustomInputMapping( pInstrument, Midi::Note( 42 ),
									Midi::Channel( 5 ) );
	CPPUNIT_ASSERT( model.applyMidiInstrumentMap( pMap ) );

	auto pEngineMap = pPref->getMidiInstrumentMap();
	CPPUNIT_ASSERT( pEngineMap != nullptr );
	CPPUNIT_ASSERT( pEngineMap == pMap );
	CPPUNIT_ASSERT( pEngineMap->getInput() ==
					MidiInstrumentMap::Input::Custom );
	CPPUNIT_ASSERT( pEngineMap->getUseGlobalInputChannel() == true );
	CPPUNIT_ASSERT( pEngineMap->getGlobalInputChannel() ==
					Midi::Channel( 5 ) );
	CPPUNIT_ASSERT( pEngineMap->getCustomInputMappingsType().size() +
					pEngineMap->getCustomInputMappingsId().size() == 1 );

	// No write-back: the user config file is byte-identical (or still
	// absent) after everything the model applied.
	{
		QFile file( sConfigPath );
		if ( bConfigExists ) {
			CPPUNIT_ASSERT( file.exists() );
			CPPUNIT_ASSERT( file.open( QIODevice::ReadOnly ) );
			CPPUNIT_ASSERT( file.readAll() == sConfigBefore );
		}
		else {
			CPPUNIT_ASSERT( ! file.exists() );
		}
	}

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMappingMidiConfigExposedReadApply()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMappingModel model( &plugin );

	// Distinctive values on all seven settings, so neither the reads
	// nor the preservation pin below can pass vacuously.
	CPPUNIT_ASSERT( model.applyMidiControlSettings(
		/*bNoteOffIgnore=*/true, Midi::Channel( 3 ),
		/*bEnableFeedback=*/true, /*bTransportInputHandling=*/false,
		/*bTransportOutputSend=*/true, Midi::Channel( 12 ),
		Preferences::MidiSendNoteOff::Never ) );

	// Read side: the exposed widgets populate from the engine.
	CPPUNIT_ASSERT( model.midiNoteOffIgnore() == true );
	CPPUNIT_ASSERT( model.midiActionChannel() == Midi::Channel( 3 ) );
	CPPUNIT_ASSERT( model.midiSendNoteOff() ==
					Preferences::MidiSendNoteOff::Never );

	// Narrow apply: changes the exposed three, preserves the rest.
	CPPUNIT_ASSERT( model.applyExposedMidiControlSettings(
		/*bNoteOffIgnore=*/false, Midi::Channel( 9 ),
		Preferences::MidiSendNoteOff::OnCustomLengths ) );

	auto pPref = plugin.getHydrogen()->getPreferences();
	CPPUNIT_ASSERT( pPref != nullptr );
	CPPUNIT_ASSERT( pPref->m_bMidiNoteOffIgnore == false );
	CPPUNIT_ASSERT( pPref->m_midiActionChannel == Midi::Channel( 9 ) );
	CPPUNIT_ASSERT( pPref->getMidiSendNoteOff() ==
					Preferences::MidiSendNoteOff::OnCustomLengths );
	// The four settings the plugin UI does not expose (feedback,
	// transport) keep their engine values — the preservation pin.
	CPPUNIT_ASSERT( pPref->m_bEnableMidiFeedback == true );
	CPPUNIT_ASSERT( pPref->getMidiTransportInputHandling() == false );
	CPPUNIT_ASSERT( pPref->getMidiTransportOutputSend() == true );
	CPPUNIT_ASSERT( pPref->getMidiFeedbackChannel() == Midi::Channel( 12 ) );

	// The reads follow the narrow apply.
	CPPUNIT_ASSERT( model.midiNoteOffIgnore() == false );
	CPPUNIT_ASSERT( model.midiActionChannel() == Midi::Channel( 9 ) );
	CPPUNIT_ASSERT( model.midiSendNoteOff() ==
					Preferences::MidiSendNoteOff::OnCustomLengths );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testChannelDisplayName()
{
	___INFOLOG( "" );

	// The channel dropdowns' display convention (the GUI's channel
	// spinboxes show the same): -1 reads "All", 0 reads "Off", 1..16
	// read as themselves. The invalid -2 of unmapped rows reads "Off"
	// too — the GUI disables those widgets instead. A bare instance —
	// no engine needed.
	const CommonStrings strings;
	CPPUNIT_ASSERT( strings.getChannelDisplayName(
						Midi::ChannelAll ) == "All" );
	CPPUNIT_ASSERT( strings.getChannelDisplayName(
						Midi::ChannelOff ) == "Off" );
	CPPUNIT_ASSERT( strings.getChannelDisplayName(
						Midi::ChannelInvalid ) == "Off" );
	CPPUNIT_ASSERT( strings.getChannelDisplayName(
						Midi::Channel( 1 ) ) == "1" );
	CPPUNIT_ASSERT( strings.getChannelDisplayName(
						Midi::Channel( 16 ) ) == "16" );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testCreatePitchLabel()
{
	___INFOLOG( "" );

	// English is deterministic here: the suite installs no translator
	// (testTranslatorInstallation deliberately leaves it removed).
	const auto keyLabel = []( Note::Key key ) {
		return CommonStrings::createPitchLabel( key );
	};
	CPPUNIT_ASSERT( keyLabel( Note::Key::C ) == "C" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::Cs ) == "C#" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::D ) == "D" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::Ef ) == "D#" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::E ) == "E" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::F ) == "F" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::Fs ) == "F#" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::G ) == "G" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::Af ) == "G#" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::A ) == "A" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::Bf ) == "A#" );
	CPPUNIT_ASSERT( keyLabel( Note::Key::B ) == "B" );

	// The octave form appends the number, deliberately unclamped (the
	// header's doc: works outside Note::Octave's valid range).
	CPPUNIT_ASSERT( CommonStrings::createPitchLabel( Note::Key::C, 4 )
					== "C4" );
	CPPUNIT_ASSERT( CommonStrings::createPitchLabel( Note::Key::Bf, -1 )
					== "A#-1" );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMixerModelStripsAndMeters()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMixerModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiMixerModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.setStripVolume( 0, 0.5f ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	auto pDrumkit = pSong->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	auto pInstrument = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	// Strip commands land on the instrument.
	CPPUNIT_ASSERT( model.setStripVolume( 0, 0.42f ) );
	CPPUNIT_ASSERT( pInstrument->getVolume() == 0.42f );
	CPPUNIT_ASSERT( model.setStripPan( 0, -0.25f ) );
	CPPUNIT_ASSERT( pInstrument->getPan() == -0.25f );
	CPPUNIT_ASSERT( model.setStripIsMuted( 0, true ) );
	CPPUNIT_ASSERT( pInstrument->isMuted() );
	CPPUNIT_ASSERT( model.setStripIsSoloed( 0, true ) );
	CPPUNIT_ASSERT( pInstrument->isSoloed() );

	// Master commands land on the song.
	CPPUNIT_ASSERT( model.setMasterVolume( 0.7f ) );
	CPPUNIT_ASSERT( pSong->getVolume() == 0.7f );
	CPPUNIT_ASSERT( model.setMasterIsMuted( true ) );
	CPPUNIT_ASSERT( pSong->getIsMuted() );

	// Meters: no snapshot yet — reads are zeroed, not garbage.
	CPPUNIT_ASSERT( ! model.hasMeterSnapshot() );
	CPPUNIT_ASSERT( model.instrumentPeakL( 0 ) == 0.0f );
	CPPUNIT_ASSERT( model.masterPeakL() == 0.0f );

	// A snapshot accepted through the sink seam is what the reads serve.
	EngineTelemetrySnapshot snapshot;
	snapshot.masterPeakL = 0.7f;
	snapshot.masterPeakR = 0.8f;
	snapshot.instPeakCount = 2;
	snapshot.peakL[ 0 ] = 0.1f;
	snapshot.peakR[ 0 ] = 0.2f;
	snapshot.peakL[ 1 ] = 0.3f;
	snapshot.peakR[ 1 ] = 0.4f;
	model.acceptMeterSnapshot( snapshot );

	CPPUNIT_ASSERT( model.hasMeterSnapshot() );
	CPPUNIT_ASSERT( model.masterPeakL() == 0.7f );
	CPPUNIT_ASSERT( model.masterPeakR() == 0.8f );
	CPPUNIT_ASSERT( model.instrumentPeakL( 0 ) == 0.1f );
	CPPUNIT_ASSERT( model.instrumentPeakR( 0 ) == 0.2f );
	CPPUNIT_ASSERT( model.instrumentPeakL( 1 ) == 0.3f );
	CPPUNIT_ASSERT( model.instrumentPeakR( 1 ) == 0.4f );
	// Beyond the snapshot's instrument count — zeroed, not out-of-bounds.
	CPPUNIT_ASSERT( model.instrumentPeakL( 2 ) == 0.0f );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMixerMeterPoll()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMixerModel model( &plugin );

	// No snapshot yet: the displayed meters default to silence —
	// nothing to request a frame for.
	CPPUNIT_ASSERT( ! model.pollMetersChanged() );

	// The first snapshot (even an all-silent one) reshapes the strips:
	// the instrument count is part of the displayed surface.
	EngineTelemetrySnapshot first;
	first.instPeakCount = 2;
	model.acceptMeterSnapshot( first );
	CPPUNIT_ASSERT( model.pollMetersChanged() );
	CPPUNIT_ASSERT( ! model.pollMetersChanged() );

	// A meter move is a change; the identical snapshot repeated by the
	// next publish cycle is not.
	EngineTelemetrySnapshot moved = first;
	moved.peakL[ 1 ] = 0.5f;
	model.acceptMeterSnapshot( moved );
	CPPUNIT_ASSERT( model.pollMetersChanged() );
	model.acceptMeterSnapshot( moved );
	CPPUNIT_ASSERT( ! model.pollMetersChanged() );

	// Non-meter fields (transport position, tempo, process time) do
	// not re-render the mixer.
	EngineTelemetrySnapshot transport = moved;
	transport.frame = 12345;
	transport.bpm = 150.0;
	model.acceptMeterSnapshot( transport );
	CPPUNIT_ASSERT( ! model.pollMetersChanged() );

	// A kit change (instrument count) is a change even when all peaks
	// are silent.
	EngineTelemetrySnapshot resized = moved;
	resized.instPeakCount = 4;
	model.acceptMeterSnapshot( resized );
	CPPUNIT_ASSERT( model.pollMetersChanged() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testEngineSinkAdapter()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;

	// The window's ownership shape: the plugin (whose session outlives
	// the UI) is destroyed first, then the sink, then the models it
	// forwards into — so an assertion failure can never leave a
	// registered sink pointing at dead models. The optionals are
	// declared before the plugin to get that destruction order while
	// still constructing the models with the plugin pointer.
	std::optional<PluginUiLandingModel> landingModel;
	std::optional<PluginUiMixerModel> mixerModel;
	std::optional<PluginUiEngineSink> sink;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	landingModel.emplace( &plugin );
	mixerModel.emplace( &plugin );
	sink.emplace( *landingModel, *mixerModel );

	// Direct forwarding (the bridge thread's calls): meter snapshots
	// land in the mixer model, info snapshots in the landing model.
	EngineTelemetrySnapshot meters;
	meters.masterPeakL = 0.5f;
	meters.masterPeakR = 0.6f;
	meters.instPeakCount = 2;
	meters.peakL[ 0 ] = 0.25f;
	meters.peakR[ 0 ] = 0.3f;
	meters.peakL[ 1 ] = 0.75f;
	meters.peakR[ 1 ] = 0.8f;
	sink->onMeterSnapshot( meters );
	CPPUNIT_ASSERT( mixerModel->hasMeterSnapshot() );
	CPPUNIT_ASSERT( mixerModel->masterPeakL() == 0.5f );
	CPPUNIT_ASSERT( mixerModel->instrumentPeakL( 1 ) == 0.75f );

	EngineInfoSnapshot info;
	info.sSongName = "Direct";
	info.mode = Song::Mode::Pattern;
	sink->onInfoSnapshot( info );
	CPPUNIT_ASSERT( landingModel->infoSnapshot() == info );

	// Events are not the adapter's channel (pull-rendered UI): a
	// forwarded event must not disturb either cache.
	sink->onEvent( Event::Type::Metronome, 7, 0 );
	CPPUNIT_ASSERT( landingModel->infoSnapshot() == info );
	CPPUNIT_ASSERT( mixerModel->masterPeakL() == 0.5f );

	// End-to-end over the real pipeline: registered on the plugin, the
	// session's publish cycle feeds the models the engine's actual
	// state (the startup song: pattern mode, the default kit).
	plugin.setEventSink( &*sink );
	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );
	auto pSong = pHydrogen->getSong();
	CPPUNIT_ASSERT( pSong != nullptr );
	CPPUNIT_ASSERT( TestHelper::pumpUntil( [&]() {
		return landingModel->infoSnapshot().mode == Song::Mode::Pattern &&
			landingModel->infoSnapshot().sSongName == pSong->getName();
	} ) );
	CPPUNIT_ASSERT( TestHelper::pumpUntil(
		[&]() { return mixerModel->hasMeterSnapshot(); } ) );

	// The poll sees the pipeline's data as a change (the displayed
	// surface still holds the injected "Direct" snapshot).
	CPPUNIT_ASSERT( landingModel->pollInfoChanged() );
	CPPUNIT_ASSERT( mixerModel->pollMetersChanged() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMixerModelStripReads()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMixerModel model( &plugin );

	// The host context: two buses (ADR 0019).
	CPPUNIT_ASSERT( model.busCount() == 2 );

	// The default engine song carries a kit with instruments.
	CPPUNIT_ASSERT( model.stripCount() > 0 );
	auto pInstrument = plugin.getHydrogen()->getSong()->getDrumkit()
		->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );
	CPPUNIT_ASSERT( model.stripName( 0 ) == pInstrument->getName() );
	// Out of range — empty, not garbage.
	CPPUNIT_ASSERT( model.stripName( model.stripCount() ).isEmpty() );

	// The strip reads follow the commands.
	CPPUNIT_ASSERT( model.setStripVolume( 0, 0.42f ) );
	CPPUNIT_ASSERT( model.stripVolume( 0 ) == 0.42f );
	CPPUNIT_ASSERT( model.setStripPan( 0, -0.25f ) );
	CPPUNIT_ASSERT( model.stripPan( 0 ) == -0.25f );
	CPPUNIT_ASSERT( model.setStripIsMuted( 0, true ) );
	CPPUNIT_ASSERT( model.stripIsMuted( 0 ) );
	CPPUNIT_ASSERT( model.setStripIsSoloed( 0, true ) );
	CPPUNIT_ASSERT( model.stripIsSoloed( 0 ) );

	// The master reads follow the commands.
	CPPUNIT_ASSERT( model.setMasterVolume( 0.7f ) );
	CPPUNIT_ASSERT( model.masterVolume() == 0.7f );
	CPPUNIT_ASSERT( model.setMasterIsMuted( true ) );
	CPPUNIT_ASSERT( model.masterIsMuted() );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testMixerModelBusCommands()
{
	___INFOLOG( "" );

	const unsigned nSampleRate = 44100;
	const unsigned nBlock = 512;
	HydrogenPlugin plugin( nSampleRate, nBlock, 2 );
	plugin.activate( nSampleRate, nBlock );
	PluginUiMixerModel model( &plugin );

	// A model without a plugin refuses every command instead of crashing.
	PluginUiMixerModel orphanModel( nullptr );
	CPPUNIT_ASSERT( ! orphanModel.setInstrumentOutputBus( 0, 2 ) );

	auto pHydrogen = plugin.getHydrogen();
	CPPUNIT_ASSERT( pHydrogen != nullptr );
	auto pDrumkit = pHydrogen->getSong()->getDrumkit();
	CPPUNIT_ASSERT( pDrumkit != nullptr );
	auto pInstrument = pDrumkit->getInstruments()->get( 0 );
	CPPUNIT_ASSERT( pInstrument != nullptr );

	// Implicit default: no custom mapping stored, the effective bus is
	// the kit position (ADR 0019 1-to-1).
	CPPUNIT_ASSERT( pInstrument->getOutputBus() == -1 );
	CPPUNIT_ASSERT( model.instrumentOutputBus( 0 ) == -1 );
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 4 ) == 0 );

	// Explicit custom mapping lands on the engine's instrument.
	CPPUNIT_ASSERT( model.setInstrumentOutputBus( 0, 2 ) );
	CPPUNIT_ASSERT( pInstrument->getOutputBus() == 2 );
	CPPUNIT_ASSERT( model.instrumentOutputBus( 0 ) == 2 );
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 4 ) == 2 );

	// Beyond the host's bus count: no individual bus (master only).
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 2 ) == -1 );

	// Reset to the implicit 1-to-1 default.
	CPPUNIT_ASSERT( model.setInstrumentOutputBus( 0, -1 ) );
	CPPUNIT_ASSERT( pInstrument->getOutputBus() == -1 );
	CPPUNIT_ASSERT( model.effectiveOutputBus( 0, 4 ) == 0 );

	___INFOLOG( "passed" );
}

void PluginUiModelTest::testTranslatorInstallation()
{
	___INFOLOG( "" );

	// A language with a shipped catalog is forced — the suite's system
	// data folder is redirected to the repo's data/, so hydrogen_cs.qm is
	// there — keeping the test independent of the host machine's locale
	// (the CI drumkit-path lesson: no environment-dependent greens).
	const QStringList languages = QStringList() << "cs";

	CPPUNIT_ASSERT( HydrogenPlugin::installTranslator( languages ) );
	// Idempotent: a second call neither reports failure nor disturbs the
	// installed state.
	CPPUNIT_ASSERT( HydrogenPlugin::installTranslator( languages ) );

	QTranslator* pTranslator = HydrogenPlugin::translator();
	CPPUNIT_ASSERT( pTranslator != nullptr );
	// A loaded catalog is non-empty.
	CPPUNIT_ASSERT( ! pTranslator->isEmpty() );

	// QCoreApplication offers no "is installed" query; removeTranslator's
	// bool return is the only probe — true proves the wiring actually
	// installed this exact translator on the application instance.
	QCoreApplication* pApp = QCoreApplication::instance();
	CPPUNIT_ASSERT( pApp != nullptr );
	CPPUNIT_ASSERT( pApp->removeTranslator( pTranslator ) );

	// Deliberately left removed: the catalogs carry CommonStrings
	// translations (the plugin UI reads the same strings), so an
	// installed Czech catalog would translate them for every test
	// running later and make their assertions order-dependent. The
	// wiring reinstalls the translator on its next call (the
	// remove-probe above is self-healing).

	___INFOLOG( "passed" );
}
