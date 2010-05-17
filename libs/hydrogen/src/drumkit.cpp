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

#include <hydrogen/drumkit.h>

#include <hydrogen/adsr.h>
#include <hydrogen/sample.h>
#include <hydrogen/instrument.h>

#include <hydrogen/filesystem.h>
#include <hydrogen/xml_helper.h>

#include <hydrogen/hydrogen.h>
#include <hydrogen/h2_exception.h>

#include <fcntl.h>
#include <errno.h>

#ifdef H2CORE_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#else
	#ifndef WIN32
		#include <zlib.h>
		#include <libtar.h>
	#endif
#endif

namespace H2Core
{
    
const char* Drumkit::__class_name = "Drumkit";

Drumkit::Drumkit() : Object( __class_name ), __instruments( 0 ) { }

Drumkit::~Drumkit() { if(__instruments) delete __instruments; }

Drumkit* Drumkit::load( const QString& dk_path ) {
    INFOLOG( QString("Load drumkit %1").arg(dk_path) );
    if( !Filesystem::drumkit_valid( dk_path ) ) {
        ERRORLOG( QString("%1 is not valid drumkit").arg(dk_path) );
        return 0;
    }
    XMLDoc doc;
    if( !doc.read( Filesystem::drumkit_file(dk_path) ) ) return 0;
    // TODO XML VALIDATION !!!!!!!!!!
    XMLNode root = doc.firstChildElement( "drumkit_info" );
    if ( root.isNull() ) {
        ERRORLOG( "drumkit_info node not found" );
        return 0;
    }
    Drumkit* drumkit = Drumkit::load_from( &root );
    drumkit->setPath( dk_path );
    return drumkit;
}

Drumkit* Drumkit::load_from( XMLNode* node ) {
    QString drumkit_name = node->read_string( "name", "", false, false );
    if ( drumkit_name.isEmpty() ) {
        ERRORLOG( "Drumkit has no name, abort" );
        return 0;
    }
    Drumkit *drumkit = new Drumkit();
    drumkit->setName( drumkit_name );
    drumkit->setAuthor( node->read_string( "author", "undefined author" ) );
    drumkit->setInfo( node->read_string( "info", "defaultInfo" ) );
    drumkit->setLicense( node->read_string( "license", "undefined license" ) );
    XMLNode instruments_node = node->firstChildElement( "instrumentList" );
    if ( instruments_node.isNull() ) {
        WARNINGLOG( "instrumentList node not found" );
        drumkit->setInstrumentList( new InstrumentList() );
    } else {
        InstrumentList *instruments = InstrumentList::load_from( &instruments_node );
        // TODO should disepear
        for(int i=0; i<instruments->get_size(); i++) instruments->get(i)->set_drumkit_name( drumkit_name );
        drumkit->setInstrumentList( instruments );
    }
    drumkit->dump();
    return drumkit;
}

bool Drumkit::save( ) {
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/" + getName();
    INFOLOG( QString("save drumkit to %1").arg(dk_dir) );
    if( !Filesystem::mkdir( dk_dir) ) return false;
    XMLDoc doc;
    QDomProcessingInstruction header = doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild( header );
    XMLNode root = doc.createElement( "drumkit_info" );
    root.write_string( "name", getName() );
    root.write_string( "author", getAuthor() );
    root.write_string( "info", getInfo() );
    root.write_string( "license", getLicense() );
    QVector<QString> tempVector(MAX_LAYERS);
    XMLNode instruments_node = doc.createElement( "instrumentList" );
    for ( int i = 0; i < getInstrumentList()->get_size(); i++ ) {
        Instrument *instr = getInstrumentList()->get( i );
        for ( int n = 0; n < MAX_LAYERS; n++ ) {
            InstrumentLayer *layer = instr->get_layer( n );
            if ( layer ) {
                Sample *sample = layer->get_sample();
                QString src = sample->get_filename();
                QString dst = src;
                /*
                 * Till rev. 743, the samples got copied into the
                 * root of the drumkit folder.
                 * Now the sample gets only copied to the folder
                 * if it doesn't reside in a subfolder of the drumkit dir.
                 * */
                if( src.startsWith( dk_dir ) ){
                    INFOLOG("sample is already in drumkit dir");
                    tempVector[ n ] = dst.remove( dk_dir + "/" );
                } else {
                    int p = dst.lastIndexOf( '/' );
                    dst = dst.mid( p + 1, dst.size() - p - 1 );
                    dst = dk_dir + "/" + dst;
                    Filesystem::file_copy( src, dst );
                    tempVector[ n ] = dst.remove( dk_dir + "/" );
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
        for ( unsigned n = 0; n < MAX_LAYERS; n++ ) {
            InstrumentLayer *layer = instr->get_layer( n );
            if ( layer == 0 ) continue;
            XMLNode layer_node = doc.createElement( "layer" );
            layer_node.write_string( "filename", tempVector[ n ] );
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

bool Drumkit::save( const QString& name, const QString& author, const QString& info, const QString& license )
{
    _INFOLOG( "Saving drumkit" );
    Drumkit *drumkit = new Drumkit();
    drumkit->setName( name );
    drumkit->setAuthor( author );
    drumkit->setInfo( info );
    drumkit->setLicense( license );
    // TODO this is not a real copy constructor, only the sample filename is copied !!!!
	drumkit->setInstrumentList( new InstrumentList( Hydrogen::get_instance()->getSong()->get_instrument_list() ) );
	bool ret = drumkit->save();
	delete drumkit;
	drumkit = 0;
    return ret;
}

bool Drumkit::removeDrumkit( const QString& sDrumkitName ) {
    QString path = Filesystem::usr_drumkits_dir() + "/" + sDrumkitName;
    _INFOLOG( QString("Removing drumkit: %1").arg(path) );
    if( !Filesystem::drumkit_valid( path ) ) {
        ERRORLOG( QString("%1 is not valid drumkit").arg(path));
        return false;
    }
    if( !Filesystem::rm_fr( path ) ) {
        _ERRORLOG( QString("Unable to remove drumkit: %1").arg(path) );
        return false;
    }
    return true;
}

void Drumkit::dump() {
	DEBUGLOG( "Drumkit dump" );
	DEBUGLOG( " |- Path = " + __path );
	DEBUGLOG( " |- Name = " + __name );
	DEBUGLOG( " |- Author = " + __author );
	DEBUGLOG( " |- Info = " + __info );
	DEBUGLOG( " |- Instrument list" );
	for ( int i=0; i<__instruments->get_size(); i++ ) {
		Instrument *instr = __instruments->get( i );
		DEBUGLOG( QString("  |- (%1 of %2) Name = %3")
			 .arg( i )
			 .arg( __instruments->get_size() )
			 .arg( instr->get_name() )
			);
		for ( int j=0; j<MAX_LAYERS; j++ ) {
			InstrumentLayer *layer = instr->get_layer( j );
			if ( layer ) {
				Sample *sample = layer->get_sample();
				if ( sample ) {
					DEBUGLOG( "   |- " + sample->get_filename() );
				} else {
					DEBUGLOG( "   |- NULL sample" );
				}
			} /*else {
				DEBUGLOG( "   |- NULL Layer" );
			}*/

		}
	}
}

#ifdef H2CORE_HAVE_LIBARCHIVE
bool Drumkit::install( const QString& filename ) {
    _INFOLOG( QString("drumkit = ").arg(filename) );
    
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
        return false;
    }
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/";
    while ( (r = archive_read_next_header(drumkitFile, &entry)) != ARCHIVE_EOF) {
        if (r != ARCHIVE_OK) {
            _ERRORLOG( QString( "Error reading drumkit file: %1").arg(archive_error_string(drumkitFile)));
            break;
        }
        // insert data directory prefix
        QString np = dk_dir + archive_entry_pathname(entry);
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
    return true;
}
#else //use libtar
#ifndef WIN32
bool Drumkit::install( const QString& filename ) {
    _INFOLOG( "drumkit = " + filename );
    // GUNZIP !!!
    QString gunzippedName = filename.left( filename.indexOf( "." ) );
    gunzippedName += ".tar";
    FILE *pGunzippedFile = fopen( gunzippedName.toLocal8Bit(), "wb" );
    gzFile gzipFile = gzopen( filename.toLocal8Bit(), "rb" );
    if ( !gzipFile ) {	
        _ERRORLOG( QString( "Error reading drumkit file: %1").arg(filename));
        return false;
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
        _ERRORLOG( QString( "tar_open(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
        return false;
    }
    char destDir[1024];
    QString dk_dir = Filesystem::usr_drumkits_dir() + "/";
    strcpy( destDir, dataDir.toLocal8Bit() );
    if ( tar_extract_all( tarFile, destDir ) != 0 ) {
        _ERRORLOG( QString( "tar_extract_all(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
    }
    if ( tar_close( tarFile ) != 0 ) {
        _ERRORLOG( QString( "tar_close(): %1" ).arg( QString::fromLocal8Bit(strerror(errno)) ) );
    }
    return true;
}
#endif

#endif

};

