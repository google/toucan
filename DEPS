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
  "buildtools"                            : "https://chromium.googlesource.com/chromium/src/buildtools@9cac81256beb5d4d36c8801afeae38fea34b8486",
  "third_party/abseil-cpp"             : "https://chromium.googlesource.com/chromium/src/third_party/abseil-cpp@04dc59d2c83238cb1fcb49083e5e416643a899ce",
  "third_party/getopt"                 : "https://github.com/skeeto/getopt@4e618ef782dc80b2cf0307ea74b68e6a62b025de",
  "third_party/llvm" : "https://github.com/llvm/llvm-project@6146a88f60492b520a36f8f8f3231e15f3cc6082",
  "third_party/dawn" : "https://dawn.googlesource.com/dawn.git@9b84810b13a112425d59661546424d5295cfdd5d",
  "third_party/jinja2" : "https://chromium.googlesource.com/chromium/src/third_party/jinja2@e2d024354e11cc6b041b0cff032d73f0c7e43a07",
  "third_party/libjpeg-turbo" : "https://github.com/libjpeg-turbo/libjpeg-turbo@3b19db4e6e7493a748369974819b4c5fa84c7614",
  "third_party/markupsafe": "https://chromium.googlesource.com/chromium/src/third_party/markupsafe@0bad08bb207bbfc1d6f3bbc82b9242b0c50e5794",
  "third_party/egl-registry" : "https://github.com/KhronosGroup/EGL-Registry@7dea2ed79187cd13f76183c4b9100159b9e3e071",
  "third_party/opengl-registry" : "https://github.com/KhronosGroup/OpenGL-Registry@5bae8738b23d06968e7c3a41308568120943ae77",
  "third_party/SPIRV-Headers" : "https://github.com/KhronosGroup/SPIRV-Headers.git@7168a5ad041f6b6b9170f027c7417f98a2056ff0",
  "third_party/SPIRV-Tools" : "https://github.com/KhronosGroup/SPIRV-Tools.git@2e743e96b990f7390290630243b00ca0a75b8547",
  "third_party/Vulkan-Headers" : "https://github.com/KhronosGroup/Vulkan-Headers@b11eecd68fb4b770f30fe2c9da522ff966f95b1e",
  "third_party/Vulkan-Tools" : "https://github.com/KhronosGroup/Vulkan-Tools@e4fb76dc08f139df0436e9c3031f75be5e1f6264",
  "third_party/Vulkan-Utility-Libraries" : "https://github.com/KhronosGroup/Vulkan-Utility-Libraries@03e1445cc7cce22baeeef8eff7bb934362d040eb",
  "third_party/home-cube" : "https://github.com/SenorBlanco/home-cube@6d801739e9311f37826f08095d09b7345350ab59",
  "third_party/emscripten" : "https://github.com/emscripten-core/emscripten@52c177bdaedc9b1cd1cffcec0372ed395d722788",
  "third_party/binaryen" : "https://github.com/WebAssembly/binaryen@700fa15b0ce8a3613702e21d2ed7ce3f028f18a6",

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
