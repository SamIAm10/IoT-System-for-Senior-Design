# IoT System for Senior Design
This repo stores some of the shareable code for my [senior design project](https://ecesd.engr.uconn.edu/ecesd2013/).

- The Android app installation file (APK) can be built from the `android_app.aia` source file.
- The `arduino_code.ino` file can be programmed onto an Arduino board using the Arduino IDE.
- The Python script `thingspeak_delay_FINAL.py` is used to measure the delay from the Arduino sending data to the cloud, to the app receiving the data from the cloud, while also detecting any errors. The Arduino must be plugged into the proper port before running. Sample data for one hour of runtime can be found in `delay_data.csv`.
