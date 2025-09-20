# qcp

Experimental C compiler front‑end that tokenizes, parses, and emits code via LLVM (IR, bitcode, or objects). Includes tests and optional micro-benchmarks.

## Prerequisites
- CMake ≥ 3.7, C++20 compiler (Clang recommended)
- LLVM with CMake config files (find_package(LLVM CONFIG REQUIRED))
- Optional: gperf (for keyword generation), Google Benchmark (only if benchmarks enabled)

Tip (macOS/Homebrew):
```
brew install llvm cmake gperf
cmake -S . -B build -DLLVM_DIR="$(brew --prefix llvm)/lib/cmake/llvm"
```

## Build
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

CMake options:
- -DQCP_BUILD_TESTS=ON/OFF (default ON)
- -DQCP_BUILD_BENCHMARKS=ON/OFF (default OFF)
- -DQCP_BUILD_MUSL_LIBC=ON/OFF (default ON)

## Run
Show help and available emitter options:
```
./build/qcp -h
```
Compile only, write object/IR/bitcode:
```
./build/qcp -c test/examples.c -o examples.o
./build/qcp -l -c test/examples.c -o examples.ll   # emit LLVM IR
./build/qcp -b -c test/examples.c -o examples.bc   # emit bitcode
```
Full compile and link (uses system ld):
```
./build/qcp test/examples.c -o examples
```
Preprocess only:
```
./build/qcp -E test/examples.c -o examples.i
```

Common flags: -I/-D/-U (preprocessor), -L/-l (linker), --pp <cmd>, --ld <path>, -p (no-pp). Use `-h` for full list.

## Tests
```
cmake -S . -B build -DQCP_BUILD_TESTS=ON
cmake --build build --target tester
ctest --test-dir build --output-on-failure
```

## Benchmarks (optional)
```
cmake -S . -B build -DQCP_BUILD_BENCHMARKS=ON
cmake --build build --target bench
./build/bench
```

## Linting (clang-tidy targets)
```
cmake --build build --target lint
```
