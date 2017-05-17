#
# Simple test program for the Python Motion SDK.
#
# @file    sdk/python/test.py
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
from MotionSDK import *

PortPreview = 32079
PortSensor = 32078
PortRaw = 32077
PortConfigurable = 32076
PortConsole = 32075
NSample = 10


def test_Client(host, port):
    client = Client(host, port)

    print("Connected to %s:%d" % (host, port))

    if PortConfigurable == port:
        xml_string = \
            "<?xml version=\"1.0\"?>" \
            "<configurable>" \
            "<preview><Gq/></preview><sensor><a/></sensor>" \
            "</configurable>"

        if client.writeData(xml_string):
            print("Sent active channel definition to Configurable service")

    if client.waitForData():
        sample_count = 0
        while sample_count < NSample:
            data = client.readData()
            if None == data:
                break

            if PortPreview == port:
                container = Format.Preview(data)
                for key in container:
                    Gq = container[key].getQuaternion(False)

                    print("Gq({}) = ({}, {}i, {}j, {}k)".format(key, *Gq))

            if PortSensor == port:
                container = Format.Sensor(data)
                for key in container:
                    a = container[key].getAccelerometer()

                    print("a({}) = ({}, {}, {})".format(key, *a))

            if PortRaw == port:
                container = Format.Raw(data)
                for key in container:
                    A = container[key].getAccelerometer()

                    print("A({}) = ({}, {}, {})".format(key, *A))

            if PortConfigurable == port:
                container = Format.Configurable(data)
                for key in container:
                    line = "data({}) = (".format(key)
                    for i in range(container[key].size()):
                        if i > 0:
                            line += ", "
                        line += "{}".format(container[key].value(i))
                    line += ")"

                    print(line)

            sample_count += 1


def test_LuaConsole(host, port):
    client = Client(host, port)

    print("Connected to %s:%d" % (host, port))

    #
    # General Lua scripting interface.
    #
    lua_chunk = \
      "if not node.is_reading() then" \
      "   node.close()" \
      "   node.scan()" \
      "   node.start()" \
      " end" \
      " if node.is_reading() then" \
      "   print('Reading from ' .. node.num_reading() .. ' device(s)')" \
      " else" \
      "   print('Failed to start reading')" \
      " end"

    print(LuaConsole.SendChunk(client, lua_chunk, 5))

    # Scripting language compatibility class. Translate Python calls into Lua
    # calls and send them to the console service.
    node = LuaConsole.Node(client)
    print("node.is_reading() = {}".format(node.is_reading()))


def test_File():
    filename = "../../test_data/sensor.bin";

    print("reading take data filename = \"{}\"".format(filename))

    take_file = File(filename)
    while True:
        data = take_file.readData(9, True)
        if None == data:
            break

        print(Format.SensorElement(data).getAccelerometer())


def main(argv):
    # Set the default host name parameter. The SDK is socket based so any
    # networked Motion Service is available.
    host = ""
    if len(argv) > 1:
        host = argv[1]

    test_LuaConsole(host, PortConsole)

    test_Client(host, PortPreview)
    test_Client(host, PortSensor)
    test_Client(host, PortRaw)
    test_Client(host, PortConfigurable)

    # Requires a data file. Do not test by default.
    #test_File()

if __name__ == "__main__":
    sys.exit(main(sys.argv))
