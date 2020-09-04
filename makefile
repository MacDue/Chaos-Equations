# An ungly make file that works.
CC = g++
SRC_DIR = src
OBJ_DIR = obj
TEMP_DIR = temp

SRC_FILES := $(shell find $(SRC_DIR) -name "*.cpp")
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

CFLAGS = -DIMGUI_IMPL_OPENGL_LOADER_GLAD -std=c++17 -I$(SRC_DIR)/ -I./include -I./$(TEMP_DIR) -I./src/imgui -I./src/imgui_impl
LFLAGS = -lboost_program_options -lglfw -lGL -lX11 -lpthread -lXrandr -ldl

LINK = $(CC) -o $@ $^ $(LFLAGS)

$(OBJ_DIR)/glad.O:
				$(MKDIR_P) `dirname $@`
				$(CC) -I./include -std=c11 -c -o $@ $(SRC_DIR)/glad.c

# The debug build.
app: CFLAGS += -g
app: $(OBJ_FILES) $(OBJ_DIR)/glad.O
				$(LINK)

# Delete objects.
clean:
				find obj/ -name "*.o" -type f -delete
				find temp/ -name "*.glsl.h" -type f -delete
				-rm -rf ./screensaver

MKDIR_P = mkdir -p

# Object compiling.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
				$(MKDIR_P) `dirname $@`
				$(CC) $(CFLAGS) -c -o $@ $< $(LFLAGS)
