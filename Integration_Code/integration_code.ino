#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BMP180advanced.h>
#include <DHT.h>
#include <MAX30100_PulseOximeter.h>
#include <LiquidCrystal_I2C.h>
#include <TCS3200.h>

// OLED Display Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Sensor Definitions
#define MQ2_ANALOG_PIN PA6
#define TOUCH_PIN PA8       // Touch sensor
#define BUTTON_INC PB10     // Increment button
#define BUTTON_DEC PB1      // Decrement button
#define BUTTON_ENTER PB0    // Enter button
#define TRIG_PIN PA0        // Ultrasonic sensor trig pin
#define ECHO_PIN PA1        // Ultrasonic sensor echo pin
#define LDR_PIN PA3         // LDR sensor pin
#define VIBRATION_PIN PA2   // Vibration sensor pin
#define DHT_PIN PA4         // DHT sensor pin
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
#define S0 PB13
#define S1 PB14
#define S2 PB4
#define S3 PB3
#define sensorOut PB5


// BMP180 Sensor
BMP180advanced myBMP(BMP180_ULTRAHIGHRES);

// MAX30100 Sensor
PulseOximeter pox;
#define REPORTING_PERIOD_MS 1000
uint32_t tsLastReport = 0;

// Global Variables
float baselineValue = 0;
int selectedSensor = 0;         // Tracks the currently selected sensor
bool isSensorActivated = false; // Flag for sensor activation
bool showSensorData = false;    // Flag for data display
int redFrequency = 0, greenFrequency = 0, blueFrequency = 0;
// Define tolerance range for color detection
const int tolerance = 5; 

// Function Prototypes
void displayMessage(const char* message);
float getDistance();           // Ultrasonic sensor function
float getLightIntensity();     // LDR sensor function
void displayBMP180Data();      // BMP180 sensor function
void displayDHTData();         // DHT11 sensor function
void displayVibrationStatus(); // Vibration sensor function
void displayMAX30100Data();    // MAX30100 sensor function
void displayMQ2Data();  

void cascadingWaveAnimation() {
  for (int i = 0; i <= SCREEN_HEIGHT; i += 2) { // Draw lines in increments of 2 pixels
    display.clearDisplay();
    for (int j = 0; j <= i; j += 2) {
      display.drawLine(0, j, SCREEN_WIDTH, j, SSD1306_WHITE); // Draw a horizontal line
    }
    display.display();
    delay(50); // Delay between frames for smooth animation
  }

  for (int i = SCREEN_HEIGHT; i >= 0; i -= 2) { // Reverse the animation
    display.clearDisplay();
    for (int j = 0; j <= i; j += 2) {
      display.drawLine(0, j, SCREEN_WIDTH, j, SSD1306_WHITE);
    }
    display.display();
    display.clearDisplay();
    delay(50);
  }
}
void expandingRectangleAnimation() {    //Animation
  for (int i = 0; i <= SCREEN_HEIGHT / 2; i += 2) {
    display.clearDisplay();
    display.drawRect(i, i, SCREEN_WIDTH - 2 * i, SCREEN_HEIGHT - 2 * i, SSD1306_WHITE); // Draw expanding rectangle
    display.display();
    delay(50);
  }
}
void diagonalLineAnimation() {      //Animation
  for (int i = 0; i <= SCREEN_WIDTH; i += 4) {
    display.clearDisplay();
    display.drawLine(0, 0, i, SCREEN_HEIGHT, SSD1306_WHITE); // Draw diagonal lines from top-left
    display.drawLine(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH - i, 0, SSD1306_WHITE); // Draw diagonal lines from bottom-right
    display.display();
    delay(50);
  }
}

// Sensor Names
const char* sensorNames[] = {
  "Sensor Ultrasonic",
  "Sensor LDR",
  "Sensor BMP180",
  "Sensor DHT11",
  "Sensor Vibration",
  "Sensor MAX30100",
  "Sensor MQ2",
  "Sensor TCS3200" // Added TCS3200 sensor
};


// Callback function for Heartbeat detection on MAX30100
void onBeatDetected() {
  // This function is called when a heartbeat is detected
  Serial.println("Heartbeat detected!");
}

