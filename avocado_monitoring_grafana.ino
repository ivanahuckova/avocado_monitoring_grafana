#include <Arduino.h>
#include <Prometheus.h>
#include <bearssl_x509.h>
#include <DHT.h>
#include <HCSR04.h>
#include <LedControl.h>

#include "certificates.h"
#include "config.h"

// Sensors
DHT dht(DHT_PIN, DHT_TYPE);
UltraSonicDistanceSensor distanceSensor(ULTRASONIC_PIN_TRIG, ULTRASONIC_PIN_ECHO);

// LED
LedControl lc=LedControl(LED_DIN,LED_CLK,LED_CS,0);

// LED visualisations
byte smile[8] = {0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C};
byte neutral[8] = {0x3C, 0x42, 0xA5, 0x81, 0xBD, 0x81, 0x42, 0x3C};
byte sad[8] = {0x3C, 0x42, 0xA5, 0x81, 0x99, 0xA5, 0x42, 0x3C};
byte off[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte err[8] = {0x00, 0x00, 0x78, 0x40, 0x70, 0x40, 0x78, 0x00};

// Prometheus client
Prometheus client;
// Create a write request for 6 series
WriteRequest req(6, 1536);

// Create a labelset arrays for the 2 labels that is going to be used for all series
LabelSet label_set[] = {{ "monitoring_type", "avocado" }, { "board_type", "esp32-devkit1" }};

// Define a TimeSeries which can hold up to 5 samples, has a name of `temperature/humidity/...` and uses the above labels of which there are 2
TimeSeries ts1(5, "temperature_celsius", label_set, 2);
TimeSeries ts2(5, "humidity_percent", label_set, 2);
TimeSeries ts3(5, "heat_index_celsius", label_set, 2);
TimeSeries ts4(5, "height_centimeter", label_set, 2);
TimeSeries ts5(5, "light_lux", label_set, 2);
TimeSeries ts6(5, "soil_moisture", label_set, 2);

int loopCounter = 0;

// Function to set up Prometheus client
void setupClient() {
  Serial.println("Setting up client...");
  // Configure the client
  client.setUrl(GC_URL);
  client.setPath(GC_PATH);
  client.setPort(GC_PORT);
  client.setUser(GC_USER);
  client.setPass(GC_PASS);
  client.setUseTls(true);
  client.setCerts(TAs, TAs_NUM);
  client.setWifiSsid(WIFI_SSID);
  client.setWifiPass(WIFI_PASSWORD);
  client.setDebug(Serial);  // Remove this line to disable debug logging of the client.
  if (!client.begin()){
      Serial.println(client.errmsg);
  }
  // Add our TimeSeries to the WriteRequest
  req.addTimeSeries(ts1);
  req.addTimeSeries(ts2);
  req.addTimeSeries(ts3);
  req.addTimeSeries(ts4);
  req.addTimeSeries(ts5);
  req.addTimeSeries(ts6);
  req.setDebug(Serial);  // Remove this line to disable debug logging of the write request serialization and compression.
}


// Function to display state of the plant on LED Matrix
void displayState(byte character []) {
  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, character[i]);
  }
}

// Function to read soil moisture measurement
int getSoilMoisture() {
  // Turn the sensor ON
  digitalWrite(MOISTURE_POWER, HIGH);  
  // Allow power to settle
  delay(10);        
  // Read the digital value form sensor                   
  int val = digitalRead(MOISTURE_PIN); 
  // Turn the sensor OFF
  digitalWrite(MOISTURE_POWER, LOW);   
  // Return moisture value
  // Serial.println(val);
  return val;         
}

// Function to read height of the plant
double getHeight() {
  double manualCurrentHeight = 25;
  double height = DISTANCE_FROM_POT - distanceSensor.measureDistanceCm();
  // Serial.println(distanceSensor.measureDistanceCm());
  if (height > manualCurrentHeight - 5 && height < manualCurrentHeight + 5) {
    return height;
  }

  return manualCurrentHeight;  
}

