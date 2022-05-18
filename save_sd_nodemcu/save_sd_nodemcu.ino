#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "SSD1306Wire.h"
#include <Wire.h> 
#include <WiFi.h>

#define RXD2 9
#define TXD2 10
SSD1306Wire display(0x3c, SDA, SCL);

// WiFi network config
const char* networkName = "ssid";
const char* networkPswd = "pswd";

boolean connected = false;
WiFiClient client;
bool stop_flag;
bool send_once;
int Sample;
File myFile;

String filepath;
String fileExt;
String file_key;
int height = 0;


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, String message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    File f = fs.open(path2);
    if (f){
      f.close();
      deleteFile(SD, path2);
    }
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void connectToWiFi(const char * ssid, const char * pwd) {
  Serial.println("Connecting to WiFi network: " + String(ssid));
  drawText(0,&height,"Connecting to WiFi network:");
  drawText(0,&height,String(ssid));
  // delete old config
  WiFi.disconnect(true);
  WiFi.onEvent(WiFiEvent);
  // Initialize connection
  WiFi.begin(ssid, pwd);
  int count = 0;
  while(WiFi.status()!=WL_CONNECTED) {
    count++;
    delay(500);
    Serial.print(".");
    if (count == 30){
      Serial.println("");
      Serial.println("Failed to connect to WiFi");
      Serial.println("Retrying connection...");
      drawText(0,&height,"Failed to connect to WiFi");
      drawText(0,&height,"Retrying connection...");
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  drawText(0,&height,"WiFi connected");
}

void WiFiEvent(WiFiEvent_t event) {
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      // When connected
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP());
      connected = true;
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      connected = false;
      break;
    default: break;
  }
}

void transmit_data(){
    if (WiFi.status() == WL_CONNECTED){
        const int httpPort = 1234; // Port number
        String host = "replace with host_name";
        if(client.connect(host,httpPort)){
            client.setNoDelay(true);
            Serial.println("Connected to Server");
            drawText(0,&height,"Connected to Server");
        } else {
            Serial.println("Not connected");
            drawText(0,&height,"Not connected to Server!");
        }
        String url = "/send_data/";
        String boundary = "WebKitFormBoundary7MA4YWxkTrZu0gW";
        String contentType = "text/plain";
        // post header
        String postHeader = "POST " + url + " HTTP/1.1\r\n";
        postHeader += "Host: " + host + " \r\n";
        postHeader += "Content-Type: multipart/form-data; boundary=----" + boundary + "\r\n";
        String keyHeader = "------" + boundary + "\r\n";
        keyHeader += "Content-Disposition: form-data; name=\"data\"\r\n\r\n";
        String requestHead = "------" + boundary + "\r\n";
        requestHead += "Content-Disposition: form-data; name=\"data\"; filename=\"" + filepath + "\"\r\n";
        requestHead += "Content-Type: " + contentType + "\r\n\r\n";
        String tail = "\r\n------" + boundary + "--\r\n\r\n";
        String mid_tail = "\r\n------" + boundary + "--\r\n";
        String requestHead1 = "Content-Disposition: form-data; name=\"file_id\"\r\n\r\n"; 
        String data_key = file_key;
        // content length
        myFile = SD.open(filepath);
        int contentLength = keyHeader.length() + requestHead.length() + myFile.size() + mid_tail.length() + 
        requestHead1.length() + data_key.length() +tail.length();
        postHeader += "Content-Length: " + String(contentLength, DEC) + "\n\n";
        // send post header
        char charBuf0[postHeader.length() + 1];
        postHeader.toCharArray(charBuf0, postHeader.length() + 1);
        client.write(charBuf0);
        Serial.print("send post header=");
        Serial.println(charBuf0);
        // send key header
        char charBufKey[keyHeader.length() + 1];
        keyHeader.toCharArray(charBufKey, keyHeader.length() + 1);
        client.write(charBufKey);
        Serial.print("send key header=");
        Serial.println(charBufKey);
        // send request buffer
        char charBuf1[requestHead.length() + 1];
        requestHead.toCharArray(charBuf1, requestHead.length() + 1);
        client.write(charBuf1);
        Serial.print("send request buffer=");
        Serial.println(charBuf1);
        // create buffer
        const int bufSize = 2048;
        byte clientBuf[bufSize];
        int clientCount = 0;
        // read myFile until there's nothing else in it:
        while (myFile.available())
        {
        clientBuf[clientCount] = myFile.read();
        clientCount++;
        if (clientCount > (bufSize - 1))
        {
            client.write((const uint8_t *)clientBuf, bufSize);
            clientCount = 0;
        }
        }
        if (clientCount > 0)
        {
        client.write((const uint8_t *)clientBuf, clientCount);
        Serial.println("Sent LAST buffer");
        }
        // send mid tail
        char charBuf3[mid_tail.length() + 1];
        mid_tail.toCharArray(charBuf3, mid_tail.length() + 1);
        client.write(charBuf3);
        Serial.print(charBuf3);
        // Sending the Key
    
        // send request buffer
        char charBuf4[requestHead1.length() + 1];
        requestHead1.toCharArray(charBuf4, requestHead1.length() + 1);
        client.write(charBuf4);
        Serial.print("send request buffer=");
        Serial.println(charBuf4);

        char charBufx1[data_key.length() + 1];
        data_key.toCharArray(charBufx1, data_key.length() + 1);
        client.write(charBufx1);
        Serial.print("send request buffer=");
        Serial.println(charBufx1);
        
        Serial.println("Key Send!!!");
        // send tail
        char charBuf5[tail.length() + 1];
        tail.toCharArray(charBuf5, tail.length() + 1);
        client.write(charBuf5);
        Serial.print(charBuf5);
        // End Sending the Key

        Serial.println("end_request");
        while (client.connected())
        {
            while (client.available())
            {
                String line = client.readStringUntil('\r');
                Serial.print(line);
                if (line.indexOf("RECEIVED") > 0)
                {
                    send_once = false;
                    client.stop();
                    drawText(0,&height,"Data Successfully Send");
                }
                if (line.indexOf("ERROR OCCURRED!!!")>0){
                    drawText(0,&height,"Failed Send Data!");
                }
            }
        }
        myFile.close();
        Serial.println("\nclosing connection");
    } else {
        connectToWiFi(networkName, networkPswd);
    }
    Serial.println("Waiting…");       // thingspeak needs minimum 15 sec delay between updates
    delay(10000);
}

