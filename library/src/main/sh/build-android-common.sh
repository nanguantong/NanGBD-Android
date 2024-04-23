#!/bin/bash
#
# Copyright 2016 leenjewel
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# https://github.com/leenjewel/openssl_for_ios_and_android
# https://github.com/wangp8895/gmssl-for-android

source ./build-common.sh

# https://developer.android.com/ndk/guides/abis
# https://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/MIPS-Options.html
#ARCHS=("android" "android-armeabi" "android64-aarch64" "android-x86" "android64" "android-mips" "android-mips64")
#ABIS=("armeabi" "armeabi-v7a" "arm64-v8a" "x86" "x86_64" "mips" "mips64")

if [ -z ${arch+x} ]; then
  arch=("arm" "arm64" "x86" "x86_64" "mips" "mips64")
fi
if [ -z ${abi+x} ]; then
  abi=("armeabi-v7a" "arm64-v8a" "x86" "x86_64" "mips" "mips64")
fi
if [ -z ${api+x} ]; then
  # Default to API 21 for it is the minimum requirement for 64 bit archs.
  api=21
fi

export PLATFORM_TYPE="Android"
export ARCHS=(${arch[@]})
export ABIS=(${abi[@]})
export ANDROID_API=(${api[@]})

# for test
# export ARCHS=("x86_64")
# export ABIS=("x86_64")

if [[ -z ${ANDROID_NDK_ROOT} ]]; then
  echo "ANDROID_NDK_ROOT not defined"
  exit 1
fi

function get_toolchain() {
  HOST_OS=$(uname -s)
  case ${HOST_OS} in
  Darwin) HOST_OS=darwin ;;
  Linux) HOST_OS=linux ;;
  FreeBsd) HOST_OS=freebsd ;;
  CYGWIN* | *_NT-*) HOST_OS=cygwin ;;
  esac

  HOST_ARCH=$(uname -m)
  case ${HOST_ARCH} in
  i?86) HOST_ARCH=x86 ;;
  x86_64 | amd64 | arm64) HOST_ARCH=x86_64 ;;
  esac

  echo "${HOST_OS}-${HOST_ARCH}"
}

function get_android_arch() {
  local common_arch=$1
  case ${common_arch} in
  arm)
    echo "arm-v7a"
    ;;
  arm64)
    echo "arm64-v8a"
    ;;
  x86)
    echo "x86"
    ;;
  x86_64)
    echo "x86-64"
    ;;
  mips)
    echo "mips"
    ;;
  mips64)
    echo "mips64"
    ;;
  esac
}


function get_android_arch_for_config() {
  local common_arch=$1
  case ${common_arch} in
  arm)
    echo "android-armeabi" # "android-arm"
    ;;
  arm64)
    echo "android64-aarch64" # "android-arm64"
    ;;
  x86)
    echo "android-x86"
    ;;
  x86_64)
    echo "android64"
    ;;
  mips)
    echo "android-mips"
    ;;
  mips64)
    echo "android-mips64"
    ;;
  esac
}

function get_target_build() {
  local arch=$1
  case ${arch} in
  arm-v7a)
    echo "arm"
    ;;
  arm64-v8a)
    echo "arm64"
    ;;
  x86)
    echo "x86"
    ;;
  x86-64)
    echo "x86_64"
    ;;
  mips)
    echo "mips"
    ;;
  mips64)
    echo "mips64"
    ;;
  esac
}

function get_build_host_internal() {
  local arch=$1
  case ${arch} in
  arm-v7a | arm-v7a-neon)
    echo "arm-linux-androideabi"
    ;;
  arm64-v8a)
    echo "aarch64-linux-android"
    ;;
  x86)
    echo "i686-linux-android"
    ;;
  x86-64)
    echo "x86_64-linux-android"
    ;;
  mips)
    echo "mipsel-linux-android"
    ;;
  mips64)
    echo "mips64el-linux-android"
    ;;
  esac
}