// Function to read light condition
float getLightLux() {
  float sensor_value = analogRead(TEMP6000_PIN);  // Get raw sensor reading
  float volts = sensor_value * TEMP6000_VCC / 1024.0;  // Convert reading to voltage
  float amps = volts / 10000.0; // Convert to amps across 10K resistor
  float microamps = amps * 1000000.0; // Convert amps to microamps
  float lux = microamps * 2.0; 
  
  return lux;
}

// Function to check if any reading failed
bool checkIfReadingFailed(float hum, float cels, int moist, double height, float light) {
  if (isnan(hum) || isnan(cels) || isnan(moist) || isnan(height) || isnan(light)) {
      // Print letter E as error
      displayState(err);
      Serial.println(F("Failed to read from one of the sensors!"));
      return true;
    }
    return false;
}

// Function to create message and display current state of the plant
String createAndDisplayState (int moist, float cels) {
    String currentState = "";
    if (moist) {
      currentState = "DRY critical";
      displayState(sad);
    } else if( cels < 16 ) {
      currentState = "COLD warning";
      displayState(neutral);
    } else if( cels > 26 ) {
      currentState = "HOT warning";
      displayState(neutral);
    } else {
      currentState = "OK info";
      displayState(smile);
    }

    return currentState;
}

// ========== MAIN FUNCTIONS: SETUP & LOOP ========== 
// SETUP: Function called at boot to initialize the system
void setup() {

  // Set up soil moisture pins - initially keep the moisture sensor OFF (prevents sensor rusting)
  pinMode(MOISTURE_POWER, OUTPUT);
  digitalWrite(MOISTURE_POWER, LOW);

  // Start the serial output at 115,200 baud 
  Serial.begin(115200);

  // Connect to WiFi
  setupClient();

  // Start the DHT sensor
  dht.begin();

  // Start LED Matrix
  lc.shutdown(0,false);       
  lc.setIntensity(0,0.0000001);      //Adjust the brightness maximum is 15
  lc.clearDisplay(0);    
}

// LOOP: Function called in a loop to read from sensors and send them do databases
void loop() {
  // Get current timestamp
  int64_t time;
  time = client.getTimeMillis();

  // Read humidity
  float hum = dht.readHumidity();

  // Read temperature as Celsius (the default)
  float cels = dht.readTemperature();

  // Read soil moisture (DRY: 1, WET: 0)
  int moist = getSoilMoisture();

  // Read height of plant
  double height = getHeight();

  //Read the light in lux
  float light = getLightLux();

  // Check if any reads failed and return early
  if (checkIfReadingFailed(hum, cels, moist, height, light)) {
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(cels, hum, false);

  // Convert data to state of the plant
  String message = createAndDisplayState(moist, cels);

  // Submit data
  if (loopCounter >= 5) {
    //Send
    loopCounter = 0;
    if (!client.send(req)) {
      Serial.println(client.errmsg);
    }
    // Reset batches after a succesful send.
    ts1.resetSamples();
    ts2.resetSamples();
    ts3.resetSamples();
    ts4.resetSamples();
    ts5.resetSamples();
    ts6.resetSamples();
  } else {
    if (!ts1.addSample(time, cels)) {
      Serial.println(ts1.errmsg);
    }
    if (!ts2.addSample(time, hum)) {
      Serial.println(ts2.errmsg);
    }
    if (!ts3.addSample(time, hic)) {
      Serial.println(ts3.errmsg);
    }
    if (!ts4.addSample(time, height)) {
      Serial.println(ts4.errmsg);
    }
    if (!ts5.addSample(time, light)) {
      Serial.println(ts5.errmsg);
    }
    if (!ts6.addSample(time, moist)) {
      Serial.println(ts6.errmsg);
    }
    loopCounter++;
  }

  // wait INTERVAL seconds, then do it again
  delay(2 * 1000);
}
