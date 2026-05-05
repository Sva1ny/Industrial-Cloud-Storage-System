# 设置CMake的最小版本
cmake_minimum_required(VERSION 3.15)

# 项目名称
#   项目可以包含多个目标(target)
#   target: 可执行程序, 静态库, 动态库
project(CloudDisk)

find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 自编库常见安装位置（workflow / wfrest / srpc 等）
link_directories(/usr/local/lib)
if(EXISTS /opt/homebrew/lib)
  link_directories(/opt/homebrew/lib)
endif()

find_path(SIMPLEAMQPCLIENT_INCLUDE_DIR NAMES SimpleAmqpClient/SimpleAmqpClient.h
  PATHS ${CMAKE_PREFIX_PATH} /opt/homebrew /usr/local
  PATH_SUFFIXES include
)
if(NOT SIMPLEAMQPCLIENT_INCLUDE_DIR)
  message(FATAL_ERROR "未找到 SimpleAmqpClient 头文件。可执行: brew install simple-amqp-client")
endif()

find_library(JWT_LIB NAMES jwt HINTS /opt/homebrew/lib /usr/local/lib)
if(NOT JWT_LIB)
  message(FATAL_ERROR "未找到 libjwt。可执行: brew install libjwt")
endif()

# 指定C++的版本: -std=c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置可执行程序生成的路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../)


# 生成的可执行程序
add_executable(server
    CloudiskServer.cpp
    CryptoUtil.cpp
    UserService.pb.cpp
    main.cpp
)

# 添加宏定义: -DDEBUG
target_compile_definitions(server PRIVATE
   DEBUG
)

# 设置编译选项
target_compile_options(server PRIVATE
    -g
)

# -lcrypto -lssl -ljwt -lwfrest
# server: target的名字
# PRIVATE: 传播属性, 填PRIVATE即可，固定的用法
target_include_directories(server PRIVATE
  ${OPENSSL_INCLUDE_DIR}
  ${SIMPLEAMQPCLIENT_INCLUDE_DIR}
)

target_link_libraries(server PRIVATE 
    wfrest
    workflow
    SimpleAmqpClient
    srpc
    lz4
    snappy
    protobuf
    ${JWT_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

# 添加可执行程序 backup
add_executable(backup 
    OssManager.cpp
    backup.cpp
)

# 添加宏定义: -DDEBUG
target_compile_definitions(backup PRIVATE
   DEBUG
)

# 设置编译选项
target_compile_options(backup PRIVATE
    -g
    -fno-rtti
)

target_include_directories(backup PRIVATE
  ${OPENSSL_INCLUDE_DIR}
  ${SIMPLEAMQPCLIENT_INCLUDE_DIR}
)

# 设置要链接的库
target_link_libraries(backup PRIVATE 
    SimpleAmqpClient
    alibabacloud-oss-cpp-sdk
    curl
    OpenSSL::Crypto
    pthread
)

# 添加可执行程序 UserService
add_executable(UserService
    UserService.cpp
    UserService.pb.cpp
    CryptoUtil.cpp
)

# 添加宏定义: -DDEBUG
target_compile_definitions(UserService PRIVATE
   DEBUG
)

# 设置编译选项
target_compile_options(UserService PRIVATE
    -g
)

target_include_directories(UserService PRIVATE ${OPENSSL_INCLUDE_DIR})

# 设置要链接的库
target_link_libraries(UserService PRIVATE
    ppconsul
    srpc
    workflow
    lz4
    snappy
    protobuf
    ${JWT_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
)