const byte PIN_BATERIA = A0;

// Divisor de tensión: R1 = 10k, R2 = 4.7k
const float R1 = 10000.0;
const float R2 = 4700.0;

void setupBateria() {
  pinMode(PIN_BATERIA, INPUT);
}

float leerVoltajeBateria() {
  long suma = 0;
  for (int i = 0; i < 10; i++) {
    suma += analogRead(PIN_BATERIA);
    delay(10);
  }
  float promedioADC = suma / 10.0;
  float voltajeA0 = (promedioADC * 5.0) / 1023.0;
  return voltajeA0 * ((R1 + R2) / R2);
}