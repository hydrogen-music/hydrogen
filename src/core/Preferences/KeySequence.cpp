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

#include <core/Preferences/KeySequence.h>

#include <QStringList>

#include <cstring>

// Qt::KeyboardModifier values (QtCore) — the four modifiers the
// portable-text format can represent. Every GUI producer
// (ShortcutCaptureDialog, MainForm) composes exactly these.
#include <QtGlobal>

namespace H2Core
{

namespace
{

struct KeyNameMap
{
	int nKey;
	const char* sName;
};

/** Key names for the portable-text format, sorted by combined value.
 *
 * Generated from Qt 5.15's Qt::Key enum: the portable name
 * QKeySequence::toString() emits where it is ASCII, and the enum name
 * for the keys Qt formats as non-ASCII or garbage (the Latin-1
 * supplement, dead keys, IME keys, the modifier key-codes,
 * Key_unknown). Enum entries aliasing the same value appear once.
 *
 * KeySequenceTest::testQnamespaceCompleteness pins this table against
 * the qnamespace.h of the Qt version being built — a Qt upgrade that
 * adds keys fails the test loudly instead of silently dropping them.
 */
const KeyNameMap keyNameMap[] = {
	{ 0x20, "Space" },
	{ 0x21, "!" },
	{ 0x22, "\"" },
	{ 0x23, "#" },
	{ 0x24, "$" },
	{ 0x25, "%" },
	{ 0x26, "&" },
	{ 0x27, "'" },
	{ 0x28, "(" },
	{ 0x29, ")" },
	{ 0x2A, "*" },
	{ 0x2B, "+" },
	{ 0x2C, "," },
	{ 0x2D, "-" },
	{ 0x2E, "." },
	{ 0x2F, "/" },
	{ 0x30, "0" },
	{ 0x31, "1" },
	{ 0x32, "2" },
	{ 0x33, "3" },
	{ 0x34, "4" },
	{ 0x35, "5" },
	{ 0x36, "6" },
	{ 0x37, "7" },
	{ 0x38, "8" },
	{ 0x39, "9" },
	{ 0x3A, ":" },
	{ 0x3B, ";" },
	{ 0x3C, "<" },
	{ 0x3D, "=" },
	{ 0x3E, ">" },
	{ 0x3F, "?" },
	{ 0x40, "@" },
	{ 0x41, "A" },
	{ 0x42, "B" },
	{ 0x43, "C" },
	{ 0x44, "D" },
	{ 0x45, "E" },
	{ 0x46, "F" },
	{ 0x47, "G" },
	{ 0x48, "H" },
	{ 0x49, "I" },
	{ 0x4A, "J" },
	{ 0x4B, "K" },
	{ 0x4C, "L" },
	{ 0x4D, "M" },
	{ 0x4E, "N" },
	{ 0x4F, "O" },
	{ 0x50, "P" },
	{ 0x51, "Q" },
	{ 0x52, "R" },
	{ 0x53, "S" },
	{ 0x54, "T" },
	{ 0x55, "U" },
	{ 0x56, "V" },
	{ 0x57, "W" },
	{ 0x58, "X" },
	{ 0x59, "Y" },
	{ 0x5A, "Z" },
	{ 0x5B, "[" },
	{ 0x5C, "\\" },
	{ 0x5D, "]" },
	{ 0x5E, "^" },
	{ 0x5F, "_" },
	{ 0x60, "`" },
	{ 0x7B, "{" },
	{ 0x7C, "|" },
	{ 0x7D, "}" },
	{ 0x7E, "~" },
	{ 0xA0, "nobreakspace" },
	{ 0xA1, "exclamdown" },
	{ 0xA2, "cent" },
	{ 0xA3, "sterling" },
	{ 0xA4, "currency" },
	{ 0xA5, "yen" },
	{ 0xA6, "brokenbar" },
	{ 0xA7, "section" },
	{ 0xA8, "diaeresis" },
	{ 0xA9, "copyright" },
	{ 0xAA, "ordfeminine" },
	{ 0xAB, "guillemotleft" },
	{ 0xAC, "notsign" },
	{ 0xAD, "hyphen" },
	{ 0xAE, "registered" },
	{ 0xAF, "macron" },
	{ 0xB0, "degree" },
	{ 0xB1, "plusminus" },
	{ 0xB2, "twosuperior" },
	{ 0xB3, "threesuperior" },
	{ 0xB4, "acute" },
	{ 0xB5, "mu" },
	{ 0xB6, "paragraph" },
	{ 0xB7, "periodcentered" },
	{ 0xB8, "cedilla" },
	{ 0xB9, "onesuperior" },
	{ 0xBA, "masculine" },
	{ 0xBB, "guillemotright" },
	{ 0xBC, "onequarter" },
	{ 0xBD, "onehalf" },
	{ 0xBE, "threequarters" },
	{ 0xBF, "questiondown" },
	{ 0xC0, "Agrave" },
	{ 0xC1, "Aacute" },
	{ 0xC2, "Acircumflex" },
	{ 0xC3, "Atilde" },
	{ 0xC4, "Adiaeresis" },
	{ 0xC5, "Aring" },
	{ 0xC6, "AE" },
	{ 0xC7, "Ccedilla" },
	{ 0xC8, "Egrave" },
	{ 0xC9, "Eacute" },
	{ 0xCA, "Ecircumflex" },
	{ 0xCB, "Ediaeresis" },
	{ 0xCC, "Igrave" },
	{ 0xCD, "Iacute" },
	{ 0xCE, "Icircumflex" },
	{ 0xCF, "Idiaeresis" },
	{ 0xD0, "ETH" },
	{ 0xD1, "Ntilde" },
	{ 0xD2, "Ograve" },
	{ 0xD3, "Oacute" },
	{ 0xD4, "Ocircumflex" },
	{ 0xD5, "Otilde" },
	{ 0xD6, "Odiaeresis" },
	{ 0xD7, "multiply" },
	{ 0xD8, "Ooblique" },
	{ 0xD9, "Ugrave" },
	{ 0xDA, "Uacute" },
	{ 0xDB, "Ucircumflex" },
	{ 0xDC, "Udiaeresis" },
	{ 0xDD, "Yacute" },
	{ 0xDE, "THORN" },
	{ 0xDF, "ssharp" },
	{ 0xF7, "division" },
	{ 0xFF, "ydiaeresis" },
	{ 0x1000000, "Esc" },
	{ 0x1000001, "Tab" },
	{ 0x1000002, "Backtab" },
	{ 0x1000003, "Backspace" },
	{ 0x1000004, "Return" },
	{ 0x1000005, "Enter" },
	{ 0x1000006, "Ins" },
	{ 0x1000007, "Del" },
	{ 0x1000008, "Pause" },
	{ 0x1000009, "Print" },
	{ 0x100000A, "SysReq" },
	{ 0x100000B, "Clear" },
	{ 0x1000010, "Home" },
	{ 0x1000011, "End" },
	{ 0x1000012, "Left" },
	{ 0x1000013, "Up" },
	{ 0x1000014, "Right" },
	{ 0x1000015, "Down" },
	{ 0x1000016, "PgUp" },
	{ 0x1000017, "PgDown" },
	{ 0x1000020, "Shift" },
	{ 0x1000021, "Control" },
	{ 0x1000022, "Meta" },
	{ 0x1000023, "Alt" },
	{ 0x1000024, "CapsLock" },
	{ 0x1000025, "NumLock" },
	{ 0x1000026, "ScrollLock" },
	{ 0x1000030, "F1" },
	{ 0x1000031, "F2" },
	{ 0x1000032, "F3" },
	{ 0x1000033, "F4" },
	{ 0x1000034, "F5" },
	{ 0x1000035, "F6" },
	{ 0x1000036, "F7" },
	{ 0x1000037, "F8" },
	{ 0x1000038, "F9" },
	{ 0x1000039, "F10" },
	{ 0x100003A, "F11" },
	{ 0x100003B, "F12" },
	{ 0x100003C, "F13" },
	{ 0x100003D, "F14" },
	{ 0x100003E, "F15" },
	{ 0x100003F, "F16" },
	{ 0x1000040, "F17" },
	{ 0x1000041, "F18" },
	{ 0x1000042, "F19" },
	{ 0x1000043, "F20" },
	{ 0x1000044, "F21" },
	{ 0x1000045, "F22" },
	{ 0x1000046, "F23" },
	{ 0x1000047, "F24" },
	{ 0x1000048, "F25" },
	{ 0x1000049, "F26" },
	{ 0x100004A, "F27" },
	{ 0x100004B, "F28" },
	{ 0x100004C, "F29" },
	{ 0x100004D, "F30" },
	{ 0x100004E, "F31" },
	{ 0x100004F, "F32" },
	{ 0x1000050, "F33" },
	{ 0x1000051, "F34" },
	{ 0x1000052, "F35" },
	{ 0x1000053, "Super_L" },
	{ 0x1000054, "Super_R" },
	{ 0x1000055, "Menu" },
	{ 0x1000056, "Hyper_L" },
	{ 0x1000057, "Hyper_R" },
	{ 0x1000058, "Help" },
	{ 0x1000059, "Direction_L" },
	{ 0x1000060, "Direction_R" },
	{ 0x1000061, "Back" },
	{ 0x1000062, "Forward" },
	{ 0x1000063, "Stop" },
	{ 0x1000064, "Refresh" },
	{ 0x1000070, "Volume Down" },
	{ 0x1000071, "Volume Mute" },
	{ 0x1000072, "Volume Up" },
	{ 0x1000073, "Bass Boost" },
	{ 0x1000074, "Bass Up" },
	{ 0x1000075, "Bass Down" },
	{ 0x1000076, "Treble Up" },
	{ 0x1000077, "Treble Down" },
	{ 0x1000080, "Media Play" },
	{ 0x1000081, "Media Stop" },
	{ 0x1000082, "Media Previous" },
	{ 0x1000083, "Media Next" },
	{ 0x1000084, "Media Record" },
	{ 0x1000085, "Media Pause" },
	{ 0x1000086, "Toggle Media Play/Pause" },
	{ 0x1000090, "Home Page" },
	{ 0x1000091, "Favorites" },
	{ 0x1000092, "Search" },
	{ 0x1000093, "Standby" },
	{ 0x1000094, "Open URL" },
	{ 0x10000A0, "Launch Mail" },
	{ 0x10000A1, "Launch Media" },
	{ 0x10000A2, "Launch (0)" },
	{ 0x10000A3, "Launch (1)" },
	{ 0x10000A4, "Launch (2)" },
	{ 0x10000A5, "Launch (3)" },
	{ 0x10000A6, "Launch (4)" },
	{ 0x10000A7, "Launch (5)" },
	{ 0x10000A8, "Launch (6)" },
	{ 0x10000A9, "Launch (7)" },
	{ 0x10000AA, "Launch (8)" },
	{ 0x10000AB, "Launch (9)" },
	{ 0x10000AC, "Launch (A)" },
	{ 0x10000AD, "Launch (B)" },
	{ 0x10000AE, "Launch (C)" },
	{ 0x10000AF, "Launch (D)" },
	{ 0x10000B0, "Launch (E)" },
	{ 0x10000B1, "Launch (F)" },
	{ 0x10000B2, "Monitor Brightness Up" },
	{ 0x10000B3, "Monitor Brightness Down" },
	{ 0x10000B4, "Keyboard Light On/Off" },
	{ 0x10000B5, "Keyboard Brightness Up" },
	{ 0x10000B6, "Keyboard Brightness Down" },
	{ 0x10000B7, "Power Off" },
	{ 0x10000B8, "Wake Up" },
	{ 0x10000B9, "Eject" },
	{ 0x10000BA, "Screensaver" },
	{ 0x10000BB, "WWW" },
	{ 0x10000BC, "Memo" },
	{ 0x10000BD, "LightBulb" },
	{ 0x10000BE, "Shop" },
	{ 0x10000BF, "History" },
	{ 0x10000C0, "Add Favorite" },
	{ 0x10000C1, "Hot Links" },
	{ 0x10000C2, "Adjust Brightness" },
	{ 0x10000C3, "Finance" },
	{ 0x10000C4, "Community" },
	{ 0x10000C5, "Media Rewind" },
	{ 0x10000C6, "Back Forward" },
	{ 0x10000C7, "Application Left" },
	{ 0x10000C8, "Application Right" },
	{ 0x10000C9, "Book" },
	{ 0x10000CA, "CD" },
	{ 0x10000CB, "Calculator" },
	{ 0x10000CC, "To-do list" },
	{ 0x10000CD, "Clear Grab" },
	{ 0x10000CE, "Close" },
	{ 0x10000CF, "Copy" },
	{ 0x10000D0, "Cut" },
	{ 0x10000D1, "Display" },
	{ 0x10000D2, "DOS" },
	{ 0x10000D3, "Documents" },
	{ 0x10000D4, "Spreadsheet" },
	{ 0x10000D5, "Browser" },
	{ 0x10000D6, "Game" },
	{ 0x10000D7, "Go" },
	{ 0x10000D8, "iTouch" },
	{ 0x10000D9, "Logoff" },
	{ 0x10000DA, "Market" },
	{ 0x10000DB, "Meeting" },
	{ 0x10000DC, "Keyboard Menu" },
	{ 0x10000DD, "Menu PB" },
	{ 0x10000DE, "My Sites" },
	{ 0x10000DF, "News" },
	{ 0x10000E0, "Home Office" },
	{ 0x10000E1, "Option" },
	{ 0x10000E2, "Paste" },
	{ 0x10000E3, "Phone" },
	{ 0x10000E4, "Calendar" },
	{ 0x10000E5, "Reply" },
	{ 0x10000E6, "Reload" },
	{ 0x10000E7, "Rotate Windows" },
	{ 0x10000E8, "Rotation PB" },
	{ 0x10000E9, "Rotation KB" },
	{ 0x10000EA, "Save" },
	{ 0x10000EB, "Send" },
	{ 0x10000EC, "Spellchecker" },
	{ 0x10000ED, "Split Screen" },
	{ 0x10000EE, "Support" },
	{ 0x10000EF, "Task Panel" },
	{ 0x10000F0, "Terminal" },
	{ 0x10000F1, "Tools" },
	{ 0x10000F2, "Travel" },
	{ 0x10000F3, "Video" },
	{ 0x10000F4, "Word Processor" },
	{ 0x10000F5, "XFer" },
	{ 0x10000F6, "Zoom In" },
	{ 0x10000F7, "Zoom Out" },
	{ 0x10000F8, "Away" },
	{ 0x10000F9, "Messenger" },
	{ 0x10000FA, "WebCam" },
	{ 0x10000FB, "Mail Forward" },
	{ 0x10000FC, "Pictures" },
	{ 0x10000FD, "Music" },
	{ 0x10000FE, "Battery" },
	{ 0x10000FF, "Bluetooth" },
	{ 0x1000100, "Wireless" },
	{ 0x1000101, "Ultra Wide Band" },
	{ 0x1000102, "Media Fast Forward" },
	{ 0x1000103, "Audio Repeat" },
	{ 0x1000104, "Audio Random Play" },
	{ 0x1000105, "Subtitle" },
	{ 0x1000106, "Audio Cycle Track" },
	{ 0x1000107, "Time" },
	{ 0x1000108, "Hibernate" },
	{ 0x1000109, "View" },
	{ 0x100010A, "Top Menu" },
	{ 0x100010B, "Power Down" },
	{ 0x100010C, "Suspend" },
	{ 0x100010D, "Adjust contrast" },
	{ 0x100010E, "Launch (G)" },
	{ 0x100010F, "Launch (H)" },
	{ 0x1000110, "Touchpad Toggle" },
	{ 0x1000111, "Touchpad On" },
	{ 0x1000112, "Touchpad Off" },
	{ 0x1000113, "Microphone Mute" },
	{ 0x1000114, "Red" },
	{ 0x1000115, "Green" },
	{ 0x1000116, "Yellow" },
	{ 0x1000117, "Blue" },
	{ 0x1000118, "Channel Up" },
	{ 0x1000119, "Channel Down" },
	{ 0x100011A, "Guide" },
	{ 0x100011B, "Info" },
	{ 0x100011C, "Settings" },
	{ 0x100011D, "Microphone Volume Up" },
	{ 0x100011E, "Microphone Volume Down" },
	{ 0x1000120, "New" },
	{ 0x1000121, "Open" },
	{ 0x1000122, "Find" },
	{ 0x1000123, "Undo" },
	{ 0x1000124, "Redo" },
	{ 0x1001103, "AltGr" },
	{ 0x1001120, "Multi_key" },
	{ 0x1001121, "Kanji" },
	{ 0x1001122, "Muhenkan" },
	{ 0x1001123, "Henkan" },
	{ 0x1001124, "Romaji" },
	{ 0x1001125, "Hiragana" },
	{ 0x1001126, "Katakana" },
	{ 0x1001127, "Hiragana Katakana" },
	{ 0x1001128, "Zenkaku" },
	{ 0x1001129, "Hankaku" },
	{ 0x100112A, "Zenkaku Hankaku" },
	{ 0x100112B, "Touroku" },
	{ 0x100112C, "Massyo" },
	{ 0x100112D, "Kana Lock" },
	{ 0x100112E, "Kana Shift" },
	{ 0x100112F, "Eisu Shift" },
	{ 0x1001130, "Eisu toggle" },
	{ 0x1001131, "Hangul" },
	{ 0x1001132, "Hangul Start" },
	{ 0x1001133, "Hangul End" },
	{ 0x1001134, "Hangul Hanja" },
	{ 0x1001135, "Hangul Jamo" },
	{ 0x1001136, "Hangul Romaja" },
	{ 0x1001137, "Code input" },
	{ 0x1001138, "Hangul Jeonja" },
	{ 0x1001139, "Hangul Banja" },
	{ 0x100113A, "Hangul PreHanja" },
	{ 0x100113B, "Hangul PostHanja" },
	{ 0x100113C, "SingleCandidate" },
	{ 0x100113D, "Multiple Candidate" },
	{ 0x100113E, "Previous Candidate" },
	{ 0x100113F, "Hangul Special" },
	{ 0x100117E, "Mode_switch" },
	{ 0x1001250, "Dead_Grave" },
	{ 0x1001251, "Dead_Acute" },
	{ 0x1001252, "Dead_Circumflex" },
	{ 0x1001253, "Dead_Tilde" },
	{ 0x1001254, "Dead_Macron" },
	{ 0x1001255, "Dead_Breve" },
	{ 0x1001256, "Dead_Abovedot" },
	{ 0x1001257, "Dead_Diaeresis" },
	{ 0x1001258, "Dead_Abovering" },
	{ 0x1001259, "Dead_Doubleacute" },
	{ 0x100125A, "Dead_Caron" },
	{ 0x100125B, "Dead_Cedilla" },
	{ 0x100125C, "Dead_Ogonek" },
	{ 0x100125D, "Dead_Iota" },
	{ 0x100125E, "Dead_Voiced_Sound" },
	{ 0x100125F, "Dead_Semivoiced_Sound" },
	{ 0x1001260, "Dead_Belowdot" },
	{ 0x1001261, "Dead_Hook" },
	{ 0x1001262, "Dead_Horn" },
	{ 0x1001263, "Dead_Stroke" },
	{ 0x1001264, "Dead_Abovecomma" },
	{ 0x1001265, "Dead_Abovereversedcomma" },
	{ 0x1001266, "Dead_Doublegrave" },
	{ 0x1001267, "Dead_Belowring" },
	{ 0x1001268, "Dead_Belowmacron" },
	{ 0x1001269, "Dead_Belowcircumflex" },
	{ 0x100126A, "Dead_Belowtilde" },
	{ 0x100126B, "Dead_Belowbreve" },
	{ 0x100126C, "Dead_Belowdiaeresis" },
	{ 0x100126D, "Dead_Invertedbreve" },
	{ 0x100126E, "Dead_Belowcomma" },
	{ 0x100126F, "Dead_Currency" },
	{ 0x1001280, "Dead_a" },
	{ 0x1001281, "Dead_A" },
	{ 0x1001282, "Dead_e" },
	{ 0x1001283, "Dead_E" },
	{ 0x1001284, "Dead_i" },
	{ 0x1001285, "Dead_I" },
	{ 0x1001286, "Dead_o" },
	{ 0x1001287, "Dead_O" },
	{ 0x1001288, "Dead_u" },
	{ 0x1001289, "Dead_U" },
	{ 0x100128A, "Dead_Small_Schwa" },
	{ 0x100128B, "Dead_Capital_Schwa" },
	{ 0x100128C, "Dead_Greek" },
	{ 0x1001290, "Dead_Lowline" },
	{ 0x1001291, "Dead_Aboveverticalline" },
	{ 0x1001292, "Dead_Belowverticalline" },
	{ 0x1001293, "Dead_Longsolidusoverlay" },
	{ 0x100FFFF, "MediaLast" },
	{ 0x1010000, "Select" },
	{ 0x1010001, "Yes" },
	{ 0x1010002, "No" },
	{ 0x1020001, "Cancel" },
	{ 0x1020002, "Printer" },
	{ 0x1020003, "Execute" },
	{ 0x1020004, "Sleep" },
	{ 0x1020005, "Play" },
	{ 0x1020006, "Zoom" },
	{ 0x1020007, "Jisho" },
	{ 0x1020008, "Oyayubi_Left" },
	{ 0x1020009, "Oyayubi_Right" },
	{ 0x102000A, "Exit" },
	{ 0x1100000, "Context1" },
	{ 0x1100001, "Context2" },
	{ 0x1100002, "Context3" },
	{ 0x1100003, "Context4" },
	{ 0x1100004, "Call" },
	{ 0x1100005, "Hangup" },
	{ 0x1100006, "Flip" },
	{ 0x1100007, "Toggle Call/Hangup" },
	{ 0x1100008, "Voice Dial" },
	{ 0x1100009, "Last Number Redial" },
	{ 0x1100020, "Camera Shutter" },
	{ 0x1100021, "Camera Focus" },
	{ 0x1FFFFFF, "unknown" },
};

struct ModifierMap
{
	const char* sPrefix;
	int nModifier;
};

/** Modifier prefixes in canonical emission order. */
const ModifierMap modifierMap[] = {
	{ "Meta+", Qt::MetaModifier },
	{ "Ctrl+", Qt::ControlModifier },
	{ "Alt+", Qt::AltModifier },
	{ "Shift+", Qt::ShiftModifier },
};

const int nModifierMask = Qt::ShiftModifier | Qt::ControlModifier
	| Qt::AltModifier | Qt::MetaModifier;

int lookupKey( const QString& sName )
{
	for ( const auto& entry : keyNameMap ) {
		if ( sName == QLatin1String( entry.sName ) ) {
			return entry.nKey;
		}
	}
	return -1;
}

const char* lookupName( int nKey )
{
	for ( const auto& entry : keyNameMap ) {
		if ( entry.nKey == nKey ) {
			return entry.sName;
		}
	}
	return nullptr;
}

}

KeySequence::KeySequence( int nKey1, int nKey2, int nKey3, int nKey4 )
{
	m_nKeys[ 0 ] = nKey1;
	m_nKeys[ 1 ] = nKey2;
	m_nKeys[ 2 ] = nKey3;
	m_nKeys[ 3 ] = nKey4;
}

KeySequence KeySequence::fromPortableText( const QString& sText )
{
	// "" is the canonical empty sequence, not an error.
	if ( sText.isEmpty() ) {
		return KeySequence();
	}

	const QStringList chords = sText.split( ", " );
	if ( chords.size() > 4 ) {
		return KeySequence();
	}

	int nKeys[ 4 ] = { 0, 0, 0, 0 };
	int nChord = 0;
	for ( const QString& sChord : chords ) {
		int nModifiers = 0;
		QString sKey = sChord;
		// Consume modifier prefixes in any order, but each one at most
		// once — a repeated modifier is malformed input.
		bool bConsumed = true;
		while ( bConsumed ) {
			bConsumed = false;
			for ( const auto& entry : modifierMap ) {
				if ( sKey.startsWith( QLatin1String( entry.sPrefix ) ) ) {
					if ( nModifiers & entry.nModifier ) {
						// Discard entries with duplicate modifiers
						return KeySequence();
					}
					nModifiers |= entry.nModifier;
					sKey = sKey.mid(
						static_cast<int>( strlen( entry.sPrefix ) ) );
					bConsumed = true;
					break;
				}
			}
		}

		int nKey;
		if ( sKey.isEmpty() ) {
			if ( nModifiers == 0 ) {
				// An empty chord is garbage, not the empty sequence.
				return KeySequence();
			}
			// A modifier-only chord ("Ctrl+") round-trips with key 0.
			nKey = 0;
		}
		else {
			nKey = lookupKey( sKey );
			if ( nKey < 0 ) {
				return KeySequence();
			}
		}
		nKeys[ nChord++ ] = nModifiers | nKey;
	}

	return KeySequence( nKeys[ 0 ], nKeys[ 1 ], nKeys[ 2 ], nKeys[ 3 ] );
}

QString KeySequence::toPortableText() const
{
	QStringList chords;
	for ( int ii = 0; ii < count(); ++ii ) {
		const int nCombined = m_nKeys[ ii ];
		QString sChord;
		for ( const auto& entry : modifierMap ) {
			if ( nCombined & entry.nModifier ) {
				sChord += QLatin1String( entry.sPrefix );
			}
		}
		// An untabled key part (0 for modifier-only chords, or bits
		// outside the four supported modifiers) formats as empty.
		const char* sName = lookupName( nCombined & ~nModifierMask );
		if ( sName != nullptr ) {
			sChord += QLatin1String( sName );
		}
		chords << sChord;
	}
	return chords.join( ", " );
}

bool KeySequence::isEmpty() const
{
	return m_nKeys[ 0 ] == 0;
}

int KeySequence::count() const
{
	if ( m_nKeys[ 0 ] == 0 ) {
		return 0;
	}
	if ( m_nKeys[ 1 ] == 0 ) {
		return 1;
	}
	if ( m_nKeys[ 2 ] == 0 ) {
		return 2;
	}
	if ( m_nKeys[ 3 ] == 0 ) {
		return 3;
	}
	return 4;
}

int KeySequence::operator[]( int nIndex ) const
{
	if ( nIndex < 0 || nIndex > 3 ) {
		return 0;
	}
	return m_nKeys[ nIndex ];
}

void KeySequence::setKey( int nIndex, int nValue )
{
	if ( nIndex < 0 || nIndex > 3 ) {
		return;
	}
	m_nKeys[ nIndex ] = nValue;
}

bool KeySequence::operator==( const KeySequence& other ) const
{
	for ( int ii = 0; ii < 4; ++ii ) {
		if ( m_nKeys[ ii ] != other.m_nKeys[ ii ] ) {
			return false;
		}
	}
	return true;
}

bool KeySequence::operator!=( const KeySequence& other ) const
{
	return ! ( *this == other );
}

bool KeySequence::operator<( const KeySequence& other ) const
{
	for ( int ii = 0; ii < 4; ++ii ) {
		if ( m_nKeys[ ii ] < other.m_nKeys[ ii ] ) {
			return true;
		}
		if ( m_nKeys[ ii ] > other.m_nKeys[ ii ] ) {
			return false;
		}
	}
	return false;
}

}
