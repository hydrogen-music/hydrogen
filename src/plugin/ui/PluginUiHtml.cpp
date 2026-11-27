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

#include "PluginUiHtml.h"

#include <QChar>
#include <QStringList>
#include <QStringView>

#include <algorithm>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace H2Core {

namespace {

// Qt's HTML subset as used by QTextDocumentFragment::fromHtml(). The plugin UI
// links QtCore only, so the GUI's rich-text path is unavailable; this is a
// text-only port of the parser/importer it relies on. Only what affects plain
// text is modelled: the element and entity tables, implicit table/row
// creation, block structure, and whitespace handling.

enum class DisplayMode {
	Block,
	Inline,
	Table,
	None
};

enum class HtmlElementId {
	Unknown = -1,
	Qt = 0,
	Body,
	A,
	Em,
	I,
	Big,
	Small,
	Strong,
	B,
	Cite,
	Address,
	Var,
	Dfn,
	H1,
	H2,
	H3,
	H4,
	H5,
	H6,
	P,
	Center,
	Font,
	Ul,
	Ol,
	Li,
	Code,
	Tt,
	Kbd,
	Samp,
	Img,
	Br,
	Hr,
	Sub,
	Sup,
	Pre,
	Blockquote,
	Head,
	Div,
	Span,
	Dl,
	Dt,
	Dd,
	U,
	S,
	Nobr,
	Table,
	Tr,
	Td,
	Th,
	Thead,
	Tbody,
	Tfoot,
	Caption,
	Html,
	Style,
	Title,
	Meta,
	Link,
	Script
};

enum class WhiteSpaceMode {
	Normal,
	Pre,
	NoWrap,
	PreWrap,
	PreLine
};

enum class CompressWhitespace {
	Remove,
	Collapse,
	Preserve
};

struct HtmlEntity {
	const char* szName;
	ushort nCode;
};

// Sorted by name, mirroring QTextHtmlParser's table.
static const HtmlEntity s_entities[] = {
	{ "AElig", 198 },
	{ "AMP", 38 },
	{ "Aacute", 193 },
	{ "Acirc", 194 },
	{ "Agrave", 192 },
	{ "Alpha", 913 },
	{ "Aring", 197 },
	{ "Atilde", 195 },
	{ "Auml", 196 },
	{ "Beta", 914 },
	{ "Ccedil", 199 },
	{ "Chi", 935 },
	{ "Dagger", 8225 },
	{ "Delta", 916 },
	{ "ETH", 208 },
	{ "Eacute", 201 },
	{ "Ecirc", 202 },
	{ "Egrave", 200 },
	{ "Epsilon", 917 },
	{ "Eta", 919 },
	{ "Euml", 203 },
	{ "GT", 62 },
	{ "Gamma", 915 },
	{ "Iacute", 205 },
	{ "Icirc", 206 },
	{ "Igrave", 204 },
	{ "Iota", 921 },
	{ "Iuml", 207 },
	{ "Kappa", 922 },
	{ "LT", 60 },
	{ "Lambda", 923 },
	{ "Mu", 924 },
	{ "Ntilde", 209 },
	{ "Nu", 925 },
	{ "OElig", 338 },
	{ "Oacute", 211 },
	{ "Ocirc", 212 },
	{ "Ograve", 210 },
	{ "Omega", 937 },
	{ "Omicron", 927 },
	{ "Oslash", 216 },
	{ "Otilde", 213 },
	{ "Ouml", 214 },
	{ "Phi", 934 },
	{ "Pi", 928 },
	{ "Prime", 8243 },
	{ "Psi", 936 },
	{ "QUOT", 34 },
	{ "Rho", 929 },
	{ "Scaron", 352 },
	{ "Sigma", 931 },
	{ "THORN", 222 },
	{ "Tau", 932 },
	{ "Theta", 920 },
	{ "Uacute", 218 },
	{ "Ucirc", 219 },
	{ "Ugrave", 217 },
	{ "Upsilon", 933 },
	{ "Uuml", 220 },
	{ "Xi", 926 },
	{ "Yacute", 221 },
	{ "Yuml", 376 },
	{ "Zeta", 918 },
	{ "aacute", 225 },
	{ "acirc", 226 },
	{ "acute", 180 },
	{ "aelig", 230 },
	{ "agrave", 224 },
	{ "alefsym", 8501 },
	{ "alpha", 945 },
	{ "amp", 38 },
	{ "and", 8869 },
	{ "ang", 8736 },
	{ "apos", 39 },
	{ "aring", 229 },
	{ "asymp", 8776 },
	{ "atilde", 227 },
	{ "auml", 228 },
	{ "bdquo", 8222 },
	{ "beta", 946 },
	{ "brvbar", 166 },
	{ "bull", 8226 },
	{ "cap", 8745 },
	{ "ccedil", 231 },
	{ "cedil", 184 },
	{ "cent", 162 },
	{ "chi", 967 },
	{ "circ", 710 },
	{ "clubs", 9827 },
	{ "cong", 8773 },
	{ "copy", 169 },
	{ "crarr", 8629 },
	{ "cup", 8746 },
	{ "curren", 164 },
	{ "dArr", 8659 },
	{ "dagger", 8224 },
	{ "darr", 8595 },
	{ "deg", 176 },
	{ "delta", 948 },
	{ "diams", 9830 },
	{ "divide", 247 },
	{ "eacute", 233 },
	{ "ecirc", 234 },
	{ "egrave", 232 },
	{ "empty", 8709 },
	{ "emsp", 8195 },
	{ "ensp", 8194 },
	{ "epsilon", 949 },
	{ "equiv", 8801 },
	{ "eta", 951 },
	{ "eth", 240 },
	{ "euml", 235 },
	{ "euro", 8364 },
	{ "exist", 8707 },
	{ "fnof", 402 },
	{ "forall", 8704 },
	{ "frac12", 189 },
	{ "frac14", 188 },
	{ "frac34", 190 },
	{ "frasl", 8260 },
	{ "gamma", 947 },
	{ "ge", 8805 },
	{ "gt", 62 },
	{ "hArr", 8660 },
	{ "harr", 8596 },
	{ "hearts", 9829 },
	{ "hellip", 8230 },
	{ "iacute", 237 },
	{ "icirc", 238 },
	{ "iexcl", 161 },
	{ "igrave", 236 },
	{ "image", 8465 },
	{ "infin", 8734 },
	{ "int", 8747 },
	{ "iota", 953 },
	{ "iquest", 191 },
	{ "isin", 8712 },
	{ "iuml", 239 },
	{ "kappa", 954 },
	{ "lArr", 8656 },
	{ "lambda", 955 },
	{ "lang", 9001 },
	{ "laquo", 171 },
	{ "larr", 8592 },
	{ "lceil", 8968 },
	{ "ldquo", 8220 },
	{ "le", 8804 },
	{ "lfloor", 8970 },
	{ "lowast", 8727 },
	{ "loz", 9674 },
	{ "lrm", 8206 },
	{ "lsaquo", 8249 },
	{ "lsquo", 8216 },
	{ "lt", 60 },
	{ "macr", 175 },
	{ "mdash", 8212 },
	{ "micro", 181 },
	{ "middot", 183 },
	{ "minus", 8722 },
	{ "mu", 956 },
	{ "nabla", 8711 },
	{ "nbsp", 160 },
	{ "ndash", 8211 },
	{ "ne", 8800 },
	{ "ni", 8715 },
	{ "not", 172 },
	{ "notin", 8713 },
	{ "nsub", 8836 },
	{ "ntilde", 241 },
	{ "nu", 957 },
	{ "oacute", 243 },
	{ "ocirc", 244 },
	{ "oelig", 339 },
	{ "ograve", 242 },
	{ "oline", 8254 },
	{ "omega", 969 },
	{ "omicron", 959 },
	{ "oplus", 8853 },
	{ "or", 8870 },
	{ "ordf", 170 },
	{ "ordm", 186 },
	{ "oslash", 248 },
	{ "otilde", 245 },
	{ "otimes", 8855 },
	{ "ouml", 246 },
	{ "para", 182 },
	{ "part", 8706 },
	{ "percnt", 37 },
	{ "permil", 8240 },
	{ "perp", 8869 },
	{ "phi", 966 },
	{ "pi", 960 },
	{ "piv", 982 },
	{ "plusmn", 177 },
	{ "pound", 163 },
	{ "prime", 8242 },
	{ "prod", 8719 },
	{ "prop", 8733 },
	{ "psi", 968 },
	{ "quot", 34 },
	{ "rArr", 8658 },
	{ "radic", 8730 },
	{ "rang", 9002 },
	{ "raquo", 187 },
	{ "rarr", 8594 },
	{ "rceil", 8969 },
	{ "rdquo", 8221 },
	{ "real", 8476 },
	{ "reg", 174 },
	{ "rfloor", 8971 },
	{ "rho", 961 },
	{ "rlm", 8207 },
	{ "rsaquo", 8250 },
	{ "rsquo", 8217 },
	{ "sbquo", 8218 },
	{ "scaron", 353 },
	{ "sdot", 8901 },
	{ "sect", 167 },
	{ "shy", 173 },
	{ "sigma", 963 },
	{ "sigmaf", 962 },
	{ "sim", 8764 },
	{ "spades", 9824 },
	{ "sub", 8834 },
	{ "sube", 8838 },
	{ "sum", 8721 },
	{ "sup", 8835 },
	{ "sup1", 185 },
	{ "sup2", 178 },
	{ "sup3", 179 },
	{ "supe", 8839 },
	{ "szlig", 223 },
	{ "tau", 964 },
	{ "there4", 8756 },
	{ "theta", 952 },
	{ "thetasym", 977 },
	{ "thinsp", 8201 },
	{ "thorn", 254 },
	{ "tilde", 732 },
	{ "times", 215 },
	{ "trade", 8482 },
	{ "uArr", 8657 },
	{ "uacute", 250 },
	{ "uarr", 8593 },
	{ "ucirc", 251 },
	{ "ugrave", 249 },
	{ "uml", 168 },
	{ "upsih", 978 },
	{ "upsilon", 965 },
	{ "uuml", 252 },
	{ "weierp", 8472 },
	{ "xi", 958 },
	{ "yacute", 253 },
	{ "yen", 165 },
	{ "yuml", 255 },
	{ "zeta", 950 },
	{ "zwj", 8205 },
	{ "zwnj", 8204 },
};

// Mirrors the size of Qt's entities table (MAX_ENTITY) — a mismatch
// means a transcription slip.
static_assert( sizeof( s_entities ) / sizeof( s_entities[ 0 ] ) == 258 );

// Sorted by name, mirroring QTextHtmlParser's table.
static const struct HtmlElement {
	const char* szName;
	HtmlElementId id;
	DisplayMode displayMode;
} s_elements[] = {
	{ "a", HtmlElementId::A, DisplayMode::Inline },
	{ "address", HtmlElementId::Address, DisplayMode::Inline },
	{ "b", HtmlElementId::B, DisplayMode::Inline },
	{ "big", HtmlElementId::Big, DisplayMode::Inline },
	{ "blockquote", HtmlElementId::Blockquote, DisplayMode::Block },
	{ "body", HtmlElementId::Body, DisplayMode::Block },
	{ "br", HtmlElementId::Br, DisplayMode::Inline },
	{ "caption", HtmlElementId::Caption, DisplayMode::Block },
	{ "center", HtmlElementId::Center, DisplayMode::Block },
	{ "cite", HtmlElementId::Cite, DisplayMode::Inline },
	{ "code", HtmlElementId::Code, DisplayMode::Inline },
	{ "dd", HtmlElementId::Dd, DisplayMode::Block },
	{ "dfn", HtmlElementId::Dfn, DisplayMode::Inline },
	{ "div", HtmlElementId::Div, DisplayMode::Block },
	{ "dl", HtmlElementId::Dl, DisplayMode::Block },
	{ "dt", HtmlElementId::Dt, DisplayMode::Block },
	{ "em", HtmlElementId::Em, DisplayMode::Inline },
	{ "font", HtmlElementId::Font, DisplayMode::Inline },
	{ "h1", HtmlElementId::H1, DisplayMode::Block },
	{ "h2", HtmlElementId::H2, DisplayMode::Block },
	{ "h3", HtmlElementId::H3, DisplayMode::Block },
	{ "h4", HtmlElementId::H4, DisplayMode::Block },
	{ "h5", HtmlElementId::H5, DisplayMode::Block },
	{ "h6", HtmlElementId::H6, DisplayMode::Block },
	{ "head", HtmlElementId::Head, DisplayMode::None },
	{ "hr", HtmlElementId::Hr, DisplayMode::Block },
	{ "html", HtmlElementId::Html, DisplayMode::Inline },
	{ "i", HtmlElementId::I, DisplayMode::Inline },
	{ "img", HtmlElementId::Img, DisplayMode::Inline },
	{ "kbd", HtmlElementId::Kbd, DisplayMode::Inline },
	{ "li", HtmlElementId::Li, DisplayMode::Block },
	{ "link", HtmlElementId::Link, DisplayMode::None },
	{ "meta", HtmlElementId::Meta, DisplayMode::None },
	{ "nobr", HtmlElementId::Nobr, DisplayMode::Inline },
	{ "ol", HtmlElementId::Ol, DisplayMode::Block },
	{ "p", HtmlElementId::P, DisplayMode::Block },
	{ "pre", HtmlElementId::Pre, DisplayMode::Block },
	{ "qt", HtmlElementId::Body, DisplayMode::Block },
	{ "s", HtmlElementId::S, DisplayMode::Inline },
	{ "samp", HtmlElementId::Samp, DisplayMode::Inline },
	{ "script", HtmlElementId::Script, DisplayMode::None },
	{ "small", HtmlElementId::Small, DisplayMode::Inline },
	{ "span", HtmlElementId::Span, DisplayMode::Inline },
	{ "strong", HtmlElementId::Strong, DisplayMode::Inline },
	{ "style", HtmlElementId::Style, DisplayMode::None },
	{ "sub", HtmlElementId::Sub, DisplayMode::Inline },
	{ "sup", HtmlElementId::Sup, DisplayMode::Inline },
	{ "table", HtmlElementId::Table, DisplayMode::Table },
	{ "tbody", HtmlElementId::Tbody, DisplayMode::Table },
	{ "td", HtmlElementId::Td, DisplayMode::Block },
	{ "tfoot", HtmlElementId::Tfoot, DisplayMode::Table },
	{ "th", HtmlElementId::Th, DisplayMode::Block },
	{ "thead", HtmlElementId::Thead, DisplayMode::Table },
	{ "title", HtmlElementId::Title, DisplayMode::None },
	{ "tr", HtmlElementId::Tr, DisplayMode::Table },
	{ "tt", HtmlElementId::Tt, DisplayMode::Inline },
	{ "u", HtmlElementId::U, DisplayMode::Inline },
	{ "ul", HtmlElementId::Ul, DisplayMode::Block },
	{ "var", HtmlElementId::Var, DisplayMode::Inline },
};

// Mirrors the size of Qt's elements[Html_NumElements] table.
static_assert( sizeof( s_elements ) / sizeof( s_elements[ 0 ] ) == 59 );

static const ushort s_windowsLatin1ExtendedCharacters[0xA0 - 0x80] = {
	0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
	0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
	0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
	0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
};

static const HtmlElement* lookupHtmlElement( const QString& sTag )
{
	for ( const auto& element : s_elements ) {
		if ( sTag == QLatin1String( element.szName ) ) {
			return &element;
		}
	}
	return nullptr;
}

static QString resolveHtmlEntity( QStringView entity )
{
	const QString sEntity = entity.toString();
	for ( const auto& candidate : s_entities ) {
		if ( sEntity == QLatin1String( candidate.szName ) ) {
			return QString( QChar( candidate.nCode ) );
		}
	}

	if ( sEntity.size() > 1 && sEntity.at( 0 ) == QLatin1Char( '#' ) ) {
		QString sNumber = sEntity.mid( 1 );
		int nBase = 10;
		bool bOk = false;
		if ( ! sNumber.isEmpty() &&
			 sNumber.at( 0 ).toLower() == QLatin1Char( 'x' ) ) {
			sNumber = sNumber.mid( 1 );
			nBase = 16;
		}
		const uint nCode = sNumber.toUInt( &bOk, nBase );
		if ( bOk ) {
			uint nResolved = nCode;
			if ( nResolved >= 0x80 &&
				 nResolved < 0x80 +
				 sizeof( s_windowsLatin1ExtendedCharacters ) /
				 sizeof( s_windowsLatin1ExtendedCharacters[ 0 ] ) ) {
				nResolved =
					s_windowsLatin1ExtendedCharacters[ nResolved - 0x80 ];
			}
			if ( nResolved <= 0xffff ) {
				return QString( QChar( static_cast<ushort>( nResolved ) ) );
			}
			if ( nResolved <= 0x10ffff ) {
				return QString::fromUcs4( &nResolved, 1 );
			}
		}
	}
	return QString();
}

struct HtmlNode {
	QString sTag;
	QString sText;
	QStringList attributes;
	int nParent = 0;
	std::vector<int> children;
	HtmlElementId id = HtmlElementId::Unknown;
	DisplayMode displayMode = DisplayMode::Inline;
	WhiteSpaceMode whiteSpace = WhiteSpaceMode::Normal;
	QString sImageSource;
	QString sImageAlt;
	int nRowSpan = 1;
	int nColSpan = 1;

