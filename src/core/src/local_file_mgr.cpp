/*
 * Hydrogen
 * Copyright(c) 2002-2008 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include "hydrogen/version.h"
#include <hydrogen/basics/adsr.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/fx/Effects.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctype.h>
#include <sys/stat.h>

#include <QDir>
#include <QApplication>
#include <QVector>
#include <QDomDocument>
#include <QLocale>

namespace H2Core
{

const char* LocalFileMng::__class_name = "LocalFileMng";

LocalFileMng::LocalFileMng()
	: Object( __class_name )
{
	//	infoLog("INIT");
}



LocalFileMng::~LocalFileMng()
{
	//	infoLog("DESTROY");
}

QString LocalFileMng::getDrumkitNameForPattern( const QString& patternDir )
{
	QDomDocument doc = LocalFileMng::openXmlDocument( patternDir );

	QDomNode rootNode = doc.firstChildElement( "drumkit_pattern" );	// root element
	if (  rootNode.isNull() ) {
		ERRORLOG( "Error reading Pattern: Pattern_drumkit_infonode not found " + patternDir);
		return NULL;
	}

	return LocalFileMng::readXmlString( rootNode,"pattern_for_drumkit", "" );
}


Pattern* LocalFileMng::loadPattern( const QString& directory )
{

	InstrumentList* instrList = Hydrogen::get_instance()->getSong()->get_instrument_list();
	Pattern *pPattern = NULL;
	QString patternInfoFile = directory;

	QFile check( patternInfoFile );
	if (check.exists() == false) {
		ERRORLOG( QString("Load Pattern: Data file %1 not found." ).arg( patternInfoFile ) );
		return NULL;
	}


	QDomDocument doc  = LocalFileMng::openXmlDocument( patternInfoFile );
	QFile file( patternInfoFile );

	// root element
	QDomNode rootNode = doc.firstChildElement( "drumkit_pattern" );	// root element
	if (  rootNode.isNull() ) {
		ERRORLOG( "Error reading Pattern: Pattern_drumkit_infonode not found" );
		return NULL;
	}

	QDomNode patternNode = rootNode.firstChildElement( "pattern" );

	QString sName( LocalFileMng::readXmlString( patternNode,"pattern_name", "" ) );
	QString sInfo( LocalFileMng::readXmlString( patternNode,"info", "" ) );
	QString sCategory( LocalFileMng::readXmlString( patternNode,"category", "" ) );


	int nSize = -1;
	nSize = LocalFileMng::readXmlInt( patternNode, "size",nSize ,false,false );
	pPattern = new Pattern( sName, sInfo, sCategory, nSize );



	QDomNode pNoteListNode = patternNode.firstChildElement( "noteList" );
	if ( ! pNoteListNode.isNull() )
	{
		// new code  :)
		QDomNode noteNode = pNoteListNode.firstChildElement( "note" );
		while (  ! noteNode.isNull()  )
		{
			Note* pNote = NULL;
			unsigned nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
			float fLeadLag = LocalFileMng::readXmlFloat( noteNode, "leadlag", 0.0 , false , false);
			float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );
			float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
			float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );
			int nLength = LocalFileMng::readXmlInt( noteNode, "length", -1, true );
			float nPitch = LocalFileMng::readXmlFloat( noteNode, "pitch", 0.0, false, false );
			QString sKey = LocalFileMng::readXmlString( noteNode, "key", "C0", false, false );
			QString nNoteOff = LocalFileMng::readXmlString( noteNode, "note_off", "false", false, false );
			int instrId = LocalFileMng::readXmlInt( noteNode, "instrument", 0, true );

			Instrument *instrRef = instrList->find( instrId );
			if ( !instrRef ) {
				ERRORLOG( QString( "Instrument with ID: '%1' not found. Note skipped." ).arg( instrId ) );
				noteNode = noteNode.nextSiblingElement( "note" );
				continue;
			}
			//assert( instrRef );
			bool noteoff = false;
			if ( nNoteOff == "true" )
				noteoff = true;

			pNote = new Note( instrRef, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch);
			pNote->set_key_octave( sKey );
			pNote->set_lead_lag(fLeadLag);
			pNote->set_note_off( noteoff );
			pPattern->insert_note( pNote );
			noteNode = noteNode.nextSiblingElement( "note" );
		}
	}

	return pPattern;

}

QString LocalFileMng::copyInstrumentLineToString(Song *song, int selectedPattern, int selectedInstrument)
{
	Instrument *instr = song->get_instrument_list()->get( selectedInstrument );
	assert( instr );
	
	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode rootNode = doc.createElement( "instrument_line" );
	//LIB_ID just in work to get better usability
	//writeXmlString( &rootNode, "LIB_ID", "in_work" );
	writeXmlString( rootNode, "author", song->get_author() );
	writeXmlString( rootNode, "license", song->get_license() );
	
	QDomNode patternList = doc.createElement( "patternList" );

	unsigned nPatterns = song->get_pattern_list()->size();
	for ( unsigned i = 0; i < nPatterns; i++ )
	{
		if ((selectedPattern >= 0) && (selectedPattern != i))
			continue;
		
		// Export pattern
		Pattern *pat = song->get_pattern_list()->get( i );

		QDomNode patternNode = doc.createElement( "pattern" );
		writeXmlString( patternNode, "pattern_name", pat->get_name() );

		QString category;
		if ( pat->get_category().isEmpty() )
			category = "No category";
		else
			category = pat->get_category();

		writeXmlString( patternNode, "info", pat->get_info() );
		writeXmlString( patternNode, "category", category  );
		writeXmlString( patternNode, "size", QString("%1").arg( pat->get_length() ) );

		QDomNode noteListNode = doc.createElement( "noteList" );
		const Pattern::notes_t* notes = pat->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it)
		{
			Note *pNote = it->second;
			assert( pNote );

			// Export only specified instrument
			if (pNote->get_instrument() == instr)
			{
				QDomNode noteNode = doc.createElement( "note" );
				writeXmlString( noteNode, "position", QString("%1").arg( pNote->get_position() ) );
				writeXmlString( noteNode, "leadlag", QString("%1").arg( pNote->get_lead_lag() ) );
				writeXmlString( noteNode, "velocity", QString("%1").arg( pNote->get_velocity() ) );
				writeXmlString( noteNode, "pan_L", QString("%1").arg( pNote->get_pan_l() ) );
				writeXmlString( noteNode, "pan_R", QString("%1").arg( pNote->get_pan_r() ) );
				writeXmlString( noteNode, "pitch", QString("%1").arg( pNote->get_pitch() ) );

				writeXmlString( noteNode, "key", pNote->key_to_string() );

				writeXmlString( noteNode, "length", QString("%1").arg( pNote->get_length() ) );
				noteListNode.appendChild( noteNode );
			}
		}
		patternNode.appendChild( noteListNode );

		patternList.appendChild( patternNode );
	}
	
	rootNode.appendChild(patternList);
	
	doc.appendChild( rootNode );
	
	// Serialize document & return
	return doc.toString();
}

bool LocalFileMng::pasteInstrumentLineFromString(Song *song, const QString & serialized, int selectedPattern, int selectedInstrument, std::list<Pattern *> & patterns)
{
	QDomDocument doc;
	if (!doc.setContent(serialized))
		return false;
	
	// Get current instrument
	Instrument *instr = song->get_instrument_list()->get( selectedInstrument );
	assert( instr );
	
	// Get pattern list
	PatternList *pList = song->get_pattern_list();
	Pattern *pSelected = (selectedPattern >= 0) ? pList->get(selectedPattern) : NULL;
	
	// Check if document has correct structure
	QDomNode rootNode = doc.firstChildElement( "instrument_line" );	// root element
	
	if ( rootNode.isNull() )
	{
		ERRORLOG( "Error pasting Clipboard:Instrument_line_info node not found ");
		return false;
	}

	// Find pattern list
	QDomNode patternList = rootNode.firstChildElement( "patternList" );
	if (patternList.isNull())
		return false;

	// Parse each pattern if needed
	QDomNode patternNode = patternList.firstChildElement( "pattern" );
	bool is_single = true;
	if (!patternNode.isNull())
		is_single = (( QDomNode )patternNode.nextSiblingElement( "pattern" )).isNull();
	
	while (!patternNode.isNull())
	{
		QString patternName(readXmlString(patternNode, "pattern_name", ""));
		
		// Check if pattern name specified
		if (patternName.length() > 0)
		{
			// Try to find pattern by name
			Pattern* pat = pList->find(patternName);
			
			// If OK - check if need to add this pattern to result
			// If there is only one pattern, we always add it to list
			// If there is no selected pattern, we add all existing patterns to list (match by name)
			// Otherwise we add only existing selected pattern to list (match by name)
			if ((is_single) || ((pat != NULL) && ((selectedPattern < 0) || (pat == pSelected))))
			{
				// Load additional pattern info & create pattern
				QString sInfo;
				sInfo = LocalFileMng::readXmlString(patternNode, "info", sInfo, false, false);
				QString sCategory;
				sCategory = LocalFileMng::readXmlString(patternNode, "category", sCategory, false, false);
				int nSize = -1;
				nSize = LocalFileMng::readXmlInt(patternNode, "size", nSize, false, false);
				
				// Change name of pattern to selected pattern
				if (pSelected != NULL)
					patternName = pSelected->get_name();

				pat = new Pattern( patternName, sInfo, sCategory, nSize );
				
				// Parse pattern data
				QDomNode pNoteListNode = patternNode.firstChildElement( "noteList" );
				if ( ! pNoteListNode.isNull() )
				{
					// Parse note-by-note
					QDomNode noteNode = pNoteListNode.firstChildElement( "note" );
					while ( ! noteNode.isNull() )
					{
						Note* pNote = NULL;

						unsigned nPosition = LocalFileMng::readXmlInt( noteNode, "position", 0 );
						float fLeadLag = LocalFileMng::readXmlFloat( noteNode, "leadlag", 0.0 , false , false );
						float fVelocity = LocalFileMng::readXmlFloat( noteNode, "velocity", 0.8f );
						float fPan_L = LocalFileMng::readXmlFloat( noteNode, "pan_L", 0.5 );
						float fPan_R = LocalFileMng::readXmlFloat( noteNode, "pan_R", 0.5 );
						int nLength = LocalFileMng::readXmlInt( noteNode, "length", -1, true );
						float nPitch = LocalFileMng::readXmlFloat( noteNode, "pitch", 0.0, false, false );
						QString sKey = LocalFileMng::readXmlString( noteNode, "key", "C0", false, false );
						QString nNoteOff = LocalFileMng::readXmlString( noteNode, "note_off", "false", false, false );

						bool noteoff = ( nNoteOff == "true" );

						pNote = new Note( instr, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch );
						pNote->set_key_octave( sKey );
						pNote->set_lead_lag( fLeadLag );
						pNote->set_note_off( noteoff );
						pat->insert_note( pNote ); // Add note to created pattern

						noteNode = ( QDomNode ) noteNode.nextSiblingElement( "note" );
					}
				}
				
				// Add loaded pattern to apply-list
				patterns.push_back(pat);
			}
		}
		
		patternNode = ( QDomNode ) patternNode.nextSiblingElement( "pattern" );
	}
	
	return true;
}


int LocalFileMng::savePattern( Song *song , const QString& drumkit_name, int selectedpattern , const QString& patternname, const QString& realpatternname, int mode)
{
	//int mode = 1 save, int mode = 2 save as
	// INSTRUMENT NODE

	Instrument *instr = song->get_instrument_list()->get( 0 );
	assert( instr );

	Pattern *pat = song->get_pattern_list()->get( selectedpattern );

	QString sPatternDir = Preferences::get_instance()->getDataDirectory() + "patterns/" +  drumkit_name;

	INFOLOG( "[savePattern]" + sPatternDir );

	// check if the directory exists
	QDir dir( sPatternDir );
	QDir dirPattern( sPatternDir );
	if ( !dir.exists() ) {
		dir.mkdir( sPatternDir );// create the drumkit directory
	}

	QString sPatternXmlFilename;
	// create the drumkit.xml file
	switch ( mode ){
	case 1: //save
		sPatternXmlFilename = sPatternDir + "/" + QString( patternname + QString( ".h2pattern" ));
		break;
	case 2: //save as
		sPatternXmlFilename = patternname;
		break;
	case 3: //"save" but overwrite a existing pattern. mode 3 disable the last file exist check
		sPatternXmlFilename = sPatternDir + "/" + QString( patternname + QString( ".h2pattern" ));
		break;
	case 4: //tmp pattern needed by undo/redo
		sPatternXmlFilename = patternname;
	default:
		WARNINGLOG( "Pattern Save unknown status");
		break;

	}

	//test if the file exists
	QFile testfile( sPatternXmlFilename );
	if ( testfile.exists() && mode == 1)
		return 1;

	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode rootNode = doc.createElement( "drumkit_pattern" );
	//LIB_ID just in work to get better usability
	//writeXmlString( &rootNode, "LIB_ID", "in_work" );
	writeXmlString( rootNode, "pattern_for_drumkit", drumkit_name );
	writeXmlString( rootNode, "author", song->get_author() );
	writeXmlString( rootNode, "license", song->get_license() );


	// pattern
	QDomNode patternNode = doc.createElement( "pattern" );
	writeXmlString( patternNode, "pattern_name", realpatternname );

	QString category;
	if ( pat->get_category().isEmpty() )
		category = "No category";
	else
		category = pat->get_category();

	writeXmlString( patternNode, "info", pat->get_info() );
	writeXmlString( patternNode, "category", category  );
	writeXmlString( patternNode, "size", QString("%1").arg( pat->get_length() ) );

	QDomNode noteListNode = doc.createElement( "noteList" );
	const Pattern::notes_t* notes = pat->get_notes();
	FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
		Note *pNote = it->second;
		assert( pNote );

		QDomNode noteNode = doc.createElement( "note" );
		writeXmlString( noteNode, "position", QString("%1").arg( pNote->get_position() ) );
		writeXmlString( noteNode, "leadlag", QString("%1").arg( pNote->get_lead_lag() ) );
		writeXmlString( noteNode, "velocity", QString("%1").arg( pNote->get_velocity() ) );
		writeXmlString( noteNode, "pan_L", QString("%1").arg( pNote->get_pan_l() ) );
		writeXmlString( noteNode, "pan_R", QString("%1").arg( pNote->get_pan_r() ) );
		writeXmlString( noteNode, "pitch", QString("%1").arg( pNote->get_pitch() ) );

		writeXmlString( noteNode, "key", pNote->key_to_string() );

		writeXmlString( noteNode, "length", QString("%1").arg( pNote->get_length() ) );
		writeXmlString( noteNode, "instrument", QString("%1").arg( pNote->get_instrument()->get_id() ) );
		noteListNode.appendChild( noteNode );
	}
	patternNode.appendChild( noteListNode );

	rootNode.appendChild( patternNode );




	doc.appendChild( rootNode );

	int rv = 0;
	QFile file( sPatternXmlFilename );
	if ( !file.open(QIODevice::WriteOnly) )
		rv = 1;

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );


	if( file.size() == 0)
		rv = 1;

	file.close();


	QFile anotherTestfile( sPatternXmlFilename );
	if ( !anotherTestfile.exists() )
		rv = 1;

	return rv; // ok
}




void LocalFileMng::fileCopy( const QString& sOrigFilename, const QString& sDestFilename )
{
	// TODO: use QT copy functions

	INFOLOG( sOrigFilename + " --> " + sDestFilename );

	if ( sOrigFilename == sDestFilename ) {
		return;
	}

	FILE *inputFile = fopen( sOrigFilename.toLocal8Bit(), "rb" );
	if ( inputFile == NULL ) {
		ERRORLOG( "Error opening " + sOrigFilename );
		return;
	}

	FILE *outputFile = fopen( sDestFilename.toLocal8Bit(), "wb" );
	if ( outputFile == NULL ) {
		ERRORLOG( "Error opening " + sDestFilename );
		return;
	}

	const int bufferSize = 512;
	char buffer[ bufferSize ];
	while ( feof( inputFile ) == 0 ) {
		size_t read = fread( buffer, sizeof( char ), bufferSize, inputFile );
		fwrite( buffer, sizeof( char ), read, outputFile );
	}

	fclose( inputFile );
	fclose( outputFile );
}



std::vector<QString> LocalFileMng::getSongList()
{
	std::vector<QString> list;
	QString sDirectory = Preferences::get_instance()->getDataDirectory();

	if( ! sDirectory.endsWith("/") ) {
		sDirectory += "/songs/";
	} else {
		sDirectory += "songs/";
	}

	QDir dir( sDirectory );

	if ( !dir.exists() ) {
		ERRORLOG( QString( "[getSongList] Directory %1 not found" ).arg( sDirectory ) );
	} else {
		dir.setFilter( QDir::Files );
		QFileInfoList fileList = dir.entryInfoList();

		for ( int i = 0; i < fileList.size(); ++i ) {
			QString sFile = fileList.at( i ).fileName();

			if ( ( sFile == "." ) || ( sFile == ".." ) || ( sFile == "CVS" )  || ( sFile == ".svn" ) ) {
				continue;
			}

			list.push_back( sFile.left( sFile.indexOf( "." ) ) );
		}
	}

	return list;
}

int LocalFileMng::getPatternList( const QString&  sPatternDir)
{
	std::vector<QString> list;
	QDir dir( sPatternDir );

	if ( !dir.exists() ) {
		ERRORLOG( QString( "[getPatternList] Directory %1 not found" ).arg( sPatternDir ) );
	} else {
		dir.setFilter( QDir::Files );
		QFileInfoList fileList = dir.entryInfoList();

		for ( int i = 0; i < fileList.size(); ++i ) {
			QString sFile = sPatternDir + "/" + fileList.at( i ).fileName();

			if( sFile.endsWith(".h2pattern") ){
				list.push_back( sFile/*.left( sFile.indexOf( "." ) )*/ );
			}
		}
	}
	mergeAllPatternList( list );
	return 0;
}


