# BigClown Repository bcp-weather-station

## Popis

Meteostanice ukazuje na chytrém LED pásku teplotu (jako stupnice). Všechny naměřené veličiny posílá po USB do Raspberry Pi, kde se dále data ukládají do InfluxDB a vizualizace je pomocí Grafana.

## Insatace

### InfluxDB

```
curl -sL https://repos.influxdata.com/influxdb.key | sudo apt-key add -
echo "deb https://repos.influxdata.com/debian jessie stable" | sudo tee /etc/apt/sources.list.d/influxdb.list
sudo apt-get update && sudo apt-get install influxdb
sudo systemctl daemon-reload
sudo systemctl enable influxdb
sudo systemctl start influxdb
```

Pokud chcete administrovat influx přes http tak v /etc/influxdb/influxdb.conf
odkomentujdete a upravte enabled = true a bind-address = ":8083"

### Grafana

* rpi

    ```
	wget "https://github.com/fg2it/grafana-on-raspberry/releases/download/v4.1.2/grafana_4.1.2-1487023783_armhf.deb"
	sudo dpkg -i grafana_4.1.2-1487023783_armhf.deb
    sudo apt-get install -f
    ```
* x86-64

    ````
	sudo apt-get install -y apt-transport-https
	curl -sL https://packagecloud.io/gpg.key | sudo apt-key add -
	echo "deb https://packagecloud.io/grafana/stable/debian/ jessie main" | sudo tee /etc/apt/sources.list.d/grafana.list
	sudo apt-get update && sudo apt-get install grafana
    ````

Společné

```
sudo systemctl daemon-reload
sudo systemctl enable grafana-server
sudo systemctl start grafana-server
```

### Další závislosti

```
sudo pip3 install influxdb
```


## Konfigurace


### Vytvoreni databaze node v InfluxDB
```
curl "http://localhost:8086/query?q=CREATE+DATABASE+%22node%22&db=_internal"
```
kontrola zda došlo k vytvoření
```
curl "http://localhost:8086/query?q=SHOW+DATABASES&db=_internal"
```

### Spuštění služby která bude překopírovavat data z mqtt do InfluxDB

```
sudo mv mqtt_to_influxdb.py /usr/bin/mqtt_to_influxdb
sudo chmod +x /usr/bin/mqtt_to_influxdb
sudo mv mqtt_to_influxdb.service /etc/systemd/system

sudo systemctl daemon-reload
sudo systemctl enable mqtt_to_influxdb.service
sudo systemctl start mqtt_to_influxdb.service
```


### Nastavení Grafany

#### Vytvoření datasource

#### Import dashboardu
