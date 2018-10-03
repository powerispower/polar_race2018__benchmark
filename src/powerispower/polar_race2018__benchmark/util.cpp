#include "powerispower/polar_race2018__benchmark/util.h"

#include <algorithm>

namespace powerispower {
namespace polar_race2018__benchmark {

char* get_cmd_option(
        char* begin[]
        , char* end[]
        , const std::string& option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end) {
        return *it;
    }
    return nullptr;
}

bool cmd_option_exist(
        char* begin[]
        , char* end[]
        , const std::string& option) {
    return std::find(begin, end, option) != end;
}

} // namespace polar_race2018__benchmark
} // namespace powerispower
