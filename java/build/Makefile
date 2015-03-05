#
# Makefile for the Java Motion SDK. Create the MotionSDK.jar
# archive file by default. Also build the test program. Requires
# the JOGL jar file to build the ExampleJOGL application. This is
# available at https://jogl.dev.java.net/.
#
# @file    tools/sdk/java/build/Makefile
# @author  Luke Tokheim, luke@motionnode.com
# @version 2.2
#
NAME     = MotionSDK.jar
DEBUG    = no

# Standard compile and linking parameters. Leave the debugging stuff (and
# optimization) stuff for later.
JAVACFLAGS = -deprecation -source 1.5 -Xlint:all

# Class path separator. Use the semicolon on Windows.
#CP_SEP     = ;
CP_SEP     = :

# Path to your Java OpenGL archive. If your distribution does not
# come with a package, just download the binaries and copy them
# into the build folder.
#JOGLPATH   = ./jogl/lib/jogl.jar$(CP_SEP)./jogl/lib/gluegen-rt.jar
JOGLPATH   = /usr/share/java/jogl.jar$(CP_SEP)/usr/share/java/gluegen-rt.jar
# If this is just a local install then you will also need to set
# the dynamic library load path such that the Java OpenGL runtime
# files are available
export LD_LIBRARY_PATH=./jogl/lib

# Or, you may have Java OpenGL installed elsewhere.
#JOGLPATH   = /usr/share/java/jogl.jar


# Add Debug/Release mode definitions.
ifeq ($(DEBUG),yes)
	JAVACFLAGS += -g
else
	JAVACFLAGS += -g:none
endif


# Tools, tools, tools.
JDK_BIN  = #/opt/jdk1.5.0_07/bin/
JAVA     = $(JDK_BIN)java
JAVAC    = $(JDK_BIN)javac
JAVADOC  = $(JDK_BIN)javadoc
JAR      = $(JDK_BIN)jar
RM       = rm -f

# Local paths. Source (.java) files, temporary output files (.class) and the
# final output path.
SOURCE_DIR = ..
OUTPUT_DIR = Motion/SDK
TARGET_DIR = .
LINK_JAVADOC = http://java.sun.com/j2se/1.5.0/docs/api/

# Generate full relative pathed versions of object targets based on the list
# of source files.
SRC    = $(wildcard $(SOURCE_DIR)/*.java)
OBJ    = $(patsubst $(SOURCE_DIR)/%.java,$(OUTPUT_DIR)/%.class,$(SRC))
TARGET = $(TARGET_DIR)/$(NAME)
EXAMPLE_JOGL = $(TARGET_DIR)/ExampleJOGL.jar

#
# Enter the rules section, of course, our friend "all".
#
all: $(TARGET) $(EXAMPLE_JOGL)

$(TARGET): $(OBJ)
	$(JAR) -cvf $@ $(OBJ) Motion/SDK/*.class

$(OUTPUT_DIR)/%.class: $(SOURCE_DIR)/%.java
	$(JAVAC) $(JAVACFLAGS) -d . -cp . $<

#
# Example JOGL program.
#
$(EXAMPLE_JOGL): $(TARGET) ExampleJOGL.class
	$(JAR) -cvf $@ ExampleJOGL*.class

ExampleJOGL.class: ../example_jogl/ExampleJOGL.java
	$(JAVAC) -d . -classpath $(TARGET)$(CP_SEP)$(JOGLPATH) $(JAVACFLAGS) $<

run: $(EXAMPLE_JOGL)
	$(JAVA) -classpath $(TARGET)$(CP_SEP)$(JOGLPATH)$(CP_SEP)$(EXAMPLE_JOGL) ExampleJOGL 127.0.0.1:32079

test: Test.class

Test.class: ../test/Test.java
	$(JAVAC) -d . -classpath $(TARGET) $(JAVACFLAGS) $<

#
# Each class includes a main method to run some tests.
#
run_test: $(TARGET)
	$(JAVA) -classpath $(TARGET) Motion.SDK.LuaConsole
	$(JAVA) -classpath $(TARGET) Motion.SDK.Client
	$(JAVA) -classpath $(TARGET) Motion.SDK.File
	$(JAVA) -classpath $(TARGET) Motion.SDK.Format
	$(JAVA) -classpath .$(CP_SEP)$(TARGET) Test

#
# Documentation.
#
doc: $(SOURCE_DIR)/*.java
	$(JAVADOC) -d docs -link $(LINK_JAVADOC) $(SOURCE_DIR)/*.java

#
# Clean all build files.
#
clean:
	$(RM) $(OBJ) Motion/SDK/*.class $(TARGET) ExampleJOGL*.class $(EXAMPLE_JOGL) Test.class