function android_get_build_host() {
  local arch=$(get_android_arch $1)
  get_build_host_internal $arch
}

function get_clang_target_host() {
  local arch=$1
  local api=$2
  case ${arch} in
  arm-v7a | arm-v7a-neon)
    echo "armv7a-linux-androideabi${api}"
    ;;
  arm64-v8a)
    echo "aarch64-linux-android${api}"
    ;;
  x86)
    echo "i686-linux-android${api}"
    ;;
  x86-64)
    echo "x86_64-linux-android${api}"
    ;;
  mips)
    echo "mipsel-linux-android${api}"
    ;;
  mips64)
    echo "mips64el-linux-android${api}"
    ;;
  esac
}

function set_android_toolchain_bin() {
  TOOLCHAIN_ROOT=${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/$(get_toolchain)
  export ANDROID_NDK_HOME=${ANDROID_NDK_ROOT}
  export SYSROOT=${TOOLCHAIN_ROOT}/sysroot
  export PATH=${TOOLCHAIN_ROOT}/bin:$PATH
  echo "${TOOLCHAIN_ROOT}"
  echo "${SYSROOT}"
  echo PATH="$PATH"
}

function set_android_toolchain() {
  local name=$1
  local arch=$(get_android_arch $2)
  local api=$3
  local build_host=$(get_build_host_internal "$arch")
  local clang_target_host=$(get_clang_target_host "$arch" "$api")

  export AR=${build_host}-ar
  export CC=${clang_target_host}-clang
  export CXX=${clang_target_host}-clang++
  export AS=${build_host}-as
  export LD=${build_host}-ld
  export RANLIB=${build_host}-ranlib
  export STRIP=${build_host}-strip
}

function get_common_includes() {
  local toolchain=$(get_toolchain)
  echo "-I${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${toolchain}/sysroot/usr/include -I${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${toolchain}/sysroot/usr/local/include"
}
function get_common_linked_libraries() {
  local api=$1
  local arch=$2
  local toolchain=$(get_toolchain)
  local build_host=$(get_build_host_internal "$arch")
  echo "-L${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${toolchain}/${build_host}/lib -L${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${toolchain}/sysroot/usr/lib/${build_host}/${api} -L${ANDROID_NDK_ROOT}/toolchains/llvm/prebuilt/${toolchain}/lib"
}

function set_android_cpu_feature() {
  local name=$1
  local arch=$(get_android_arch $2)
  local api=$3
  case ${arch} in
  arm-v7a | arm-v7a-neon)
    export CFLAGS="-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -Wno-unused-function -fno-integrated-as -fstrict-aliasing -fPIC -DANDROID -D__ANDROID_API__=${api} -Os -ffunction-sections -fdata-sections $(get_common_includes)"
    export CXXFLAGS="-std=c++11 -Os -ffunction-sections -fdata-sections"
    export LDFLAGS="-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -Wl,--fix-cortex-a8 -Wl,--gc-sections -Os -ffunction-sections -fdata-sections $(get_common_linked_libraries ${api} ${arch})"
    export CPPFLAGS=${CFLAGS}
    NDK_FLAGS="--arch=arm"
    ;;
  arm64-v8a)
    export CFLAGS="-march=armv8-a -Wno-unused-function -fno-integrated-as -fstrict-aliasing -fPIC -DANDROID -D__ANDROID_API__=${api} -Os -ffunction-sections -fdata-sections $(get_common_includes)"
    export CXXFLAGS="-std=c++11 -Os -ffunction-sections -fdata-sections"
    export LDFLAGS="-march=armv8-a -Wl,--gc-sections -Os -ffunction-sections -fdata-sections $(get_common_linked_libraries ${api} ${arch})"
    export CPPFLAGS=${CFLAGS}
    NDK_FLAGS="--arch=arm64"
    ;;
  x86)
    export CFLAGS="-march=i686 -mtune=intel -mssse3 -mfpmath=sse -m32 -Wno-unused-function -fno-integrated-as -fstrict-aliasing -fPIC -DANDROID -D__ANDROID_API__=${api} -Os -ffunction-sections -fdata-sections $(get_common_includes)"
    export CXXFLAGS="-std=c++11 -Os -ffunction-sections -fdata-sections"
    export LDFLAGS="-march=i686 -Wl,--gc-sections -Os -ffunction-sections -fdata-sections $(get_common_linked_libraries ${api} ${arch})"
    export CPPFLAGS=${CFLAGS}
    NDK_FLAGS="--arch=x86"
    ;;
  x86-64)
    export CFLAGS="-march=x86-64 -msse4.2 -mpopcnt -m64 -mtune=intel -Wno-unused-function -fno-integrated-as -fstrict-aliasing -fPIC -DANDROID -D__ANDROID_API__=${api} -Os -ffunction-sections -fdata-sections $(get_common_includes)"
    export CXXFLAGS="-std=c++11 -Os -ffunction-sections -fdata-sections"
    export LDFLAGS="-march=x86-64 -Wl,--gc-sections -Os -ffunction-sections -fdata-sections $(get_common_linked_libraries ${api} ${arch})"
    export CPPFLAGS=${CFLAGS}
    NDK_FLAGS="--arch=x86_64"
    ;;
  mips)
    export CFLAGS="-march=mips -Wno-unused-function -fno-integrated-as -fstrict-aliasing -fPIC -DANDROID -D__ANDROID_API__=${api} -Os -ffunction-sections -fdata-sections $(get_common_includes)"
    export CXXFLAGS="-std=c++11 -Os -ffunction-sections -fdata-sections"
    export LDFLAGS="-march=mips -Wl,--gc-sections -Os -ffunction-sections -fdata-sections $(get_common_linked_libraries ${api} ${arch})"
    export CPPFLAGS=${CFLAGS}
    NDK_FLAGS="--arch=mips"
    ;;
  mips64)
    export CFLAGS="-march=mips64 -Wno-unused-function -fno-integrated-as -fstrict-aliasing -fPIC -DANDROID -D__ANDROID_API__=${api} -Os -ffunction-sections -fdata-sections $(get_common_includes)"
    export CXXFLAGS="-std=c++11 -Os -ffunction-sections -fdata-sections"
    export LDFLAGS="-march=mips64 -Wl,--gc-sections -Os -ffunction-sections -fdata-sections $(get_common_linked_libraries ${api} ${arch})"
    export CPPFLAGS=${CFLAGS}
    NDK_FLAGS="--arch=mips64"
    ;;
  esac
}

function android_printf_global_params() {
  local arch=$1
  local abi=$2
  local abi_triple=$3
  local in_dir=$4
  local out_dir=$5
  echo -e "arch =           $arch"
  echo -e "abi =            $abi"
  echo -e "abi_triple =     $abi_triple"
  echo -e "PLATFORM_TYPE =  $PLATFORM_TYPE"
  echo -e "ANDROID_API =    $ANDROID_API"
  echo -e "ANDROID_NDK =    $ANDROID_NDK_ROOT"
  echo -e "in_dir =         $in_dir"
  echo -e "out_dir =        $out_dir"
  echo -e "AR =             $AR"
  echo -e "CC =             $CC"
  echo -e "CXX =            $CXX"
  echo -e "AS =             $AS"
  echo -e "LD =             $LD"
  echo -e "RANLIB =         $RANLIB"
  echo -e "STRIP =          $STRIP"
  echo -e "CFLAGS =         $CFLAGS"
  echo -e "CXXFLAGS =       $CXXFLAGS"
  echo -e "LDFLAGS =        $LDFLAGS"
  echo -e "CPPFLAGS =       $CPPFLAGS"
}
