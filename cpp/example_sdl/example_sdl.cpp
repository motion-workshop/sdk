/**
  Simple test program for the C++ Motion SDK.
  
  Draw a coordinate system that defines the real-time orientation of a single
  MotionNode IMU. Use the @ref Client class to read preview data from the remote
  host. Use the @ref Format class to access specific preview data elements.

  Implemented in OpenGL layered on top of the Simple Directmedia
  Layer (SDL), a "cross-platform multimedia library". The SDL library
  is available at http://www.libsdl.org/ under the GNU LGPL version 2
  license.

  @file    tools/sdk/cpp/example_sdl/example_sdl.cpp
  @author  Luke Tokheim, luke@motionnode.com
  @version 2.0

  (C) Copyright Motion Workshop 2013. All rights reserved.

  The coded instructions, statements, computer programs, and/or related
  material (collectively the "Data") in these files contain unpublished
  information proprietary to Motion Workshop, which is protected by
  US federal copyright law and by international treaties.

  The Data may not be disclosed or distributed to third parties, in whole
  or in part, without the prior written consent of Motion Workshop.

  The Data is provided "as is" without express or implied warranty, and
  with no claim as to its suitability for any purpose.
*/
#include <Client.hpp>
#include <Format.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_thread.h>


// Defaults to "127.0.0.1"
const std::string Host = "127.0.0.1";
// Default port of the Motion Service Preview data stream.
const unsigned Port = 32079;
// Throttle down the main event and drawing loop to approximately
// this frame rate, specified in milliseconds.
const Uint32 TargetFrameRate = (1000/30);


/**
  Simple function to wrap a function object of type T as
  an SDL thread function, suitable for a call to
  SDL_CreateThread. Define function object as a class with
  T::operator()(void).
*/
template <typename T>
int sdl_thread_functor(void *instance)
{
  reinterpret_cast<T *>(instance)->operator()();
  return 0;
}

/**
  Scoped lock object for a SDL_mutex. Simply acquire an
  exclusive lock for the lifetime of this object.

  Example usage:
  @code
  SDL_mutex *mutex = SDL_CreateMutex();
  {
    sdl_mutex_lock lock(mutex);
  
    // This thread has the exclusive lock.

  } // Lock object goes out of scope, release exclusive lock.
  @endcode
*/
class sdl_mutex_lock {
public:
  /**
    Acquire the exclusive lock for the mutex.

    @param  mutex is a pointer to the mutex associated with this
            exclusive lock
    @pre    <tt>NULL != mutex</tt>
    @pre    <tt>mutex</tt> is lockable with SDL_mutexP function
    @throw  std::logic_error if <tt>NULL == mutex</tt>
    @throw  std::runtime_error if <tt>0 != SDL_mutexP(mutex)</tt>,
            <tt>mutex</tt> is not lockable
  */
  explicit sdl_mutex_lock(SDL_mutex *mutex)
    : m_mutex(NULL)
  {
    if (NULL != mutex) {
      if (0 == SDL_mutexP(mutex)) {
        m_mutex = mutex;
      } else {
        throw std::runtime_error("failed to acquire exclusive lock");
      }
    } else {
      throw std::logic_error("invalid mutex object");
    }
  }
  /**
    Release the exclusive lock.
    
    Does not throw any exceptions.
  */
  ~sdl_mutex_lock()
  {
    SDL_mutex *mutex = m_mutex;
    m_mutex = NULL;

    if (NULL != mutex) { 
      // Do not check this. We could only throw an exception,
      // which we do not want to do in the destructor anyways.
      SDL_mutexV(mutex);
    }
  }
private:
  /**
    Store the locked mutex such that we can release it in the
    destructor.
  */
  SDL_mutex * m_mutex;

  /** Disable the copy-constructor. */
  sdl_mutex_lock(const sdl_mutex_lock &);

  /** Disable the assignment operator. */
  const sdl_mutex_lock & operator=(const sdl_mutex_lock &);
}; // class sdl_mutex_lock


/**
  Encapsulate the OpenGL and Motion::SDK::Client code for the SDL example
  application in this class. Implements most of the interface defined by the
  "javax.media.opengl.GLEventListener" class used in the Java SDK.

  Also, implements a function object that connects to the Motion Service Preview
  stream and reads real-time orientation data. This should run in its own
  thread. The ExampleSDL class handles synchronization of the transformation
  matrix. Note that if there are multiple sensors, this will only use the first
  one.
*/
class ExampleSDL {
public:
  ExampleSDL()
    : m_mutex(NULL), m_transform(4*4), m_euler(3), m_quit(false)
  {
    m_mutex = SDL_CreateMutex();

    for (int i=0; i<4; i++) {
      m_transform[4*i+i] = 1;
    }
  }

