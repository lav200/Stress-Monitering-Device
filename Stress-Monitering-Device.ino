// libraries needed!


#include <WiFi.h>                //wifi library
#include <BlynkSimpleEsp32.h>    // blynk library
#include <Adafruit_GFX.h>        // OLED Library
#include <Adafruit_SSD1306.h>    // OLED Library
#include <Wire.h>                // I2C  Library
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "MAX30105.h"            // MAX3010x library
#include "heartRate.h"           // Heart rate calculating algorithm




#define BLYNK_PRINT Serial
WiFiClient  client;




BlynkTimer timer;


char authBlynk[] = "9dXleInVrfDzCKLMk8RS7UuhWJAXZ2pU"; 
char ssid[] = "HWifi";
char pass[] = "12345678";


Adafruit_BME280 bme;
MAX30105 particleSensor;


const byte RATE_SIZE = 4;        // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE];           // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0;               // Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;


#define SCREEN_WIDTH 128         // OLED display width, in pixels
#define SCREEN_HEIGHT 64         // OLED display height, in pixels
#define OLED_RESET    -1         // Reset pin # (or -1 if sharing Arduino reset pin)


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Declaring the display name (display)




void setup() {
  bool status = bme.begin(0x76);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  Serial.begin(115200); 
  Serial.println("BIO WATCH DEVICE..."); 
  Wire.setClock(400000);                         // Set I2C speed to 400kHz
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);     // Start the OLED display
  display.setTextColor(WHITE);


  // Initialize sensor
  particleSensor.begin(Wire, I2C_SPEED_FAST);    // Use default I2C port, 400kHz speed
  particleSensor.setup();                        // Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A);     // Turn Red LED to low to indicate sensor is running

  Blynk.begin(authBlynk, ssid, pass);
  delay (3000);
 
}


void loop() {
  long irValue = particleSensor.getIR();              // Reading the IR value it will permit us to know if there's a finger on the sensor or not




  //  Blynk.run();


  if (irValue > 50000)
  {


    if (checkForBeat(irValue) == true)                  //If a heart beat is detected, call checkForBeat as frequent as possible to get accurate value
    {
      long delta = millis() - lastBeat;                 //Measure duration between two beats
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);           //Calculating the BPM
      if (beatsPerMinute < 255 && beatsPerMinute > 20)  //To calculate the average we strore some values (4) then do some math to calculate the average
      {
        rates[rateSpot++] = (byte)beatsPerMinute;       //Store this reading in the array
        rateSpot %= RATE_SIZE;                          //Wrap variable


        //Take average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;




        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0, 5);
        display.print("TEMP:");
        display.print(String(bme.readTemperature()));
        display.print("C");
        display.setTextSize(3);
        display.setCursor(0, 25);
        display.print("BPM:");
        display.print(beatAvg);
        display.setTextSize(1);
        display.print("BPM");
        display.display();
        Blynk.virtualWrite(V0, temperature);
        Blynk.virtualWrite(V1,beatAvg );
      }
    }
  }
  else
  {
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(0, 20);
    display.print("BPM:");
    display.print(0);
    display.setCursor(0, 45);
    display.setTextSize(1);
    display.print("Ware it properly and wait..");
    display.display();
    Blynk.virtualWrite(V0, 0);
    Blynk.virtualWrite(V1,0 );
  }
}