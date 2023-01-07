#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <driver/rtc_io.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

#define WIFI_SSID ""  //ENTER YOUR SSID HERE
#define WIFI_PASSWORD "" //ENTER YOUR PASSWORD HERE
#define API_KEY "" //ENTER API KEY HERE
#define STORAGE_BUCKET_ID "" //ENTER STORAGE BUCKET ID HERE
#define DATABASE_URL ""  //ENTER DATA BASE URL HERE
#define PATH "signal/holdPicture"

// Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;
bool signupOK = false;
String FILE_PHOTO = "/";
bool boolValue;
camera_fb_t *frame = nullptr;

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// HANDSHAKE
bool getHandshake()
{
  if (Firebase.ready())
  {
    if (Firebase.RTDB.getBool(&fbdo, "signal/Esp1"))
    {
      if (fbdo.dataType() == "boolean")
      {
        return fbdo.boolData();
      }
    }
    else
    {

      Serial.println(fbdo.errorReason());
    }
  }
  return false;
}

void setHandshake(String name, bool value)
{
  if (Firebase.ready())
  {
    String path = "signal/" + name;
    Serial.println(path);
    if (Firebase.RTDB.setBool(&fbdo, path, value))
    {
      Serial.println("SIGNAL PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {

      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

void setUploadPicture()
{
  if (Firebase.ready())
  {
    if (Firebase.RTDB.setString(&fbdo, "signal/uploadPicture", "NULL"))
    {
      Serial.println("NULL PASSED TO UPLOAD PICTURE");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}

// Timestamp
void getTimeStamp()
{
  if (Firebase.ready())
  {
    if (Firebase.RTDB.getString(&fbdo, "signal/path"))
    {
      if (fbdo.dataType() == "string")
      {
        FILE_PHOTO += fbdo.stringData();
        Serial.println(FILE_PHOTO);
      }
      else
      {

        Serial.println(fbdo.dataType());
      }
    }
    else
    {

      Serial.println(fbdo.errorReason());
    }
  }
}

// UPLOAD PICS
void UploadToStorageAndRTDB()
{
  if (Firebase.ready())
  {
    Serial.print("Uploading picture... ");
    if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO, mem_storage_type_flash, FILE_PHOTO + "/person.jpg", "image/jpeg"))
    {
      Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
      String dir = "data" + FILE_PHOTO + "/personPicture"; // Root Node
      String link = fbdo.downloadURL().c_str();
      if (Firebase.RTDB.setString(&fbdo, dir, link))
      {
        Serial.println("Link for Picture uploaded");
        Serial.println("PATH: " + fbdo.dataPath());
        Serial.println("TYPE: " + fbdo.dataType());
      }
      else
      {
        Serial.println("Failed to upload link of picture");
        Serial.println("REASON: " + fbdo.errorReason());
      }
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
}

// CapturePICS
void CapturePictureAndSaveToSPIFFS()
{
  Serial.println("Taking a photo...");

  // Photo file name
  File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);
  // Insert the data in the photo file
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
  }
  else
  {
    uint8_t *buffer = new uint8_t[frame->len];
    size_t len = frame->len;

    // Flip Photo
    for (int i = 0; i < len; i++)
    {
      buffer[(len - 1) - i] = frame->buf[i];
    }
    uint8_t *jpg_buf = (uint8_t *)heap_caps_malloc(200000, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    size_t jpg_size = 0;
    fmt2jpg(buffer, len, frame->width, frame->height, frame->format, 31, &jpg_buf, &jpg_size);
    file.write(jpg_buf, jpg_size); // payload (image), payload length
    Serial.print("The picture has been saved in ");
    Serial.print(FILE_PHOTO);
    Serial.print(" - Size: ");
    Serial.print(file.size());
    Serial.println(" bytes");
    delete[] buffer;
    buffer = nullptr;
  }
  // Close the file
  file.close();
}

// GET DATA FROM FIREBASE
bool CheckFirstSignal()
{
  if (Firebase.ready())
  {

    if (Firebase.RTDB.getBool(&fbdo, PATH))
    {
      if (fbdo.dataType() == "boolean")
      {
        boolValue = fbdo.boolData();
        Serial.println(boolValue);
        setHandshake("holdPicture", false);
        return boolValue;
      }
    }
    else
    {

      Serial.println(fbdo.errorReason());
    }
  }
  else
  {
    Serial.println("Firebase Fails");
  }
  return false;
}

bool CheckFinalSignal()
{
  if (Firebase.ready())
  {
    String flag;
    do
    {
      if (Firebase.RTDB.getString(&fbdo, "signal/uploadPicture"))
      {
        if (fbdo.dataType() == "string")
        {
          flag = fbdo.stringData();
          Serial.println(flag);
        }
      }
      else
      {
        Serial.println(fbdo.errorReason());
      }
      Serial.println("Waiting for final response from Esp1");
    } while ((flag != "no") &&
             (flag != "yes"));
    setUploadPicture();     // Clear the node with uploadPicture
    return (flag == "yes"); // This will tell us we need to upload the picture to real time database
  }
  else
  {
    Serial.println("Firebase Fails");
  }
  return false;
}

// INIT CAMERA
void initCamera()
{
  // OV2640 camera module
  // OV2640 camera module
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
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST; // VERY IMPORTANT FOR MOTION DETECTION

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }
}

// Init WIFI
void initWiFi()

{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
}

// INIT SPIFFS
void initSPIFFS()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  else
  {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
}

void setup()
{
  Serial.begin(115200);
  initSPIFFS();
  initWiFi();
  initCamera();
  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  //  Assign the api key
  configF.api_key = API_KEY;

  // Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  /* Assign the RTDB URL (required) */
  configF.database_url = DATABASE_URL;

  if (Firebase.signUp(&configF, &auth, "", ""))
  {
    Serial.println("Logged into Firebase");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", configF.signer.signupError.message.c_str());
  }

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Sent handshake to esp2");
  setHandshake("Esp2", true);
  Serial.print("Checking for handshake from esp1");
  while (!getHandshake())
  {
  }
  Serial.println("Handshake was successful");
  setHandshake("Esp1", false);
  setHandshake("Esp2", false);
  Serial.println("Esp1 and Esp2 handshake set to false");
}

void loop()
{
  frame = esp_camera_fb_get();
  if (!frame)
  {
    Serial.println("Camera capture failed");
    return;
  }
  delay(1000);
  if (CheckFirstSignal() && // CHECK FOR FIRST SIGNAL
      CheckFinalSignal())   // CHECK FOR FINAL SIGNAL
  {
    getTimeStamp();
    CapturePictureAndSaveToSPIFFS(); // EVERY 1 second capture a new photo
    UploadToStorageAndRTDB();
  }
  FILE_PHOTO.clear();
  FILE_PHOTO = "/";
  esp_camera_fb_return(frame);
  frame = esp_camera_fb_get();
  esp_camera_fb_return(frame);
}