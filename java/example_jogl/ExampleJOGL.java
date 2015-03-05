/**
  Draw a coordinate system that defines the real-time orientation
  of a single MotionNode IMU. Use the {@link Motion.SDK.Client} class
  to read preview data from the remote host. Use the
  {@link Motion.SDK.Format} class to access specific preview data
  elements.

  Implemented in OpenGL layered on top of a Java Binding for the OpenGL
  API (JOGL). The JOGL library is available at http://jogl.dev.java.net/
  under the Berkeley Software Distribution (BSD) License.

  @file    tools/sdk/java/example_jogl/ExampleJOGL.java
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.2

  Copyright (c) 2015, Motion Workshop
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
import Motion.SDK.Client;
import Motion.SDK.Format;

import java.io.IOException;

import java.awt.Frame;
import java.awt.GridLayout;
import java.awt.Label;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.Map;

import javax.media.opengl.GL;
import javax.media.opengl.GL2;
import javax.media.opengl.GLAutoDrawable;
import javax.media.opengl.GLEventListener;
import javax.media.opengl.awt.GLCanvas;
import javax.media.opengl.glu.GLU;
import com.jogamp.opengl.util.FPSAnimator;

/**
   Encapsulate the OpenGL and Motion.SDK.Client code for the JOGL example
   application in this class. Implements the {@link javax.media.opengl.GLEventListener}
   interface.

   Also, implements a function object that connects to the Motion Service preview
   stream and reads real-time orientation data. This should run in its own
   thread. The ExampleJOGL class handles synchronization of the transformation
   matrix. Note that if there are multiple device channels, this will only use
   the first one.
*/
class ExampleJOGL implements GLEventListener, Runnable
{
    private float[] orientation = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    private static final float[][] axis = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    private final String host;
    private final int port;

    /**
      Default constructor for backwards compatiblity.
    */
    ExampleJOGL()
    {
        host = "";
        port = 32079;
    }

    /**
      Constructor with runtime configurable remote
      host and port.
    */
    ExampleJOGL(String host_in, int port_in)
    {
        host = host_in;
        port = port_in;
    }

    /**
       OpenGL function, draw the current frame.
    */
    public void display(GLAutoDrawable drawable)
    {
        final GL2 gl = drawable.getGL().getGL2();
        gl.glClear(GL.GL_COLOR_BUFFER_BIT);
        gl.glClear(GL.GL_DEPTH_BUFFER_BIT);
        {
            // Draw a fixed mono-chromatic reference coordinate
            // system of some sort.
            gl.glColor3f(0.7f, 0.7f, 0.7f);
            gl.glBegin(GL.GL_LINES);
            {
                gl.glVertex3f(-1, 0, 0);
                gl.glVertex3f(1, 0, 0);
                gl.glVertex3f(0, -1, 0);
                gl.glVertex3f(0, 1, 0);
                gl.glVertex3f(0, 0, -1);
                gl.glVertex3f(0, 0, 1);
            }
            gl.glEnd();


            // Transform a colored coordinate system by the current
            // orientation state of this object.
            gl.glPushMatrix();
            {
                float[] rotate = orientation_matrix(null);
                if ((null != rotate) && (16 == rotate.length))
                {
                    gl.glMultTransposeMatrixf(rotate, 0);
                }

                for (int i = 0; i < axis.length; i++)
                {
                    gl.glColor3fv(axis[i], 0);
                    gl.glBegin(GL.GL_LINES);
                    {
                        gl.glVertex3f(0, 0, 0);
                        gl.glVertex3fv(axis[i], 0);
                    }
                    gl.glEnd();
                }
            }
            gl.glPopMatrix();
        }

    }

    public void displayChanged(GLAutoDrawable drawable,
                   boolean modeChanged,
                   boolean deviceChanged)
    {
    }
    
