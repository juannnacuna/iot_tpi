#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>   // https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// #define DEBUG_TELEGRAM_RESPONSE_TIME // Comentar/descomentar para habilitar/deshabilitar el debug por serial de tiempo de respuesta con bot de Telegram
#define NODE_ID 37 // Numero de este nodo
#define PIN_LED 2
#define PIN_SENSOR_A_TRIG 18
#define PIN_SENSOR_A_ECHO 19
#define PIN_SENSOR_B_TRIG 34
#define PIN_SENSOR_B_ECHO 35
#define PIN_SENSOR_C_TRIG 32
#define PIN_SENSOR_C_ECHO 33
#define PIN_SENSOR_D_TRIG 26
#define PIN_SENSOR_D_ECHO 27
#define DISTANCIA_OCUPADO_MEDIA 15  // cm
#define DISTANCIA_OCUPADO_MAS_MENOS 15  // cm
#define SOUND_SPEED 0.034

// Credenciales WiFi
const char* ssid = "Losacu 2.4Ghz";
const char* password = "abu9275561";
WiFiClientSecure client;

// Claves Telegram
#define BOTtoken "7029463441:AAEg9I7_jsPnNKC4XCf769Z-vVodiSp10p0" // @catedra_iot_2025_acuna_integbot
#define CHAT_ID "8067384394"  // ID del chat
UniversalTelegramBot bot(BOTtoken, client);

// Datos del Broker (MQTT)
const char* MQTT_BROKER_ADRESS = "broker.mqtt.cool"; // Otra opcion "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_NAME = "CursadaIoT_Acuna_Integrador";
const char* MQTT_USER = "";
const char* MQTT_PASS = "";

// Variables de Publisher (MQTT)
WiFiClient client2;
PubSubClient mqttClient(client2);
char msg[50];
int value = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Constantes
const unsigned long BOT_REQUEST_DELAY = 1000; // Para mensajes recibidos de Telegram
const unsigned long INTERVALO_NOTIFICACION = 5000; // Para enviar notificaciones por Telegram
const unsigned long INTERVALO_TRY_CONNECT = 2500; // Para reconectar al broker MQTT
const unsigned long INTERVALO_LECTURA = 5000; // Para hacer lectura de datos de cada nodo
const unsigned long INTERVALO_PUBLISH_ESTADO = 20000; // Para publicar datos en el broker MQTT
const unsigned long POTENCIALMENTE_LIBRE_TIMEOUT = 30000; // ms, tiempo para considerar una unidad como libre después de ser potencialmente libre

// Variables
enum UnitState {
  LIBRE = 0,
  OCUPADO = 1,
  POTENCIALMENTE_LIBRE = 2
};
UnitState stateUnitA = POTENCIALMENTE_LIBRE;
UnitState stateUnitB = POTENCIALMENTE_LIBRE;
UnitState stateUnitC = POTENCIALMENTE_LIBRE;
UnitState stateUnitD = POTENCIALMENTE_LIBRE;
long distanceSensorA = -1;
long distanceSensorB = -1;
long distanceSensorC = -1;
long distanceSensorD = -1;
unsigned long unitALibreTimeout = 0;
unsigned long unitBLibreTimeout = 0;
unsigned long unitCLibreTimeout = 0;
unsigned long unitDLibreTimeout = 0;
unsigned long lastRead = 0;
unsigned long lastMsg = 0;
unsigned long lastTimeBotRan = 0;
unsigned long lastNotificationTime = 0;
unsigned long timerAux = 0;
unsigned long lastConnectTry = 0;

