#include <Arduino.h>

// WIFI
#include <WiFi.h>

// SD CARD
#include <SPI.h>
#include <SD.h>

SPIClass spi = SPIClass(2);

// HTTP
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// SCREEN
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>
#include <LittleFS.h>

#define E 18
#define B1 27
#define B2 13
#define G1 26
#define G2 12
#define R1 25
#define R2 14

#define ALT 2
#define ENABLE_DOUBLE_BUFFER 1

uint8_t matrix_brightness = 60;

File gif_file;
AnimatedGIF gif;
MatrixPanel_I2S_DMA *matrix_display = nullptr;

TaskHandle_t Task1;

String th_filePath = "";
bool allowPlaying = true;
bool isPlayable = false;

void playGif(String filePath) {
  th_filePath = filePath;
  isPlayable = true;
  allowPlaying = false;
}

void stopGif() {
  isPlayable = false;
  allowPlaying = false;
}

void initWiFi()
{
    WiFi.begin("_d123", "NoWieszTyCo__GG*&^%");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - start > 30000)
        {
            Serial.println("Failed to connect to WiFi");
            return;
        }
        delay(500);
    }
    Serial.println("Network: OK, IP: " + WiFi.localIP().toString());
}

bool initSdCard() {
    spi.begin(33, 32, 21, 22);

    if (SD.begin(22, spi, 8000000) && SD.cardType() != CARD_NONE)
    {
        uint64_t sizeGB = SD.cardSize() / (1024 * 1024 * 1024);
        Serial.printf("SD Card: OK, %llu GB\n", sizeGB);

        return true;
    }

    Serial.println("SD Card: FAIL");
    return false;
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
    if (type == WS_EVT_DATA) {
        char* message = new char[len + 1];
        memcpy(message, data, len);
        message[len] = '\0';

        char* command = strtok(message, " ");

        if (strcmp(command, "help") == 0) {
            client->text("{\"message\":\"Available commands: help\",\"isError\":false}");
        } else if (strcmp(command, "play") == 0) {
            char* path = strtok(NULL, " ");
            playGif(path);
            client->text("{\"message\":\"Playing GIF\",\"isError\":false}");
        } else if (strcmp(command, "stop") == 0) {
            stopGif();
            client->text("{\"message\":\"Stopping GIF\",\"isError\":false}");
        } else {
            client->text("{\"message\":\"Unknown command\",\"isError\":true}");
        }

        delete[] message;
    }
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index) {
        String path = "/";

        if (request->hasParam("path", true)) {
            path = request->getParam("path", true)->value();

            if (!path.startsWith("/")) {
                path = "/" + path;
            }

            if (!path.endsWith("/")) {
                path += "/";
            }
        }

        logmessage = "Upload Start: " + String(path + filename);
        request->_tempFile = SD.open(path + filename, "w");
        Serial.println(logmessage);
    }

    if (len) {
        request->_tempFile.write(data, len);
        logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
        Serial.println(logmessage);
    }

    if (final) {
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
        request->_tempFile.close();
        Serial.println(logmessage);
    }
}












const int panelResX = 128;
const int panelResY = 32;
const int panels_in_X_chain = 1;
const int panels_in_Y_chain = 1;
const int totalWidth = panelResX * panels_in_X_chain;
const int totalHeight = panelResY * panels_in_Y_chain;
int16_t xPos = 0, yPos = 0;




void *GIFOpenFile(const char *fname, int32_t *pSize) {
  Serial.print("Playing gif: ");
  Serial.println(fname);
  gif_file = LittleFS.open(fname);

  if (gif_file) {
    *pSize = gif_file.size();
    return (void *)&gif_file;
  }

  return NULL;
}

void *GIFSDOpenFile(const char *fname, int32_t *pSize) {
  Serial.print("Playing gif from SD: ");
  Serial.println(fname);
  gif_file = SD.open(fname);

  if (gif_file) {
    *pSize = gif_file.size();
    return (void *)&gif_file;
  }

  return NULL;
}

