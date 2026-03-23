LINUX_CC=g++

LINUX_LIBS=-lglfw -lGL -ldl -lpthread -lX11 -lXrandr -lXi

SRC_DIR=src
BUILD_DIR=build

ENGINE_DIR=$(SRC_DIR)/engine
RENDERER_DIR=$(SRC_DIR)/renderer

SRCS=$(shell find $(SRC_DIR) -name "*.cpp")
OBJS=$(SRCS:%.cpp=$(BUILD_DIR)/%.o)

ENGINE_NAME=KonEngine

.PHONY: all clean engine

all: run

engine: $(BUILD_DIR)/$(ENGINE_NAME)

#
# ENGINE
#

$(BUILD_DIR)/$(ENGINE_NAME): $(OBJS)
	$(LINUX_CC) -o $(BUILD_DIR)/$(ENGINE_NAME) $(LINUX_LIBS) $(OBJS)

$(BUILD_DIR)/%.o : %.cpp
	mkdir -p $(dir $@)
	$(LINUX_CC) -c $< -o $@
clean:
	rm -rf $(BUILD_DIR)

run: engine
	./$(BUILD_DIR)/$(ENGINE_NAME)