	bool isBlock() const { return displayMode == DisplayMode::Block; }
	bool isTableCell() const
	{
		return id == HtmlElementId::Td || id == HtmlElementId::Th;
	}
	bool mayNotHaveChildren() const
	{
		return id == HtmlElementId::Img || id == HtmlElementId::Hr ||
			id == HtmlElementId::Br || id == HtmlElementId::Meta;
	}
	bool hasOnlyWhitespace() const
	{
		for ( const QChar& character : sText ) {
			if ( ! character.isSpace() ||
				 character == QChar::LineSeparator ) {
				return false;
			}
		}
		return true;
	}
};

class HtmlParser {
public:
	explicit HtmlParser( const QString& sHtml )
		: m_sHtml( sHtml ), m_nLength( static_cast<int>( sHtml.size() ) )
	{
		m_nodes.emplace_back();
	}

	void parse()
	{
		while ( m_nPos < m_nLength ) {
			const QChar character = m_sHtml.at( m_nPos++ );
			if ( character == QLatin1Char( '<' ) ) {
				parseTag();
			}
			else if ( character == QLatin1Char( '&' ) ) {
				m_nodes.back().sText += parseEntity();
			}
			else {
				m_nodes.back().sText += character;
			}
		}
	}

	const std::vector<HtmlNode>& nodes() const { return m_nodes; }
	bool textEditMode() const { return m_bTextEditMode; }

private:
	bool hasPrefix( QChar character, int nOffset = 0 ) const
	{
		return m_nPos + nOffset < m_nLength &&
			m_sHtml.at( m_nPos + nOffset ) == character;
	}

