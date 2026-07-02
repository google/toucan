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

if(ANDROID)
  add_library(android_main STATIC android_main.cc)
  target_include_directories(android_main PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/android_native_app_glue
  )
endif()
add_library(samples_main STATIC main.cc)
target_include_directories(samples_main PUBLIC ${CMAKE_SOURCE_DIR})

if(BUILD_SAMPLES)
  toucan_app(decode SOURCES decode.t)
  toucan_app(disco-dragon SOURCES disco-dragon.t)
  toucan_app(skybox SOURCES skybox.t)
  toucan_app(springy SOURCES springy.t)
  toucan_app(springy-3d SOURCES springy-3d.t)
  toucan_app(springy-compute SOURCES springy-compute.t)
  toucan_app(springy-compute-3d SOURCES springy-compute-3d.t)
  toucan_app(shiny SOURCES shiny.t)
  toucan_app(shiny-anim SOURCES shiny-anim.t)
  toucan_app(shiny-compute-anim SOURCES shiny-compute-anim.t)
  toucan_app(shiny-dragon SOURCES shiny-dragon.t)
  toucan_app(triangle SOURCES triangle.t)
  toucan_app(gouraud SOURCES gouraud.t)
  toucan_app(window SOURCES window.t)
  toucan_app(mipped-cube SOURCES mipped-cube.t)
endif()
