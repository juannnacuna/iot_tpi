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

#define PIN_LED 2
#define PIN_NODE_A 
#define PIN_NODE_B 
#define PIN_NODE_C 
#define PIN_NODE_D 
#define DISTANCIA_OCUPADO_MEDIA 15  // cm
#define DISTANCIA_OCUPADO_MAS_MENOS 5  // cm

// Credenciales WiFi
const char* ssid = "Losacu 2.4Ghz";
const char* password = "abu9275561";
WiFiClientSecure client;

// Claves Telegram
#define BOTtoken "7029463441:AAEg9I7_jsPnNKC4XCf769Z-vVodiSp10p0" // @catedra_iot_2025_acuna_integbot
#define CHAT_ID "4779982027"  // ID del grupo
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
const unsigned long INTERVALO_PRINCIPAL = 5000; // Para simulación, para publicación de datos

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
void getData();

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
  spiffsInit();
  setupServer();
  pinMode(PIN_LED, OUTPUT);
}

void loop() {
  mqttVerifyReconnectReceive();
  getData();
  mqttPublishData();
  telegramCheckNewMessages();
  delay(100); // Delay para no saturar el loop
}

void mqttPublishData() {
  if ((millis() - lastMsg) >= INTERVALO_PRINCIPAL) {
    if (mqttClient.connected()) {
        lastMsg = millis();
        mqttClient.publish("TPI_ACUNA_BNA/37", msg);
    }
  }
}

// Acá iría la lógica para obtener los datos de los sensores o del estado de las unidades
void getData() {
  Serial.println("Obteniendo datos...");
  Serial.println("Estado de las unidades:");
  Serial.println("Unidad A: " + stateToString(stateUnitA));
  Serial.println("Unidad B: " + stateToString(stateUnitB));
  Serial.println("Unidad C: " + stateToString(stateUnitC));
  Serial.println("Unidad D: " + stateToString(stateUnitD));
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
  if (var == "STATE_NODE_A") {
    return stateToString(stateUnitA);
  }
  if (var == "STATE_NODE_B") {
    return stateToString(stateUnitB);
  }
  if (var == "STATE_NODE_C") {
    return stateToString(stateUnitC);
  }
  if (var == "STATE_NODE_D") {
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
  server.on("/nodeA/ocupar", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitA = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/nodeA/liberar", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitA == OCUPADO) {
      stateUnitA = POTENCIALMENTE_LIBRE;
    } else if (stateUnitA == POTENCIALMENTE_LIBRE) {
      stateUnitA = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Cambiar estado de nodo B
  server.on("/nodeB/ocupar", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitB = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/nodeB/liberar", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitB == OCUPADO) {
      stateUnitB = POTENCIALMENTE_LIBRE;
    } else if (stateUnitB == POTENCIALMENTE_LIBRE) {
      stateUnitB = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Cambiar estado de nodo C
  server.on("/nodeC/ocupar", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitC = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/nodeC/liberar", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitC == OCUPADO) {
      stateUnitC = POTENCIALMENTE_LIBRE;
    } else if (stateUnitC == POTENCIALMENTE_LIBRE) {
      stateUnitC = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Cambiar estado de nodo D
  server.on("/nodeD/ocupar", HTTP_GET, [](AsyncWebServerRequest *request){
    stateUnitD = OCUPADO;
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/nodeD/liberar", HTTP_GET, [](AsyncWebServerRequest *request){
    if (stateUnitD == OCUPADO) {
      stateUnitD = POTENCIALMENTE_LIBRE;
    } else if (stateUnitD == POTENCIALMENTE_LIBRE) {
      stateUnitD = LIBRE;
    }
    request->send(SPIFFS, "/index.html", String(), false, processor);
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
      estado += "Unidad A: " + stateToString(stateUnitA) + "\n";
      estado += "Unidad B: " + stateToString(stateUnitB) + "\n";
      estado += "Unidad C: " + stateToString(stateUnitC) + "\n";
      estado += "Unidad D: " + stateToString(stateUnitD) + "\n";
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
      mqttClient.subscribe("TPI_ACUNA_BNA/37");
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
  if (String(topic) == "TPI_ACUNA_BNA/37") {
  }
}