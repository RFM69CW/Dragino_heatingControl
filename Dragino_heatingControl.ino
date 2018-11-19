#include <Bridge.h>
#include <Console.h>
#include <PubSubClient.h>
#include <BridgeClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define MQTT_SERVER "192.168.xxx.xxx"
#define MQTT_PORT 1883
#define MQTT_CLIENTID "PelletNode"
long lastReconnectAttempt = 0;

#define ONE_WIRE_BUS_PIN 10
#define MAX_DALLAS 1

float temp[MAX_DALLAS] = {};
OneWire oneWire(ONE_WIRE_BUS_PIN);  // Setup a oneWire to com with any OneWire devices
DallasTemperature sensors(&oneWire);  //Pass our oneWire reference to Dallas Temp.
DeviceAddress address[MAX_DALLAS] =
{
  0x28, 0xFF, 0xB5, 0xC1, 0x71, 0x17, 0x03, 0x47
};

int trigger = 9;
int echo = 8;
long duration = 0;
long distance = 0;
int HEART_LED = A2;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  //  Console.print("Message arrived [");
  //  Console.print(topic);
  //  Console.print("] ");
  //  for (int i=0;i<length;i++) {
  //    Console.print((char)payload[i]);
  //  }
  //  Console.println();
}

BridgeClient bridgeClient;
PubSubClient mqtt(MQTT_SERVER, MQTT_PORT, mqttCallback, bridgeClient);

void setup() {
  sensors.begin();
  pinMode(HEART_LED, OUTPUT);
  flashLed(HEART_LED, 3, 100);
  Bridge.begin();
  flashLed(HEART_LED, 3, 50);
  Console.begin();
  flashLed(HEART_LED, 3, 10);
  mqtt.connect(MQTT_CLIENTID);
  Console.println(F("STARTUP"));
  flashLed(HEART_LED, 3, 10);
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
}

void loop() {
  if (!mqtt.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      flashLed(HEART_LED, 1, 10);
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    Console.println(freeRam());
    distance = getDistance();
    float factor = 100 / (float)70;
    float percent = factor * distance;
    long percentOut = round(percent);
    if (percentOut > 100) percentOut = 100;
    if (percentOut < 0) percentOut = 0;
    Console.println(distance);
    Console.println(percent);
    char buf[50];
    Console.println(ltoa(distance, buf, 10));
    boolean result = mqtt.publish("heating/level", ltoa(distance, buf, 10));    
    memset(&buf[0], 0, sizeof(buf));
    mqtt.publish("heating/level/percentage", ltoa(percentOut, buf, 10));
    //mqtt.publish("pellet/temp", ltoa(temp[0], buf, 10));
    Console.println(result);
    flashLed(HEART_LED, 3, 100);
    delay(10000);

    mqtt.loop();
  }

}


long getDistance()
{
  long distance = 0;
  noInterrupts();
  digitalWrite(trigger, LOW);
  delay(5);
  digitalWrite(trigger, HIGH);
  delay(10);
  digitalWrite(trigger, LOW);
  duration = pulseIn(echo, HIGH);
  interrupts();
  distance = (duration / 2) * 0.03431;
  if (distance >= 500 || distance <= 0)
  {
    Console.println(F("No valid result"));
    flashLed(HEART_LED, 10, 10);
  }
  else
  {
    // green to 25 cm
    // yellow to 50 cm
    // red under 50 cm
    // shutdown at 70 cm
    distance = 70 - distance;
    Console.print(distance);
    Console.println(F(" cm"));
  }
  return distance;
}

void flashLed(int pin, int times, int wait)
{

  for (int i = 0; i < times; i++) {
    digitalWrite(pin, HIGH);
    delay(wait);
    digitalWrite(pin, LOW);

    if (i + 1 < times) {
      delay(wait);
    }
  }
}


int freeRam ()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void getTemperatures(void)
{
  sensors.requestTemperatures();
  delay(1000);
  for (int j = 0; j < MAX_DALLAS; j++)
  {
    if (sensors.validAddress(address[j]) && sensors.isConnected(address[j]))
    {
      temp[j] = sensors.getTempC(address[j]);
    }
    else
    {
      temp[j] = 0;  //DELETE FROM S_Sensor5_2016 where temp_2='0.0';
    }
  }
}

boolean reconnect() {
  if (mqtt.connect(MQTT_CLIENTID)) {
  }
  return mqtt.connected();
}

