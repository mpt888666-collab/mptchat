cmake_minimum_required(VERSION 4.0)

add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-D_WIN32_WINNT=0x0601)
cmake_policy(SET CMP0167 OLD)

set(CMAKE_TOOLCHAIN_FILE "C:/Users/mpt/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")

project(ResourceServer)
if (MSVC)
    add_compile_options("/utf-8")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS
        system
        thread
        filesystem
)
if(NOT TARGET synchronization)
    add_library(synchronization INTERFACE)
endif()
find_package(nlohmann_json REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(redis++ CONFIG REQUIRED)
find_package(unofficial-libmysql REQUIRED)
# 直接使用gRPC目标，不需要单独find_package(Protobuf)
set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/message.proto)
get_filename_component(PROTO_NAME "${PROTO_FILE}" NAME_WE)

# 生成protobuf基础代码
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

# 生成grpc服务代码，使用目标文件代替find_program
add_custom_command(
        OUTPUT ${PROTO_NAME}.grpc.pb.cc ${PROTO_NAME}.grpc.pb.h
        COMMAND $<TARGET_FILE:protobuf::protoc>
        ARGS
        --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        -I${CMAKE_CURRENT_SOURCE_DIR}
        ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
)

add_executable(ResourceServer main.cpp
        const.h
        ConfigMgr.cpp
        ConfigMgr.h
        MsgNode.cpp
        MsgNode.h
        AsioIOServicePool.cpp
        AsioIOServicePool.h
        Singleton.h
        CServer.cpp
        CServer.h
        CSession.cpp
        CSession.h
        LogicSystem.cpp
        LogicSystem.h
        MysqlMgr.h
        MysqlMgr.cpp
        RedisMgr.cpp
        FileSystem.cpp
        FileSystem.h
        FileWorker.cpp
        FileWorker.h
        base64.cpp
        base64.h
        data.cpp)


target_link_libraries(ResourceServer PRIVATE
        ${Boost_LIBRARIES}
        nlohmann_json::nlohmann_json
        gRPC::grpc++
        gRPC::grpc++_reflection
        redis++::redis++ hiredis::hiredis
        unofficial::libmysql::libmysql
)
