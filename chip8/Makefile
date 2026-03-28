CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g  -I$(INC_DIR) $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

BUILD_DIR = build
SRC_DIR = src
INC_DIR = inc

TARGET = $(BUILD_DIR)/chip8emu
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: clean