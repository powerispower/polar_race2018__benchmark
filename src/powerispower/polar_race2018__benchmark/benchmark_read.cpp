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
#include <map>

#include <engine_race/engine_race.h>

#include "powerispower/polar_race2018__benchmark/util.h"

std::string FLAGS_db_name = "./test_db";
std::string FLAGS_input_data = "./benchmark_write.out";
std::int64_t FLAGS_thread_num = 64;
std::int64_t FLAGS_data_num_per_thread = 1000000;

struct WorkState {
    std::thread::id worker_id;
    std::chrono::system_clock::time_point start_time;

    WorkState()
    {}
};

int parse_flags(int argc, char* argv[]) {
    using namespace ::powerispower::polar_race2018__benchmark;

    static const std::string& help_message =
        "Usage : ./benchmark_write"
        "\n\t--db_name (database name)"
        "\n\t--input_data (the name of the file included input data) default: ./benchmark_write.out"
        "\n\t--thread_num (benchmark thread num) default: 64"
        "\n\t--data_num_per_thread (xxx) default: 1e6";

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
    // parse FLAGS_input_data
    {
        char* option = get_cmd_option(argv, argv + argc, "--input_data");
        if (option != nullptr) {
            FLAGS_input_data = option;
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

    // load data from file
    std::map<std::int64_t, std::size_t> hash_mem;
    std::vector<std::int64_t> key_int64_vec;
    auto key_int64_vec_size = key_int64_vec.size();
    {
        std::cerr << "try to load data from file, start..." << std::endl;

        std::chrono::duration<double> load_data_spend;
        auto start = std::chrono::high_resolution_clock::now();

        std::ifstream hash_file(FLAGS_input_data);
        
        std::int64_t k;
        std::size_t v;

        while(hash_file >> k >> v){
            hash_mem[k] = v;
            key_int64_vec.push_back(k);
        }
        hash_file.close();

        key_int64_vec_size = key_int64_vec.size();

        auto end = std::chrono::high_resolution_clock::now();
        load_data_spend = end - start;
        std::cerr << "load data from file end"
                  <<", load_data_spend_s="<<load_data_spend.count()
                  <<std::endl;
    }

    std::vector<std::thread> workers;
    std::vector<WorkState> work_states(FLAGS_thread_num);

    // run worker
    for (int i = 0; i < FLAGS_thread_num; i++) {
        std::int64_t offset = i * FLAGS_data_num_per_thread;
        workers.push_back(std::thread(
            [&db_engine, &work_states, i, &hash_mem, &key_int64_vec, &key_int64_vec_size, &offset] () {
                auto& work_state = work_states[i];
                work_state.start_time = std::chrono::high_resolution_clock::now();
                work_state.worker_id = std::this_thread::get_id();
                std::chrono::duration<double> framework_io_spend, hash_spend;
                std::shared_ptr<void> defer_0(
                    nullptr
                    , [&work_state, &framework_io_spend, &hash_spend](...) {
                        std::chrono::duration<double> duration =
                            std::chrono::high_resolution_clock::now() - work_state.start_time;
                        // 除去IO耗时
                        duration -= framework_io_spend;
                        // 除去hash耗时
                        duration -= hash_spend;

                        std::ostringstream oss;
                        oss << "thread-" << std::this_thread::get_id() << " is over"
                            << ", duration_s=" << duration.count()
                            << ", framework_io_spend_s=" << framework_io_spend.count()
                            << ", hash_spend_s=" << hash_spend.count()
                            << std::endl;
                        std::cerr << oss.str();
                    });

                std::int64_t index = offset;
                for (int i = 0; i < FLAGS_data_num_per_thread; i++) {
                    // index合法性检验
                    if(!(index >= 0 && index < key_int64_vec_size)){
                        std::ostringstream oss;
                        oss << "invalid index of vector: "<< index << std::endl;
                        std::cerr << oss.str(); 
                        return;
                    }

                    // 构造key, value
                    std::string key, value;
                    std::int64_t key_int64 = key_int64_vec[index++];
                    key.resize(sizeof(key_int64));
                    memcpy((char*)key.c_str(), (char*)&key_int64, sizeof(key_int64));
                    
                    // read from db
                    ::polar_race::RetCode ret =
                        db_engine->Read(key, &value);

                    if (ret == ::polar_race::RetCode::kSucc) {

                        // 计算hash耗时(和map查找耗时)
                        auto start = std::chrono::high_resolution_clock::now();
                        std::size_t value_hashed_in_db = std::hash<std::string>{}(value);
                        std::size_t value_hashed_in_mem = hash_mem[key_int64];
                        hash_spend +=
                            std::chrono::high_resolution_clock::now() - start;

                        if(value_hashed_in_db!=value_hashed_in_mem){
                            auto start = std::chrono::high_resolution_clock::now();
                            std::ostringstream oss;
                            oss << "db_engine->Read, not match!, key_int64="<< key_int64
                                << ", hash(value)="<<value_hashed_in_db
                                << ", should be: "<<value_hashed_in_mem
                                << std::endl;
                            std::cerr << oss.str();
                            framework_io_spend +=
                                std::chrono::high_resolution_clock::now() - start;
                        }
                    }
                    else if (ret == ::polar_race::RetCode::kNotFound) {
                        auto start = std::chrono::high_resolution_clock::now();
                        std::ostringstream oss;
                        oss << "db_engine->Read: not found!"
                            <<" key_int64="<< key_int64
                            << std::endl;
                        std::cerr << oss.str();
                        framework_io_spend +=
                            std::chrono::high_resolution_clock::now() - start;
                    }
                    else{
                        std::ostringstream oss;
                        oss << "engine->Read: failed!"
                            <<" ret=" << ret << std::endl;
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
