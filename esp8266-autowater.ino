#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

const int ANALOG_HUMIDITY_READ_FREQ = 30000; // millis

const byte HUMIDITY_SENSOR_DIGITAL_PIN = 16;
const byte HUMIDITY_SENSOR_DIGITAL_LED_PIN = 13;
const byte HUMIDITY_SENSOR_ANALOG_PIN = A0;
const byte WATER_LEVEL_SENSOR_PIN = 5;
const byte WATER_LEVEL_SENSOR_LED_PIN = 12;
const byte MODE_PIN = 0;
const byte PUMP_PIN = 4;

const byte MODE_MANUAL = 0;
const byte MODE_AUTO = 1;
const byte HUMIDITY_DRY = 1;
const byte WATER_LEVEL_HAS_WATER = 0;

const char *ssid = "SSID";
const char *password = "PASSWORD";
const char *mqtt_server = "SERVER_IP";
const char *device_id = "esp8266";

WiFiClient espClient;
PubSubClient client(espClient);

const byte TEST_LED_PIN = 2;

char messageBuffer[128];

struct Humidity {
  int analog;
  int digital;
} humidity = { -1, -1 };

int waterLevel = -1;
int mode = -1;
boolean pumpIsWorking = false;
boolean lastPumpControlMessage = false;

unsigned long lastAnalogHumidityRead = 0;

const String createTopicFromDeviceId(String topic) {
  return (String(device_id) + "/" + topic).c_str();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  int q;
  for (q = 0; q < length; q++) {
    messageBuffer[q] = (char)payload[q];
  }
  messageBuffer[q] = '\0';

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  String msgString = String(messageBuffer);
  Serial.println(msgString);
  String topicString = String(topic);

  if (topicString == createTopicFromDeviceId("pump/control")) {
    if (msgString == "1") {
      lastPumpControlMessage = true;
    } else if (msgString == "0") {
      lastPumpControlMessage = false;
    }
  }

  if (topicString == createTopicFromDeviceId("led_control")) {
    if (msgString == "1")
    {
      Serial.println("ON");
      digitalWrite(TEST_LED_PIN, HIGH); // PIN HIGH will switch OFF the relay
    }
    if (msgString == "0")
    {
      Serial.println("OFF");
      digitalWrite(TEST_LED_PIN, LOW); // PIN LOW will switch ON the relay
    }
  }
}

void setPump(boolean isOn) {
  if (isOn != pumpIsWorking) {
    if (isOn) {
      digitalWrite(PUMP_PIN, HIGH);
      publish(createTopicFromDeviceId("pump/state").c_str(), "1");
    } else {
      digitalWrite(PUMP_PIN, LOW);
      publish(createTopicFromDeviceId("pump/state").c_str(), "0");
    }
    pumpIsWorking = isOn;
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(device_id))
    {
      Serial.println("connected");
      subscribe(createTopicFromDeviceId("led_control"));
      subscribe(createTopicFromDeviceId("pump/control"));
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to ");
  Serial.println(ssid);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // LEDs
  pinMode(TEST_LED_PIN, OUTPUT);
  pinMode(WATER_LEVEL_SENSOR_LED_PIN, OUTPUT);
  pinMode(HUMIDITY_SENSOR_DIGITAL_LED_PIN, OUTPUT);

  pinMode(HUMIDITY_SENSOR_DIGITAL_PIN, INPUT);
  pinMode(HUMIDITY_SENSOR_ANALOG_PIN, INPUT);
  pinMode(WATER_LEVEL_SENSOR_PIN, INPUT_PULLUP);
  pinMode(MODE_PIN, INPUT_PULLUP);
  pinMode(PUMP_PIN, OUTPUT);
}

void handleInputs() {
  unsigned long time = millis();
  // --------------------------------
  // MODE
  // --------------------------------
  int newMode = digitalRead(MODE_PIN);
  if (mode != newMode) {
    mode = newMode;
    Serial.print("Mode changed to ");
    Serial.println(mode);
    publish(createTopicFromDeviceId("mode").c_str(), mode);
  }
  // --------------------------------
  // HUMIDITY (DIGITAL)
  // --------------------------------
  int newHumidityDigital = digitalRead(HUMIDITY_SENSOR_DIGITAL_PIN);
  if (humidity.digital != newHumidityDigital) {
    humidity.digital = newHumidityDigital;
    Serial.print("Humidity changed to ");
    Serial.print(humidity.digital);
    Serial.println(" (digital)");
    publish(createTopicFromDeviceId("humidity/digital").c_str(), humidity.digital);
    if (humidity.digital == HUMIDITY_DRY) {
      digitalWrite(HUMIDITY_SENSOR_DIGITAL_LED_PIN, HIGH);
    } else {
      digitalWrite(HUMIDITY_SENSOR_DIGITAL_LED_PIN, LOW);
    }
  }
  // --------------------------------
  // HUMIDITY (ANALOG)
  // --------------------------------
  /*if (abs(time - lastAnalogHumidityRead) >= ANALOG_HUMIDITY_READ_FREQ) {
    lastAnalogHumidityRead = time;
    int newHumidityAnalog = analogRead(HUMIDITY_SENSOR_ANALOG_PIN);
    humidity.analog = newHumidityAnalog;
    Serial.print("Humidity changed to ");
    Serial.print(humidity.analog);
    Serial.println(" (analog)");
    char humidityAnalogPayload[8];
    itoa(humidity.analog, humidityAnalogPayload, 10);
    publish(createTopicFromDeviceId("humidity/analog"), humidityAnalogPayload);
    }*/
  // --------------------------------
  // WATER LEVEL
  // --------------------------------
  int newWaterLevel = digitalRead(WATER_LEVEL_SENSOR_PIN);
  if (waterLevel != newWaterLevel) {
    waterLevel = newWaterLevel;
    Serial.print("Water level changed to ");
    Serial.println(waterLevel);
    publish(createTopicFromDeviceId("water_level").c_str(), waterLevel);
    if (waterLevel != WATER_LEVEL_HAS_WATER) {
      digitalWrite(WATER_LEVEL_SENSOR_LED_PIN, HIGH);
    } else {
      digitalWrite(WATER_LEVEL_SENSOR_LED_PIN, LOW);
    }
  }
}

boolean publish(String topic, const char* payload) {
  return client.publish(topic.c_str(), payload);
}

boolean publish(String topic, const int value) {
  char payload[8];
  itoa(value, payload, 10);
  return publish(topic, payload);
}

boolean subscribe(String topic) {
  return client.subscribe(topic.c_str());
}

void pumpLoop() {
  boolean shouldWorkAuto = humidity.digital == HUMIDITY_DRY && waterLevel == WATER_LEVEL_HAS_WATER;
  boolean shouldWorkManual = lastPumpControlMessage;

  setPump((mode == MODE_AUTO && shouldWorkAuto) || (mode == MODE_MANUAL && shouldWorkManual));
}

void loop() {
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  handleInputs();
  pumpLoop();
}