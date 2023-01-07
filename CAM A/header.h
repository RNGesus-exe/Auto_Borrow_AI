#ifndef HEADER_H
#define HEADER_H

#include <LinkedList.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include <driver/rtc_io.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <time.h>

//ENTER YOUR CREDENTIALS IN THE BELOW STRINGS

#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define API_KEY ""
#define STORAGE_BUCKET_ID ""
#define DATABASE_URL ""

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

class Image : public LinkedList
{
    // Define NTP Server attributes
    const char *ntpServer = "1.pk.pool.ntp.org";
    const long gmtOffset_sec = 14400;
    const int daylightOffset_sec = 3600;
    String curr_time;

    // Image Credentials
    const int HEIGHT = 240;
    const int WIDTH = 320;
    const int PIXEL_THRESHOLD = 30;
    const int MOTION_THRESHOLD = 3000; // CHANGE TO 5000 WHEN IN UNIVERSITY
    const int CHUNK_THRESHOLD = 1500;

    // Rectangle Explosion factor
    const int RECT_FACTOR = 2; // SLOW AND MORE PRECISE
    const double CHUNK_PROBABILITY = 0.5;
    LinkedList skipCoordinates;

    // Define Firebase Data objects
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig configF;
    int iteration;
    uint8_t *tempBuffer;
    String path;

    // Buffer containers
    uint8_t **reference_frame;
    uint8_t **current_frame;

    String monthInDigit(char buffer[])
    {
        if (buffer[0] == 'J' &&
            buffer[1] == 'a' &&
            buffer[2] == 'n')
        {
            return "01";
        }
        else if (buffer[0] == 'F' &&
                 buffer[1] == 'e' &&
                 buffer[2] == 'b')
        {
            return "02";
        }
        else if (buffer[0] == 'M' &&
                 buffer[1] == 'a' &&
                 buffer[2] == 'r')
        {
            return "03";
        }
        else if (buffer[0] == 'A' &&
                 buffer[1] == 'p' &&
                 buffer[2] == 'r')
        {
            return "04";
        }
        else if (buffer[0] == 'M' &&
                 buffer[1] == 'a' &&
                 buffer[2] == 'y')
        {
            return "05";
        }
        else if (buffer[0] == 'J' &&
                 buffer[1] == 'u' &&
                 buffer[2] == 'n')
        {
            return "06";
        }
        else if (buffer[0] == 'J' &&
                 buffer[1] == 'u' &&
                 buffer[2] == 'l')
        {
            return "07";
        }
        else if (buffer[0] == 'A' &&
                 buffer[1] == 'u' &&
                 buffer[2] == 'g')
        {
            return "08";
        }
        else if (buffer[0] == 'S' &&
                 buffer[1] == 'e' &&
                 buffer[2] == 'p')
        {
            return "09";
        }
        else if (buffer[0] == 'O' &&
                 buffer[1] == 'c' &&
                 buffer[2] == 't')
        {
            return "10";
        }
        else if (buffer[0] == 'N' &&
                 buffer[1] == 'o' &&
                 buffer[2] == 'v')
        {
            return "11";
        }
        return "12"; // December Case
    }

