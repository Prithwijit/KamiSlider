#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <FlexyStepper.h>
#include "config_cameraslider.h"
#include "config_wifi.h"
#include "include/PersistSettings.h"
#include <DNSServer.h>
#include "Point.h"
#include <AccelStepper.h>
#include <MultiStepper.h>
//#include <Arduino.h>
//#include "driver/adc.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Ticker.h>
#include "bitmap32.h"

FlexyStepper stepper_slide;
FlexyStepper stepper_pan;
AccelStepper stepper1(1, 19, 18); // (Type: driver, STEP, DIR) - DIR moved to GPIO 18
AccelStepper stepper2(1, 17, 16); // (Type: driver, STEP, DIR) - DIR moved to GPIO 23

// Create a Ticker instance for display updates
Ticker displayTicker;
MultiStepper StepperControl; 
AsyncWebServer server(80);
const byte DNS_PORT =53;
DNSServer dnsServer;
int WiFi_status = WL_IDLE_STATUS; 

// Static IP configuration
IPAddress local_IP(192, 168, 43, 100);       // Static IP for ESP32 on phone's hotspot (outside DHCP range)
IPAddress gateway(192, 168, 43, 1);          // Default gateway (phone's IP address)
IPAddress subnet(255, 255, 255, 0);         // Subnet mask

// Optional: Set DNS servers (Google's DNS in this case)
IPAddress primaryDNS(8, 8, 8, 8);  // Google's DNS
IPAddress secondaryDNS(8, 8, 4, 4); // Google's DNS (optional)
boolean updateULflag=false;
int updateRailVal=100;
int flag=0; 
unsigned long switch0=0;
volatile boolean TurnDetected;  
volatile boolean rotationdirection;  
unsigned long lastInterruptTime = 0; // Debounce timer
boolean inMultiMotion =false;
int multiStep=-1;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50; 
int lastADButtonState = LOW;   // the previous reading from the input pin
// We will use persistent storage to store our Camera Slider configuration
// but also allow us to edit it trough web/api
// This way we can decouple firmware from electronics/mech changes
// i.e We can change rail lenght or motor direction without recompiling firmware
// Note: You should not edit config below. Instead modify defaults inside `config_cameraslider.h`
struct SliderConfigStruct
{
    static const unsigned int Version = 1;

    uint16_t rail_length = RAIL_LENGTH_MM;
    uint16_t min_slider_step = MIN_STEP_SLIDER;


    int homing_direction = -1;  // Specify if we should reverse homing direction
    int slider_direction = 1;   // Increase (1) or decrease(-1) steps to get positive movement
    int rotate_direction = 1;   // Increase (1) or decrease(-1) steps to get positive movement
    int exact_time = 1;   // Increase (1) or decrease(-1) steps to get positive movement

    uint16_t slide_steps_per_mm = SLIDE_STEPS_PER_MM;
    uint16_t pan_steps_per_degree = PAN_STEPS_PER_DEGREE;

    uint16_t homing_speed_slide = DEFAULT_HOMING_SPEED_SLIDE;
    uint16_t homing_speed_pan   = DEFAULT_HOMING_SPEED_PAN;

    float default_slider_speed = DEFAULT_SLIDE_TO_POS_SPEED;
    float default_slider_accel = DEFAULT_SLIDE_TO_POS_ACCEL;
    float default_rotate_speed = DEFAULT_ROTATE_TO_POS_SPEED;
    float default_rotate_accel = DEFAULT_ROTATE_TO_POS_ACCEL;
};

PersistSettings<SliderConfigStruct> SliderConfig(SliderConfigStruct::Version);

// Create an instance of the display object
// The second parameter is the I2C address, 0x3C is the most common address
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);  // -1 for default reset pin


// ISR function to handle the interrupt
void IRAM_ATTR outerLimit() {
    updateULflag=true;
    //CameraSlider_UpdateRailLength(getSliderPos());
}
// ISR function to handle the interrupt
void IRAM_ATTR switchAnalog() {
    SwitchAD();
}


void IRAM_ATTR Switch()  
{
 if(millis()-switch0>500 &&  digitalRead(PIN_A_D)==HIGH)
 {
 flag=flag+1;
 }
 switch0=millis();
}

void IRAM_ATTR Rotary() {
    unsigned long interruptTime = millis();  // Get the current time
  
    // Only handle the interrupt if enough time has passed since the last one
    if (interruptTime - lastInterruptTime > debounceDelay) {
      lastInterruptTime = interruptTime;  // Update the last interrupt time
  
      // Read the CLK and DT pins to determine the rotation direction
      if (digitalRead(PIN_ENC_CLK)) {
        rotationdirection = !digitalRead(PIN_ENC_DT);
      } else {
        rotationdirection = digitalRead(PIN_ENC_DT);
      }
      Serial.print(rotationdirection);
      Serial.print("rotationdirection");
      TurnDetected = true;  // Flag that a turn was detected
    }
  }
