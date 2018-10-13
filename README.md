# 使用方法

```bash
# 编译
# 拉取 https://github.com/powerispower/polar_race2018__engine.git
git submodule init
git submodule update --remote --rebase
make clean && make

# 运行benchmark_write
./benchmark_write \
    --db_name your_db_name \
    --thread_num 64 \
    --data_num_per_thread 100 \
    --record_key_value false \
    --fake_data_seed 19937 \
    --key_mask 18446744073709551615 \
    --data_num_per_thread 100

# 运行benchmark_read
./benchmark_read \
    --db_name your_db_name \
    --input_data your_input_file \
    --thread_num 64

example of your_input_file:
-701650746043007101	12510867064863635312
7485958427358951629	6514538137122403340
3298304116713096505	10768630635511849718
6381434364343182315	13393317789206788936
6411843271829142533	6981962204903017771
···

example to get the your_input_file from the benchmark_write:
./benchmark_write \
    --db_name your_db_name \
    --thread_num 64 \
    --record_key_value true > ./benchmark_write.out
```