    void acquireBuffer(uint8_t **&buf)
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
            return;
        }

        this->improveContrast(fb->buf);

        for (int i = 0; i < HEIGHT; i++)
        {
            for (int j = 0; j < WIDTH; j++)
            {
                buf[i][j] = fb->buf[i * WIDTH + j];
            }
        }

        esp_camera_fb_return(fb);
    }

    void deallocateBuffer(uint8_t **&buf)
    {
        if (buf)
        {
            for (int i = 0; i < HEIGHT; i++)
            {
                delete[] buf[i];
            }
            delete[] buf;
            buf = nullptr;
        }
        else
        {
            Serial.println("There was an error deallocating your buffer!!!");
        }
    }

    void deallocateBuffer(uint8_t *&buf)
    {
        if (buf)
        {
            delete[] buf;
            buf = nullptr;
        }
        else
        {
            Serial.println("There was an error deallocating your buffer!!!");
        }
    }

    void copyBuffer(uint8_t **&destination, uint8_t *&source)
    {
        if (destination && source)
        {
            for (int i = 0; i < HEIGHT; i++)
            {
                for (int j = 0; j < WIDTH; j++)
                {
                    destination[i][j] = source[i * WIDTH + j];
                }
            }
        }
        else
        {
            Serial.println("There was error copying buffer!!");
        }
    }

    void copyBuffer(uint8_t **&destination, uint8_t **&source)
    {
        if (destination && source)
        {
            for (int i = 0; i < HEIGHT; i++)
            {
                for (int j = 0; j < WIDTH; j++)
                {
                    destination[i][j] = source[i][j];
                }
            }
        }
        else
        {
            Serial.println("There was error copying buffer!!");
        }
    }

    void allocateBuffer(uint8_t **&buf)
    {
        if (!buf)
        {
            buf = new uint8_t *[HEIGHT];
            for (int i = 0; i < HEIGHT; i++)
            {
                buf[i] = new uint8_t[WIDTH];
            }
        }
        else
        {
            Serial.printf("\nThere was an error allocating memory on heap!!");
        }
    }

    void allocateBuffer(uint8_t *&buf, int height, int width)
    {
        if (!buf)
        {
            buf = new uint8_t[height * width];
        }
        else
        {
            Serial.printf("\nThere was an error allocating memory on heap!!");
        }
    }

    void improveContrast(uint8_t *&buf)
    {
        int min_val = INT32_MAX;
        int max_val = INT32_MIN;
        for (int i = 0; i < (this->HEIGHT * this->WIDTH); i++)
        {
            if ((int)buf[i] < min_val)
            {
                min_val = (int)buf[i];
            }
            if ((int)buf[i] > max_val)
            {
                max_val = (int)buf[i];
            }
        }
        // Serial.printf("MAX = %d , MIN = %d\n", max_val, min_val);
        for (int i = 0; i < (this->HEIGHT * this->WIDTH); i++)
        {
            buf[i] = ((int)buf[i] - min_val) * (255 / (max_val - min_val));
        }
    }

    void printFrameHistogram(int hist[])
    {
        for (int i = 0; i < 256; i++)
        {
            Serial.printf("%d,", hist[i]);
        }
        Serial.println();
    }

