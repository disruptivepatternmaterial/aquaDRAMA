/*
   need to do this
   make a way to send in callib commands/numbers
   set vars for result codes for monitoring from afar
   email end points for alarms
   clean up globals
   really use methofs vs globals
   mqtt for calibration?
   MKR1000
   wifi and thinger.io vars are externalized

*/
#define _DEBUG_
#include <SPI.h>
#include <WiFi101.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>                //enable I2C.
//#include <avr/dtostrf.h>
#include <ThingerWifi101.h>
#include "WiFi_Info.h"

// Pin use
#define ONEWIRE 4     //pin to use for One Wire interface
#define CHLR 5        //this will go high when the chiller is on
#define WARNLED 6     //define my warning LED

// i2c addresses on shield
#define EC_address 100              //default I2C ID number for EZO EC Circuit.
#define pH_address 99              //default I2C ID number for EZO pH Circuit.

// Set up which Arduino pin will be used for the 1-wire interface to the sensor
OneWire oneWire(ONEWIRE);
DallasTemperature sensors(&oneWire);

//start tup thinger
ThingerWifi101 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

//vars for sg and other items
byte code = 0;
char ec_data[48];                //we make a 48 byte character array to hold incoming data from the EC circuit.
byte in_char = 0;                //used as a 1 byte buffer to store in bound bytes from the EC Circuit.
byte i = 0;                      //counter used for ec_data array.

char *ec;                        //char pointer used in string parsing.
char *tds;                       //char pointer used in string parsing.
char *sal;                       //char pointer used in string parsing.
char *sg;                        //char pointer used in string parsing.

float ec_float;                  //float var used to hold the float value of the conductivity.
float tds_float;                 //float var used to hold the float value of the TDS.
float sal_float;                 //float var used to hold the float value of the salinity.
float sg_float;                  //float var used to hold the float value of the specific gravity.
 

float tempF;
float tempC;
float pH;
float spec_grav;
float sg_thinger_mess;
int chiller_status;

String ec_calib_code;
String ph_calib_code;
String ec_code;
String ph_code;


/*==============================================================================
  End of Configuration Variables
  =============================================================================*/

void setup() {

  //bring up serial
  Serial.begin(115200);
  thing.add_wifi(wifi_ssid, wifi_password);

  //what are my pins up to?
  pinMode(CHLR, INPUT_PULLUP);
  pinMode(WARNLED, OUTPUT);

  // Start up the OneWire Sensors library
  Serial.println("Starting Temp Monitor");
  sensors.begin();
  delay(500);
  Serial.println("Starting I2C");
  Wire.begin();                          //enable I2C port.
  delay(500);

  // the main thinger loop to do the work

  thing["tankdata"] >> [](pson & out) {
    digitalWrite(WARNLED, HIGH);   // turn the LED on (HIGH is the voltage level)
    //get temp
    read_temp();
    //sg call
    read_sg(tempC);
    //ph call
    read_ph(tempC);
    //chiller
    read_chiller();

    out["temp"] = tempF;
    out["pH"] = pH;
    out["specificgravity"] = spec_grav;
    out["thingerfixedsg"] = sg_thinger_mess; 
    out["chiller"] = chiller_status;
    out["millis"] = millis();
    out["sgCalibCode"] = ec_calib_code;
    out["phCalibCode"] = ph_calib_code;
    out["sgProbeResponse"] = ec_code;
    out["phProbeResponse"] = ph_code;
    digitalWrite(WARNLED, LOW);   // turn the LED on (HIGH is the voltage level)
  };


}


void loop() {
  thing.handle();
}

/*==============================================================================
  functions
  =============================================================================*/

