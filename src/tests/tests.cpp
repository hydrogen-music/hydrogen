
#include "hydrogen/logger.h"
#include "hydrogen/object.h"
#include "hydrogen/helpers/filesystem.h"

int main(int argc, char *argv[]) {

    int log_level = Logger::Debug | Logger::Info | Logger::Warning | Logger::Error;
    /* Logger */
    Logger* logger = Logger::bootstrap( log_level );
    /* Object */
    Object::bootstrap( logger, logger->should_log(Logger::Debug) );
    /* Filesystem */
    H2Core::Filesystem::bootstrap( logger );
    H2Core::Filesystem::info();

    sleep(1);
    delete logger;

    return EXIT_SUCCESS;
}
