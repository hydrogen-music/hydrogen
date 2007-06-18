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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <string>
#include <vector>
using std::string;
using std::vector;

#include <hydrogen/Globals.h>
#include <hydrogen/Object.h>

// forward declaration
class TiXmlNode;


namespace H2Core {

const float FALLOFF_SLOW = 	1.08f;
const float FALLOFF_NORMAL=	1.1f;
const float FALLOFF_FAST =	1.5f;


class WindowProperties : public Object
{
	public:
		int x;
		int y;
		int width;
		int height;
		bool visible;

		WindowProperties();
		~WindowProperties();
};



class H2RGBColor : public Object
{
	public:
		H2RGBColor(int r = -1, int g = -1, int b = -1);
		H2RGBColor( const std::string& sColor );
		~H2RGBColor();

		std::string toStringFmt();

		int getRed() const {	return m_red;	}
		int getGreen() const {	return m_green;	}
		int getBlue() const {	return m_blue;	}

	private:
		int m_red;
		int m_green;
		int m_blue;

};


///
/// Colors for hydogen
///
class UIStyle : public Object
{
	public:
		UIStyle();
		H2RGBColor m_songEditor_backgroundColor;
		H2RGBColor m_songEditor_alternateRowColor;
		H2RGBColor m_songEditor_selectedRowColor;
		H2RGBColor m_songEditor_lineColor;
		H2RGBColor m_songEditor_textColor;
		H2RGBColor m_songEditor_pattern1Color;

		H2RGBColor m_patternEditor_backgroundColor;
		H2RGBColor m_patternEditor_alternateRowColor;
		H2RGBColor m_patternEditor_selectedRowColor;
		H2RGBColor m_patternEditor_textColor;
		H2RGBColor m_patternEditor_noteColor;
		H2RGBColor m_patternEditor_lineColor;
		H2RGBColor m_patternEditor_line1Color;
		H2RGBColor m_patternEditor_line2Color;
		H2RGBColor m_patternEditor_line3Color;
		H2RGBColor m_patternEditor_line4Color;
		H2RGBColor m_patternEditor_line5Color;
};



/// Manager for User Preferences File (singleton)
class Preferences : public Object
{
	public:
		enum {
			USE_JACK_TRANSPORT,
			NO_JACK_TRANSPORT
		};

		std::string m_sPreferencesFilename;
		std::string m_sPreferencesDirectory;

		bool m_bFollowPlayhead;

		//___ audio engine properties ___
		string m_sAudioDriver;		///< Audio Driver
		bool m_bUseMetronome;		///< Use metronome?
		float m_fMetronomeVolume;	///< Metronome volume FIXME: remove this volume!!
		unsigned m_nMaxNotes;		///< max notes
		unsigned m_nBufferSize;		///< Audio buffer size
		unsigned m_nSampleRate;		///< Audio sample rate

		//___ oss driver properties ___
		string m_sOSSDevice;		///< Device used for output

		//___ MIDI Driver properties
		std::string m_sMidiDriver;
		std::string m_sMidiPortName;
		int m_nMidiChannelFilter;
		bool m_bMidiNoteOffIgnore;
		bool m_bUseMidiTransport;

		//___  alsa audio driver properties ___
		string m_sAlsaAudioDevice;

		//___  jack driver properties ___
		string m_sJackPortName1;
		string m_sJackPortName2;
		bool m_bJackTransportMode;
		bool m_bJackConnectDefaults;


		/// Returns an instance of PreferencesMng class
		static Preferences* getInstance();

		~Preferences();

		/// Load the preferences file
		void loadPreferences( bool bGlobal );

		/// Save the preferences file
		void savePreferences();

		string getDemoPath() {	return demoPath;	}
		const std::string& getDataDirectory() {	return m_sDataDirectory;	}


		// General
		void setRestoreLastSongEnabled( bool restore ) {	restoreLastSong = restore;	}
		bool isRestoreLastSongEnabled() {	return restoreLastSong;	}

		void setLastSongFilename( const std::string& filename ) {	lastSongFilename = filename;	}
		string getLastSongFilename() {	return lastSongFilename;	}

		void setHearNewNotes( bool value ) {	hearNewNotes = value;	}
		bool getHearNewNotes() {	return hearNewNotes;	}

		void setRecordEvents( bool value ) {	recordEvents = value;	}
		bool getRecordEvents() {	return recordEvents;	}

		void setQuantizeEvents( bool value ) {	quantizeEvents = value;	}
		bool getQuantizeEvents() {	return quantizeEvents;	}

		vector<string> getRecentFiles() {	return m_recentFiles;	}
		void setRecentFiles( vector<string> recentFiles );

		vector<string> getLadspaPath() {	return m_ladspaPathVect;	}
		void setLadspaPath( vector<string> pathVect ) {	m_ladspaPathVect = pathVect;	}

		string getLastNews() {	return m_sLastNews;	}
		void setLastNews( const std::string& sNews ) {	m_sLastNews = sNews;	}


		// GUI Properties
		string getQTStyle() {	return m_sQTStyle;	}
		void setQTStyle( const std::string& sStyle ) {	m_sQTStyle = sStyle;	}


