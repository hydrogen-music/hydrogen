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

#include <QDir>

#include <hydrogen/SoundLibrary.h>
#include <hydrogen/Instrument.h>
#include <hydrogen/Sample.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/H2Exception.h>
#include <hydrogen/Hydrogen.h>
#include <hydrogen/ADSR.h>
#include <hydrogen/Preferences.h>

#include <zlib.h>	// used for drumkit install
#include <libtar.h>	// used for drumkit install
#include <fcntl.h>
#include <errno.h>

namespace H2Core {

SoundLibrary::SoundLibrary()
: Object( "SoundLibrary" )
{
}



SoundLibrary::~SoundLibrary()
{
}



// ::::::::::



Drumkit::Drumkit()
 : Object( "Drumkit" )
 , m_pInstrumentList( NULL )
{
}



Drumkit::~Drumkit()
{
	delete m_pInstrumentList;
}




Drumkit* Drumkit::load( const std::string& sFilename )
{
	LocalFileMng mng;
	return mng.loadDrumkit( sFilename );
}



std::vector<std::string> Drumkit::getUserDrumkitList()
{
	LocalFileMng mng;
	return mng.getUserDrumkitList();
}



std::vector<std::string> Drumkit::getSystemDrumkitList()
{
	LocalFileMng mng;
	return mng.getSystemDrumkitList();
}



void Drumkit::dump()
{
	INFOLOG( "Drumkit dump" );
	INFOLOG( "\t|- Name = " + m_sName );
	INFOLOG( "\t|- Author = " + m_sAuthor );
	INFOLOG( "\t|- Info = " + m_sInfo );

	INFOLOG( "\t|- Instrument list" );
	for ( unsigned nInstrument = 0; nInstrument < m_pInstrumentList->getSize(); ++nInstrument) {
		Instrument *pInstr = m_pInstrumentList->get( nInstrument );
		INFOLOG( "\t\t|- (" + toString( nInstrument ) + " of " + toString( m_pInstrumentList->getSize() ) + ") Name = " + pInstr->m_sName );
		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
			InstrumentLayer *pLayer = pInstr->getLayer( nLayer );
			if ( pLayer ) {
				Sample *pSample = pLayer->m_pSample;
				if ( pSample ) {
					INFOLOG( "\t\t   |- " + pSample->m_sFilename );
				}
				else {
					INFOLOG( "\t\t   |- NULL sample" );
				}
			}
			else {
				INFOLOG( "\t\t   |- NULL Layer" );
			}

		}
//		cout << "\t\t" << i << " - " << instr->getSample()->getFilename() << endl;
	}
}



void Drumkit::install( const std::string& filename )
{
	_INFOLOG( "[Drumkit::install] drumkit = " + filename );
	string dataDir = Preferences::getInstance()->getDataDirectory();

	// GUNZIP !!!
	string gunzippedName = filename.substr( 0, filename.rfind( "." ) );
	gunzippedName += ".tar";
	FILE *pGunzippedFile = fopen( gunzippedName.c_str(), "wb" );
	gzFile gzipFile = gzopen( filename.c_str(), "rb" );
	uchar buf[4096];
	while ( gzread( gzipFile, buf, 4096 ) > 0 ) {
		fwrite( buf, sizeof( uchar ), 4096, pGunzippedFile );
	}
	gzclose( gzipFile );
	fclose( pGunzippedFile );


	// UNTAR !!!
	TAR *tarFile;

	char tarfilename[1024];
	strcpy( tarfilename, gunzippedName.c_str() );

	if (tar_open(&tarFile, tarfilename, NULL, O_RDONLY, 0, TAR_VERBOSE | TAR_GNU ) == -1) {
		_ERRORLOG( "[Drumkit::install] tar_open(): " + strerror(errno) );
	}

	char destDir[1024];
	strcpy( destDir, dataDir.c_str() );
	if ( tar_extract_all( tarFile, destDir ) != 0) {
		_ERRORLOG( "[Drumkit::install] tar_extract_all(): " + strerror(errno) );
	}

	if ( tar_close( tarFile ) != 0 ) {
		_ERRORLOG( "[Drumkit::install] tar_close(): " + strerror(errno) );
	}
}



