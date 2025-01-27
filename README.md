# tup

## How to build

### Requirements:

- A recent C++ compiler: Either [Clang](https://llvm.org/) 18 or GCC 13
- CMake 3.17 (or newer): [cmake.org](https://cmake.org/download/) ([Ubuntu APT Repository](https://apt.kitware.com/))
- Ninja: [ninja-build.org](https://ninja-build.org/)
- Git

### Building
```shell
mkdir build && cd build
cmake .. -G Ninja
ninja
```