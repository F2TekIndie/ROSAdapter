# ROSAdapter

ROS 2 gRPC server for ROSVis. Requires C++20, ROS 2 Jazzy (ament_cmake,
rclcpp, std_msgs), protobuf and gRPC C++ development tools.

```sh
source /opt/ros/jazzy/setup.bash
cmake -S . -B build/local
cmake --build build/local --parallel
./build/local/rosadapter
# Optional address override:
./build/local/rosadapter --ros-args -p grpc_address:=127.0.0.1:8081
```

Defaults to 127.0.0.1:8080 using insecure gRPC credentials. Stop with Ctrl+C.
A long-lived ROS node spins alongside the gRPC server. A mutex protects the
in-memory topic registry, subscriptions and latest protobuf samples.

ListTopics refreshes discovery while retaining active subscriptions, even if a
publisher disappears. Subscribe requires the full advertised ROS type name;
the shared ROSData bundle currently covers 143 standard Jazzy message types.
Conversion uses ROS runtime introspection and generated protobuf definitions.
Types without a generated schema return UNIMPLEMENTED; unavailable or mismatched
runtime type support returns FAILED_PRECONDITION. Repeating Subscribe for the same type
is idempotent; changing its type requires Unsubscribe first.

Subscriptions belong to the adapter, persist across client disconnects and are
shared by clients. Unsubscribe removes the subscription and cached sample;
repeating it for a known topic succeeds. A new subscription starts at sequence
zero. GetLatest reports FAILED_PRECONDITION for unsubscribed topics. Before the
first sample its payload is unset. Only the latest sample is retained, without
history or server streaming. QoS is volatile, best effort, keep-last 1, allowing
both reliable and best-effort publishers. Discovery may initially be incomplete.

See ../ROSData/adapter.proto for the complete wire contract. The old custom TCP
framing, console commands and remote process-exit operation have been removed.

## Finding gRPC

On Ubuntu/Debian install the development libraries and code generator:

```sh
sudo apt install libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev protobuf-compiler pkg-config
```

CMake first tries the gRPC CMake package, then `pkg-config grpc++`. For a custom
installation, pass `-DCMAKE_PREFIX_PATH=/path/to/install` or
`-DgRPC_DIR=/path/to/directory/containing/gRPCConfig.cmake`. The code generator
can also be specified with `-DGRPC_CPP_PLUGIN_EXECUTABLE=/path/to/grpc_cpp_plugin`.
Re-run CMake after installing dependencies.

## Converter tests

```sh
source /opt/ros/jazzy/setup.bash
cmake -S . -B build/local -DBUILD_TESTING=ON
cmake --build build/local --parallel
ctest --test-dir build/local --output-on-failure
```

Tests exercise all 143 bundled types, nonzero image/point-cloud/array data,
conversion limits and live ROS-to-gRPC subscription/cache replacement. The live
test uses an isolated ROS domain and a temporary localhost gRPC port.

ListSupportedTypes returns bundled schema names. ListTopics marks advertised
supported_types. GetSchema exports protobuf descriptors for a requested ROS type.
LatestData now contains a protobuf Any. Failed conversions expose conversion_error
and retain the last valid payload. See ../ROSData/README.md for custom packages.
