# Dwear-Arduino
## Data Collection Module.
Responsible for collecting data and sending it to the webserver.
Comprises of an Arduino uno board, MPU6050 accelerometer sensor and SIM800l GSM module.
MPU6050 collects accelerometer readings on x, y, z planes and sends it to the Arduino Uno board.
The SIM800l accepts incoming messages and sends the stop signal to the Arduino Uno when a specific message format is received.
Message format #SIGNAL#CODE#.  The '#' is used as a delimiter. The SIGNAL = [TEST, STOP, START], CODE = 123, 345, ...

## Data Transmission Module
This consist of the NodeMCU-32 and Micro SD card reader. It actively receives data from the Arduino Uno board.
When the STOP signal is received, it receives the code and stops listening to incoming feed.
It renames the file stored in the SD card by including the code in it.
It connects to the internet and sends the data to the webserver.
