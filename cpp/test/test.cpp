/**
  Simple test program for the C++ Motion SDK.

  @file    tools/sdk/cpp/test/test.cpp
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
#include <LuaConsole.hpp>
#include <File.hpp>
#include <Format.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>


// Defaults to "127.0.0.1"
const std::string Host = "";
// Use 32079 for preview data service.
// Use 32078 for sensor data service
// Use 32077 for raw data service.
// Use 32076 for configurable data service.
// Use 32075 for console service.
const unsigned PortPreview = 32079;
const unsigned PortSensor = 32078;
const unsigned PortRaw = 32077;
const unsigned PortConfigurable = 32076;
const unsigned PortConsole = 32075;

// Read this many samples in our test loops.
const std::size_t NSample = 100;


int test_Configurable(const std::string &host, const unsigned &port)
{
  try {
    using Motion::SDK::Client;
    using Motion::SDK::Format;

    // Open connection to the data server.
    Client client(host, port);
    std::cout << "Connected to " << host << ":" << port << std::endl;

    // The Configurable data service requires an XML definition of the
    // requested channel names.
    {
      Client::data_type xml_definition;
      {
        std::ifstream fin("../../test_data/configurable.xml",
                          std::ios_base::binary | std::ios_base::in);
        if (fin.is_open()) {
          fin.seekg(0, std::ios_base::end);
          int num_bytes = fin.tellg();
          fin.seekg(0, std::ios_base::beg);

          if (num_bytes > 0) {
            xml_definition.resize(num_bytes);
            if (fin.read(&xml_definition[0], num_bytes)) {
            } else {
              xml_definition.clear();
            }
          }
        }
      }

      // Make a default definition here, in case we could not find our file.
      // Access the global quaternion and calibrated accelerometer streams.
      if (xml_definition.empty()) {
        std::string xml_string =
          "<?xml version=\"1.0\"?>"
          "<configurable>"
          "<preview><Gq/></preview>"
          "<sensor><a/></sensor>"
          "</configurable>";
        xml_definition.assign(xml_string.begin(), xml_string.end());
      }

      if (!xml_definition.empty() && client.writeData(xml_definition)) {
        std::cout
          << "Sent active channel definition to Configurable service"
          << std::endl;
      }
    }

    if (client.waitForData()) {
      std::size_t sample_count = 0;

      // Read data samples in a loop. This is a blocking call so
      // we can simply wait on an open connection until a data
      // sample comes in.
      Client::data_type data;
      while ((sample_count++ < NSample) && client.readData(data)) {
        typedef Format::configurable_service_type map_type;

        map_type container = Format::Configurable(data.begin(), data.end());
        if (!container.empty()) {
          std::cout
            << Format::ConfigurableElement::Name << ": " << container.size();

          for (map_type::iterator itr=container.begin(); itr!=container.end();
               ++itr) {
            std::cout << " data(" << itr->first << ") = ";
            for (std::size_t i=0; i<itr->second.size(); i++) {
              std::cout << itr->second[i] << " ";
            }

            std::cout << std::endl;
          }
        }

        std::cout << std::endl;
      }
    } else {
      std::cout << "No current data available, giving up" << std::endl;
    }

    std::string message;
    if (client.getErrorString(message)) {
      std::cerr << "Error: " << message << std::endl;
    }

  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}

int test_Client(const std::string &host, const unsigned &port)
{
  try {
    using Motion::SDK::Client;
    using Motion::SDK::Format;

    // Open connection to the data server.
    Client client(host, port);
    std::cout << "Connected to " << host << ":" << port << std::endl;

    if (client.waitForData()) {
      std::size_t sample_count = 0;

      // Read data samples in a loop. This is a blocking call so
      // we can simply wait on an open connection until a data
      // sample comes in.
      Client::data_type data;
      while ((sample_count++ < NSample) && client.readData(data)) {

        if (PortPreview == port) {
          typedef Format::preview_service_type map_type;

          map_type preview = Format::Preview(data.begin(), data.end());
          if (!preview.empty()) {
            std::cout << Format::PreviewElement::Name << ": " << preview.size();

            for (map_type::iterator itr=preview.begin(); itr!=preview.end();
                 ++itr) {
              Format::PreviewElement::data_type q =
                itr->second.getQuaternion(false);
              std::cout
                << " q(" << itr->first << ") = (" << q[0] << ", " << q[1]
                << ", " << q[2] << ", " << q[3] << ")"
                << std::endl;
            }
          }
        }


        if (PortSensor == port) {
          typedef Format::sensor_service_type map_type;

          map_type sensor = Format::Sensor(data.begin(), data.end());
          if (!sensor.empty()) {
            std::cout << Format::SensorElement::Name << ": " << sensor.size();

            for (map_type::iterator itr=sensor.begin(); itr!=sensor.end();
                 ++itr) {
              Format::SensorElement::data_type acc =
                itr->second.getAccelerometer();
              std::cout
                << " a(" << itr->first << ") = " << acc[0] << " " << acc[1]
                << " " << acc[2]
                << std::endl;
            }
          }
        }


        if (PortRaw == port) {
          typedef Format::raw_service_type map_type;
          
          map_type raw = Format::Raw(data.begin(), data.end());
          if (!raw.empty()) {
            std::cout << Format::RawElement::Name << ": "<< raw.size();

            for (map_type::iterator itr=raw.begin(); itr!=raw.end(); ++itr) {
              Format::RawElement::data_type acc =
                itr->second.getAccelerometer();
              std::cout
                << " a(" << itr->first << ") = " << acc[0] << " " << acc[1]
                << " " << acc[2]
                << std::endl;
            }
          }
        }

      }
    } else {
      std::cout << "No current data available, giving up" << std::endl;
    }

    std::string message;
    if (client.getErrorString(message)) {
      std::cerr << "Error: " << message << std::endl;
    }

  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}

int test_LuaConsole(const std::string &host, const unsigned &port)
{
  try {
    using Motion::SDK::Client;
    using Motion::SDK::LuaConsole;

    // Connect to the Console data service.
    Client client(host, port);
    std::cout << "Connected to " << host << ":" << port << std::endl;

    // Scan for devices and start reading.
    std::string lua_chunk =
      "if not node.is_reading() then"
      "   node.close()"
      "   node.scan()"
      "   node.start()"
      " end"
      " if node.is_reading() then"
      "   print('Reading from ' .. node.num_reading() .. ' device(s)')"
      " else"
      "   print('Failed to start reading')"
      " end"
      ;

    LuaConsole::result_type result = LuaConsole::SendChunk(client, lua_chunk);
    if (LuaConsole::Success == result.first) {
      std::cout << result.second;
    } else if (LuaConsole::Continue == result.first) {
      std::cerr << "incomplete Lua chunk: " << result.second << std::endl;
    } else {
      std::cerr << "command failed: " << result.second << std::endl;
    }

    std::string message;
    if (client.getErrorString(message)) {
      std::cerr << "Error: " << message << std::endl;
    }

  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}

int test_File()
{
  int result = 0;

  try {
    using Motion::SDK::File;
    using Motion::SDK::Format;

    File file("../../test_data/raw.bin");

    Format::RawElement::data_type data(Format::RawElement::Length);
    while (file.readData(data)) {
      // Access the data directly.
      std::copy(
        data.begin(), data.end(),
        std::ostream_iterator<short>(std::cout, " "));
      std::cout << std::endl;


      // Or wrap the data in the associated Format class.
      Format::RawElement element(data);

      // Read the three vector of magnetometer data.
      Format::RawElement::data_type magnetometer =
        element.getMagnetometer();
    }

  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    result = 1;
  }

  try {
    using Motion::SDK::File;
    using Motion::SDK::Format;

    File file("../../test_data/sensor.bin");

    Format::SensorElement::data_type data(Format::SensorElement::Length);
    while (file.readData(data)) {
      // Access the data directly.
      std::copy(
        data.begin(), data.end(),
        std::ostream_iterator<float>(std::cout, " "));
      std::cout << std::endl;


      // Or wrap the data in the associated Format class.
      Format::SensorElement element(data);

      // Read the three vector of magnetometer data.
      Format::SensorElement::data_type magnetometer =
        element.getMagnetometer();
    }

  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
    result = 1;
  }

  return result;
}

int main(int argc, char **argv)
{
  // Choose a remote host on the command line.
  // Note that this must be an IP address or
  // you will need to add a hostname lookup.
  std::string host;
  if (argc > 1) {
    host.assign(argv[1]);
  }

  // LuaConsole class allows for remote control of the
  // Motion Service. Run this test first since it
  // will start reading from any available sensors.
  test_LuaConsole(host, PortConsole);

  // Configurable data service. New feature that
  // allows all streams types to be read from one
  // Client connection. The user selects that active
  // channels at connect time.
  test_Configurable(host, PortConfigurable);

  // Regular SDK data stream access. Each service has
  // its own port. Run a test of all three main data
  // services.
  test_Client(host, PortPreview);
  test_Client(host, PortSensor);
  test_Client(host, PortRaw);

  // File class reads binary take files.
  test_File();

  return 0;
}