void GIFCloseFile(void *pHandle) {
  File *gif_file = static_cast<File *>(pHandle);

  if (gif_file != NULL) {
    gif_file->close();
  }
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *gif_file = static_cast<File *>(pFile->fHandle);

  if ((pFile->iSize - pFile->iPos) < iLen) {
    iBytesRead = pFile->iSize - pFile->iPos - 1;
  }

  if (iBytesRead <= 0) {
    return 0;
  }

  iBytesRead = (int32_t)gif_file->read(pBuf, iBytesRead);
  pFile->iPos = gif_file->position();
  return iBytesRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  int i = micros();
  File *gif_file = static_cast<File *>(pFile->fHandle);
  gif_file->seek(iPosition);
  pFile->iPos = (int32_t)gif_file->position();
  i = micros() - i;
  return pFile->iPos;
}

void span(uint16_t *src, int16_t x, int16_t y, int16_t width) {
  if (x >= totalWidth) {
    return;
  }

  int16_t x2 = x + width - 1;

  if (x2 < 0) {
    return;
  }

  if (x < 0) {
    width += x;
    src -= x;
    x = 0;
  }

  if (x2 >= totalWidth) {
    width -= (x2 - totalWidth + 1);
  }

  while (x <= x2) {
    int16_t xOffset = (totalWidth - gif.getCanvasWidth()) / 2;
    matrix_display->drawPixel((x++) + xOffset, y, *src++);
  }
}

void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y;

  y = pDraw->iY + pDraw->y;

  int16_t screenY = yPos + y;

  if ((screenY < 0) || (screenY >= totalHeight)) {
    return;
  }

  usPalette = pDraw->pPalette;

  s = pDraw->pPixels;

  if (pDraw->ucHasTransparency) {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + pDraw->iWidth;
    x = 0;
    iCount = 0;
    while (x < pDraw->iWidth) {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent) {
          s--;
        } else {
          *d++ = usPalette[c];
          iCount++;
        }
      }
      if (iCount) {
        span(usTemp, xPos + pDraw->iX + x, screenY, iCount);
        x += iCount;
        iCount = 0;
      }

      c = ucTransparent;
      while (c == ucTransparent && s < pEnd) {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--;
      }
      if (iCount) {
        x += iCount;
        iCount = 0;
      }
    }
  } else {
    s = pDraw->pPixels;

    for (x = 0; x < pDraw->iWidth; x++) {
      usTemp[x] = usPalette[*s++];
    }

    span(usTemp, xPos + pDraw->iX, screenY, pDraw->iWidth);
  }
}

