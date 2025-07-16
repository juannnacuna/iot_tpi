# Stack NodeRed + MQTT + InfluxDB + Telegraf + Grafana

Este repositorio contiene lo minimo requerido para correr un stack con 

* [NodeRed](https://nodered.org/) Sistema de programacion orientada a flujos (open-source)
* [Mosquitto](https://mosquitto.org/) Servidor MQTT (open-source)
* [InfluxDB](https://www.influxdata.com/products/influxdb-overview/) Base de datos orientada a series temporales (open-source)
* [Telegraf](https://www.influxdata.com/time-series-platform/telegraf/) InfluxDB software agent (open-source)
* [Grafana](https://grafana.com/) Dashboard software (open-source)

Clonado originalmente de @dgraselli

> git clone git@gitlab.com:dgraselli/stack-nodered-mqtt

## Run

Ejecutar el stack

> docker-compose -d up

Si todo esta OK, ya se puede ver:

* [NodeRed IDE](http://localhost:1880)
* [Grafana](http://localhost:3000)

## Stop

> docker-compose stop

## Remove

> docker-compose down