void Drumkit::save( const std::string& sName, const std::string& sAuthor, const std::string& sInfo )
{
	_INFOLOG( "Saving drumkit" );

	Drumkit *pDrumkitInfo = new Drumkit();
	pDrumkitInfo->setName( sName );
	pDrumkitInfo->setAuthor( sAuthor );
	pDrumkitInfo->setInfo( sInfo );

	Song *pSong = Hydrogen::getInstance()->getSong();
	InstrumentList *pSongInstrList = pSong->getInstrumentList();
	InstrumentList *pInstrumentList = new InstrumentList();

	for ( uint nInstrument = 0; nInstrument < pSongInstrList->getSize(); nInstrument++ ) {
		Instrument *pOldInstr = pSongInstrList->get( nInstrument );
		Instrument *pNewInstr = new Instrument();
		pNewInstr->m_sId = pOldInstr->m_sId;
		pNewInstr->m_fVolume = pOldInstr->m_fVolume;
		pNewInstr->m_sName = pOldInstr->m_sName;
		pNewInstr->m_fPan_L = pOldInstr->m_fPan_L;
		pNewInstr->m_fPan_R = pOldInstr->m_fPan_R;
		pNewInstr->m_bIsMuted = pOldInstr->m_bIsMuted;
		pNewInstr->m_fRandomPitchFactor = pOldInstr->m_fRandomPitchFactor;
		pNewInstr->m_nMuteGroup = pOldInstr->m_nMuteGroup;
		pNewInstr->m_pADSR = new ADSR( *(pOldInstr->m_pADSR) );

		pNewInstr->m_bFilterActive = pOldInstr->m_bFilterActive;
		pNewInstr->m_fCutoff = pOldInstr->m_fCutoff;
		pNewInstr->m_fResonance = pOldInstr->m_fResonance;


		string sInstrDrumkit = pOldInstr->m_sDrumkitName;

		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer *pOldLayer = pOldInstr->getLayer( nLayer );
			if ( pOldLayer ) {
				Sample *pSample = pOldLayer->m_pSample;

				Sample *pNewSample = new Sample( 0, pSample->m_sFilename );	// is not a real sample, it contains only the filename information
				InstrumentLayer *pLayer = new InstrumentLayer( pNewSample );
				pLayer->m_fGain = pOldLayer->m_fGain;
				pLayer->m_fPitch = pOldLayer->m_fPitch;
				pLayer->m_fStartVelocity = pOldLayer->m_fStartVelocity;
				pLayer->m_fEndVelocity = pOldLayer->m_fEndVelocity;

				pNewInstr->setLayer( pLayer, nLayer );
			}
			else {
				pNewInstr->setLayer( NULL, nLayer );
			}
		}
		pInstrumentList->add ( pNewInstr );
	}

	pDrumkitInfo->setInstrumentList( pInstrumentList );

	LocalFileMng fileMng;
	int err = fileMng.saveDrumkit( pDrumkitInfo );
	if (err != 0) {
		_ERRORLOG( "Error saving the drumkit" );
		throw H2Exception( "Error saving the drumkit" );
	}

	// delete the drumkit info
	delete pDrumkitInfo;
	pDrumkitInfo = NULL;

}




void Drumkit::removeDrumkit( const std::string& sDrumkitName )
{
	_INFOLOG( "Removing drumkit: " + sDrumkitName );

	string dataDir = Preferences::getInstance()->getDataDirectory();
	dataDir += sDrumkitName;
	string cmd = string( "rm -rf \"" ) + dataDir + string( "\"" );
	_INFOLOG( cmd );
	if ( system( cmd.c_str() ) != 0 ) {
		_ERRORLOG( "Error executing '" + cmd + "'" );
		throw H2Exception( "Error executing '" + cmd + "'" );
	}
}

};

