#include "rosdata/converter.h"
#include "ros_messages.pb.h"
#include <rclcpp/rclcpp.hpp>
#include <rclcpp/serialization.hpp>
#include <rclcpp/typesupport_helpers.hpp>
#include <rosidl_typesupport_introspection_cpp/message_introspection.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/joint_state.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <std_msgs/msg/int8.hpp>
#include <std_msgs/msg/byte.hpp>
#include <std_msgs/msg/u_int64.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>



//
namespace{
    void require( bool condition, const std::string& message ){
        if ( !condition ){
            throw std::runtime_error( message );
        }
    }

    template <typename T>
    void roundTrip( const std::string& type, const T& source ){
        rclcpp::Serialization<T> serialization;
        rclcpp::SerializedMessage serialized;
        serialization.serialize_message( &source, &serialized );
        rosdata::Converter converter( type );
        auto protobuf = converter.toProtobuf( serialized );
        auto restored = converter.toSerialized( protobuf );
        T result;
        serialization.deserialize_message( &restored, &result );
        require( source == result, "Round trip failed for " + type );
    }

    void defaultRoundTrips(){
        rosdata::SchemaRegistry schemas;
        for ( const auto& type : schemas.supportedTypes() ){
            auto library = rclcpp::get_typesupport_library( type, "rosidl_typesupport_introspection_cpp" );
            auto* support = rclcpp::get_message_typesupport_handle( type, "rosidl_typesupport_introspection_cpp", *library );
            const auto* members = static_cast<const rosidl_typesupport_introspection_cpp::MessageMembers*>( support->data );
            void* value = ::operator new( members->size_of_ );
            members->init_function( value, rosidl_runtime_cpp::MessageInitialization::ALL );
            try{
                auto serializationLibrary = rclcpp::get_typesupport_library( type, "rosidl_typesupport_cpp" );
                auto* serializationSupport = rclcpp::get_message_typesupport_handle( type, "rosidl_typesupport_cpp", *serializationLibrary );
                rclcpp::SerializationBase serialization( serializationSupport );
                rclcpp::SerializedMessage sample;
                serialization.serialize_message( value, &sample );
                rosdata::Converter converter( type, schemas );
                auto first = converter.toProtobuf( sample );
                auto restored = converter.toSerialized( first );
                auto second = converter.toProtobuf( restored );
                require( first.type_url() == second.type_url() && first.value() == second.value(), "Default round trip failed for " + type );
                std::cout << "Verified " << type << std::endl;
            }
            catch ( ... ){
                members->fini_function( value );
                ::operator delete( value );
                throw;
            }
            members->fini_function( value );
            ::operator delete( value );
        }
    }

    void invalidValues(){
        robotics::ros::std_msgs__msg__Int8 invalid;
        invalid.set_data( 128 );
        google::protobuf::Any payload;
        payload.PackFrom( invalid );
        bool rejected = false;
        try{
            rosdata::Converter( "std_msgs/msg/Int8" ).toSerialized( payload );
        }
        catch ( const std::out_of_range& ){
            rejected = true;
        }
        require( rejected, "Out-of-range int8 was accepted" );

        robotics::ros::sensor_msgs__msg__CameraInfo camera;
        camera.add_k( 1 );
        payload.PackFrom( camera );
        rejected = false;
        try{
            rosdata::Converter( "sensor_msgs/msg/CameraInfo" ).toSerialized( payload );
        }
        catch ( const std::invalid_argument& ){
            rejected = true;
        }
        require( rejected, "Invalid fixed array length was accepted" );

        rejected = false;
        try{
            rosdata::Converter( "std_msgs/msg/Int8" ).toSerialized( payload );
        }
        catch ( const std::invalid_argument& ){
            rejected = true;
        }
        require( rejected, "Wrong Any type was accepted" );

        rosdata::ConversionLimits limits;
        limits.maxBytes = 1;
        payload.PackFrom( invalid );
        rejected = false;
        try{
            rosdata::Converter( "std_msgs/msg/Int8", {}, limits ).toSerialized( payload );
        }
        catch ( const std::length_error& ){
            rejected = true;
        }
        require( rejected, "Oversized payload was accepted" );
    }
} // namespace

//
int main( int argc, char** argv ){
    rclcpp::init( argc, argv );
    try{
        defaultRoundTrips();
        sensor_msgs::msg::Image image;
        image.header.frame_id = "camera";
        image.header.stamp.sec = -17;
        image.header.stamp.nanosec = 123;
        image.width = 1024;
        image.height = 2048;
        image.encoding = "rgb8";
        image.step = image.width * 3;
        image.data.resize( image.step * image.height );
        for ( std::size_t i = 0; i < image.data.size(); ++i ){
            image.data[i] = i % 256;
        }
        roundTrip( "sensor_msgs/msg/Image", image );

        sensor_msgs::msg::PointCloud2 cloud;
        cloud.width = 2;
        cloud.height = 1;
        cloud.point_step = 4;
        cloud.row_step = 8;
        cloud.is_dense = true;
        cloud.fields.resize( 1 );
        cloud.fields[0].name = "intensity";
        cloud.fields[0].datatype = 7;
        cloud.fields[0].count = 1;
        cloud.data = { 0, 255, 128, 1, 2, 3, 4, 5 };
        roundTrip( "sensor_msgs/msg/PointCloud2", cloud );

        nav_msgs::msg::OccupancyGrid grid;
        grid.info.width = 3;
        grid.info.height = 1;
        grid.data = { -1, 0, 100 };
        roundTrip( "nav_msgs/msg/OccupancyGrid", grid );

        sensor_msgs::msg::CameraInfo camera;
        camera.k = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        camera.d = { -0.5, 0.25 };
        camera.distortion_model = "plumb_bob";
        roundTrip( "sensor_msgs/msg/CameraInfo", camera );

        sensor_msgs::msg::JointState joints;
        joints.name = { "left", "right" };
        joints.position = { -3.14159, 1.25 };
        roundTrip( "sensor_msgs/msg/JointState", joints );

        std_msgs::msg::Byte byteValue;
        byteValue.data = 255;
        roundTrip( "std_msgs/msg/Byte", byteValue );

        std_msgs::msg::Int8 signedValue;
        signedValue.data = -128;
        roundTrip( "std_msgs/msg/Int8", signedValue );
        std_msgs::msg::UInt64 unsignedValue;
        unsignedValue.data = std::numeric_limits<uint64_t>::max();
        roundTrip( "std_msgs/msg/UInt64", unsignedValue );
        invalidValues();
    }
    catch ( const std::exception& error ){
        std::cerr << error.what() << std::endl;
        rclcpp::shutdown();
        return 1;
    }
    rclcpp::shutdown();
    return 0;
}
