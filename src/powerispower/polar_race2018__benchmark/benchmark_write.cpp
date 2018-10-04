#include <algorithm>
#include <climits>
#include <chrono>
#include <limits>
#include <iostream>
#include <fstream>
#include <functional>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <engine_race/engine_race.h>

#include "powerispower/polar_race2018__benchmark/util.h"

std::string FLAGS_db_name = "./test_db";
std::int64_t FLAGS_thread_num = 64;
std::int64_t FLAGS_data_num_per_thread = 1000000;
std::int64_t FLAGS_key_size_byte = 8;
std::int64_t FLAGS_value_size_byte = 4096;
bool FLAGS_record_key_value = false;

struct WorkState {
    std::mt19937_64 random_uint64_generator;
    // random buffer for value
    std::string fake_data;
};

int parse_flags(int argc, char* argv[]) {
    using namespace ::powerispower::polar_race2018__benchmark;

    static const std::string& help_message =
        "Usage : ./benchmark_write"
        "\n\t--db_name (database name)"
        "\n\t--thread_num (benchmark thread num) default: 64"
        "\n\t--data_num_per_thread (xxx) default: 1e6"
        "\n\t--record_key_value (xxx) default: false";

    if (cmd_option_exist(argv, argv + argc, "--help")) {
        std::cerr << help_message << std::endl;
        return 0;
    }

    // parse FLAGS_db_name
    {
        char* option = get_cmd_option(argv, argv + argc, "--db_name");
        if (option != nullptr) {
            FLAGS_db_name = option;
        }
    }

    // parse FLAGS_thread_num
    {
        char* option = get_cmd_option(argv, argv + argc, "--thread_num");
        if (option != nullptr) {
            FLAGS_thread_num = std::stoi(option);
        }
    }

    // parse FLAGS_data_num_per_thread
    {
        char* option = get_cmd_option(argv, argv + argc, "--data_num_per_thread");
        if (option != nullptr) {
            FLAGS_data_num_per_thread = std::stoi(option);
        }
    }

    // parse FLAGS_record_key_value
    {
        char* option = get_cmd_option(argv, argv + argc, "--record_key_value");
        if (option != nullptr) {
            FLAGS_record_key_value = (std::string(option) == "true");
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
    int ret = parse_flags(argc, argv);
    if (ret != 0) {
        std::cerr << "parse_flags failed, ret=" << ret
            << std::endl;
        return -1;
    }

    // open db
    std::unique_ptr<::polar_race::Engine> db_engine;
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
        db_engine.reset(engine_ptr);
    }

    std::vector<std::thread> workers;
    std::vector<WorkState> work_states(FLAGS_thread_num);
    {
        std::random_device true_rand;
        for (auto& work_state : work_states) {
            // init work_state.random_uint64_generator
            work_state.random_uint64_generator.seed(true_rand());

            // init work_state.fake_data
            work_state.fake_data.resize(
                std::max(std::int64_t(1048576), FLAGS_value_size_byte)
            );
            std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> random_bytes_engine;
            random_bytes_engine.seed(true_rand());
            std::generate(
                work_state.fake_data.begin(), work_state.fake_data.end()
                , std::ref(random_bytes_engine));
        }
    }

    for (int i = 0; i < FLAGS_thread_num; i++) {
        workers.push_back(std::thread(
            [&db_engine, &work_states, i] () {
                auto thread_start_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double> framework_io_spend;
                std::shared_ptr<void> defer_0(
                    nullptr
                    , [&thread_start_time, &framework_io_spend](...) {
                        std::chrono::duration<double> duration =
                            std::chrono::high_resolution_clock::now() - thread_start_time;
                        std::ostringstream oss;
                        oss << "thread-" << std::this_thread::get_id() << " is over"
                            << ", duration_s=" << duration.count()
                            << ", framework_io_spend_s=" << framework_io_spend.count()
                            << std::endl;
                        std::cerr << oss.str();
                    });

                auto& work_state = work_states[i];
                int buffer_offset = 0;
                for (int i = 0; i < FLAGS_data_num_per_thread; i++) {
                    // prepare key-value
                    std::uint64_t key_uint64 = work_state.random_uint64_generator();
                    ::polar_race::PolarString key(
                        (char*)&key_uint64
                        , FLAGS_key_size_byte
                    );
                    if (buffer_offset + FLAGS_value_size_byte > (std::int64_t)work_state.fake_data.size()) {
                        buffer_offset = 0;
                    }
                    ::polar_race::PolarString value(
                        work_state.fake_data.c_str() + buffer_offset
                        , FLAGS_value_size_byte
                    );
                    buffer_offset += FLAGS_value_size_byte;

                    // write db
                    ::polar_race::RetCode ret =
                        db_engine->Write(key, value);
                    if (ret != ::polar_race::RetCode::kSucc) {
                        std::ostringstream oss;
                        oss << "engine->Write failed"
                            << ", ret=" << ret
                            << std::endl;
                        std::cerr << oss.str();
                        return;
                    }

                    // record key-value
                    if (FLAGS_record_key_value) {
                        auto start = std::chrono::high_resolution_clock::now();
                        std::ostringstream oss;
                        oss << *(std::int64_t*)(key.data())
                            << "\t" << std::hash<std::string>{}(value.ToString())
                            << std::endl;
                        std::cout << oss.str();
                        framework_io_spend +=
                            std::chrono::high_resolution_clock::now() - start;
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
