#include <Arduino.h>
#include <Wire.h>
#include <sequencer2.h> //imports a 2 function sequencer
#include <Ezo_i2c_util.h> //brings in common print statements
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"

Ezo_board HUM = Ezo_board(111, "HUM");
Ezo_board o2 = Ezo_board(112, "o2");
LiquidCrystal_I2C lcd(0x27, 16, 2);

char Humidity_data[32];
char *HUMID;
char *TMP;
char *NUL;
char o2_data[20]; //since the o2 sensor only gives a single value response, we do not need to parse.

float HUMID_float;
float TMP_float;



#define LOG_INTERVAL  3000 // mills between entries (reduce to take more/faster data)
#define SYNC_INTERVAL 3000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()
#define ECHO_TO_SERIAL   1 // echo data to serial port
RTC_DS1307 RTC; // define the Real Time Clock object
const int chipSelect = 10;
File logfile;



void step1();  //forward declarations of functions to use them in the sequencer before defining them
void step2();

Sequencer2 Seq(&step1, 1000, &step2, 0);  //calls the steps in sequence with time in between them

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}

void setup() {
  Wire.begin();                           //start the I2C
  Serial.begin(9600);                     //start the serial communication to the computer
  Seq.reset();                            //initialize the sequencer

  delay(3000);
  HUM.send_cmd("o,t,1");        //send command to enable temperature output
  delay(300);
  HUM.send_cmd("o,dew,0");      //send command to disable dew point output
  delay(300);
  Serial.println("SETUP COMPLETE");
  Serial.println("");

  lcd.init(); // initialize the lcd
  lcd.backlight();

  // initialize the SD card
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");

  // create a new file
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

  Wire.begin();
  if (!RTC.begin()) {
    logfile.println("RTC failed");
  #if ECHO_TO_SERIAL
    Serial.println("RTC failed");
  #endif  //ECHO_TO_SERIAL
  }

  logfile.println("millis,stamp,datetime,temp,hum,o2");
  #if ECHO_TO_SERIAL
    Serial.println("millis,stamp,datetime,temp,hum,o2");
  #endif
}

void loop()
{
  DateTime now;
  Seq.run();                              //run the sequncer to do the polling
  delay(2000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("HUM: ");
  lcd.print(HUMID);
  lcd.print("%");
  lcd.setCursor(0,1);
  lcd.print("O2: ");
  lcd.print(o2_data);
  lcd.print("%");

  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(",");
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(",");
#endif
  // fetch the time
  now = RTC.now();
  // log time
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(",");
  //logfile.print('"');
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
  //logfile.print('"');
#if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // seconds since 1/1/1970
  Serial.print(",");
  //Serial.print('"');
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
  //Serial.print('"');
#endif //ECHO_TO_SERIAL

logfile.print(",");
  logfile.print(TMP);
#if ECHO_TO_SERIAL
  Serial.print(", ");
  Serial.print(TMP);
#endif //ECHO_TO_SERIAL

logfile.print(",");
  logfile.print(HUMID);
#if ECHO_TO_SERIAL
  Serial.print(",");
  Serial.print(HUMID);
#endif //ECHO_TO_SERIAL

logfile.print(",");
  logfile.print(o2_data);
#if ECHO_TO_SERIAL
  Serial.print(",");
  Serial.print(o2_data);
#endif //ECHO_TO_SERIAL

  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
#endif // ECHO_TO_SERIAL

 logfile.flush();
}


void step1(){
  //tell the sensors to do a reading
  HUM.send_cmd("r");
  o2.send_cmd("r");
}

void step2(){
  //read and parse sensor responses
  HUM.receive_cmd(Humidity_data, 32);       //put the response into the buffer
    HUMID = strtok(Humidity_data, ",");       //let's pars the string at each comma. variable HUMID
    TMP = strtok(NULL, ",");                  //let's pars the string at each comma. variable TMP
    NUL = strtok(NULL, ",");                  //let's pars the string at each comma (the sensor outputs the word "DEW" in the string, we dont need it.
  o2.receive_cmd(o2_data,20);    //o2 data stores in variable o2_data
}