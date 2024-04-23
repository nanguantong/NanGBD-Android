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

set -u

source ./build-android-common.sh

if [ -z ${version+x} ]; then
  version="2.5.4"
fi

CUR_ROOT=`pwd`
LIB_NAME="gmssl-$version"
LIB_SRC_DIR="${CUR_ROOT}/../cpp/gmssl"

# Configure and make for specified architectures
configure_make() {
  ARCH=$1; ABI=$2; ABI_TRIPLE=$3

  PREFIX_DIR=${LIB_SRC_DIR}/${ABI}
  OUTPUT_ROOT=${PREFIX_DIR}
  LIB_DEST_DIR="${CUR_ROOT}/../jniLibs/${ABI}"
  INC_DEST_DIR="${CUR_ROOT}/../cpp/include"
  LOG_DIR=${OUTPUT_ROOT}/log
  LOG_FILE=${LOG_DIR}/${ABI}.log
  mkdir -p ${LOG_DIR}

  log_info "configure $ABI start..."

  local arch=$(get_android_arch_for_config ${ARCH})
  set_android_toolchain_bin
  set_android_toolchain "gmssl" "${ARCH}" "${ANDROID_API}"
  set_android_cpu_feature "gmssl" "${ARCH}" "${ANDROID_API}"
  android_printf_global_params "$ARCH" "$ABI" "$ABI_TRIPLE" "$PREFIX_DIR" "$OUTPUT_ROOT"

  pushd "${LIB_SRC_DIR}"

  ./Configure ${arch} \
              --prefix=${PREFIX_DIR} \
              --with-zlib-include=$SYSROOT/usr/include \
              --with-zlib-lib=$SYSROOT/usr/lib \
              zlib \
              no-asm \
              no-shared \
              no-unit-test\
              no-serpent

  log_info "make $ABI start..."

  make clean >"${LOG_FILE}"
  if make -j$(get_cpu_count) >>"${LOG_FILE}" 2>&1; then
    log_info "installing ..."

    make install_sw >>"${LOG_FILE}" 2>&1
    make install_ssldirs >>"${LOG_FILE}" 2>&1

    [ -d ${INC_DEST_DIR} ] || mkdir -p ${INC_DEST_DIR}
    cp -r ${LIB_SRC_DIR}/${ABI}/include/openssl ${INC_DEST_DIR}

    [ -d ${LIB_DEST_DIR} ] || mkdir -p ${LIB_DEST_DIR}
    cp ${LIB_SRC_DIR}/${ABI}/lib/libcrypto.a ${LIB_DEST_DIR}
    cp ${LIB_SRC_DIR}/${ABI}/lib/libssl.a ${LIB_DEST_DIR}

    log_info "installed"
  fi
  popd
}

for ((i = 0; i < ${#ARCHS[@]}; i++)); do
  if [[ $# -eq 0 || "$1" == "${ARCHS[i]}" ]]; then
    # Do not build 64 bit arch if ANDROID_API is less than 21 which is
    # the minimum supported API level for 64 bit.
    # [[ ${ANDROID_API} -lt 21 ]] && ( echo "${ABIS[i]}" | grep 64 > /dev/null ) && continue;
    configure_make "${ARCHS[i]}" "${ABIS[i]}" "${ARCHS[i]}-linux-android"
  fi
done

log_info "${PLATFORM_TYPE} ${LIB_NAME} end..."
