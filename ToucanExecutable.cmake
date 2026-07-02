# Copyright 2026 The Toucan Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

function(toucan_objects TARGET_NAME)
  cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

  set(MAKE_ACTION "make_${TARGET_NAME}")
  set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.o")
  set(INIT_TYPES_CC "${CMAKE_CURRENT_BINARY_DIR}/init_types_${TARGET_NAME}.cc")

  set(ABS_SOURCES "")
  foreach(src ${ARG_SOURCES})
    if(IS_ABSOLUTE "${src}")
      list(APPEND ABS_SOURCES "${src}")
    else()
      list(APPEND ABS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
    endif()
  endforeach()

  if(CMAKE_CROSSCOMPILING)
    set(TC_CMD "${CMAKE_BINARY_DIR}/host/tc")
  else()
    set(TC_CMD "$<TARGET_FILE:tc>")
  endif()

  if(EMSCRIPTEN)
    set(TARGET_TRIPLE_ARG -t wasm32-unknown-unknown)
    set(FEATURES_ARG -f +simd128)
  endif()

  if(ANDROID)
    if("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "arm64-v8a")
      set(TARGET_TRIPLE_ARG -t aarch64-linux-android)
    elseif("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "armeabi-v7a")
      set(TARGET_TRIPLE_ARG -t armv7a-linux-androideabi)
    elseif("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "x86_64")
      set(TARGET_TRIPLE_ARG -t x86_64-linux-android)
    elseif("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "x86")
      set(TARGET_TRIPLE_ARG -t i686-linux-android)
    else()
      set(TARGET_TRIPLE_ARG -t aarch64-linux-android)
    endif()
  elseif(IOS)
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
      set(TARGET_TRIPLE_ARG -t arm64-apple-ios15.0)
    elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm")
      set(TARGET_TRIPLE_ARG -t armv7-apple-ios15.0)
    else()
      set(TARGET_TRIPLE_ARG -t arm64-apple-ios15.0)
    endif()
  endif()

  add_custom_command(
    OUTPUT ${OBJ_FILE} ${INIT_TYPES_CC}
    COMMAND ${TC_CMD}
            -o ${OBJ_FILE}
            -i ${INIT_TYPES_CC}
            -I ${CMAKE_SOURCE_DIR}
            -I ${CMAKE_SOURCE_DIR}/samples/include
            ${TARGET_TRIPLE_ARG}
            ${FEATURES_ARG}
            ${ABS_SOURCES}
    DEPENDS ${ABS_SOURCES} tc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Compiling Toucan sources for ${TARGET_NAME}"
  )

  add_custom_target(${MAKE_ACTION} DEPENDS ${OBJ_FILE} ${INIT_TYPES_CC})
endfunction()

function(toucan_executable TARGET_NAME)
  toucan_objects(${TARGET_NAME} ${ARGN})

  set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.o")
  set(INIT_TYPES_CC "${CMAKE_CURRENT_BINARY_DIR}/init_types_${TARGET_NAME}.cc")

  add_executable(${TARGET_NAME} ${OBJ_FILE} ${INIT_TYPES_CC})

  target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR})

  target_link_libraries(${TARGET_NAME} PRIVATE samples_main api ast)

  if(NOT EMSCRIPTEN)
    if(ANDROID OR IOS)
      target_link_libraries(${TARGET_NAME} PRIVATE dawn_proc dawn_native)
    else()
      target_link_libraries(${TARGET_NAME} PRIVATE dawn_proc webgpu_dawn)
    endif()
    if(WIN32)
      add_custom_command(
        TARGET ${TARGET_NAME}
        DEPENDS ${CMAKE_BINARY_DIR}/webgpu_dawn.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/third_party/dawn/webgpu_dawn.dll
          ${CMAKE_BINARY_DIR}/webgpu_dawn.dll
      )
    endif()
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT EMSCRIPTEN)
    target_link_libraries(${TARGET_NAME} PRIVATE X11 X11-xcb dl pthread)
  elseif(APPLE)
    if(IOS)
      find_library(UIKIT_FRAMEWORK UIKit)
      find_library(METAL_FRAMEWORK Metal)
      find_library(QUARTZCORE_FRAMEWORK QuartzCore)
      find_library(FOUNDATION_FRAMEWORK Foundation)
      target_link_libraries(${TARGET_NAME} PRIVATE
        ${UIKIT_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
      )
      # Turn off auto-bundling; the .app file will be created manually
      set_target_properties(${TARGET_NAME} PROPERTIES MACOSX_BUNDLE FALSE)
    else()
      find_library(APPKIT_FRAMEWORK AppKit)
      find_library(METAL_FRAMEWORK Metal)
      find_library(QUARTZCORE_FRAMEWORK QuartzCore)
      target_link_libraries(${TARGET_NAME} PRIVATE
        ${APPKIT_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
      )
    endif()
  elseif(WIN32)
    target_link_libraries(${TARGET_NAME} PRIVATE gdi32 user32)
  elseif(EMSCRIPTEN)
    target_link_options(${TARGET_NAME} PRIVATE
      "--shell-file=${CMAKE_SOURCE_DIR}/emscripten/shell.html"
      "-sINITIAL_MEMORY=67108864"
      "-sSTACK_SIZE=4194304"
      "-sASYNCIFY=2"
      "--no-wasm-opt"
      "-Wno-experimental"
      "-lembind"
      "-msimd128"
    )
    set_target_properties(${TARGET_NAME} PROPERTIES SUFFIX ".html")
  endif()
endfunction()

function(toucan_android_main_lib TARGET_NAME)
  toucan_objects(${TARGET_NAME} ${ARGN})

  set(INIT_TYPES_CC "${CMAKE_CURRENT_BINARY_DIR}/init_types_${TARGET_NAME}.cc")
  add_library(${TARGET_NAME} SHARED ${INIT_TYPES_CC})

  # Strip debug info from non-Debug builds
  # FIXME: try to use installed Dawn builds so the libs will be automatically stripped
  if(NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_link_options(${TARGET_NAME} PRIVATE "-Wl,--strip-debug")
  endif()
  set(OBJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.o")
  target_sources(${TARGET_NAME} PRIVATE ${OBJ_FILE} $<TARGET_OBJECTS:android_native_app_glue>)
  target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
  target_link_libraries(${TARGET_NAME} PRIVATE api ast android_main dawn_proc dawn_native android)
endfunction()

function(toucan_android_apk)
  toucan_android_main_lib("${TARGET_NAME}" ${ARGN})
  set(TARGET_APK ${CMAKE_BINARY_DIR}/${TARGET_NAME}.apk)
  set(MAIN_LIB ${CMAKE_BINARY_DIR}/lib${TARGET_NAME}.so)
  get_filename_component(ANDROID_NDK_VERSION_DIR "${CMAKE_ANDROID_NDK}" DIRECTORY)
  get_filename_component(ANDROID_SDK_ROOT "${ANDROID_NDK_VERSION_DIR}" DIRECTORY)

  add_custom_command(
    OUTPUT ${TARGET_APK}
    COMMAND ${CMAKE_COMMAND} -E env
            ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tools/make-apk.py
            --target-name ${TARGET_NAME}
            --sdk-dir ${ANDROID_SDK_ROOT}
            --target-abi ${CMAKE_ANDROID_ARCH_ABI}
            --source-lib ${MAIN_LIB}
            --keystore ${CMAKE_SOURCE_DIR}/tools/toucan.keystore
            --out ${CMAKE_BINARY_DIR}/${TARGET_NAME}.apk
    DEPENDS ${MAIN_LIB}
  )
  add_custom_target("make_apk${TARGET_NAME}" ALL DEPENDS ${TARGET_APK})
endfunction()

function(toucan_apple_app)
  toucan_executable(${TARGET_NAME} ${ARGN})
  set(MAKE_ACTION "make_apple_app_${TARGET_NAME}")
  set(TARGET_APP ${CMAKE_BINARY_DIR}/${TARGET_NAME}.app)
  list(APPEND MAKE_APP_ARGS
    --target-name ${TARGET_NAME}
    --app-icon "${CMAKE_SOURCE_DIR}/emscripten/toucan-logo.svg"
    --minimum-deployment-target "15.0"
  )
  if(IOS)
    if("$ENV{MOBILE_PROVISION_FILE}" STREQUAL "")
      message(FATAL_ERROR "Error: MOBILE_PROVISION_FILE not specified")
    endif()
    if("$ENV{CODESIGN_IDENTITY}" STREQUAL "")
      message(FATAL_ERROR "Error: CODESIGN_IDENTITY not specified")
    endif()
    if("$ENV{TEAM_IDENTIFIER}" STREQUAL "")
      message(FATAL_ERROR "Error: TEAM_IDENTIFIER not specified")
    endif()
    list(APPEND MAKE_APP_ARGS
      --target-os ios
      --mobile-provision $ENV{MOBILE_PROVISION_FILE}
      --codesign-identity $ENV{CODESIGN_IDENTITY}
      --team-identifier $ENV{TEAM_IDENTIFIER}
    )
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
      list(APPEND MAKE_APP_ARGS --enable-debugging)
    endif()
  else()
    list(APPEND MAKE_APP_ARGS
      --target-os mac
    )
  endif()
  add_custom_command(
    OUTPUT ${TARGET_APP}
    COMMAND ${CMAKE_COMMAND} -E env
            ${Python3_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/make-app.py"
            ${MAKE_APP_ARGS}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${TARGET_NAME} "${CMAKE_SOURCE_DIR}/emscripten/toucan-logo.svg"
  )
  add_custom_target("make_app${TARGET_NAME}" ALL DEPENDS ${TARGET_APP})
endfunction()

function("toucan_apple_ipa")
  toucan_apple_app(target_name ${ARGN})
  set(TARGET_APP "${CMAKE_BINARY_DIR}/${TARGET_NAME}.app")
  set(TARGET_IPA "${CMAKE_BINARY_DIR}/${TARGET_NAME}.ipa")
  add_custom_command(
    OUTPUT ${TARGET_IPA}
    COMMAND ${CMAKE_COMMAND} -E env
            ${Python3_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/tools/make-ipa.py"
            --app-file ${TARGET_APP}
            --ipa-file ${TARGET_IPA}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS ${TARGET_APP}
  )
  add_custom_target("make_ipa${TARGET_NAME}" ALL DEPENDS ${TARGET_IPA})
endfunction()

function(toucan_app TARGET_NAME)
  if(ANDROID)
    toucan_android_apk(${TARGET_NAME} ${ARGN})
  elseif(APPLE)
    if(IOS)
      toucan_apple_ipa(${TARGET_NAME} ${ARGN})
    else()
      toucan_apple_app(${TARGET_NAME} ${ARGN})
    endif()
  else()
    toucan_executable(${TARGET_NAME} ${ARGN})
  endif()
endfunction()
