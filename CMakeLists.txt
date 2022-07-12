cmake_minimum_required(VERSION 3.16)
project(threadpool)

set(CMAKE_CXX_STANDARD 14)
set(EXECUTABLE_OUTPUT_PATH "build")

include_directories(.)
LINK_LIBRARIES(/usr/local/lib/libleveldb.a)
LINK_LIBRARIES(/usr/local/lib/libsnappy.a)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-pthread")

# 找到所有mains文件夹下的cpp文件，保存在mains变量里
file(GLOB mains
        "${CMAKE_CURRENT_SOURCE_DIR}/mains/*.cpp"
)

# 循环遍历每个含main的文件，分别编译生成exe文件
foreach(mainfile IN LISTS mains)
    # Get file name without directory
    get_filename_component(mainname ${mainfile} NAME_WE)
    add_executable(${mainname} ${mainfile})
endforeach()
