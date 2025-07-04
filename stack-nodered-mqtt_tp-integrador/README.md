# Stack NodeRed + MQTT + InfluxDB + Telegraf + Grafana

Este repositorio contiene lo minimo requerido para correr un stack con 

* [NodeRed](https://nodered.org/) Sistema de programacion orientada a flujos (open-source)
* [Mosquitto](https://mosquitto.org/) Servidor MQTT (open-source)
* [InfluxDB](https://www.influxdata.com/products/influxdb-overview/) Base de datos orientada a series temporales (open-source)
* [Telegraf](https://www.influxdata.com/time-series-platform/telegraf/) InfluxDB software agent (open-source)
* [Grafana](https://grafana.com/) Dashboard software (open-source)

## Run

Clonar repositorio

> git clone git@gitlab.com:dgraselli/stack-nodered-mqtt

Ejecutar el stack

> docker-compose -d up

Si todo esta OK, ya se puede ver:

* [NodeRed IDE](http://localhost:1880)
* [NodeRed UI](http://localhost:1880/ui)

## Stop

> docker-compose stop

## Remove

> docker-compose down

## First steps

### Add database

Para crear la base de datos "test" en influxdb, utilizar el sig. comando: 

> echo "create database test" | docker exec -i stack-nodered-mqtt-influxdb_v2_influxdb_1 influx

Para verificar que se creo correctamente, utilizar este: 

> echo "show databases"  | docker exec -i stack-nodered-mqtt-influxdb_v2_influxdb_1 influx