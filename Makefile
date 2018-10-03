SOURCES_DIR=src/powerispower/polar_race2018__benchmark

# Declaration of variables
CC = g++
CC_FLAGS = -w -std=c++11 \
	-W -Wextra -Wall -Wsign-compare \
	-Wno-unused-parameter -Woverloaded-virtual \
	-Wnon-virtual-dtor -Wno-missing-field-initializers
LDFLAGS = -pthread
INCPATHS = -I./src

# File names
EXEC = benchmark_write
SOURCES = $(wildcard $(SOURCES_DIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
 
# Main target
$(EXEC): $(OBJECTS)
	@echo "OBJECTS="$(OBJECTS)
	$(CC) $(INCPATHS) $(OBJECTS) $(LDFLAGS) -o $(EXEC)

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CC_FLAGS) $(INCPATHS) $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)
