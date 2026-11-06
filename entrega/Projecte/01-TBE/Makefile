CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
CPPFLAGS ?= -MMD -MP

INCLUDES = -I. \
           -Icore \
           -Ientities \
           -Ientities/enemies \
           -Igraphics \
           -Iui \
           -Iworld \
           -Isoil

LIBS = -lglfw -lGLEW -lGL

SRCS = main.cpp \
       $(wildcard core/*.cpp) \
       $(wildcard entities/*.cpp) \
       $(wildcard entities/enemies/*.cpp) \
       $(wildcard graphics/*.cpp) \
       $(wildcard ui/*.cpp) \
       $(wildcard world/*.cpp) \
       $(wildcard soil/*.cpp)

OBJ_DIR = build
OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)

TARGET = 01-TBE

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LIBS) -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all run clean
