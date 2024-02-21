# 使用的CMake版本
cmake_minimum_required(VERSION 3.10)

# 项目名
project(RTSPStream)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
# 查找Opencv库文件
find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

include_directories(/usr/local/ffmpeg/include ${OpenCV_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
link_directories(RTSPStream PRIVATE /usr/local/ffmpeg/lib)

# 添加源代码文件
add_executable(${PROJECT_NAME} main.cpp PushOpencv.cpp)

# 链接FFmpeg、OpenCV库
target_link_libraries(${PROJECT_NAME} PRIVATE
libpthread.so
libavcodec.so
libavformat.so
libavfilter.so
libavdevice.so
libavutil.so
libswscale.so
${OpenCV_LIBS}
${CMAKE_DL_LIBS}
)