	void eatSpace()
	{
		while ( m_nPos < m_nLength && m_sHtml.at( m_nPos ).isSpace() &&
				m_sHtml.at( m_nPos ) != QChar::ParagraphSeparator ) {
			++m_nPos;
		}
	}

	QString parseWord()
	{
		QString sWord;
		if ( hasPrefix( QLatin1Char( '"' ) ) ) {
			++m_nPos;
			while ( m_nPos < m_nLength ) {
				const QChar character = m_sHtml.at( m_nPos++ );
				if ( character == QLatin1Char( '"' ) ) {
					break;
				}
				else if ( character == QLatin1Char( '&' ) ) {
					sWord += parseEntity();
				}
				else {
					sWord += character;
				}
			}
		}
		else if ( hasPrefix( QLatin1Char( '\'' ) ) ) {
			++m_nPos;
			while ( m_nPos < m_nLength ) {
				const QChar character = m_sHtml.at( m_nPos++ );
				if ( character == QLatin1Char( '\'' ) &&
					 m_nPos > 1 &&
					 m_sHtml.at( m_nPos - 2 ) != QLatin1Char( '\\' ) ) {
					break;
				}
				sWord += character;
			}
		}
		else {
			while ( m_nPos < m_nLength ) {
				const QChar character = m_sHtml.at( m_nPos++ );
				if ( character == QLatin1Char( '>' ) ||
					 ( character == QLatin1Char( '/' ) &&
					   m_nPos < m_nLength &&
					   m_sHtml.at( m_nPos ) == QLatin1Char( '>' ) ) ||
					 character == QLatin1Char( '<' ) ||
					 character == QLatin1Char( '=' ) ||
					 character.isSpace() ) {
					--m_nPos;
					break;
				}
				if ( character == QLatin1Char( '&' ) ) {
					sWord += parseEntity();
				}
				else {
					sWord += character;
				}
			}
		}
		return sWord;
	}