// Prototipos de funciones
void setupWifi();
void setupMqtt();
void spiffsInit();
void setupServer();
void mqttPublishData();
void mqttVerifyReconnectReceive();
void reconnectToBroker();
void callback(char* topic, byte* message, unsigned int length);
void telegramCheckNewMessages();
void telegramHandleNewMessages(int numNewMessages);
void readDistanceAndHandleUnits();
void readDistanceAndHandleUnit(const String& unitName, int trigPin, int echoPin, long& sensorDistance, UnitState& state, unsigned long& unitLibreTimeout);
long getUnitDistance(const String& unitName, int trigPin, int echoPin);
bool distanceMeansOcupado(long distance);
String stateToString(UnitState state);
String processor(const String& var);

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
  spiffsInit();
  setupServer();
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SENSOR_A_TRIG, OUTPUT);
  pinMode(PIN_SENSOR_A_ECHO, INPUT);
  pinMode(PIN_SENSOR_B_TRIG, OUTPUT);
  pinMode(PIN_SENSOR_B_ECHO, INPUT);
  pinMode(PIN_SENSOR_C_TRIG, OUTPUT);
  pinMode(PIN_SENSOR_C_ECHO, INPUT);
  pinMode(PIN_SENSOR_D_TRIG, OUTPUT);
  pinMode(PIN_SENSOR_D_ECHO, INPUT);
}

void loop() {
  mqttVerifyReconnectReceive();
  readDistanceAndHandleUnits();
  mqttPublishData();
  telegramCheckNewMessages();
  delay(100); // Delay para no saturar el loop
}

void mqttPublishData() {
  if ((mqttClient.connected()) && (millis() - lastMsg) >= INTERVALO_PUBLISH_ESTADO) {
    mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/A/state").c_str(), stateToString(stateUnitA).c_str());
    mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/B/state").c_str(), stateToString(stateUnitB).c_str());
    mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/C/state").c_str(), stateToString(stateUnitC).c_str());
    mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/D/state").c_str(), stateToString(stateUnitD).c_str());
    lastMsg = millis();
  }
}

void readDistanceAndHandleUnits() {
  if ((millis() - lastRead) >= INTERVALO_LECTURA) {
    readDistanceAndHandleUnit("A", PIN_SENSOR_A_TRIG, PIN_SENSOR_A_ECHO, distanceSensorA, stateUnitA, unitALibreTimeout);
    readDistanceAndHandleUnit("B", PIN_SENSOR_B_TRIG, PIN_SENSOR_B_ECHO, distanceSensorB, stateUnitB, unitBLibreTimeout);
    readDistanceAndHandleUnit("C", PIN_SENSOR_C_TRIG, PIN_SENSOR_C_ECHO, distanceSensorC, stateUnitC, unitCLibreTimeout);
    readDistanceAndHandleUnit("D", PIN_SENSOR_D_TRIG, PIN_SENSOR_D_ECHO, distanceSensorD, stateUnitD, unitDLibreTimeout);
    lastRead = millis();
  }
}

void readDistanceAndHandleUnit(const String& unitName, int trigPin, int echoPin, long& sensorDistance, UnitState& state, unsigned long& unitLibreTimeout) {
  sensorDistance = getUnitDistance(unitName, trigPin, echoPin);
  mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/" + unitName + "/distance").c_str(), String(sensorDistance).c_str());

  if (sensorDistance == -1) {
    Serial.printf("Error al leer datos de la unidad %s.\n", unitName.c_str());
    return;
  }

  if ((state == LIBRE) && (distanceMeansOcupado(sensorDistance))) {
    state = OCUPADO;
    Serial.printf("Unidad %s ocupada.\n", unitName.c_str());
    mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/" + unitName + "/state").c_str(), stateToString(state).c_str());
    return;
  }

  if ((state == OCUPADO) && (!distanceMeansOcupado(sensorDistance))) {
    state = POTENCIALMENTE_LIBRE;
    unitLibreTimeout = millis();
    Serial.printf("Unidad %s potencialmente libre.\n", unitName.c_str());
    mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/" + unitName + "/state").c_str(), stateToString(state).c_str());
    return;
  }

  if (state == POTENCIALMENTE_LIBRE) {
    if (distanceMeansOcupado(sensorDistance)) {
      state = OCUPADO;
      Serial.printf("Unidad %s sigue ocupada.\n", unitName.c_str());
      mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/" + unitName + "/state").c_str(), stateToString(state).c_str());
      return;
    }
    if ((millis() - unitLibreTimeout) >= POTENCIALMENTE_LIBRE_TIMEOUT) {
      state = LIBRE;
      Serial.printf("Unidad %s libre.\n", unitName.c_str());
      mqttClient.publish(("TPI_ACUNA_BNMM/" + String(NODE_ID) + "/" + unitName + "/state").c_str(), stateToString(state).c_str());
    }
  }
}

