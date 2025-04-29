# Integration Tests
When tup itself has been built, one might want to try it in combination with consumer software to test it properly.
This described how to set up motis together with tup with consistent datasets.

## Setup tup + MOTIS
1. Get motis and extract it:
    ```shell
    wget -O motis.tar.bz2 https://github.com/motis-project/motis/releases/latest/download/motis-linux-amd64.tar.bz2
    ```
    ```shell
    tar -xf motis.tar.bz2
    ```
2. Download the GTFS-Feed of your choice:
    ```shell
    wget -O gtfs.zip https://gtfs.tpbi.ro/regional/BUCHAREST-REGION.zip
    unzip gtfs.zip -d input
    ```
3. Start tup with the matching vehiclePosition feed:
    ```shell
    ../build/tup-backend -i input -v "https://gtfs.tpbi.ro/api/gtfs-rt/vehiclePositions"
    ```
4. Download an according OpenStreetMap extract:
    ```shell
    wget -O region-latest.osm.pbf https://download.geofabrik.de/europe/romania-latest.osm.pbf
    ```
5. Create a `config.yml` file with the according content:
    ```yml
    server:
      port: 8080
      web_folder: ui
    osm: region-latest.osm.pbf
    timetable:
      datasets:
        gtfs:
          path: gtfs.zip
          rt:
            - url: http://localhost:8000/tripUpdates
    tiles:
      profile: tiles-profiles/full.lua
    geocoding: true
    osr_footpath: false
    ```

6. Run motis import first and then start the server:
    ```shell
    ./motis import
    ./motis server
    ```
