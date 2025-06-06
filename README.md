# Toucan

## Overview

*This is not an officially supported Google product.*

Toucan is an experimental, domain-specific language for accelerated graphical
applications. It was designed to answer the following question: "Could a
unified programming environment reduce the friction of developing CPU/GPU
apps?". Built on WebGPU and featuring unified CPU/GPU SIMD datatypes, syntax
and API, it aims for native-level performance with a single, concise source.
It currently runs on Windows, MacOS, Linux, Android, and Web.

For further information, see [this presentation](https://docs.google.com/presentation/d/1tUlG9w7AsP8pexBWRwk1uYDrwNMvTJqcxdH482ip4LM/edit?usp=sharing&resourcekey=0-gEpoxkfBbXC3qbbKRQhrwg) and [this document](https://docs.google.com/document/d/1oWNt2IoA1u-j7i2D24pnlFP6JIlQdrKZlNY6qoW2K80/edit?usp=sharing&resourcekey=0-mb8K3ATyRv-ZGl34Y4C-zQ).

## Viewing the Toucan samples

If you simply wish to try some live Toucan demos without building the toolchain, navigate to the [Toucan Samples page](https://senorblanco.github.io/toucan-samples/) and follow the instructions.

## Building the Toucan toolchain

### Prerequisites

- Python 3.0
- CMake
- A C++20 toolchain (clang, gcc, or MSVC)
- GNU flex
- GNU bison
- node.js and npm (for the WebAssembly build)

Toucan uses the GN and ninja build systems from the Chromium project.
These will be retrieved by the git-sync-deps script.

### Target executables

- `tj`: the Toucan JIT compiler. Accepts a single Toucan file, performs compilation, semantic analysis, code generation, JITs the result, and runs it with an embedded runtime API. Runs on Windows/D3D11, Mac/Metal (x86) and Linux/Vulkan.
- `tc`: the Toucan static compiler. Performs compilation, semantic analysis and code generation, and produces a native executable. Targets all of the above, plus the Web (via WebAssembly).
- `springy`: a particle-based physics demo, with a 10x10 grid of particles
- `springy-compute`: the same particle demo, running in a compute shader
- `springy-3d`: a larger, 3D mesh version of "springy"
- `springy-compute-3d`: a compute-based version of "springy-3d"
- `shiny`: a cube-mapped reflective Utah teapot, rendered in a skybox

## Build instructions (Linux, Mac)

1. Run `tools/git-sync-deps` to update `third_party` dependencies

2. Build LLVM (Makefiles):
```
   pushd third_party/llvm/llvm
   mkdir -p out/Release
   cd out/Release
   cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="lld;clang" -DLLVM_TARGETS_TO_BUILD="host;X86;ARM;AArch64;WebAssembly" -DLLVM_BUILD_LLVM_DYLIB=On -DLLVM_DYLIB_COMPONENTS=all -DLLVM_INCLUDE_BENCHMARKS=false -DLLVM_INCLUDE_EXAMPLES=false -DLLVM_INCLUDE_TESTS=false ../..
   make
   popd
```

3. Build tc, tj and native samples:

```
   mkdir -p out/Release
   echo "is_debug=false" > out/Release/args.gn
   gn gen out/Release
   ninja -C out/Release
```

## Build instructions (Windows)

1. Run `tools\git-sync-deps.bat` to update `third_party` dependencies

2. Build LLVM (Visual Studio project files):
```
   pushd third_party\llvm\llvm
   mkdir out
   cd out
   cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="lld;clang" -DLLVM_TARGETS_TO_BUILD="host;X86;ARM;AArch64;WebAssembly" -DLLVM_INCLUDE_BENCHMARKS=false -DLLVM_INCLUDE_EXAMPLES=false -DLLVM_INCLUDE_TESTS=false ..
   cmake --build . --config Release
   popd
```

3. Install Bison and Flex:
```
   python3 tools/fetch-win-flex-bison.py
```

4. Build tc, tj and native samples:

```
   mkdir out\Release
   echo is_debug=false > out\Release\args.gn
   gn gen out\Release
   ninja -C out\Release
```

## Build instructions (Android)

1. Run `tools/git-sync-deps` to update `third_party` dependencies

2. Install [Java 17](https://www.oracle.com/java/technologies/javase/jdk17-archive-downloads.html) or later.

3. Install the [Android SDK command line tools](https://developer.android.com/studio/index.html#command-line-tools-only), and ensure `ANDROID_SDK_ROOT` is set.

4. Install all required Android SDK components:

```
$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager --install 'platforms;android-26' 'ndk;26.3.11579264' 'build-tools;34.0.0' platform-tools
```
or on Windows:

```
%ANDROID_SDK_ROOT%\cmdline-tools\latest\bin\sdkmanager --install platforms;android-26 ndk;26.3.11579264 build-tools;34.0.0 platform-tools
```

5. Set `ANDROID_NDK_ROOT` to `$ANDROID_SDK_ROOT/ndk/26.3.11579264`.

6. Build LLVM (Makefiles):
```
   pushd third_party/llvm/llvm
   mkdir -p out/Release
   cd out/Release
   cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="lld;clang" -DLLVM_TARGETS_TO_BUILD="host;X86;ARM;AArch64;WebAssembly" -DLLVM_INCLUDE_BENCHMARKS=false -DLLVM_INCLUDE_EXAMPLES=false -DLLVM_INCLUDE_TESTS=false ../..
   make
   popd
```

7. Build tc and native samples:

```
   mkdir -p out/Release-android
   echo is_debug=false > out/Release-android/args.gn
   echo target_os=\"android\" >> out/Release-android/args.gn
   echo target_cpu=\"arm64\" >> out/Release-android/args.gn
   echo android_ndk_dir=\"$ANDROID_NDK_ROOT\" >> out/Release-android/args.gn
   echo android_sdk_dir=\"$ANDROID_SDK_ROOT\" >> out/Release-android/args.gn
   gn gen out/Release-android
   ninja -C out/Release-android
```

## Build instructions (WebAssembly)

1. Bootstrap emscripten
```
pushd third_party/emscripten
./bootstrap
popd
```

2. Build binaryen

```
pushd third_party/binaryen
git submodule init
git submodule update
cmake . && make
popd
```

3. Build WASM targets

```
mkdir -p out/Release-wasm
echo 'is_debug=false\
target_os="wasm"\
target_cpu="wasm"' > out/Release-wasm/args.gn
gn gen out/Release-wasm
ninja -C out/Release-wasm
```

## Running tj, the Toucan JIT:

```
out/DIR/tj <filename>
```

e.g., `out/Release/tj samples/springy.t`

## Running native samples

```
out/DIR/<filename>
```

e.g., `out/Release/springy`

## Running WebAssembly samples

- `npx http-server out/Release-wasm`
- start Chrome with the runtime flag `--enable-features=WebAssemblyExperimentalJSPI`, or enable "Experimental WebAssembly JavaScript Promise Integration (JSPI)" in about:flags
- open `http://localhost:8080`
- open sample (e.g., springy.html)

## Installing and running Android samples

```
$ANDROID_SDK_ROOT/platform-tools/adb install -r out/Release-android/$SAMPLE.apk
$ANDROID_SDK_ROOT/platform-tools/adb shell am start -a android.intent.action.MAIN -n org.toucanlang.$SAMPLE/android.app.NativeActivity
```

e.g.,

```
$ANDROID_SDK_ROOT/platform-tools/adb install -r out/Release-android/springy.apk
$ANDROID_SDK_ROOT/platform-tools/adb shell am start -a android.intent.action.MAIN -n org.toucanlang.springy/android.app.NativeActivity
```

## Testing instructions

There is a primitive test harness in test/test.py, and a set of end-to-end
tests. This requires tj, so it is only supported on desktop platforms.

On Linux and MacOS:
```
test/test.py >& test/test-expectations.txt
```

On Windows:
```
python3 test\test.py > test\test-expectations.txt 2>&1
python3 tools\unixify.py test\test-expectations.txt
```
