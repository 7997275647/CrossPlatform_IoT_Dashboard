# Qt Sensor Dashboard

Qt 6.8+ / QML app that subscribes to MQTT sensor topics, keeps the **last 1024 samples per sensor**
in memory and visualizes them (inspired by the SEBA Hydrocenter portal: latest-value tiles,
time-series graphs, value tables, min/max/avg lines, CSV export).

## Topics and payloads

With base topic `sensors` (editable in the UI):

| Topic                 | Accepted payloads                                              |
|-----------------------|----------------------------------------------------------------|
| `sensors/temperature` | `23.5` or `{"temperature": 23.5}` (or `{"value": 23.5}`)       |
| `sensors/humidity`    | `48.2` or `{"humidity": 48.2}` (or `{"value": 48.2}`)          |
| `sensors/gyro`        | `{"x":1,"y":2,"z":3}`, `[1,2,3]` or `1,2,3`                    |

Samples are timestamped on arrival. Once 1024 samples are stored, the oldest is dropped.

## Build

Requires Qt 6.8+ with the **Qt MQTT** module (`aqt install-qt ... -m qtmqtt`), CMake and a C++17 compiler.

    cmake -S . -B build -DCMAKE_PREFIX_PATH=<path-to-Qt>
    cmake --build build

## Try it

1. Start a broker, e.g. `mosquitto -v`.
2. Start the simulator: `pip install paho-mqtt` then `python tools/simulate_sensors.py`.
3. Run the app and press **Connect**.

Code layout: `SensorChannel` (ring buffer + table model + stats + CSV export), `SensorHub`
(MQTT client and topic routing), `SensorChart.qml` (Canvas line chart), `SensorTable.qml`, `Main.qml`.
