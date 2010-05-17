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

#include <QDir>

#include <hydrogen/SoundLibrary.h>
#include <hydrogen/instrument.h>
#include <hydrogen/sample.h>
#include <hydrogen/filesystem.h>
#include <hydrogen/xml_helper.h>
#include <hydrogen/h2_exception.h>
#include <hydrogen/hydrogen.h>
#include <hydrogen/adsr.h>
#include <hydrogen/Preferences.h>

#include <cstdlib>

#ifdef H2CORE_HAVE_LIBARCHIVE
#include <archive.h>		// used for drumkit install
#include <archive_entry.h>	// used for drumkit install
#else //use libtar
	//disabled libtar on windows
	#ifndef WIN32
		#include <zlib.h>       // used for drumkit install
		#include <libtar.h>     // used for drumkit install
	#endif
#endif

#include <fcntl.h>
#include <errno.h>
namespace H2Core
{

const char* Drumkit::__class_name = "Drumkit";

Drumkit::Drumkit()
		: Object( __class_name )
		, m_pInstrumentList( NULL )
{
}



Drumkit::~Drumkit()
{
	delete m_pInstrumentList;
}




Drumkit* Drumkit::load( const QString& path ) {
    INFOLOG( "[Drumkit::load]" );
    if( !Filesystem::drumkit_valid( path ) ) {
        ERRORLOG( QString("%1 is not valid drumkit").arg(path));
        return 0;
    }
    XMLDoc doc;
    if( !doc.read( Filesystem::drumkit_file(path) ) ) return 0;
    // TODO XML VALIDATION !!!!!!!!!!
    XMLNode root = doc.firstChildElement( "drumkit_info" );
    if ( root.isNull() ) {
        ERRORLOG( "drumkit_info node not found" );
        return 0;
    }
    QString drumkit_name = root.read_string( "name", "", false, false );
    if ( drumkit_name.isEmpty() ) {
        ERRORLOG( QString("Drumkit: %1 has no name, abort ").arg(path));
        return 0;
    }
    Drumkit *drumkitInfo = new Drumkit();
    drumkitInfo->setName( drumkit_name );
    drumkitInfo->setAuthor( root.read_string( "author", "undefined author" ) );
    drumkitInfo->setInfo( root.read_string( "info", "defaultInfo" ) );
    drumkitInfo->setLicense( root.read_string( "license", "undefined license" ) );
    InstrumentList *instrumentList = new InstrumentList();
    XMLNode instruments_node = root.firstChildElement( "instrumentList" );
    if ( instruments_node.isNull() ) {
        WARNINGLOG( "instrumentList node not found" );
    } else {
        int count = 0;
        XMLNode node = instruments_node.firstChildElement( "instrument" );
        while ( !node.isNull() ) {
            count++;
            if ( count > MAX_INSTRUMENTS ) {
                ERRORLOG( QString("Drumkit: %1 instrument count >= %2").arg(drumkitInfo->getName()).arg(MAX_INSTRUMENTS) );
                break;
            }
            QString id = node.read_string( "id", "" );
            if ( id.isEmpty() ) {
                ERRORLOG( QString("Empty ID for instrument. The drumkit %1 is corrupted. Skipping instrument %2").arg(drumkit_name).arg(count) );
                node = node.nextSiblingElement( "instrument" );
                continue;
            }
            Instrument *instrument = new Instrument( id, node.read_string( "name", "" ), 0 );
            instrument->set_volume( node.read_float( "volume", 1.0f ) );
            instrument->set_muted( node.read_bool( "isMuted", false ) );
            instrument->set_pan_l( node.read_float( "pan_L", 1.0f ) );
            instrument->set_pan_r( node.read_float( "pan_R", 1.0f ) );
            // may not exist, but can't be empty
            instrument->set_filter_active( node.read_bool( "filterActive", true, false ) );
            instrument->set_filter_cutoff( node.read_float( "filterCutoff", 1.0f, true, false ) );
            instrument->set_filter_resonance( node.read_float( "filterResonance", 0.0f, true, false ) );
            instrument->set_random_pitch_factor( node.read_float( "randomPitchFactor", 0.0f, true, false ) );
            float fAttack = node.read_float( "Attack", 0, true, false );
            float fDecay = node.read_float( "Decay", 0, true, false  );
            float fSustain = node.read_float( "Sustain", 1.0, true, false );
            float fRelease = node.read_float( "Release", 1000, true, false );
            instrument->set_adsr( new ADSR( fAttack, fDecay, fSustain, fRelease ) );
            instrument->set_gain( node.read_float( "gain", 1.0f, true, false ) );
            instrument->set_mute_group( node.read_int( "muteGroup", -1, true, false ) );
            instrument->set_midi_out_channel( node.read_int( "midiOutChannel", -1, true, false ) );
            instrument->set_midi_out_note( node.read_int( "midiOutNote", 60, true, false ) );
            instrument->set_stop_note( node.read_bool( "isStopNote", true ,false ) );
            QDomNode filenameNode = node.firstChildElement( "filename" );
            if ( ! filenameNode.isNull() ) {
                // back compatibility code
                WARNINGLOG( "Using back compatibility code. filename node found" );
                QString sFilename = node.read_string( "filename", "" );
                Sample *pSample = new Sample( 0, sFilename, 0 );
                InstrumentLayer *pLayer = new InstrumentLayer( pSample );
                instrument->set_layer( pLayer, 0 );
            } else {
                int nLayer = 0;
                XMLNode layerNode = node.firstChildElement( "layer" );
                while ( !layerNode.isNull() ) {
                    if ( nLayer >= MAX_LAYERS ) {
                        ERRORLOG( "nLayer > MAX_LAYERS" );
                        layerNode = layerNode.nextSiblingElement( "layer" );
                        continue;
                    }
                    Sample* sample = new Sample( 0, layerNode.read_string( "filename", "" ), 0 );
                    InstrumentLayer* layer = new InstrumentLayer( sample );
                    layer->set_start_velocity( layerNode.read_float( "min", 0.0 ) );
                    layer->set_end_velocity( layerNode.read_float( "max", 1.0 ) );
                    layer->set_gain( layerNode.read_float( "gain", 1.0, true, false ) );
                    layer->set_pitch( layerNode.read_float( "pitch", 0.0, true, false ) );
                    instrument->set_layer( layer, nLayer );
                    nLayer++;
                    layerNode = layerNode.nextSiblingElement( "layer" );
                }
            }
            instrumentList->add( instrument );
            node = node.nextSiblingElement( "instrument" );
        }
    }
    drumkitInfo->setInstrumentList( instrumentList );
    return drumkitInfo;
}

bool Drumkit::save( ) {
    INFOLOG( "[Drumkit::save]" );
    QVector<QString> tempVector(MAX_LAYERS);
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/" + getName();
    if( !Filesystem::mkdir( dk_dir) ) return false;
    XMLDoc doc;
    QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild( header );
    XMLNode root = doc.createElement( "drumkit_info" );
    root.write_string( "name", getName() );
    root.write_string( "author", getAuthor() );
    root.write_string( "info", getInfo() );
    root.write_string( "license", getLicense() );
    XMLNode instruments_node = doc.createElement( "instrumentList" );
    for ( int i = 0; i < getInstrumentList()->get_size(); i++ ) {
        Instrument *instr = getInstrumentList()->get( i );
        for ( int nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
            InstrumentLayer *layer = instr->get_layer( nLayer );
            if ( layer ) {
                Sample *pSample = layer->get_sample();
                QString src = pSample->get_filename();
                QString dst = src;
                /*
                 * Till rev. 743, the samples got copied into the
                 * root of the drumkit folder.
                 * Now the sample gets only copied to the folder
                 * if it doesn't reside in a subfolder of the drumkit dir.
                 * */
                if( src.startsWith( dk_dir ) ){
                    INFOLOG("sample is already in drumkit dir");
                    tempVector[ nLayer ] = dst.remove( dk_dir + "/" );
                } else {
                    int nPos = dst.lastIndexOf( '/' );
                    dst = dst.mid( nPos + 1, dst.size() - nPos - 1 );
                    dst = dk_dir + "/" + dst;
                    Filesystem::file_copy( src, dst );
                    tempVector[ nLayer ] = dst.remove( dk_dir + "/" );
                }
            }
        }
        XMLNode instrument_node = doc.createElement( "instrument" );
        instrument_node.write_string( "id", instr->get_id() );
        instrument_node.write_string( "name", instr->get_name() );
        instrument_node.write_float( "volume", instr->get_volume() );
        instrument_node.write_bool( "isMuted", instr->is_muted() );
        instrument_node.write_float( "pan_L", instr->get_pan_l() );
        instrument_node.write_float( "pan_R", instr->get_pan_r() );
        instrument_node.write_float( "randomPitchFactor", instr->get_random_pitch_factor() );
        instrument_node.write_float( "gain", instr->get_gain() );
        instrument_node.write_bool( "filterActive", instr->is_filter_active() );
        instrument_node.write_float( "filterCutoff", instr->get_filter_cutoff() );
        instrument_node.write_float( "filterResonance", instr->get_filter_resonance() );
        instrument_node.write_float( "Attack", instr->get_adsr()->__attack );
        instrument_node.write_float( "Decay", instr->get_adsr()->__decay );
        instrument_node.write_float( "Sustain", instr->get_adsr()->__sustain );
        instrument_node.write_float( "Release", instr->get_adsr()->__release );
        instrument_node.write_int( "muteGroup", instr->get_mute_group() );
        instrument_node.write_bool( "isStopNote", instr->is_stop_notes() );
        instrument_node.write_int( "midiOutChannel", instr->get_midi_out_channel() );
        instrument_node.write_int( "midiOutNote", instr->get_midi_out_note() );
        for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
            InstrumentLayer *layer = instr->get_layer( nLayer );
            if ( layer == NULL ) continue;
            XMLNode layer_node = doc.createElement( "layer" );
            layer_node.write_string( "filename", tempVector[ nLayer ] );
            layer_node.write_float( "min", layer->get_start_velocity() );
            layer_node.write_float( "max", layer->get_end_velocity() );
            layer_node.write_float( "gain", layer->get_gain() );
            layer_node.write_float( "pitch", layer->get_pitch() );
            instrument_node.appendChild( layer_node );
        }
        instruments_node.appendChild( instrument_node );
    }
    root.appendChild( instruments_node );
    doc.appendChild( root );
    return doc.write( Filesystem::drumkit_file(dk_dir) );
}