void showGIF(const char *name) {
  matrix_display->setBrightness8(matrix_brightness);
  matrix_display->clearScreen();

  if (gif.open(name, GIFSDOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    GIFINFO pInfo;
    gif.getInfo(&pInfo);
    Serial.printf("[INFO] Showing %s GIF for %d seconds\n", name, pInfo.iDuration / 1000);
    int i = 0;
    do {
      i++;
    } while (allowPlaying && gif.playFrame(true, NULL));
    if (!allowPlaying) {
      allowPlaying = true;
    }
    gif.close();
  }
}

void GifThread(void *parameter) {
  while (true) {
    if (
      isPlayable && (strncmp(th_filePath.c_str(), "/", 1) == 0) && (strlen(th_filePath.c_str()) != 0) && SD.exists(th_filePath)) {
      showGIF(th_filePath.c_str());
      continue;
    }
    delay(1);
  }
}

void setup()
{
    // SERIAL
    Serial.begin(115200);

    initWiFi();

    // get current time from NTP server, for timezone of Europe/Warsaw
    configTime(3600, 0, "pool.ntp.org", "time.nist.gov");

    initSdCard();

    // HTTP
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Get info about ESP32
    server.on("/info", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";

        json += "\"cores\":" + String(ESP.getChipCores()) + ",";
        json += "\"chipModel\":\"" + String(ESP.getChipModel()) + "\",";
        json += "\"chipRevision\":" + String(ESP.getChipRevision()) + ",";
        json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
        json += "\"cycleCount\":" + String(ESP.getCycleCount()) + ",";
        json += "\"efuseMac\":\"" + String((uint32_t)(ESP.getEfuseMac()>>32), HEX) + String((uint32_t)ESP.getEfuseMac(), HEX) + "\",";
        json += "\"flashChipMode\":" + String(ESP.getFlashChipMode()) + ",";
        json += "\"flashChipSize\":" + String(ESP.getFlashChipSize()) + ",";
        json += "\"flashChipSpeed\":" + String(ESP.getFlashChipSpeed()) + ",";
        json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
        json += "\"freePsram\":" + String(ESP.getFreePsram()) + ",";
        json += "\"freeSketchSpace\":" + String(ESP.getFreeSketchSpace()) + ",";
        json += "\"heapSize\":" + String(ESP.getHeapSize()) + ",";
        json += "\"maxAllocHeap\":" + String(ESP.getMaxAllocHeap()) + ",";
        json += "\"maxAllocPsram\":" + String(ESP.getMaxAllocPsram()) + ",";
        json += "\"minFreeHeap\":" + String(ESP.getMinFreeHeap()) + ",";
        json += "\"minFreePsram\":" + String(ESP.getMinFreePsram()) + ",";
        json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\",";
        json += "\"sketchMD5\":\"" + String(ESP.getSketchMD5()) + "\",";
        json += "\"sketchSize\":" + String(ESP.getSketchSize());

        json += "}";

        request->send(200, "application/json", json);
    });

    // Get list of files in specified path
    server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request) {
        String path = "/";

        if (request->hasParam("path")) {
            path = request->getParam("path")->value();
        }

        File dir = SD.open(path);

        if (!dir) {
            request->send(500, "text/plain", "Failed to open directory");
            return;
        }

        if (!dir.isDirectory()) {
            request->send(400, "text/plain", "Path is not a directory");
            return;
        }

        String json = "[";
        File file = dir.openNextFile();
        while (file) {
            if (json != "[") json += ",";
            json += "{\"name\":\"" + String(file.name()) + "\",\"type\":\"";
            if (file.isDirectory()) {
                json += "directory";
            } else {
                json += "file";
            }
            json += "\"}";
            file = dir.openNextFile();
        }
        json += "]";
        request->send(200, "application/json", json);
        dir.close();
    });

    // Create dir in specified path
    server.on("/mkdir", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, "text/plain", "Missing 'path' parameter");
            return;
        }

        String path = request->getParam("path")->value();

        if (SD.exists(path)) {
            request->send(200, "text/plain", "Directory already exists");
            return;
        }

        if (SD.mkdir(path)) {
            request->send(200, "text/plain", "Directory created successfully");
        } else {
            request->send(500, "text/plain", "Directory creation failed");
        }
    });

    // Delete file or directory
    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, "text/plain", "Missing 'path' parameter");
            return;
        }

        String path = request->getParam("path")->value();

        if (!SD.exists(path)) {
            request->send(200, "text/plain", "File or directory does not exist");
            return;
        }

        File file = SD.open(path);

        if (file.isDirectory()) {
            if (SD.rmdir(path)) {
                request->send(200, "text/plain", "Directory deleted successfully");
            } else {
                request->send(500, "text/plain", "Directory deletion failed");
            }
        } else {
            if (SD.remove(path)) {
                request->send(200, "text/plain", "File deleted successfully");
            } else {
                request->send(500, "text/plain", "File deletion failed");
            }
        }
    });

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200);
    }, onUpload);

    // Create empty file
    server.on("/touch", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, "text/plain", "Missing 'path' parameter");
            return;
        }

        String path = request->getParam("path")->value();

        if (SD.exists(path)) {
            request->send(200, "text/plain", "File already exists");
            return;
        }

        File file = SD.open(path, FILE_WRITE);

        if (file) {
            file.close();
            request->send(200, "text/plain", "File created successfully");
        } else {
            request->send(500, "text/plain", "File creation failed");
        }
    });

    // Preview file (send JSON with information about file)
    server.on("/preview", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("path")) {
            request->send(400, "text/plain", "Missing 'path' parameter");
            return;
        }

        String path = request->getParam("path")->value();

        if (!SD.exists(path)) {
            request->send(400, "text/plain", "File does not exist");
            return;
        }

        File file = SD.open(path);

        if (!file) {
            request->send(500, "text/plain", "Failed to open file");
            return;
        }

        if (file.isDirectory()) {
            request->send(400, "text/plain", "Path is a directory");
            return;
        }

        String contentType = "text/plain";

        if (path.endsWith(".jpg") || path.endsWith(".jpeg")) {
            contentType = "image/jpeg";
        } else if (path.endsWith(".png")) {
            contentType = "image/png";
        } else if (path.endsWith(".gif")) {
            contentType = "image/gif";
        } else if (path.endsWith(".bmp")) {
            contentType = "image/bmp";
        } else if (path.endsWith(".webp")) {
            contentType = "image/webp";
        } else if (path.endsWith(".ico")) {
            contentType = "image/x-icon";
        } else if (path.endsWith(".svg")) {
            contentType = "image/svg+xml";
        }

        time_t t = file.getLastWrite();
        struct tm *tmstruct = localtime(&t);

        String year = String((tmstruct->tm_year * 1) + 1900);
        String month = String((tmstruct->tm_mon * 1) + 1);
        String day = String(tmstruct->tm_mday * 1);
        String hour = String(tmstruct->tm_hour * 1);
        String minute = String(tmstruct->tm_min * 1);
        String second = String(tmstruct->tm_sec * 1);

        month = month.length() < 2 ? "0" + month : month;
        day = day.length() < 2 ? "0" + day : day;
        hour = hour.length() < 2 ? "0" + hour : hour;
        minute = minute.length() < 2 ? "0" + minute : minute;
        second = second.length() < 2 ? "0" + second : second;

        String modifiedAt = year + "-" + month + "-" + day + " " + hour + ":" + minute + ":" + second; // spaces!

        String json = "{";
        json += "\"name\":\"" + String(file.name()) + "\",";
        json += "\"path\":\"" + path + "\",";
        json += "\"size\":" + String(file.size()) + ",";
        json += "\"contentType\":" + String("\"" + contentType + "\",");
        json += "\"modifiedAt\":" + String("\"" + modifiedAt + "\"");
        json += "}";

        file.close();

        request->send(200, "application/json", json);
    });

    // server.onFileUpload(handleUpload);

    // Serve static files
    server.serveStatic("/", SD, "/").setDefaultFile("index.html");

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    server.begin();
    Serial.println("HTTP: OK");

    // SCREEN

    HUB75_I2S_CFG mxconfig(panelResX, panelResY, panels_in_X_chain);

    mxconfig.gpio.e = E;
    mxconfig.gpio.r1 = R1;
    mxconfig.gpio.r2 = R2;

    if (digitalRead(ALT) == HIGH) {
        mxconfig.gpio.b1 = B1;
        mxconfig.gpio.b2 = B2;
        mxconfig.gpio.g1 = G1;
        mxconfig.gpio.g2 = G2;
    } else {
        mxconfig.gpio.b1 = G1;
        mxconfig.gpio.b2 = G2;
        mxconfig.gpio.g1 = B1;
        mxconfig.gpio.g2 = B2;
    }

    mxconfig.clkphase = false;

    matrix_display = new MatrixPanel_I2S_DMA(mxconfig);
    matrix_display->begin();
    matrix_display->clearScreen();
    matrix_display->setBrightness8(matrix_brightness);
    matrix_display->setTextWrap(false);

    // Multicore
    xTaskCreatePinnedToCore(GifThread, "GifThreadTask", 5000, NULL, 2, &Task1, 0);
}

void loop()
{
}
