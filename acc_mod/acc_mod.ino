#include <SoftwareSerial.h>
#include <string.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

SoftwareSerial UnoConn(3,4);
SoftwareSerial mySerial(5,6);
MPU6050 accelgyro;
int16_t ax, ay, az;
int16_t gx, gy, gz;
float AX, AY, AZ, GX, GY, GZ;
#define OUTPUT_READABLE_ACCELGYRO
#define SEND_TO_NODE
bool stop_flag;

void setup() {
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif
    stop_flag = false;
    // initialize serial communication
    Serial.begin(9600);
    UnoConn.begin(19200);
    mySerial.begin(9600);
    
    // GSM
    mySerial.begin(9600);

    Serial.print("Initializing."); 
    delay(1000);

    mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
    delay(1000);
    Serial.print('.');
    updateSerial();
    
    mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
    delay(1000);
    Serial.print('.');
    updateSerial();
    mySerial.println("AT+CNMI=1,2,0,0,0"); // Decides how newly arrived SMS messages should be handled
    delay(1000);
    Serial.print('.');
    updateSerial();
    mySerial.println("AT+CMGDA=\"DEL ALL\"");
    delay(1000);
    Serial.println('.');
    updateSerial();
    // END GSM
    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    // use the code below to change accel/gyro offset values
    
    Serial.println("Updating internal sensor offsets...");
    // -76	-2359	1688	0	0	0

    accelgyro.setXAccelOffset(1445);
    accelgyro.setYAccelOffset(915);
    accelgyro.setZAccelOffset(1395);
    accelgyro.setXGyroOffset(602);
    accelgyro.setYGyroOffset(-140);
    accelgyro.setZGyroOffset(91);
    delay(5000);
    
}

void loop() {
    // read raw accel/gyro measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    AX = (float)ax/16384.0;
    AY = (float)ay/16384.0;
    AZ = (float)az/16384.0;
    GX = (float)gx/131.0;
    GY = (float)gy/131.0;
    GZ = (float)gz/131.0;    // these methods (and a few others) are also available
    //accelgyro.getAcceleration(&ax, &ay, &az);
    //accelgyro.getRotation(&gx, &gy, &gz);
      
    #ifdef OUTPUT_READABLE_ACCELGYRO
    if (!stop_flag){
        // display tab-separated accel/gyro x/y/z values
        Serial.print("a/g:\t");
        Serial.print(AX); Serial.print("\t");
        Serial.print(AY); Serial.print("\t");
        Serial.print(AZ); Serial.print("\t");
        Serial.print(GX); Serial.print("\t");
        Serial.print(GY); Serial.print("\t");
        Serial.println(GZ);
        updateSerial();
    }
    #endif

    #ifdef SEND_TO_NODE
    // Send data to node_mcu
    if (!stop_flag)
    {
      UnoConn.print(AX);
      UnoConn.print(" #");
      UnoConn.print(AY);
      UnoConn.print(" #");
      UnoConn.print(AZ);
      UnoConn.print(" #");
      UnoConn.print(GX);
      UnoConn.print(" #");
      UnoConn.print(GY);
      UnoConn.print(" #");
      UnoConn.print(GZ);
      UnoConn.print("#");
      UnoConn.print("\n#");
      UnoConn.flush();  
    } 
      
    #endif
    delay(1000);
    
}

void updateSerial()
{
  String str;
  delay(500);
  while (Serial.available()) 
  {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(mySerial.available()) 
  {
  str = mySerial.readStringUntil('#');
    if (str=="TEST")//Forward what Software Serial received to Serial Port
    {
      str = mySerial.readStringUntil('#');      
      UnoConn.println("#STOP#"+str+"#");   
      stop_flag = true;
    } else {
      str = "";
    }
  }
}