public:
    Image()
    {
        this->reference_frame = nullptr;
        this->current_frame = nullptr;
        this->iteration = 0;
        this->tempBuffer = nullptr;
    }

    void allocateCurrentBuffer()
    {
        this->allocateBuffer(this->current_frame);
    }

    void updateCurrentBuffer()
    {
        this->acquireBuffer(this->current_frame);
    }

    void uploadTimeStamp(String timestamp)
    {
        if (Firebase.ready())
        {
            // Write a boolean on the database path signal/Esp1
            if (Firebase.RTDB.setString(&fbdo, "signal/path", timestamp))
            {
                Serial.println("TimeStamp has been successfully uploaded\n");
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

    int fitRectangle(int &up, int &down, int &right, int &left, uint8_t **&prev_frame, uint8_t *&curr_frame)
    {
        // Perform 1 small explosion
        up -= 1;
        down += 1;
        left -= 1;
        right += 1;

        int prev_up;    // x1
        int prev_down;  // x2
        int prev_right; // y1
        int prev_left;  // y2
        while (((prev_up != up) ||
                (prev_down != down) ||
                (prev_left != left) ||
                (prev_right != right)))
        {
            // Save current in previous
            prev_up = up;
            prev_down = down;
            prev_left = left;
            prev_right = right;

            // DATA MEMBERS for the exploding rectangle algorithm
            int count = 0;
            bool go_up = false;
            bool go_down = false;
            bool go_left = false;
            bool go_right = false;

            // CHECK UP, LEFT, DOWN and RIGHT FOR EXPANSION

            // FIRST WE WILL CHECK THE UPPER OUTER REGION OF THE RECTANGLE
            count = 0;
            for (int index = left; index <= right; index++)
            {
                if (abs((int)prev_frame[up][index] - (int)curr_frame[up * WIDTH + index]) > PIXEL_THRESHOLD)
                {
                    count++;
                }
            }
            if (((double)(count * 100) / abs(right - left)) >= CHUNK_PROBABILITY)
            {
                go_up = true;
            }

            // FIRST WE WILL CHECK THE LEFT OUTER REGION OF THE RECTANGLE
            count = 0;
            for (int index = up; index <= down; index++)
            {
                if ((abs((int)prev_frame[index][left] - (int)curr_frame[index * WIDTH + left]) > PIXEL_THRESHOLD))
                {
                    count++;
                }
            }
            if (((double)(count * 100) / abs(down - up)) >= CHUNK_PROBABILITY)
            {
                go_left = true;
            }

            // THEN WE WILL CHECK THE BOTTOM OUTER REGION OF THE RECTANGLE
            count = 0;
            for (int index = left; index <= right; index++)
            {
                if (abs((int)prev_frame[down][index] - (int)curr_frame[down * WIDTH + index]) > PIXEL_THRESHOLD)
                {
                    count++;
                }
            }
            if (((double)(count * 100) / abs(right - left)) >= CHUNK_PROBABILITY)
            {
                go_down = true;
            }

            // THEN WE WILL CHECK THE RIGHT OUTER REGION OF THE RECTANGLE
            count = 0;
            for (int index = up; index <= down; index++)
            {
                if (abs((int)prev_frame[index][right] - (int)curr_frame[index * WIDTH + right]) > PIXEL_THRESHOLD)
                {
                    count++;
                }
            }
            if (((double)(count * 100) / abs(down - up)) >= CHUNK_PROBABILITY)
            {
                return true;
            }

            // PERFORM STRETCHING ACCORDING TO FLAGS
            if (go_up)
            {
                if ((up - RECT_FACTOR) < 0)
                {
                    up = 0;
                }
                else
                {
                    up -= RECT_FACTOR;
                }
            }
            if (go_down)
            {
                if ((down + RECT_FACTOR) >= HEIGHT)
                {
                    down = HEIGHT - 1;
                }
                else
                {
                    down += RECT_FACTOR;
                }
            }
            if (go_left)
            {
                if ((left - RECT_FACTOR) < 0)
                {
                    left = 0;
                }
                else
                {
                    left -= RECT_FACTOR;
                }
            }
            if (go_right)
            {
                if ((right + RECT_FACTOR) >= WIDTH)
                {
                    right = WIDTH - 1;
                }
                else
                {
                    right += RECT_FACTOR;
                }
            }
        }
        return (down - up) * (right - left);
    }

    void capturePhotoSaveSpiffs(int index, uint8_t *&buf, int height, int width)
    {
        // Take a photo with the camera
        Serial.println("Taking a photo...");
        // Photo file name
        this->path = "/" + this->curr_time + "/photo" + String(index) + ".jpg";
        File file = SPIFFS.open(path, FILE_WRITE);
        // Insert the data in the photo file
        if (!file)
        {
            Serial.println("Failed to open file in writing mode");
        }
        else
        {
            size_t jpg_size = 0;
            pixformat_t format = PIXFORMAT_GRAYSCALE;
            fmt2jpg(buf, width * height, width, height, format, 31, &buf, &jpg_size);
            file.write(buf, jpg_size); // payload (image), payload length
            // Serial.print("The picture has been saved in ");
            // Serial.print(path);
            // Serial.print(" - Size: ");
            // Serial.print(file.size());
            // Serial.println(" bytes");
        }
        file.close(); // Close the file
    }

    void advanceCopyBuffer(uint8_t *&destination, uint8_t **&source, int x1, int x2, int y1, int y2)
    {
        // Serial.printf("x1 = %d ,x2 = %d ,y1 = %d ,y2 = %d\n", x1, x2, y1, y2);
        for (int i = 0; i < (x2 - x1); i++)
        {
            for (int j = 0; j < (y2 - y1); j++)
            {
                // Serial.printf("Index = %d, Row = %d, Column = %d\n", (i * (y2 - y1) + j), (i + x1), (j + y1));
                destination[i * (y2 - y1) + j] = source[i + x1][j + y1];
            }
        }
    }

    void advanceCopyBuffer(uint8_t *&destination, uint8_t *&source, int x1, int x2, int y1, int y2)
    {
        // Serial.printf("x1 = %d ,x2 = %d ,y1 = %d ,y2 = %d\n", x1, x2, y1, y2);
        for (int i = 0; i < (x2 - x1); i++)
        {
            for (int j = 0; j < (y2 - y1); j++)
            {
                // Serial.printf("Index = %d, Row = %d, Column = %d\n", (i * (y2 - y1) + j), (i + x1), (j + y1));
                destination[i * (y2 - y1) + j] = source[(i + x1) * (this->WIDTH) + (j + y1)];
            }
        }
    }

    void uploadTimeAndStatus(bool issueReturn)
    {
        if (Firebase.ready()) // Upload the cropped frame to firebase
        {

            Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
            String dir = "data/" + this->curr_time; // Root Node
            String link = fbdo.downloadURL().c_str();
            // First upload Date & Time
            if (Firebase.RTDB.setString(&fbdo, dir + "/time", this->curr_time))
            {
                Serial.println("Time has been uploaded in RTDB");
                Serial.println("PATH: " + fbdo.dataPath());
                Serial.println("TYPE: " + fbdo.dataType());
            }
            else
            {
                Serial.println("Failed to upload time");
                Serial.println("REASON: " + fbdo.errorReason());
            }
            // Secondly upload status AKA Issue/Return
            if (Firebase.RTDB.setString(&fbdo, dir + "/status", (issueReturn) ? "issue" : "return"))
            {
                Serial.println("Status has been uploaded in RTDB");
                Serial.println("PATH: " + fbdo.dataPath());
                Serial.println("TYPE: " + fbdo.dataType());
            }
            else
            {
                Serial.println("Failed to upload status");
                Serial.println("REASON: " + fbdo.errorReason());
            }
        }
    }

    void uploadPicturesToFirebase(int picID)
    {
        if (Firebase.ready()) // Upload the cropped frame to firebase
        {
            Serial.print("Uploading picture... ");
            // MIME type should be valid to avoid the download problem.
            // The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
            Serial.printf("\nHeap: %d", ESP.getFreeHeap());
            if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, this->path, mem_storage_type_flash, this->path, "image/jpeg"))
            {
                Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str());
                String dir = "data/" + this->curr_time + "/pictures/picture" + String(picID); // Root Node
                String link = fbdo.downloadURL().c_str();
                // Upload firebase storage link to firebase
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

    void uploadPictures(camera_fb_t *fb)
    {
        // FIRST UPLOAD THE REFERENCE FRAME
        this->allocateBuffer(this->tempBuffer, HEIGHT, WIDTH);
        this->advanceCopyBuffer(this->tempBuffer, this->reference_frame, 0, HEIGHT, 0, WIDTH);
        this->capturePhotoSaveSpiffs(69, this->tempBuffer, HEIGHT, WIDTH);
        this->uploadPicturesToFirebase(69);
        this->deallocateBuffer(this->tempBuffer);
        SPIFFS.remove(this->path);

        // NOW UPLOAD THE CURRENT FRAME
        this->allocateBuffer(this->tempBuffer, HEIGHT, WIDTH);
        this->advanceCopyBuffer(this->tempBuffer, fb->buf, 0, HEIGHT, 0, WIDTH);
        this->capturePhotoSaveSpiffs(420, this->tempBuffer, HEIGHT, WIDTH);
        this->uploadPicturesToFirebase(420);
        this->deallocateBuffer(this->tempBuffer);
        SPIFFS.remove(this->path);
    }

    bool decideIssueOrReturn(uint8_t **&prev, uint8_t *&curr, int up, int down, int left, int right)
    {
        // THIS WILL ONLY WORK FOR A WHITE BACKGROUND
        // True = ISSUE && False = RETURN
        int prev_count = 0; // Count zeros in previous buffer
        int curr_count = 0; // Count zeros in current buffer
        for (int i = up; i < down; i++)
        {
            for (int j = left; j < right; j++)
            {
                if ((int)prev[i][j] < 10)
                {
                    prev_count++;
                }
                if ((int)curr[i * this->WIDTH + j] < 10)
                {
                    curr_count++;
                }
            }
        }
        Serial.printf("prev = %d, curr = %d\n", prev_count, curr_count);
        if (prev_count > curr_count)
        {
            Serial.println("An item was issued!!");
            return true;
        }
        Serial.println("An item was returned!!");
        return false;
    }

    void uploadTotalItemsToFirebase(int totalItems)
    {
        if (Firebase.ready()) // Upload the cropped frame to firebase
        {
            Serial.println("Uploading totalItems to Firebase... ");

            String dir = "data/" + this->curr_time + "/pictures/totalItems"; // Root Node
            String link = fbdo.downloadURL().c_str();
            // Upload firebase storage link to firebase
            if (Firebase.RTDB.setInt(&fbdo, dir, totalItems))
            {
                Serial.println("Successfully uploaded totalItems to Firebase");
                Serial.println("PATH: " + fbdo.dataPath());
                Serial.println("TYPE: " + fbdo.dataType());
            }
            else
            {
                Serial.println("Failed to upload totalItems to firebase");
                Serial.println("REASON: " + fbdo.errorReason());
            }
        }
    }

    void solve()
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
        }
        else
        {
            this->FetchTime(); // Get the current time from the NTP server
            this->improveContrast(fb->buf);
            // this->iteration++; // Keeping track of the amount of time the algo ran
            // uploadPictures(fb);
            int up = 0;               // Max height
            int down = 0;             // Min height
            int left = 0;             // Min Width
            int right = 0;            // Max Width
            int count = 0;            // To keep track of how many pictures were uploaded to firebase
            bool finalSignal = true;  // To make sure signal is sent only once
            bool issueReturn = false; // 1 : Issue && 0 : Return
            for (int i = 1; i < (HEIGHT - 1); i++)
            {
                for (int j = 1; j < (WIDTH - 1); j++)
                {
                    if (((abs((int)this->reference_frame[i][j] - (int)fb->buf[i * fb->width + j]) > PIXEL_THRESHOLD) &&
                         ((abs((int)this->reference_frame[i - 1][j] - (int)fb->buf[(i - 1) * fb->width + j]) > PIXEL_THRESHOLD) ||
                          (abs((int)this->reference_frame[i + 1][j] - (int)fb->buf[(i + 1) * fb->width + j]) > PIXEL_THRESHOLD) ||
                          (abs((int)this->reference_frame[i][j - 1] - (int)fb->buf[i * fb->width + (j - 1)]) > PIXEL_THRESHOLD) ||
                          (abs((int)this->reference_frame[i][j + 1] - (int)fb->buf[i * fb->width + (j + 1)] > PIXEL_THRESHOLD)))) &&
                        this->skipCoordinates.isNotExplored(i, j))
                    {
                        up = i;    // Max height
                        down = i;  // Min height
                        left = j;  // Min Width
                        right = j; // Max Width
                        if (fitRectangle(up, down, right, left, this->reference_frame, fb->buf) > CHUNK_THRESHOLD)
                        {
                            // Run the AI to get the item frames + upload the frames
                            count++;
                            this->skipCoordinates.insertNode(up, down, left, right);             // We will store the coordinates
                            this->allocateBuffer(this->tempBuffer, (down - up), (right - left)); // Allocate buffer for the temp buffer
                            // DECIDE WHICH FRAME TO TAKE (ISSUE/RETURN)
                            if (decideIssueOrReturn(this->reference_frame, fb->buf, up, down, left, right))
                            {
                                this->advanceCopyBuffer(this->tempBuffer, this->reference_frame, up, down, left, right);
                                issueReturn = true;
                            }
                            else
                            {
                                this->advanceCopyBuffer(this->tempBuffer, fb->buf, up, down, left, right);
                                issueReturn = false;
                            }
                            // Give the final signal to the firebase (ONCE)
                            if (finalSignal)
                            {
                                this->FetchTime();                      // Get time from NTP server
                                this->uploadTimeStamp(this->curr_time); // Upload the time path for ESP 2
                                this->sendFinalSignal("yes");           // Send a final signal to ESP 2
                                this->uploadTimeAndStatus(issueReturn); // To make the root node for the picture to be uploaded in
                                finalSignal = false;                    // To make sure signal is not sent again
                            }
                            this->capturePhotoSaveSpiffs(count, this->tempBuffer, (down - up), (right - left)); // Save the photo in SPIFFS to upload in Firebase later on
                            this->deallocateBuffer(this->tempBuffer);                                           // Deallocate memory
                            this->uploadPicturesToFirebase(count);                                              // This will upload the pictures one by one to firebase
                        }
                    }
                }
            }
            if (finalSignal &&
                (!count))
            {
                this->sendFinalSignal("no");
            }
            else
            {
                this->uploadTotalItemsToFirebase(count);
                Serial.printf("Amount of items detected = %d\n", count);
            }
            // this->skipCoordinates.printNodes();
            this->skipCoordinates.empty();
            this->copyBuffer(this->reference_frame, fb->buf);
        }
        esp_camera_fb_return(fb);
        fb = esp_camera_fb_get();
        esp_camera_fb_return(fb); // DISCARD SECOND BUFFER
        Serial.println("Algorithm Ended....");
    }

    void moveCurrentToReference()
    {
        if (this->reference_frame && this->current_frame)
        {
            copyBuffer(this->reference_frame, this->current_frame);
        }
        else
        {
            Serial.println("There was an error copying the data");
        }
    }

    void deallocateCurrentFrame()
    {
        this->deallocateBuffer(this->current_frame);
    }

    void allocateReferenceFrame()
    {
        allocateBuffer(this->reference_frame);
    }

    void updateReferenceFrame()
    {
        acquireBuffer(this->reference_frame);
    }

    void sendSignal()
    {
        if (Firebase.ready())
        {
            // Write a boolean on the database path signal/takePicture
            if (Firebase.RTDB.setBool(&fbdo, "signal/holdPicture", true))
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

    void sendFinalSignal(String flag)
    {
        if (Firebase.ready())
        {
            // Write a boolean on the database path signal/takePicture
            if (Firebase.RTDB.setString(&fbdo, "signal/uploadPicture", flag))
            {
                Serial.println("FINAL SIGNAL PASSED");
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

    bool isMotion()
    {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            Serial.println("Camera capture failed");
            return false;
        }
        this->improveContrast(fb->buf); // Improve the contrast for the current buffer
        long count = 0;
        for (int i = 1; i < (HEIGHT - 1); i++)
        {
            for (int j = 1; j < (WIDTH - 1); j++)
            {
                if ((abs((int)this->reference_frame[i][j] - (int)fb->buf[i * fb->width + j]) > PIXEL_THRESHOLD) &&
                    ((abs((int)this->reference_frame[i - 1][j] - (int)fb->buf[(i - 1) * fb->width + j]) > PIXEL_THRESHOLD) ||
                     (abs((int)this->reference_frame[i + 1][j] - (int)fb->buf[(i + 1) * fb->width + j]) > PIXEL_THRESHOLD) ||
                     (abs((int)this->reference_frame[i][j - 1] - (int)fb->buf[i * fb->width + (j - 1)]) > PIXEL_THRESHOLD) ||
                     (abs((int)this->reference_frame[i][j + 1] - (int)fb->buf[i * fb->width + (j + 1)] > PIXEL_THRESHOLD))))
                {
                    count++;
                }
            }
        }
        Serial.printf("Change in pixels = %d\n", count);
        if (count >= MOTION_THRESHOLD)
        {
            Serial.println("Motion was detected!!!");
            esp_camera_fb_return(fb);
            fb = esp_camera_fb_get();
            esp_camera_fb_return(fb); // DISCARD THE SECOND BUFFER
            return true;
        }
        Serial.println("No motion was detected!!!");
        esp_camera_fb_return(fb);
        fb = esp_camera_fb_get();
        esp_camera_fb_return(fb); // DISCARD THE SECOND BUFFER
        return false;
    }

    String getCurrentTime() const
    {
        return this->curr_time;
    }

    void FetchTime()
    {
        struct tm timeinfo;
        while (!getLocalTime(&timeinfo))
        {
            Serial.println("Failed to obtain time");
        }
        this->curr_time = "YYYYMMDDTHHMMSS";
        char buffer[5];
        strftime(buffer, 3, "%d", &timeinfo); // DAY
        this->curr_time[6] = buffer[0];
        this->curr_time[7] = buffer[1];
        buffer[0] = buffer[1] = buffer[2] = '\0';
        strftime(buffer, 4, "%B", &timeinfo); // MONTH
        String temp = this->monthInDigit(buffer);
        this->curr_time[4] = temp[0];
        this->curr_time[5] = temp[1];
        buffer[0] = buffer[1] = buffer[2] = buffer[3] = '\0';
        strftime(buffer, 5, "%Y", &timeinfo);
        this->curr_time[0] = buffer[0];
        this->curr_time[1] = buffer[1];
        this->curr_time[2] = buffer[2];
        this->curr_time[3] = buffer[3];
        buffer[0] = buffer[1] = buffer[2] = buffer[3] = buffer[4] = '\0';
        strftime(buffer, 3, "%I", &timeinfo);
        this->curr_time[9] = buffer[0];
        this->curr_time[10] = buffer[1];
        buffer[0] = buffer[1] = buffer[2] = '\0';
        strftime(buffer, 3, "%M", &timeinfo);
        this->curr_time[11] = buffer[0];
        this->curr_time[12] = buffer[1];
        buffer[0] = buffer[1] = buffer[2] = '\0';
        strftime(buffer, 3, "%S", &timeinfo);
        this->curr_time[13] = buffer[0];
        this->curr_time[14] = buffer[1];
        buffer[0] = buffer[1] = buffer[2] = '\0';
    }

    void initNTPServer()
    {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        FetchTime();
        Serial.println(getCurrentTime());
    }

    void confirmHandshake()
    {
        Serial.println("Checking for handshake.");
        while (!this->checkHandshake())
        {
            Serial.print(".");
            delay(500);
        }
        Serial.println("Handshake was successful!");
    }

    bool checkHandshake()
    {
        if (Firebase.ready())
        {
            // Write a boolean on the database path signal/Esp1
            if (Firebase.RTDB.getBool(&fbdo, "signal/Esp2") && (fbdo.dataType() == "boolean"))
            {
                Serial.println("Handshake status attained from esp2");
                Serial.println("PATH: " + fbdo.dataPath());
                Serial.println("TYPE: " + fbdo.dataType());
                return fbdo.boolData();
            }
            else
            {
                Serial.println("FAILED");
                Serial.println("REASON: " + fbdo.errorReason());
            }
        }
        return false;
    }

    void sendHandshake()
    {
        if (Firebase.ready())
        {
            // Write a boolean on the database path signal/Esp1
            if (Firebase.RTDB.setBool(&fbdo, "signal/Esp1", true))
            {
                Serial.println("HandShake sent to ESP2");
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

    void initFirebase()
    {
        //  Assign the api key
        configF.api_key = API_KEY;

        // Assign the callback function for the long running token generation task
        configF.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

        /* Assign the RTDB URL (required) */
        configF.database_url = DATABASE_URL;

        if (Firebase.signUp(&configF, &auth, "", ""))
        {
            Serial.println("Logged into Firebase");
        }
        else
        {
            Serial.printf("%s\n", configF.signer.signupError.message.c_str());
        }

        Firebase.begin(&configF, &auth);
        Firebase.reconnectWiFi(true);
    }

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

    void initWiFi()
    {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.println("Connecting to WiFi...");
        }
    }

    void initCamera()
    {
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
};

#endif