  ~ExampleSDL()
  {
    if (NULL != m_mutex) {
      SDL_DestroyMutex(m_mutex);
      m_mutex = NULL;
    }
  }

  /**
    OpenGL function, draw the current frame.
  */
  void display()
  {
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);

    {
      // Draw a fixed mono-chromatic reference coordinate
      // system of some sort.
      glColor3f(0.7f, 0.7f, 0.7f);
      glBegin(GL_LINES);
      {
        glVertex3f(-1, 0, 0);
        glVertex3f(1, 0, 0);
        glVertex3f(0, -1, 0);
        glVertex3f(0, 1, 0);
        glVertex3f(0, 0, -1);
        glVertex3f(0, 0, 1);
      }
      glEnd();
    }

    {
      // Transform a colored coordinate system by the current
      // orientation state of this object.
      glPushMatrix();
      {
        std::vector<float> transform;
        std::vector<float> euler;
        {
          sdl_mutex_lock lock(m_mutex);
          transform = m_transform;
          euler = m_euler;
        }

        // Matrix based transformation.
        if (4*4 == transform.size()) {
          // Transpose matrix for OpenGL column-major order.
          for (int i=0; i<4; i++) {
            for (int j=(i+1); j<4; j++) {
              float temp = transform[4*i+j];
              transform[4*i+j] = transform[4*j+i];
              transform[4*j+i] = temp;
            }
          }

          glMultMatrixf(&transform[0]);
        }

        const float axis[3][3] = {
          {1, 0, 0},
          {0, 1, 0},
          {0, 0, 1}
        };

        // Euler angle based transformation.
        //if (3 == euler.size()) {
        //  for (int i=2; i>=0; i--) {
        //    glRotatef(euler[i] * (180.0/3.14), axis[i][0], axis[i][1], axis[i][2]);
        //  }
        //}


        for (int i=0; i<3; i++) {
          glColor3fv(axis[i]);
          glBegin(GL_LINES);
          {
            glVertex3f(0, 0, 0);
            glVertex3fv(axis[i]);
          }
          glEnd();
        }
      }
      glPopMatrix();
    }
  }

  /**
    OpenGL function, initialize parameters.
  */
  void init()
  {
    glClearColor(0, 0, 0, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
  }

  /**
    OpenGL function, resize the parent window.
  */
  void reshape(int width, int height)
  {
    if (height <= 0) {
      height = 1;
    }
    float height_real = static_cast<float>(width) / static_cast<float>(height);

    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));

    glMatrixMode(GL_PROJECTION);
    {	
      glLoadIdentity();
      gluPerspective(60, height_real, 1, 1000);
    }

    glMatrixMode(GL_MODELVIEW);
    {
      glLoadIdentity();
      gluLookAt(-2, 2, 2, 0, 0, 0, 0, 1, 0);
    }
  }

  /**
    Set the quit flag for this thread to true.
  */
  void quit()
  {
    sdl_mutex_lock lock(m_mutex);
    m_quit = true;
  }

  /**
    Thread function. Make a connection to the remote Host:Port and read
    Preview data in a loop.
  */
  bool operator()()
  {
    using Motion::SDK::Client;
    using Motion::SDK::Format;

    try {
      // Open a connection to the Motion Service Preview stream running
      // on Host:Port.
      Client client(Host, Port);

      std::cout << "Connected to " << Host << ":" << Port << std::endl;

      bool quit_thread = false;
      while (!quit_thread) {

        // Make a check for the quit flag here.
        {
          sdl_mutex_lock lock(m_mutex);
          if (m_quit) {
            quit_thread = true;
          }
        }

        // Block on this call until a single data sample arrives from the
        // remote service. Use default time out of 5 seconds.
        if (client.waitForData()) {

          // The Client#readData method will time out after 1 second. Then
          // just go back to the blocking Client#waitForData method to wait
          // for more incoming data.
          Client::data_type data;
          while (!quit_thread && client.readData(data)) {

            // We have a message from the remote Preview service. Use the
            // Format::Preview method to create a std::map<integer,Format::PreviewElement>
            // of all active device connections.

            std::vector<float> transform;
            std::vector<float> euler;
            {
              // Iterate through Nodes, but just grab the first one.
              Format::preview_service_type preview = Format::Preview(data.begin(), data.end());
              for (Format::preview_service_type::iterator itr=preview.begin(); itr!=preview.end(); ++itr) {
                transform = itr->second.getMatrix(false);
                euler = itr->second.getEuler();
                break;
              }
            }

            // Copy the new transformation matrix into the mutex protected
            // state. The drawing thread reads this.
            {
              sdl_mutex_lock lock(m_mutex);
              if (transform.size() == m_transform.size()) {
                m_transform = transform;
              }
              if (euler.size() == m_euler.size()) {
                m_euler = euler;
              }
              if (m_quit) {
                quit_thread = true;
                break;
              }
            }
          }

        }

      }

    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
      return false;
    }

    
    std::cout << "Leaving client thread" << std::endl;

    return true;
  }

