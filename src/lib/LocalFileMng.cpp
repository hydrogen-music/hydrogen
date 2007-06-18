/*
 * Hydrogen
 * Copyright(c) 2002-2005 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <cstdlib>
#include <sys/stat.h>
#include <qdir.h>
#include <assert.h>

#include "config.h"
#include "DataPath.h"
#include "Song.h"
#include "LocalFileMng.h"
#include "xml/tinyxml.h"
#include "Preferences.h"
#include "Sample.h"
#include "ADSR.h"
#include "fx/LadspaFX.h"

LocalFileMng::LocalFileMng() : Object( "LocalFileMng" )
{
//	infoLog("INIT");
}



LocalFileMng::~LocalFileMng(){
//	infoLog("DESTROY");
}



void LocalFileMng::fileCopy( const string& sOrigFilename, const string& sDestFilename )
{
	infoLog( "[fileCopy] " + sOrigFilename + " --> " + sDestFilename );

	if ( sOrigFilename == sDestFilename ) {
		return;
	}

	FILE *inputFile = fopen( sOrigFilename.c_str(), "rb" );
	if (inputFile == NULL) {
		errorLog( "[fileCopy] Error opening " + sOrigFilename );
		return;
	}

	FILE *outputFile = fopen( sDestFilename.c_str(), "wb" );
	if (outputFile == NULL) {
		errorLog( "[fileCopy] Error opening " + sDestFilename );
		return;
	}

	const int bufferSize = 512;
	char buffer[ bufferSize ];
	while( feof( inputFile ) == 0 ) {
		size_t read = fread( buffer, sizeof(char), bufferSize, inputFile );
		fwrite( buffer, sizeof(char), read, outputFile );
	}

	fclose( inputFile );
	fclose( outputFile );
	infoLog( "[fileCopy] fine della copia" );
}




vector<string> LocalFileMng::listUserDrumkits() {
	vector<string> list;

	QString sDirectory = QDir::homeDirPath().append( "/.hydrogen/data" );
	QDir dir(sDirectory);
	if ( !dir.exists() ) {
		errorLog( string( "[listUserDrumkits] Directory ").append( sDirectory.latin1() ).append( " not found." ) );
	}
	else {
		const QFileInfoList *pList = dir.entryInfoList();
		QFileInfoListIterator it( *pList );
		QFileInfo *pFileInfo;

		dir.setFilter( QDir::Dirs );
		while ( (pFileInfo = it.current()) != 0 ) {
			string sFile = pFileInfo->fileName().latin1();
			if( ( sFile == "." ) || ( sFile == ".." ) ){
				++it;
				continue;
			}
			list.push_back( sFile );
			++it;
		}
	}
	return list;
}




vector<string> LocalFileMng::listSystemDrumkits()
{
	vector<string> list;

	QString sDirectory = QString( DataPath::getDataPath().append( "/drumkits" ).c_str() );
	QDir dir(sDirectory);
	if ( !dir.exists() ) {
		errorLog( string( "[listSystemDrumkits] Directory ").append( sDirectory.latin1() ).append( " not found." ) );
	}
	else {
		const QFileInfoList *pList = dir.entryInfoList();
		QFileInfoListIterator it( *pList );
		QFileInfo *pFileInfo;

		dir.setFilter( QDir::Dirs );
		while ( (pFileInfo = it.current()) != 0 ) {
			string sFile = pFileInfo->fileName().latin1();
			if( ( sFile == "." ) || ( sFile == ".." ) ){
				++it;
				continue;
			}
			list.push_back( sFile );
			++it;
		}
	}
	return list;
}




string LocalFileMng::getDrumkitDirectory( string drumkitName ) {
	// search in system drumkit
	vector<string> systemDrumkits = listSystemDrumkits();
	for ( unsigned i = 0; i < systemDrumkits.size(); i++ ) {
		if ( systemDrumkits[ i ] == drumkitName ) {
			string path = string( DataPath::getDataPath() ) + "/drumkits/";
			return path;
		}
	}

	// search in user drumkit
	vector<string> userDrumkits = listUserDrumkits();
	for ( unsigned i = 0; i < userDrumkits.size(); i++ ) {
		if ( userDrumkits[ i ] == drumkitName ) {
			string path = QDir::homeDirPath().append("/.hydrogen/data/").ascii();
			return path;
		}
	}

	errorLog( "[getDrumkitDirectory] drumkit \"" + drumkitName + "\" not found" );
	return "";	// FIXME
}



/// Restituisce un oggetto DrumkitInfo.
/// Gli strumenti non hanno dei veri propri sample,
/// viene utilizzato solo il campo filename.
DrumkitInfo* LocalFileMng::loadDrumkit( string directory )
{
//	infoLog( "loadDrumkit " + directory );

	// che if the drumkit.xml file exists
	string drumkitInfoFile = directory + "/drumkit.xml";
	std::ifstream verify( drumkitInfoFile.c_str() , std::ios::in | std::ios::binary );
	if (verify == NULL){
		errorLog( "[loadDrumkit] Load Instrument: Data file " + drumkitInfoFile + " not found." );
		return NULL;
	}

	TiXmlDocument doc( drumkitInfoFile.c_str() );
	doc.LoadFile();

	// root element
	TiXmlNode* drumkitNode;	// root element
	if ( !( drumkitNode = doc.FirstChild( "drumkit_info" ) ) ) {
		errorLog( "[loadDrumkit] Error reading drumkit: drumkit_info node not found" );
		return NULL;
	}

	// Name
	string name = readXmlString( this, drumkitNode, "name", "");
	if (name == "") {
		errorLog( "[loadDrumkit] Error reading drumkit: name node not found" );
		return NULL;
	}

	string author = readXmlString( this, drumkitNode, "author", "undefined author", true );
	string info = readXmlString( this, drumkitNode, "info", "defaultInfo", true );

	DrumkitInfo *drumkitInfo = new DrumkitInfo();
	drumkitInfo->setName( name );
	drumkitInfo->setAuthor( author );
	drumkitInfo->setInfo( info );

	InstrumentList *instrumentList = new InstrumentList();

	TiXmlNode* instrumentListNode;
	if ( (instrumentListNode = drumkitNode->FirstChild( "instrumentList" ) ) ) {
		// INSTRUMENT NODE
		int instrumentList_count = 0;
		TiXmlNode* instrumentNode = 0;
		for( instrumentNode = instrumentListNode->FirstChild("instrument"); instrumentNode; instrumentNode = instrumentNode->NextSibling("instrument")) {
			instrumentList_count++;

			string id = readXmlString( this, instrumentNode, "id", "" );
			string name = readXmlString( this, instrumentNode, "name", "" );
			float volume = readXmlFloat( this, instrumentNode, "volume", 1.0f );
			bool isMuted = readXmlBool( this, instrumentNode, "isMuted", false );
			bool isLocked = readXmlBool( this, instrumentNode, "isLocked", false, false );
			float pan_L = readXmlFloat( this, instrumentNode, "pan_L", 1.0f );
			float pan_R = readXmlFloat( this, instrumentNode, "pan_R", 1.0f );
			bool bFilterActive = readXmlBool( this, instrumentNode, "filterActive", false, false );
			float fFilterCutoff = readXmlFloat( this, instrumentNode, "filterCutoff", 1.0f, false, false );
			float fFilterResonance = readXmlFloat( this, instrumentNode, "filterResonance", 0.0f, false, false );
			float fRandomPitchFactor = readXmlFloat( this, instrumentNode, "randomPitchFactor", 0.0f, false, false );
			float fAttack = LocalFileMng::readXmlFloat( this, instrumentNode, "Attack", 0, false, false );		// Attack
			float fDecay = LocalFileMng::readXmlFloat( this, instrumentNode, "Decay", 0, false, false  );		// Decay
			float fSustain = LocalFileMng::readXmlFloat( this, instrumentNode, "Sustain", 1.0, false, false );	// Sustain
			float fRelease = LocalFileMng::readXmlFloat( this, instrumentNode, "Release", 1000, false, false );	// Release
			float fGain = readXmlFloat( this, instrumentNode, "gain", 1.0f, false, false );

			Instrument *pInstrument = new Instrument( id, name, volume );

			// exclude-notes vector
			TiXmlNode* excludeNode = instrumentNode->FirstChild( "exclude" );
			if ( excludeNode ) {
				TiXmlNode* idNode = 0;
				for( idNode = excludeNode->FirstChild( "id"); idNode; idNode = idNode->NextSibling( "id" )) {
					int id = atoi( idNode->FirstChild()->Value() );
					pInstrument->m_excludeVectId.push_back( id );
				}
			}
			else {
//				warningLog( "Error reading drumkit: exclude node not found" );
			}

			// back compatibility code
			TiXmlNode* filenameNode = instrumentNode->FirstChild( "filename" );
			if ( filenameNode ) {
				//warningLog( "Using back compatibility code. filename node found" );
				string sFilename = LocalFileMng::readXmlString( this, instrumentNode, "filename", "" );
				Sample *pSample = new Sample(0, sFilename );
				InstrumentLayer *pLayer = new InstrumentLayer( pSample );
				pInstrument->setLayer( pLayer, 0 );
			}
			//~ back compatibility code
			else {
				unsigned nLayer = 0;
				for( TiXmlNode* layerNode = instrumentNode->FirstChild( "layer" ); layerNode; layerNode = layerNode->NextSibling( "layer" ) ) {
					if (nLayer >= MAX_LAYERS) {
						errorLog( "[loadDrumkit] nLayer > MAX_LAYERS" );
						continue;
					}
					string sFilename = LocalFileMng::readXmlString( this, layerNode, "filename", "" );
					float fMin = LocalFileMng::readXmlFloat( this, layerNode, "min", 0.0 );
					float fMax = LocalFileMng::readXmlFloat( this, layerNode, "max", 1.0 );
					float fGain = LocalFileMng::readXmlFloat( this, layerNode, "gain", 1.0, false, false );
					float fPitch = LocalFileMng::readXmlFloat( this, layerNode, "pitch", 0.0, false, false );

					Sample *pSample = new Sample(0, sFilename );
					//Sample *pSample = Sample::load( directory + "/" + sFilename );
					//pSample->setFilename( sFilename );
					InstrumentLayer *pLayer = new InstrumentLayer( pSample );
					pLayer->m_fStartVelocity = fMin;
					pLayer->m_fEndVelocity = fMax;
					pLayer->m_fGain = fGain;
					pLayer->m_fPitch = fPitch;
					pInstrument->setLayer( pLayer, nLayer );

					nLayer++;
				}
			}

			pInstrument->m_bFilterActive = bFilterActive;
			pInstrument->m_fCutoff = fFilterCutoff;
			pInstrument->m_fResonance = fFilterResonance;
			pInstrument->m_bIsMuted = isMuted;
			pInstrument->m_bIsLocked = isLocked;
			pInstrument->m_fPan_L = pan_L;
			pInstrument->m_fPan_R = pan_R;
			pInstrument->m_fRandomPitchFactor = fRandomPitchFactor;
			pInstrument->m_sDrumkitName = drumkitInfo->getName();
			pInstrument->m_fGain = fGain;

			pInstrument->m_pADSR = new ADSR( fAttack, fDecay, fSustain, fRelease );
			instrumentList->add( pInstrument );
		}
		// prepare the exclude vector
		for (unsigned nInstr = 0; nInstr < instrumentList->getSize(); nInstr++) {
			Instrument* pInstr = instrumentList->get( nInstr );
			for (unsigned i = 0; i < pInstr->m_excludeVectId.size(); i++) {
				int id = pInstr->m_excludeVectId[ i ];
				Instrument* pExcluded = instrumentList->get( id );
				pInstr->m_excludeVect.push_back( pExcluded );
			}
		}
	}
	else {
		warningLog( "[loadDrumkit] Error reading drumkit: instrumentList node not found" );
	}
	drumkitInfo->setInstrumentList( instrumentList );

	return drumkitInfo;
}



int LocalFileMng::saveDrumkit( DrumkitInfo *info )
{
	infoLog( "[saveDrumkit]" );
	info->dump();	// debug

	string sDrumkitDir = QDir::homeDirPath().append( "/.hydrogen/data/" ).append( info->getName().c_str() ).ascii();

	// check if the directory exists
	QDir dir( QString( sDrumkitDir.c_str() ) );
	if (!dir.exists()) {
		dir.mkdir( QString( sDrumkitDir.c_str() ) );// create the drumkit directory
		//mkdir( sDrumkitDir.c_str(), S_IRWXU );
	}
	else {
		/*
		warningLog( "[saveDrumkit] Cleaning directory " + sDrumkitDir );
		// clear all the old files in the directory
		string clearCmd = "rm -f " + sDrumkitDir + "/*";
		system( clearCmd.c_str() );
		*/
	}


	// create the drumkit.xml file
	string sDrumkitXmlFilename = sDrumkitDir + string( "/drumkit.xml" );

	TiXmlDocument doc( sDrumkitXmlFilename.c_str() );

	TiXmlElement rootNode( "drumkit_info" );

	writeXmlString( &rootNode, "name", info->getName() );	// name
	writeXmlString( &rootNode, "author", info->getAuthor() );	// author
	writeXmlString( &rootNode, "info", info->getInfo() );	// info

	TiXmlElement instrumentListNode( "instrumentList" );		// instrument list
	unsigned nInstrument = info->getInstrumentList()->getSize();
	// INSTRUMENT NODE
	for (unsigned i = 0; i < nInstrument; i++) {
		Instrument *instr = info->getInstrumentList()->get(i);

		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer *pLayer = instr->getLayer( nLayer );
			if (pLayer) {
				Sample *pSample = pLayer->m_pSample;
				string sOrigFilename = pSample->m_sFilename;

				string sDestFilename = sOrigFilename;

				int nPos = sDestFilename.rfind( '/' );
				sDestFilename = sDestFilename.substr( nPos + 1, sDestFilename.size() - nPos - 1 );
				sDestFilename = sDrumkitDir + "/" + sDestFilename;

				fileCopy( sOrigFilename, sDestFilename );
				//string sCmd = "cp \"" + sOrigFilename + "\" \"" + sDestFilename + "\"";
				//infoLog( sCmd );
				//system( sCmd.c_str() );
			}
		}

		TiXmlElement instrumentNode("instrument");

		LocalFileMng::writeXmlString( &instrumentNode, "id", instr->m_sId );
		LocalFileMng::writeXmlString( &instrumentNode, "name", instr->m_sName );
		LocalFileMng::writeXmlString( &instrumentNode, "volume", toString( instr->m_fVolume ) );
		LocalFileMng::writeXmlBool( &instrumentNode, "isMuted", instr->m_bIsMuted );
		LocalFileMng::writeXmlBool( &instrumentNode, "isLocked", instr->m_bIsLocked );
		LocalFileMng::writeXmlString( &instrumentNode, "pan_L", toString( instr->m_fPan_L ) );
		LocalFileMng::writeXmlString( &instrumentNode, "pan_R", toString( instr->m_fPan_R ) );
		LocalFileMng::writeXmlString( &instrumentNode, "randomPitchFactor", toString( instr->m_fRandomPitchFactor ) );
		LocalFileMng::writeXmlString( &instrumentNode, "gain", toString( instr->m_fGain ) );

		LocalFileMng::writeXmlBool( &instrumentNode, "filterActive", instr->m_bFilterActive );
		LocalFileMng::writeXmlString( &instrumentNode, "filterCutoff", toString( instr->m_fCutoff ) );
		LocalFileMng::writeXmlString( &instrumentNode, "filterResonance", toString( instr->m_fResonance ) );

		LocalFileMng::writeXmlString( &instrumentNode, "Attack", toString( instr->m_pADSR->m_fAttack ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Decay", toString( instr->m_pADSR->m_fDecay ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Sustain", toString( instr->m_pADSR->m_fSustain ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Release", toString( instr->m_pADSR->m_fRelease ) );


		// exclude vector
		TiXmlElement excludeNode( "exclude" );
		if (instr->m_excludeVectId.size() != 0) {
			for (unsigned i = 0; i <instr->m_excludeVectId.size(); i++) {
				writeXmlString( &excludeNode, "id" , toString( instr->m_excludeVectId[ i ] ) );
			}
		}
		instrumentNode.InsertEndChild( excludeNode);

		for (unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++) {
			InstrumentLayer *pLayer = instr->getLayer( nLayer );
			if (pLayer == NULL) continue;
			Sample *pSample = pLayer->m_pSample;

			string sFilename = pSample->m_sFilename;

			//if (instr->getDrumkitName() != "") {
				// se e' specificato un drumkit, considero solo il nome del file senza il path
				int nPos = sFilename.rfind("/");
				sFilename = sFilename.substr( nPos + 1, sFilename.length() );
			//}

			TiXmlElement layerNode( "layer" );
			LocalFileMng::writeXmlString( &layerNode, "filename", sFilename );
			LocalFileMng::writeXmlString( &layerNode, "min", toString( pLayer->m_fStartVelocity ) );
			LocalFileMng::writeXmlString( &layerNode, "max", toString( pLayer->m_fEndVelocity ) );
			LocalFileMng::writeXmlString( &layerNode, "gain", toString( pLayer->m_fGain ) );
			LocalFileMng::writeXmlString( &layerNode, "pitch", toString( pLayer->m_fPitch ) );

			instrumentNode.InsertEndChild( layerNode );
		}

		instrumentListNode.InsertEndChild(instrumentNode);
	}

	rootNode.InsertEndChild(instrumentListNode);

	doc.InsertEndChild( rootNode );
	doc.SaveFile();

	return 0; // ok
}



void LocalFileMng::installDrumkit( string filename ) {
	infoLog( "[installDrumkit] drumkit = " + filename );

	string dataDir = QDir::homeDirPath().append( "/.hydrogen/data/" ).ascii();

	// unpack the drumkit
	string cmd = string( "cd " ) + dataDir + string( "; tar xzf \"" ) + filename + string( "\"" );
	infoLog( cmd );
	if ( system( cmd.c_str() ) != -1 ) {
		errorLog( "[installDrumkit] Error executing '" + cmd + "'" );
	}
}



int LocalFileMng::uninstallDrumkit( string drumkitName ) {
	infoLog( "uninstall drumkit " + drumkitName );

	// verificare che non sia un drumkit di sistema

	return 0;	// OK
}



Song* LocalFileMng::loadSong(string filename) {

	Song *song = NULL;

	SongReader reader;
	song = reader.readSong(filename);

	return song;
}



void LocalFileMng::saveSong(Song *song, string filename) {
	SongWriter writer;

	writer.writeSong(song, filename);
}



string LocalFileMng::readXmlString( Object* obj, TiXmlNode* parent, string nodeName, string defaultValue, bool bCanBeEmpty )
{
	TiXmlNode* node;
	if ( ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			return node->FirstChild()->Value();
		}
		else {
			if (!bCanBeEmpty) {
				obj->warningLog( "[readXmlString] Using default value in " + nodeName );
			}
			return defaultValue;
		}
	}
	else {
		obj->warningLog( "[readXmlString] '" + nodeName + "' node not found" );
		return defaultValue;
	}
}