void setup() {
  Serial.begin(9600);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  cascadingWaveAnimation();
  expandingRectangleAnimation();
  diagonalLineAnimation();

  
  displayMessage("Initializing...");

  // Initialize Buttons
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUTTON_INC, INPUT_PULLUP);
  pinMode(BUTTON_DEC, INPUT_PULLUP);
  pinMode(BUTTON_ENTER, INPUT_PULLUP);
  
    pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);

  // Set frequency scaling to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Initialize Ultrasonic Sensor Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize Vibration Sensor Pin
  pinMode(VIBRATION_PIN, INPUT);

  // Initialize BMP180
  if (!myBMP.begin()) {
    displayMessage("BMP180 Fail");
    while (1);
  }
   // Initialize DHT Sensor
  dht.begin();

  // Initialize MAX30100 Sensor
  if (!pox.begin()) {
    displayMessage("MAX30100 Fail");
    while (1);
  }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  pox.setOnBeatDetectedCallback(onBeatDetected); // Set the callback for heartbeat detection

  displayMessage("Calibrating MQ2...");
  long sum = 0;
  for (int i = 0; i < 100; i++) {
    sum += analogRead(MQ2_ANALOG_PIN);
    delay(50);
  }
  baselineValue = sum / 100.0;
  displayMessage("MQ2 Calibration Done!");

  displayMessage("Ready!");
}

void loop() {
  // Check touch sensor to toggle activation
  if (digitalRead(TOUCH_PIN) == HIGH) {
    isSensorActivated = !isSensorActivated;
    delay(500); // Debounce delay

    if (isSensorActivated) {
      displayMessage("Sensors Active                            Press button to      select");
      selectedSensor = 0;
      showSensorData = false;
    } else {
      displayMessage("Sensors Deactivated");
      showSensorData = false;
    }
  }

  if (isSensorActivated) {
    // Handle Increment button
    if (digitalRead(BUTTON_INC) == LOW) {
      selectedSensor = (selectedSensor + 1) % 8; // Adjust for 6 sensors (including MAX30100)
      displayMessage(sensorNames[selectedSensor]);
      showSensorData = false;
      delay(300); // Debounce delay
    }

    // Handle Decrement button
    if (digitalRead(BUTTON_DEC) == LOW) {
      selectedSensor = (selectedSensor - 1 + 8) % 8; // Wrap-around logic
      displayMessage(sensorNames[selectedSensor]);
      showSensorData = false;
      delay(300); // Debounce delay
    }

    // Handle Enter button
    if (digitalRead(BUTTON_ENTER) == LOW) {
      showSensorData = true;
      delay(300); // Debounce delay
    }

    // Display data when Enter is pressed
    if (showSensorData) {
      switch (selectedSensor) {
        case 0:
          {
            float distance = getDistance();
            if (distance == -1) {
              displayMessage("Out of Range");
            } else {
              String message = "Distance: " + String(distance, 2) + "cm";
              displayMessage(message.c_str());
            }
          }
          break;
        case 1:
          {
            float lightIntensity = getLightIntensity();
            String message = "Light: " + String(lightIntensity, 1) + " %";
            displayMessage(message.c_str());
          }
          break;
        case 2:
          displayBMP180Data();
          break;
        case 3:
          displayDHTData();
          break;
        case 4:
          displayVibrationStatus();
          break;
        case 5:
          displayMAX30100Data();
          break;
          case 6:
          displayMQ2Data(); // MQ2 sensor case
          break;
          case 7:
          displayTCS3200Data(); // TCS3200 color sensor case
          break;
        default:
          displayMessage("Invalid Sensor");
      }
      delay(500); // Stabilize display updates
    }
  } else {
    displayMessage("Sensor Kit Deactive  Touch to Activate");

    delay(500);
  }
}

// Sensor Functions
float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 50000); // 50ms timeout
  if (duration == 0) return -1;
  float distance = duration * 0.034 / 2;
  return (distance >= 2 && distance <= 400) ? distance : -1;
}

