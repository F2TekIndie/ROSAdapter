# ROSAdapter

**Status: proof-of-concept prototype.** This project is for experimentation and
learning. Features are incomplete, interfaces may change, and it is not
production-ready.

C++ bridge between a ROS 2 environment and the ROSVis desktop prototype.
It queries the ROS graph and exchanges Protocol Buffer messages over TCP.
The intended deployment is one ROSAdapter connected to one ROSVis client.

## Current functionality

- Query ROS topic names and their message types.
- Return topic queries over TCP, preserving the request ID.
- Accept local `TopicsQuery` and `Exit` console commands.
- Accept a remote exit request.

Subscription helpers exist for string and Int32 messages, but subscription
commands and continuous topic-data forwarding are not implemented end to end.

## Related projects

- [ROSVis](../ROSVis/README.md): Qt/QML desktop client.
- [ROSData](../ROSData/README.md): shared protocol definitions.

Keep the repositories alongside one another; the application builds include
`../ROSData` directly:

```text
ROS2Visualizer/
├── ROSAdapter/
├── ROSData/
└── ROSVis/
```

## Dependencies and build

- CMake 3.16 or later and a C++20 compiler.
- ROS 2 Jazzy with `ament_cmake`, `rclcpp`, and `std_msgs`.
- Boost.System with Asio coroutine and experimental channel support.
- Protocol Buffers compiler (`protoc`) and C++ development library.
- Platform threading support.

The current build contains ROS paths under `/opt/ros/jazzy`. Console input uses
POSIX file descriptors, making Linux the current development target.

From this repository, in your ROS development environment:

```sh
source /opt/ros/jazzy/setup.bash
cmake -S . -B build/local -DCMAKE_BUILD_TYPE=Debug
cmake --build build/local --parallel
./build/local/rosadapter config.json
```

Exactly one configuration-path argument is currently required. The loader is a
stub and ignores that path; a configuration file does not need to exist yet.

## Connection and protocol

The server listens on IPv4 port **8080**, on all local interfaces. Start it
before ROSVis. Stop any old ROSAdapter process using the port before launching
another instance.

Frames contain a four-byte unsigned payload length in big-endian order,
followed by a serialized `robotics::Message`. The payload limit is 16 MiB.
There is no newline separator for network messages.

## Prototype limitations

Configuration, reconnect handling, error reporting, and subscription execution
still need work. Topic queries create a temporary ROS node, so results may be
incomplete while discovery is taking place. Transport authentication and
encryption are not implemented. Use this prototype in a trusted development
network. No automated test suite is provided.
