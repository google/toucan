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

set(API_HEADER "${CMAKE_CURRENT_BINARY_DIR}/api.h")

if(CMAKE_CROSSCOMPILING)
  set(API_HEADER_GEN_CMD "${CMAKE_BINARY_DIR}/host/api_header_generator")
else()
  set(API_HEADER_GEN_CMD $<TARGET_FILE:api_header_generator>)
endif()

add_custom_command(
  OUTPUT ${API_HEADER}
  COMMAND ${API_HEADER_GEN_CMD}
          -o ${API_HEADER}
          ${CMAKE_CURRENT_SOURCE_DIR}/api.t
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/api.t api_header_generator
  COMMENT "Generating api.h from api.t"
)

add_custom_target(generate_api_header DEPENDS ${API_HEADER})

set(DAWN_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen/third_party/dawn")
set(DAWN_GEN_HEADERS
  "${DAWN_GEN_DIR}/include/dawn/webgpu_cpp.h"
  "${DAWN_GEN_DIR}/include/dawn/webgpu.h"
  "${DAWN_GEN_DIR}/include/dawn/dawn_proc_table.h"
  "${DAWN_GEN_DIR}/include/dawn/webgpu_cpp_print.h"
  "${DAWN_GEN_DIR}/include/webgpu/webgpu_cpp_chained_struct.h"
  "${DAWN_GEN_DIR}/src/emdawnwebgpu/include/webgpu/webgpu_cpp.h"
)

add_custom_command(
  OUTPUT ${DAWN_GEN_HEADERS}
  COMMAND ${CMAKE_COMMAND} -E env
          "PYTHONPATH=${CMAKE_SOURCE_DIR}/third_party:${CMAKE_SOURCE_DIR}/third_party/jinja2:${CMAKE_SOURCE_DIR}/third_party/markupsafe"
          ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/third_party/dawn/generator/dawn_json_generator.py
          --template-dir ${CMAKE_SOURCE_DIR}/third_party/dawn/generator/templates
          --root-dir ${CMAKE_SOURCE_DIR}/third_party/dawn
          --output-dir ${DAWN_GEN_DIR}
          --dawn-json ${CMAKE_SOURCE_DIR}/third_party/dawn/src/dawn/dawn.json
          --wire-json ${CMAKE_SOURCE_DIR}/third_party/dawn/src/dawn/dawn_wire.json
          --jinja2-path ${CMAKE_SOURCE_DIR}/third_party/dawn/third_party/jinja2
          --targets headers,cpp_headers,emdawnwebgpu_headers
  DEPENDS ${CMAKE_SOURCE_DIR}/third_party/dawn/src/dawn/dawn.json
          ${CMAKE_SOURCE_DIR}/third_party/dawn/src/dawn/dawn_wire.json
  COMMENT "Generating Dawn WebGPU headers"
)

add_custom_target(generate_dawn_headers DEPENDS ${DAWN_GEN_HEADERS})

add_library(api STATIC api_dawn.cc api_image_codecs.cc)

if(WIN32)
  target_sources(api PRIVATE api_win.cc)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT EMSCRIPTEN)
  target_sources(api PRIVATE api_x11.cc)
elseif(APPLE)
  if(IOS)
    target_sources(api PRIVATE api_ios.mm)
  else()
    target_sources(api PRIVATE api_mac.mm)
  endif()
elseif(EMSCRIPTEN)
  target_sources(api PRIVATE api_wasm.cc)
elseif(ANDROID)
  target_sources(api PRIVATE api_android.cc)
  target_include_directories(api PUBLIC ${CMAKE_SOURCE_DIR}/third_party/android_native_app_glue)
endif()

add_dependencies(api generate_api_header generate_dawn_headers)

target_include_directories(api PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/third_party/libjpeg-turbo
  ${CMAKE_BINARY_DIR}/third_party/libjpeg-turbo
)

if(EMSCRIPTEN)
  target_include_directories(api BEFORE PUBLIC
    ${DAWN_GEN_DIR}/src/emdawnwebgpu/include
    ${DAWN_GEN_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/dawn/include
  )
  target_link_libraries(api PUBLIC emdawnwebgpu_cpp)
else()
  target_include_directories(api PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/dawn/include
    ${DAWN_GEN_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party/Vulkan-Headers/include
  )
endif()

target_link_libraries(api PUBLIC jpeg-static)
