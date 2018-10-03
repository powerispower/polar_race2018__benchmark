#pragma once

#include <string>

namespace powerispower {
namespace polar_race2018__benchmark {

char* get_cmd_option(
        char* begin[]
        , char* end[]
        , const std::string& option);

bool cmd_option_exist(
        char* begin[]
        , char* end[]
        , const std::string& option);

} // namespace polar_race2018__benchmark
} // namespace powerispower
