# Compiler
CXX = clang++-19

# Compiler flags
CXXFLAGS = -g -std=c++17 -Wall -Wextra -pedantic

# Target executable
BUILD_DIR = build
CLIENT_TARGET = client
SERVER_TARGET = server

# Source dirs
#LAYER_DIRS=$(shell ls -dC *layer/feature/*/src/)
#$(info Layers: $(LAYER_DIRS))

SRC_DIR=$(shell ls -dC *layer/feature/*/src/)
# Debug: Print SRC_DIR to verify it's populated
$(info Sources Dir: $(SRC_DIR))

# Source files
SRCS=$(foreach srcs,$(SRC_DIR),$(wildcard ./$(srcs)/*.cpp)) ./app/src/main.cpp
# Debug: Print SRCS to verify it's populated
$(info Sources: $(SRCS))

# Include dirs
INCLUDE_DIRS=$(shell ls -dC *layer/feature/*/include/)
INCLUDE_DIR=$(foreach incl,$(INCLUDE_DIRS),-I ./$(incl))
# Debug: Print INCLUDE_DIR to verify it's populated
$(info Include Dir: $(INCLUDE_DIR))

# Include files
INCLUDES=$(foreach incld,$(INCLUDE_DIR),$(wildcard ./$(incld)/*.h))
# Debug: Print INCLUDES to verify it's populated
$(info Includes: $(INCLUDES))

# Generate corresponding object files in the build directory
OBJS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))

run: $(TARGET)
	@echo "Running $<"
	@sudo ./$(BUILD_DIR)/$<

# Default target
#all: target_server target_client

target_server: CXXFLAGS += -DSERVER
target_server: $(SERVER_TARGET)

target_client: CXXFLAGS += -DCLIENT
target_client: $(CLIENT_TARGET)

# Build the server target
$(SERVER_TARGET): $(OBJS)
	@echo "Linking $@"
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)
	@mv $(SERVER_TARGET) $(BUILD_DIR)
	@echo "Build complete"

# Build the client target
$(CLIENT_TARGET): $(OBJS)
	@echo "Linking $@"
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)
	@mv $(CLIENT_TARGET) $(BUILD_DIR)
	@echo "Build complete"

# Compile each source file into an object file
$(BUILD_DIR)/%.o: %.cpp
	@echo "Compiling $< -> $@"
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $@

# Clean up build files
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

.PHONY: all clean
