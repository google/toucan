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
  "third_party/llvm" : "https://github.com/llvm/llvm-project@4c26a1e4d7e490a38dcd2a24e4c8939075fd4a5a",
  "third_party/dawn" : "https://dawn.googlesource.com/dawn.git@a7b805c0712ea6480f8dfd0f3151685aa3d10e66",
  "third_party/jinja2" : "https://chromium.googlesource.com/chromium/src/third_party/jinja2@e2d024354e11cc6b041b0cff032d73f0c7e43a07",
  "third_party/libjpeg-turbo" : "https://github.com/libjpeg-turbo/libjpeg-turbo@3b19db4e6e7493a748369974819b4c5fa84c7614",
  "third_party/markupsafe": "https://chromium.googlesource.com/chromium/src/third_party/markupsafe@0bad08bb207bbfc1d6f3bbc82b9242b0c50e5794",
  "third_party/egl-registry" : "https://github.com/KhronosGroup/EGL-Registry@7dea2ed79187cd13f76183c4b9100159b9e3e071",
  "third_party/opengl-registry" : "https://github.com/KhronosGroup/OpenGL-Registry@eae1d6dde1e283f6fdf803274a2484007e592599",
  "third_party/SPIRV-Headers" : "https://github.com/KhronosGroup/SPIRV-Headers.git@a62b032007b2e7a69f24a195cbfbd0cf22d31bb0",
  "third_party/SPIRV-Tools" : "https://github.com/KhronosGroup/SPIRV-Tools.git@c173df736cb89ba4daba0beec039317292f22e0d",
  "third_party/Vulkan-Headers" : "https://github.com/KhronosGroup/Vulkan-Headers@29f979ee5aa58b7b005f805ea8df7a855c39ff37",
  "third_party/Vulkan-Tools" : "https://github.com/KhronosGroup/Vulkan-Tools@6991ccb68890681b1a3e0f56acd8f53b20ad1e79",
  "third_party/Vulkan-Utility-Libraries" : "https://github.com/KhronosGroup/Vulkan-Utility-Libraries@0a786ee3e4fd3602f68ff0ffd9fdcb12e0efb646",
  "third_party/home-cube" : "https://github.com/SenorBlanco/home-cube@6d801739e9311f37826f08095d09b7345350ab59",
  "third_party/emscripten" : "https://github.com/emscripten-core/emscripten@d9c260d877fab10f34241325ec4c4c2b072d899c",
  "third_party/binaryen" : "https://github.com/WebAssembly/binaryen@5fca52781efe63c1683c436cb0c5e08cc4a87b9e",

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
