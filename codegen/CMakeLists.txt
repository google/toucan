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

add_library(codegen STATIC codegen_llvm.cc codegen_spirv.cc)

target_include_directories(codegen PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}/bindings
  ${CMAKE_SOURCE_DIR}/third_party/SPIRV-Headers/include
)

target_include_directories(codegen PUBLIC ${LLVM_INCLUDE_DIRS})

target_link_libraries(codegen PUBLIC ast tint_api)
