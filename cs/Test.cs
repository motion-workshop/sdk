/**
  @file    tools/sdk/cs/Test.cs
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
using System;
using System.Collections.Generic;
using System.Text;

using Motion.SDK;

namespace Test
{
  /**
   Test functions for the Motion SDK.
   */
  class Test
  {
    // Use 32079 for preview data service.
    // Use 32078 for sensor data service
    // Use 32077 for raw data service.
    // Use 32076 for configurable data service.
    // Use 32075 for console service.
    const int PortPreview = 32079;
    const int PortSensor = 32078;
    const int PortRaw = 32077;
    const int PortConfigurable = 32076;
    const int PortConsole = 32075;

    // Read this many samples in our test loops.
    const int NSample = 10;


    static void test_LuaConsole(String host, int port)
    {
      try
      {
        Client client = new Client(host, port);

        Console.WriteLine("Connected to " + host + ":" + port);

        String chunk =
          "if not node.is_reading() then" +
          "   node.close()" +
          "   node.scan()" +
          "   node.start()" +
          " end" +
          " if node.is_reading() then" +
          "   print('Reading from ' .. node.num_reading() .. ' device(s)')" +
          " else" +
          "   print('Failed to start reading')" +
          " end";

        LuaConsole.ResultType result =
          LuaConsole.SendChunk(client, chunk);

        if (LuaConsole.ResultCode.Success == result.first)
        {
          Console.WriteLine(result.second);
        }
        else if (LuaConsole.ResultCode.Continue == result.first)
        {
          Console.WriteLine("incomplete Lua chunk: " + result.second);
        }
        else
        {
          Console.WriteLine("command failed: " + result.second);
        }
      }
      catch (Exception e)
      {
        Console.WriteLine(e.ToString());
      }
    }
    
    static void test_Client(String host, int port)
    {
      try
      {
        Client client = new Client(host, port);

        Console.WriteLine("Connected to " + host + ":" + port);

        // The Configurable data service requires that we specify a list of channels
        // that we would like to receive.
        if (PortConfigurable == port)
        {
          // Access the global quaternion and calibrated accelerometer streams.
          String xml_definition =
              "<?xml version=\"1.0\"?>" +
              "<configurable>" +
              "<preview><Gq/></preview>" +
              "<sensor><a/></sensor>" +
              "</configurable>";

          if (client.writeData(xml_definition))
          {
            Console.WriteLine("Sent active channel definition to Configurable service");
          }
        }

        if (client.waitForData())
        {
          int sample_count = 0;
          while (true)
          {
            byte[] data = client.readData();
            if ((null == data) || (sample_count++ >= NSample))
            {
              break;
            }

            if (PortPreview == port)
            {
              IDictionary<int, Format.PreviewElement> container = Format.Preview(data);
              foreach (KeyValuePair<int, Format.PreviewElement> itr in container)
              {
                float[] q = itr.Value.getQuaternion(false);
                Console.WriteLine("q(" + itr.Key + ") = (" + q[0] + ", " + q[1] + "i, " + q[2] + "j, " + q[3] + "k)");
              }
            }

            if (PortSensor == port)
            {
              IDictionary<int, Format.SensorElement> container = Format.Sensor(data);
              foreach (KeyValuePair<int, Format.SensorElement> itr in container)
              {
                float[] a = itr.Value.getAccelerometer();
                Console.WriteLine("a(" + itr.Key + ") = (" + a[0] + ", " + a[1] + ", " + a[2] + ") g");
              }
            }

            if (PortRaw == port)
            {
              IDictionary<int, Format.RawElement> container = Format.Raw(data);
              foreach (KeyValuePair<int, Format.RawElement> itr in container)
              {
                short[] a = itr.Value.getAccelerometer();
                Console.WriteLine("a(" + itr.Key + ") = (" + a[0] + ", " + a[1] + ", " + a[2] + ") g");
              }
            }

            if (PortConfigurable == port)
            {
              IDictionary<int, Format.ConfigurableElement> container = Format.Configurable(data);
              foreach (KeyValuePair<int, Format.ConfigurableElement> itr in container)
              {
                Console.Write("data(" + itr.Key + ") = (");
                for (int i=0; i<itr.Value.size(); i++)
                {
                  if (i > 0)
                  {
                    Console.Write(", ");
                  }
                  Console.Write(itr.Value.value(i));
                }
                Console.WriteLine(")");
              }
            }

          }
        }
      }
      catch (Exception e)
      {
        Console.WriteLine(e.ToString());
      }
    }

    static void test_File()
    {
      try
      {
        String filename = "../../../../test_data/output.bin";
        Console.WriteLine("test_File, reading data file: \"" + filename + "\"");
        
        File file = new File(filename);
        int i = 0;
        while (i++ < 10)
        {
          float[] x = file.readOutputData();
          if (null == x)
          {
            break;
          }
          else
          {
            Console.WriteLine("q(" + i + ") = <" + x[0] + ", " + x[1] + "i, " + x[2] + "j, " + x[3] + "k>");
          }
        }

        file.close();
      }
      catch (Exception e)
      {
        Console.WriteLine(e.ToString());
      }
    }

    static void Main(string[] args)
    {
      String host = "";
      if (args.Length > 0)
      {
        host = args[0];
      }

      test_LuaConsole(host, PortConsole);

      test_Client(host, PortPreview);
      test_Client(host, PortSensor);
      test_Client(host, PortRaw);
      test_Client(host, PortConfigurable);
      
      //test_File();
    }
  }
}
