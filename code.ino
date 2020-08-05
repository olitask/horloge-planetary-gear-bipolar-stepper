

/*wemos mini d1 pro
  pin  TX  RX  D1  D2  D3  D4
  GPIO 1   3   5   4   0   2

esp-01
 pin      01  GND
          CH  02
 GPIO     RST 00
          VCC 03
  
*/


#if defined ARDUINO_ARCH_ESP8266  // s'il s'agit d'un ESP8266
#include <ESP8266WiFi.h>
#elif defined ARDUINO_ARCH_ESP32  // s'il s'agit d'un ESP32
#include "WiFi.h"
#endif

#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
const long utcOffsetInSeconds = 3600;  //heure hiver
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "fr.pool.ntp.org", utcOffsetInSeconds);
unsigned long derniereDemande = millis(); // moment de la plus récente demande au serveur NTP
unsigned long derniereMaJ = millis(); // moment de la plus récente mise à jour de l'affichage de l'heure
const int delaiDemande = 5 * 60; // nombre de secondes entre deux demandes consécutives au serveur NTP
unsigned long clock_time ;        //datetime varable to store the current position of the clock's arms
unsigned long rtc_time ;
unsigned long seconds ;
unsigned long steps ;
int dir = 1;

#include "BasicStepperDriver.h"
// for your motor
#define MOTOR_STEPS 400
#define RPM 60
#define MICROSTEPS 1
#define DIR 1
#define STEP 2
//Uncomment line to use enable/disable functionality
#define SLEEP 3
#define Hall_pin 0          //pin for the hall effect sensor signal
#define Step_fast_speed 120 //fast speed of the stepper
#define Step_slow_speed 70  //slow speed of the stepper
#define Sec_per_step 3L     //ammount of seconds per step (change this for different ratios between stepper and minute hand)

BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP, SLEEP);

const char* ssid = "WifiBox";
const char* password = "PasswordOfWifiBox";

void setup() {
 // Serial.begin(115200);
  delay(100);
  WiFi.mode(WIFI_STA);



  WiFi.begin(ssid, password);
 // Serial.print("Connexion au reseau WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  delay(100);

  stepper.begin(RPM, MICROSTEPS);
  // if using enable/disable on ENABLE pin (active LOW) instead of SLEEP uncomment next line
  stepper.setEnableActiveState(LOW);
  pinMode(Hall_pin, INPUT);
  timeClient.begin();
  delay(100);
  timeClient.update();
  calibrate();
}

void loop() {

  rtc_time = (timeClient.getHours() * 60 + timeClient.getMinutes()) * 60;


  // est-ce le moment de demander l'heure NTP?
  if ((millis() - derniereDemande) >=  delaiDemande * 1000 ) {
    timeClient.update();
    derniereDemande = millis();

    //Serial.println("Interrogation du serveur NTP");
  }

  // est-ce que millis() a débordé?
  if (millis() < derniereDemande ) {
    timeClient.update();
    derniereDemande = millis();
  }

  // est-ce le moment de raffraichir la date indiquée?
  if ((millis() - derniereMaJ) >=   5000 ) {

    afficheHeure();
    calc_steps();                       // use this void to calculate if, and how many steps are neccecary, result is stored in the variable "steps"
    if (steps != 0)                     // only if a step is neccecary this part of the code is used
    {
      //Serial.print("steps:");
      //Serial.println(steps);
      move_stepper();
    }


    derniereMaJ = millis();
  }
}




void afficheHeure() {
  /*Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print("   -:");
  Serial.println(rtc_time);*/
}

void calibrate()
{
  //This void matches the physical time of the hands with the time the RTC prescibes.
  //After a power down the Arduino does not know where the hands of the clock are
  //Therefore we use the hall effect sensor to detect when the hands are at a certain time
  //after this we move from this known time to the actual time we want to display

  //Stepper1.enableOutputs(); //start using accelstepper libary
  stepper.enable();
  stepper.setRPM(dir * Step_fast_speed);             // set the stepper to use fast speed
  //Serial.println("Calibrating-Fast Mode......");
  while (digitalRead(Hall_pin))                         // while we have not reached the hall effect sensor
  {
    stepper.move(50 * dir); // run the engine at constant speed
  }
  delay(400);
  //Serial.println("Position found, moving back");
  stepper.move(-200 * dir);                             // move back a litte bit to compensate for overshoot of fast movement
  delay(400);
  stepper.setRPM(dir * Step_slow_speed);             // set the stepper to use slow speed
  //Serial.println("Calibrating-Slow Mode......");
  while (digitalRead(Hall_pin))                         // while we have not reached the hall effect sensor
  {
    stepper.move(20 * dir); // run the engine at constant speed
    delay(20);
  }
  delay(1500);                                          // delay to give the user a chance to read the time
  stepper.setRPM(dir * Step_fast_speed);             // set the engine back to fast mode

  //======================================
  //The next line sets the time at which the hands stop moving after hall effect sensor detection
  //in my case this is 5:49 or 17:49 (doesnt matter), change this accordingly
  clock_time = (5 * 60 + 30) * 60 ;   // Set the position of the arms (Year, Month, Day, Hour, minute, second) Year, Month and day are not important
  //======================================
  
}

//====================================================================================================================================
void calc_steps()
{
  seconds = (rtc_time - clock_time);       //calculate the difference between where the arms are and where they should be


  if (seconds > 43200)                        // we have an analog clock so it only displays 12 hours, if we need to turn 16 hours for example we can better turn 4 hours because its the same
  {
    seconds = seconds - 43200;
  }

  if (seconds > 21600)                        // if the difference is bigger than 6 hours its better to turn backwards
  {
    seconds = seconds - 43200;
  }
  steps = steps + (seconds / Sec_per_step);   //convert the seconds to steps
}


//====================================================================================================================================
void move_stepper()
{
  stepper.enable();
  stepper.move(dir * steps);           //tell the motor how many steps are needed and in which direction
  clock_time = rtc_time;                //since the steps will be made by the motor we can assume the position of the arms is equal to the new position
  steps = 0;
  stepper.disable();
}
