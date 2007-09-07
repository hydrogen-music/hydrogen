/*
 * Hydrogen
 * Copyright(c) 2002-2007 by Alex >Comix< Cominu [comix@users.sourceforge.net]
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

#include <QDir>
#include <QApplication>
#include <cassert>

#include "xml/tinyxml.h"
#include <hydrogen/data_path.h>
#include <hydrogen/Song.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/Sample.h>
#include <hydrogen/adsr.h>
#include <hydrogen/note.h>
#include <hydrogen/fx/Effects.h>

#include <hydrogen/instrument.h>
#include <hydrogen/Pattern.h>
#include <hydrogen/SoundLibrary.h>
#include <hydrogen/H2Exception.h>



namespace H2Core {

LocalFileMng::LocalFileMng()
 : Object( "LocalFileMng" )
{
//	infoLog("INIT");
}



LocalFileMng::~LocalFileMng()
{
//	infoLog("DESTROY");
}



void LocalFileMng::fileCopy( const string& sOrigFilename, const string& sDestFilename )
{
	INFOLOG( sOrigFilename + " --> " + sDestFilename );

	if ( sOrigFilename == sDestFilename ) {
		return;
	}

	FILE *inputFile = fopen( sOrigFilename.c_str(), "rb" );
	if (inputFile == NULL) {
		ERRORLOG( "Error opening " + sOrigFilename );
		return;
	}

	FILE *outputFile = fopen( sDestFilename.c_str(), "wb" );
	if (outputFile == NULL) {
		ERRORLOG( "Error opening " + sDestFilename );
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
}




vector<string> LocalFileMng::getUserDrumkitList()
{
	vector<string> list;

	//QString sDirectory = QDir::homePath().append( "/.hydrogen/data" );
	QString sDirectory = Preferences::getInstance()->getDataDirectory().c_str();

	QDir dir(sDirectory);
	if ( !dir.exists() ) {
		ERRORLOG( string( "[listUserDrumkits] Directory ").append( sDirectory.toStdString() ).append( " not found." ) );
	}
	else {
		QFileInfoList fileList = dir.entryInfoList();
		dir.setFilter( QDir::Dirs );
		for ( int i = 0; i < fileList.size(); ++i ) {
			string sFile = fileList.at( i ).fileName().toStdString();
			if( ( sFile == "." ) || ( sFile == ".." ) || ( sFile == "CVS" )  || ( sFile == ".svn" ) ) {
				continue;
			}
			list.push_back( sFile );
		}
	}
	return list;
}




vector<string> LocalFileMng::getSystemDrumkitList()
{
	vector<string> list;

	QString sDirectory = QString( DataPath::get_data_path().append( "/drumkits" ).c_str() );
	QDir dir(sDirectory);
	if ( !dir.exists() ) {
		WARNINGLOG( string( "Directory ").append( sDirectory.toStdString() ).append( " not found." ) );
	}
	else {
		QFileInfoList fileList = dir.entryInfoList();
		dir.setFilter( QDir::Dirs );
		for (int i = 0; i < fileList.size(); ++i ) {
			string sFile = fileList.at( i ).fileName().toStdString();
			if( ( sFile == "." ) || ( sFile == ".." ) || ( sFile == "CVS" ) || ( sFile == ".svn" ) ) {
				continue;
			}
			list.push_back( sFile );
		}
	}
	return list;
}



string LocalFileMng::getDrumkitDirectory( const std::string& drumkitName )
{
	// search in system drumkit
	vector<string> systemDrumkits = Drumkit::getSystemDrumkitList();
	for ( unsigned i = 0; i < systemDrumkits.size(); i++ ) {
		if ( systemDrumkits[ i ] == drumkitName ) {
			string path = string( DataPath::get_data_path() ) + "/drumkits/";
			return path;
		}
	}

	// search in user drumkit
	vector<string> userDrumkits = Drumkit::getUserDrumkitList();
	for ( unsigned i = 0; i < userDrumkits.size(); i++ ) {
		if ( userDrumkits[ i ] == drumkitName ) {
		  string path = Preferences::getInstance()->getDataDirectory();
			//string path = QDir::homePath().append("/.hydrogen/data/").toStdString();
			return path;
		}
	}

	ERRORLOG( "drumkit \"" + drumkitName + "\" not found" );
	return "";	// FIXME
}



/// Restituisce un oggetto DrumkitInfo.
/// Gli strumenti non hanno dei veri propri sample,
/// viene utilizzato solo il campo filename.
Drumkit* LocalFileMng::loadDrumkit( const std::string& directory )
{
	//INFOLOG( directory );

	// che if the drumkit.xml file exists
	string drumkitInfoFile = directory + "/drumkit.xml";
	std::ifstream verify( drumkitInfoFile.c_str() , std::ios::in | std::ios::binary );
	if (verify == NULL){
		ERRORLOG( "Load Instrument: Data file " + drumkitInfoFile + " not found." );
		return NULL;
	}

	TiXmlDocument doc( drumkitInfoFile.c_str() );
	doc.LoadFile();

	// root element
	TiXmlNode* drumkitNode;	// root element
	if ( !( drumkitNode = doc.FirstChild( "drumkit_info" ) ) ) {
		ERRORLOG( "Error reading drumkit: drumkit_info node not found" );
		return NULL;
	}

	// Name
	string sDrumkitName = readXmlString( drumkitNode, "name", "");
	if ( sDrumkitName == "") {
		ERRORLOG( "Error reading drumkit: name node not found" );
		return NULL;
	}

	string author = readXmlString( drumkitNode, "author", "undefined author", true );
	string info = readXmlString( drumkitNode, "info", "defaultInfo", true );

	Drumkit *drumkitInfo = new Drumkit();
	drumkitInfo->setName( sDrumkitName );
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
			if ( instrumentList_count > MAX_INSTRUMENTS ) {
				ERRORLOG( "Instrument count >= MAX_INSTRUMENTS. Drumkit: " + drumkitInfo->getName() );
				break;
			}

			string id = readXmlString( instrumentNode, "id", "" );
			string name = readXmlString( instrumentNode, "name", "" );
			float volume = readXmlFloat( instrumentNode, "volume", 1.0f );
			bool isMuted = readXmlBool( instrumentNode, "isMuted", false );
			float pan_L = readXmlFloat( instrumentNode, "pan_L", 1.0f );
			float pan_R = readXmlFloat( instrumentNode, "pan_R", 1.0f );
			bool bFilterActive = readXmlBool( instrumentNode, "filterActive", false, false );
			float fFilterCutoff = readXmlFloat( instrumentNode, "filterCutoff", 1.0f, false, false );
			float fFilterResonance = readXmlFloat( instrumentNode, "filterResonance", 0.0f, false, false );
			float fRandomPitchFactor = readXmlFloat( instrumentNode, "randomPitchFactor", 0.0f, false, false );
			float fAttack = LocalFileMng::readXmlFloat( instrumentNode, "Attack", 0, false, false );		// Attack
			float fDecay = LocalFileMng::readXmlFloat( instrumentNode, "Decay", 0, false, false  );		// Decay
			float fSustain = LocalFileMng::readXmlFloat( instrumentNode, "Sustain", 1.0, false, false );	// Sustain
			float fRelease = LocalFileMng::readXmlFloat( instrumentNode, "Release", 1000, false, false );	// Release
			float fGain = readXmlFloat( instrumentNode, "gain", 1.0f, false, false );
			string sMuteGroup = readXmlString( instrumentNode, "muteGroup", "-1", false, false );
			int nMuteGroup = atoi( sMuteGroup.c_str() );

			// some sanity checks
			if ( id == "" ) {
				ERRORLOG( "Empty ID for instrument. The drumkit '" + sDrumkitName + "' is corrupted. Skipping instrument '" + name + "'" );
				continue;
			}

			Instrument *pInstrument = new Instrument( id, name, new ADSR() );
			pInstrument->set_volume(volume);


			// back compatibility code
			TiXmlNode* filenameNode = instrumentNode->FirstChild( "filename" );
			if ( filenameNode ) {
				//warningLog( "Using back compatibility code. filename node found" );
				string sFilename = LocalFileMng::readXmlString( instrumentNode, "filename", "" );
				Sample *pSample = new Sample(0, sFilename );
				InstrumentLayer *pLayer = new InstrumentLayer( pSample );
				pInstrument->set_layer( pLayer, 0 );
			}
			//~ back compatibility code
			else {
				unsigned nLayer = 0;
				for( TiXmlNode* layerNode = instrumentNode->FirstChild( "layer" ); layerNode; layerNode = layerNode->NextSibling( "layer" ) ) {
					if (nLayer >= MAX_LAYERS) {
						ERRORLOG( "nLayer > MAX_LAYERS" );
						continue;
					}
					string sFilename = LocalFileMng::readXmlString( layerNode, "filename", "" );
					float fMin = LocalFileMng::readXmlFloat( layerNode, "min", 0.0 );
					float fMax = LocalFileMng::readXmlFloat( layerNode, "max", 1.0 );
					float fGain = LocalFileMng::readXmlFloat( layerNode, "gain", 1.0, false, false );
					float fPitch = LocalFileMng::readXmlFloat( layerNode, "pitch", 0.0, false, false );

					Sample *pSample = new Sample(0, sFilename );
					InstrumentLayer *pLayer = new InstrumentLayer( pSample );
					pLayer->set_start_velocity( fMin );
					pLayer->set_end_velocity( fMax );
					pLayer->set_gain( fGain );
					pLayer->set_pitch( fPitch );
					pInstrument->set_layer( pLayer, nLayer );

					nLayer++;
				}
			}

			pInstrument->set_filter_active( bFilterActive );
			pInstrument->set_filter_cutoff( fFilterCutoff );
			pInstrument->set_filter_resonance( fFilterResonance );
			pInstrument->set_muted( isMuted );
			pInstrument->set_pan_l( pan_L );
			pInstrument->set_pan_r( pan_R );
			pInstrument->set_random_pitch_factor( fRandomPitchFactor );
			pInstrument->set_drumkit_name( drumkitInfo->getName() );
			pInstrument->set_gain( fGain );
			pInstrument->set_mute_group( nMuteGroup );

			pInstrument->set_adsr( new ADSR( fAttack, fDecay, fSustain, fRelease ) );
			instrumentList->add( pInstrument );
		}
	}
	else {
		WARNINGLOG( "Error reading drumkit: instrumentList node not found" );
	}
	drumkitInfo->setInstrumentList( instrumentList );

	return drumkitInfo;
}



int LocalFileMng::saveDrumkit( Drumkit *info )
{
	INFOLOG( "[saveDrumkit]" );
	info->dump();	// debug

	string sDrumkitDir = Preferences::getInstance()->getDataDirectory() + info->getName().c_str();

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
	unsigned nInstrument = info->getInstrumentList()->get_size();
	// INSTRUMENT NODE
	for (unsigned i = 0; i < nInstrument; i++) {
		Instrument *instr = info->getInstrumentList()->get(i);

		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer *pLayer = instr->get_layer( nLayer );
			if (pLayer) {
				Sample *pSample = pLayer->get_sample();
				string sOrigFilename = pSample->m_sFilename;

				string sDestFilename = sOrigFilename;

				int nPos = sDestFilename.rfind( '/' );
				sDestFilename = sDestFilename.substr( nPos + 1, sDestFilename.size() - nPos - 1 );
				sDestFilename = sDrumkitDir + "/" + sDestFilename;

				fileCopy( sOrigFilename, sDestFilename );
			}
		}

		TiXmlElement instrumentNode("instrument");

		LocalFileMng::writeXmlString( &instrumentNode, "id", instr->get_id());
		LocalFileMng::writeXmlString( &instrumentNode, "name", instr->get_name());
		LocalFileMng::writeXmlString( &instrumentNode, "volume", toString( instr->get_volume()) );
		LocalFileMng::writeXmlBool( &instrumentNode, "isMuted", instr->is_muted());
		LocalFileMng::writeXmlString( &instrumentNode, "pan_L", toString( instr->get_pan_l()) );
		LocalFileMng::writeXmlString( &instrumentNode, "pan_R", toString( instr->get_pan_r()) );
		LocalFileMng::writeXmlString( &instrumentNode, "randomPitchFactor", toString( instr->get_random_pitch_factor()) );
		LocalFileMng::writeXmlString( &instrumentNode, "gain", toString( instr->get_gain()) );

		LocalFileMng::writeXmlBool( &instrumentNode, "filterActive", instr->is_filter_active());
		LocalFileMng::writeXmlString( &instrumentNode, "filterCutoff", toString( instr->get_filter_cutoff()) );
		LocalFileMng::writeXmlString( &instrumentNode, "filterResonance", toString( instr->get_filter_resonance()) );

		LocalFileMng::writeXmlString( &instrumentNode, "Attack", toString( instr->get_adsr()->__attack ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Decay", toString( instr->get_adsr()->__decay ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Sustain", toString( instr->get_adsr()->__sustain ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Release", toString( instr->get_adsr()->__release ) );

		LocalFileMng::writeXmlString( &instrumentNode, "muteGroup", toString( instr->get_mute_group()) );

		for (unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++) {
			InstrumentLayer *pLayer = instr->get_layer( nLayer );
			if (pLayer == NULL) continue;
			Sample *pSample = pLayer->get_sample();

			string sFilename = pSample->m_sFilename;

			//if (instr->getDrumkitName() != "") {
				// se e' specificato un drumkit, considero solo il nome del file senza il path
				int nPos = sFilename.rfind("/");
				sFilename = sFilename.substr( nPos + 1, sFilename.length() );
			//}

			TiXmlElement layerNode( "layer" );
			LocalFileMng::writeXmlString( &layerNode, "filename", sFilename );
			LocalFileMng::writeXmlString( &layerNode, "min", toString( pLayer->get_start_velocity()) );
			LocalFileMng::writeXmlString( &layerNode, "max", toString( pLayer->get_end_velocity()) );
			LocalFileMng::writeXmlString( &layerNode, "gain", toString( pLayer->get_gain()) );
			LocalFileMng::writeXmlString( &layerNode, "pitch", toString( pLayer->get_pitch() ) );

			instrumentNode.InsertEndChild( layerNode );
		}

		instrumentListNode.InsertEndChild(instrumentNode);
	}

	rootNode.InsertEndChild(instrumentListNode);

	doc.InsertEndChild( rootNode );
	doc.SaveFile();

	return 0; // ok
}




string LocalFileMng::readXmlString( TiXmlNode* parent, const std::string& nodeName, const std::string& defaultValue, bool bCanBeEmpty, bool bShouldExists )
{
	TiXmlNode* node;
	if ( parent && ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			return node->FirstChild()->Value();
		}
		else {
			if (!bCanBeEmpty) {
				_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	}
	else {
		if ( bShouldExists ) {
			_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}



float LocalFileMng::readXmlFloat( TiXmlNode* parent, const std::string& nodeName, float defaultValue, bool bCanBeEmpty, bool bShouldExists )
{
	TiXmlNode* node;
	if ( parent && ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			float res = stringToFloat( node->FirstChild()->Value() );
			return res;
		}
		else {
			if (!bCanBeEmpty) {
				_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	}
	else {
		if ( bShouldExists ) {
			_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}



int LocalFileMng::readXmlInt( TiXmlNode* parent, const std::string& nodeName, int defaultValue, bool bCanBeEmpty, bool bShouldExists)
{
	TiXmlNode* node;
	if ( parent && ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			return atoi( node->FirstChild()->Value() );
		}
		else {
			if (!bCanBeEmpty) {
				_WARNINGLOG( "Using default value in " + nodeName );
			}
			return defaultValue;
		}
	}
	else {
		if (bShouldExists)  {
			_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}



bool LocalFileMng::readXmlBool( TiXmlNode* parent, const std::string& nodeName, bool defaultValue, bool bShouldExists )
{
	TiXmlNode* node;
	if ( parent && ( node = parent->FirstChild( nodeName.c_str() ) ) ) {
		if ( node->FirstChild() ) {
			if ( string( node->FirstChild()->Value() ) == "true" ) {
				return true;
			}
			else {
				return false;
			}
		}
		else {
			_WARNINGLOG( "Using default value in " + nodeName );
			return defaultValue;
		}
	}
	else {
		if (bShouldExists) {
			_WARNINGLOG( "'" + nodeName + "' node not found" );
		}
		return defaultValue;
	}
}



void LocalFileMng::writeXmlString( TiXmlNode *parent, const std::string& name, const std::string& text )
{
	TiXmlElement versionNode( name );
	TiXmlText versionText( text );
	versionNode.InsertEndChild( versionText );
	parent->InsertEndChild( versionNode );
}



void LocalFileMng::writeXmlBool( TiXmlNode *parent, const std::string& name, bool value )
{
	if (value) {
		writeXmlString( parent, name, string("true") );
	}
	else {
		writeXmlString( parent, name, string("false") );
	}
}












//-----------------------------------------------------------------------------
//	Implementation of SongWriter class
//-----------------------------------------------------------------------------


SongWriter::SongWriter()
 : Object( "SongWriter" )
{
//	infoLog("init");
}



SongWriter::~SongWriter() {
//	infoLog("destroy");
}



void SongWriter::writeSong(Song *song, const std::string& filename)
{
	INFOLOG( "Saving song " + filename );

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

/*	LocalFileMng::writeXmlBool( &songNode, "delayFXEnabled", song->m_bDelayFXEnabled );
	LocalFileMng::writeXmlString( &songNode, "delayFXWetLevel", toString( song->m_fDelayFXWetLevel ) );
	LocalFileMng::writeXmlString( &songNode, "delayFXFeedback", toString( song->m_fDelayFXFeedback ) );
	LocalFileMng::writeXmlString( &songNode, "delayFXTime", toString( song->m_nDelayFXTime ) );
*/

	// instrument list
	TiXmlElement instrumentListNode("instrumentList");
	unsigned nInstrument = song->getInstrumentList()->get_size();

	// INSTRUMENT NODE
	for ( unsigned i = 0; i < nInstrument; i++ ) {
		Instrument *instr = song->getInstrumentList()->get(i);
		assert( instr );

		TiXmlElement instrumentNode("instrument");

		LocalFileMng::writeXmlString( &instrumentNode, "id", instr->get_id());
		LocalFileMng::writeXmlString( &instrumentNode, "drumkit", instr->get_drumkit_name());
		LocalFileMng::writeXmlString( &instrumentNode, "name", instr->get_name());
		LocalFileMng::writeXmlString( &instrumentNode, "volume", toString( instr->get_volume()) );
		LocalFileMng::writeXmlBool( &instrumentNode, "isMuted", instr->is_muted());
		LocalFileMng::writeXmlString( &instrumentNode, "pan_L", toString( instr->get_pan_l()) );
		LocalFileMng::writeXmlString( &instrumentNode, "pan_R", toString( instr->get_pan_r()) );
		LocalFileMng::writeXmlString( &instrumentNode, "gain", toString( instr->get_gain() ) );

		LocalFileMng::writeXmlBool( &instrumentNode, "filterActive", instr->is_filter_active());
		LocalFileMng::writeXmlString( &instrumentNode, "filterCutoff", toString( instr->get_filter_cutoff()) );
		LocalFileMng::writeXmlString( &instrumentNode, "filterResonance", toString( instr->get_filter_resonance()) );

		LocalFileMng::writeXmlString( &instrumentNode, "FX1Level", toString( instr->get_fx_level( 0 ) ) );
		LocalFileMng::writeXmlString( &instrumentNode, "FX2Level", toString( instr->get_fx_level( 1 ) ) );
		LocalFileMng::writeXmlString( &instrumentNode, "FX3Level", toString( instr->get_fx_level( 2 ) ) );
		LocalFileMng::writeXmlString( &instrumentNode, "FX4Level", toString( instr->get_fx_level( 3 ) ) );

		assert( instr->get_adsr());
		LocalFileMng::writeXmlString( &instrumentNode, "Attack", toString( instr->get_adsr()->__attack ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Decay", toString( instr->get_adsr()->__decay ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Sustain", toString( instr->get_adsr()->__sustain ) );
		LocalFileMng::writeXmlString( &instrumentNode, "Release", toString( instr->get_adsr()->__release ) );

		LocalFileMng::writeXmlString( &instrumentNode, "randomPitchFactor", toString( instr->get_random_pitch_factor()) );

		LocalFileMng::writeXmlString( &instrumentNode, "muteGroup", toString( instr->get_mute_group()) );

		for (unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++) {
			InstrumentLayer *pLayer = instr->get_layer( nLayer );
			if (pLayer == NULL) continue;
			Sample *pSample = pLayer->get_sample();
			if (pSample == NULL) continue;

			string sFilename = pSample->m_sFilename;

			if (instr->get_drumkit_name() != "") {
				// se e' specificato un drumkit, considero solo il nome del file senza il path
				int nPos = sFilename.rfind("/");
				sFilename = sFilename.substr( nPos + 1, sFilename.length() );
			}

			TiXmlElement layerNode( "layer" );
			LocalFileMng::writeXmlString( &layerNode, "filename", sFilename );
			LocalFileMng::writeXmlString( &layerNode, "min", toString( pLayer->get_start_velocity()) );
			LocalFileMng::writeXmlString( &layerNode, "max", toString( pLayer->get_end_velocity()) );
			LocalFileMng::writeXmlString( &layerNode, "gain", toString( pLayer->get_gain()) );
			LocalFileMng::writeXmlString( &layerNode, "pitch", toString( pLayer->get_pitch()) );

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

		TiXmlElement noteListNode( "noteList" );
		std::multimap <int, Note*>::iterator pos;
		for ( pos = pat->m_noteMap.begin(); pos != pat->m_noteMap.end(); ++pos ) {
			Note *pNote = pos->second;
			assert( pNote );

			TiXmlElement noteNode("note");
			LocalFileMng::writeXmlString( &noteNode, "position", toString( pNote->get_position()) );
			LocalFileMng::writeXmlString( &noteNode, "velocity", toString( pNote->get_velocity() ) );
			LocalFileMng::writeXmlString( &noteNode, "pan_L", toString( pNote->get_pan_l()) );
			LocalFileMng::writeXmlString( &noteNode, "pan_R", toString( pNote->get_pan_r()) );
			LocalFileMng::writeXmlString( &noteNode, "pitch", toString( pNote->get_pitch() ) );

			LocalFileMng::writeXmlString( &noteNode, "key", Note::keyToString( pNote->m_noteKey ) );

			LocalFileMng::writeXmlString( &noteNode, "length", toString( pNote->get_lenght() ) );
			LocalFileMng::writeXmlString( &noteNode, "instrument", pNote->get_instrument()->get_id());
			noteListNode.InsertEndChild( noteNode );
		}
		patternNode.InsertEndChild( noteListNode );

		patternListNode.InsertEndChild( patternNode );
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
		LadspaFX *pFX = Effects::getInstance()->getLadspaFX(nFX);
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


};