void setup()
{
    // Configure Serial communication
	Serial.begin(115200);
    Serial.println("DIY Camera Slider");

    // Peristent device config
    SliderConfig.Begin();
    if( SliderConfig.Valid() )
    {
        Serial.println("Reloading camera slider settings.");
    }
    else
    {
        Serial.println("Camera settings invalid. Resetting to default.");
    }

    // Configure and initialize GPIOs
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_MTR_nRST, OUTPUT);
	pinMode(PIN_MTR_nEN, OUTPUT);
	pinMode(PIN_END_SWICH_X, INPUT_PULLUP);
	pinMode(PIN_OEND_SWICH_X, INPUT_PULLUP);
	pinMode(PIN_JOY_X, INPUT);
	pinMode(PIN_JOY_SW, INPUT_PULLUP);
	pinMode(PIN_A_D, INPUT_PULLUP);
	pinMode(PIN_ENC_CLK, INPUT_PULLUP);
	pinMode(PIN_ENC_DT, INPUT_PULLUP);
	pinMode(PIN_ENC_SW, INPUT_PULLUP);

   
	digitalWrite(PIN_LED, HIGH);
	digitalWrite(PIN_MTR_nRST, HIGH);
	digitalWrite(PIN_MTR_nEN, LOW);

    // Attach the interrupt to the pin
    attachInterrupt(digitalPinToInterrupt(PIN_OEND_SWICH_X), outerLimit, FALLING);
    attachInterrupt (digitalPinToInterrupt(PIN_ENC_SW),Switch,RISING); // SW connected to D2
    attachInterrupt (digitalPinToInterrupt(PIN_ENC_CLK),Rotary,RISING); // CLK Connected to D3
 //attachInterrupt(digitalPinToInterrupt(PIN_A_D), switchAnalog, CHANGE);
    // Set up the interrupt on pin D25 (button press)
    //attachInterrupt(digitalPinToInterrupt(PIN_JOY_SW), switchAnalog, FALLING);
   
	delay(1000);
    // Start communication with the display
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Replace SSD1306_I2C_ADDRESS with 0x3C
        Serial.println(F("SSD1306 allocation failed"));
        for (;;); // Loop forever if the display is not found
    }

    display.display();  // Initialize the display




WiFi.softAP(Myssid,Mypassword);
IPAddress IP=WiFi.softAPIP();
Serial.print("AP IP:");
Serial.print(IP);
dnsServer.start(DNS_PORT,"kamislider.com",WiFi.softAPIP());
//   // Configure the static IP address
//   if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
//     Serial.println("STA Failed to configure");
//   }
/*



    int attempt=0;
	// Connect to WiFi
	while ( WiFi.status() != WL_CONNECTED && attempt<10)
    {
		Serial.print("Connecting to SSID: ");
		Serial.println(ssid);
		WiFi_status = WiFi.begin(ssid, password);
        /*
        // Clear the display
        display.clearDisplay();
        display.setTextSize(2);      // Text size
        display.setTextColor(SSD1306_WHITE);  // White text
        display.setCursor(0,0);      // Set cursor at the top-left
        display.print(F("Connecting to WiFi"));  // Constant message

        display.display();  // Refresh the display to show the message
*/
	/*	// wait 5 seconds before retrying
		delay(5000);
        attempt++;
	}*/
    /*
    // Clear the display
    display.clearDisplay();
    display.setTextSize(2);      // Text size
    display.setCursor(0,0);      // Set cursor at the top-left
    display.print(F("Connected"));  // Constant message

    display.display();  // Refresh the display to show the message
*/
	// Initialize SPIFFS
	if(!SPIFFS.begin(true))
    {
		Serial.println("An Error has occurred while mounting SPIFFS");
		while(1);
	}

	Serial.print("WiFi IP: ");
	Serial.println(WiFi.localIP());

    if (!MDNS.begin(MDNS_NAME))
    {
        Serial.println("Error starting mDNS");
    }
    else
    {
        Serial.println((String) "mDNS http://" + MDNS_NAME + ".local");
    }

	// Setup motors
	setupMotors();
	CameraSlider_EnableMotors(true);
	digitalWrite(PIN_LED, LOW);

    // Initialize Web server
	setupWebServer();
	server.begin();
    /*
    // Clear the display
    display.clearDisplay();
    display.setTextSize(2);      // Text size
    display.setCursor(0,0);      // Set cursor at the top-left
    display.print(F("Ready to go."));  // Constant message

    display.display();  // Refresh the display to show the message
*/
  // display Boot logo
  
  display.clearDisplay();  
  display.drawBitmap(0, 0, Dheu, 128, 64, 1);
  display.display();
  delay(2000);
  display.clearDisplay();  

  // display Boot logo
  display.drawBitmap(0, 0, KamiSlider, 128, 64, 1);
  display.display();
  delay(2000);
  display.clearDisplay();  

  display.drawBitmap(0, 0, DevelopedBy, 128, 64, 1);
  display.display();
  display.setTextSize(1);      // Text size
  display.setTextColor(SSD1306_WHITE);  // White text
  display.setCursor(32,20);
  display.print(F("  Aakash"));
  display.setCursor(32,34);
  display.print(F("Prithwijit"));
  display.setCursor(38,48);
  display.print(F("  Polo"));
  display.display();
  delay(2000);
  display.clearDisplay();  
/*
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("Developed By:");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,16);
  display.print("Aakash, ");
  display.println("Prithwijit, ");
  display.println("Polo");
  display.display();
  delay(2000);
  display.clearDisplay(); */
    // Debug message to signal we are initialized and entering loop
        // Clear the display
        display.clearDisplay();
        display.setTextSize(1);      // Text size
        display.setTextColor(SSD1306_WHITE);  // White text
        display.setCursor(0,0);      // Set cursor at the top-left
        display.print(F("Ready to go."));  // Constant message
    
        display.display();  // Refresh the display to show the message
	Serial.println("Ready to go.");
    // Initialize the Ticker for display updates every 500 milliseconds
    displayTicker.attach(0.5, updateDisplay);
    //displayTicker.attach(0.001, analogControl);
    CameraSlider_HomeSlidingRail();
}


void loop()
{
	while(1)
	{
        dnsServer.processNextRequest();
		CameraSlider_tick();
	}
}
