#include <utils.h>

#if DEBUG_LOG
void debug_impl(const std::string& msg) {
    std::cout << "# " << msg << std::endl;
}
#endif