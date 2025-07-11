// Import required libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "DHT.h"  // <-- Agregamos la librería del sensor

// Replace with your network credentials
const char* ssid = "xxxxx";
const char* password = "xxxxx";

// DHT Sensor settings
#define DHTPIN 25       // Pin conectado al DHT11
#define DHTTYPE DHT11   // Tipo de sensor

DHT dht(DHTPIN, DHTTYPE);  // Creamos objeto del sensor

// Set LED GPIO
const int ledPin = 2;
const int PIN_OUTPUT = 26;
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Replaces placeholder with LED state value
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    } else {
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}
 
void setup(){
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(PIN_OUTPUT, OUTPUT);
  
  dht.begin(); // <-- Inicializamos el sensor

  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  // Ruta principal
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Ruta para CSS
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Encender LED
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);  
    digitalWrite(PIN_OUTPUT, HIGH);  
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Apagar LED
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    digitalWrite(PIN_OUTPUT, LOW); 
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // 🔥 Nueva ruta para temperatura
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    float t = dht.readTemperature();
    if (isnan(t)) {
      request->send(200, "text/plain", "Error al leer temperatura");
    } else {
      request->send(200, "text/plain", String(t));
    }
  });

  // 💧 Nueva ruta para humedad
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    float h = dht.readHumidity();
    if (isnan(h)) {
      request->send(200, "text/plain", "Error al leer humedad");
    } else {
      request->send(200, "text/plain", String(h));
    }
  });

  server.begin();
}
 
void loop(){
  // Nada aquí, usamos el servidor asincrónico
  float t = dht.readTemperature();
  Serial.print("Temperatura: ");
  Serial.println(t);
  delay(1000);
}
