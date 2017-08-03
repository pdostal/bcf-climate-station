FROM python:3

WORKDIR /app

RUN pip install influxdb paho-mqtt

ADD mqtt_to_influxdb.py .
ADD mqtt_to_influxdb_conf.py .

CMD [ "python", "./mqtt_to_influxdb.py" ]

