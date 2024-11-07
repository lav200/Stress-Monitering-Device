#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <UniversalTelegramBot.h>
#include "MAX30105.h"
#include "heartRate.h"

// WiFi credentials
#define WIFI_SSID "LAVKUSH_CHAUDHARI "
#define WIFI_PASSWORD "8108491665"

// Telegram bot token
#define BOT_API_TOKEN "7534847312:AAEmhLTw7KuXPPPt9pWi00ePsqpilst1zdo"

// Pin for the LED
#define LED_PIN 13  // LED connected to pin 13

// BME280 sensor configuration
Adafruit_BME280 bme; // I2C

// MAX30105 sensor configuration
MAX30105 particleSensor;
const byte RATE_SIZE = 4; // Increase for more averaging
byte rates[RATE_SIZE]; // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time of the last beat
float beatsPerMinute;
int beatAvg;

// Variables to manage the chat ID and message ID
String chat_id = "";
int message_id = -1;

// Object for secure WiFi client management
WiFiSSLClient securedClient;

// Object for managing the Telegram bot
UniversalTelegramBot bot(BOT_API_TOKEN, securedClient);

// Constant for the interval between message checks
const unsigned long interval = 1000;
unsigned long last_call = 0;

// Function to handle incoming messages from Telegram
void handleMessages(int num_new_messages) {
  for (int i = 0; i < num_new_messages; i++) {
    message_id = bot.messages[i].message_id;
    chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    // Command handling
    if (text == "/ON") {
      digitalWrite(LED_PIN, HIGH);  // Turn the LED on
      bot.sendMessage(chat_id, "LED is ON.", "Markdown");
    } else if (text == "/OFF") {
      digitalWrite(LED_PIN, LOW);   // Turn the LED off
      bot.sendMessage(chat_id, "LED is OFF.", "Markdown");
    } else if (text == "/TEMP") {
      float temperature = bme.readTemperature(); // Read temperature
      bot.sendMessage(chat_id, "Temperature: " + String(temperature, 2) + " °C", "Markdown");
      Serial.print("Temperature: ");
      Serial.print(temperature, 2);
      Serial.println(" °C");
    } else if (text == "/BPM") {
      bot.sendMessage(chat_id, "Average BPM: " + String(beatAvg), "Markdown");
    } else if (text == "/start") {
      bot.sendMessage(chat_id, "Use /ON to turn the LED on, /OFF to turn it off, /TEMP to get the temperature, and /BPM to get the average heart rate.", "Markdown");
    }
  }
}

// Initialization function
void setup() {
  Serial.begin(115200);             // Start serial communication
  pinMode(LED_PIN, OUTPUT);         // Set LED pin as output
  digitalWrite(LED_PIN, LOW);       // Ensure the LED is off initially
  connectToWiFi();                  // Connect to WiFi
  
  // Initialize BME280 sensor
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1); // Stop execution if sensor is not found
  }

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power.");
    while (1);
  }
  particleSensor.setup(); // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); // Set Red LED amplitude
  particleSensor.setPulseAmplitudeGreen(0); // Turn off Green LED

  bot.sendMessage(chat_id, "BOT Started", "Markdown"); // Notify that the bot has started
}

// Main loop
void loop() {
  // Check for new messages from Telegram
  if (millis() - last_call > interval) {
    int num_new_messages = bot.getUpdates(bot.last_message_received + 1);
    while (num_new_messages) {
      handleMessages(num_new_messages);  // Handle incoming messages
      num_new_messages = bot.getUpdates(bot.last_message_received + 1);
    }
    last_call = millis(); // Update the last call time
  }

  // Heart rate monitoring
  long irValue = particleSensor.getIR();
  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();
    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store reading in the array
      rateSpot %= RATE_SIZE; // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }

  // Print heart rate data to Serial Monitor
  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 30000)
    Serial.print(" No finger?");

  Serial.println();
}

// Function to connect to the WiFi network
void connectToWiFi() {
  Serial.print("Connecting to WiFi network ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  
  Serial.print("WiFi connected. IP Address: ");
  Serial.println(WiFi.localIP());
}
