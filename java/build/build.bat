@rem
@rem Batch file to build the Java Motion SDK. This file will require some
@rem manual adjustment to build the ExampleJOGL class. In particular you will
@rem need to install JOGL locally and enter the path to the jar file here.
@rem JOGL is available at https://jogl.dev.java.net/.
@rem
@rem @file    tools/sdk/java/build/build.bat
@rem @author  Luke Tokheim, luke@motionnode.com
@rem
javac -d . -classpath . -Xlint:all -deprecation -g:none -source 1.5 ..\Client.java ..\File.java ..\Format.java
jar -cvf MotionSDK.jar Motion\SDK\*.class

@rem javac -d . -classpath .;E:\local\java\jogl\jogl-all.jar -Xlint:all -deprecation -g:none -source 1.5 ..\example_jogl\ExampleJOGL.java
@rem jar -cvf ExampleJOGL.jar ExampleJOGL*.class

@rem javadoc -d ..\doc -link http://java.sun.com/j2se/1.5.0/docs/api/ ..\Client.java ..\File.java ..\Format.java