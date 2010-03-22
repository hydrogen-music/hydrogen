
#include "config.h"
#include "hydrogen/version.h"

static const std::string extra_version = HYDROGEN_SVN_VERSION;

static const std::string version = HYDROGEN_VERSION_FULL;

std::string get_version() { return version; }
