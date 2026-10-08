cmake_minimum_required(VERSION 3.25)

add_definitions(-DWIN32_LEAN_AND_MEAN)
add_definitions(-D_WIN32_WINNT=0x0601)

if (MSVC)
    add_compile_options("/utf-8")
endif()

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 OLD)
endif()

# vcpkg 工具链：优先使用命令行传入的 -DCMAKE_TOOLCHAIN_FILE（CI 与其他机器走这条），
# 否则回退到作者本机路径；两者都不满足时需要自行指定，见 README「快速开始」
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "C:/Users/mpt/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/mpt/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET AND WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
endif()


project(GetServer)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 依赖库
find_package(Boost REQUIRED COMPONENTS
        system
        thread
        filesystem
)
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

# 可执行程序
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(GetServer
        main.cpp
        CServer.cpp
        CServer.h
        HttpConnection.cpp
        HttpConnection.h
        Singleton.h
        LogicSystem.cpp
        LogicSystem.h
        const.h
        ${PROTO_SRCS} ${PROTO_HDRS}
        ${PROTO_NAME}.grpc.pb.cc ${PROTO_NAME}.grpc.pb.h
        VarifyGrpcClient.h
        ConfigMgr.h
        ConfigMgr.cpp
        AsioIOServicePool.cpp
        AsioIOServicePool.h
        VarifyGrpcClient.cpp
        RedisMgr.cpp
        RedisMgr.h
        MysqlMgr.cpp
        MysqlMgr.h
        StatusGrpcClient.cpp
        StatusGrpcClient.h
)

# 链接库
target_link_libraries(GetServer PRIVATE
        ${Boost_LIBRARIES}
        nlohmann_json::nlohmann_json
        gRPC::grpc++
        gRPC::grpc++_reflection
        redis++::redis++ hiredis::hiredis
        unofficial::libmysql::libmysql


)