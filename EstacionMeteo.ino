#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h>
#include <EEPROM.h>

// =====================================================================
// PROTOTIPOS DE FUNCIONES QUE ESTÁN EN OTROS .ino
// (el IDE solo prototipa automáticamente las del .ino principal)
// =====================================================================
void setupDHT22();
void setupPluviometro();
void setupBateria();
void setupRed();
void enviarDatosClima();
void enviarSoloLluvia(uint16_t lluviaCont);
bool consultarYLimpiarNuevaLluvia();
uint16_t obtenerContadorLluvia();
void reiniciarArduino();
float leerTemperatura();
float leerHumedad();
float leerVoltajeBateria();

// =====================================================================
// CONFIGURACIÓN DE RED
// =====================================================================
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress serverIP(10, 0, 10, 154);   // IP de la PC donde corre Flask (API)
int serverPort = 5000;

IPAddress ip(10, 0, 10, 187);         // IP fija del Arduino
IPAddress gateway(10, 0, 10, 1);
IPAddress subnet(255, 255, 255, 0);

EthernetClient client;

// =====================================================================
// TEMPORIZADORES
// =====================================================================
unsigned long ultimoEnvio = 0;
unsigned long intervaloEnvio = 1800000;  // 30 minutos por defecto
unsigned long ultimaConexionExitosa = 0;

// =====================================================================
// SETUP
// =====================================================================
void setup() {
  Serial.begin(9600);
  Serial.println(F("\n=== ESTACION METEOROLOGICA INICIANDO ==="));

  // Liberar bus SPI: CS del SD (pin 4) y CS del Ethernet (pin 10) en reposo
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  setupDHT22();
  setupPluviometro();
  setupBateria();
  setupRed();

  // Primer envío al arrancar
  enviarDatosClima();

  unsigned long ahora = millis();
  ultimoEnvio = ahora;
  ultimaConexionExitosa = ahora;
}

// =====================================================================
// LOOP
// =====================================================================
void loop() {
  unsigned long tiempoActual = millis();

  Ethernet.maintain();

  // 1. Evento instantáneo de lluvia (con rate-limit dentro de la función)
  if (consultarYLimpiarNuevaLluvia()) {
    enviarSoloLluvia(obtenerContadorLluvia());
  }

  // 2. Reporte de rutina según intervalo dinámico
  if (tiempoActual - ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = tiempoActual;
    enviarDatosClima();
  }

  // 3. Watchdog: si pasó intervalo + 60s sin POST exitoso, reiniciar
  if (tiempoActual - ultimaConexionExitosa >= (intervaloEnvio + 60000)) {
    reiniciarArduino();
  }
}