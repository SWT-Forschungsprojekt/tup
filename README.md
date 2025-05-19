# tup

## How to build

### Requirements

- A recent C++ compiler: Either [Clang](https://llvm.org/) 18 or GCC 13
- CMake 3.17 (or newer): [cmake.org](https://cmake.org/download/) ([Ubuntu APT Repository](https://apt.kitware.com/))
- Ninja: [ninja-build.org](https://ninja-build.org/)
- Git

### Building

```shell
cmake . -G Ninja -B build
ninja -C build
```

### Running

### Requirements:

- wget
- unzip

The tool needs a GTFS feed

```shell
wget -O BUCHAREST-REGION.zip https://gtfs.tpbi.ro/regional/BUCHAREST-REGION.zip
unzip BUCHAREST-REGION.zip -d input
```

Then it can be run as follows:

```shell
./build/tup-backend -i input -v "https://gtfs.tpbi.ro/api/gtfs-rt/vehiclePositions"
```

### Tests
To run the tests, first build it as usual and enter the build directory and run the following:
```shell
ctest
```