	QString parseEntity()
	{
		const int nRecover = m_nPos;
		int nEntityLength = 0;
		while ( m_nPos < m_nLength ) {
			const QChar character = m_sHtml.at( m_nPos++ );
			if ( character.isSpace() || m_nPos - nRecover > 9 ) {
				m_nPos = nRecover;
				return QStringLiteral( "&" );
			}
			if ( character == QLatin1Char( ';' ) ) {
				break;
			}
			++nEntityLength;
		}
		if ( nEntityLength > 0 ) {
			const QString sParsed = resolveHtmlEntity( QStringView(
				m_sHtml.constData() + nRecover, nEntityLength ) );
			if ( ! sParsed.isNull() ) {
				return sParsed;
			}
		}
		m_nPos = nRecover;
		return QStringLiteral( "&" );
	}

	QStringList parseAttributes()
	{
		QStringList attributes;
		while ( m_nPos < m_nLength ) {
			eatSpace();
			if ( hasPrefix( QLatin1Char( '>' ) ) ||
				 hasPrefix( QLatin1Char( '/' ) ) ) {
				break;
			}
			const QString sKey = parseWord().toLower();
			QString sValue = QStringLiteral( "1" );
			if ( sKey.isEmpty() ) {
				break;
			}
			eatSpace();
			if ( hasPrefix( QLatin1Char( '=' ) ) ) {
				++m_nPos;
				eatSpace();
				sValue = parseWord();
			}
			if ( sValue.isEmpty() ) {
				continue;
			}
			attributes << sKey << sValue;
		}
		return attributes;
	}

	void applyAttributes()
	{
		HtmlNode& node = m_nodes.back();
		if ( node.attributes.size() % 2 != 0 ) {
			return;
		}
		bool bSeenQt3RichText = false;
		for ( int ii = 0; ii < node.attributes.size(); ii += 2 ) {
			const QString& sKey = node.attributes.at( ii );
			const QString& sValue = node.attributes.at( ii + 1 );
			if ( node.id == HtmlElementId::Meta ) {
				if ( sKey == QStringLiteral( "name" ) &&
					 sValue == QStringLiteral( "qrichtext" ) ) {
					bSeenQt3RichText = true;
				}
				if ( sKey == QStringLiteral( "content" ) &&
					 sValue == QStringLiteral( "1" ) &&
					 bSeenQt3RichText ) {
					m_bTextEditMode = true;
				}
			}
			else if ( node.id == HtmlElementId::Img ) {
				if ( sKey == QStringLiteral( "src" ) ||
					 sKey == QStringLiteral( "source" ) ) {
					node.sImageSource = sValue;
				}
				else if ( sKey == QStringLiteral( "alt" ) ) {
					node.sImageAlt = sValue;
				}
				else if ( sKey == QStringLiteral( "title" ) ) {
					node.sText = sValue;
				}
			}
			else if ( node.id == HtmlElementId::Td ||
					  node.id == HtmlElementId::Th ) {
				bool bOk = false;
				if ( sKey == QStringLiteral( "rowspan" ) ) {
					const int nValue = sValue.toInt( &bOk );
					if ( bOk ) {
						node.nRowSpan = std::max( 1, nValue );
					}
				}
				else if ( sKey == QStringLiteral( "colspan" ) ) {
					const int nValue = sValue.toInt( &bOk );
					if ( bOk ) {
						node.nColSpan = std::max( 1, std::min( nValue,
															  20480 ) );
					}
				}
			}
		}
	}

	int newNode( int nParent )
	{
		const int nLast = static_cast<int>( m_nodes.size() ) - 1;
		bool bReuse = true;
		if ( m_nodes.size() == 1 ) {
			bReuse = false;
		}
		else if ( m_nodes[ nLast ].sTag.isEmpty() ) {
			if ( m_nodes[ nLast ].sText.isEmpty() ) {
				bReuse = true;
			}
			else if ( m_nodes[ nLast ].sText.size() == 1 &&
					  m_nodes[ nLast ].sText.at( 0 ).isSpace() ) {
				int nLastSibling = static_cast<int>( m_nodes.size() ) - 2;
				while ( nLastSibling &&
						m_nodes[ nLastSibling ].nParent !=
						m_nodes[ nLast ].nParent &&
						m_nodes[ nLastSibling ].displayMode ==
						DisplayMode::Inline ) {
					nLastSibling = m_nodes[ nLastSibling ].nParent;
				}
				bReuse = m_nodes[ nLastSibling ].displayMode !=
					DisplayMode::Inline;
			}
			else {
				bReuse = false;
			}
		}
		else {
			bReuse = false;
		}

		if ( bReuse ) {
			HtmlNode& node = m_nodes.back();
			node.sTag.clear();
			node.sText.clear();
			node.attributes.clear();
			node.children.clear();
			node.id = HtmlElementId::Unknown;
			node.nParent = nParent;
		}
		else {
			m_nodes.emplace_back();
			m_nodes.back().nParent = nParent;
		}
		return static_cast<int>( m_nodes.size() ) - 1;
	}

