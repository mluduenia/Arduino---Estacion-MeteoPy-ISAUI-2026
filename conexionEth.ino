// --- VARIABLES Y OBJETOS EXTERNOS (Definidos en EstacionMeteo.ino) ---
extern EthernetClient client;
extern IPAddress serverIP;
extern int serverPort;
extern byte mac[];
extern IPAddress ip;
extern IPAddress gateway;
extern IPAddress subnet;
extern unsigned long ultimaConexionExitosa;
extern unsigned long intervaloEnvio;

// --- INICIALIZACIÓN DE RED ---
void setupRed() {
  Serial.println(F("Iniciando Ethernet Shield (W5100)..."));
  Ethernet.begin(mac, ip, gateway, subnet);
  delay(2000);

  Serial.print(F(" -> [ETH] Arduino listo en IP: "));
  Serial.println(Ethernet.localIP());
}

// Función auxiliar para leer "intervalo_ms" desde el JSON que devuelve Flask
void procesarRespuestaServidor() {
  unsigned long inicioEspera = millis();
  String respuesta = "";

  while (client.connected() && millis() - inicioEspera < 2000) {
    while (client.available()) {
      char c = client.read();
      respuesta += c;
    }
  }

  int pos = respuesta.indexOf("\"intervalo_ms\":");
  if (pos != -1) {
    String sub = respuesta.substring(pos + 15);
    unsigned long nuevoIntervalo = sub.toInt();

    // Rango de seguridad: entre 1 minuto y 24 horas
    if (nuevoIntervalo >= 60000 && nuevoIntervalo <= 86400000) {
      if (intervaloEnvio != nuevoIntervalo) {
        intervaloEnvio = nuevoIntervalo;
        Serial.print(F(" -> [ETH] ¡Intervalo de reporte actualizado desde Flask!: "));
        Serial.print(intervaloEnvio / 60000);
        Serial.println(F(" minutos."));
      }
    }
  }
}

// 1. REPORTE DE RUTINA
void enviarDatosClima() {
  client.stop();
  delay(50);

  float h = leerHumedad();
  float t = leerTemperatura();
  uint16_t lluviaCont = obtenerContadorLluvia();
  float vBat = leerVoltajeBateria();

  Serial.print(F("Lectura Clima -> Temp: "));
  Serial.print(t, 2);
  Serial.print(F(" C | Hum: "));
  Serial.print(h, 2);
  Serial.print(F(" % | Lluvia Cont: "));
  Serial.print(lluviaCont);
  Serial.print(F(" | Bat: "));
  Serial.print(vBat, 2);
  Serial.println(F(" V"));

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Error al leer DHT22"));
    return;
  }

  String body = "{\"temperatura\":" + String(t, 1) +
                ",\"humedad\":" + String(h, 1) +
                ",\"lluvia_contador\":" + String(lluviaCont) +
                ",\"bateria\":" + String(vBat, 2) + "}";

  if (client.connect(serverIP, serverPort)) {
    // Host header como texto ASCII (no client.print(serverIP))
    String host = String(serverIP[0]) + "." + String(serverIP[1]) + "." +
                  String(serverIP[2]) + "." + String(serverIP[3]) + ":" +
                  String(serverPort);

    client.println(F("POST /api/sensor HTTP/1.1"));
    client.print(F("Host: "));
    client.println(host);
    client.println(F("Content-Type: application/json"));
    client.print(F("Content-Length: "));
    client.println(body.length());
    client.println(F("Connection: close"));
    client.println();
    client.print(body);

    Serial.println(F(" -> [ETH] Reporte enviado. Procesando respuesta..."));

    procesarRespuestaServidor();

    ultimaConexionExitosa = millis();
    delay(100);
    client.stop();
  } else {
    Serial.println(F(" -> [ETH] Error de conexión en reporte de rutina."));
  }
}

// 2. EVENTO INSTANTÁNEO DE LLUVIA
void enviarSoloLluvia(uint16_t lluviaCont) {
  client.stop();
  delay(50);

  String body = "{\"lluvia_contador\":" + String(lluviaCont) + "}";

  if (client.connect(serverIP, serverPort)) {
    String host = String(serverIP[0]) + "." + String(serverIP[1]) + "." +
                  String(serverIP[2]) + "." + String(serverIP[3]) + ":" +
                  String(serverPort);

    client.println(F("POST /api/sensor HTTP/1.1"));
    client.print(F("Host: "));
    client.println(host);
    client.println(F("Content-Type: application/json"));
    client.print(F("Content-Length: "));
    client.println(body.length());
    client.println(F("Connection: close"));
    client.println();
    client.print(body);

    Serial.println(F(" -> [ETH] ¡Evento de lluvia enviado! Procesando respuesta..."));

    procesarRespuestaServidor();

    ultimaConexionExitosa = millis();
    delay(100);
    client.stop();
  } else {
    Serial.println(F(" -> [ETH] Error de conexión en evento de lluvia."));
  }
}

// 3. WATCHDOG: REINICIO POR SOFTWARE
void reiniciarArduino() {
  Serial.println(F("!!! ALERTA: TIEMPO LIMITE SIN CONEXION. REINICIANDO... !!!"));
  Serial.flush();
  delay(500);
  asm volatile ("jmp 0");
}