float getLightIntensity() {
  int sensorValue = analogRead(LDR_PIN);
  return (sensorValue / 4095.0) * 100;
}

void displayBMP180Data() {
  float pressure = myBMP.getPressure_hPa();
  float seaLevelPressure = myBMP.getSeaLevelPressure_hPa(893);
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Pressure: ");
  display.print(pressure, 1);
  display.print(" hPa");
  display.setCursor(0, 10);
  display.print("Sea Level: ");
  display.print(seaLevelPressure, 1);
  display.print(" hPa");
  display.display();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pressure: ");
  lcd.print(pressure, 1);
  lcd.print("hPa");
  lcd.setCursor(0, 1);
  lcd.print("Sea Level: ");
  lcd.print(seaLevelPressure, 1);
  lcd.print("hPa");
}


void displayDHTData() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  if (isnan(temperature) || isnan(humidity)) {
    displayMessage("DHT11 Error!");
  } else {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(temperature, 1);
    display.print(" C");
    display.setCursor(0, 16);
    display.print("Humidity: ");
    display.print(humidity, 1);
    display.print(" %");
    display.display();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp ");
    lcd.print(temperature, 1);
    lcd.print(" C");
    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(humidity, 1);
    lcd.print("%");
}
  }

void displayVibrationStatus() {
  int vibrationState = digitalRead(VIBRATION_PIN);
  displayMessage(vibrationState == HIGH ? "Vibration: Detected" : "Vibration: None");
}

void displayMQ2Data() {
  int analogValue = analogRead(MQ2_ANALOG_PIN); // Read the analog output from MQ-2 sensor
  float adjustedValue = analogValue - baselineValue; // Subtract baseline
  float gasLevel = (adjustedValue / 4095.0) * 100; // Calculate gas level percentage
  float smokeLevel = (adjustedValue / 4095.0) * 200; // Scale smoke level differently

  // Ensure levels don't go negative if baseline is higher
  gasLevel = gasLevel < 0 ? 0 : gasLevel;
  smokeLevel = smokeLevel < 0 ? 0 : smokeLevel;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("Gas Level: ");
  display.print(gasLevel, 1); // Display gas level with 1 decimal place
  display.println("%");
  display.print("Smoke Level: ");
  display.print(smokeLevel, 1); // Display smoke level with 1 decimal place
  display.display();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gas Level: ");
  lcd.print(gasLevel,1);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Smoke: ");
  lcd.print(smokeLevel,1);
  lcd.print("%");
}

void displayTCS3200Data() {  // Read Red
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  redFrequency = pulseIn(sensorOut, LOW);

  // Read Green
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  greenFrequency = pulseIn(sensorOut, LOW);

  // Read Blue
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  blueFrequency = pulseIn(sensorOut, LOW);
  // Display RGB values on OLED
  display.clearDisplay();
  display.setCursor(0, 0);            // Start at the top left corner
  display.print("R: ");
  display.print(redFrequency);
  display.setCursor(0, 10);           // Move cursor to next line
  display.print("G: ");
  display.print(greenFrequency);
  display.setCursor(0, 20);           // Move cursor to next line
  display.print("B: ");
  display.print(blueFrequency);
  display.display(); 
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("R:");
  lcd.print(redFrequency);
  lcd.setCursor(7,0);
  lcd.print("G:");
  lcd.print(greenFrequency);
  lcd.setCursor(0,1);
  lcd.print("B:");
  lcd.print(blueFrequency);
   
  delay(1000); // Update every second
}


    void displayMAX30100Data(){
    pox.update();

    // Print and display the heart rate and SpO2 value
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        float heartRate = pox.getHeartRate();
        float spO2 = pox.getSpO2();
        // Display on the OLED
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("HR: ");

        display.print(heartRate);
        display.print(" bpm");

        display.setCursor(0, 16);
        display.print("SpO2: ");
        display.print(spO2);
        display.print(" %");
        display.display();

        tsLastReport = millis();
    }
    }

void displayMessage(const char* message) {  
  display.clearDisplay();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(message);
  lcd.setCursor(0, 0);
  lcd.print(message);
  display.display();
  delay(500);
}
