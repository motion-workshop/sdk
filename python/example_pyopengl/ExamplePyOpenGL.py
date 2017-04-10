#
# @file    sdk/python/example_pyopengl/ExamplePyOpenGL.py
# @version 2.5
#
# Copyright (c) 2017, Motion Workshop
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
import sys
import threading

# Data stream access.
from MotionSDK import *

# Drawing.
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *


class ScopedLock:
    def __init__(self, mutex):
        self.__mutex = None

        mutex.acquire()
        self.__mutex = mutex

    def __del__(self):
        self.__mutex.release()


class ExamplePyOpenGL (threading.Thread):

    def __init__(self):
        threading.Thread.__init__(self)

        self.__mutex = None
        self.__transform = None

        self.__mutex = threading.Lock()

    def __del__(self):
        if None != self.__mutex:
            lock = ScopedLock(self.__mutex)
            self.__mutex = None
            self.__transform = None

    def init(self):
        glClearColor(0, 0, 0, 0)
        glEnable(GL_DEPTH_TEST)
        glDepthFunc(GL_LEQUAL)

    def display(self):
        glClear(GL_COLOR_BUFFER_BIT)
        glClear(GL_DEPTH_BUFFER_BIT)

        # Draw a fixed mono-chromatic reference coordinate
        # system of some sort.
        glColor3f(0.7, 0.7, 0.7)
        glBegin(GL_LINES)
        glVertex3f(-1, 0, 0)
        glVertex3f(1, 0, 0)
        glVertex3f(0, -1, 0)
        glVertex3f(0, 1, 0)
        glVertex3f(0, 0, -1)
        glVertex3f(0, 0, 1)
        glEnd()


        # Transform a colored coordinate system by the current
        # orientation state of this object.
        glPushMatrix();

        transform = None
        if None != self.__mutex:
            lock = ScopedLock(self.__mutex)
            if None != self.__transform:
                # Make a copy of the list, not just reference.
                transform = self.__transform[:]
            lock = None


        if (None != transform) and (16 == len(transform)):
            # Transpose matrix for OpenGL column-major order.
            for i in range(0, 4):
                for j in range((i+1), 4):
                    temp = transform[4*i+j];
                    transform[4*i+j] = transform[4*j+i];
                    transform[4*j+i] = temp;

            glMultMatrixf(transform);

        axis = [
          [1, 0, 0],
          [0, 1, 0],
          [0, 0, 1]
        ];

        # Draw coordinate system.
        for value in axis:
          glColor3fv(value)
          glBegin(GL_LINES)
          glVertex3f(0, 0, 0)
          glVertex3fv(value)
          glEnd()

        glPopMatrix()

        glutSwapBuffers()

    def reshape(self, width, height):
        if height <= 0:
            height = 1

        height_real = float(width) / float(height)

        glViewport(0, 0, width, height)

        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(60, height_real, 1, 1000)

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        gluLookAt(-2, 2, 2, 0, 0, 0, 0, 1, 0)

    def idle(self):
        glutPostRedisplay()

    def visibility(self, visible):
        if visible == GLUT_VISIBLE:
            glutIdleFunc(self.idle)
        else:
            glutIdleFunc(None)

    def run(self):
        Host = ""
        Port = 32079

        client = Client(Host, Port)

        print "Connected to " + str(Host) + ":" + str(Port)

        while True:
            if client.waitForData():
                while True:
                    data = client.readData()
                    if None == data:
                        break

                    preview = Format.Preview(data)
                    for key, value in preview.iteritems():
                        transform = value.getMatrix(False)

                        if (None != transform) and (None != self.__mutex):
                            lock = ScopedLock(self.__mutex)
                            # Make a copy of the list, not just reference.
                            self.__transform = transform[:]
                            lock = None

                            break


        print "Leaving client thread"

#
# END class ExamplePyOpenGL
#


def main():
    glutInit(sys.argv)
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH)

    glutInitWindowPosition(0, 0)
    glutInitWindowSize(600, 600)
    glutCreateWindow("Example")

    example = ExamplePyOpenGL()
    example.start()

    example.init()

    glutDisplayFunc(example.display)
    glutReshapeFunc(example.reshape)
    glutVisibilityFunc(example.visibility)
    glutIdleFunc(example.idle)

    glutMainLoop()


if __name__ == "__main__":
    sys.exit(main())
