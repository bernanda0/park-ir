#include <HTTPClient.h>
#include <IRremote.hpp>
#include <WiFi.h>
#include <WiFiMulti.h>
 #include <freertos/FreeRTOS.h>
 #include <freertos/task.h>
#include <ESP32Servo.h>

#define IR_RECEIVE_PIN 25
#define LED_BUILTIN 2 // Replace with the actual built-in LED pin
#define MOTOR_PIN 4   // Replace with the actual pin controlling the DC motor
#define SOUND_SPEED 0.034

const char* ssid = "CogniSafe";
const char* password = "12345678";
const char* endpoint = "http://192.168.137.1:4444/plate/verify";
Servo myservo;  // create servo object to control a servo
int servoPin = 33;
const int trigPin = 32;
const int echoPin = 35;
long duration;
float distanceCm;
static TaskHandle_t xIRTaskHandle = NULL;
static TaskHandle_t xHTTPTaskHandle = NULL;
static TaskHandle_t xCallbackTaskHandle = NULL;
static TaskHandle_t xSensorTaskHandle = NULL;



String g_receivedID;
char receivedID[20];
const char location[120] = "Parkiran Digi";
bool readAgain = true;
int counter = 0;

WiFiMulti wifiMulti;

SemaphoreHandle_t httpSemaphore, receiverSemaphore;

void taskIRReceiver(void *pvParameters) {
  int delayInterval = 0;
  int pos;
  while (1) { 
    if(distanceCm <= 5){
      if (IrReceiver.decode() && readAgain) {
        if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
            Serial.printf("Received noise or an unknown protocol wait 1 seconds %d\n", counter++);
            delayInterval = 1;

        } else {
            sprintf(receivedID, "VID%08X", IrReceiver.decodedIRData.decodedRawData);
            Serial.printf("%s\n", receivedID);
            xSemaphoreGive(receiverSemaphore); 
            delayInterval = 1000;
        }
        IrReceiver.resume();
      }
    }else{
      xSemaphoreTake(receiverSemaphore, 0); 
      delay(delayInterval);
      vTaskResume(xHTTPTaskHandle);
      pos = myservo.read();
      if(pos > 90){
        pos = 0;
      }
    for (pos; pos >= 0; pos -= 1) { // goes from 0 degrees to 180 degrees
        // in steps of 1 degree
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(5);                       // waits 15 ms for the servo to reach the position
      }      //vTaskSuspend(xIRTaskHandle);
          }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void taskHTTPClient(void *pvParameters) {
  while (1) {
    if (wifiMulti.run() == WL_CONNECTED && xSemaphoreTake(receiverSemaphore, portMAX_DELAY) == pdTRUE) {
      WiFiClient client;
      HTTPClient http;

      if (http.begin(client, endpoint)) {
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        char payload[128];
        sprintf(payload, "v_id=%s&location=%s", receivedID, location);
        Serial.println(payload);
        readAgain = false;
        int httpResponseCode = http.POST(payload);

        if (httpResponseCode == 200) {
          Serial.println("Request Success");
          xSemaphoreGive(httpSemaphore); // Trigger other tasks          
          readAgain = false;
          vTaskSuspend(xHTTPTaskHandle);
        } else {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
          readAgain = true;
        }

        http.end();
      } else {
         Serial.println("Client failed!");
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void onRequestSuccess(void *pvParameters) {
  int pos;
  while (1) {
    if (xSemaphoreTake(httpSemaphore, portMAX_DELAY) == pdTRUE) {
      digitalWrite(LED_BUILTIN, HIGH); // Turn on the LED
      vTaskDelay(300 / portTICK_PERIOD_MS); // Keep the LED on for 1 second
      digitalWrite(LED_BUILTIN, LOW); // Turn off the LED

    for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
        // in steps of 1 degree
        myservo.write(pos);              // tell servo to go to position in variable 'pos'
        delay(5);                       // waits 15 ms for the servo to reach the position
      }      //vTaskSuspend(xIRTaskHandle);
      
      readAgain = true;
    }
    
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void vSensorTask(void *pvParam){
  while(1){
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculate the distance
    distanceCm = duration * SOUND_SPEED/2;
    Serial.println(distanceCm);
    vTaskDelay(pdMS_TO_TICKS(100)); //memberikan delay

  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(IR_RECEIVE_PIN, INPUT);
  myservo.setPeriodHertz(100);    // standard 50 hz servo
	myservo.attach(servoPin); 
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid, password);
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("Connected to WiFi in ");
  Serial.print(WiFi.localIP());
  Serial.println();
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  httpSemaphore = xSemaphoreCreateBinary();
  receiverSemaphore = xSemaphoreCreateBinary();
  
  xTaskCreatePinnedToCore(taskHTTPClient, "HTTPClient", 10000, NULL, 1, &xHTTPTaskHandle, 0);
  xTaskCreatePinnedToCore(taskIRReceiver, "IRReceiver", 10000, NULL, 1, &xIRTaskHandle, 0);
  xTaskCreatePinnedToCore(onRequestSuccess, "RequestSuccessCallback", 10000, NULL, 1, &xCallbackTaskHandle, 0);
  xTaskCreatePinnedToCore(vSensorTask, "SensorTask", 10000, NULL, 2, &xSensorTaskHandle, 1);


}

void loop() {}
