# Copyright 2023 The Toucan Authors
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

vars = {
  'ninja_version': 'version:2@1.11.1.chromium.6',
}

deps = {
  "buildtools"                            : "https://chromium.googlesource.com/chromium/buildtools.git@505de88083136eefd056e5ee4ca0f01fe9b33de8",
  "third_party/abseil-cpp"             : "https://chromium.googlesource.com/chromium/src/third_party/abseil-cpp@4ef9b33175828ea46d091e7e5ec28259d39a8ba5",
  "third_party/getopt"                 : "https://github.com/skeeto/getopt@4e618ef782dc80b2cf0307ea74b68e6a62b025de",
  "third_party/llvm" : "https://github.com/llvm/llvm-project@987087df90026605fc8d03ebda5a1cd31b71e609",
  "third_party/dawn" : "https://dawn.googlesource.com/dawn.git@38ce9873b82c78d03a70afd76b1be0b4b9dd3720",
  "third_party/jinja2" : "https://chromium.googlesource.com/chromium/src/third_party/jinja2@515dd10de9bf63040045902a4a310d2ba25213a0",
  "third_party/libjpeg-turbo" : "https://github.com/libjpeg-turbo/libjpeg-turbo@3b19db4e6e7493a748369974819b4c5fa84c7614",
  "third_party/markupsafe": "https://chromium.googlesource.com/chromium/src/third_party/markupsafe@8f45f5cfa0009d2a70589bcda0349b8cb2b72783",
  "third_party/egl-registry" : "https://github.com/KhronosGroup/EGL-Registry@7dea2ed79187cd13f76183c4b9100159b9e3e071",
  "third_party/opengl-registry" : "https://github.com/KhronosGroup/OpenGL-Registry@eae1d6dde1e283f6fdf803274a2484007e592599",
  "third_party/SPIRV-Headers" : "https://github.com/KhronosGroup/SPIRV-Headers.git@05cc486580771e4fa7ddc89f5c9ee1e97382689a",
  "third_party/SPIRV-Tools" : "https://github.com/KhronosGroup/SPIRV-Tools.git@b0a5c4ac12b742086ffb16e2ba0ad4903450ae1d",
  "third_party/Vulkan-Headers" : "https://github.com/KhronosGroup/Vulkan-Headers@df60f0316899460eeaaefa06d2dd7e4e300c1604",
  "third_party/Vulkan-Tools" : "https://github.com/KhronosGroup/Vulkan-Tools@520163b4b3f3f6e4696a6355f029d9e05d3bb9cf",
  "third_party/Vulkan-Utility-Libraries" : "https://github.com/KhronosGroup/Vulkan-Utility-Libraries@7ec2f4aa9ef807e11b61ea013c272352d647341a",
  "third_party/home-cube" : "https://github.com/SenorBlanco/home-cube@6d801739e9311f37826f08095d09b7345350ab59",
  "third_party/emscripten" : "https://github.com/emscripten-core/emscripten@ee97739c3cbe33777e451a657b43640945e01490",
  "third_party/binaryen" : "https://github.com/WebAssembly/binaryen@11dba9b1c2ad988500b329727f39f4d8786918c5",

  'bin': {
    'packages': [
      {
        'package': 'infra/3pp/tools/ninja/${{platform}}',
        'version': Var('ninja_version'),
      }
    ],
    'dep_type': 'cipd',
  },
}
