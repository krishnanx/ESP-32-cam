#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include "camera_pins.h"
const char* ssid = "EASWAR MANI";
const char* password = "easwer123";
const int PIN_TO_SENSOR = 13; // GPIO13 pin connected to OUTPUT pin of sensor
int pinStateCurrent   = LOW;  // current state of pin
int pinStatePrevious  = LOW;  // previous state of pin
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Initialize Telegram BOT
String BOTtoken = "6277596652:AAHxuXdY1MKSjeOZYT0f7iG38APqjcMmQGs";  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
String CHAT_ID = "1481949434";

bool sendPhoto = false;

WiFiClientSecure clientTCP;
UniversalTelegramBot bot(BOTtoken, clientTCP);

#define FLASH_LED_PIN 4
bool flashState = LOW;

//Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

bool motionState = LOW;

void configInitCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
   //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 8;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 8;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  // Drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
}

void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);
    
    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      String welcome = "Welcome , " + from_name + "\n";
      welcome += "Use the following commands to interact with the ESP32-CAM \n";
      welcome += "False trigger may occur, dont worry:)\n";
      welcome += "/motionon : turns on motion detection\n";
      welcome += "/motionoff : turns off motion detection\n";
      welcome += "/photo : takes a new photo\n";
      welcome += "/flash : toggles flash LED \n";
      welcome += "/status : shows status\n";
      bot.sendMessage(CHAT_ID, welcome, "");
    }
    if (text == "/motionon"){
      motionState = HIGH; 
      bot.sendMessage(CHAT_ID,"MOTION_DETECTION ON", "");
    }
    if (text == "/motionoff"){
      motionState = LOW;
      bot.sendMessage(CHAT_ID,"MOTION_DETECTION off", "");
    }
    if (text == "/flash") {
      if (flashState = !flashState){
        digitalWrite(FLASH_LED_PIN, flashState);
        bot.sendMessage(CHAT_ID,"FLASH ON", "");}
      else{
        digitalWrite(FLASH_LED_PIN, flashState);
        bot.sendMessage(CHAT_ID,"FLASH OFF", "");
      }
    }
    if (text == "/photo") {
      sendPhoto = true;
      //Serial.println("New photo request");
      String msg = "Please bear with us as we get you a photo:)\n";
      msg += "it could take a few seconds";
      msg += "dont spam /photo :)\n";
      msg += "try again in 5 sec if you dont recieve\n";
      bot.sendMessage(CHAT_ID,msg, "");
      
    }
    if (text == "/status"){
      if (flashState == HIGH){
        bot.sendMessage(CHAT_ID,"FLASH:ON", "");
      }
      if (flashState != HIGH){
        bot.sendMessage(CHAT_ID,"FLASH:OFF", "");
      }
      if (motionState == HIGH){
        bot.sendMessage(CHAT_ID,"MOTION_DETECTION:ON","");
      }
      if (motionState != HIGH){
        bot.sendMessage(CHAT_ID,"MOTION_DETECTION:OFF","");
      }
    }
  }
}


String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";
  digitalWrite(FLASH_LED_PIN,HIGH);
  // Clear the frame buffer before capturing a new image
  camera_fb_t * fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);
  fb = esp_camera_fb_get(); 
  if(!fb) {
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    ESP.restart();
    return "camera capture failed";
  }
  delay(500);
  digitalWrite(FLASH_LED_PIN,LOW);
  
  Serial.println("Connect to " + String(myDomain));


  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    
    String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + CHAT_ID + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--RandomNerdTutorials--\r\n";

    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
    delay(2000);
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
    clientTCP.println();
    clientTCP.print(head);
    delay(500);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }   
    clientTCP.print(tail);
    Serial.println("photo sent");
    esp_camera_fb_return(fb);
    
    int waitTime = 2000;   // timeout 10 seconds
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
}


void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  pinMode(PIN_TO_SENSOR,INPUT); // set ESP32 pin to input mode to read value from OUTPUT pin of sensor
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, flashState);

  // Config and init the camera
  configInitCamera();

  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
  WiFi.setSleep(false);
  uint32_t notConnectedCounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    notConnectedCounter++;
    if(notConnectedCounter > 150) { // Reset board if not connected after 15s
        Serial.println("Resetting due to Wifi not connecting...");
        ESP.restart();
    }
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP()); 
  // startCameraServer();

}

void loop() {
  if (motionState == HIGH){
    pinStateCurrent = digitalRead(PIN_TO_SENSOR);   // read new state
    // Serial.println(pinStateCurrent);
    // Serial.println(pinStatePrevious);
    if (pinStateCurrent == HIGH) {   // pin state change: LOW -> HIGH
      Serial.println("Motion detected!");
      bot.sendMessage(CHAT_ID,"HEY!!! MOTION DETECTED!!","");
      bot.sendMessage(CHAT_ID,"sending picture right now!","");
      // TODO: turn on alarm, light or activate a device ... here
      sendPhotoTelegram();
      delay(500);
    }
    if ( pinStateCurrent == LOW){
      Serial.println("No motion");
    }
  }
  if (sendPhoto) {
    Serial.println("Preparing photo");
    sendPhotoTelegram(); 
    sendPhoto = false; 
    
  }
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  delay(750);
}
