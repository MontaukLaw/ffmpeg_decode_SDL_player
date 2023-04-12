cmake_minimum_required(VERSION 3.5)
project(stable_version_2022_main)
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/ffmpeg/include)
set(ffmpeg_libs_src ${CMAKE_SOURCE_DIR}/ffmpeg/libs)
message("CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

link_directories(${ffmpeg_libs_src})

set(source_dir ${CMAKE_SOURCE_DIR}/src)
# file(GLOB source_files "${CMAKE_SOURCE_DIR}/src*.cpp")

message(STATUS "source_dir: ${source_dir}")

add_executable(ffmpeg_player 
                #${source_dir}/main.cpp 
                ${source_dir}/video_decoder_core.cpp
                ${source_dir}/udp_rev.cpp
                ${source_dir}/io_data.cpp
                #${source_dir}/video_encoder_core.cpp
                )

target_link_libraries(ffmpeg_player 
                ${ffmpeg_libs_src}/libavcodec.so
                ${ffmpeg_libs_src}/libavdevice.so 
                ${ffmpeg_libs_src}/libavfilter.so 
                ${ffmpeg_libs_src}/libavformat.so 
                ${ffmpeg_libs_src}/libavutil.so 
                ${ffmpeg_libs_src}/libswresample.so
                ${ffmpeg_libs_src}/libswscale.so
                -lpthread)

include_directories(/home/marc/SDL2/include/)                 
include_directories(/home/marc/SDL2/build/include)       
link_directories(/home/marc/SDL2/build)  

add_executable(darry_demo 
        ${source_dir}/darry_demo.cpp
        ${source_dir}/DerryPlayer.cpp
        ${source_dir}/VideoChannel.cpp
        )

target_link_libraries(darry_demo
                # -Wl,--start-group
                # avcodec avfilter avformat avutil swresample swscale
                # -Wl,--end-group
                ${ffmpeg_libs_src}/libavcodec.so
                ${ffmpeg_libs_src}/libavdevice.so 
                ${ffmpeg_libs_src}/libavfilter.so 
                ${ffmpeg_libs_src}/libavformat.so 
                ${ffmpeg_libs_src}/libavutil.so 
                ${ffmpeg_libs_src}/libswresample.so
                ${ffmpeg_libs_src}/libswscale.so
                -lpthread -lz -lSDL2 -lm -ldl 
                -lstdc++ -lm -lasound -lx264)   


add_executable(ffmpeg_from_csdn ${source_dir}/ffmpeg_from_csdn.cpp) 
target_link_libraries(ffmpeg_from_csdn
                 ${ffmpeg_libs_src}/libavcodec.so
                 ${ffmpeg_libs_src}/libavdevice.so 
                 ${ffmpeg_libs_src}/libavfilter.so 
                 ${ffmpeg_libs_src}/libavformat.so 
                 ${ffmpeg_libs_src}/libavutil.so 
                 ${ffmpeg_libs_src}/libswresample.so
                 ${ffmpeg_libs_src}/libswscale.so
                 -lpthread -lz -lSDL2 -lm -ldl 
                 -lstdc++ -lm -lasound -lx264)