	static bool allowedInContext( HtmlElementId id, HtmlElementId parentId )
	{
		switch ( id ) {
			case HtmlElementId::Dd:
			case HtmlElementId::Dt:
				return parentId == HtmlElementId::Dl;
			case HtmlElementId::Tr:
				return parentId == HtmlElementId::Table ||
					parentId == HtmlElementId::Thead ||
					parentId == HtmlElementId::Tbody ||
					parentId == HtmlElementId::Tfoot;
			case HtmlElementId::Th:
			case HtmlElementId::Td:
				return parentId == HtmlElementId::Tr;
			case HtmlElementId::Thead:
			case HtmlElementId::Tbody:
			case HtmlElementId::Tfoot:
				return parentId == HtmlElementId::Table;
			case HtmlElementId::Caption:
				return parentId == HtmlElementId::Table;
			case HtmlElementId::Body:
				return parentId != HtmlElementId::Head;
			default:
				return true;
		}
	}

	void resolveParent()
	{
		int nNode = static_cast<int>( m_nodes.size() ) - 1;
		const HtmlElementId id = m_nodes[ nNode ].id;
		int nParent = m_nodes[ nNode ].nParent;

		if ( id == HtmlElementId::Td ) {
			int nAncestor = nParent;
			while ( nAncestor &&
					m_nodes[ nAncestor ].id != HtmlElementId::Tr ) {
				nAncestor = m_nodes[ nAncestor ].nParent;
			}
			if ( nAncestor == 0 ) {
				m_nodes.insert( m_nodes.end() - 1, HtmlNode() );
				m_nodes.insert( m_nodes.end() - 1, HtmlNode() );
				const int nTable = static_cast<int>( m_nodes.size() ) - 3;
				const int nRow = static_cast<int>( m_nodes.size() ) - 2;
				m_nodes[ nTable ].nParent = nParent;
				m_nodes[ nTable ].id = HtmlElementId::Table;
				m_nodes[ nTable ].sTag = QStringLiteral( "table" );
				m_nodes[ nTable ].displayMode = DisplayMode::Table;
				m_nodes[ nTable ].children.push_back( nRow );
				m_nodes[ nRow ].nParent = nTable;
				m_nodes[ nRow ].id = HtmlElementId::Tr;
				m_nodes[ nRow ].sTag = QStringLiteral( "tr" );
				m_nodes[ nRow ].displayMode = DisplayMode::Table;
				nParent = nRow;
				nNode = static_cast<int>( m_nodes.size() ) - 1;
			}
		}

		if ( id == HtmlElementId::Tr ) {
			int nAncestor = nParent;
			while ( nAncestor &&
					m_nodes[ nAncestor ].id != HtmlElementId::Table ) {
				nAncestor = m_nodes[ nAncestor ].nParent;
			}
			if ( nAncestor == 0 ) {
				m_nodes.insert( m_nodes.end() - 1, HtmlNode() );
				const int nTable = static_cast<int>( m_nodes.size() ) - 2;
				m_nodes[ nTable ].nParent = nParent;
				m_nodes[ nTable ].id = HtmlElementId::Table;
				m_nodes[ nTable ].sTag = QStringLiteral( "table" );
				m_nodes[ nTable ].displayMode = DisplayMode::Table;
				nParent = nTable;
				nNode = static_cast<int>( m_nodes.size() ) - 1;
			}
		}

		if ( id == HtmlElementId::P ) {
			while ( nParent != 0 && ! m_nodes[ nParent ].isBlock() ) {
				nParent = m_nodes[ nParent ].nParent;
			}
			if ( nParent == 0 ||
				 m_nodes[ nParent ].id != HtmlElementId::P ) {
				nParent = m_nodes[ nNode ].nParent;
			}
		}

		if ( id == m_nodes[ nParent ].id &&
			 ( id == HtmlElementId::P || id == HtmlElementId::Li ) ) {
			nParent = m_nodes[ nParent ].nParent;
		}

		while ( ( nParent != 0 &&
				  ! allowedInContext( id, m_nodes[ nParent ].id ) ) ||
				m_nodes[ nParent ].mayNotHaveChildren() ) {
			nParent = m_nodes[ nParent ].nParent;
		}

		m_nodes[ nNode ].nParent = nParent;
		m_nodes[ nParent ].children.push_back( nNode );
	}

	void initializeProperties()
	{
		HtmlNode& node = m_nodes.back();
		const HtmlNode& parent = m_nodes[ node.nParent ];
		node.whiteSpace = parent.whiteSpace;
		if ( parent.displayMode == DisplayMode::None ) {
			node.displayMode = DisplayMode::None;
		}
		if ( node.id == HtmlElementId::Pre ) {
			node.whiteSpace = WhiteSpaceMode::Pre;
		}
		if ( node.id == HtmlElementId::Br ) {
			node.sText = QString( QChar::LineSeparator );
		}
	}

	void resolveNode()
	{
		initializeProperties();
	}

	void parseExclamationTag()
	{
		++m_nPos;
		if ( hasPrefix( QLatin1Char( '-' ) ) &&
			 hasPrefix( QLatin1Char( '-' ), 1 ) ) {
			m_nPos += 2;
			const auto nEnd = m_sHtml.indexOf( QStringLiteral( "-->" ),
											   m_nPos );
			m_nPos = nEnd >= 0
				? static_cast<int>( nEnd ) + 3
				: static_cast<int>( m_sHtml.size() );
		}
		else {
			while ( m_nPos < m_nLength ) {
				const QChar character = m_sHtml.at( m_nPos++ );
				if ( character == QLatin1Char( '>' ) ) {
					break;
				}
			}
		}
	}

