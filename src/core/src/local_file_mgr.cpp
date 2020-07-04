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
#include <hydrogen/basics/drumkit_component.h>
#include <hydrogen/basics/instrument.h>
#include <hydrogen/basics/instrument_list.h>
#include <hydrogen/basics/instrument_component.h>
#include <hydrogen/basics/instrument_layer.h>
#include <hydrogen/LocalFileMng.h>
#include <hydrogen/basics/note.h>
#include <hydrogen/basics/pattern.h>
#include <hydrogen/basics/pattern_list.h>
#include <hydrogen/basics/playlist.h>
#include <hydrogen/Preferences.h>
#include <hydrogen/timeline.h>
#include <hydrogen/basics/song.h>
#include <hydrogen/basics/drumkit.h>
#include <hydrogen/basics/sample.h>
#include <hydrogen/helpers/filesystem.h>
#include <hydrogen/automation_path_serializer.h>
#include <hydrogen/fx/Effects.h>

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <ctype.h>
#include <sys/stat.h>

#include <QDir>
//#include <QCoreApplication>
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
	QDomDocument doc = openXmlDocument( patternDir );

	QDomNode rootNode = doc.firstChildElement( "drumkit_pattern" );	// root element
	if (  rootNode.isNull() ) {
		ERRORLOG( "Error reading Pattern: Pattern_drumkit_infonode not found " + patternDir);
		return nullptr;
	}

	QString dk_name = LocalFileMng::readXmlString( rootNode,"drumkit_name", "" );
	if ( dk_name.isEmpty() ) {
		dk_name = LocalFileMng::readXmlString( rootNode,"pattern_for_drumkit", "" );
	}
	return dk_name;
}

/* New QtXml based methods */

QString LocalFileMng::processNode( QDomNode node, const QString& nodeName, bool bCanBeEmpty, bool bShouldExists )
{
	QDomElement element = node.firstChildElement( nodeName );

	if ( !node.isNull() && !element.isNull() ) {
		QString text = element.text();
		if( !text.isEmpty() ) {
			return text;
		} else {
			if ( !bCanBeEmpty ) {
				_WARNINGLOG( "node '" + nodeName + "' is empty" );
			}
		}
	} else {
		if (  bShouldExists ) {
			_WARNINGLOG( "node '" + nodeName + "' is not found" );
		}
	}
	return nullptr;
}

QString LocalFileMng::readXmlString( QDomNode node , const QString& nodeName, const QString& defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QString text = processNode( node, nodeName, bCanBeEmpty, bShouldExists );
	if ( text == nullptr ) {
		_WARNINGLOG( QString( "\tusing default value : '%1' for node '%2'" ).arg( defaultValue ).arg( nodeName ) );
		return defaultValue;
	} else {
		return text;
	}
}

float LocalFileMng::readXmlFloat( QDomNode node , const QString& nodeName, float defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QString text = processNode( node, nodeName, bCanBeEmpty, bShouldExists );
	if ( text == nullptr ) {
		_WARNINGLOG( QString( "\tusing default value : '%1' for node '%2'" ).arg( defaultValue ).arg( nodeName ));
		return defaultValue;
	} else {
		return QLocale::c().toFloat( text );
	}
}

int LocalFileMng::readXmlInt( QDomNode node , const QString& nodeName, int defaultValue, bool bCanBeEmpty, bool bShouldExists, bool tinyXmlCompatMode)
{
	QString text = processNode( node, nodeName, bCanBeEmpty, bShouldExists );
	if ( text == nullptr ) {
		_WARNINGLOG( QString( "\tusing default value : '%1' for node '%2'" ).arg( defaultValue ).arg( nodeName ));
		return defaultValue;
	} else {
		return QLocale::c().toInt( text );
	}
}