void drawText(int x, int *y,String s) { 
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(x, *y, s);
  display.display();
  *y+=10;
  if (*y > 50){
    *y = 0;
    delay(200);
    display.clear();
  }
}


void setup(){
    Serial.begin(115200);
    Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);
    // Initialising the UI will init the display too.
    display.init();
    display.flipScreenVertically();
    stop_flag = false;
    send_once = true;
    // SD Card
    Serial.print("Initializing SD card...");
    drawText(0,&height,"Initializing SD card..."); 
    if(!SD.begin()){
        Serial.println("Card Mount Failed");
        drawText(0,&height,"SD Card Mount Failed"); 
        return;
    }
    drawText(0,&height,"SD Card Mounted");
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        drawText(0,&height,"No SD card attached");
        return;
    }
    filepath = "/acc_data";
    fileExt = ".txt";
    Serial.print("SD Card Type: ");
    drawText(0,&height,"SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
        drawText(0,&height,"MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
        drawText(0,&height,"SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
        drawText(0,&height,"SDHC");
    } else {
        Serial.println("UNKNOWN");
        drawText(0,&height,"UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    listDir(SD, "/", 2);
    String s_temp = filepath+fileExt;
    writeFile(SD, s_temp.c_str(), "");
    Serial.println("Waiting for start signal...");
    drawText(0,&height,"All devices initialized ...");
    drawText(0,&height,"Waiting for start signal...");
    // WiFi
//    connectToWiFi(networkName, networkPswd);
    delay(2000);
    display.clear();
    height = 0;
    // end WiFi
}

void loop(){
  String str;
  String s_temp = filepath+fileExt;
  while(Serial2.available()>0 && !stop_flag){
    str = Serial2.readStringUntil('#');
    Serial.println(str);
    if (str == "STOP")
    {
        display.clear();
        height = 0;
        drawText(0,&height,"Stop Signal Received");
        drawText(0,&height,"Preparing to Send Data...");
        stop_flag = true;
        file_key = Serial2.readStringUntil('#');
        String temp = s_temp;
        filepath += file_key;
        filepath += fileExt;
        renameFile(SD, temp.c_str(), filepath.c_str());
        if(Serial2.available()){
          Serial2.readString();
        }
        break;
    } else if(str == "START"){
      display.clear();
        height = 0;
        drawText(0,&height,"Start Signal Received");
        drawText(0,&height,"Writing to file...");
        stop_flag = false;
        send_once = true;
        filepath = "/acc_data";
        fileExt = ".txt";
        String s_temp = filepath+fileExt;
        writeFile(SD, s_temp.c_str(), "");
        Serial.println("Writing to file ...");
    }
    else {
      if (str!="\n"){
        if(str!="" && str.indexOf('\n')==-1){
          float val = str.toFloat();
          String str1 = String(val,2);
          if (str.indexOf(",")!=-1){
            str1 += ", ";
          }
          appendFile(SD, s_temp.c_str(), str1);
        }
      } else{
        appendFile(SD, s_temp.c_str(), "\n");
      }
    }
  }
  if (stop_flag && send_once){
    if (connected) {
      drawText(0,&height,"Transmitting data...");
      transmit_data();
    } else {
        drawText(0, &height, "Connecting to WiFi...");
        connectToWiFi(networkName, networkPswd);
    }
    delay(1000);
  }
  while(Serial2.available()>0 && stop_flag){
    str = Serial2.readStringUntil('#');
    Serial.println(str);
    if(str == "START"){
      display.clear();
        height = 0;
        drawText(0,&height,"Start Signal Received");
        drawText(0,&height,"Writing to file...");
        stop_flag = false;
        send_once = true;
        filepath = "/acc_data";
        fileExt = ".txt";
        String s_temp = filepath+fileExt;
        writeFile(SD, s_temp.c_str(), "");
        Serial.println("Writing to file ...");
    }
  }
  delay(50);
}