	void parseTag()
	{
		eatSpace();

		if ( hasPrefix( QLatin1Char( '!' ) ) ) {
			parseExclamationTag();
			if ( m_nodes.back().whiteSpace != WhiteSpaceMode::Pre &&
				 m_nodes.back().whiteSpace != WhiteSpaceMode::PreWrap &&
				 ! m_bTextEditMode ) {
				eatSpace();
			}
			return;
		}

		if ( hasPrefix( QLatin1Char( '/' ) ) ) {
			parseCloseTag();
			return;
		}

		int nParent = static_cast<int>( m_nodes.size() ) - 1;
		while ( nParent && m_nodes[ nParent ].sTag.isEmpty() ) {
			nParent = m_nodes[ nParent ].nParent;
		}

		newNode( nParent );
		HtmlNode& node = m_nodes.back();
		node.sTag = parseWord().toLower();
		const HtmlElement* element = lookupHtmlElement( node.sTag );
		if ( element != nullptr ) {
			node.id = element->id;
			node.displayMode = element->displayMode;
		}
		else {
			node.id = HtmlElementId::Unknown;
		}

		node.attributes.clear();
		if ( m_nPos < m_nLength && m_sHtml.at( m_nPos ).isSpace() ) {
			node.attributes = parseAttributes();
		}

		resolveParent();
		resolveNode();
		applyAttributes();

		bool bTagClosed = false;
		while ( m_nPos < m_nLength &&
				m_sHtml.at( m_nPos ) != QLatin1Char( '>' ) ) {
			if ( m_sHtml.at( m_nPos ) == QLatin1Char( '/' ) ) {
				bTagClosed = true;
			}
			++m_nPos;
		}
		++m_nPos;

		HtmlNode& closedNode = m_nodes.back();
		if ( ( closedNode.whiteSpace == WhiteSpaceMode::Pre ||
			   closedNode.whiteSpace == WhiteSpaceMode::PreWrap ||
			   closedNode.whiteSpace == WhiteSpaceMode::PreLine ) &&
			 closedNode.isBlock() ) {
			if ( m_nPos < m_nLength - 1 &&
				 m_sHtml.at( m_nPos ) == QLatin1Char( '\n' ) ) {
				++m_nPos;
			}
		}

		if ( closedNode.mayNotHaveChildren() || bTagClosed ) {
			const int nNewParent = closedNode.nParent;
			newNode( nNewParent );
			resolveNode();
		}
	}

	void parseCloseTag()
	{
		++m_nPos;
		QString sTag = parseWord().toLower().trimmed();
		while ( m_nPos < m_nLength ) {
			const QChar character = m_sHtml.at( m_nPos++ );
			if ( character == QLatin1Char( '>' ) ) {
				break;
			}
		}

		int nParent = static_cast<int>( m_nodes.size() ) - 1;
		if ( nParent > 0 &&
			 m_nodes[ nParent - 1 ].sTag == sTag &&
			 m_nodes[ nParent - 1 ].mayNotHaveChildren() ) {
			--nParent;
		}
		while ( nParent && m_nodes[ nParent ].sTag != sTag ) {
			nParent = m_nodes[ nParent ].nParent;
		}
		if ( nParent == 0 ) {
			return;
		}

		if ( ( m_nodes[ nParent ].whiteSpace == WhiteSpaceMode::Pre ||
			   m_nodes[ nParent ].whiteSpace == WhiteSpaceMode::PreWrap ||
			   m_nodes[ nParent ].whiteSpace == WhiteSpaceMode::PreLine ) &&
			 m_nodes[ nParent ].isBlock() &&
			 m_nodes.back().sText.endsWith( QLatin1Char( '\n' ) ) ) {
			m_nodes.back().sText.chop( 1 );
		}

		const int nNewParent = m_nodes[ nParent ].nParent;
		newNode( nNewParent );
		resolveNode();
	}

	QString m_sHtml;
	int m_nPos = 0;
	int m_nLength = 0;
	bool m_bTextEditMode = false;
	std::vector<HtmlNode> m_nodes;
};

class HtmlImporter {
public:
	HtmlImporter( const std::vector<HtmlNode>& nodes, bool bTextEditMode )
		: m_nodes( nodes ), m_bTextEditMode( bTextEditMode )
	{
		m_blocks.append( QString() );
	}

	void run()
	{
		const int nCount = static_cast<int>( m_nodes.size() );
		for ( int ii = 0; ii < nCount; ++ii ) {
			const HtmlNode& node = m_nodes[ ii ];
			m_whiteSpace = m_bTextEditMode
				? WhiteSpaceMode::PreWrap : node.whiteSpace;

			if ( ii > 0 && node.nParent != ii - 1 ) {
				const bool bLastBlockTagClosed = closeTag( ii );
				m_bBlockTagClosed = m_bBlockTagClosed ||
					bLastBlockTagClosed;
				if ( m_bBlockTagClosed && ! node.isBlock() &&
					 node.id != HtmlElementId::Unknown ) {
					m_bHasBlock = false;
				}
			}

			if ( node.displayMode == DisplayMode::None ) {
				continue;
			}

			if ( node.id == HtmlElementId::Table ) {
				ii = renderTable( ii ) - 1;
				continue;
			}
			if ( node.id == HtmlElementId::Tr ) {
				continue;
			}
			if ( node.id == HtmlElementId::Hr ) {
				appendBlock();
				m_bHasBlock = false;
				m_compress = CompressWhitespace::Remove;
				continue;
			}
			if ( node.id == HtmlElementId::Img ) {
				m_blocks.last() += QChar::ObjectReplacementCharacter;
				m_bHasBlock = false;
				m_compress = CompressWhitespace::Collapse;
				continue;
			}
			if ( node.id == HtmlElementId::Body ||
				 node.id == HtmlElementId::Ul ||
				 node.id == HtmlElementId::Ol ) {
				m_compress = CompressWhitespace::Remove;
			}

			if ( node.isBlock() ) {
				processBlockNode();
			}

			if ( m_bBlockTagClosed && ! m_bHasBlock && ! node.isBlock() &&
				 ! node.sText.isEmpty() && ! node.hasOnlyWhitespace() &&
				 node.displayMode == DisplayMode::Inline ) {
				appendBlock();
				m_bBlockTagClosed = false;
				m_bHasBlock = true;
			}

			if ( appendNodeText( node ) ) {
				m_bHasBlock = false;
			}
		}
	}

	QString text() const
	{
		return m_blocks.join( QStringLiteral( "\n" ) );
	}

private:
	void appendBlock()
	{
		m_blocks.append( QString() );
		if ( m_whiteSpace != WhiteSpaceMode::Pre &&
			 m_whiteSpace != WhiteSpaceMode::PreWrap ) {
			m_compress = CompressWhitespace::Remove;
		}
	}

	void processBlockNode()
	{
		if ( ! m_bHasBlock ) {
			appendBlock();
		}
		m_bHasBlock = true;
		m_bBlockTagClosed = false;
	}

	int depth( int nIndex ) const
	{
		int nDepth = 0;
		while ( nIndex ) {
			nIndex = m_nodes[ nIndex ].nParent;
			++nDepth;
		}
		return nDepth;
	}

