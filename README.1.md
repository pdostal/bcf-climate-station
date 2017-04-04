# BigClown Repository bcp-weather-station

## Popis

Meteostanice ukazuje na chytrém LED pásku teplotu (jako stupnice). Všechny naměřené veličiny posílá po USB do Raspberry Pi, kde se dále data ukládají do InfluxDB a vizualizace je pomocí Grafana.

<a href="images/weather-station-with-grafana.jpg" target="_blank">
    <img src="images/weather-station-with-grafana.jpg" alt="weather-station-with-grafana" width="800px"/>
</a>

## Firmware

Je dobré mít vždy vše aktuální, takže si zaktualizujeme firmware, můžeš k tomu použít vlastní počítač pak postupuj dle návodu [zde](https://doc.bigclown.cz/core-module-flashing.html), nebo k tomu využít Raspberry jako já.

Přečti si co je dfu mód [zde](https://doc.bigclown.cz/core-module-flashing.html#nahrávání-programu-přes-usb-dfu-bootloader)

* Stáhni si aktuální firmware podle počtu diod přímo z repozitáře [https://github.com/bigclownlabs/bcp-climate-station/releases/latest]([https://github.com/bigclownlabs/bcp-climate-station/releases/latest])
a pro nahrání použi tento příkaz
    ```
    sudo dfu-util -s 0x08000000:leave -d 0483:df11 -a 0 -D firmware-72pixel.bin
    ```

* Nebo si jej zkompiluj
  ```
  git clone --recursive https://github.com/bigclownlabs/bcp-climate-station.git
  cd bcp-climate-station
  make release PIXEL_COUNT=72
  make dfu
  ```

## Instalace


### Aktualizace

```
sudo apt-get update && sudo apt-get upgrade -y
```

### Instalace BigClown komponent existující systém

V případě že používáš raspbian od BigClown tento bod přeskoč.

Co který řádek provádí je popsáno [zde](https://doc.bigclown.cz/raspberry-pi-installation.html#instalace-bigclown-balíčků-na-existující-systém)

```
sudo apt-get install apt-transport-https wget
sudo sh -c 'echo "deb https://repo.bigclown.com/debian jessie main" >/etc/apt/sources.list.d/bigclown.list'
wget https://repo.bigclown.com/debian/pubkey.gpg -O - | sudo apt-key add -
sudo apt-get update && sudo apt-get upgrade -y
sudo apt-get install bc-workroom-gateway
sudo apt-get install mosquitto-clients
```

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

```
sudo apt-get install adduser libfontconfig -y
```

* rpi

    ```
	wget "https://github.com/fg2it/grafana-on-raspberry/releases/download/v4.1.2/grafana_4.1.2-1487023783_armhf.deb"
	sudo dpkg -i grafana_4.1.2-1487023783_armhf.deb
    ```
* x86-64

    ````
	sudo apt-get install -y apt-transport-https
	curl -sL https://packagecloud.io/gpg.key | sudo apt-key add -
	echo "deb https://packagecloud.io/grafana/stable/debian/ jessie main" | sudo tee /etc/apt/sources.list.d/grafana.list
	sudo apt-get update && sudo apt-get install grafana
    ````

```
sudo systemctl daemon-reload
sudo systemctl enable grafana-server
sudo systemctl start grafana-server
```

### Další závislosti

```
sudo apt-get install python3-pip
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
curl "https://raw.githubusercontent.com/bigclownlabs/bcp-weather-station/master/mqtt_to_influxdb.py" > mqtt_to_influxdb
sudo mv mqtt_to_influxdb /usr/bin/mqtt_to_influxdb
sudo chmod +x /usr/bin/mqtt_to_influxdb
curl "https://raw.githubusercontent.com/bigclownlabs/bcp-weather-station/master/mqtt_to_influxdb.service" > mqtt_to_influxdb.service
sudo mv mqtt_to_influxdb.service /etc/systemd/system

sudo systemctl daemon-reload
sudo systemctl enable mqtt_to_influxdb.service
sudo systemctl start mqtt_to_influxdb.service
```

### Nastavení Grafany

Pripoj se na grafanu http://ip-raspberry:3000
Login admin a heslo admin




#### Vytvoření datasource

#### Import dashboardu
