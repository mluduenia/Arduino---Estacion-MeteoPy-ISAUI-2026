#define DHTPIN 2
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setupDHT22() {
  dht.begin();
}

float leerTemperatura() {
  return dht.readTemperature();
}

float leerHumedad() {
  return dht.readHumidity();
}