/**
 * Return the names of all patterns in the soundlibrary.
 * \return A vector of QString elements which represent the pattern names.
 */

std::vector<QString> LocalFileMng::getAllPatternNames()
{
	std::vector<QString> alllist;

	for (uint i = 0; i < m_allPatternList.size(); ++i) {
		QString patternInfoFile =  m_allPatternList[i];

		QDomDocument doc  = LocalFileMng::openXmlDocument( patternInfoFile );

		QDomNode rootNode =  doc.firstChildElement( "drumkit_pattern" );	// root element
		if ( rootNode.isNull() ) {
			ERRORLOG( "Error reading Pattern: Pattern_drumkit_info node not found ");
		}else{
			QDomNode patternNode = rootNode.firstChildElement( "pattern" );

			QString sPatternName( LocalFileMng::readXmlString( patternNode,"pattern_name", "" ) );
			alllist.push_back(sPatternName);
		}

	}
	return alllist;
}

/**
 * Generate a list of all used categories. This includes only patterns which are located in
 * hydrogens soundlibrary.
 * \return A vector of QString elements which represent the pattern categories.
 */

std::vector<QString> LocalFileMng::getAllCategoriesFromPattern()
{
	Preferences *pPref = H2Core::Preferences::get_instance();
	std::list<QString>::const_iterator cur_testpatternCategories;

	std::vector<QString> categorylist;
	for (uint i = 0; i < m_allPatternList.size(); ++i) {
		QString patternInfoFile =  m_allPatternList[i];
		QDomDocument doc  = LocalFileMng::openXmlDocument( patternInfoFile );


		QDomNode rootNode = doc.firstChildElement( "drumkit_pattern" );	// root element
		if ( rootNode.isNull() ) {
			ERRORLOG( "Error reading Pattern: Pattern_drumkit_info node not found ");
		}else{
			QDomNode patternNode = rootNode.firstChildElement( "pattern" );
			QString sCategoryName( LocalFileMng::readXmlString( patternNode,"category", "" ) );


			if ( sCategoryName.isEmpty() ){
				sCategoryName = "No category";
			}

			bool test = true;
			for (uint i = 0; i < categorylist.size(); ++i){
				if ( sCategoryName == categorylist[i] ){
					test = false;
				}
			}
			if (test == true){
				categorylist.push_back( sCategoryName );

				//this merge new categories to user categories list
				bool test2 = true;
				for( cur_testpatternCategories = pPref->m_patternCategories.begin(); cur_testpatternCategories != pPref->m_patternCategories.end(); ++cur_testpatternCategories ){
					if ( sCategoryName == *cur_testpatternCategories ){
						test2 = false;
					}
				}

				if (test2 == true ) {
					pPref->m_patternCategories.push_back( sCategoryName );
				}
			}

		}
	}

	std::sort(categorylist.begin(), categorylist.end());
	return categorylist;
}