void Drumkit::dump()
{
	INFOLOG( "Drumkit dump" );
	INFOLOG( "\t|- Name = " + m_sName );
	INFOLOG( "\t|- Author = " + m_sAuthor );
	INFOLOG( "\t|- Info = " + m_sInfo );

	INFOLOG( "\t|- Instrument list" );
	for ( unsigned nInstrument = 0; nInstrument < m_pInstrumentList->get_size(); ++nInstrument ) {
		Instrument *pInstr = m_pInstrumentList->get( nInstrument );
		INFOLOG( QString("\t\t|- (%1 of %2) Name = %3")
			 .arg( nInstrument )
			 .arg( m_pInstrumentList->get_size() )
			 .arg( pInstr->get_name() )
			);
		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; ++nLayer ) {
			InstrumentLayer *pLayer = pInstr->get_layer( nLayer );
			if ( pLayer ) {
				Sample *pSample = pLayer->get_sample();
				if ( pSample ) {
					INFOLOG( "\t\t   |- " + pSample->get_filename() );
				} else {
					INFOLOG( "\t\t   |- NULL sample" );
				}
			} else {
				INFOLOG( "\t\t   |- NULL Layer" );
			}

		}
//		cout << "\t\t" << i << " - " << instr->getSample()->getFilename() << endl;
	}
}

