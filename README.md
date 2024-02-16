# Toucan

## Overview

Toucan is a memory-safe, type-safe, domain-specific language for
accelerated graphical applications.  Featuring unified CPU/GPU
datatypes, syntax and API, specifically focused on high-performance
graphics. It runs on Windows, MacOS, Linux, and WASM.

## Target executables

- `tj`: the Toucan JIT compiler, and runs on Windows/D3D11, Mac/Metal and Linux/Vulkan. Accepts a single Toucan file, performs compilation, semantic analysis, code generation, JITs the result, and runs it with an embedded runtime API.
- `tc`: the Toucan static compiler, and targets all of the above plus WASM. Performs compilation, semantic analysis and code generation, and produces a native executable.
- `springy`: a particle-based physics demo, with a 10x10 grid of particles
- `springy-compute`: the same particle demo, running in a compute shader
- `springy-3d`: a larger, 3D mesh version of "springy"
- `springy-compute-3d`: a compute-based version of "springy-3d"
- `shiny`: a cube-mapped reflective Utah teapot, rendered in a skybox

## Build instructions

1) Run `python3 tools/git-sync-deps` to update `third_party` dependencies

2a) Build LLVM (Makefiles):
```
   pushd third_party/llvm/llvm
   mkdir -p out/Release
   cd out/Release
   cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="lld;clang" -DLLVM_TARGETS_TO_BUILD="host;WebAssembly" -DLLVM_INCLUDE_BENCHMARKS=false -DLLVM_INCLUDE_EXAMPLES=false -DLLVM_INCLUDE_TESTS=false ../..
   make [-j whatever]
   popd
```

2b) Build LLVM (Visual Studio project files):
```
   pushd third_party\llvm\llvm
   mkdir out
   cd out
   cmake -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_PROJECTS="lld;clang" -DLLVM_TARGETS_TO_BUILD="host;WebAssembly" -DLLVM_INCLUDE_BENCHMARKS=false -DLLVM_INCLUDE_EXAMPLES=false -DLLVM_INCLUDE_TESTS=false ..
   cmake --build . --config Release
   popd
```

3) (Windows only)
   Install GnuWin32 versions of bison and flex:
   https://gnuwin32.sourceforge.net/packages/bison.htm
   https://gnuwin32.sourceforge.net/packages/flex.htm
   Download modified bison from http://marin.jb.free.fr/bison/
   Overwrite Program Files (x86)\GnuWin32\bin\bison.exe with the modified version.

4) Build tj:

```
   mkdir -p out/Release
   echo "is_debug=false" > out/Release/args.gn
   gn gen out/Release
   ninja -C out/Release
```

## Run instructions

out/DIR/tj <filename>

e.g., out/Release/tj samples/springy.t

## Testing instructions

There is a primitive test harness in test/test.py, and a set of end-to-end
tests. Run it as follows:

`test/test.py >& test/test-expectations.txt`

## Building for WASM

1) Bootstrap emscripten
```
pushd third_party/emscripten
./bootstrap
popd
```

2) Build binaryen

```
pushd third_party/binaryen
git submodule init
git submodule update
cmake . && make
popd
```

3) Build WASM targets

```
mkdir -p out/wasm
echo 'is_debug=false\
target_os="wasm"\
target_cpu="wasm"' > out/wasm/args.gn
gn gen out/wasm
ninja -C out/wasm
```