/**
 * Generate a list of all patterns for a given drumkit
 * \param sDrumkit the name of drumkit
 * \return A vector of QString elements which represent the pattern names.
 */

std::vector<QString> LocalFileMng::getPatternsForDrumkit( const QString& sDrumkit )
{
	std::vector<QString> list;

	QDir dir( Preferences::get_instance()->getDataDirectory() + "/patterns/" + sDrumkit );

	if ( !dir.exists() ) {
		INFOLOG( QString( "No patterns for drumkit '%1'." ).arg( sDrumkit ) );
	} else {
		dir.setFilter( QDir::Dirs );
		QFileInfoList fileList = dir.entryInfoList();

		for ( int i = 0; i < fileList.size(); ++i ) {
			QString sFile = fileList.at( i ).fileName();

			if ( ( sFile == "." ) || ( sFile == ".." ) || ( sFile == "CVS" )  || ( sFile == ".svn" ) ) {
				continue;
			}

			list.push_back( sFile.left( sFile.indexOf( "." ) ) );
		}
	}

	return list;
}

/**
 * Return a list of drumkits that are located inside a directory.
 * \param sDirectory The directory where the method looks for drumkits
 * \return A vector of QString elements which represent the drumkits.
 */

std::vector<QString> LocalFileMng::getDrumkitsFromDirectory( QString sDirectory )
{
	std::vector<QString> list;

	QDir dir( sDirectory );
	if ( !dir.exists() ) {
		ERRORLOG( QString( "[getDrumkitList] Directory %1 not found" ).arg( sDirectory ) );
	} else {
		dir.setFilter( QDir::Dirs );
		QFileInfoList fileList = dir.entryInfoList();

		for ( int i = 0; i < fileList.size(); ++i ) {
			QString sFile = fileList.at( i ).fileName();
			if ( ( sFile == "." ) || ( sFile == ".." ) || ( sFile == "CVS" )  || ( sFile == ".svn" ) ||
				 (sFile =="songs" ) || ( sFile == "patterns" )  || (sFile == "drumkits" || sFile == "playlists" ) || (sFile == "scripts" )) {
				continue;
			}
			if(! sDirectory.endsWith("/")) sDirectory = sDirectory + "/";
			list.push_back( sDirectory + sFile );
		}
	}

	return list;
}


