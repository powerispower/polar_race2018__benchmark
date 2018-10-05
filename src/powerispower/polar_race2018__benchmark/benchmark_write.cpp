#include <algorithm>
#include <atomic>
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
std::int64_t FLAGS_fake_data_seed = 19937;
std::uint64_t FLAGS_key_mask = std::numeric_limits<std::uint64_t>::max();

struct WorkState {
    std::mt19937_64 random_uint64_generator;
    // random buffer for value
    std::string fake_data;

    std::thread::id worker_id;
    std::chrono::system_clock::time_point start_time;
    std::atomic<std::int64_t> processed_data_num;

    WorkState()
        : processed_data_num(0)
    {}
};

int parse_flags(int argc, char* argv[]) {
    using namespace ::powerispower::polar_race2018__benchmark;

    static const std::string& help_message =
        "Usage : ./benchmark_write"
        "\n\t--db_name (database name)"
        "\n\t--thread_num (benchmark thread num) default: 64"
        "\n\t--data_num_per_thread (xxx) default: 1e6"
        "\n\t--record_key_value (xxx) default: false"
        "\n\t--fake_data_seed (fake data random generator seed) default: 19937";

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
            FLAGS_thread_num = std::stoll(option);
        }
    }

    // parse FLAGS_data_num_per_thread
    {
        char* option = get_cmd_option(argv, argv + argc, "--data_num_per_thread");
        if (option != nullptr) {
            FLAGS_data_num_per_thread = std::stoll(option);
        }
    }

    // parse FLAGS_record_key_value
    {
        char* option = get_cmd_option(argv, argv + argc, "--record_key_value");
        if (option != nullptr) {
            FLAGS_record_key_value = (std::string(option) == "true");
        }
    }

    // parse FLAGS_fake_data_seed
    {
        char* option = get_cmd_option(argv, argv + argc, "--fake_data_seed");
        if (option != nullptr) {
            FLAGS_fake_data_seed = std::stoll(option);
        }
    }

    // parse FLAGS_key_mask
    {
        char* option = get_cmd_option(argv, argv + argc, "--key_mask");
        if (option != nullptr) {
            FLAGS_key_mask = std::stoull(option);
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
        std::cerr << "open db start" << std::endl;
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
        std::cerr << "open db end" << std::endl;
    }

    std::vector<std::thread> workers;
    std::vector<WorkState> work_states(FLAGS_thread_num);
    // prepare work_states
    {
        std::cerr << "prepare work_states start" << std::endl;
        std::mt19937_64 seed_generator(FLAGS_fake_data_seed);
        for (auto& work_state : work_states) {
            // init work_state.random_uint64_generator
            work_state.random_uint64_generator.seed(seed_generator());

            // init work_state.fake_data
            work_state.fake_data.resize(
                std::max(std::int64_t(1048576), FLAGS_value_size_byte)
            );
            std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char> random_bytes_engine;
            random_bytes_engine.seed(seed_generator());
            std::generate(
                work_state.fake_data.begin(), work_state.fake_data.end()
                , std::ref(random_bytes_engine));
        }
        std::cerr << "prepare work_states end" << std::endl;
    }

    int monitor_control_flag = 1;
    // run monitor
    std::thread monitor(
        [&work_states, &monitor_control_flag] () {
            while (monitor_control_flag == 1) {
                std::chrono::duration<double> duration_sum(0);
                std::int64_t processed_data_num_sum = 0;
                for (const auto& work_state: work_states) {
                    std::chrono::duration<double> duration =
                        std::chrono::high_resolution_clock::now() - work_state.start_time;
                    std::ostringstream oss;
                    oss << "thread-" << work_state.worker_id << " :"
                        << " duration_s=" << duration.count()
                        << ", processed_data_num=" << work_state.processed_data_num
                        << std::endl;
                    std::cerr << oss.str();

                    duration_sum += duration;
                    processed_data_num_sum += work_state.processed_data_num;
                }

                // notice log
                {
                    std::ostringstream oss;
                    oss << "std::time(nullptr)=" << std::time(nullptr)
                        << ", duration_sum_s=" << duration_sum.count()
                        << ", processed_data_num_sum=" << processed_data_num_sum
                        << std::endl;
                    std::cerr << oss.str();
                }

                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        });

    // run worker
    for (int i = 0; i < FLAGS_thread_num; i++) {
        workers.push_back(std::thread(
            [&db_engine, &work_states, i] () {
                auto& work_state = work_states[i];
                work_state.start_time = std::chrono::high_resolution_clock::now();
                work_state.worker_id = std::this_thread::get_id();
                std::chrono::duration<double> framework_io_spend;
                std::shared_ptr<void> defer_0(
                    nullptr
                    , [&work_state, &framework_io_spend](...) {
                        std::chrono::duration<double> duration =
                            std::chrono::high_resolution_clock::now() - work_state.start_time;
                        std::ostringstream oss;
                        oss << "thread-" << std::this_thread::get_id() << " is over"
                            << ", duration_s=" << duration.count()
                            << ", framework_io_spend_s=" << framework_io_spend.count()
                            << std::endl;
                        std::cerr << oss.str();
                    });

                int buffer_offset = 0;
                for (int i = 0; i < FLAGS_data_num_per_thread; i++) {
                    // prepare key-value
                    std::uint64_t key_uint64 = work_state.random_uint64_generator() & FLAGS_key_mask;
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
                        oss << "db_engine->Write failed"
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

                    work_state.processed_data_num += 1;
                }
            }
        ));
    }

    for (auto& worker : workers) {
        worker.join();
    }
    monitor_control_flag = 0;
    monitor.join();

    return 0;
}