		string getApplicationFontFamily() {	return applicationFontFamily;	}
		void setApplicationFontFamily( const std::string& family ) {	applicationFontFamily = family;	}

		int getApplicationFontPointSize() {	return applicationFontPointSize;	}
		void setApplicationFontPointSize(int size) {	applicationFontPointSize = size;	}

		string getMixerFontFamily() {	return mixerFontFamily;	}
		void setMixerFontFamily( const std::string& family ) {	mixerFontFamily = family;	}
		int getMixerFontPointSize() {	return mixerFontPointSize;	}
		void setMixerFontPointSize(int size) {	mixerFontPointSize = size;	}
		float getMixerFalloffSpeed() {	return mixerFalloffSpeed;	}
		void setMixerFalloffSpeed(float value) {	mixerFalloffSpeed = value;	}
		bool showInstrumentPeaks() {	return m_bShowInstrumentPeaks;	}
		void setInstrumentPeaks( bool value ) {	m_bShowInstrumentPeaks = value;	}

		int getPatternEditorGridResolution() {	return m_nPatternEditorGridResolution;	}
		void setPatternEditorGridResolution( int value ) {	m_nPatternEditorGridResolution = value;	}

		bool isPatternEditorUsingTriplets() {	return m_bPatternEditorUsingTriplets;	}
		void setPatternEditorUsingTriplets( bool value ) {	m_bPatternEditorUsingTriplets = value;	}

		bool isFXTabVisible() {	return m_bIsFXTabVisible;	}
		void setFXTabVisible( bool value ) {	m_bIsFXTabVisible = value;	}

		unsigned getPatternEditorGridHeight() {	return m_nPatternEditorGridHeight;	}
		void setPatternEditorGridHeight(unsigned value) {	m_nPatternEditorGridHeight = value;	}

		unsigned getPatternEditorGridWidth() {	return m_nPatternEditorGridWidth;	}
		void setPatternEditorGridWidth(unsigned value) {	m_nPatternEditorGridWidth = value;	}

		WindowProperties getMainFormProperties() {	return mainFormProperties;	}
		void setMainFormProperties( const WindowProperties& prop ) {	mainFormProperties = prop;	}

		WindowProperties getMixerProperties() {	return mixerProperties;	}
		void setMixerProperties( const WindowProperties& prop ) {	mixerProperties = prop;	}

		WindowProperties getPatternEditorProperties() {	return patternEditorProperties;	}
		void setPatternEditorProperties( const WindowProperties& prop ) {	patternEditorProperties = prop;	}

		WindowProperties getSongEditorProperties() {	return songEditorProperties;	}
		void setSongEditorProperties( const WindowProperties& prop ) {	songEditorProperties = prop;	}

		WindowProperties getDrumkitManagerProperties() {	return drumkitManagerProperties;	}
		void setDrumkitManagerProperties( const WindowProperties& prop ) {	drumkitManagerProperties = prop;	}

		WindowProperties getAudioEngineInfoProperties() {	return audioEngineInfoProperties;	}
		void setAudioEngineInfoProperties( const WindowProperties& prop ) {	audioEngineInfoProperties = prop;	}

		WindowProperties getLadspaProperties(unsigned nFX) {	return m_ladspaProperties[nFX];	}
		void setLadspaProperties( unsigned nFX, const WindowProperties& prop ) {	m_ladspaProperties[nFX] = prop;	}

		UIStyle* getDefaultUIStyle(){	return m_pDefaultUIStyle;	}

	private:
		static Preferences *instance;

		std::string m_sDataDirectory;


		/** directory of demo songs */
		string demoPath;

		//___ General properties ___
		bool restoreLastSong;		///< Restore last song?
		string lastSongFilename;	///< Last song used
		bool hearNewNotes;
		vector<string> m_recentFiles;
		vector<string> m_ladspaPathVect;
		bool quantizeEvents;
		bool recordEvents;
		string m_sLastNews;


		//___ GUI properties ___
		string m_sQTStyle;

		string applicationFontFamily;
		int applicationFontPointSize;
		string mixerFontFamily;
		int mixerFontPointSize;
		float mixerFalloffSpeed;
		int m_nPatternEditorGridResolution;
		bool m_bPatternEditorUsingTriplets;
		bool m_bShowInstrumentPeaks;
		bool m_bIsFXTabVisible;
		unsigned m_nPatternEditorGridHeight;
		unsigned m_nPatternEditorGridWidth;
		WindowProperties mainFormProperties;
		WindowProperties mixerProperties;
		WindowProperties patternEditorProperties;
		WindowProperties songEditorProperties;
		WindowProperties drumkitManagerProperties;
		WindowProperties audioEngineInfoProperties;
		WindowProperties m_ladspaProperties[MAX_FX];

		UIStyle*  m_pDefaultUIStyle;


		Preferences();

		/// Create preferences directory
		void createPreferencesDirectory();

		/// Create data directory
		void createDataDirectory();

		WindowProperties readWindowProperties( TiXmlNode *parent, const std::string& windowName, WindowProperties defaultProp );
		void writeWindowProperties( TiXmlNode& parent, const std::string& windowName, const WindowProperties& prop );

		void writeUIStyle( TiXmlNode& parent );
		void readUIStyle( TiXmlNode& parent );
};

};

#endif