/**
 * Merge two vectors of QStrings.
 * \param firstVector The first vector.
 * \param secondVector The second vector.
 * \return The resulting vector which is a union of firstVector and secondVector.
 */

std::vector<QString> mergeQStringVectors( std::vector<QString> firstVector , std::vector<QString> secondVector )
{

	if( firstVector.size() == 0 ) return secondVector;
	if( secondVector.size() == 0 ) return firstVector;

	std::vector<QString> newVector;

	newVector = firstVector;
	newVector.resize(firstVector.size()+ secondVector.size());


	for ( int i = 0; i < (int)secondVector.size(); ++i )
	{
		QString toFind = secondVector[i];

		for ( int ii = 0; ii < (int)firstVector.size(); ++ii )
		{
			if( toFind == firstVector[ii])
			{
				//the String already exists in firstVector, don't copy it to the resulting vector
				break;
			}
		}
		newVector[firstVector.size() + i] = toFind;
	}

	return newVector;
}


/**
 * Return a list of directories that may contain patterns
 * \return A vector of QString elements which represent the directory names.
 */

std::vector<QString> LocalFileMng::getPatternDirList()
{
	return getDrumkitsFromDirectory( Preferences::get_instance()->getDataDirectory() + "patterns" );
}


int  LocalFileMng::mergeAllPatternList( std::vector<QString> current )
{
	m_allPatternList = mergeQStringVectors (m_allPatternList, current );
	return 0;
}


/**
 * Save the currently loaded playlist to disk.
 * \param playlist_name The filename of the output file.
 * \return Returns an Errorcode.
 */

int LocalFileMng::savePlayList( const std::string& filename)
{

	std::string name = filename.c_str();

	std::string realname = name.substr(name.rfind("/")+1);

	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode rootNode = doc.createElement( "playlist" );

	writeXmlString( rootNode, "Name", QString (realname.c_str()) );

	QDomNode playlistNode = doc.createElement( "Songs" );
	for ( uint i = 0; i < Hydrogen::get_instance()->m_PlayList.size(); ++i ){
		QDomNode nextNode = doc.createElement( "next" );

		LocalFileMng::writeXmlString ( nextNode, "song", Hydrogen::get_instance()->m_PlayList[i].m_hFile );
		LocalFileMng::writeXmlString ( nextNode, "script", Hydrogen::get_instance()->m_PlayList[i].m_hScript );
		LocalFileMng::writeXmlString ( nextNode, "enabled", Hydrogen::get_instance()->m_PlayList[i].m_hScriptEnabled );

		playlistNode.appendChild( nextNode );
	}

	rootNode.appendChild( playlistNode );
	doc.appendChild( rootNode );

	int rv = 0;
	QFile file( filename.c_str() );
	if ( !file.open(QIODevice::WriteOnly) )
		rv = 1;

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );

	if( file.size() == 0)
		rv = 1;

	file.close();

	return rv; // ok

}

/**
 * Load a playlist from disk.
 * \param filename The name of the playlist to be saved.
 * \return Returns an Errorcode.
 */

int LocalFileMng::loadPlayList( const std::string& filename)
{
	std::string playlistInfoFile = filename;
	std::ifstream verify( playlistInfoFile.c_str() , std::ios::in | std::ios::binary );
	if ( verify == NULL ) {
		return 1;
	}

	QDomDocument doc = LocalFileMng::openXmlDocument( QString( filename.c_str() ) );

	Hydrogen::get_instance()->m_PlayList.clear();

	QDomNode rootNode = doc.firstChildElement( "playlist" );	// root element
	if ( rootNode.isNull() ) {
		ERRORLOG( "Error reading playlist: playlist node not found" );
		return 1;
	}
	QDomNode playlistNode = rootNode.firstChildElement( "Songs" );

	if ( ! playlistNode.isNull() ) {
		Hydrogen::get_instance()->m_PlayList.clear();
		QDomNode nextNode = playlistNode.firstChildElement( "next" );
		SongReader reader;
		while (  ! nextNode.isNull() ) {
			Hydrogen::HPlayListNode playListItem;
			playListItem.m_hFile = LocalFileMng::readXmlString( nextNode, "song", "" );
			QString FilePath = reader.getPath( playListItem.m_hFile );
			playListItem.m_hFileExists = Filesystem::file_readable( FilePath );
			playListItem.m_hScript = LocalFileMng::readXmlString( nextNode, "script", "" );
			playListItem.m_hScriptEnabled = LocalFileMng::readXmlString( nextNode, "enabled", "" );

			Hydrogen::get_instance()->m_PlayList.push_back( playListItem );
			nextNode = nextNode.nextSiblingElement( "next" );
		}
	}
	return 0; // ok
}



