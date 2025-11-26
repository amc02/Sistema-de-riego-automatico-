#include <ArduinoJson.h>
#include <Arduino_JSON.h>
#include "thingProperties.h"
#include <WiFi.h>
#include <HTTPClient.h>

// -------------------- CONFIGURACIÓN --------------------

String apiKey = "d4dfdaefcb29b23415df7362e0ac2b9a";  // TU API KEY
String ciudad_clima = "Ecatepec";                     // Tu ciudad
String urlClima;

// Pines
const int pinSensor = 34;  // Sensor capacitivo humedad
const int pinRele   = 14;  // Relay activar bomba

// Umbral de humedad
int humedadMin = 68;       // Ajusta según tu sensor

// Control de tiempo
unsigned long ultimoClima = 0;
unsigned long intervaloClima = 30000; // 30s

// -------------------- SETUP --------------------

void setup() {
  Serial.begin(9600);
  delay(1500);

  pinMode(pinRele, OUTPUT);
  digitalWrite(pinRele, HIGH); // Relay apagado

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);

  // Construcción de URL
  urlClima = "https://api.openweathermap.org/data/2.5/weather?q=" 
             + ciudad_clima +
             "&appid=" + apiKey +
             "&units=metric&lang=es";

  Serial.println("Sistema de riego inteligente iniciado.");
}

// -------------------- LOOP --------------------

void loop() {
  ArduinoCloud.update();

  leerHumedadSuelo();

  // Llamar API cada 30 segundos
  if (millis() - ultimoClima > intervaloClima) {
    obtenerClima();
    ultimoClima = millis();
  }

  logicaRiego();
}

// --------------------------------------------------------
//          LECTURA DEL SENSOR CAPACITIVO
// --------------------------------------------------------

void leerHumedadSuelo() {
  int raw = analogRead(pinSensor);

  // Sensor capacitivo: valores se invierten
  humedadSuelo = map(raw, 4095, 1600, 0, 100);

  humedadSuelo = constrain(humedadSuelo, 0, 100);

  Serial.print("Humedad del suelo (%): ");
  Serial.println(humedadSuelo);
}

// --------------------------------------------------------
//                OBTENER CLIMA (GRATIS)
// --------------------------------------------------------

void obtenerClima() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(urlClima);

    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      JSONVar data = JSON.parse(payload);

      // Usamos porcentaje de nubosidad
      prob_lluvia_api = double(data["clouds"]["all"]);

      Serial.println("=== Datos API recibidos ===");
      Serial.print("Nubosidad (%): ");
      Serial.println(prob_lluvia_api);

    } else {
      Serial.print("Error API: ");
      Serial.println(httpCode);
    }

    http.end();
  }
}

// --------------------------------------------------------
//                   LÓGICA DE RIEGO
// --------------------------------------------------------

void logicaRiego() {
  bool sueloSeco = humedadSuelo < humedadMin;
  bool noVaLlover = prob_lluvia_api < 50;  // Basado en nubosidad

  if (sueloSeco && noVaLlover) {
    activarRiego();
  } else {
    detenerRiego();
  }
}

// --------------------------------------------------------
//                FUNCIONES DE RIEGO
// --------------------------------------------------------

void activarRiego() {
  digitalWrite(pinRele, LOW);  // Activa bomba
  regando = true;

  Serial.println(">>> RIEGO ACTIVADO <<<");
}

void detenerRiego() {
  digitalWrite(pinRele, HIGH); // Apaga bomba
  regando = false;

  Serial.println(">>> RIEGO APAGADO <<<");
}

// --------------------------------------------------------
//      REQUERIDO PARA ARDUINO CLOUD (Dashboard manual)
// --------------------------------------------------------

void onRegandoChange() {
  if (regando) {
    digitalWrite(pinRele, LOW);  
  } else {
    digitalWrite(pinRele, HIGH);
  }
}
