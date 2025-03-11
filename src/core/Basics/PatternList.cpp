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

#include <core/Basics/PatternList.h>

#include <algorithm>

#include <core/Basics/Drumkit.h>
#include <core/Basics/InstrumentList.h>
#include <core/Basics/Pattern.h>
#include <core/Helpers/Xml.h>

namespace H2Core
{


PatternList::PatternList() {
}

PatternList::PatternList( std::shared_ptr<PatternList> pOther ) {
	for ( const auto& ppPattern : *pOther ) {
		add( std::make_shared<Pattern>( ppPattern ) );
	}
}

PatternList::~PatternList() {
}

std::shared_ptr<PatternList> PatternList::loadFrom( const XMLNode& node,
													 const QString& sDrumkitName,
													 bool bSilent ) {
	XMLNode patternsNode = node.firstChildElement( "patternList" );
	if ( patternsNode.isNull() ) {
		ERRORLOG( "'patternList' node not found. Unable to load pattern list." );
		return nullptr;
	}

	auto pPatternList = std::make_shared<PatternList>();
	int nPatternCount = 0;

	XMLNode patternNode =  patternsNode.firstChildElement( "pattern" );
	while ( !patternNode.isNull()  ) {
		nPatternCount++;
		auto pPattern = Pattern::loadFrom( patternNode, sDrumkitName, bSilent );
		if ( pPattern != nullptr ) {
			pPatternList->add( pPattern );
		}
		else {
			ERRORLOG( "Error loading pattern" );
			return nullptr;
		}
		patternNode = patternNode.nextSiblingElement( "pattern" );
	}
	if ( nPatternCount == 0 && ! bSilent ) {
		WARNINGLOG( "0 patterns?" );
	}

	return pPatternList;
}

void PatternList::saveTo( XMLNode& node, int nInstrumentId,
						   const QString& sType, int nPitch ) const {
	XMLNode patternListNode = node.createNode( "patternList" );
	
	for ( const auto& pPattern : m_pPatterns ) {
		if ( pPattern != nullptr ) {
			pPattern->saveTo( patternListNode, nInstrumentId, sType, nPitch );
		}
	}
}
	
void PatternList::add( std::shared_ptr<Pattern> pPattern, bool bAddVirtuals )
{
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	if ( pPattern == nullptr ) {
		ERRORLOG( "Provided pattern is invalid" );
		return;
	}

	// do nothing if already in m_pPatterns
	if ( index( pPattern ) != -1 ) {
		INFOLOG( "Provided pattern is already contained" );
		return;
	}

	// In case the added pattern is a virtual one, deactivate the
	// individual patterns it encompasses in case one of them was
	// already activated. (They will be only activated as virtual
	// patterns from here on).
	auto pVirtualPatterns = pPattern->getVirtualPatterns();
	for ( int ii = m_pPatterns.size() - 1; ii >= 0 && ii < m_pPatterns.size(); --ii ) {
		auto ppPattern = m_pPatterns[ ii ];
		if ( pVirtualPatterns->find( ppPattern ) != pVirtualPatterns->end() ) {
			del( ii );
		}
	}
	
	m_pPatterns.push_back( pPattern );

	if ( bAddVirtuals ) {
		pPattern->addFlattenedVirtualPatterns( shared_from_this() );
	}
}

void PatternList::insert( int nIdx, std::shared_ptr<Pattern> pPattern )
{
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	// do nothing if already in m_pPatterns
	if ( index( pPattern ) != -1 ) {
		return;
	}
	if ( nIdx > m_pPatterns.size() ) {
		m_pPatterns.resize( nIdx );
	}
	m_pPatterns.insert( m_pPatterns.begin() + nIdx, pPattern );
}

std::shared_ptr<Pattern> PatternList::get( int idx ) const
{
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	if ( idx < 0 || idx >= m_pPatterns.size() ) {
		ERRORLOG( QString( "idx %1 out of [0;%2]" ).arg( idx ).arg( size() ) );
		return nullptr;
	}
	assert( idx >= 0 && idx < m_pPatterns.size() );
	return m_pPatterns[idx];
}

int PatternList::index( const std::shared_ptr<Pattern> pattern ) const
{
	for( int i=0; i<m_pPatterns.size(); i++ ) {
		if ( m_pPatterns[i]==pattern ) {
			return i;
		}
	}
	return -1;
}

std::shared_ptr<Pattern> PatternList::del( int idx )
{
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	if ( idx >= 0 && idx < m_pPatterns.size() ) {
		std::shared_ptr<Pattern> pattern = m_pPatterns[idx];
		m_pPatterns.erase( m_pPatterns.begin() + idx );
		return pattern;
	}
	return nullptr;
}

std::shared_ptr<Pattern> PatternList::del( std::shared_ptr<Pattern> pPattern )
{
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	for ( int i = 0; i < m_pPatterns.size(); i++ ) {
		if ( m_pPatterns[ i ] == pPattern ) {
			return del( i );
		}
	}
	return nullptr;
}

std::shared_ptr<Pattern> PatternList::replace( int idx,
											   std::shared_ptr<Pattern> pPattern )
{
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	/*
	 * if we insert a new pattern (copy, add new pattern, undo delete pattern and so on will do this)
	 * idx is > __pattern.size(). that's why i add +1 to assert expression
	 */

	assert( idx >= 0 && idx <= m_pPatterns.size() +1 );
	if( idx < 0 || idx >= m_pPatterns.size() ) {
		ERRORLOG( QString( "index out of bounds %1 (size:%2)" )
				  .arg( idx ).arg( m_pPatterns.size() ) );
		return nullptr;
	}

	m_pPatterns.insert( m_pPatterns.begin() + idx, pPattern );
	m_pPatterns.erase( m_pPatterns.begin() + idx + 1 );

	//create return pattern after patternlist tätatä to return the right one
	std::shared_ptr<Pattern> ret = m_pPatterns[idx];
	return ret;
}

void PatternList::flattenedVirtualPatternsCompute()
{
	for ( int i=0 ; i<m_pPatterns.size() ; i++ ) {
		m_pPatterns[i]->flattenedVirtualPatternsClear();
	}
	for ( int i=0 ; i<m_pPatterns.size() ; i++ ) {
		m_pPatterns[i]->flattenedVirtualPatternsCompute();
	}
}

void PatternList::virtualPatternDel( std::shared_ptr<Pattern> pPattern )
{
	for ( int i = 0; i < m_pPatterns.size(); i++ ) {
		m_pPatterns[ i ]->virtualPatternsDel( pPattern );
	}
}

bool PatternList::checkName( const QString& sPatternName,
							  std::shared_ptr<Pattern> pIgnore ) const
{
	if ( sPatternName.isEmpty() ) {
		return false;
	}

	for ( int i = 0; i < m_pPatterns.size(); i++ ) {
		if ( m_pPatterns[ i ] != pIgnore &&
			 m_pPatterns[ i ]->getName() == sPatternName ) {
			return false;
		}
	}
	return true;
}

QString PatternList::findUnusedPatternName( const QString& sSourceName,
											   std::shared_ptr<Pattern> pIgnore ) const
{
	QString unusedPatternNameCandidate;
	QString sSource { sSourceName };
	
	if( sSource.isEmpty() ) {
		sSource = "Pattern 11";
	}

	int i = 1;
	QString suffix = "";
	unusedPatternNameCandidate = sSource;

	// Check if the sSource already has a number suffix, and if so, start
	// searching for an unused name from that number.
	QRegularExpression numberSuffixRe("(.+) #(\\d+)$");
	QRegularExpressionMatch match = numberSuffixRe.match(sSource);
	if (match.hasMatch()) {
		QString numberSuffix = match.captured(2);

		i = numberSuffix.toInt();
		suffix = " #" + QString::number(i);
		unusedPatternNameCandidate = match.captured(1);
	}

	while( !checkName( unusedPatternNameCandidate + suffix, pIgnore ) ) {
		suffix = " #" + QString::number(i);
		i++;
	}

	unusedPatternNameCandidate += suffix;

	return unusedPatternNameCandidate;
}

int PatternList::longestPatternLength( bool bIncludeVirtuals ) const {
	int nMax = -1;
	for ( const auto& ppPattern : m_pPatterns ) {
		if ( ppPattern->getLength() > nMax ) {
			nMax = ppPattern->getLength();
		}

		if ( bIncludeVirtuals ) {
			for ( const auto& ppVirtualPattern : *ppPattern->getFlattenedVirtualPatterns() ) {
				if ( ppVirtualPattern->getLength() > nMax ) {
					nMax = ppVirtualPattern->getLength();
				}
			}
		}
	}
	return nMax;
}

void PatternList::mapTo( std::shared_ptr<Drumkit> pDrumkit,
						 std::shared_ptr<Drumkit> pOldDrumkit ) {
	for ( auto& ppPattern : m_pPatterns ) {
		ppPattern->mapTo( pDrumkit, pOldDrumkit );
	}
}

bool operator==( const PatternList& pLhs, const PatternList& pRhs ) {
	if ( pLhs.size() != pRhs.size() ) {
		return false;
	}

	for ( int ii = 0; ii < pLhs.size(); ii++ ) {
		if ( pLhs.get( ii ) != pRhs.get( ii ) ) {
			return false;
		}
	}

	return true;
}

bool operator!=( const PatternList& pLhs, const PatternList& pRhs ) {
	if ( pLhs.size() != pRhs.size() ) {
		return true;
	}

	for ( int ii = 0; ii < pLhs.size(); ii++ ) {
		if ( pLhs.get( ii ) != pRhs.get( ii ) ) {
			return true;
		}
	}

	return false;
}

QString PatternList::toQString( const QString& sPrefix, bool bShort ) const {
	QString s = Base::sPrintIndention;
	QString sOutput;
	if ( ! bShort ) {
		sOutput = QString( "%1[PatternList]\n" ).arg( sPrefix );
		for ( const auto& pp : m_pPatterns ) {
			if ( pp != nullptr ) {
				sOutput.append( QString( "%1" ).arg( pp->toQString( sPrefix + s, bShort ) ) );
			}
		}
	} else {
		sOutput = QString( "[PatternList] " );
		for ( const auto& pp : m_pPatterns ) {
			if ( pp != nullptr ) {
				sOutput.append( QString( "[%1] " ).arg( pp->getName() ) );
			}
		}
	}
	
	return sOutput;
}

std::set<DrumkitMap::Type> PatternList::getAllTypes() const {
	std::set<DrumkitMap::Type> types;

	for ( const auto& ppPattern : m_pPatterns ) {
		if ( ppPattern != nullptr ) {
			types.merge( ppPattern->getAllTypes() );
		}
	}

	return types;
}

std::vector<std::shared_ptr<Note>> PatternList::getAllNotesOfType(
	const DrumkitMap::Type& sType ) const
{
	std::vector<std::shared_ptr<Note>> notes;

	for ( const auto& ppPattern : m_pPatterns ) {
		if ( ppPattern != nullptr ) {
			auto patternNotes = ppPattern->getAllNotesOfType( sType );
			notes.insert(
				notes.end(), patternNotes.begin(), patternNotes.end() );
		}
	}

	return notes;
}


std::vector<std::shared_ptr<Pattern>>::iterator PatternList::begin() {
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	return m_pPatterns.begin();
}

std::vector<std::shared_ptr<Pattern>>::iterator PatternList::end() {
	return m_pPatterns.end();
}

std::vector<std::shared_ptr<Pattern>>::const_iterator PatternList::cbegin() const {
	ASSERT_AUDIO_ENGINE_LOCKED( toQString() );
	return m_pPatterns.begin();
}

std::vector<std::shared_ptr<Pattern>>::const_iterator PatternList::cend() const {
	return m_pPatterns.end();
}
 
}



/* vim: set softtabstop=4 noexpandtab: */
