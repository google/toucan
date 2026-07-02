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

cmake_minimum_required(VERSION 3.20)

if(CMAKE_GENERATOR STREQUAL "Ninja")
  set(CMAKE_MAKE_PROGRAM ${CMAKE_SOURCE_DIR}/third_party/ninja/ninja CACHE FILEPATH "" FORCE)
endif()

project(Toucan CXX C)

# Build all executables and dynamic libs in the root output directory.
# This matches the GN build.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(WIN32)
  set(FLEX_EXECUTABLE ${CMAKE_SOURCE_DIR}/third_party/winflexbison/win_flex.exe)
  set(BISON_EXECUTABLE ${CMAKE_SOURCE_DIR}/third_party/winflexbison/win_bison.exe)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# CMake 3.31+ reserves "codegen" as a built-in target name. Set CMP0171 to OLD
# to permit Toucan's target library named "codegen".
if(POLICY CMP0171)
  cmake_policy(SET CMP0171 OLD)
endif()

option(BUILD_SAMPLES "Build Toucan samples" ON)
option(BUILD_TESTS "Build Toucan tests" ON)

add_compile_definitions("STACK_SIZE=4194304")

set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_BUILD_MONOLITHIC_LIBRARY ON)
set(DAWN_BUILD_TESTS OFF)
set(DAWN_BUILD_SAMPLES OFF)
set(DAWN_USE_GLFW OFF)

if(WIN32)
  set(DAWN_ENABLE_VULKAN OFF)
endif()

set(TINT_BUILD_CMD_TOOLS OFF)
set(TINT_BUILD_TESTS OFF)
if(NOT EMSCRIPTEN)
  set(TINT_BUILD_SPV_READER ON)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT CMAKE_CROSSCOMPILING)
  if (NOT WIN32)
    set(LLVM_DIR "${CMAKE_SOURCE_DIR}/third_party/llvm/llvm/out/${CMAKE_BUILD_TYPE}")
  else()
    set(LLVM_DIR "${CMAKE_SOURCE_DIR}/third_party/llvm/llvm/out/lib/cmake")
  endif()

  find_package(LLVM REQUIRED CONFIG
               COMPONENTS engine mcjit x86codegen armcodegen aarch64codegen webassemblycodegen
               HINTS ${LLVM_DIR})

  set(LLVM_LIBS LLVM)
  if(WIN32)
    set(LLVM_LIBS
      LLVMWebAssemblyCodeGen
      LLVMWebAssemblyUtils
      LLVMWebAssemblyDesc
      LLVMWebAssemblyInfo
      LLVMAArch64CodeGen
      LLVMPasses
      LLVMHipStdPar
      LLVMCoroutines
      LLVMAArch64Desc
      LLVMAArch64Utils
      LLVMAArch64Info
      LLVMARMCodeGen
      LLVMipo
      LLVMVectorize
      LLVMSandboxIR
      LLVMLinker
      LLVMFrontendOpenMP
      LLVMFrontendDirective
      LLVMFrontendAtomic
      LLVMFrontendOffloading
      LLVMObjectYAML
      LLVMARMDesc
      LLVMARMUtils
      LLVMARMInfo
      LLVMX86CodeGen
      LLVMX86Desc
      LLVMX86Info
      LLVMMCDisassembler
      LLVMInstrumentation
      LLVMIRPrinter
      LLVMGlobalISel
      LLVMSelectionDAG
      LLVMCFGuard
      LLVMAsmPrinter
      LLVMMCJIT
      LLVMInterpreter
      LLVMExecutionEngine
      LLVMRuntimeDyld
      LLVMOrcTargetProcess
      LLVMOrcShared
      LLVMCodeGen
      LLVMTarget
      LLVMScalarOpts
      LLVMInstCombine
      LLVMAggressiveInstCombine
      LLVMObjCARCOpts
      LLVMTransformUtils
      LLVMCodeGenTypes
      LLVMCGData
      LLVMBitWriter
      LLVMAnalysis
      LLVMProfileData
      LLVMSymbolize
      LLVMDebugInfoBTF
      LLVMDebugInfoPDB
      LLVMDebugInfoMSF
      LLVMDebugInfoCodeView
      LLVMDebugInfoGSYM
      LLVMDebugInfoDWARF
      LLVMObject
      LLVMTextAPI
      LLVMMCParser
      LLVMIRReader
      LLVMAsmParser
      LLVMMC
      LLVMDebugInfoDWARFLowLevel
      LLVMBitReader
      LLVMFrontendHLSL
      LLVMCore
      LLVMRemarks
      LLVMBitstreamReader
      LLVMBinaryFormat
      LLVMTargetParser
      LLVMSupport
      LLVMDemangle
    )
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")
include(ToucanExecutable)

# CMAKE_INSTALL_DOCDIR is set to satisfy libjpeg-turbo's configure-time
# install() syntax check without requiring full GNUInstallDirs. Note that
# libjpeg-turbo does not actually build docs during compilation.
set(CMAKE_INSTALL_DOCDIR "doc")
set(ENABLE_SHARED OFF CACHE BOOL "Build shared libraries" FORCE)
set(ENABLE_STATIC ON CACHE BOOL "Build static libraries" FORCE)
set(WITH_SIMD OFF CACHE BOOL "Include SIMD extensions" FORCE)
set(WITH_TURBOJPEG OFF CACHE BOOL "Include TurboJPEG API" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "Minimum policy version" FORCE)
add_subdirectory(third_party/dawn EXCLUDE_FROM_ALL)
add_subdirectory(third_party/libjpeg-turbo EXCLUDE_FROM_ALL)
add_subdirectory(parser EXCLUDE_FROM_ALL)
add_subdirectory(ast EXCLUDE_FROM_ALL)
add_subdirectory(bindings EXCLUDE_FROM_ALL)
add_subdirectory(api EXCLUDE_FROM_ALL)
add_subdirectory(codegen EXCLUDE_FROM_ALL)
add_subdirectory(compilers)
add_subdirectory(samples)
add_subdirectory(test)
if(EMSCRIPTEN)
add_subdirectory(emscripten)
endif()
if(ANDROID)
add_subdirectory(third_party/android_native_app_glue EXCLUDE_FROM_ALL)
endif()
