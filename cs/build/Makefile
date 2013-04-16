#
# @file    tools/sdk/cs/build/Makefile
# @author  Luke Tokheim, luke@motionnode.com
# @version 2.0
#
# Use the Mono toolset to compile the SDK instead of Microsoft tools.
#
NAME     = MotionSDK.dll
DEBUG    = no

# Tools, tools, tools.
GMCS     = gmcs

# Local paths. Source (.cs) files and the final output path.
TARGET_DIR = .

# Generate full relative pathed versions of object targets based on the list
# of source files.
SRC    = ../Client.cs ../File.cs ../Format.cs ../LuaConsole.cs
TARGET = $(TARGET_DIR)/$(NAME)
TEST = test.exe

#
# Enter the rules section, of course, our friend "all".
#
all: $(TARGET) test
test: $(TEST)

$(TARGET): $(SRC)
	$(GMCS) -target:library -out:$@ $(SRC)

$(TEST): $(TARGET) ../Test.cs
	$(GMCS) -target:exe -out:$@ -reference:$(TARGET) ../Test.cs

clean:
	$(RM) $(TARGET) $(TEST)
