
#ifndef H2C_LEGACY_H
#define H2C_LEGACY_H

#include <hydrogen/object.h>

namespace H2Core {

class Drumkit;

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
};

};

#endif  // H2C_LEGACY_H

/* vim: set softtabstop=4 expandtab: */
