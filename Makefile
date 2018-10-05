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
BINS = benchmark_write

all: $(BINS)

# Main target
sources = $(SOURCES_DIR)/benchmark_write.cpp $(SOURCES_DIR)/util.cpp
objects = $(sources:.cpp=.o)
$(BINS): module_engine $(objects)
	@echo "objects="$(objects)
	$(CC) $(INCPATHS) $(objects) $(LDFLAGS) -o benchmark_write

# pre build
module_engine:
	@echo "shjwudp make "$(MODULE_ENGINE)
	make -C $(MODULE_ENGINE)

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $(INCPATHS) $< -o $@

# To remove generated files
clean:
	rm -f $(BINS) $(OBJECTS)
	rm -rf $(OUT_DIR)
	make -C $(MODULE_ENGINE) clean
