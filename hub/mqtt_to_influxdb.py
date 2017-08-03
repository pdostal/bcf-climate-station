#!/usr/bin/env python3
import os
import sys
from logging import DEBUG, INFO
import logging as log
import json
import time
import datetime
import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

LOG_FORMAT = '%(asctime)s %(levelname)s: %(message)s'

if os.path.isfile("mqtt_to_influxdb_conf.py"):
    from mqtt_to_influxdb_conf import *
else:
    MQTT_HOSTNAME = "localhost"
    MQTT_PORT = 1883
    MQTT_TOPICS = ["node/#", "nodes/#"]
    INFLUX_HOSTNAME = "localhost"
    INFLUX_PORT = 8086
    INFLUX_TOPIC = "node"

def mgtt_on_connect(client, userdata, flags, rc):
    log.info('Connected to MQTT broker with (code %s)', rc)

    for topic in MQTT_TOPICS:
        client.subscribe(topic)


def mgtt_on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode('utf-8'))
    except Exception as e:
        return

    topic = msg.topic.split('/')
    now = datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')

    if isinstance(payload, str):
        return

    elif isinstance(payload, dict):
        for k, v in payload.items():
            value = v[0] if isinstance(v, list) else v
            json_body = [{'measurement': '.'.join(topic[1:]) + '.' + k,
                          'time': now,
                          'tags': {},
                          'fields': {'value': value}}]
            userdata['influx'].write_points(json_body)

    else:
        json_body = [{'measurement': '.'.join(topic[1:]),
                      'time': now,
                      'tags': {},
                      'fields': {'value': payload}}]
        userdata['influx'].write_points(json_body)


def main():
    log.basicConfig(level=INFO, format=LOG_FORMAT)

    client = InfluxDBClient(INFLUX_HOSTNAME, INFLUX_PORT, 'root', 'root', INFLUX_TOPIC)

    mqttc = mqtt.Client(userdata={'influx': client})
    mqttc.on_connect = mgtt_on_connect
    mqttc.on_message = mgtt_on_message

    mqttc.connect(MQTT_HOSTNAME, MQTT_PORT, keepalive=10)
    mqttc.loop_forever()


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(0)
    except Exception as e:
        log.error(e)
        if os.getenv('DEBUG', False):
            raise e
        sys.exit(1)
