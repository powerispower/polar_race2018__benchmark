# 使用方法

```bash
# 构造测试数据
dd if=/dev/urandom of=./test.in bs=1M count=64

# 编译
# 拉取 https://github.com/powerispower/polar_race2018__engine.git
git submodule init
git submodule update --remote --rebase
make clean && make

# 运行benchmark_write
./benchmark_write \
    --input_data ./test.in \
    --db_name your_db_name \
    --thread_num 64 \
    --data_num_per_thread 100
```

