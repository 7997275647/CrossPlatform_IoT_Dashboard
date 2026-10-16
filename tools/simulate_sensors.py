"""Publishes simulated temperature, humidity and gyro readings over MQTT.

    pip install paho-mqtt
    python tools/simulate_sensors.py [--host localhost] [--port 1883] [--base sensors] [--rate 10]
"""
import argparse
import json
import math
import random
import time

import paho.mqtt.client as mqtt


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--base", default="sensors")
    parser.add_argument("--rate", type=float, default=10.0, help="samples per second")
    args = parser.parse_args()

    try:
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)  # paho-mqtt >= 2.0
    except AttributeError:
        client = mqtt.Client()
    client.connect(args.host, args.port)
    client.loop_start()

    start = time.time()
    while True:
        t = time.time() - start
        temperature = 22 + 3 * math.sin(t / 20) + random.gauss(0, 0.1)
        humidity = 50 + 10 * math.sin(t / 30 + 1) + random.gauss(0, 0.3)
        gyro = {
            "x": 30 * math.sin(t),
            "y": 30 * math.cos(t * 0.7),
            "z": 10 * math.sin(t * 2) + random.gauss(0, 0.5),
        }
        client.publish(f"{args.base}/temperature", f"{temperature:.2f}")
        client.publish(f"{args.base}/humidity", json.dumps({"humidity": round(humidity, 2)}))
        client.publish(f"{args.base}/gyro", json.dumps(gyro))
        time.sleep(1.0 / args.rate)


if __name__ == "__main__":
    main()
