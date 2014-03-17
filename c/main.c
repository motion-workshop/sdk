/**
  @file    tools/sdk/c/main.c
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

/**
  Win32/BSD/Posix C example for direct socket connection to a Motion Service
  data stream. Self contained, using only basic libraries to do the work.
*/

#if defined(_WIN32)
#  include <winsock2.h>
#else
#  include <arpa/inet.h>
#  include <netinet/in.h>
#  include <string.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

#include <stdio.h>


/**
  Make some Windows/Linux compatibility functions and definitions.
*/
#if defined(_WIN32)

int close(int fd)
{
  return closesocket(fd);
}

int inet_pton(int af, const char *src, struct in_addr *dst)
{
  unsigned long result = 0;
  if (AF_INET == af) {
    result = inet_addr(src);
    if (INADDR_NONE == result) {
      return 0;
    } else{
      dst->s_addr = result;
      return 1;
    }
  } else {
    return 0;
  }
}

#endif

#if !defined(MSG_NOSIGNAL)
#  define MSG_NOSIGNAL 0
#endif

/**
  Constants for run-time configuration.
*/
#define DestinationAddress  "127.0.0.1"
#define DestinationPort     32079
#define BufferSize          1024


/**
  Main function. Initiate a TCP socket connection to the Motion Service on the
  localhost. Read back a real-time data stream in either Sensor or Preview
  format. Note that the port number (32079, 32078) defines which stream we
  attach to. Print out results as they arrive to the standard output.
*/
int main(int argc, char **argv)
{
  int result;
  int fd;
  struct sockaddr_in address;
  int length;
  char buffer[BufferSize];
  float * fp;

#if defined(_WIN32)
  /**
    Winsock. Must call the initialization function before we do anything.
  */
  WSADATA wsaData;
  if (0 != WSAStartup(MAKEWORD(1, 1), &wsaData)) {
    fprintf(stderr, "Winsock initialization failed\n");

    return -1;
  }
#endif

  /**
    Create a socket descriptor for a TCP stream.
  */
  fd = (int)socket(AF_INET, SOCK_STREAM, 0);
  if (fd > 0) {
    /**
      Fill the destination address.
    */
    address.sin_family = AF_INET;
    address.sin_port = htons(DestinationPort);
    memset(&address.sin_zero, 0, 8);
    inet_pton(AF_INET, DestinationAddress, &address.sin_addr);

    /**
      Connect to remote address. Motion Service data stream.
    */
    result = connect(fd, (struct sockaddr *)&address, sizeof(struct sockaddr));
    if (0 == result) {
      printf("Connected to %s:%d\n", DestinationAddress, DestinationPort);

      /**
        Socket read loop. Read the message length field followed by the binary
        data payload.

        Length field in network byte order (big-endian). Data in little-endian
        format. Real values are all 32-bit, single precision, floating point
        format.

        [length] [data ... ]
      */
      for (;;) {
        result = recv(fd, (char *)&length, sizeof(length), MSG_NOSIGNAL);
        if (sizeof(length) != result) {
          fprintf(
            stderr,
            "failed to read message length header, received %d of %d bytes\n",
            result, sizeof(length));

          break;
        }

        /**
          Message length field is in network byte order.
        */
        length = ntohl(length);
        
        /**
          Safety check. We should not receive any empty or really long
          messages.
        */
        if ((length <= 0) || (length >= BufferSize)) {
          fprintf(stderr, "invalid incoming message length: %d\n", length);

          break;
        }

        /**
          Zero out the message buffer.
        */
        memset(buffer, 0, sizeof(buffer));

        /**
          Read the data payload.
        */
        result = recv(fd, buffer, length, MSG_NOSIGNAL);
        if (length == result) {
          if ((length >= 5) && (0 == memcmp("<?xml", buffer, 5))) {
            /**
              XML string message. Does not contain stream data.
            */
          } else if (0 == length % (sizeof(int) + 14*sizeof(float))) {
            /**
              Preview data comes after the integer identifier for
              the first node.

              [id] [14 float] ...
            */
            fp = (float *)&buffer[sizeof(int)];
            printf("Euler = %f, %f, %f rad\n", fp[8], fp[9], fp[10]);
          } else if (0 == length % (sizeof(int) + 9*sizeof(float))) {
            /**
              Sensor data comes after the integer identifier for
              the first node.

              [id] [9 float] ...
            */
            fp = (float *)&buffer[sizeof(int)];
            printf("Accelerometer = %f, %f, %f g\n", fp[0], fp[1], fp[2]);
          } else {
            /**
              Unknown packet.
            */
            fprintf(stderr, "unknown message length, %d bytes\n", length);
          }
        } else {
          /**
            Socket communication error. Probably timed out or lost remote
            connection.
          */
          fprintf(
            stderr,
            "failed to read incoming message, received %d of %d bytes\n",
            result, length);
        }
      }

    } else {
      fprintf(stderr, "failed to connect TCP socket to endpoint\n");
    }

    close(fd);
    fd = 0;
  } else {
    fprintf(stderr, "failed to create TCP socket\n");
  }

#if defined(_WIN32)
  WSACleanup();
#endif

  return 0;
}