	bool closeTag( int nIndex )
	{
		const HtmlNode* closedNode = &m_nodes[ nIndex - 1 ];
		const int nEndDepth = depth( nIndex ) - 1;
		int nDepth = depth( nIndex - 1 );
		bool bBlockTagClosed = false;

		while ( nDepth > nEndDepth ) {
			switch ( closedNode->id ) {
				case HtmlElementId::Tr:
					bBlockTagClosed = true;
					break;
				case HtmlElementId::Table:
					bBlockTagClosed = false;
					m_compress = CompressWhitespace::Remove;
					break;
				case HtmlElementId::Th:
				case HtmlElementId::Td:
					bBlockTagClosed = true;
					m_compress = CompressWhitespace::Remove;
					break;
				case HtmlElementId::Ol:
				case HtmlElementId::Ul:
					bBlockTagClosed = true;
					break;
				case HtmlElementId::Br:
					m_compress = CompressWhitespace::Remove;
					break;
				case HtmlElementId::Div: {
					if ( ! m_blocks.last().isEmpty() ||
						 m_blocks.size() > 1 ) {
						const QChar lastCharacter =
							! m_blocks.last().isEmpty()
							? m_blocks.last().at(
								m_blocks.last().size() - 1 )
							: QChar::ParagraphSeparator;
						if ( ! closedNode->children.empty() &&
							 lastCharacter != QChar::LineSeparator ) {
							bBlockTagClosed = true;
						}
					}
					break;
				}
				case HtmlElementId::H1:
				case HtmlElementId::H2:
				case HtmlElementId::H3:
				case HtmlElementId::H4:
				case HtmlElementId::H5:
				case HtmlElementId::H6:
					bBlockTagClosed = true;
					break;
				default:
					if ( closedNode->isBlock() ) {
						bBlockTagClosed = true;
					}
					break;
			}
			closedNode = &m_nodes[ closedNode->nParent ];
			--nDepth;
		}
		return bBlockTagClosed;
	}

	bool appendNodeText( const HtmlNode& node )
	{
		bool bInserted = false;
		const WhiteSpaceMode whiteSpace = m_whiteSpace;
		if ( whiteSpace == WhiteSpaceMode::Pre ||
			 whiteSpace == WhiteSpaceMode::PreWrap ) {
			m_compress = CompressWhitespace::Preserve;
		}

		QString sTextToInsert;
		for ( const QChar& character : node.sText ) {
			QChar output = character;
			const bool bWhitespace =
				output.isSpace() &&
				output != QChar::Nbsp &&
				output != QChar::ParagraphSeparator &&
				output != QChar::LineSeparator;

			if ( bWhitespace ) {
				if ( whiteSpace == WhiteSpaceMode::PreLine &&
					 ( output == QLatin1Char( '\n' ) ||
					   output == QLatin1Char( '\r' ) ) ) {
					m_compress = CompressWhitespace::Preserve;
				}
				if ( m_compress == CompressWhitespace::Collapse ) {
					m_compress = CompressWhitespace::Remove;
				}
				else if ( m_compress == CompressWhitespace::Remove ) {
					continue;
				}

				if ( whiteSpace == WhiteSpaceMode::Pre ||
					 m_bTextEditMode ) {
					if ( output == QLatin1Char( '\n' ) ) {
						if ( m_bTextEditMode ) {
							continue;
						}
					}
					else if ( output == QLatin1Char( '\r' ) ) {
						continue;
					}
				}
				else if ( whiteSpace != WhiteSpaceMode::PreWrap ) {
					m_compress = CompressWhitespace::Remove;
					if ( whiteSpace == WhiteSpaceMode::PreLine &&
						 ( output == QLatin1Char( '\n' ) ||
						   output == QLatin1Char( '\r' ) ) ) {
					}
					else if ( whiteSpace == WhiteSpaceMode::NoWrap ) {
						output = QChar::Nbsp;
					}
					else {
						output = QLatin1Char( ' ' );
					}
				}
			}
			else {
				m_compress = CompressWhitespace::Preserve;
			}

			if ( output == QLatin1Char( '\n' ) ||
				 output == QChar::ParagraphSeparator ) {
				if ( ! sTextToInsert.isEmpty() ) {
					m_blocks.last() += sTextToInsert;
					sTextToInsert.clear();
					bInserted = true;
				}
				appendBlock();
				bInserted = true;
			}
			else {
				sTextToInsert += output;
			}
		}

		if ( ! sTextToInsert.isEmpty() ) {
			m_blocks.last() += sTextToInsert;
			bInserted = true;
		}
		return bInserted;
	}

	bool isDescendant( int nIndex, int nAncestor ) const
	{
		int nParent = m_nodes[ nIndex ].nParent;
		while ( nParent ) {
			if ( nParent == nAncestor ) {
				return true;
			}
			nParent = m_nodes[ nParent ].nParent;
		}
		return false;
	}

	int subtreeEnd( int nIndex ) const
	{
		int nEnd = nIndex + 1;
		while ( nEnd < static_cast<int>( m_nodes.size() ) &&
				isDescendant( nEnd, nIndex ) ) {
			++nEnd;
		}
		return nEnd;
	}

	bool hasAncestor( int nIndex, HtmlElementId id ) const
	{
		int nParent = m_nodes[ nIndex ].nParent;
		while ( nParent ) {
			if ( m_nodes[ nParent ].id == id ) {
				return true;
			}
			nParent = m_nodes[ nParent ].nParent;
		}
		return false;
	}

	int nearestTable( int nIndex ) const
	{
		int nParent = m_nodes[ nIndex ].nParent;
		while ( nParent ) {
			if ( m_nodes[ nParent ].id == HtmlElementId::Table ) {
				return nParent;
			}
			nParent = m_nodes[ nParent ].nParent;
		}
		return -1;
	}

	QString renderSubtree( int nIndex, int nEnd ) const
	{
		std::vector<HtmlNode> nodes( m_nodes.begin() + nIndex,
									 m_nodes.begin() + nEnd );
		for ( int ii = 0; ii < static_cast<int>( nodes.size() ); ++ii ) {
			if ( ii == 0 ) {
				nodes[ ii ].nParent = 0;
			}
			else {
				nodes[ ii ].nParent = std::max(
					0, nodes[ ii ].nParent - nIndex );
			}
		}
		HtmlImporter importer( nodes, m_bTextEditMode );
		importer.run();
		return importer.text();
	}

