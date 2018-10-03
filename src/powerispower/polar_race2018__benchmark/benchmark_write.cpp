#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

// #include <include/engine_race.h>

#include "powerispower/polar_race2018__benchmark/util.h"

int main(int argc, char* argv[]) {
    using namespace ::powerispower::polar_race2018__benchmark;

    static const std::string& help_message =
        "Usage : ./benchmark_write"
        "\n --input_data (input file)"
        "\n --thread_num (benchmark thread num) default: 64"
        "\n --data_num_per_thread (xxx) default: 1e6";

    if (cmd_option_exist(argv, argv + argc, "--help")) {
        std::cerr << help_message << std::endl;
        return 0;
    }

    // parse input_data
    std::string input_filename;
    {
        char* option = get_cmd_option(argv, argv + argc, "--input_data");
        if (option == nullptr) {
            std::cerr << help_message << std::endl;
            return -1;
        }
        input_filename = option;
    }

    // parse thread_num
    int thread_num = 0;
    {
        char* option = get_cmd_option(argv, argv + argc, "--thread_num");
        if (option == nullptr) {
            thread_num = 64;
        } else {
            thread_num = std::stoi(option);
        }
    }

    // parse data_num_per_thread
    std::int64_t data_num_per_thread = 0;
    {
        char* option = get_cmd_option(argv, argv + argc, "--data_num_per_thread");
        if (option == nullptr) {
            data_num_per_thread = 1000000;
        } else {
            data_num_per_thread = std::stoi(option);
        }
    }

    // check file size
    {
        std::ifstream input_file(input_filename
            , std::ifstream::ate | std::ifstream::binary);
        if (input_file.tellg() < thread_num * data_num_per_thread * 4008) {
            std::cerr << "input_file.tellg(" << input_file.tellg() << ")"
                << " < (thread_num * data_num_per_thread * 4008)("
                << thread_num * data_num_per_thread * 4008 << ")"
                << std::endl;
            return -1;
        }
    }

    std::vector<std::thread> workers;
    for (int i = 0; i < thread_num; i++) {
        std::int64_t data_num = data_num_per_thread;
        std::int64_t offset = i * data_num * 4008;
        workers.push_back(std::thread(
            [&input_filename, offset, data_num] () {
                std::ifstream in(input_filename);
                in.seekg(offset);
                std::string buffer;
                buffer.resize(4008);
                for (int i = 0; i < data_num; i++) {
                    if (!in.read(&buffer[0], buffer.size())) {
                        std::cerr << "in.read fail"
                            << std::endl;
                        return;
                    }
                }
            }
        ));
    }

    for (auto& worker : workers) {
        worker.join();
    }

    return 0;
}
