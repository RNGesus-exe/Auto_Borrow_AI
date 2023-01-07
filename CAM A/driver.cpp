#include <header.h>

Image Sudo;

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);

  Sudo.initWiFi(); // Startup the WiFi

  Sudo.initSPIFFS(); // Startup SPIFFS

  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Sudo.initCamera(); // Startup the camera

  Sudo.initFirebase(); // Startup Firebase

  Sudo.initNTPServer(); // Startup the NTP Server

  // Now we start the handshake protocol with ESP_CAM_2

  Sudo.sendHandshake(); // First we confirm a handshake from our side

  Sudo.confirmHandshake(); // Secondly we wait for the handshake from ESP_CAM_2

  Serial.println("System has started....");
  Sudo.allocateReferenceFrame(); // Allocate Memory for previous buffer
  Sudo.updateReferenceFrame();   // Take a picture and store it in previous buffer
  // Serial.printf("\nHeap: %d", ESP.getFreeHeap());
}

void loop()
{
  if (Sudo.isMotion()) // We wait for motion to be detected
  {
    Sudo.sendSignal(); // We now send a signal to ESP2 to save the picture and wait for confimation signal
    delay(5000);       // PRAY THAT ITEM GETS PICKED UP WITHIN 3 seconds
    // TO DO: Add an algo to detect when motion stops
    Sudo.solve(); // This will be the main spine of our project
  }
}