bool LocalFileMng::readXmlBool( QDomNode node , const QString& nodeName, bool defaultValue, bool bShouldExists, bool tinyXmlCompatMode)
{
	QString text = processNode( node, nodeName, bShouldExists, bShouldExists );
	if ( text == nullptr ) {
		_WARNINGLOG( QString( "\tusing default value : '%1' for node '%2'" ).arg( defaultValue ? "true" : "false" ).arg( nodeName ) );
		return defaultValue;
	} else {
		if ( text == "true") {
			return true;
		} else {
			return false;
		}
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

	if ( !file.open(QIODevice::ReadOnly) ) {
		return false;
	}

	QString line = file.readLine();
	file.close();
	if ( line.startsWith( "<?xml" )){
		return false;
	} else  {
		_WARNINGLOG( QString("File '%1' is being read in "
							 "TinyXML compatibility mode")
					 .arg(filename) );
		return true;
	}



}

QDomDocument LocalFileMng::openXmlDocument( const QString& filename )
{
	bool TinyXMLCompat = LocalFileMng::checkTinyXMLCompatMode( filename );

	QDomDocument doc;
	QFile file( filename );

	if ( !file.open(QIODevice::ReadOnly) ) {
		return QDomDocument();
	}

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


// Returns 0 on success, passes the TinyXml error code otherwise.
int SongWriter::writeSong( Song * pSong, const QString& filename )
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
	LocalFileMng::writeXmlString( songNode, "bpm", QString("%1").arg( pSong->__bpm ) );
	LocalFileMng::writeXmlString( songNode, "volume", QString("%1").arg( pSong->get_volume() ) );
	LocalFileMng::writeXmlString( songNode, "metronomeVolume", QString("%1").arg( pSong->get_metronome_volume() ) );
	LocalFileMng::writeXmlString( songNode, "name", pSong->__name );
	LocalFileMng::writeXmlString( songNode, "author", pSong->__author );
	LocalFileMng::writeXmlString( songNode, "notes", pSong->get_notes() );
	LocalFileMng::writeXmlString( songNode, "license", pSong->get_license() );
	LocalFileMng::writeXmlBool( songNode, "loopEnabled", pSong->is_loop_enabled() );
	LocalFileMng::writeXmlBool( songNode, "patternModeMode", Preferences::get_instance()->patternModePlaysSelected());
	
	LocalFileMng::writeXmlString( songNode, "playbackTrackFilename", QString("%1").arg( pSong->get_playback_track_filename() ) );
	LocalFileMng::writeXmlBool( songNode, "playbackTrackEnabled", pSong->get_playback_track_enabled() );
	LocalFileMng::writeXmlString( songNode, "playbackTrackVolume", QString("%1").arg( pSong->get_playback_track_volume() ) );

	
	if ( pSong->get_mode() == Song::SONG_MODE ) {
		LocalFileMng::writeXmlString( songNode, "mode", QString( "song" ) );
	} else {
		LocalFileMng::writeXmlString( songNode, "mode", QString( "pattern" ) );
	}

	LocalFileMng::writeXmlString( songNode, "humanize_time", QString("%1").arg( pSong->get_humanize_time_value() ) );
	LocalFileMng::writeXmlString( songNode, "humanize_velocity", QString("%1").arg( pSong->get_humanize_velocity_value() ) );
	LocalFileMng::writeXmlString( songNode, "swing_factor", QString("%1").arg( pSong->get_swing_factor() ) );

	// component List
	QDomNode componentListNode = doc.createElement( "componentList" );
	for (std::vector<DrumkitComponent*>::iterator it = pSong->get_components()->begin() ; it != pSong->get_components()->end(); ++it) {
		DrumkitComponent* pCompo = *it;

		QDomNode componentNode = doc.createElement( "drumkitComponent" );

		LocalFileMng::writeXmlString( componentNode, "id", QString("%1").arg( pCompo->get_id() ) );
		LocalFileMng::writeXmlString( componentNode, "name", pCompo->get_name() );
		LocalFileMng::writeXmlString( componentNode, "volume", QString("%1").arg( pCompo->get_volume() ) );

		componentListNode.appendChild( componentNode );
	}
	songNode.appendChild( componentListNode );

	// instrument list
	QDomNode instrumentListNode = doc.createElement( "instrumentList" );
	unsigned nInstrument = pSong->get_instrument_list()->size();

	// INSTRUMENT NODE
	for ( unsigned i = 0; i < nInstrument; i++ ) {
		Instrument * pInstr = pSong->get_instrument_list()->get( i );
		assert( pInstr );

		QDomNode instrumentNode = doc.createElement( "instrument" );

		LocalFileMng::writeXmlString( instrumentNode, "id", QString("%1").arg( pInstr->get_id() ) );
		LocalFileMng::writeXmlString( instrumentNode, "name", pInstr->get_name() );
		LocalFileMng::writeXmlString( instrumentNode, "drumkit", pInstr->get_drumkit_name() );
		LocalFileMng::writeXmlString( instrumentNode, "volume", QString("%1").arg( pInstr->get_volume() ) );
		LocalFileMng::writeXmlBool( instrumentNode, "isMuted", pInstr->is_muted() );
		LocalFileMng::writeXmlString( instrumentNode, "pan_L", QString("%1").arg( pInstr->get_pan_l() ) );
		LocalFileMng::writeXmlString( instrumentNode, "pan_R", QString("%1").arg( pInstr->get_pan_r() ) );
		LocalFileMng::writeXmlString( instrumentNode, "gain", QString("%1").arg( pInstr->get_gain() ) );
		LocalFileMng::writeXmlBool( instrumentNode, "applyVelocity", pInstr->get_apply_velocity() );

		LocalFileMng::writeXmlBool( instrumentNode, "filterActive", pInstr->is_filter_active() );
		LocalFileMng::writeXmlString( instrumentNode, "filterCutoff", QString("%1").arg( pInstr->get_filter_cutoff() ) );
		LocalFileMng::writeXmlString( instrumentNode, "filterResonance", QString("%1").arg( pInstr->get_filter_resonance() ) );

		LocalFileMng::writeXmlString( instrumentNode, "FX1Level", QString("%1").arg( pInstr->get_fx_level( 0 ) ) );
		LocalFileMng::writeXmlString( instrumentNode, "FX2Level", QString("%1").arg( pInstr->get_fx_level( 1 ) ) );
		LocalFileMng::writeXmlString( instrumentNode, "FX3Level", QString("%1").arg( pInstr->get_fx_level( 2 ) ) );
		LocalFileMng::writeXmlString( instrumentNode, "FX4Level", QString("%1").arg( pInstr->get_fx_level( 3 ) ) );

		assert( pInstr->get_adsr() );
		LocalFileMng::writeXmlString( instrumentNode, "Attack", QString("%1").arg( pInstr->get_adsr()->get_attack() ) );
		LocalFileMng::writeXmlString( instrumentNode, "Decay", QString("%1").arg( pInstr->get_adsr()->get_decay() ) );
		LocalFileMng::writeXmlString( instrumentNode, "Sustain", QString("%1").arg( pInstr->get_adsr()->get_sustain() ) );
		LocalFileMng::writeXmlString( instrumentNode, "Release", QString("%1").arg( pInstr->get_adsr()->get_release() ) );

		LocalFileMng::writeXmlString( instrumentNode, "randomPitchFactor", QString("%1").arg( pInstr->get_random_pitch_factor() ) );

		LocalFileMng::writeXmlString( instrumentNode, "muteGroup", QString("%1").arg( pInstr->get_mute_group() ) );
		LocalFileMng::writeXmlBool( instrumentNode, "isStopNote", pInstr->is_stop_notes() );
		switch ( pInstr->sample_selection_alg() ) {
			case Instrument::VELOCITY:
				LocalFileMng::writeXmlString( instrumentNode, "sampleSelectionAlgo", "VELOCITY" );
				break;
			case Instrument::RANDOM:
				LocalFileMng::writeXmlString( instrumentNode, "sampleSelectionAlgo", "RANDOM" );
				break;
			case Instrument::ROUND_ROBIN:
				LocalFileMng::writeXmlString( instrumentNode, "sampleSelectionAlgo", "ROUND_ROBIN" );
				break;
		}

		LocalFileMng::writeXmlString( instrumentNode, "midiOutChannel", QString("%1").arg( pInstr->get_midi_out_channel() ) );
		LocalFileMng::writeXmlString( instrumentNode, "midiOutNote", QString("%1").arg( pInstr->get_midi_out_note() ) );
		LocalFileMng::writeXmlString( instrumentNode, "isHihat", QString("%1").arg( pInstr->get_hihat_grp() ) );
		LocalFileMng::writeXmlString( instrumentNode, "lower_cc", QString("%1").arg( pInstr->get_lower_cc() ) );
		LocalFileMng::writeXmlString( instrumentNode, "higher_cc", QString("%1").arg( pInstr->get_higher_cc() ) );

		for (std::vector<InstrumentComponent*>::iterator it = pInstr->get_components()->begin() ; it != pInstr->get_components()->end(); ++it) {
			InstrumentComponent* pComponent = *it;

			QDomNode componentNode = doc.createElement( "instrumentComponent" );

			LocalFileMng::writeXmlString( componentNode, "component_id", QString("%1").arg( pComponent->get_drumkit_componentID() ) );
			LocalFileMng::writeXmlString( componentNode, "gain", QString("%1").arg( pComponent->get_gain() ) );

			for ( unsigned nLayer = 0; nLayer < InstrumentComponent::getMaxLayers(); nLayer++ ) {
				InstrumentLayer *pLayer = pComponent->get_layer( nLayer );
				if ( pLayer == nullptr ) {
					continue;
				}
				
				Sample *pSample = pLayer->get_sample();
				if ( pSample == nullptr ) {
					continue;
				}

				bool sIsModified = pSample->get_is_modified();
				Sample::Loops lo = pSample->get_loops();
				Sample::Rubberband ro = pSample->get_rubberband();
				QString sMode = pSample->get_loop_mode_string();

				
				QDomNode layerNode = doc.createElement( "layer" );
				LocalFileMng::writeXmlString( layerNode, "filename", Filesystem::prepare_sample_path( pSample->get_filepath() ) );
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
					LocalFileMng::writeXmlString( volumeNode, "volume-position", QString("%1").arg( velocity->at(y)->frame ) );
					LocalFileMng::writeXmlString( volumeNode, "volume-value", QString("%1").arg( velocity->at(y)->value ) );
					layerNode.appendChild( volumeNode );
				}

				Sample::PanEnvelope* pan = pSample->get_pan_envelope();
				for (int y = 0; y < pan->size(); y++){
					QDomNode panNode = doc.createElement( "pan" );
					LocalFileMng::writeXmlString( panNode, "pan-position", QString("%1").arg( pan->at(y)->frame ) );
					LocalFileMng::writeXmlString( panNode, "pan-value", QString("%1").arg( pan->at(y)->value ) );
					layerNode.appendChild( panNode );
				}

				componentNode.appendChild( layerNode );
			}
			instrumentNode.appendChild( componentNode );
		}

		instrumentListNode.appendChild( instrumentNode );
	}
	songNode.appendChild( instrumentListNode );


	// pattern list
	QDomNode patternListNode = doc.createElement( "patternList" );

	unsigned nPatterns = pSong->get_pattern_list()->size();
	for ( unsigned i = 0; i < nPatterns; i++ ) {
		const Pattern *pPattern = pSong->get_pattern_list()->get( i );

		// pattern
		QDomNode patternNode = doc.createElement( "pattern" );
		LocalFileMng::writeXmlString( patternNode, "name", pPattern->get_name() );
		LocalFileMng::writeXmlString( patternNode, "category", pPattern->get_category() );
		LocalFileMng::writeXmlString( patternNode, "size", QString("%1").arg( pPattern->get_length() ) );
		LocalFileMng::writeXmlString( patternNode, "info", pPattern->get_info() );

		QDomNode noteListNode = doc.createElement( "noteList" );
		const Pattern::notes_t* notes = pPattern->get_notes();
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
			LocalFileMng::writeXmlString( noteNode, "probability", QString("%1").arg( pNote->get_probability() ) );

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
		const Pattern *pat = pSong->get_pattern_list()->get( i );

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

	unsigned nPatternGroups = pSong->get_pattern_group_vector()->size();
	for ( unsigned i = 0; i < nPatternGroups; i++ ) {
		QDomNode groupNode = doc.createElement( "group" );

		PatternList *pList = ( *pSong->get_pattern_group_vector() )[i];
		for ( unsigned j = 0; j < pList->size(); j++ ) {
			const Pattern *pPattern = pList->get( j );
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
		H2FX *pH2FX = Effects::get_instance()->getLadspaFX( nFX );
		
		
		if ( pH2FX->isLadspaFX() ) {
			LadspaFX* pFX = pH2FX->isLadspaFX();
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
	Timeline * pTimeline = Hydrogen::get_instance()->getTimeline();

	QDomNode bpmTimeLine = doc.createElement( "BPMTimeLine" );

	if(pTimeline->m_timelinevector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(pTimeline->m_timelinevector.size()); t++){
			QDomNode newBPMNode = doc.createElement( "newBPM" );
			LocalFileMng::writeXmlString( newBPMNode, "BAR",QString("%1").arg( pTimeline->m_timelinevector[t].m_htimelinebeat ));
			LocalFileMng::writeXmlString( newBPMNode, "BPM", QString("%1").arg( pTimeline->m_timelinevector[t].m_htimelinebpm  ) );
			bpmTimeLine.appendChild( newBPMNode );
		}
	}
	songNode.appendChild( bpmTimeLine );

	//time line tag
	QDomNode timeLineTag = doc.createElement( "timeLineTag" );
	if(pTimeline->m_timelinetagvector.size() >= 1 ){
		for ( int t = 0; t < static_cast<int>(pTimeline->m_timelinetagvector.size()); t++){
			QDomNode newTAGNode = doc.createElement( "newTAG" );
			LocalFileMng::writeXmlString( newTAGNode, "BAR",QString("%1").arg( pTimeline->m_timelinetagvector[t].m_htimelinetagbeat ));
			LocalFileMng::writeXmlString( newTAGNode, "TAG", QString("%1").arg( pTimeline->m_timelinetagvector[t].m_htimelinetag  ) );
			timeLineTag.appendChild( newTAGNode );
		}
	}
	songNode.appendChild( timeLineTag );

	// Automation Paths
	QDomNode automationPathsTag = doc.createElement( "automationPaths" );
	AutomationPath *pPath = pSong->get_velocity_automation_path();
	if (pPath) {
		QDomElement pathNode = doc.createElement("path");
		pathNode.setAttribute("adjust", "velocity");

		AutomationPathSerializer serializer;
		serializer.write_automation_path(pathNode, *pPath);

		automationPathsTag.appendChild(pathNode);
	}
	songNode.appendChild( automationPathsTag );

	QFile file(filename);
	if ( !file.open(QIODevice::WriteOnly) ) {
		rv = 1;
	}

	QTextStream TextStream( &file );
	doc.save( TextStream, 1 );

	if( file.size() == 0) {
		rv = 1;
	}

	file.close();

	if( rv ) {
		WARNINGLOG("File save reported an error.");
	} else {
		pSong->set_is_modified( false );
		INFOLOG("Save was successful.");
	}

	pSong->set_filename( filename );

	return rv;
}

};