#ifdef H2CORE_HAVE_LIBARCHIVE
void Drumkit::install( const QString& filename )
{
	_INFOLOG( "[Drumkit::install] drumkit = " + filename );
	QString dataDir = Preferences::get_instance()->getDataDirectory() + "drumkits/";

	int r;
	struct archive *drumkitFile;
	struct archive_entry *entry;
	char newpath[1024];

	drumkitFile = archive_read_new();
	archive_read_support_compression_all(drumkitFile);
	archive_read_support_format_all(drumkitFile);
	if (( r = archive_read_open_file(drumkitFile, filename.toLocal8Bit(), 10240) )) {
		_ERRORLOG( QString( "Error: %2, Could not open drumkit: %1" )
			.arg( archive_errno(drumkitFile))
			.arg( archive_error_string(drumkitFile)) );
		archive_read_close(drumkitFile);
		archive_read_finish(drumkitFile);
		return;
	}
	while ( (r = archive_read_next_header(drumkitFile, &entry)) != ARCHIVE_EOF) {
		if (r != ARCHIVE_OK) {
			_ERRORLOG( QString( "Error reading drumkit file: %1")
				.arg(archive_error_string(drumkitFile)));
			break;
		}

		// insert data directory prefix
		QString np = dataDir + archive_entry_pathname(entry);
		strcpy( newpath, np.toLocal8Bit() );
		archive_entry_set_pathname(entry, newpath);

		// extract tarball
		r = archive_read_extract(drumkitFile, entry, 0);
		if (r == ARCHIVE_WARN) {
			_WARNINGLOG( QString( "warning while extracting %1 (%2)").arg(filename).arg(archive_error_string(drumkitFile)));
		} else if (r != ARCHIVE_OK) {
			_ERRORLOG( QString( "error while extracting %1 (%2)").arg(filename).arg(archive_error_string(drumkitFile)));
			break;
		}
	}
	archive_read_close(drumkitFile);
	archive_read_finish(drumkitFile);
}
#else //use libtar