	void appendLooseText( int nIndex )
	{
		const WhiteSpaceMode oldWhiteSpace = m_whiteSpace;
		m_whiteSpace = m_bTextEditMode
			? WhiteSpaceMode::PreWrap : m_nodes[ nIndex ].whiteSpace;
		appendNodeText( m_nodes[ nIndex ] );
		m_whiteSpace = oldWhiteSpace;
	}

	struct TableCell {
		QString sText;
		int nRowSpan = 1;
		int nColSpan = 1;
	};

	int renderTable( int nIndex )
	{
		const int nEnd = subtreeEnd( nIndex );
		std::vector<int> rows;
		std::vector<int> loose;
		int nCaption = -1;

		if ( ! m_nodes[ nIndex ].sText.isEmpty() ) {
			loose.push_back( nIndex );
		}
		for ( int jj = nIndex + 1; jj < nEnd; ++jj ) {
			if ( nearestTable( jj ) != nIndex ) {
				continue;
			}
			if ( m_nodes[ jj ].id == HtmlElementId::Caption ||
				 hasAncestor( jj, HtmlElementId::Tr ) ||
				 hasAncestor( jj, HtmlElementId::Td ) ||
				 hasAncestor( jj, HtmlElementId::Th ) ||
				 hasAncestor( jj, HtmlElementId::Caption ) ) {
				continue;
			}
			if ( ! m_nodes[ jj ].sText.isEmpty() ) {
				loose.push_back( jj );
			}
		}
		for ( int jj = nIndex + 1; jj < nEnd; ++jj ) {
			if ( m_nodes[ jj ].id == HtmlElementId::Tr &&
				 nearestTable( jj ) == nIndex ) {
				rows.push_back( jj );
			}
			else if ( m_nodes[ jj ].id == HtmlElementId::Caption &&
					  nearestTable( jj ) == nIndex ) {
				nCaption = jj;
			}
		}

		for ( int looseIndex : loose ) {
			appendLooseText( looseIndex );
		}
		if ( nCaption >= 0 ) {
			appendBlock();
			m_blocks.last() += renderSubtree(
				nCaption, subtreeEnd( nCaption ) );
		}

		std::map<std::pair<int, int>, TableCell> placements;
		std::set<std::pair<int, int>> occupied;
		int nColumns = 0;
		for ( int rr = 0; rr < static_cast<int>( rows.size() ); ++rr ) {
			int nColumn = 0;
			for ( int cellIndex : m_nodes[ rows[ rr ] ].children ) {
				const HtmlNode& cell = m_nodes[ cellIndex ];
				if ( ! cell.isTableCell() ) {
					continue;
				}
				while ( occupied.count( { rr, nColumn } ) != 0 ) {
					++nColumn;
				}
				TableCell rendered;
				rendered.sText = renderSubtree(
					cellIndex, subtreeEnd( cellIndex ) );
				rendered.nRowSpan = cell.nRowSpan;
				rendered.nColSpan = cell.nColSpan;
				placements[ { rr, nColumn } ] = rendered;
				for ( int nRow = rr; nRow < rr + cell.nRowSpan; ++nRow ) {
					for ( int nCol = nColumn;
						  nCol < nColumn + cell.nColSpan; ++nCol ) {
						occupied.insert( { nRow, nCol } );
					}
				}
				nColumn += cell.nColSpan;
				nColumns = std::max( nColumns, nColumn );
			}
		}

		QString sTable;
		for ( int rr = 0; rr < static_cast<int>( rows.size() ); ++rr ) {
			for ( int nColumn = 0; nColumn < nColumns; ++nColumn ) {
				const auto it = placements.find( { rr, nColumn } );
				if ( it != placements.end() ) {
					sTable += QChar( static_cast<ushort>( 0xfdd0 ) );
					sTable += it->second.sText;
				}
				else if ( occupied.count( { rr, nColumn } ) == 0 ) {
					sTable += QChar( static_cast<ushort>( 0xfdd0 ) );
				}
			}
		}
		sTable += QChar( static_cast<ushort>( 0xfdd1 ) );

		m_blocks.last() += sTable;
		m_bHasBlock = false;
		m_compress = CompressWhitespace::Remove;
		return nEnd;
	}

	const std::vector<HtmlNode>& m_nodes;
	const bool m_bTextEditMode;
	QStringList m_blocks;
	bool m_bHasBlock = true;
	CompressWhitespace m_compress = CompressWhitespace::Remove;
	bool m_bBlockTagClosed = false;
	WhiteSpaceMode m_whiteSpace = WhiteSpaceMode::Normal;
};

static QString htmlToPlainTextImpl( const QString& sHtml )
{
	QString sSource = sHtml;
	const int nStartFragment = static_cast<int>(
		sSource.indexOf( QStringLiteral( "<!--StartFragment-->" ) ) );
	if ( nStartFragment != -1 ) {
		const QString sQt3RichTextHeader = QStringLiteral(
			"<meta name=\"qrichtext\" content=\"1\" />" );

		// Hack for Qt3: the header may sit outside the fragment, so the
		// check has to run on the full source before it gets sliced.
		const bool bHasQtRichtextMetaTag =
			sSource.contains( sQt3RichTextHeader );

		const int nEndFragment = static_cast<int>(
			sSource.indexOf( QStringLiteral( "<!--EndFragment-->" ) ) );
		if ( nStartFragment < nEndFragment ) {
			sSource = sSource.mid( nStartFragment,
								   nEndFragment - nStartFragment );
		}
		else {
			sSource = sSource.mid( nStartFragment );
		}

		if ( bHasQtRichtextMetaTag ) {
			sSource.prepend( sQt3RichTextHeader );
		}
	}

	HtmlParser parser( sSource );
	parser.parse();
	HtmlImporter importer( parser.nodes(), parser.textEditMode() );
	importer.run();

	const QString sRaw = importer.text();
	QString sPlainText;
	sPlainText.reserve( sRaw.size() );
	for ( const QChar& character : sRaw ) {
		switch ( character.unicode() ) {
			case QChar::ParagraphSeparator:
			case QChar::LineSeparator:
			case 0xfdd0:
			case 0xfdd1:
				sPlainText += QLatin1Char( '\n' );
				break;
			case QChar::Nbsp:
				sPlainText += QLatin1Char( ' ' );
				break;
			default:
				sPlainText += character;
				break;
		}
	}
	return sPlainText;
}

} // namespace

QString pluginUiHtmlToPlainText( const QString& sHtml )
{
	return htmlToPlainTextImpl( sHtml );
}

} // namespace H2Core
