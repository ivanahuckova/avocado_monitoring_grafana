#define WIFI_SSID     "wifi_name" // Add wifi name
#define WIFI_PASSWORD "wifi_password"  // Add wifi passowrd

#define ID "avocado" // Add unique name for this sensor
#define INTERVAL 5  // Add interval (e.g. 1 min)

#define DHT_PIN 5    // Which pin is DHT 22 connected to
#define DHT_TYPE DHT11 // Type DHT 11

#define MOISTURE_POWER 33 // Which pin is soil moisture sensor connected to as a source of power (prevents fast corrosioon)
#define MOISTURE_PIN 27 // Which pin is soil moisture sensor connected to
#define DISTANCE_FROM_POT 36.40 // Calculated so we get the height of plant frm the soil

#define ULTRASONIC_PIN_TRIG 14 // Which pin is HC-SR04's trig connected to
#define ULTRASONIC_PIN_ECHO 12 // Which pin is HC-SR04's echo connected to

#define TEMP6000_PIN 26 // Light sensor pin
#define TEMP6000_VCC 5.0 // Light used voltage

// LED Matrix
#define LED_DIN 18
#define LED_CS 19
#define LED_CLK 21

// Prometheus client
#define GC_PROM_URL "prometheus-us-central1.grafana.net" // Url to Prometheus instance 
#define GC_PROM_PATH "/api/prom/push" // Path
#define GC_PORT 443
#define GC_PROM_USER "" // Username
#define GC_PROM_PASS "" // API key