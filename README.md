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
    --key_mask 18446744073709551615
```