    /**
       OpenGL function, initialize parameters.
    */
    public void init(GLAutoDrawable drawable)
    {
        final GL2 gl = drawable.getGL().getGL2();
        gl.glShadeModel(GL2.GL_SMOOTH);
        gl.glClearColor(0, 0, 0, 0);
        gl.glClearDepth(1);
        gl.glEnable(GL.GL_DEPTH_TEST);
        gl.glDepthFunc(GL.GL_LEQUAL);
        gl.glHint(GL2.GL_PERSPECTIVE_CORRECTION_HINT, GL.GL_NICEST);
    }

    public void dispose(GLAutoDrawable drawable)
    {
    }

    /**
       OpenGL function, resize the parent window.
    */
    public void reshape(GLAutoDrawable drawable,
        int x, int y, int width, int height)
    {
        final GL2 gl = drawable.getGL().getGL2();
        final GLU glu = new GLU();
        if (height <= 0)
        {
            height = 1;
        }
        final float h = (float)width / (float)height;

        gl.glViewport(0, 0, width, height);

        gl.glMatrixMode(GL2.GL_PROJECTION);
        {
            gl.glLoadIdentity();
            glu.gluPerspective(60, h, 1, 1000);
        }

        gl.glMatrixMode(GL2.GL_MODELVIEW);
        {
            gl.glLoadIdentity();
            glu.gluLookAt(-2, 2, 2, 0, 0, 0, 0, 1, 0);
        }
    }

    /**
       Thread function. Make a connection to the remote Host:Port and read
       Preview data in a loop.
    */
    public void run()
    {
        try {
            Client client = new Client(host, port);

            System.out.println("Connected to " + host + ":" + port);

            while (true) {
                if (client.waitForData()) {
                    while (true) {
                        ByteBuffer data = client.readData();
                        if (null == data) {
                            break;
                        }

                        Map<Integer,Format.PreviewElement> preview = Format.Preview(data);
                        for (Map.Entry<Integer,Format.PreviewElement> entry: preview.entrySet()) {
                            orientation_matrix(entry.getValue().getMatrix(false));
                            break;
                        }
                    }
                }
            }

        } catch (Exception e) {
            System.err.println(e);
        }

        System.out.println("Leaving client thread");
    }

    /**
       Read and write operations on the member <tt>orientation</tt> should go
       through this method. Use the built-in synchronization stuff so we can
       call it from multiple threads.
    */
    private synchronized float[] orientation_matrix(float[] value)
    {
        if ((null != value) && (value.length == orientation.length))
        {
            orientation = value;
        }
        return orientation;
    }

    /**
       Entry point. Initialize a window with an OpenGL GLCanvas child
       object. Start client thread and main event loop.
    */
    public static void main(String[] args)
        throws IOException
    {
        // Connect to port 32079 on localhost.
        String host = "";
        int port = 32079;

        // Parse a single command line argument that defines
        // the remote host and port.
        // For example, "www.google.com:80" or "10.0.0.1:32079"
        // 
        // The port argument is optional and defaults to 32079.
        //
        if (args.length > 0)
        {
            String[] token_list = args[0].split(":");
            if (token_list.length > 0)
            {
                host = token_list[0];
            }
            if (token_list.length > 1)
            {
                port = Integer.parseInt(token_list[1]);
            }
        }


        ExampleJOGL example = new ExampleJOGL(host, port);
        {
            new Thread(example).start();
        }

        GLCanvas canvas = new GLCanvas();
        canvas.addGLEventListener(example);

        Frame frame = new Frame("Example");
        frame.add(canvas);
        frame.setSize(600, 600);

        final FPSAnimator animator = new FPSAnimator(canvas, 30);
        frame.addWindowListener(new WindowAdapter()
        {
            public void windowClosing(WindowEvent e)
            {
                // Run this on another thread than the AWT event queue to
                // make sure the call to Animator.stop() completes before
                // exiting
                new Thread(new Runnable()
                {
                    public void run()
                    {
                        animator.stop();
                        System.exit(0);
                    }
                }).start();
            }
        });
        frame.setVisible(true);
        animator.start();
    }
}
