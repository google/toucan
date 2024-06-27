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
  "buildtools"                            : "https://chromium.googlesource.com/chromium/src/buildtools@efa920ce144e4dc1c1841e73179cd7e23b9f0d5e",
  "third_party/abseil-cpp"             : "https://chromium.googlesource.com/chromium/src/third_party/abseil-cpp@f81f6c011baf9b0132a5594c034fe0060820711d",
  "third_party/getopt"                 : "https://github.com/skeeto/getopt@4e618ef782dc80b2cf0307ea74b68e6a62b025de",
  "third_party/llvm" : "https://github.com/llvm/llvm-project@987087df90026605fc8d03ebda5a1cd31b71e609",
  "third_party/dawn" : "https://dawn.googlesource.com/dawn.git@d75047380f40c11649b8d8b0b5f0ddc3d7f9b594",
  "third_party/jinja2" : "https://chromium.googlesource.com/chromium/src/third_party/jinja2@e2d024354e11cc6b041b0cff032d73f0c7e43a07",
  "third_party/libjpeg-turbo" : "https://github.com/libjpeg-turbo/libjpeg-turbo@3b19db4e6e7493a748369974819b4c5fa84c7614",
  "third_party/markupsafe": "https://chromium.googlesource.com/chromium/src/third_party/markupsafe@0bad08bb207bbfc1d6f3bbc82b9242b0c50e5794",
  "third_party/egl-registry" : "https://github.com/KhronosGroup/EGL-Registry@7dea2ed79187cd13f76183c4b9100159b9e3e071",
  "third_party/opengl-registry" : "https://github.com/KhronosGroup/OpenGL-Registry@eae1d6dde1e283f6fdf803274a2484007e592599",
  "third_party/SPIRV-Headers" : "https://github.com/KhronosGroup/SPIRV-Headers.git@2acb319af38d43be3ea76bfabf3998e5281d8d12",
  "third_party/SPIRV-Tools" : "https://github.com/KhronosGroup/SPIRV-Tools.git@ca004da9f9c7fa7ed536709823bd604fab3cd7da",
  "third_party/Vulkan-Headers" : "https://github.com/KhronosGroup/Vulkan-Headers@e3c37e6e184a232e10b01dff5a065ce48c047f88",
  "third_party/Vulkan-Tools" : "https://github.com/KhronosGroup/Vulkan-Tools@345af476e583366352e014ee8e43fc5ddf421ab9",
  "third_party/Vulkan-Utility-Libraries" : "https://github.com/KhronosGroup/Vulkan-Utility-Libraries@60fe7d0c153dc07325a8fb45310723a1767db811",
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
