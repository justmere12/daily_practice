cmake_minimum_required(VERSION 3.12)
project(ring_buffer VERSION 1.0.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall -Wextra -Werror")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wextra -Werror")
endif()

# 添加头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 主库
add_library(ring_buffer STATIC src/ring_buffer.cpp)

# 设置安装路径
install(DIRECTORY include/ring_buffer
        DESTINATION include
        FILES_MATCHING PATTERN "*.h")

install(TARGETS ring_buffer
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

# 添加示例
add_subdirectory(examples)