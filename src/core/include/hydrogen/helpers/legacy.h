
#ifndef H2C_LEGACY_H
#define H2C_LEGACY_H

#include <hydrogen/object.h>

namespace H2Core {

class Drumkit;
class Playlist;
class Pattern;
class InstrumentList;

/**
 * Legacy is a container for legacy code which should be once removed
 *
 * \ingroup docCore
 */
class Legacy : public H2Core::Object {
	public:
		/** \return #m_sClassName*/
		static const char* className() { return m_sClassName; }
		/**
		 * load drumkit information from a file
		 * \param dk_path is a path to an xml file
		 * \return a Drumkit on success, 0 otherwise
		 */
		static Drumkit* load_drumkit( const QString& dk_path );
		/**
		 * load pattern from a file
		 * \param pattern_path is a path to an xml file
		 * \param instrList
		 * \return a Pattern on success, 0 otherwise
		 */
		static Pattern* load_drumkit_pattern( const QString& pattern_path, InstrumentList* instrList );
		/**
		 * load playlist from a file
		 * \param pl the playlist to feed
		 * \param pl_path is a path to an xml file
		 * \return a Playlist on success, 0 otherwise
		 */
		static Playlist* load_playlist( Playlist* pl, const QString& pl_path );
	private:
		/** Contains the name of the class.
		 *
		 * This variable allows from more informative log messages
		 * with the name of the class the message is generated in
		 * being displayed as well. Queried using className().*/
		static const char* m_sClassName;
};

};

#endif  // H2C_LEGACY_H

/* vim: set softtabstop=4 noexpandtab: */
