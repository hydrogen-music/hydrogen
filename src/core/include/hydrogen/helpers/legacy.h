
#ifndef H2C_LEGACY_H
#define H2C_LEGACY_H

#include <hydrogen/object.h>

namespace H2Core {

class Drumkit;
class Pattern;
class InstrumentList;

/**
 * Legacy is a container for legacy code which should be once removed
 */
class Legacy : public H2Core::Object {
		H2_OBJECT
	public:
		/**
		 * load drumkit information from a file
		 * \param dk_path is a path to an xml file
		 * \return a Drumkit on success, 0 otherwise
		 */
		static Drumkit* load_drumkit( const QString& dk_path );
		/**
		 * load pattern from a file
		 * \param pattern_path is a path to an xml file
		 * \return a Pattern on success, 0 otherwise
		 */
		static Pattern* load_drumkit_pattern( const QString& pattern_path, InstrumentList* instrList );
};

};

#endif  // H2C_LEGACY_H

/* vim: set softtabstop=4 noexpandtab: */
