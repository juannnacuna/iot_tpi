# Gestión Inteligente de Unidades de Estudio: Biblioteca Nacional Mariano Moreno

Este proyecto fue desarrollado como Trabajo Final para la materia Internet de las Cosas (2025), Facultad de Informática, Universidad Nacional de La Plata (UNLP)

## Notas

- Bot de Telegram corriendo por defecto en @catedra_iot_2025_acuna_integbot.
- Recordar configurar las credenciales del WiFi en el código del ESP32.
- Prestar atención a la IP asignada al nodo para acceder a la interfaz web (se imprime por serial al iniciarse).
- No pude persistir la base de datos. Es necesario crear una llamada 'mydb'.
- En relación al punto anterior, si bien en Grafana si se persistió un Dashboard básico, puede que sea necesario configurar la conexión a la base de datos.

## Informe

### Problema

En entornos como bibliotecas universitarias o pisos de oficina existe una problemática común: la ineficiente gestión de sus espacios. En algunos casos los usuarios pierden tiempo buscando una unidad disponible. Otras veces no se cuenta con datos precisos sobre el uso real de las unidades, incluso aunque exista un sistema de reserva de unidades o salas privadas.

A modo de ejemplo, se tomará como caso de estudio a la Biblioteca Nacional Mariano Moreno (BNMM), teniendo en cuenta su inmensa cantidad de unidades de estudio.

### Objetivo

Mediante el uso de IoT, el objetivo de este proyecto es resolver estos problemas con una solución que mejore la experiencia del usuario, proporcione una manera de recolectar información en tiempo real (fundamental para optimizar el uso de los espacios y la toma de decisiones administrativas), y siente las bases de un sistema escalable.

### Solución

La solución implementada consiste en un sistema de nodos inteligentes ESP32 que serán distribuidos a lo largo de las mesas de trabajo que están repartidas en las distintas salas y salones de lectura de la biblioteca.

Se entiende por unidad de estudio a un conjunto asiento-escritorio: un lugar fijo que es usado por los asistentes a la biblioteca y que es ocupado y liberado constantemente a lo largo del día.

A cada nodo se le asignan unidades, para las cuales:
- Hace una lectura de distancia al suelo, utilizando un sensor de distancia (Sensor Ultrasónico HC-SR04) que se ubica en la parte inferior de la mesa.
- Maneja su estado (Libre, Potencialmente libre u Ocupado)

El propósito del estado Potencialmente libre es el de evitar falsos positivos, contemplando el uso real de las unidades ya que un usuario usualmente se levanta de su asiento brevemente, por ejemplo para buscar un libro o ir al sanitario.

El cambio de estados es el siguiente: del estado Ocupado se pasa al Potencialmente libre cuando se lee una distancia larga, lo que quiere decir que ya no hay nadie sentado y la lectura del sensor se interpreta como la distancia de la mesa al suelo. En ese momento, se setea un timer que en caso de expirar la unidad pasa al estado Libre. Finalmente, tanto del estado Libre como del Potencialmente libre, en caso de leerse desde el sensor una distancia corta (interpretando que hay alguien sentado) la unidad pasa a estar ocupada.

El nodo se encarga de enviar el estado de sus unidades y las distancias leídas hacia un servidor central a través del protocolo MQTT. Para ello, el nodo se conecta a la red WiFi local.
Adicionalmente, el nodo hostea una interfaz web que permite interactuar con él localmente. Es posible visualizar las lecturas de los sensores así como alterar el estado de cada unidad asignada.

La arquitectura de la solución es la siguiente:

Como se explicó anteriormente, cada nodo envía información al servidor central a través del protocolo MQTT. El servidor central, mediante el uso del broker Mosquitto, recibe los mensajes. Esa información es manejada por una instancia de Node-RED, que hace de “cerebro”: interpreta la información, la guarda en variables, hace distintos cálculos y se encarga de guardarla en la base de datos InfluxDB. Esta información se puede visualizar fácilmente desde Grafana. Por último, Node-RED también se encarga de controlar un bot de Telegram (@catedra_iot_2025_acuna_integbot), encargándose de responder los comandos.

### Conclusión

Se ha implementado un sistema que cumple con los objetivos propuestos, y que efectivamente sienta las bases de un sistema escalable. Algunos de los puntos a extender pueden ser:

- Agregar funcionalidades al nodo, especialmente si se utilizará el nodo en una sala cerrada como un box investigador o una sala de conferencias pequeña. Se podrían controlar las luces, dispositivos de refrigeración/calefacción, etc.
- Complejizar la lógica de estados con la ayuda de nuevos sensores (pulsador, otro sensor de distancia, cámara de video) para mejorar la ocupación de unidades, aunque debería evaluarse si priorizar datos certeros a costa de automatización y facilidad de uso para el usuario, por ejemplo.Extender la solución a todas las salas de la biblioteca.
- Llevar la solución a otros casos de estudio (otra biblioteca, piso de oficina, aula) adaptándola según sea necesario.
- Nuevos comandos para Telegram que involucren control de nodos, haciendo que el nodo reaccione a mensajes recibidos, y con todo el sistema de autenticación que eso conllevaría: registrar administradores.
- Dashboards de Grafana más robustos. Algunas visualizaciones nuevas podrían ser: horas pico de uso, salas o unidades más usadas, alertas de sala llena o vacía.
