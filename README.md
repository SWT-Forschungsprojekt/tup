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


# Testing all feeds

TPBI
```shell
./build/tup-backend -P schedule-based -i tpbi -v "https://gtfs.tpbi.ro/api/gtfs-rt/vehiclePositions"
```

Arriva
```shell
./build/tup-backend -P schedule-based -i arriva -v "https://gtfs.ovapi.nl/nl/vehiclePositions.pb"
```

Translink
```shell
./build/tup-backend -P schedule-based -i Translink -v "https://gtfsrt.api.translink.com.au/api/realtime/SEQ/VehiclePositions"
```
Vy Express
```shell
./build/tup-backend -P schedule-based -i VyExpress -v "https://api.entur.io/realtime/v1/gtfs-rt/vehicle-positions?datasource=VYX"
```
Commbus
```shell
./build/tup-backend -P schedule-based -i Commbus -v "https://citycommbus.com/gtfs-rt/vehiclepositions"
```
ZDiTM
```shell
./build/tup-backend -P schedule-based -i ZDiTM -v "https://www.zditm.szczecin.pl/storage/gtfs/gtfs-rt-vehicles.pb"
```
Mountain Line Transit	
```shell
./build/tup-backend -P schedule-based -i MountainLineTransit -v "https://mountainline.syncromatics.com/gtfs-rt/vehiclepositions"
```
City of Madison	
```shell
./build/tup-backend -P schedule-based -i CityOfMadison -v "https://metromap.cityofmadison.com/gtfsrt/vehicles"
```