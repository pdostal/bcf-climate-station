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


def mgtt_on_connect(client, userdata, flags, rc):
    log.info('Connected to MQTT broker with (code %s)', rc)

    for topic in ('nodes/#', 'node/#'):
        client.subscribe(topic)


def mgtt_on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode('utf-8'))
    except Exception as e:
        return

    if not payload:
        return

    if not isinstance(payload, dict):
        return

    topic = msg.topic.split('/')

    now = datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')

    for k, v in payload.items():
        json_body = [{'measurement': '-'.join(topic[1:]) + '.' + k,
                      'time': now,
                      'tags': {},
                      'fields': {'value': v[0] if isinstance(v, list) else v}}]

        userdata['influx'].write_points(json_body)


def main():
    log.basicConfig(level=INFO, format=LOG_FORMAT)

    client = InfluxDBClient('localhost', 8086, 'root', 'root', 'node')

    mqttc = mqtt.Client(userdata={'influx': client})
    mqttc.on_connect = mgtt_on_connect
    mqttc.on_message = mgtt_on_message

    mqttc.connect('localhost', 1883, keepalive=10)
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
