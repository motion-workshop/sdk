Motion Software Development Kit (SDK)
=======

The Motion Software Development Kit (SDK) is a collection of classes that
provides real-time access to the outputs of the Motion Service. This includes
orientation output as well as the raw and calibrated accelerometer, gyroscope,
and magnetometer sensor signals. The SDK is open source and available in the
C++, C#, Java, JavaScript, and Python programming languages.

The Motion Service publishes data over network sockets. The SDK includes classes
to handle the transport of messages and to unpack the messages into language
native containers. Every platform and operating system has sockets and they are
very fast. By leveraging this standard technology, our SDK can be used anywhere.

The SDK is not required to access data from Motion inertial tracking system. It
is intended to simplify development of third-party applications.