/* New QtXml based methods */

QString LocalFileMng::readXmlString( QDomNode node , const QString& nodeName, const QString& defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QDomElement element = node.firstChildElement( nodeName );

	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			return element.text();
		} else {
			if ( !bCanBeEmpty ) {
				_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	} else {
		if(  bShouldExists ){
			_WARNINGLOG( "'" + nodeName + "' node not found" );

		}
		return defaultValue;
	}
}

float LocalFileMng::readXmlFloat( QDomNode node , const QString& nodeName, float defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QLocale c_locale = QLocale::c();
	QDomElement element = node.firstChildElement( nodeName );

	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			return c_locale.toFloat(element.text());
		} else {
			if ( !bCanBeEmpty ) {
				_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	} else {
		if(  bShouldExists ){
			_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}

int LocalFileMng::readXmlInt( QDomNode node , const QString& nodeName, int defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QLocale c_locale = QLocale::c();
	QDomElement element = node.firstChildElement( nodeName );

	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			return c_locale.toInt( element.text() );
		} else {
			if ( !bCanBeEmpty ) {
				_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	} else {
		if(  bShouldExists ){
			_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}

bool LocalFileMng::readXmlBool( QDomNode node , const QString& nodeName, bool defaultValue, bool bShouldExists, bool tinyXmlCompatMode)
{
	QDomElement element = node.firstChildElement( nodeName );

	if( !node.isNull() && !element.isNull() ){
		if(  !element.text().isEmpty() ){
			if( element.text() == "true"){
				return true;
			} else {
				return false;
			}
		} else {
			_WARNINGLOG( "Using default value in " + nodeName );
			return defaultValue;
		}
	} else {
		if(  bShouldExists ){
			_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}


void LocalFileMng::writeXmlString( QDomNode parent, const QString& name, const QString& text )
{
	QDomDocument doc;
	QDomElement elem = doc.createElement( name );
	QDomText t = doc.createTextNode( text );
	elem.appendChild( t );
	parent.appendChild( elem );
}



void LocalFileMng::writeXmlBool( QDomNode parent, const QString& name, bool value )
{
	if ( value ) {
		writeXmlString( parent, name, QString( "true" ) );
	} else {
		writeXmlString( parent, name, QString( "false" ) );
	}
}

/* Convert (in-place) an XML escape sequence into a literal byte,
 * rather than the character it actually refers to.
 */
void LocalFileMng::convertFromTinyXMLString( QByteArray* str )
{
	/* When TinyXML encountered a non-ASCII character, it would
	 * simply write the character as "&#xx;" -- where "xx" is
	 * the hex character code.  However, this doesn't respect
	 * any encodings (e.g. UTF-8, UTF-16).  In XML, &#xx; literally
	 * means "the Unicode character # xx."  However, in a UTF-8
	 * sequence, this could be an escape character that tells
	 * whether we have a 2, 3, or 4-byte UTF-8 sequence.
	 *
	 * For example, the UTF-8 sequence 0xD184 was being written
	 * by TinyXML as "&#xD1;&#x84;".  However, this is the UTF-8
	 * sequence for the cyrillic small letter EF (which looks
	 * kind of like a thorn or a greek phi).  This letter, in
	 * XML, should be saved as &#x00000444;, or even literally
	 * (no escaping).  As a consequence, when &#xD1; is read
	 * by an XML parser, it will be interpreted as capital N
	 * with a tilde (~).  Then &#x84; will be interpreted as
	 * an unknown or control character.
	 *
	 * So, when we know that TinyXML wrote the file, we can
	 * simply exchange these hex sequences to literal bytes.
	 */
	int pos = 0;

	pos = str->indexOf("&#x");
	while( pos != -1 ) {
		if( isxdigit(str->at(pos+3))
				&& isxdigit(str->at(pos+4))
				&& (str->at(pos+5) == ';') ) {
			char w1 = str->at(pos+3);
			char w2 = str->at(pos+4);

			w1 = tolower(w1) - 0x30;  // '0' = 0x30
			if( w1 > 9 ) w1 -= 0x27;  // '9' = 0x39, 'a' = 0x61
			w1 = (w1 & 0xF);

			w2 = tolower(w2) - 0x30;  // '0' = 0x30
			if( w2 > 9 ) w2 -= 0x27;  // '9' = 0x39, 'a' = 0x61
			w2 = (w2 & 0xF);

			char ch = (w1 << 4) | w2;
			(*str)[pos] = ch;
			++pos;
			str->remove(pos, 5);
		}
		pos = str->indexOf("&#x");
	}
}

bool LocalFileMng::checkTinyXMLCompatMode( const QString& filename )
{
	/*
		Check if filename was created with TinyXml or QtXml
		TinyXML: return true
		QtXml: return false
	*/

	QFile file( filename );

	if ( !file.open(QIODevice::ReadOnly) )
		return false;

	QString line = file.readLine();
	file.close();
	if ( line.startsWith( "<?xml" )){
		return false;
	} else  {
		_WARNINGLOG( QString("File '%1' is being read in "
							 "TinyXML compatability mode")
					 .arg(filename) );
		return true;
	}



}

QDomDocument LocalFileMng::openXmlDocument( const QString& filename )
{
	bool TinyXMLCompat = LocalFileMng::checkTinyXMLCompatMode( filename );

	QDomDocument doc;
	QFile file( filename );

	if ( !file.open(QIODevice::ReadOnly) )
		return QDomDocument();

	if( TinyXMLCompat ) {
		QString enc = QTextCodec::codecForLocale()->name();
		if( enc == QString("System") ) {
			enc = "UTF-8";
		}
		QByteArray line;
		QByteArray buf = QString("<?xml version='1.0' encoding='%1' ?>\n")
				.arg( enc )
				.toLocal8Bit();

		while( !file.atEnd() ) {
			line = file.readLine();
			LocalFileMng::convertFromTinyXMLString( &line );
			buf += line;
		}

		if( ! doc.setContent( buf ) ) {
			file.close();
			return QDomDocument();
		}

	} else {
		if( ! doc.setContent( &file ) ) {
			file.close();
			return QDomDocument();
		}
	}
	file.close();

	return doc;
}


int LocalFileMng::writeTempPatternList(Song *song, const QString& filename)
{
	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );


	QDomNode tempPatternListNode = doc.createElement( "tempPatternList" );

	unsigned nPatterns = song->get_pattern_list()->size();

	QDomNode virtualPatternListNode = doc.createElement( "virtualPatternList" );
	for ( unsigned i = 0; i < nPatterns; i++ ) {
		Pattern *pat = song->get_pattern_list()->get( i );

		// pattern
		if (pat->get_virtual_patterns()->empty() == false) {
			QDomNode patternNode = doc.createElement( "pattern" );
			LocalFileMng::writeXmlString( patternNode, "name", pat->get_name() );

			for (Pattern::virtual_patterns_it_t  virtIter = pat->get_virtual_patterns()->begin(); virtIter != pat->get_virtual_patterns()->end(); ++virtIter) {
				LocalFileMng::writeXmlString( patternNode, "virtual", (*virtIter)->get_name() );
			}//for

			virtualPatternListNode.appendChild( patternNode );
		}//if
	}//for
	tempPatternListNode.appendChild(virtualPatternListNode);

	// pattern sequence
	QDomNode patternSequenceNode = doc.createElement( "patternSequence" );

	unsigned nPatternGroups = song->get_pattern_group_vector()->size();
	for ( unsigned i = 0; i < nPatternGroups; i++ ) {
		QDomNode groupNode = doc.createElement( "group" );

		PatternList *pList = ( *song->get_pattern_group_vector() )[i];
		for ( unsigned j = 0; j < pList->size(); j++ ) {
			Pattern *pPattern = pList->get( j );
			LocalFileMng::writeXmlString( groupNode, "patternID", pPattern->get_name() );
		}
		patternSequenceNode.appendChild( groupNode );
	}

	tempPatternListNode.appendChild( patternSequenceNode );
	doc.appendChild(tempPatternListNode);

	QFile file(filename);
	if ( !file.open(QIODevice::WriteOnly) )
		return 0;

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );

	file.close();

	return 0; // ok

}

//-----------------------------------------------------------------------------
//	Implementation of SongWriter class
//-----------------------------------------------------------------------------

const char* SongWriter::__class_name = "SongWriter";

SongWriter::SongWriter()
	: Object( __class_name )
{
	//	infoLog("init");
}



SongWriter::~SongWriter()
{
	//	infoLog("destroy");
}


/*
* This methods decides if a filename should be stored
* with a relative path or with an absolute path.
* Files are getting stored with a relative path if the
* file relates to a valid drumkit which is stored either
* in the user directory or in the system directory.
* Otherwise, the file is stored with an absolute path.
* A relative path is relative to the drumkit dir.
*/

QString prepare_filename( QString fname)
{
	if( Filesystem::file_is_partof_drumkit( fname ) ){
		if( fname.startsWith( Filesystem::usr_drumkits_dir() ) )
		{
			fname.remove( 0, Filesystem::usr_drumkits_dir().size() + 1 );
			return	fname.remove( 0, fname.indexOf(("/")) );
		}

		if( fname.startsWith( Filesystem::sys_drumkits_dir() ) )
		{
			fname.remove( 0, Filesystem::sys_drumkits_dir().size() + 1 );
			return	fname.remove( 0, fname.indexOf(("/")) );
		}
	}
	return fname;
}

// Returns 0 on success, passes the TinyXml error code otherwise.
int SongWriter::writeSong( Song *song, const QString& filename )
{
	INFOLOG( "Saving song " + filename );
	int rv = 0; // return value

	// FIXME: has the file write-permssion?
	// FIXME: verificare che il file non sia gia' esistente
	// FIXME: effettuare copia di backup per il file gia' esistente


	QDomDocument doc;
	QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
	doc.appendChild( header );

	QDomNode songNode = doc.createElement( "song" );

	LocalFileMng::writeXmlString( songNode, "version", QString( get_version().c_str() ) );
	LocalFileMng::writeXmlString( songNode, "bpm", QString("%1").arg( song->__bpm ) );
	LocalFileMng::writeXmlString( songNode, "volume", QString("%1").arg( song->get_volume() ) );
	LocalFileMng::writeXmlString( songNode, "metronomeVolume", QString("%1").arg( song->get_metronome_volume() ) );
	LocalFileMng::writeXmlString( songNode, "name", song->__name );
	LocalFileMng::writeXmlString( songNode, "author", song->__author );
	LocalFileMng::writeXmlString( songNode, "notes", song->get_notes() );
	LocalFileMng::writeXmlString( songNode, "license", song->get_license() );
	LocalFileMng::writeXmlBool( songNode, "loopEnabled", song->is_loop_enabled() );
	LocalFileMng::writeXmlBool( songNode, "patternModeMode", Preferences::get_instance()->patternModePlaysSelected());

	if ( song->get_mode() == Song::SONG_MODE ) {
		LocalFileMng::writeXmlString( songNode, "mode", QString( "song" ) );
	} else {
		LocalFileMng::writeXmlString( songNode, "mode", QString( "pattern" ) );
	}

	LocalFileMng::writeXmlString( songNode, "humanize_time", QString("%1").arg( song->get_humanize_time_value() ) );
	LocalFileMng::writeXmlString( songNode, "humanize_velocity", QString("%1").arg( song->get_humanize_velocity_value() ) );
	LocalFileMng::writeXmlString( songNode, "swing_factor", QString("%1").arg( song->get_swing_factor() ) );

	// instrument list
	QDomNode instrumentListNode = doc.createElement( "instrumentList" );
	unsigned nInstrument = song->get_instrument_list()->size();

	// INSTRUMENT NODE
	for ( unsigned i = 0; i < nInstrument; i++ ) {
		Instrument *instr = song->get_instrument_list()->get( i );
		assert( instr );

		QDomNode instrumentNode = doc.createElement( "instrument" );

		LocalFileMng::writeXmlString( instrumentNode, "id", QString("%1").arg( instr->get_id() ) );
		LocalFileMng::writeXmlString( instrumentNode, "name", instr->get_name() );
		LocalFileMng::writeXmlString( instrumentNode, "drumkit", instr->get_drumkit_name() );
		LocalFileMng::writeXmlString( instrumentNode, "volume", QString("%1").arg( instr->get_volume() ) );
		LocalFileMng::writeXmlBool( instrumentNode, "isMuted", instr->is_muted() );
		LocalFileMng::writeXmlString( instrumentNode, "pan_L", QString("%1").arg( instr->get_pan_l() ) );
		LocalFileMng::writeXmlString( instrumentNode, "pan_R", QString("%1").arg( instr->get_pan_r() ) );
		LocalFileMng::writeXmlString( instrumentNode, "gain", QString("%1").arg( instr->get_gain() ) );

		LocalFileMng::writeXmlBool( instrumentNode, "filterActive", instr->is_filter_active() );
		LocalFileMng::writeXmlString( instrumentNode, "filterCutoff", QString("%1").arg( instr->get_filter_cutoff() ) );
		LocalFileMng::writeXmlString( instrumentNode, "filterResonance", QString("%1").arg( instr->get_filter_resonance() ) );

		LocalFileMng::writeXmlString( instrumentNode, "FX1Level", QString("%1").arg( instr->get_fx_level( 0 ) ) );
		LocalFileMng::writeXmlString( instrumentNode, "FX2Level", QString("%1").arg( instr->get_fx_level( 1 ) ) );
		LocalFileMng::writeXmlString( instrumentNode, "FX3Level", QString("%1").arg( instr->get_fx_level( 2 ) ) );
		LocalFileMng::writeXmlString( instrumentNode, "FX4Level", QString("%1").arg( instr->get_fx_level( 3 ) ) );

		assert( instr->get_adsr() );
		LocalFileMng::writeXmlString( instrumentNode, "Attack", QString("%1").arg( instr->get_adsr()->get_attack() ) );
		LocalFileMng::writeXmlString( instrumentNode, "Decay", QString("%1").arg( instr->get_adsr()->get_decay() ) );
		LocalFileMng::writeXmlString( instrumentNode, "Sustain", QString("%1").arg( instr->get_adsr()->get_sustain() ) );
		LocalFileMng::writeXmlString( instrumentNode, "Release", QString("%1").arg( instr->get_adsr()->get_release() ) );

		LocalFileMng::writeXmlString( instrumentNode, "randomPitchFactor", QString("%1").arg( instr->get_random_pitch_factor() ) );

		LocalFileMng::writeXmlString( instrumentNode, "muteGroup", QString("%1").arg( instr->get_mute_group() ) );
		LocalFileMng::writeXmlBool( instrumentNode, "isStopNote", instr->is_stop_notes() );

		LocalFileMng::writeXmlString( instrumentNode, "midiOutChannel", QString("%1").arg( instr->get_midi_out_channel() ) );
		LocalFileMng::writeXmlString( instrumentNode, "midiOutNote", QString("%1").arg( instr->get_midi_out_note() ) );

		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer *pLayer = instr->get_layer( nLayer );
			if ( pLayer == NULL ) continue;
			Sample *pSample = pLayer->get_sample();
			if ( pSample == NULL ) continue;

			bool sIsModified = pSample->get_is_modified();
			Sample::Loops lo = pSample->get_loops();
			Sample::Rubberband ro = pSample->get_rubberband();
			QString sMode = pSample->get_loop_mode_string();


			QDomNode layerNode = doc.createElement( "layer" );
			LocalFileMng::writeXmlString( layerNode, "filename", prepare_filename( pSample->get_filepath() ) );
			LocalFileMng::writeXmlBool( layerNode, "ismodified", sIsModified);
			LocalFileMng::writeXmlString( layerNode, "smode", pSample->get_loop_mode_string() );
			LocalFileMng::writeXmlString( layerNode, "startframe", QString("%1").arg( lo.start_frame ) );
			LocalFileMng::writeXmlString( layerNode, "loopframe", QString("%1").arg( lo.loop_frame ) );
			LocalFileMng::writeXmlString( layerNode, "loops", QString("%1").arg( lo.count ) );
			LocalFileMng::writeXmlString( layerNode, "endframe", QString("%1").arg( lo.end_frame ) );
			LocalFileMng::writeXmlString( layerNode, "userubber", QString("%1").arg( ro.use ) );
			LocalFileMng::writeXmlString( layerNode, "rubberdivider", QString("%1").arg( ro.divider ) );
			LocalFileMng::writeXmlString( layerNode, "rubberCsettings", QString("%1").arg( ro.c_settings ) );
			LocalFileMng::writeXmlString( layerNode, "rubberPitch", QString("%1").arg( ro.pitch ) );
			LocalFileMng::writeXmlString( layerNode, "min", QString("%1").arg( pLayer->get_start_velocity() ) );
			LocalFileMng::writeXmlString( layerNode, "max", QString("%1").arg( pLayer->get_end_velocity() ) );
			LocalFileMng::writeXmlString( layerNode, "gain", QString("%1").arg( pLayer->get_gain() ) );
			LocalFileMng::writeXmlString( layerNode, "pitch", QString("%1").arg( pLayer->get_pitch() ) );


			Sample::VelocityEnvelope* velocity = pSample->get_velocity_envelope();
			for (int y = 0; y < velocity->size(); y++){
				QDomNode volumeNode = doc.createElement( "volume" );
				LocalFileMng::writeXmlString( volumeNode, "volume-position", QString("%1").arg( velocity->at(y).frame ) );
				LocalFileMng::writeXmlString( volumeNode, "volume-value", QString("%1").arg( velocity->at(y).value ) );
				layerNode.appendChild( volumeNode );
			}

			Sample::PanEnvelope* pan = pSample->get_pan_envelope();
			for (int y = 0; y < pan->size(); y++){
				QDomNode panNode = doc.createElement( "pan" );
				LocalFileMng::writeXmlString( panNode, "pan-position", QString("%1").arg( pan->at(y).frame ) );
				LocalFileMng::writeXmlString( panNode, "pan-value", QString("%1").arg( pan->at(y).value ) );
				layerNode.appendChild( panNode );
			}

			instrumentNode.appendChild( layerNode );
		}

		instrumentListNode.appendChild( instrumentNode );
	}
	songNode.appendChild( instrumentListNode );


	// pattern list
	QDomNode patternListNode = doc.createElement( "patternList" );

	unsigned nPatterns = song->get_pattern_list()->size();
	for ( unsigned i = 0; i < nPatterns; i++ ) {
		Pattern *pat = song->get_pattern_list()->get( i );

		// pattern
		QDomNode patternNode = doc.createElement( "pattern" );
		LocalFileMng::writeXmlString( patternNode, "name", pat->get_name() );
		LocalFileMng::writeXmlString( patternNode, "category", pat->get_category() );
		LocalFileMng::writeXmlString( patternNode, "size", QString("%1").arg( pat->get_length() ) );

		QDomNode noteListNode = doc.createElement( "noteList" );
		const Pattern::notes_t* notes = pat->get_notes();
		FOREACH_NOTE_CST_IT_BEGIN_END(notes,it) {
			Note *pNote = it->second;
			assert( pNote );

			QDomNode noteNode = doc.createElement( "note" );
			LocalFileMng::writeXmlString( noteNode, "position", QString("%1").arg( pNote->get_position() ) );
			LocalFileMng::writeXmlString( noteNode, "leadlag", QString("%1").arg( pNote->get_lead_lag() ) );
			LocalFileMng::writeXmlString( noteNode, "velocity", QString("%1").arg( pNote->get_velocity() ) );
			LocalFileMng::writeXmlString( noteNode, "pan_L", QString("%1").arg( pNote->get_pan_l() ) );
			LocalFileMng::writeXmlString( noteNode, "pan_R", QString("%1").arg( pNote->get_pan_r() ) );
			LocalFileMng::writeXmlString( noteNode, "pitch", QString("%1").arg( pNote->get_pitch() ) );

			LocalFileMng::writeXmlString( noteNode, "key", pNote->key_to_string() );

			LocalFileMng::writeXmlString( noteNode, "length", QString("%1").arg( pNote->get_length() ) );
			LocalFileMng::writeXmlString( noteNode, "instrument", QString("%1").arg( pNote->get_instrument()->get_id() ) );

			QString noteoff = "false";
			if ( pNote->get_note_off() ) noteoff = "true";
			LocalFileMng::writeXmlString( noteNode, "note_off", noteoff );
			noteListNode.appendChild( noteNode );

		}
		patternNode.appendChild( noteListNode );

		patternListNode.appendChild( patternNode );
	}
	songNode.appendChild( patternListNode );

	QDomNode virtualPatternListNode = doc.createElement( "virtualPatternList" );
	for ( unsigned i = 0; i < nPatterns; i++ ) {
		Pattern *pat = song->get_pattern_list()->get( i );

		// pattern
		if (pat->get_virtual_patterns()->empty() == false) {
			QDomNode patternNode = doc.createElement( "pattern" );
			LocalFileMng::writeXmlString( patternNode, "name", pat->get_name() );

			for (Pattern::virtual_patterns_it_t  virtIter = pat->get_virtual_patterns()->begin(); virtIter != pat->get_virtual_patterns()->end(); ++virtIter) {
				LocalFileMng::writeXmlString( patternNode, "virtual", (*virtIter)->get_name() );
			}//for

			virtualPatternListNode.appendChild( patternNode );
		}//if
	}//for
	songNode.appendChild(virtualPatternListNode);

	// pattern sequence
	QDomNode patternSequenceNode = doc.createElement( "patternSequence" );

	unsigned nPatternGroups = song->get_pattern_group_vector()->size();
	for ( unsigned i = 0; i < nPatternGroups; i++ ) {
		QDomNode groupNode = doc.createElement( "group" );

		PatternList *pList = ( *song->get_pattern_group_vector() )[i];
		for ( unsigned j = 0; j < pList->size(); j++ ) {
			Pattern *pPattern = pList->get( j );
			LocalFileMng::writeXmlString( groupNode, "patternID", pPattern->get_name() );
		}
		patternSequenceNode.appendChild( groupNode );
	}

	songNode.appendChild( patternSequenceNode );


	// LADSPA FX
	QDomNode ladspaFxNode = doc.createElement( "ladspa" );

	for ( unsigned nFX = 0; nFX < MAX_FX; nFX++ ) {
		QDomNode fxNode = doc.createElement( "fx" );

#ifdef H2CORE_HAVE_LADSPA
		LadspaFX *pFX = Effects::get_instance()->getLadspaFX( nFX );
		if ( pFX ) {
			LocalFileMng::writeXmlString( fxNode, "name", pFX->getPluginLabel() );
			LocalFileMng::writeXmlString( fxNode, "filename", pFX->getLibraryPath() );
			LocalFileMng::writeXmlBool( fxNode, "enabled", pFX->isEnabled() );
			LocalFileMng::writeXmlString( fxNode, "volume", QString("%1").arg( pFX->getVolume() ) );
			for ( unsigned nControl = 0; nControl < pFX->inputControlPorts.size(); nControl++ ) {
				LadspaControlPort *pControlPort = pFX->inputControlPorts[ nControl ];
				QDomNode controlPortNode = doc.createElement( "inputControlPort" );
				LocalFileMng::writeXmlString( controlPortNode, "name", pControlPort->sName );
				LocalFileMng::writeXmlString( controlPortNode, "value", QString("%1").arg( pControlPort->fControlValue ) );
				fxNode.appendChild( controlPortNode );
			}
			for ( unsigned nControl = 0; nControl < pFX->outputControlPorts.size(); nControl++ ) {
				LadspaControlPort *pControlPort = pFX->inputControlPorts[ nControl ];
				QDomNode controlPortNode = doc.createElement( "outputControlPort" );
				LocalFileMng::writeXmlString( controlPortNode, "name", pControlPort->sName );
				LocalFileMng::writeXmlString( controlPortNode, "value", QString("%1").arg( pControlPort->fControlValue ) );
				fxNode.appendChild( controlPortNode );
			}
		}
#else
		if ( false ) {
		}
#endif
		else {
			LocalFileMng::writeXmlString( fxNode, "name", QString( "no plugin" ) );
			LocalFileMng::writeXmlString( fxNode, "filename", QString( "-" ) );
			LocalFileMng::writeXmlBool( fxNode, "enabled", false );
			LocalFileMng::writeXmlString( fxNode, "volume", "0.0" );
		}
		ladspaFxNode.appendChild( fxNode );
	}

	songNode.appendChild( ladspaFxNode );
	doc.appendChild( songNode );


	//bpm time line
	QDomNode bpmTimeLine = doc.createElement( "BPMTimeLine" );
	if(Hydrogen::get_instance()->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinevector.size()); t++){
			QDomNode newBPMNode = doc.createElement( "newBPM" );
			LocalFileMng::writeXmlString( newBPMNode, "BAR",QString("%1").arg( Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebeat ));
			LocalFileMng::writeXmlString( newBPMNode, "BPM", QString("%1").arg( Hydrogen::get_instance()->m_timelinevector[t].m_htimelinebpm  ) );
			bpmTimeLine.appendChild( newBPMNode );
		}
	}
	songNode.appendChild( bpmTimeLine );

	//time line tag
	QDomNode timeLineTag = doc.createElement( "timeLineTag" );
	if(Hydrogen::get_instance()->m_timelinetagvector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(Hydrogen::get_instance()->m_timelinetagvector.size()); t++){
			QDomNode newTAGNode = doc.createElement( "newTAG" );
			LocalFileMng::writeXmlString( newTAGNode, "BAR",QString("%1").arg( Hydrogen::get_instance()->m_timelinetagvector[t].m_htimelinetagbeat ));
			LocalFileMng::writeXmlString( newTAGNode, "TAG", QString("%1").arg( Hydrogen::get_instance()->m_timelinetagvector[t].m_htimelinetag  ) );
			timeLineTag.appendChild( newTAGNode );
		}
	}
	songNode.appendChild( timeLineTag );

	QFile file(filename);
	if ( !file.open(QIODevice::WriteOnly) )
		rv = 1;

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );

	if( file.size() == 0)
		rv = 1;

	file.close();

	if( rv ) {
		WARNINGLOG("File save reported an error.");
	} else {
		song->__is_modified = false;
		INFOLOG("Save was successful.");
	}

	song->set_filename( filename );

	return rv;
}

};