long getUnitDistance(const String& unitName, int trigPin, int echoPin) {
  long duration, distance;
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);

  if (duration <= 0) {
    return -1;
  }

  distance = (duration * SOUND_SPEED) / 2; // cm
  Serial.printf("Unidad %s Distance: %ld cm\n", unitName.c_str(), distance);
  return distance;
}

bool distanceMeansOcupado(long distance) {
  if (distance <= DISTANCIA_OCUPADO_MEDIA + DISTANCIA_OCUPADO_MAS_MENOS 
    && distance >= DISTANCIA_OCUPADO_MEDIA - DISTANCIA_OCUPADO_MAS_MENOS) {
    return true;
  }
  return false;
}

void setupMqtt() {
  mqttClient.setServer(MQTT_BROKER_ADRESS, MQTT_PORT);
  mqttClient.setCallback(callback);
}

void spiffsInit() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
}

String stateToString(UnitState state) {
  switch (state) {
    case LIBRE:
      return "Libre";
    case OCUPADO:
      return "Ocupado";
    case POTENCIALMENTE_LIBRE:
      return "Potencialmente libre";
    default:
      return "Estado desconocido";
  }
}

// Processor function for updating variables in HTML
String processor(const String& var){
  Serial.println(var);
  if (var == "STATE_UNIT_A") {
    return stateToString(stateUnitA);
  }
  if (var == "STATE_UNIT_B") {
    return stateToString(stateUnitB);
  }
  if (var == "STATE_UNIT_C") {
    return stateToString(stateUnitC);
  }
  if (var == "STATE_UNIT_D") {
    return stateToString(stateUnitD);
  }
  return String();
}

void setupServer() {
  // Ruta principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Ruta para CSS
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Cambiar estado de nodo A
  server.on("/ocuparUnitA", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitA = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/liberarUnitA", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitA == OCUPADO) {
      stateUnitA = POTENCIALMENTE_LIBRE;
      unitALibreTimeout = millis();
    } else if (stateUnitA == POTENCIALMENTE_LIBRE) {
      stateUnitA = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/stateUnitA", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", stateToString(stateUnitA));
  });
  server.on("/distanceSensorA", HTTP_GET, [](AsyncWebServerRequest *request){
    if (distanceSensorA == -1) {
      request->send(200, "text/plain", "ERROR");
    } else {
      request->send(200, "text/plain", String(distanceSensorA));
    }
  });

  // Cambiar estado de nodo B
  server.on("/ocuparUnitB", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitB = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/liberarUnitB", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitB == OCUPADO) {
      stateUnitB = POTENCIALMENTE_LIBRE;
      unitBLibreTimeout = millis();
    } else if (stateUnitB == POTENCIALMENTE_LIBRE) {
      stateUnitB = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/stateUnitB", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", stateToString(stateUnitB));
  });
  server.on("/distanceSensorB", HTTP_GET, [](AsyncWebServerRequest *request){
    if (distanceSensorB == -1) {
      request->send(200, "text/plain", "ERROR");
    } else {
      request->send(200, "text/plain", String(distanceSensorB));
    }
  });

  // Cambiar estado de nodo C
  server.on("/ocuparUnitC", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitC = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/liberarUnitC", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitC == OCUPADO) {
      stateUnitC = POTENCIALMENTE_LIBRE;
      unitCLibreTimeout = millis();
    } else if (stateUnitC == POTENCIALMENTE_LIBRE) {
      stateUnitC = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/stateUnitC", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", stateToString(stateUnitC));
  });
  server.on("/distanceSensorC", HTTP_GET, [](AsyncWebServerRequest *request){
    if (distanceSensorC == -1) {
      request->send(200, "text/plain", "ERROR");
    } else {
      request->send(200, "text/plain", String(distanceSensorC));
    }
  });

  // Cambiar estado de nodo D
  server.on("/ocuparUnitD", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitD = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/liberarUnitD", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitD == OCUPADO) {
      stateUnitD = POTENCIALMENTE_LIBRE;
      unitDLibreTimeout = millis();
    } else if (stateUnitD == POTENCIALMENTE_LIBRE) {
      stateUnitD = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/stateUnitD", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", stateToString(stateUnitD));
  });
  server.on("/distanceSensorD", HTTP_GET, [](AsyncWebServerRequest *request){
    if (distanceSensorD == -1) {
      request->send(200, "text/plain", "ERROR");
    } else {
      request->send(200, "text/plain", String(distanceSensorD));
    }
  });

  server.begin();
  Serial.println("HTTP server started");
}

