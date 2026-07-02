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

if(CMAKE_CROSSCOMPILING)
  include(ExternalProject)
  ExternalProject_Add(tc
      SOURCE_DIR ${CMAKE_SOURCE_DIR}
      CMAKE_ARGS -DCMAKE_CROSSCOMPILING=OFF -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                 -UCMAKE_TOOLCHAIN_FILE
      BINARY_DIR ${CMAKE_BINARY_DIR}/host
      INSTALL_COMMAND ""
      BUILD_COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/host --target tc
      BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/host/tc
      USES_TERMINAL_CONFIGURE TRUE
      USES_TERMINAL_BUILD TRUE
  )
else()
  add_executable(tc tc.cc)

  target_include_directories(tc PRIVATE ${CMAKE_SOURCE_DIR} ${LLVM_INCLUDE_DIRS})

  target_compile_definitions(tc PRIVATE API_PATH="${CMAKE_SOURCE_DIR}/api")

  target_link_libraries(tc PRIVATE ast gen_bindings_sources codegen parser ${LLVM_LIBS})

  if(WIN32)
    target_include_directories(tc PRIVATE ${CMAKE_SOURCE_DIR}/third_party/getopt)
  endif()

  add_executable(tj tj.cc)
  set_target_properties(tj PROPERTIES ENABLE_EXPORTS TRUE)

  target_include_directories(tj PRIVATE ${CMAKE_SOURCE_DIR} ${LLVM_INCLUDE_DIRS})

  target_compile_definitions(tj PRIVATE API_PATH="${CMAKE_SOURCE_DIR}/api")

  target_link_directories(tj PRIVATE ${CMAKE_BINARY_DIR})

  target_link_libraries(tj PRIVATE api ast codegen parser dawn_proc webgpu_dawn ${LLVM_LIBS})

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(tj PRIVATE X11 X11-xcb dl pthread z zstd curses)
  elseif(APPLE)
    find_library(APPKIT_FRAMEWORK AppKit)
    find_library(METAL_FRAMEWORK Metal)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)
    target_link_libraries(tj PRIVATE
      ${APPKIT_FRAMEWORK}
      ${METAL_FRAMEWORK}
      ${QUARTZCORE_FRAMEWORK}
      z curses
    )
  elseif(WIN32)
    target_include_directories(tj PRIVATE ${CMAKE_SOURCE_DIR}/third_party/getopt)
    target_link_libraries(tj PRIVATE gdi32 user32 ws2_32 ntdll)
  endif()
endif()