#ifndef WIN32
void Drumkit::install( const QString& filename )
{
        _INFOLOG( "[Drumkit::install] drumkit = " + filename );
        QString dataDir = Preferences::get_instance()->getDataDirectory() + "drumkits/";

        // GUNZIP !!!
        QString gunzippedName = filename.left( filename.indexOf( "." ) );
        gunzippedName += ".tar";
        FILE *pGunzippedFile = fopen( gunzippedName.toLocal8Bit(), "wb" );
        gzFile gzipFile = gzopen( filename.toLocal8Bit(), "rb" );
	if ( !gzipFile ) {	
		throw H2Exception( "Error opening gzip file" );
	}

        uchar buf[4096];
        while ( gzread( gzipFile, buf, 4096 ) > 0 ) {
                fwrite( buf, sizeof( uchar ), 4096, pGunzippedFile );
        }
        gzclose( gzipFile );
        fclose( pGunzippedFile );


        // UNTAR !!!
        TAR *tarFile;

        char tarfilename[1024];
        strcpy( tarfilename, gunzippedName.toLocal8Bit() );

        if ( tar_open( &tarFile, tarfilename, NULL, O_RDONLY, 0, TAR_VERBOSE | TAR_GNU ) == -1 ) { 
		_ERRORLOG( QString( "[Drumkit::install] tar_open(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
		return;
        }

        char destDir[1024];
        strcpy( destDir, dataDir.toLocal8Bit() );
        if ( tar_extract_all( tarFile, destDir ) != 0 ) {
                _ERRORLOG( QString( "[Drumkit::install] tar_extract_all(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
        }

        if ( tar_close( tarFile ) != 0 ) {
                _ERRORLOG( QString( "[Drumkit::install] tar_close(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
        }
}
#endif

#endif

void Drumkit::save( const QString& sName, const QString& sAuthor, const QString& sInfo, const QString& sLicense )
{
	_INFOLOG( "Saving drumkit" );

	Drumkit *pDrumkitInfo = new Drumkit();
	pDrumkitInfo->setName( sName );
	pDrumkitInfo->setAuthor( sAuthor );
	pDrumkitInfo->setInfo( sInfo );
	pDrumkitInfo->setLicense( sLicense );

	Song *pSong = Hydrogen::get_instance()->getSong();
	InstrumentList *pSongInstrList = pSong->get_instrument_list();
	InstrumentList *pInstrumentList = new InstrumentList();

	for ( uint nInstrument = 0; nInstrument < pSongInstrList->get_size(); nInstrument++ ) {
		Instrument *pOldInstr = pSongInstrList->get( nInstrument );
		Instrument *pNewInstr = new Instrument( pOldInstr->get_id(), pOldInstr->get_name(), new ADSR( *( pOldInstr->get_adsr() ) ) );
		pNewInstr->set_gain( pOldInstr->get_gain() );
		pNewInstr->set_volume( pOldInstr->get_volume() );
		pNewInstr->set_pan_l( pOldInstr->get_pan_l() );
		pNewInstr->set_pan_r( pOldInstr->get_pan_r() );
		pNewInstr->set_muted( pOldInstr->is_muted() );
		pNewInstr->set_random_pitch_factor( pOldInstr->get_random_pitch_factor() );
		pNewInstr->set_mute_group( pOldInstr->get_mute_group() );
		pNewInstr->set_filter_active( pOldInstr->is_filter_active() );
		pNewInstr->set_filter_cutoff( pOldInstr->get_filter_cutoff() );
		pNewInstr->set_filter_resonance( pOldInstr->get_filter_resonance() );
		QString sInstrDrumkit = pOldInstr->get_drumkit_name();
		for ( unsigned nLayer = 0; nLayer < MAX_LAYERS; nLayer++ ) {
			InstrumentLayer *pOldLayer = pOldInstr->get_layer( nLayer );
			if ( pOldLayer ) {
				Sample *pSample = pOldLayer->get_sample();
				Sample *pNewSample = new Sample( 0, pSample->get_filename(), 0 );	// is not a real sample, it contains only the filename information
				InstrumentLayer *pLayer = new InstrumentLayer( pNewSample );
				pLayer->set_gain( pOldLayer->get_gain() );
				pLayer->set_pitch( pOldLayer->get_pitch() );
				pLayer->set_start_velocity( pOldLayer->get_start_velocity() );
				pLayer->set_end_velocity( pOldLayer->get_end_velocity() );
				pNewInstr->set_layer( pLayer, nLayer );
			} else {
				pNewInstr->set_layer( NULL, nLayer );
			}
		}
		pInstrumentList->add ( pNewInstr );
	}
	pDrumkitInfo->setInstrumentList( pInstrumentList );
	if( !pDrumkitInfo->save() ) {
		throw H2Exception( "Error saving the drumkit" );
	}
	delete pDrumkitInfo;
	pDrumkitInfo = NULL;

}




void Drumkit::removeDrumkit( const QString& sDrumkitName )
{
    QString path = Filesystem::usr_drumkits_dir() + "/" + sDrumkitName;
	_INFOLOG( "Removing drumkit: " + path );
    if( !Filesystem::rm_fr( path ) ) {
	    _ERRORLOG( "Unable to remove drumkit: " + path );
    }
}

};