void telegramCheckNewMessages() {
  if ((millis() - lastTimeBotRan) >= BOT_REQUEST_DELAY)  {
    #ifdef DEBUG_TELEGRAM_RESPONSE_TIME
      Serial.printf("Checkeando si hay mensajes... ");
      timerAux = millis();
    #endif
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    #ifdef DEBUG_TELEGRAM_RESPONSE_TIME
      Serial.printf("Hay %d mensajes nuevos. (%d ms)\n", numNewMessages, millis()-timerAux);
    #endif
    while(numNewMessages) {
      Serial.println("Mensaje recibido");
      telegramHandleNewMessages(numNewMessages);
      #ifdef DEBUG_TELEGRAM_RESPONSE_TIME
        Serial.printf("Checkeando si hay mensajes... ");
        timerAux = millis();
      #endif
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      #ifdef DEBUG_TELEGRAM_RESPONSE_TIME
        Serial.printf("Hay %d mensajes nuevos. (%d ms)\n", numNewMessages, millis()-timerAux);
      #endif
    }
    lastTimeBotRan = millis();
  }
}

void telegramHandleNewMessages(int numNewMessages) {
  Serial.printf("Manejando mensaje nuevo.\n");
  for (int i=0; i<numNewMessages; i++) {
    // Checkeo si el mensaje viene de alguien autorizado
    String chat_id = String(bot.messages[i].chat_id); // Get chat id of the requester
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Usuario no autorizado.", "");
      continue;
    }

    // Print the received message on serial
    String text = bot.messages[i].text;
    Serial.printf("Mensaje %d : %s\n", i, text.c_str());
    
    // Menu de comandos
    if (text == "/start") {
      String welcome = "*¡Bienvenido al bot de la Biblioteca Nacional!* 😄\n";
      welcome += "Acá vas a poder consultar el estado de las unidades de estudio.\n";
      welcome += "Para comenzar, utiliza el comando */estado*.\n";
      welcome += "Si necesitas ayuda, utiliza el comando */ayuda*.\n";
      welcome += "Si querés saber más sobre el proyecto, utiliza el comando */info*.\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    } else if (text == "/estado") {
      String estado = "*Estado de las unidades:*\n";
      estado += "Unidad " + String(NODE_ID) + "A: " + stateToString(stateUnitA) + "\n";
      estado += "Unidad " + String(NODE_ID) + "B: " + stateToString(stateUnitB) + "\n";
      estado += "Unidad " + String(NODE_ID) + "C: " + stateToString(stateUnitC) + "\n";
      estado += "Unidad " + String(NODE_ID) + "D: " + stateToString(stateUnitD) + "\n";
      bot.sendMessage(chat_id, estado, "Markdown");
    } else {
      bot.sendMessage(chat_id, "Comando no reconocido.", "");
    }
  }
}

void setupWifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.printf("Connecting to %s", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  IPAddress ip = WiFi.localIP();
  Serial.printf("Local IP Address: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
}

void mqttVerifyReconnectReceive() {
  if (!mqttClient.connected() && (millis() - lastConnectTry) >= INTERVALO_TRY_CONNECT) {
    reconnectToBroker();
  }
  mqttClient.loop();
}

void reconnectToBroker() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("MQTT connection...");
    
    if (mqttClient.connect(MQTT_CLIENT_NAME)) { // OJO credenciales MOSQUITTO MQTT_USER, MQTT_PASS
      Serial.println("connected");
      // Subscribe to topics
      mqttClient.subscribe(("TPI_ACUNA_BNMM/" + String(NODE_ID)).c_str()); // Suscribirse al topic del nodo, pero no a todos los subtopicos
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  // Print the received message on serial
  Serial.printf("Message arrived on topic: %s. Message: ", topic);
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Lógica de algo
  if (String(topic) == "TPI_ACUNA_BNMM/" + String(NODE_ID)) {
  }
}