private:
  /**
    Mutex to synchronize internal state variables.
  */
  SDL_mutex *m_mutex;

  /**
    Current 4-by-4 transformation matrix.
  */
  std::vector<float> m_transform;
  std::vector<float> m_euler;

  /**
    Quit flag for the socket reading thread.
  */
  bool m_quit;

  /** Disable the copy constructor. */
  ExampleSDL(const ExampleSDL &);

  /** Disable assignment operator. */
  const ExampleSDL & operator=(const ExampleSDL &);
}; // class ExampleSDL


/**
  Entry point. Initialize the SDL subsystems and run the event
  loop.
*/
int main(int argc, char **argv)
{
  try {
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      throw std::runtime_error(std::string("failed to initialize SDL: ") + SDL_GetError());
    }

    const int width = 600;
    const int height = 600;
    const int bpp = 32;
    const Uint32 video_flags = SDL_OPENGL | SDL_RESIZABLE;

    {
      if (0 != SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)) {
        throw std::runtime_error(std::string("failed to set SDL OpenGL attribute: ") + SDL_GetError());
      }
    }

    if (NULL == SDL_SetVideoMode(width, height, bpp, video_flags)) {
      throw std::runtime_error(std::string("failed to set SDL video mode: ") + SDL_GetError());
    }

    // Set the Window Manager title and icon name.
    SDL_WM_SetCaption("Example", "example");


    // This class handles all of the OpenGL initialization and drawing code.
    ExampleSDL example;
    example.init();
    example.reshape(width, height);


    // Start a client thread.
    SDL_Thread * thread = SDL_CreateThread(&sdl_thread_functor<ExampleSDL>, &example);
    if (NULL == thread) {
      throw std::runtime_error("failed to create SDL thread");
    }


    Uint32 previous_sdl_tick = SDL_GetTicks();

    // SDL event loop. Run until we receive the SDL_QUIT message.
    bool run_event_loop = true;
    while (run_event_loop) {

      // Process the current event queue.
      SDL_Event event;
      while (SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_VIDEORESIZE:
          {
            SDL_Surface * screen = SDL_SetVideoMode(event.resize.w, event.resize.h, bpp, video_flags);
            if (NULL != screen) {
              example.reshape(screen->w, screen->h);
            } else {
              throw std::runtime_error(std::string("failed to set SDL video mode: ") + SDL_GetError());
            }
          }
          break;
        case SDL_QUIT:
          run_event_loop = false;
          break;
        default:
          break;
        }
      }


      example.display();

      SDL_GL_SwapBuffers();


      // Check for any outstanding OpenGL errors.
      {
        GLenum gl_error = glGetError();
        if (GL_NO_ERROR != gl_error) {
          throw std::runtime_error(std::string("OpenGL error: ") + reinterpret_cast<const char *>(gluErrorString(gl_error)));
        }
      }
      
      // Check for any outstanding SDL errors.
      {
        const char *sdl_error = SDL_GetError();
        if ('\0' != sdl_error[0]) {
          throw std::runtime_error(std::string("SDL error: ") + sdl_error);
          SDL_ClearError();
        }
      }

      // Put this thread to sleep if necessary to slow the event
      // loop down to the target frame rate.
      {
        Uint32 current_sdl_tick = SDL_GetTicks();

        Uint32 elapsed = 0;
        if (current_sdl_tick > previous_sdl_tick) {
          elapsed = current_sdl_tick - previous_sdl_tick;
        
          if (elapsed < TargetFrameRate) {
            SDL_Delay(TargetFrameRate - elapsed);
            current_sdl_tick = SDL_GetTicks();
          }
        }

        previous_sdl_tick = current_sdl_tick;
      }

    } // while (run_event_loop)

    // Optional cleanup code to gracefully close the communications
    // thread. We do not wait by default, just let it evaporate.
    //example.quit();
    //if (NULL != thread) {
    //  SDL_WaitThread(thread, NULL);
    //}

  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }

  SDL_Quit();

  return 0;
}
