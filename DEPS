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
  "third_party/abseil-cpp"             : "https://chromium.googlesource.com/chromium/src/third_party/abseil-cpp@bc3ab29356a081d0b5dd4ac55e30f4b45d8794cc",
  "third_party/llvm" : "https://github.com/llvm/llvm-project@e37fc3cc3915196fa6d3bc3df27d7683857bbec5",
  "third_party/dawn" : "https://dawn.googlesource.com/dawn.git@f26b1269bdc7a450c02aa7d198028d2501fe3678",
  "third_party/jinja2" : "https://chromium.googlesource.com/chromium/src/third_party/jinja2@515dd10de9bf63040045902a4a310d2ba25213a0",
  "third_party/libjpeg-turbo" : "https://github.com/libjpeg-turbo/libjpeg-turbo@3b19db4e6e7493a748369974819b4c5fa84c7614",
  "third_party/markupsafe": "https://chromium.googlesource.com/chromium/src/third_party/markupsafe@8f45f5cfa0009d2a70589bcda0349b8cb2b72783",
  "third_party/opengl-registry" : "https://github.com/KhronosGroup/OpenGL-Registry@eae1d6dde1e283f6fdf803274a2484007e592599",
  "third_party/SPIRV-Headers" : "https://github.com/KhronosGroup/SPIRV-Headers.git@7f1d2f4158704337aff1f739c8e494afc5716e7e",
  "third_party/SPIRV-Tools" : "https://github.com/KhronosGroup/SPIRV-Tools.git@6110f30a36775e4d452968407f12b16694df2cc8",
  "third_party/Vulkan-Headers" : "https://github.com/KhronosGroup/Vulkan-Headers@870a531486f77dfaf124395de80ed38867400d31",
  "third_party/Vulkan-Tools" : "https://github.com/KhronosGroup/Vulkan-Tools@df10a2759b4b60d59b735882217a749d8e5be660",
  "third_party/home-cube" : "https://github.com/SenorBlanco/home-cube@6d801739e9311f37826f08095d09b7345350ab59",
  "third_party/emscripten" : "https://github.com/emscripten-core/emscripten@09e43acbc620d4d851dd883b980efe1cc96f7d17",
  "third_party/binaryen" : "https://github.com/WebAssembly/binaryen@ef7f98e50662374b17d88c149a2ba1c11f918e5c",

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