void string_pars() {                  //this function will break up the CSV string into its 4 individual parts. EC|TDS|SAL|SG.
  //this is done using the C command “strtok”.
  ec = strtok(ec_data, ",");          //let's pars the string at each comma.
  tds = strtok(NULL, ",");            //let's pars the string at each comma.
  sal = strtok(NULL, ",");            //let's pars the string at each comma.
  sg = strtok(NULL, ",");             //let's pars the string at each comma.

  ec_float = atof(ec);
  tds_float = atof(tds);
  sal_float = atof(sal);
  sg_float = atof(sg);

}

void temp_cal(int chan, int temp) {
  Wire.beginTransmission(chan);          //call the circuit by its ID number.
  String tempcmd = "t,";
  tempcmd += temp;
  Wire.write(tempcmd.c_str());
  Wire.endTransmission(); //end the I2C data transmission.
  delay(200); //slow down loop
  Wire.requestFrom(chan, 48, 1);          //call the circuit and request 48 bytes (this is more than we need)
  code = Wire.read();                     //the first byte is the response code, we read this separately.
  Serial.print("T Calib: " + String(chan) + " ");
  Serial.println(code);
  if (chan == 100) {
    ec_calib_code = code;
  } else {
    ph_calib_code = code;
  }
}

void read_ph(float temp) {
  //float pH;
  //cal temp
  temp_cal(pH_address, temp);
  //read ph
  Wire.beginTransmission(pH_address);
  Wire.write('r');
  Wire.endTransmission();
  delay(1000);
  // need to do the reading here???
  Wire.requestFrom(pH_address, 48, 1);          //call the circuit and request 48 bytes (this is more than we need)
  delay(100);
  code = Wire.read();                        //the first byte is the response code, we read this separately.
  //Serial.println(code);
  ph_code = code;
  while (Wire.available()) {                  //are there bytes to receive.
    //Serial.println(ec_data);
    in_char = Wire.read();                    //receive a byte.
    ec_data[i] = in_char;                     //load this byte into our array.
    i += 1;                                   //incur the counter for the array element.
    if (in_char == 0) {                       //if we see that we have been sent a null command.
      i = 0;                                  //reset the counter i to 0.
      Wire.endTransmission();                 //end the I2C data transmission.
      break;                                  //exit the while loop.
    }
  }
  Serial.print("ph data: ");
  Serial.println(ec_data);
  pH = atof(ec_data);
  //pH = 6.77;
  //return pH;
}

void read_sg(float temp) {
  //float sg;
  //cal temp
  temp_cal(EC_address, temp);
  Wire.beginTransmission(EC_address);
  Wire.write('r');
  Wire.endTransmission();
  delay(1000);
  // need to do the reading here???
  Wire.requestFrom(EC_address, 48, 1);          //call the circuit and request 48 bytes (this is more than we need)
  delay(100);
  code = Wire.read();                        //the first byte is the response code, we read this separately.
  //Serial.println(code);
  ec_code = code;
  while (Wire.available()) {                  //are there bytes to receive.
    in_char = Wire.read();                    //receive a byte.
    ec_data[i] = in_char;                     //load this byte into our array.
    i += 1;                                   //incur the counter for the array element.
    if (in_char == 0) {                       //if we see that we have been sent a null command.
      i = 0;                                  //reset the counter i to 0.
      Wire.endTransmission();                 //end the I2C data transmission.
      break;                                  //exit the while loop.
    }
  }
  Serial.print("sg data: ");
  Serial.println(ec_data);
  string_pars();
  spec_grav = sg_float;
  spec_grav = sg_float + .003 ; //adjustment for error
  sg_thinger_mess = spec_grav * 1000;
  //  spec_grav = 1.255;
  //return sg;
}

void read_temp() {
  //float tempC;
  sensors.requestTemperatures(); // Send the command to get temperatures
  tempC = sensors.getTempCByIndex(0);
  tempF = DallasTemperature::toFahrenheit(tempC);
  //return tempC;
}

void read_chiller() {
  int CHLRState;
  CHLRState = digitalRead(CHLR);
  chiller_status = CHLRState;
  //return CHLRState;
}
