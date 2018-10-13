# Declaration of variables
SOURCES_DIR = src/powerispower/polar_race2018__benchmark
MODULE_ENGINE = third_party/powerispower/polar_race2018__engine
CC = g++
CC_FLAGS = -g -pipe -Wall \
	-W -fPIC -Wno-unused-parameter \
	-Wno-strict-aliasing -fno-omit-frame-pointer \
	-Wno-invalid-offsetof -std=c++11 -O2

LDFLAGS = -L$(MODULE_ENGINE)/lib -lengine -pthread
INCPATHS = -I./src -I$(MODULE_ENGINE)

# File names
BINS_W = benchmark_write
BINS_R = benchmark_read

all: $(BINS_W) $(BINS_R)

# Main target
sources_w = $(SOURCES_DIR)/benchmark_write.cpp $(SOURCES_DIR)/util.cpp
objects_w = $(sources_w:.cpp=.o)
$(BINS_W): module_engine $(objects_w)
	@echo "objects="$(objects_w)
	$(CC) $(INCPATHS) $(objects_w) $(LDFLAGS) -o benchmark_write

sources_r = $(SOURCES_DIR)/benchmark_read.cpp $(SOURCES_DIR)/util.cpp
objects_r = $(sources_r:.cpp=.o)
$(BINS_R): module_engine $(objects_r)
	@echo "objects="$(objects_r)
	$(CC) $(INCPATHS) $(objects_r) $(LDFLAGS) -o benchmark_read

# pre build
module_engine:
	@echo "make "$(MODULE_ENGINE)
	make -C $(MODULE_ENGINE)

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $(INCPATHS) $< -o $@

# To remove generated files
clean:
	rm -f $(BINS_W) $(OBJECTS_W) $(BINS_R) $(OBJECTS_R)
	rm -rf $(OUT_DIR)
	make -C $(MODULE_ENGINE) clean
