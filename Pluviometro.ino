#include <EEPROM.h>

const byte PIN_PLUVIOMETRO = 3;
const unsigned long TIEMPO_REBOTES = 300; // ms anti-rebote
const int DIRECCION_EEPROM = 0;           // byte 0-1 de la EEPROM

volatile uint16_t contadorLluvia = 0;
volatile unsigned long ultimoTiempoPulsoLluvia = 0;
volatile bool hayNuevaLluvia = false;

void contarLluvia() {
  unsigned long tiempoActual = millis();
  if (tiempoActual - ultimoTiempoPulsoLluvia > TIEMPO_REBOTES) {
    contadorLluvia = (contadorLluvia + 1) % 2048;

    // Guardar el nuevo valor en la EEPROM para que no se pierda al reiniciar
    EEPROM.put(DIRECCION_EEPROM, contadorLluvia);

    ultimoTiempoPulsoLluvia = tiempoActual;
    hayNuevaLluvia = true;
  }
}

void setupPluviometro() {
  pinMode(PIN_PLUVIOMETRO, INPUT_PULLUP);

  // Al arrancar, recuperamos el último contador guardado antes del reinicio
  EEPROM.get(DIRECCION_EEPROM, contadorLluvia);

  // Si la EEPROM está virgen/corrupta (devuelve 65535), la inicializamos en 0
  if (contadorLluvia >= 2048) {
    contadorLluvia = 0;
    EEPROM.put(DIRECCION_EEPROM, contadorLluvia);
  }

  attachInterrupt(digitalPinToInterrupt(PIN_PLUVIOMETRO), contarLluvia, FALLING);
}

uint16_t obtenerContadorLluvia() {
  noInterrupts();
  uint16_t valor = contadorLluvia;
  interrupts();
  return valor;
}

bool consultarYLimpiarNuevaLluvia() {
  noInterrupts();
  bool estado = hayNuevaLluvia;
  hayNuevaLluvia = false;
  interrupts();
  return estado;
}