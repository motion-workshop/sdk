/**
   Simple test program for the Java Motion SDK.

   @file    tools/sdk/java/test/Test.java
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
import Motion.SDK.Client;
import Motion.SDK.File;
import Motion.SDK.Format;
import Motion.SDK.LuaConsole;

import java.io.IOException;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.Iterator;
import java.util.Map;
import java.util.Vector;


class Test {
    // Use 32079 for preview data service.
    // Use 32078 for sensor data service
    // Use 32077 for raw data service.
    // Use 32076 for configurable data service.
    // Use 32075 for console service.
    public static final int PortPreview = 32079;
    public static final int PortSensor = 32078;
    public static final int PortRaw = 32077;
    public static final int PortConfigurable = 32076;
    public static final int PortConsole = 32075;

    // Read this many samples in our test loops.
    public static final int NSample = 10;


    public static void test_Client(String host, int port)
        throws IOException
    {
        Client client = new Client(host, port);

        System.out.println("Connected to " + host + ":" + port);


        // The Configurable data service requires that we specify a list of channels
        // that we would like to receive.
        if (PortConfigurable == port) {
            // Access the global quaternion and calibrated accelerometer streams.
            String xml_definition =
                "<?xml version=\"1.0\"?>" +
                "<configurable>" +
                "<preview><Gq/></preview>" +
                "<sensor><a/></sensor>" +
                "</configurable>";

            if (client.writeData(ByteBuffer.wrap(xml_definition.getBytes()))) {
                System.out.println("Sent active channel definition to Configurable service");
            }
        }


        if (client.waitForData()) {
            int sample_count = 0;

            // Read data samples in a loop. Client.readData is a blocking call
            // so we can simply wait on an open connection until a data
            // sample comes in.
            while (true) {
                ByteBuffer data = client.readData();
                if ((null == data) || (sample_count++ >= NSample)) {
                    break;
                }

                if (PortPreview == port) {
                    Map<Integer,Format.PreviewElement> container = Format.Preview(data);
                    for (Map.Entry<Integer,Format.PreviewElement> entry: container.entrySet()) {
                        float[] q = entry.getValue().getQuaternion(false);
                        System.out.println("q(" + entry.getKey() + ") = (" + q[0] + ", " + q[1] + "i, " + q[2] + "j, " + q[3] + "k)");
                    }
                } 

                if (PortSensor == port) {
                    Map<Integer,Format.SensorElement> container = Format.Sensor(data);
                    for (Map.Entry<Integer,Format.SensorElement> entry: container.entrySet()) {
                        float[] a = entry.getValue().getAccelerometer();
                        System.out.println("a(" + entry.getKey() + ") = (" + a[0] + ", " + a[1] + ", " + a[2] + ") g");
                    }
                }

                if (PortRaw == port) {
                    Map<Integer,Format.RawElement> container = Format.Raw(data);
                    for (Map.Entry<Integer,Format.RawElement> entry: container.entrySet()) {
                        short[] a = entry.getValue().getAccelerometer();
                        System.out.println("a(" + entry.getKey() + ") = (" + a[0] + ", " + a[1] + ", " + a[2] + ")");
                    }
                }

                if (PortConfigurable == port) {
                    Map<Integer,Format.ConfigurableElement> container = Format.Configurable(data);
                    for (Map.Entry<Integer,Format.ConfigurableElement> entry: container.entrySet()) {
                        System.out.print("data(" + entry.getKey() + ") = (");
                        for (int i=0; i<entry.getValue().size(); i++) {
                            if (i > 0) {
                                System.out.print(", ");
                            }
                            System.out.print(entry.getValue().value(i));
                        }
                        System.out.println(")");
                    }
                }
            }
        } else {
            System.err.println("No current data available, giving up");
        }
    }


    public static void test_LuaConsole(String host, int port)
        throws Exception, IOException
    {
        Client client = new Client(host, port);

        System.out.println("Connected to " + host + ":" + port);

        // Create a LuaConsole object with our existing connection.
        LuaConsole console = new LuaConsole(client);

        // Make a Lua chunk, which is just a number of Lua commands
        // that are compiled as one unit. We can only receive the
        // printed results as feedback.
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


        LuaConsole.Result result = console.SendChunk(chunk, 5);
        if (LuaConsole.ResultCode.Success == result.code) {
            // This will be the printed output.
            System.out.println(result.string);
        } else if (LuaConsole.ResultCode.Continue == result.code) {
            System.err.println("incomplete Lua chunk: " + result.string);
        } else {
            System.err.println("command failed: " + result.string);
        }
    }

    public static void test_File()
        throws IOException
    {
        File file = new File("../../../test_data/sensor.bin");

        while (true)
        {
            Vector<Float> data = file.readFloatData(9);
            if (null == data)
            {
                        break;
            }

            System.out.println("Incoming message, size = " + data.size());
        }
    }



    /**
       Entry point. Initialize a window with an OpenGL GLCanvas child
       object. Start client thread and main event loop.
    */
    public static void main(String[] args)
        throws Exception
    {
        String host = new String();

        // Copy a single command line argument that defines
        // the remote host.
        // For example, "www.google.com" or "10.0.0.1"
        //
        if (args.length > 0)
        {
            host = args[0];
        }

        test_LuaConsole(host, PortConsole);

        test_Client(host, PortPreview);
        test_Client(host, PortSensor);
        test_Client(host, PortRaw);
        test_Client(host, PortConfigurable);

        test_File();
    }
}
