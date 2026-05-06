#include <utils.h>

bool doDebug = false;

void debug(std::string msg) {
    if (doDebug) {
        std::cout << "# " << msg << std::endl;
    }
}