float LocalFileMng::readXmlFloat( Object* obj, TiXmlNode* parent, string nodeName, float defaultValue, bool bCanBeEmpty, bool bShouldExists )
{
	TiXmlNode* node;
	if ( ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			//float res = atof( node->FirstChild()->Value() );
			float res = stringToFloat( node->FirstChild()->Value() );
			return res;
		}
		else {
			if (!bCanBeEmpty) {
				obj->warningLog( "[readXmlFloat] Using default value in " + nodeName );
			}
			return defaultValue;
		}
	}
	else {
		if ( bShouldExists ) {
			obj->warningLog( "[readXmlFloat] '" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}



int LocalFileMng::readXmlInt( Object* obj, TiXmlNode* parent, string nodeName, int defaultValue, bool bCanBeEmpty, bool bShouldExists)
{
	TiXmlNode* node;
	if ( ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			return atoi( node->FirstChild()->Value() );
		}
		else {
			if (!bCanBeEmpty) {
				obj->warningLog( "[readXmlInt] Using default value in " + nodeName );
			}
			return defaultValue;
		}
	}
	else {
		if (bShouldExists)  {
			obj->warningLog( "[readXmlInt] '" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}



bool LocalFileMng::readXmlBool( Object* obj, TiXmlNode* parent, string nodeName, bool defaultValue, bool bShouldExists )
{
	TiXmlNode* node;
	if ( ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			if ( string( node->FirstChild()->Value() ) == "true" ) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			obj->warningLog( "[readXmlBool] Using default value in " + nodeName );
			return defaultValue;
		}
	}
	else {
		if (bShouldExists) {
			obj->warningLog( "[readXmlBool] '" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}



void LocalFileMng::writeXmlString( TiXmlNode *parent, string name, string text ) {
	TiXmlElement versionNode( name );
	TiXmlText versionText( text );
	versionNode.InsertEndChild( versionText );
	parent->InsertEndChild( versionNode );
}



void LocalFileMng::writeXmlBool( TiXmlNode *parent, string name, bool value ) {
	if (value) {
		writeXmlString( parent, name, string("true") );
	}
	else {
		writeXmlString( parent, name, string("false") );
	}
}





//-----------------------------------------------------------------------------
//	Implementation of SongReader class
//-----------------------------------------------------------------------------


SongReader::SongReader() : Object( "SongReader" )
{
//	infoLog("init");
}





SongReader::~SongReader() {
//	infoLog("destroy");
}



/// Read a song.
/// return NULL = error reading song file
Song* SongReader::readSong(string filename)
{
	infoLog("[readSong] " + filename);
	Song* song = NULL;

	std::ifstream verify(filename.c_str() , std::ios::in | std::ios::binary);	// song file exists??
	if (verify == NULL) {
		errorLog("[readSong] Song file " + filename + " not found.");
		return NULL;
	}


	TiXmlDocument doc(filename.c_str());
	doc.LoadFile();

	TiXmlNode* songNode;	// root element
	if ( !( songNode = doc.FirstChild("song") ) ) {
		errorLog("[readSong] Error reading song: song node not found");
		return NULL;
	}


	m_sSongVersion = LocalFileMng::readXmlString( this, songNode, "version", "Unknown version" );
	if ( m_sSongVersion != VERSION ) {
		warningLog( "[readSong] Trying to load a song created with a different version of hydrogen.");
		warningLog( "[readSong] Song [" + filename + "] saved with version " + m_sSongVersion );
	}

	float fBpm = LocalFileMng::readXmlFloat( this, songNode, "bpm", 120 );
	float fVolume = LocalFileMng::readXmlFloat( this, songNode, "volume", 0.5 );
	float fMetronomeVolume = LocalFileMng::readXmlFloat( this, songNode, "metronomeVolume", 0.5 );
	string sName = LocalFileMng::readXmlString( this, songNode, "name", "Untitled Song" );
	string sAuthor = LocalFileMng::readXmlString( this, songNode, "author", "Unknown Author" );
	string sNotes = LocalFileMng::readXmlString( this, songNode, "notes", "..." );
	bool bLoopEnabled = LocalFileMng::readXmlBool( this, songNode, "loopEnabled", false );

	Song::SongMode nMode = Song::PATTERN_MODE;	// Mode (song/pattern)
	string sMode = LocalFileMng::readXmlString( this, songNode, "mode", "pattern" );
	if ( sMode == "song" ) {
		nMode = Song::SONG_MODE;
	}

	float fHumanizeTimeValue = LocalFileMng::readXmlFloat( this, songNode, "humanize_time", 0.0 );
	float fHumanizeVelocityValue = LocalFileMng::readXmlFloat( this, songNode, "humanize_velocity", 0.0 );
	float fSwingFactor = LocalFileMng::readXmlFloat( this, songNode, "swing_factor", 0.0 );

	song = new Song( sName, sAuthor, fBpm, fVolume );
	song->setMetronomeVolume( fMetronomeVolume );
	song->setNotes( sNotes );
	song->setLoopEnabled( bLoopEnabled );
	song->setMode( nMode );
	song->setHumanizeTimeValue( fHumanizeTimeValue );
	song->setHumanizeVelocityValue( fHumanizeVelocityValue );
	song->setSwingFactor( fSwingFactor );
	song->m_bDelayFXEnabled = LocalFileMng::readXmlBool( this, songNode, "delayFXEnabled", false, false );
	song->m_fDelayFXWetLevel = LocalFileMng::readXmlFloat( this, songNode, "delayFXWetLevel", 1.0, false, false );
	song->m_fDelayFXFeedback= LocalFileMng::readXmlFloat( this, songNode, "delayFXFeedback", 0.4, false, false );
	song->m_nDelayFXTime = LocalFileMng::readXmlInt( this, songNode, "delayFXTime", MAX_NOTES / 4, false, false );


	//  Instrument List
	LocalFileMng localFileMng;
	InstrumentList *instrumentList = new InstrumentList();

	TiXmlNode* instrumentListNode;
	if ( (instrumentListNode = songNode->FirstChild("instrumentList") ) ) {
		// INSTRUMENT NODE
		int instrumentList_count = 0;
		TiXmlNode* instrumentNode = 0;
		for( instrumentNode = instrumentListNode->FirstChild("instrument"); instrumentNode; instrumentNode = instrumentNode->NextSibling("instrument")) {
			instrumentList_count++;

			string sId = LocalFileMng::readXmlString( this, instrumentNode, "id", "" );			// instrument id
			string sDrumkit = LocalFileMng::readXmlString( this, instrumentNode, "drumkit", "" );	// drumkit
			string sName = LocalFileMng::readXmlString( this, instrumentNode, "name", "" );		// name
			float fVolume = LocalFileMng::readXmlFloat( this, instrumentNode, "volume", 1.0 );	// volume
			bool bIsMuted = LocalFileMng::readXmlBool( this, instrumentNode, "isMuted", false );	// is muted
			bool bIsLocked = LocalFileMng::readXmlBool( this, instrumentNode, "isLocked", false, false );	// is locked
			float fPan_L = LocalFileMng::readXmlFloat( this, instrumentNode, "pan_L", 1.0 );	// pan L
			float fPan_R = LocalFileMng::readXmlFloat( this, instrumentNode, "pan_R", 1.0 );	// pan R
			float fFX1Level = LocalFileMng::readXmlFloat( this, instrumentNode, "FX1Level", 0.0 );	// FX level
			float fFX2Level = LocalFileMng::readXmlFloat( this, instrumentNode, "FX2Level", 0.0 );	// FX level
			float fFX3Level = LocalFileMng::readXmlFloat( this, instrumentNode, "FX3Level", 0.0 );	// FX level
			float fFX4Level = LocalFileMng::readXmlFloat( this, instrumentNode, "FX4Level", 0.0 );	// FX level
			float fGain = LocalFileMng::readXmlFloat( this, instrumentNode, "gain", 1.0, false, false );	// instrument gain

			int fAttack = LocalFileMng::readXmlInt( this, instrumentNode, "Attack", 0, false, false );		// Attack
			int fDecay = LocalFileMng::readXmlInt( this, instrumentNode, "Decay", 0, false, false );		// Decay
			float fSustain = LocalFileMng::readXmlFloat( this, instrumentNode, "Sustain", 1.0, false, false );	// Sustain
			int fRelease = LocalFileMng::readXmlInt( this, instrumentNode, "Release", 1000, false, false );	// Release

			float fRandomPitchFactor = LocalFileMng::readXmlFloat( this, instrumentNode, "randomPitchFactor", 0.0f, false, false );

			bool bFilterActive = LocalFileMng::readXmlBool( this, instrumentNode, "filterActive", false, false );
			float fFilterCutoff = LocalFileMng::readXmlFloat( this, instrumentNode, "filterCutoff", 1.0f, false, false );
			float fFilterResonance = LocalFileMng::readXmlFloat( this, instrumentNode, "filterResonance", 0.0f, false, false );


			// create a new instrument
			Instrument *pInstrument = new Instrument( sId, sName, fVolume, bIsMuted, bIsLocked, fPan_L, fPan_R, sDrumkit );
			pInstrument->setFXLevel( 0, fFX1Level );
			pInstrument->setFXLevel( 1, fFX2Level );
			pInstrument->setFXLevel( 2, fFX3Level );
			pInstrument->setFXLevel( 3, fFX4Level );
			pInstrument->m_fRandomPitchFactor = fRandomPitchFactor;
			pInstrument->m_pADSR = new ADSR( fAttack, fDecay, fSustain, fRelease );
			pInstrument->m_bFilterActive = bFilterActive;
			pInstrument->m_fCutoff = fFilterCutoff;
			pInstrument->m_fResonance = fFilterResonance;
			pInstrument->m_fGain = fGain;

			string drumkitPath = "";
			if ( ( sDrumkit != "" ) && (sDrumkit != "-" ) ) {
//				drumkitPath = localFileMng.getDrumkitDirectory( sDrumkit ) + sDrumkit + "/";
				drumkitPath = localFileMng.getDrumkitDirectory( sDrumkit ) + sDrumkit;
			}

			// back compatibility code ( song version <= 0.9.0 )
			TiXmlNode* filenameNode = instrumentNode->FirstChild( "filename" );
			if ( filenameNode ) {
				warningLog( "[readSong] Using back compatibility code. filename node found" );
				string sFilename = LocalFileMng::readXmlString( this, instrumentNode, "filename", "" );

				if (drumkitPath != "") {
					sFilename = drumkitPath + "/" + sFilename;
				}
				Sample *pSample = Sample::load( sFilename );
				if (pSample == NULL) {
					// nel passaggio tra 0.8.2 e 0.9.0 il drumkit di default e' cambiato.
					// Se fallisce provo a caricare il corrispettivo file in formato flac
//					warningLog( "[readSong] Error loading sample: " + sFilename + " not found. Trying to load a flac..." );
					sFilename = sFilename.substr( 0, sFilename.length() - 4 );
					sFilename += ".flac";
					pSample = Sample::load( sFilename );
/*					if ( pSample ) {
						infoLog( "[readSong] Found FLAC file!" );
					}*/
				}
				if (pSample == NULL) {
					errorLog( "[readSong] Error loading sample: " + sFilename + " not found" );
					pInstrument->m_bIsMuted = true;
				}
				InstrumentLayer *pLayer = new InstrumentLayer( pSample );
				pInstrument->setLayer( pLayer, 0 );
			}
			//~ back compatibility code
			else {
				unsigned nLayer = 0;
				for( TiXmlNode* layerNode = instrumentNode->FirstChild( "layer" ); layerNode; layerNode = layerNode->NextSibling( "layer" ) ) {
					if (nLayer >= MAX_LAYERS) {
						errorLog( "[readSong] nLayer > MAX_LAYERS" );
						continue;
					}
					string sFilename = LocalFileMng::readXmlString( this, layerNode, "filename", "" );
					float fMin = LocalFileMng::readXmlFloat( this, layerNode, "min", 0.0 );
					float fMax = LocalFileMng::readXmlFloat( this, layerNode, "max", 1.0 );
					float fGain = LocalFileMng::readXmlFloat( this, layerNode, "gain", 1.0 );
					float fPitch = LocalFileMng::readXmlFloat( this, layerNode, "pitch", 0.0, false, false );

					if (drumkitPath != "") {
						sFilename = drumkitPath + "/" + sFilename;
					}
					Sample *pSample = Sample::load( sFilename );
					if (pSample == NULL) {
						errorLog( "[readSong] Error loading sample: " + sFilename + " not found" );
						pInstrument->m_bIsMuted = true;
					}
					InstrumentLayer *pLayer = new InstrumentLayer( pSample );
					pLayer->m_fStartVelocity = fMin;
					pLayer->m_fEndVelocity = fMax;
					pLayer->m_fGain = fGain;
					pLayer->m_fPitch = fPitch;
					pInstrument->setLayer( pLayer, nLayer );
					nLayer++;
				}
			}

			// exclude-notes vector
			TiXmlNode* excludeNode = instrumentNode->FirstChild( "exclude" );
			if ( excludeNode ) {
				TiXmlNode* idNode = 0;
				for( idNode = excludeNode->FirstChild( "id"); idNode; idNode = idNode->NextSibling( "id" )) {
					int id = atoi( idNode->FirstChild()->Value() );
					pInstrument->m_excludeVectId.push_back( id );
				}
			}
			else {
				warningLog( "[readSong] Error loading song: exclude node not found" );
			}

			instrumentList->add( pInstrument );
		}

		// prepare the exclude vector
		for (unsigned nInstr = 0; nInstr < instrumentList->getSize(); nInstr++) {
			Instrument* pInstr = instrumentList->get( nInstr );
			if (pInstr) {
				for (unsigned i = 0; i < pInstr->m_excludeVectId.size(); i++) {
					int id = pInstr->m_excludeVectId[ i ];
					Instrument* pExcluded = instrumentList->get( id );
					pInstr->m_excludeVect.push_back( pExcluded );
				}
			}
			else {
				errorLog( "[readSong] pInstr == NULL" );
			}
		}


		song->setInstrumentList( instrumentList );
		if ( instrumentList_count != MAX_INSTRUMENTS ) {
			warningLog( "[readSong] Instrument number != MAX_INSTRUMENTS. n=" + toString( instrumentList_count ) + ">" + toString(MAX_INSTRUMENTS) );

			// create the missing instruments with empty sample
			unsigned nInstrMissing = MAX_INSTRUMENTS - instrumentList_count;
			for (unsigned i = 0; i < nInstrMissing; i++) {
				int instrId = instrumentList_count + i;
//				infoLog( "[readSong] Creating empty instrument");

				Instrument *pInstrument = new Instrument( "id", "", 0.8f );

//				Instrument *instr = Instrument::load( string(DataPath::getDataPath()) + "/emptySample.wav" );
				char idStr[10];
				sprintf(idStr,"%d", instrId);
				pInstrument->m_sId = idStr;
				pInstrument->m_fVolume = 0.1f;
				pInstrument->m_sName = "Empty";
				pInstrument->m_bIsMuted = false;
				pInstrument->m_bIsLocked = false;
				pInstrument->m_fPan_L = 1.0;
				pInstrument->m_fPan_R = 1.0;

				string sFilename = string(DataPath::getDataPath()) + "/emptySample.wav";
				Sample *pSample = Sample::load( sFilename );
				if (pSample == NULL) {
					errorLog( "[readSong] Error loading sample: " + sFilename + " not found" );
				}
				InstrumentLayer *pLayer = new InstrumentLayer( pSample );
				pInstrument->setLayer( pLayer, 0 );

				instrumentList->add( pInstrument );
			}
		}
	}
	else {
		errorLog("[readSong] Error reading song: instrumentList node not found");
		delete song;
		return NULL;
	}





	// Pattern list
	TiXmlNode* patterns = songNode->FirstChild("patternList");

	PatternList *patternList = new PatternList();
	int pattern_count = 0;
	TiXmlNode* patternNode = 0;
	for (patternNode = patterns->FirstChild("pattern"); patternNode; patternNode = patternNode->NextSibling( "pattern" )) {
		pattern_count++;
		Pattern *pat = getPattern(patternNode, instrumentList);
		if (pat) {
			patternList->add(pat);
		}
		else {
			errorLog( "[readSong] Error loading pattern" );
			delete patternList;
			delete song;
			return NULL;
		}
	}
	song->setPatternList(patternList);


	// Pattern sequence
	TiXmlNode* patternSequenceNode = songNode->FirstChild("patternSequence");

	vector<PatternList*>* pPatternGroupVector = new vector<PatternList*>;

	// back-compatibility code..
	for (TiXmlNode* pPatternIDNode = patternSequenceNode->FirstChild("patternID"); pPatternIDNode; pPatternIDNode = pPatternIDNode->NextSibling("patternID")) {
		warningLog( "[readSong] Using old patternSequence code for back compatibility" );
		PatternList *patternSequence = new PatternList();
		string patId = pPatternIDNode->FirstChild()->Value();

		Pattern *pat = NULL;
		for (unsigned i = 0; i < patternList->getSize(); i++) {
			Pattern *tmp = patternList->get(i);
			if (tmp) {
				if (tmp->m_sName == patId) {
					pat = tmp;
					break;
				}
			}
		}
		patternSequence->add( pat );

		pPatternGroupVector->push_back( patternSequence );
	}

	for (TiXmlNode* groupNode = patternSequenceNode->FirstChild("group"); groupNode; groupNode = groupNode->NextSibling("group")) {
		PatternList *patternSequence = new PatternList();
		for (TiXmlNode* patternId = groupNode->FirstChild("patternID"); patternId; patternId = patternId->NextSibling("patternID")) {
			string patId = patternId->FirstChild()->Value();

			Pattern *pat = NULL;
			for (unsigned i = 0; i < patternList->getSize(); i++) {
				Pattern *tmp = patternList->get(i);
				if (tmp) {
					if (tmp->m_sName == patId) {
						pat = tmp;
						break;
					}
				}
			}
			patternSequence->add( pat );
		}
		pPatternGroupVector->push_back( patternSequence );
	}

	song->setPatternGroupVector( pPatternGroupVector );


	// LADSPA FX
	TiXmlNode* ladspaNode = songNode->FirstChild( "ladspa" );
	if (ladspaNode) {
		int nFX = 0;
		TiXmlNode* fxNode;
		for (fxNode = ladspaNode->FirstChild("fx"); fxNode; fxNode = fxNode->NextSibling("fx")) {
			string sName = LocalFileMng::readXmlString( this, fxNode, "name", "" );
			string sFilename = LocalFileMng::readXmlString( this, fxNode, "filename", "" );
			bool bEnabled = LocalFileMng::readXmlBool( this, fxNode, "enabled", false );
			float fVolume = LocalFileMng::readXmlFloat( this, fxNode, "volume", 1.0 );

			if (sName != "no plugin" ) {
				// FIXME: il caricamento va fatto fare all'engine, solo lui sa il samplerate esatto
#ifdef LADSPA_SUPPORT
				LadspaFX* pFX = LadspaFX::load( sFilename, sName, 44100 );
				song->setLadspaFX( nFX, pFX );
				if (pFX) {
					pFX->setEnabled( bEnabled );
					pFX->setVolume( fVolume );
					TiXmlNode* inputControlNode;
					for ( inputControlNode = fxNode->FirstChild("inputControlPort"); inputControlNode; inputControlNode = inputControlNode->NextSibling("inputControlPort")) {
						string sName = LocalFileMng::readXmlString( this, inputControlNode, "name", "" );
						float fValue = LocalFileMng::readXmlFloat( this, inputControlNode, "value", 0.0 );

						for (unsigned nPort = 0; nPort < pFX->inputControlPorts.size(); nPort++) {
							LadspaControlPort *port = pFX->inputControlPorts[ nPort ];
							if ( string(port->sName) == sName) {
								port->fControlValue = fValue;
							}
						}
					}

					TiXmlNode* outputControlNode;
					for ( outputControlNode = fxNode->FirstChild("outputControlPort"); outputControlNode; outputControlNode = outputControlNode->NextSibling("outputControlPort")) {
					}
				}
#endif
			}
			nFX++;
		}
	}
	else {
		warningLog( "[readSong()] ladspa node not found" );
	}


	song->m_bIsModified = false;
	song->setFilename(filename);

	return song;
}



Pattern* SongReader::getPattern(TiXmlNode* pattern, InstrumentList* instrList){
	Pattern *pat = NULL;

	string sName = "";	// name
	sName = LocalFileMng::readXmlString( this, pattern, "name", sName );

	int nSize = -1;
	nSize = LocalFileMng::readXmlInt( this, pattern, "size", nSize, false, false );

	// back compatibility code...what a mess...
	bool bConvertFrom080 = false;
	if ( nSize == -1 ) {
		// size is missing..
		nSize = MAX_NOTES;

		if (
			( m_sSongVersion == "0.8.0" ) ||
			( m_sSongVersion == "0.8.0beta1" ) ||
			( m_sSongVersion == "0.8.0beta2" ) ||
			( m_sSongVersion == "0.8.0beta3" )
			) {
			bConvertFrom080 = true;
		}
		warningLog( "[getPattern] pattern size not found! Using default: " + toString( MAX_NOTES ) );
	}



	pat = new Pattern( sName, nSize );

	SequenceList *sequenceList = new SequenceList();

	TiXmlNode* sequenceListNode = pattern->FirstChild("sequenceList");

	int sequence_count = 0;
	TiXmlNode* sequenceNode = 0;
	for (sequenceNode = sequenceListNode->FirstChild("sequence"); sequenceNode; sequenceNode = sequenceNode->NextSibling("sequence")) {
		sequence_count++;
		Sequence *seq = getSequence(sequenceNode, instrList, bConvertFrom080);

		sequenceList->add(seq);
	}
	pat->m_pSequenceList = sequenceList;
	if ( sequence_count != MAX_INSTRUMENTS ) {
		warningLog( "[getPattern] sequence number != MAX_INSTRUMENTS" );

		unsigned nMissingSequences = MAX_INSTRUMENTS - sequence_count;
		for (unsigned i = 0; i < nMissingSequences; i++) {
//			infoLog( "[getPattern] Creating empty Sequence");
			Sequence *seq = new Sequence();
			sequenceList->add(seq);
		}
	}
	return pat;
}



Sequence* SongReader::getSequence(TiXmlNode* sequence, InstrumentList* instrList, bool bConvertFrom080 )
{
	Sequence* seq = new Sequence();

	TiXmlNode* noteListNode = sequence->FirstChild( "noteList" );

	TiXmlNode* noteNode = 0;
	for (noteNode = noteListNode->FirstChild("note"); noteNode; noteNode = noteNode->NextSibling("note")) {
		Note *note = getNote(noteNode, instrList);
		if ( bConvertFrom080 ) {
			note->m_nPosition = note->m_nPosition * 3;
		}

		seq->m_noteList[ note->m_nPosition ] = note;
	}

	return seq;
}



Note* SongReader::getNote(TiXmlNode* noteNode, InstrumentList *instrList){
	Note* note = NULL;

	unsigned nPosition = LocalFileMng::readXmlInt( this, noteNode, "position", 0 );
	float fVelocity = LocalFileMng::readXmlFloat( this, noteNode, "velocity", 0.8f );
	float fPan_L = LocalFileMng::readXmlFloat( this, noteNode, "pan_L", 1.0 );
	float fPan_R = LocalFileMng::readXmlFloat( this, noteNode, "pan_R", 1.0 );
	int nLength = LocalFileMng::readXmlInt( this, noteNode, "length", -1, true );
	float nPitch = LocalFileMng::readXmlFloat( this, noteNode, "pitch", 0.0, false, false );

	TiXmlNode* instrNode = noteNode->FirstChild("instrument");
	string instrId = instrNode->FirstChild()->Value();

	Instrument *instrRef = NULL;
	// search instrument by ref
	for (unsigned i = 0; i < instrList->getSize(); i++) {
		Instrument *instr = instrList->get(i);
		if (instrId == instr->m_sId) {
			instrRef = instr;
			break;
		}
	}
	if (instrRef == NULL) {
		errorLog("[getNote] instrRef NULL");
	}

	note = new Note( instrRef, nPosition, fVelocity, fPan_L, fPan_R, nLength, nPitch);

	return note;
}






//-----------------------------------------------------------------------------
//	Implementation of SongWriter class
//-----------------------------------------------------------------------------


SongWriter::SongWriter() : Object( "SongWriter" )
{
//	infoLog("init");
}



SongWriter::~SongWriter() {
//	infoLog("destroy");
}



void SongWriter::writeSong(Song *song, string filename) {
	infoLog( "Saving song " + filename );

	// FIXME: verificare se e' possibile scrivere il file
	// FIXME: verificare che il file non sia gia' esistente
	// FIXME: effettuare copia di backup per il file gia' esistente

	TiXmlDocument doc(filename);

	TiXmlElement songNode("song");

	LocalFileMng::writeXmlString( &songNode, "version", string(VERSION) );
	LocalFileMng::writeXmlString( &songNode, "bpm", toString( song->m_fBPM ) );
	LocalFileMng::writeXmlString( &songNode, "volume", toString( song->getVolume() ) );
	LocalFileMng::writeXmlString( &songNode, "metronomeVolume", toString( song->getMetronomeVolume() ) );
	LocalFileMng::writeXmlString( &songNode, "name", song->m_sName );
	LocalFileMng::writeXmlString( &songNode, "author", song->m_sAuthor );
	LocalFileMng::writeXmlString( &songNode, "notes", song->getNotes() );
	LocalFileMng::writeXmlBool( &songNode, "loopEnabled", song->isLoopEnabled() );

	if (song->getMode() == Song::SONG_MODE ) {
		LocalFileMng::writeXmlString( &songNode, "mode", string("song") );
	}
	else {
		LocalFileMng::writeXmlString( &songNode, "mode", string("pattern") );
	}

	LocalFileMng::writeXmlString( &songNode, "humanize_time", toString( song->getHumanizeTimeValue() ) );
	LocalFileMng::writeXmlString( &songNode, "humanize_velocity", toString( song->getHumanizeVelocityValue() ) );
	LocalFileMng::writeXmlString( &songNode, "swing_factor", toString( song->getSwingFactor() ) );

	LocalFileMng::writeXmlBool( &songNode, "delayFXEnabled", song->m_bDelayFXEnabled );
	LocalFileMng::writeXmlString( &songNode, "delayFXWetLevel", toString( song->m_fDelayFXWetLevel ) );
	LocalFileMng::writeXmlString( &songNode, "delayFXFeedback", toString( song->m_fDelayFXFeedback ) );
	LocalFileMng::writeXmlString( &songNode, "delayFXTime", toString( song->m_nDelayFXTime ) );

	// instrument list
	TiXmlElement instrumentListNode("instrumentList");
	unsigned nInstrument = song->getInstrumentList()->getSize();

	// INSTRUMENT NODE
	for (unsigned i = 0; i < nInstrument; i++) {
		Instrument *instr = song->getInstrumentList()->get(i);
		assert( instr );

		TiXmlElement instrumentNode("instrument");

		LocalFileMng::writeXmlString( &instrumentNode, "id", instr->m_sId );
		LocalFileMng::writeXmlString( &instrumentNode, "drumkit", instr->m_sDrumkitName );
		LocalFileMng::writeXmlString( &instrumentNode, "name", instr->m_sName );
		LocalFileMng::writeXmlString( &instrumentNode, "volume", toString( instr->m_fVolume ) );
		LocalFileMng::writeXmlBool( &instrumentNode, "isMuted", instr->m_bIsMuted );
		LocalFileMng::writeXmlBool( &instrumentNode, "isLocked", instr->m_bIsLocked );
		LocalFileMng::writeXmlString( &instrumentNode, "pan_L", toString( instr->m_fPan_L ) );
		LocalFileMng::writeXmlString( &instrumentNode, "pan_R", toString( instr->m_fPan_R ) );
		LocalFileMng::writeXmlString( &instrumentNode, "gain", toString( instr->m_fGain ) );

		LocalFileMng::writeXmlString( &instrumentNode, "FX1Level", toString( instr->getFXLevel(0) ) );
		LocalFileMng::writeXmlString( &instrumentNode, "FX2Level", toString( instr->getFXLevel(1) ) );
		LocalFileMng::writeXmlString( &instrumentNode, "FX3Level", toString( instr->getFXLevel(2) ) );
		LocalFileMng::writeXmlString( &instrumentNode, "FX4Level", toString( instr->getFXLevel(3) ) );

		assert( instr->m_pADSR );
		LocalFileMng::writeXmlString( &instrumentNode, "Attack", toString( instr->m_pADSR->m_fAttack ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Decay", toString( instr->m_pADSR->m_fDecay ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Sustain", toString( instr->m_pADSR->m_fSustain ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Release", toString( instr->m_pADSR->m_fRelease ) );

		LocalFileMng::writeXmlString( &instrumentNode, "randomPitchFactor", toString( instr->m_fRandomPitchFactor ) );

		TiXmlElement excludeNode( "exclude" );
		if (instr->m_excludeVectId.size() != 0) {
			for (unsigned i = 0; i <instr->m_excludeVectId.size(); i++) {
				LocalFileMng::writeXmlString( &excludeNode, "id" , toString( instr->m_excludeVectId[ i ] ) );
			}
		}
		instrumentNode.InsertEndChild( excludeNode);

		for (unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++) {
			InstrumentLayer *pLayer = instr->getLayer( nLayer );
			if (pLayer == NULL) continue;
			Sample *pSample = pLayer->m_pSample;
			if (pSample == NULL) continue;

			string sFilename = pSample->m_sFilename;

			if (instr->m_sDrumkitName != "") {
				// se e' specificato un drumkit, considero solo il nome del file senza il path
				int nPos = sFilename.rfind("/");
				sFilename = sFilename.substr( nPos + 1, sFilename.length() );
			}

			TiXmlElement layerNode( "layer" );
			LocalFileMng::writeXmlString( &layerNode, "filename", sFilename );
			LocalFileMng::writeXmlString( &layerNode, "min", toString( pLayer->m_fStartVelocity ) );
			LocalFileMng::writeXmlString( &layerNode, "max", toString( pLayer->m_fEndVelocity ) );
			LocalFileMng::writeXmlString( &layerNode, "gain", toString( pLayer->m_fGain ) );
			LocalFileMng::writeXmlString( &layerNode, "pitch", toString( pLayer->m_fPitch ) );

			instrumentNode.InsertEndChild( layerNode );
		}

		instrumentListNode.InsertEndChild(instrumentNode);
	}
	songNode.InsertEndChild(instrumentListNode);


	// pattern list
	TiXmlElement patternListNode("patternList");

	unsigned nPatterns = song->getPatternList()->getSize();
	for (unsigned i = 0; i < nPatterns; i++) {
		Pattern *pat = song->getPatternList()->get(i);

		// pattern
		TiXmlElement patternNode("pattern");
		LocalFileMng::writeXmlString( &patternNode, "name", pat->m_sName );
		LocalFileMng::writeXmlString( &patternNode, "size", toString( pat->m_nSize ) );


		// sequence list
		TiXmlElement sequenceListNode("sequenceList");

		unsigned nSequences = pat->m_pSequenceList->getSize();
		for (unsigned j = 0; j < nSequences; j++) {
			Sequence *seq = pat->m_pSequenceList->get(j);

			// Sequence
			TiXmlElement sequenceNode("sequence");

			// Note List
			TiXmlElement noteListNode("noteList");

//			unsigned nNotes = MAX_NOTES;
			unsigned nNotes = pat->m_nSize;
			for (unsigned y = 0; y < nNotes; y++) {
				Note *note = seq->m_noteList[ y ];
				if (note != NULL) {
					// note
					TiXmlElement noteNode("note");
					LocalFileMng::writeXmlString( &noteNode, "position", toString( note->m_nPosition ) );
					LocalFileMng::writeXmlString( &noteNode, "velocity", toString( note->m_fVelocity ) );
					LocalFileMng::writeXmlString( &noteNode, "pan_L", toString( note->m_fPan_L ) );
					LocalFileMng::writeXmlString( &noteNode, "pan_R", toString( note->m_fPan_R ) );
					LocalFileMng::writeXmlString( &noteNode, "pitch", toString( note->m_fPitch ) );
					LocalFileMng::writeXmlString( &noteNode, "length", toString( note->m_nLength ) );
					LocalFileMng::writeXmlString( &noteNode, "instrument", note->getInstrument()->m_sId );
					noteListNode.InsertEndChild(noteNode);
				}
			}
			sequenceNode.InsertEndChild(noteListNode);
			sequenceListNode.InsertEndChild(sequenceNode);
		}
		patternNode.InsertEndChild(sequenceListNode);
		patternListNode.InsertEndChild(patternNode);
	}
	songNode.InsertEndChild(patternListNode);


	// pattern sequence
	TiXmlElement patternSequenceNode( "patternSequence" );

	unsigned nPatternGroups = (song->getPatternGroupVector())->size();
	for ( unsigned i = 0; i < nPatternGroups; i++ ) {
		TiXmlElement groupNode( "group" );

		PatternList *pList = (*song->getPatternGroupVector())[i];
		for (unsigned j = 0; j < pList->getSize(); j++) {
			Pattern *pPattern = pList->get(j);
			LocalFileMng::writeXmlString( &groupNode, "patternID", pPattern->m_sName );
		}
		patternSequenceNode.InsertEndChild(groupNode);
	}

	songNode.InsertEndChild( patternSequenceNode );


	// LADSPA FX
	TiXmlElement ladspaFxNode( "ladspa" );

	for (unsigned nFX = 0; nFX < MAX_FX; nFX++) {
		TiXmlElement fxNode( "fx" );

#ifdef LADSPA_SUPPORT
		LadspaFX *pFX = song->getLadspaFX(nFX);
		if ( pFX ) {
			LocalFileMng::writeXmlString( &fxNode, "name", pFX->getPluginLabel() );
			LocalFileMng::writeXmlString( &fxNode, "filename", pFX->getLibraryPath() );
			LocalFileMng::writeXmlBool( &fxNode, "enabled", pFX->isEnabled() );
			LocalFileMng::writeXmlString( &fxNode, "volume", toString( pFX->getVolume() ) );
			for (unsigned nControl = 0; nControl < pFX->inputControlPorts.size(); nControl++) {
				LadspaControlPort *pControlPort = pFX->inputControlPorts[ nControl ];
				TiXmlElement controlPortNode( "inputControlPort" );
				LocalFileMng::writeXmlString( &controlPortNode, "name", pControlPort->sName );
				LocalFileMng::writeXmlString( &controlPortNode, "value", toString( pControlPort->fControlValue ) );
				fxNode.InsertEndChild( controlPortNode );
			}
			for (unsigned nControl = 0; nControl < pFX->outputControlPorts.size(); nControl++) {
				LadspaControlPort *pControlPort = pFX->inputControlPorts[ nControl ];
				TiXmlElement controlPortNode( "outputControlPort" );
				LocalFileMng::writeXmlString( &controlPortNode, "name", pControlPort->sName );
				LocalFileMng::writeXmlString( &controlPortNode, "value", toString( pControlPort->fControlValue ) );
				fxNode.InsertEndChild( controlPortNode );
			}
		}
#else
		if ( false ) {
		}
#endif
		else {
			LocalFileMng::writeXmlString( &fxNode, "name", string("no plugin") );
			LocalFileMng::writeXmlString( &fxNode, "filename", string("-") );
			LocalFileMng::writeXmlBool( &fxNode, "enabled", false );
			LocalFileMng::writeXmlString( &fxNode, "volume", toString( 0.0 ) );
		}
		ladspaFxNode.InsertEndChild( fxNode );
	}

	songNode.InsertEndChild( ladspaFxNode );




	doc.InsertEndChild(songNode);
        doc.SaveFile();

	song->m_bIsModified = false;
	song->setFilename(filename);
}


