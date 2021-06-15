#include <WiFi.h>           // Invoca libreria para utilizar Wifi
#include <PubSubClient.h>   // Invoca libreria para publicar y subscribirse con el protocolo MQTT
#include <Wire.h>           // Invoca libreria para usar protocolo i2c
#include "DHT.h"            // Invoca libreria para usar sensor dht
#include <BH1750.h>         // Invoca libreria para usar sensor de temperatura

#define DHTPIN 18           // Pin donde está conectado el sensor dht
#define DHTTYPE DHT22       // Tipo del sensor dht, en este caso dht22
#define CALOR 25            // Variable de umbral para temperatura
#define intensidad 30       // Variable de umbral para luz
#define nivelAgua 3000      // Variable de umbral para nivel de agua

BH1750 sensorLuz;           // Se crea objeto sensorLuz con las propiedades de su libreria
DHT dht(DHTPIN, DHTTYPE);   // Se crea objeto dht con las propiedades de su libreria

// Variables de ssid y contraseña de conexion Wifi
const char* ssid = "nombre de red;
const char* password = "contraseña wifi";

// Server MQTT a usar, en este caso Eclipse Mosquitto
const char* mqtt_server = "test.mosquitto.org";

WiFiClient espClient;           // Se crea un objeto WiFiClient

PubSubClient client(espClient); // Se crea un objeto client con propiedades de la libreria para usar MQTT

// Variables para los mensajes que llegaran
long lastMsg = 0;
char msg[50];
int value = 0;

// Variables para guardar temperatura y humedad
float temperature = 0;
float humidity = 0;

// Variables para asignar pines
int ventilador = 15;
int luces = 4;
int bomba = 5;
const int pinSensor = 34;

int sensorNivel;              // En esta variable se guardaran los datos del sensor de nivel

void setup() {
  Serial.begin(9600);         // Inicialización de comunicacion serial
  Wire.begin();               // Abre comunicacion i2c
  dht.begin();                // Inicialización de sensor dht
  sensorLuz.begin();          // Inicialización de sensor BH1750

  Serial.println("Iniciando...");   // Imprime en monitor serial

  setup_wifi();                     // Llamado de la funcion de seteo Wifi

  client.setServer(mqtt_server, 1883);  // Server mqtt y puerto al cual se conecta
  client.setCallback(callback);         // Funcion que recibe los mensajes

// Se definen los pines como salida
  pinMode(ventilador, OUTPUT);
  pinMode(luces, OUTPUT);
  pinMode(bomba, OUTPUT);
}

// Configuracion de conexíon Wifi
void setup_wifi() {
  delay(10);

// Imprime datos de conexíon
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

// Ingreso de valores de la conexion, ssid y pass.
  WiFi.begin(ssid, password);

// Comprobacion de conexion cada medio segundo
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

}

// Funcion encargada de recibir mensajes desde algun dispositivo en un cierto topico
void callback(char* topic, byte* message, unsigned int length) {

  String messageTemp;       // esto imprime elmensaje que llego

// Array para guardar mensaje
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println();

// Evalua que llego al topico y muestra un mensaje dependiendo si llego un on u off
  if (String(topic) == "esp32/hidroponico/cristian/mensaje") {
    Serial.print(messageTemp);
    if (messageTemp == "on") {
      Serial.println("on");
    }
    else if (messageTemp == "off") {
      Serial.println("off");
    }
  }
}

// Funcion es para reconectar la conexion en caso de caida, ademas de subscribirse al topico para recibir un mensaje
void reconnect() {
// Imprime un mensaje al conectarse
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

// If si se conecta o no
    if (client.connect("ESP32Client")) {
      Serial.println("connected");

// Se susbscribe altopico para recibir mensaje
      client.subscribe("esp32/hidroponico/cristian/mensaje");
    } else {
// Imprime falla de conexion si no se conecta al servidor MQTT
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }
}

// Comienza el loop principal
void loop() {
// Reconecta en caso de caida
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

// Actualiza que llego un mensaje
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

// Se realizan las lecturas: Humedad, Temperatura, Nivel de agua y nivel de luz.
    float humedad = dht.readHumidity();
    float temperatura = dht.readTemperature();
    sensorNivel = analogRead(pinSensor);
    unsigned int lux = sensorLuz.readLightLevel();

// Se convierte en array la lectura del sensor agua
    char aguaString[8];
    dtostrf(sensorNivel, 1, 2, aguaString);
    client.publish("esp32/hidroponico/cristian/agua", aguaString);                // Se publica por MQTT en su topico el dato de nivel de agua

// Se convierte en array la lectura del sensor de humedad
    char humedadString[8];
    dtostrf(humedad, 1, 2, humedadString);
    client.publish("esp32/hidroponico/cristian/humedad", humedadString);          // Se publica por MQTT en su topico el dato de nivel de humedad

// Se convierte en array la lectura del sensor de temperatura
    char temperaturaString[8];
    dtostrf(temperatura, 1, 2, temperaturaString);
    client.publish("esp32/hidroponico/cristian/temperatura", temperaturaString);  // Se publica por MQTT en su topico el dato de nivel de temperatura

// Se convierte en array la lectura del sensor de luz
    char luxString[8];
    dtostrf(lux, 1, 2, luxString);
    client.publish("esp32/hidroponico/cristian/lux", luxString);                  // Se publica por MQTT en su topico el dato de nivel de luz

// Se imprimen en pantalla todas las lecturas
    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.print("%-");

    Serial.print("Temperatura: ");
    Serial.print(temperatura); 
    Serial.print(" °C");

    Serial.print("-Nivel de luz: "); 
    Serial.print(lux);
    Serial.print(" lx ");

    Serial.print("Nivel de agua en tubos: ");
    Serial.println(sensorNivel);// 0  AL 1023

// Finaliza la imprension por puerto serial comienzan las condicionales que activan los reles

// Condicional temperatura

    if (temperatura > CALOR) {            // Si lectura de sensor es mayor a calor
      digitalWrite(ventilador, LOW);      // Enciende relé (HIGH = 1 = 5Voltios LOW = 0 = 0Voltios)
    }
    else {                                // Si no
      digitalWrite(ventilador, HIGH);     // Apaga relé
    }

// Condicional intensidad de luz

    if (lux < intensidad) {               // Si la lectura de sensor es menor a intensidad
      digitalWrite(luces, LOW);           // Enciende relé
    }
    else if (lux > intensidad + 20) {     // Si lectura de sensor es mayor a intensidad + 20 (por si el cambio de luz es demasiado lento)
      digitalWrite(luces, HIGH);          // Apaga relé
    }

// Condicional de nivel de agua
    if (sensorNivel < nivelAgua) {         //Si la lectura de sensor es menor a nivelAgua
      digitalWrite(bomba, LOW);            //Enciende bomba
    }
    else if (sensorNivel > nivelAgua) {    //Si lectura del sensor es mayor que nivelAgua
      digitalWrite(bomba, HIGH);           //Apaga bomba
    }
  }
}
