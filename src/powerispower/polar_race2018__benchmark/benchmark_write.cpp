#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <engine_race/engine_race.h>

#include "powerispower/polar_race2018__benchmark/util.h"

std::string FLAGS_input_data;
std::string FLAGS_db_name;
std::int64_t FLAGS_thread_num = 64;
std::int64_t FLAGS_data_num_per_thread = 1000000;
std::int64_t FLAGS_key_size_byte = 8;
std::int64_t FLAGS_value_size_byte = 4000;

int main(int argc, char* argv[]) {
    using namespace ::powerispower::polar_race2018__benchmark;

    static const std::string& help_message =
        "Usage : ./benchmark_write"
        "\n\t--input_data (input file)"
        "\n\t--db_name (database name)"
        "\n\t--thread_num (benchmark thread num) default: 64"
        "\n\t--data_num_per_thread (xxx) default: 1e6";

    if (cmd_option_exist(argv, argv + argc, "--help")) {
        std::cerr << help_message << std::endl;
        return 0;
    }

    // parse FLAGS_input_data
    {
        char* option = get_cmd_option(argv, argv + argc, "--input_data");
        if (option == nullptr) {
            std::cerr << help_message << std::endl;
            return -1;
        }
        FLAGS_input_data = option;
    }

    // parse FLAGS_db_name
    {
        char* option = get_cmd_option(argv, argv + argc, "--db_name");
        if (option == nullptr) {
            std::cerr << help_message << std::endl;
            return -1;
        }
        FLAGS_db_name = option;
    }

    // parse FLAGS_thread_num
    {
        char* option = get_cmd_option(argv, argv + argc, "--thread_num");
        if (option != nullptr) {
            FLAGS_thread_num = std::stoi(option);
        }
    }

    // parse FLAGS_data_num_per_thread
    std::int64_t data_num_per_thread = 0;
    {
        char* option = get_cmd_option(argv, argv + argc, "--data_num_per_thread");
        if (option != nullptr) {
            FLAGS_data_num_per_thread = std::stoi(option);
        }
    }

    // check file size
    {
        std::ifstream input_file(FLAGS_input_data
            , std::ifstream::ate | std::ifstream::binary);
        std::int64_t required_file_size =
            FLAGS_thread_num * data_num_per_thread * (FLAGS_key_size_byte + FLAGS_value_size_byte);
        if (input_file.tellg() < required_file_size) {
            std::cerr << "input_file.tellg(" << input_file.tellg() << ")"
                << " < required_file_size(" << required_file_size << ")"
                << std::endl;
            return -1;
        }
    }

    // open db
    std::unique_ptr<::polar_race::Engine> engine;
    {
        ::polar_race::Engine* engine_ptr = nullptr;
        ::polar_race::RetCode ret =
            ::polar_race::EngineRace::Open(FLAGS_db_name, &engine_ptr);
        if (ret != ::polar_race::RetCode::kSucc) {
            std::cerr << "::polar_race::EngineRace::Open failed"
                << ", ret=" << ret
                << std::endl;
            return -1;
        }
        engine.reset(engine_ptr);
    }

    std::vector<std::thread> workers;
    for (int i = 0; i < FLAGS_thread_num; i++) {
        std::int64_t data_num = FLAGS_data_num_per_thread;
        std::int64_t offset = i * data_num * (FLAGS_key_size_byte + FLAGS_value_size_byte);
        workers.push_back(std::thread(
            [&engine, offset, data_num] () {
                std::ifstream in(FLAGS_input_data);
                in.seekg(offset);
                std::string buffer;
                buffer.resize(FLAGS_key_size_byte + FLAGS_value_size_byte);
                for (int i = 0; i < data_num; i++) {
                    if (!in.read(&buffer[0], buffer.size())) {
                        std::ostringstream oss;
                        oss << "in.read fail" << std::endl;
                        std::cerr << oss.str();
                        return;
                    }
                    ::polar_race::PolarString key(
                        buffer.substr(0, FLAGS_key_size_byte).c_str()
                        , FLAGS_key_size_byte
                    );
                    ::polar_race::PolarString value(
                        buffer.substr(FLAGS_key_size_byte).c_str()
                        , FLAGS_value_size_byte);
                    ::polar_race::RetCode ret =
                        engine->Write(key, value);
                    if (ret != ::polar_race::RetCode::kSucc) {
                        std::ostringstream oss;
                        oss << "engine->Write failed"
                            << ", ret=" << ret
                            << std::endl;
                        std::cerr << oss.str();
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
