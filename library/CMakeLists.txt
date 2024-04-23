# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

add_library(
            # Sets the name of the library.
            NanGBD

            # Sets the library as a shared library.
            SHARED

            # Provides a relative path to your source file(s).
            # Associated headers in the same location as their source
            # file are automatically included.
            src/main/cpp/gb_native.cpp

            src/main/cpp/utils/ifaddrs.c
            src/main/cpp/utils/queue.cpp
            src/main/cpp/net/udp_server.cpp
            src/main/cpp/ua/sip_ua.cpp
            src/main/cpp/ua/gm_ssl.cpp
            src/main/cpp/ua/gm_codec.cpp
            src/main/cpp/gb_muxer.cpp
            src/main/cpp/gb_demuxer.cpp
            src/main/cpp/gb_sender.cpp
            )

add_library(
            avcodec
            SHARED
            IMPORTED
            )

add_library(
            avfilter
            SHARED
            IMPORTED
            )

add_library(
            avformat
            SHARED
            IMPORTED
            )

add_library(
            avutil
            SHARED
            IMPORTED
            )

add_library(
            swresample
            SHARED
            IMPORTED
            )

add_library(
            swscale
            SHARED
            IMPORTED
           )

add_library(
        exosip
        STATIC
        IMPORTED
)
add_library(
            osip
            STATIC
            IMPORTED
            )

add_library(
        x264
        STATIC
        IMPORTED
)
add_library(
        x265
        STATIC
        IMPORTED
)

add_library(
        crypto
        STATIC
        IMPORTED
)
add_library(
        ssl
        STATIC
        IMPORTED
)

################################################################################
############################### arm64-v8a ######################################

if(${ANDROID_ABI} STREQUAL "arm64-v8a")
set_target_properties(
        avcodec
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libavcodec.so
)

set_target_properties(
        avfilter
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libavfilter.so
        )

set_target_properties(
            avformat
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libavformat.so
            )

set_target_properties(
            avutil
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libavutil.so
            )

set_target_properties(
            swresample
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libswresample.so
             )

set_target_properties(
            swscale
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libswscale.so
             )

set_target_properties(
        exosip
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libexosip.a
)
set_target_properties(
        osip
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libosip.a
)

set_target_properties(
        x264
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libx264.a
)
set_target_properties(
        x265
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libx265.a
)

set_target_properties(
        crypto
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libcrypto.a
)
set_target_properties(
        ssl
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/arm64-v8a/libssl.a
)
endif(${ANDROID_ABI} STREQUAL "arm64-v8a")

################################################################################
############################## armeabi-v7a #####################################

if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
set_target_properties(
        avcodec
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libavcodec.so
        )

set_target_properties(
        avfilter
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libavfilter.so
        )

set_target_properties(
            avformat
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libavformat.so
            )

set_target_properties(
            avutil
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libavutil.so
            )

set_target_properties(
            swresample
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libswresample.so
             )

set_target_properties(
            swscale
            PROPERTIES IMPORTED_LOCATION
            ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libswscale.so
             )

set_target_properties(
        exosip
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libexosip.a
)
set_target_properties(
        osip
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libosip.a
)

set_target_properties(
        x264
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libx264.a
)
set_target_properties(
        x265
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libx265.a
)
set_target_properties(
        crypto
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libcrypto.a
)
set_target_properties(
        ssl
        PROPERTIES IMPORTED_LOCATION
        ${CMAKE_SOURCE_DIR}/src/main/jniLibs/armeabi-v7a/libssl.a
)
endif(${ANDROID_ABI} STREQUAL "armeabi-v7a")

################################################################################

# https://www.jianshu.com/p/38909cf4f9af Android ndk cmake编译libyuv
# https://chromium.googlesource.com/libyuv/libyuv/+/HEAD/docs/getting_started.md
include_directories(
        ${CMAKE_SOURCE_DIR}/src/main/cpp/libyuv/include
        #${CMAKE_SOURCE_DIR}/ffmpeg-3.2.5/
)

target_include_directories(NanGBD PRIVATE ${CMAKE_SOURCE_DIR}/src/main/cpp/include)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/main/cpp/libyuv)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log
            )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                      NanGBD
                      avcodec
                      avfilter
                      avformat
                      avutil
                      swresample
                      swscale
                      exosip
                      osip
                      x264
                      x265
                      yuv
                      crypto
                      ssl
                      -lz
                      # Links the target library to the log library
                      # included in the NDK.
                      ${log-lib} )