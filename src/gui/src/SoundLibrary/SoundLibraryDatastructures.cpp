#include "SoundLibraryImportDialog.h"
#include "SoundLibraryRepositoryDialog.h"
#include "SoundLibraryPanel.h"

#include "../widgets/DownloadWidget.h"
#include "../HydrogenApp.h"
#include "../InstrumentRack.h"

#include <hydrogen/LocalFileMng.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/helpers/filesystem.h>

#include "SoundLibraryDatastructures.h"

#include <hydrogen/object.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/basics/drumkit.h>

using namespace H2Core;

SoundLibraryDatabase* SoundLibraryDatabase::__instance = NULL;

const char* SoundLibraryDatabase::__class_name = "SoundLibraryDatabase";

SoundLibraryDatabase::SoundLibraryDatabase() : Object( __class_name )
{
	INFOLOG( "INIT" );
	patternVector = new soundLibraryInfoVector();
	updatePatterns();
}

SoundLibraryDatabase::~SoundLibraryDatabase()
{
	//Clean up the patterns data structure
	soundLibraryInfoVector::iterator mapIterator;
	for( mapIterator=patternVector->begin(); mapIterator != patternVector->end(); mapIterator++ )
	{
		delete *mapIterator;
	}

	delete patternVector;
}

void SoundLibraryDatabase::create_instance()
{
	if ( __instance == 0 ) {
		__instance = new SoundLibraryDatabase;
	}
}

void SoundLibraryDatabase::printPatterns()
{
	soundLibraryInfoVector::iterator mapIterator;
	for( mapIterator=patternVector->begin(); mapIterator != patternVector->end(); mapIterator++ )
	{
		INFOLOG(  QString( "Name: " + (*mapIterator)->getName() ) );
	}

	for (int i = 0; i < patternCategories.size(); ++i)
			  INFOLOG( patternCategories.at(i) )
}

bool SoundLibraryDatabase::isPatternInstalled( const QString& patternName)
{

	soundLibraryInfoVector::iterator mapIterator;
	for( mapIterator=patternVector->begin(); mapIterator != patternVector->end(); mapIterator++ )
	{
		if( (*mapIterator)->getName() == patternName ) return true;
	}
	return false;
}

void SoundLibraryDatabase::update()
{
	updatePatterns();
	//updateSongs();
	//updateDrumkits();
}

void SoundLibraryDatabase::updatePatterns()
{
	//Clear the current pattern informations, then start to fill it again..
	patternVector->clear();
	patternCategories = QStringList();

	/* First location to store patterns: inside .hydrogen/data/patterns/GMkit etc.
	 * each subdir builds a relationship between the pattern and the drumkit
	 */

	std::vector<QString> perDrumkitPatternDirectories = LocalFileMng::getDrumkitsFromDirectory( Preferences::get_instance()->getDataDirectory() + "patterns" );
	std::vector<QString>::iterator drumkitIterator;

	for( drumkitIterator = perDrumkitPatternDirectories.begin(); drumkitIterator != perDrumkitPatternDirectories.end(); drumkitIterator++ )
	{
		getPatternFromDirectory( *drumkitIterator, patternVector);
	}

	//2. This is the general location for patterns which do not belong to a certain drumkit.
	QString userPatternDirectory = Preferences::get_instance()->getDataDirectory() + "patterns";
	getPatternFromDirectory( userPatternDirectory, patternVector);
}

int SoundLibraryDatabase::getPatternFromDirectory( const QString& sPatternDir, std::vector<SoundLibraryInfo*>* patternVector )
{
	QDir dir( sPatternDir );

	if ( !dir.exists() ) {
		ERRORLOG( QString( "[getPatternList] Directory %1 not found" ).arg( sPatternDir ) );
	} else {
		dir.setFilter( QDir::Files );
		QFileInfoList fileList = dir.entryInfoList();

		for ( int i = 0; i < fileList.size(); ++i ) {
			QString sFile = sPatternDir + "/" + fileList.at( i ).fileName();

			if ( sFile.endsWith( Filesystem::pattern_ext ) ) {
				SoundLibraryInfo* slInfo = new SoundLibraryInfo( sFile );
				patternVector->push_back( slInfo );

				if(! patternCategories.contains( slInfo->getCategory() ) ) patternCategories << slInfo->getCategory();
			}
		}
	}
	return 0;
}


soundLibraryInfoVector* SoundLibraryDatabase::getAllPatterns() const
{
	return patternVector;
}





const char* SoundLibraryInfo::__class_name = "SoundLibraryInfo";
SoundLibraryInfo::SoundLibraryInfo() : Object( __class_name )
{
	//default constructor
}

SoundLibraryInfo::SoundLibraryInfo(const QString &path) : Object( __class_name )
{
	/*
	 *Use the provided file instantiate this object with the corresponding meta
	 *data from either a drumkit, a pattern or a song.
	 */

	QDomDocument doc  = LocalFileMng::openXmlDocument( path );
	setPath( path );

	QDomNode rootNode =  doc.firstChildElement( "drumkit_pattern" );
	if ( !rootNode.isNull() )
	{
		setType( "pattern" );
		setAuthor( LocalFileMng::readXmlString( rootNode,"author", "undefined author" ) );
		setLicense( LocalFileMng::readXmlString( rootNode,"license", "undefined license" ) );

		QDomNode patternNode = rootNode.firstChildElement( "pattern" );
		setName( LocalFileMng::readXmlString( patternNode,"pattern_name", "" ) );
		setInfo( LocalFileMng::readXmlString( patternNode,"info", "No information available." ) );
		setCategory( LocalFileMng::readXmlString( patternNode,"category", "" ) );
	}


	//New drumkits
	rootNode = doc.firstChildElement( "drumkit_info" );
	if ( !rootNode.isNull() )
	{
		setType( "drumkit" );
		setAuthor( LocalFileMng::readXmlString( rootNode,"author", "undefined author" ) );
		setLicense( LocalFileMng::readXmlString( rootNode,"license", "undefined license" ) );
		setName( LocalFileMng::readXmlString( rootNode,"name", "" ) );
		setInfo( LocalFileMng::readXmlString( rootNode,"info", "No information available." ) );
		setImage( LocalFileMng::readXmlString( rootNode,"image", "" ) );
		setImageLicense( LocalFileMng::readXmlString( rootNode,"imageLicense", "undefined license" ) );

		//setCategory( LocalFileMng::readXmlString( rootNode,"category", "" ) );
	}

	//Songs
	rootNode = doc.firstChildElement( "song" );
	if ( !rootNode.isNull() )
	{
		setType( "song" );
		setAuthor( LocalFileMng::readXmlString( rootNode,"author", "undefined author" ) );
		setLicense( LocalFileMng::readXmlString( rootNode,"license", "undefined license" ) );
		setName( LocalFileMng::readXmlString( rootNode,"name", "" ) );
		setInfo( LocalFileMng::readXmlString( rootNode,"info", "No information available." ) );
		//setCategory( LocalFileMng::readXmlString( rootNode,"category", "" ) );
	}
}

SoundLibraryInfo::~SoundLibraryInfo()
{
	//default deconstructor
}




