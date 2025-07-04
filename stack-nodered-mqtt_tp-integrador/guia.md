

## Basico

* Timestamp (explicar tipos de entrada, y recurrencia,  TOPIC )
* Debug     (payload o completo)

Agregar

* Delay 5s  (Ej. Hay que retrasar el flujo para dar tiempo a algun mecanismo)
* Trigger   (Envia un msg, espera y envia otro. Ej. Para activar por 5 segundos una salida)
  
Agregar

* function (msg.minuto = new Date(msg.payload).getMinutes())
* switch   (condicional)
* change   (seteo de constantes, cambio de valores)

Agregar function con muchos mensajes

var m1 = {topic:'Hora', payload: new Date(msg.payload).getHours()}; 
var m2 = {topic:'Debug', payload: "Se separo la fecha"}; 
return [m1, m2]

un altra

var m11 = {topic:'Hora', payload: new Date(msg.payload).getHours()}; 
var m12 = {topic:'Minutos', payload: new Date(msg.payload).getMinutes()}; 
var m13 = {topic:'Segundos', payload: new Date(msg.payload).getSeconds()}; 
var m2 = {topic:'Debug', payload: "Se separo la fecha"}; 
return [[m11,m12,m13], m2]



## API

Visitar
http://api.openweathermap.org/   ( API -> DOC )

Agregar 
q=La%20Plata,AR&lang=es&units=metric

http://api.openweathermap.org/data/2.5/weather?q=La%20Plata,AR&lang=es&units=metric&appid=defcd0af4a0be39c7a488df2051e5752

* Agregar JSON y mostrar Debug

* Copiar TEMP y usar "Change" + Debug

* Usar OUT


## MQTT

Usar un random con MQTT out con 2 topics

* Usar Links (para tomar de la salidad de la API)
* Usar MQTT para registar laas temperaturas de la API

En nuevo flow. Usar MQTT in para guardar en InfluxDB

* Usar mqtt in
* Usar "function" para poner 

msg.measurement = 'temperatura'
msg.payload = [
{
    lugar: 'La Plata'
},
{
    value: msg.payload
}]
return msg;

* Usar influx 
* ver con "docker exec -it stacknoderedmqttinfluxdb_influxdb_1 influx -database test"
* select * from temp;


## HTTP

* Usar "http-in" con un simple "http-out" para simplicidad

* Usar "http-request" con 

https://api.openweathermap.org/data/2.5/onecall?lat=-34.920345&lon=-57.969559&lang=es&units=metric&appid=[API_KEY]

* Usar json
* Usar Template por partes ( primero simple, luego agregar el for )

<h1>Temperatura Actual</h1>

Temperatura: <b>{{payload.current.temp}}</b> <br>
Sensacion Termica: <b>{{payload.current.feels_like}}</b> <br>

Luego agregar pronostico

<h1>Pronostico</h1>

{{#payload.daily}}
<li>{{weather.0.description}}</li>
{{/payload.daily}}


## Subflows

Agrupar algunos nodos en un subblow



## Dashboards

* Button y Gauge para ver temperatura con links
* Chart
* Usar template "La tempratura es de {{msg.payload}} grados centigrados"
* Usar salida Audio
  
* Usar sentiment en español
* Usar gauge -10 a 10
* Frases
* Me gusta el futbol, me encanta, es hermoso, fantástico.
* Es horrible, lo detesto, espantoso.

Ej. Ej. sensar twiter

## Extras para recomendar

https://flows.nodered.org/node/node-red-contrib-uibuilder

https://flows.nodered.org/node/node-red-node-pi-gpio



# Ej cooking book

Manejo de caldera y aire acondicionado
https://flows.nodered.org/flow/4271d6617c89544ad318e7ab17211ba0