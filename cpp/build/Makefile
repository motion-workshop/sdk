#
# @file    tools/sdk/cpp/build/Makefile
# @author  Luke Tokheim, luke@motionnode.com
# @version 1.4
#
NAME     = libMotionSDK.a
DEBUG    = no

# Standard compile and linking parameters. Leave the debugging stuff (and
# optimization) stuff for later.
CPPFLAGS = -Wall -pedantic
INCLUDE = -I..

# Add Debug/Release mode definitions.
ifeq ($(DEBUG),yes)
	CPPFLAGS += -g -D_DEBUG
else
	CPPFLAGS += -O3 -DNDEBUG
endif


# Tools, tools, tools. Replace these
# with your preferred toolset.
PREFIX   =
CPP      = $(PREFIX)g++
AR       = $(PREFIX)ar cr
RANLIB   = $(PREFIX)ranlib
RM       = rm -f

# Local paths. Source (.cpp) files, temporary output files (.o) and the
# final output path.
SOURCE_DIR = ../src
OUTPUT_DIR = .
TARGET_DIR = .

# Generate full relative pathed versions of object targets based on the list
# of source files.
SRC    = $(wildcard $(SOURCE_DIR)/*.cpp)
OBJ    = $(patsubst $(SOURCE_DIR)/%.cpp,$(OUTPUT_DIR)/%.o,$(SRC))
TARGET = $(TARGET_DIR)/$(NAME)
TEST     = $(TARGET_DIR)/test
TEST_OBJ = $(OUTPUT_DIR)/test.o
EXAMPLE_SDL     = $(TARGET_DIR)/example_sdl
EXAMPLE_SDL_OBJ = $(OUTPUT_DIR)/example_sdl.o
BINARY_TO_TEXT     = $(TARGET_DIR)/binary_to_text
BINARY_TO_TEXT_OBJ = $(OUTPUT_DIR)/binary_to_text.o


#
# Enter the rules section, of course, our friend "all".
#
all: $(TARGET) $(TEST) $(BINARY_TO_TEXT) $(EXAMPLE_SDL)

$(TARGET): $(OBJ)
	$(AR) $@ $(OBJ)
	$(RANLIB) $@

$(OUTPUT_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	$(CPP) -c $(CPPFLAGS) $(INCLUDE) $< -o $@

#
# Test program.
#
$(TEST): $(TARGET) $(TEST_OBJ)
	$(CPP) -o $@ $(TEST_OBJ) -L. -lMotionSDK

$(TEST_OBJ): ../test/test.cpp
	$(CPP) -c $(CPPFLAGS) $(INCLUDE) $< -o $@

#
# SDL example program.
#
$(EXAMPLE_SDL): $(TARGET) $(EXAMPLE_SDL_OBJ)
	$(CPP) -o $@ $(EXAMPLE_SDL_OBJ) -lSDLmain -lSDL -lGL -lGLU -lnsl -L. -lMotionSDK

$(EXAMPLE_SDL_OBJ): ../example_sdl/example_sdl.cpp
	$(CPP) -c $(CPPFLAGS) $(INCLUDE) $< -o $@

#
# Binary to text utility program.
#
$(BINARY_TO_TEXT): $(TARGET) $(BINARY_TO_TEXT_OBJ)
	$(CPP) -o $@ $(BINARY_TO_TEXT_OBJ) -L. -lMotionSDK

$(BINARY_TO_TEXT_OBJ): ../binary_to_text/binary_to_text.cpp
	$(CPP) -c $(CPPFLAGS) $(INCLUDE) $< -o $@

clean:
	$(RM) $(OBJ) $(TARGET) $(TEST) $(TEST_OBJ) $(EXAMPLE_SDL) $(EXAMPLE_SDL_OBJ) $(BINARY_TO_TEXT) $(BINARY_TO_TEXT_OBJ)
