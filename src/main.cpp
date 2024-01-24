#include <Arduino.h>
#include <Ezo_i2c.h> //include the EZO I2C library from https://github.com/Atlas-Scientific/Ezo_I2c_lib
#include <Wire.h>    //include arduinos i2c library
#include <sequencer2.h> //imports a 2 function sequencer
#include <Ezo_i2c_util.h> //brings in common print statements
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

// I2C addresses
    Ezo_board HUM = Ezo_board(111, "HUM");
    Ezo_board o2 = Ezo_board(112, "o2");
    LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
    char Humidity_data[32];
    char *HUMID;
    char *TMP;
    char *NUL;
    char o2_data[20]; //since the o2 sensor only gives a single value response, we do not need to parse.

    float HUMID_float;
    float TMP_float;
    float o2_float;
    float PRES;
    float TMP2; //temperature from BMP pressure and temperature sensor
    float HUMppm;
    float O2ppm;

//Conversion Functions
    float convertHumidityToPPM(float humidity, float temperature) {
        float E = 6.1094 * exp((17.625 * temperature) / (temperature + 243.04));
        float VP = (humidity / 100.0) * E;
        float atmosphericPressure = 1013.25; // Standard atmospheric pressure at sea level
        return (VP / atmosphericPressure) * 1000000;
    }

    float convertOxygenToPPM(float oxygenPercentage) {
        return oxygenPercentage * 10000;
    }

// Definitions
    #define LOG_INTERVAL  3000 // mills between entries (reduce to take more/faster data)
    #define SYNC_INTERVAL 3000 // mills between calls to flush() - to write data to the card
    uint32_t syncTime = 0; // time of last sync()
    #define ECHO_TO_SERIAL   1 // echo data to serial port
    RTC_DS1307 RTC; // define the Real Time Clock object
    const int chipSelect = 10;
    File logfile;

    void step1();
    void step2();
    Sequencer2 Seq(&step1, 1000, &step2, 0);

// Error handling
    void error(char *str)
    {
        Serial.print("error: ");
        Serial.println(str);
        while(1);
    }


void setup(){
        Wire.begin();
        Serial.begin(9600);
        Seq.reset();

    //LCD card setup
        lcd.init();
        lcd.clear();
        lcd.backlight();

        // Clear the display
        lcd.clear();

        // Display "Mandarijn Monitoor"
        lcd.setCursor(0, 0);
        lcd.print("Mandarijn");
        lcd.setCursor(0, 1);
        lcd.print("Monitoor");

        delay(3000);
        HUM.send_cmd("o,t,1");        //send command to enable temperature output
        delay(300);
        HUM.send_cmd("o,dew,1");      //send command to disable dew point output
        delay(300);

        Serial.println("SETUP COMPLETE");
        Serial.println("");


    //SD card logging setup
        Serial.print("Initializing SD card now...");
        pinMode(10, OUTPUT);
        if (!SD.begin(chipSelect)) {
            error("Card failed, or not present");
        }
        Serial.println("card initialized.");
        char filename[] = "LOGGER00.CSV";
        for (uint8_t i = 0; i < 100; i++) {
            filename[6] = i/10 + '0';
            filename[7] = i%10 + '0';
            if (! SD.exists(filename)) {
                // only open a new file if it doesn't exist
                logfile = SD.open(filename, FILE_WRITE);
                break;  // leave the loop!
            }
        }
        if (! logfile) {
            error("couldnt create file");
        }
        Serial.print("Logging to: ");
        Serial.println(filename);

    //RTC Setup
        Wire.begin();
        if (!RTC.begin()) {
            logfile.println("RTC failed");
            Serial.println("RTC failed");
        }

    //Logfile Header
        logfile.println("millis,stamp,datetime,temp, HUM, O2, h20(ppm),o2(ppm)");
        Serial.println("millis,stamp,datetime,temp,HUM, O2, hum(ppm),o2(ppm)");
        delay(2000);
    }


void loop() {
        DateTime now;
        Seq.run();
        delay(2000);

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("HUM: ");
        lcd.print(HUMID);
        lcd.print(" %");
        lcd.setCursor(0,1);
        lcd.print("O2: ");
        lcd.print(o2_data);
        lcd.print(" %");

        uint32_t m = millis();
        logfile.print(m);
        logfile.print(",");
        Serial.print(m);
        Serial.print(",");

        now = RTC.now();
        logfile.print(now.unixtime()); // seconds since 1/1/1970
        logfile.print(",");
        logfile.print(now.year(), DEC);
        logfile.print("/");
        logfile.print(now.month(), DEC);
        logfile.print("/");
        logfile.print(now.day(), DEC);
        logfile.print(" ");
        logfile.print(now.hour(), DEC);
        logfile.print(":");
        logfile.print(now.minute(), DEC);
        logfile.print(":");
        logfile.print(now.second(), DEC);

        Serial.print(now.unixtime()); // seconds since 1/1/1970
        Serial.print(",");
        Serial.print(now.year(), DEC);
        Serial.print("/");
        Serial.print(now.month(), DEC);
        Serial.print("/");
        Serial.print(now.day(), DEC);
        Serial.print(" ");
        Serial.print(now.hour(), DEC);
        Serial.print(":");
        Serial.print(now.minute(), DEC);
        Serial.print(":");
        Serial.print(now.second(), DEC);

        logfile.print(",");
        logfile.print(TMP);
        Serial.print(", ");
        Serial.print(TMP);

        logfile.print(",");
        logfile.print(HUMID);
        Serial.print(",");
        Serial.print(HUMID);

        logfile.print(",");
        logfile.print(o2_data);
        Serial.print(",");
        Serial.print(o2_data);

        logfile.print(",");
        logfile.print(HUMppm);
        Serial.print(",");
        Serial.print(HUMppm);

        logfile.print(",");
        logfile.print(O2ppm);
        Serial.print(",");
        Serial.print(O2ppm);

        logfile.println();
        Serial.println();

        logfile.flush();

}

void step1(){
        HUM.send_cmd("r");
        o2.send_cmd("r");
    }

void step2(){
        HUM.receive_cmd(Humidity_data, 32);
        HUMID = strtok(Humidity_data, ",");
        TMP = strtok(NULL, ",");
        NUL = strtok(NULL, ",");
        o2.receive_cmd(o2_data,20);

        TMP_float = atof(TMP);
        HUMID_float = atof(HUMID);
        o2_float = atof(o2_data);

        HUMppm = convertHumidityToPPM(HUMID_float, TMP_float);
        O2ppm = convertOxygenToPPM(o2_float);

    }


