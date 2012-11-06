#include <OneWire.h>
#include <Servo.h>
#include <PID_v1.h>

// Pins
#define ONE_WIRE_PIN 10
#define ONE_WIRE2_PIN 11
#define SERVO_PIN 9
#define THERMO_RELAY_PIN 13
#define POWER_RELAY_PIN 7
#define OK_LED_PIN 6

// Parameters
#define INITIAL_WATER_TEMP 12
#define LOW_WATER_TEMP 12
#define INITIAL_AMBIANT_SETPOINT 16

#define INITIAL_SERVO_ANGLE 30
#define MIN_SERVO_ANGLE 30
#define MAX_SERVO_ANGLE 130

// Sensors mapping
#define WATER_SENSOR 4
#define AMBIANT_SENSOR 1
#define OUTSIDE_SENSOR 2
#define GARAGE_SENSOR 0
#define MEZZANINE_SENSOR 0
#define EASTROOM_SENSOR 3

// Onewire
OneWire  ds(ONE_WIRE_PIN); 
OneWire  ds2(ONE_WIRE2_PIN); 
#define MAX_DS1820_SENSORS 16
byte addr[MAX_DS1820_SENSORS][8];
byte addr2[MAX_DS1820_SENSORS][8];

// Servo
Servo myservo; 

//Alarm flag
int alarm=0;
int alarm_count=0;
int errors=0;

// PID controler
double Setpoint, Input, Output;
int Angle=INITIAL_SERVO_ANGLE;
//PID pid(&Input, &Output, &Setpoint,2,5,2, DIRECT);
//PID pid(&Input, &Output, &Setpoint,0.2,1,1, DIRECT);
PID pid(&Input, &Output, &Setpoint,10,3,1, DIRECT);
int ambiant_setpoint = INITIAL_AMBIANT_SETPOINT;
int heater=0;    
int ref_temp_index=1;

// function to get the temperature from a given sensor
float get_temperature(OneWire ds, int sensor_id) {
  int HighByte, LowByte, TReading, SignBit, Tc_100;
  int sign=1;
  byte present = 0;
  byte data[12];
  int i;
  if ( addr[sensor_id][0] == 0x28) // We got a good sensor
    {     
        // Let's get the temperature      
        ds.reset();
        ds.select(addr[sensor_id]);
        ds.write(0x44,0);	   // start conversion, with parasite power off at the end
  
        delay(250); // wait a bit

        present = ds.reset();
        ds.select(addr[sensor_id]);
        ds.write(0xBE);	   // Read Scratchpad
  
        for ( i = 0; i < 9; i++) {	     // we need 9 bytes
          data[i] = ds.read();
        }
        
        // Float conversion
        LowByte = data[0];
        HighByte = data[1];
        TReading = (HighByte << 8) + LowByte;
        SignBit = TReading & 0x8000;  // test most sig bit
        if (SignBit) // negative
        {
          TReading = (TReading ^ 0xffff) + 1; // 2's comp
          sign=-1;
        }
        Tc_100 = (6 * TReading) + TReading / 4;    // multiply by (100 * 0.0625) or 6.25

        return Tc_100*1.00*sign / 100 ;
    }else{
        return -255;
    } 
}

void setup(void) 
{
  byte sensor;
 
  //Init serial line
  Serial.begin(9600);
  
  // Init servo pin
  //myservo.attach(9);
  
  // Init relay pin
  pinMode(THERMO_RELAY_PIN, OUTPUT); 
  digitalWrite(THERMO_RELAY_PIN, LOW);
  
  // Init OK led
  pinMode(OK_LED_PIN, OUTPUT);
  digitalWrite(OK_LED_PIN, LOW);
  
  // Init power relay pin (bus hard reset)
  pinMode(POWER_RELAY_PIN, OUTPUT);
  digitalWrite(POWER_RELAY_PIN, LOW);
  delay(3000);
  digitalWrite(POWER_RELAY_PIN, HIGH);
  delay(2000);
  
  // init PID
  pid.SetOutputLimits(MIN_SERVO_ANGLE,MAX_SERVO_ANGLE); 		
  Output = INITIAL_SERVO_ANGLE;
  Setpoint = INITIAL_WATER_TEMP; 
  pid.SetMode(AUTOMATIC);
   
  
  // Reset of the OneWire bus
  ds.reset();
  ds2.reset();
  delay(500);
  ds.reset_search();
  ds2.reset_search();
  
  //Search for sensors on bus1
  boolean found=false;
  for (sensor=0;sensor<MAX_DS1820_SENSORS;sensor++)
  { 
    if (!ds.search(addr[sensor]) && !found) 
    {
      Serial.print("Found ");
      Serial.print(sensor,16);
      Serial.print(" sensors\n");
      ds.reset_search();
      delay(500);
      found=true;
    }
    // Better doing this into a while and search for the next good sensor:
    if ( found && OneWire::crc8( addr[sensor], 7) != addr[sensor][7]) 
    {
      Serial.print("CRC is not valid\n");
      return;
    }
    if ( found ) {
      digitalWrite(OK_LED_PIN, HIGH);
    }
  }
  
  //Search for sensors on bus2
  for (sensor=0;sensor<MAX_DS1820_SENSORS;sensor++)
  { 
    if (!ds2.search(addr2[sensor])) 
    {
      Serial.print("Found ");
      Serial.print(sensor,16);
      Serial.print(" sensors\n");
      ds2.reset_search();
      delay(500);
      return;
    }
    // Better doing this into a while and search for the next good sensor:
    if ( OneWire::crc8( addr2[sensor], 7) != addr2[sensor][7]) 
    {
      Serial.print("CRC is not valid\n");
      return;
    }
  }
}


void loop(void) 
{    
  byte sensor;
  float ambiant_temp, outside_temp, garage_temp, water_temp, eastroom_temp, ref_temp; 
  int cmd;
  int i;
  int first_digit;
  int second_digit;
  byte trash;
  
  // Garage temperature
  garage_temp=get_temperature(ds,GARAGE_SENSOR);
      
  // Eastroom temperature
  eastroom_temp=get_temperature(ds,EASTROOM_SENSOR);
      
  // Outside temperature
  outside_temp=get_temperature(ds,OUTSIDE_SENSOR);
      
  // Sonde vanne 4 voies
  water_temp=get_temperature(ds,WATER_SENSOR);

  // Sonde ambiante
  ambiant_temp=get_temperature(ds,AMBIANT_SENSOR);

  // Reference temp
  ref_temp=ambiant_temp;
  if (ref_temp_index == 2) { ref_temp=eastroom_temp; }; 
  //if (ref_temp_index == 3) { ref_temp=mezzanine_temp; }; 
 
  // Regul température eau de chauffage
  if (water_temp < 5 || water_temp > 99) {
    // Exception case, closing valve for security
    Angle=MIN_SERVO_ANGLE;
    alarm_count++;
    if (alarm_count > 2) { alarm=1; }
  }else{
    Input=water_temp;
    pid.Compute();
    Angle=Output;
    alarm=0;
    alarm_count=0;
  };
  // By attaching the servo during only 1 second, we prevent 
  // it from behing stressed too much (I burned one...)
  myservo.attach(SERVO_PIN);
  myservo.write(Angle);
  delay(1000);
  myservo.detach();

  // Thermostat
  if (ambiant_temp < 1 || ambiant_temp >  40) {
     // There must be a problem, turning off!
     heater=0;
     digitalWrite(THERMO_RELAY_PIN, LOW);
     Setpoint=LOW_WATER_TEMP;
     alarm_count++;
     if (alarm_count > 2) { alarm=1; }
  }else{
      alarm=0;
      alarm_count=0;
      // Thermostat
    if (ref_temp < (ambiant_setpoint - 0.1)) {
      // Activate the heater relay
      digitalWrite(THERMO_RELAY_PIN, HIGH);
      // Boost the opening of the  valve
      // at heater power on
      if (heater == 0) {
        Output = INITIAL_SERVO_ANGLE;
      }
      // Set the flag on
      heater=1;
    };
    if (ref_temp > (ambiant_setpoint + 0.1)) {
      // Close the heater relay
      digitalWrite(THERMO_RELAY_PIN, LOW);
      // Set the flag off
      heater=0;
      // Set the heater valve to cold water
      Setpoint=LOW_WATER_TEMP;
    };
  };
  // This is the most important part of the code!
  if (heater == 1) {
      // Open the heater valve to a temperature that is
      // a function of the outside air temperature and
      // the difference between the setpoint and the current
      // ambiant temperature.
      Setpoint=53-outside_temp+(ambiant_setpoint-ref_temp)*5;
  }
  
   
  // Read commands
  // T#: set a new ambiant setpoint
  if (Serial.available() == 3) {
    cmd=Serial.read();
    first_digit=Serial.read();
    second_digit=Serial.read();
    // Set ambiant temperature setpoint
    if (cmd == 'T') {
      ambiant_setpoint=(first_digit-'0')*10 + (second_digit-'0');
      Serial.println("OK");
    }
  // R#: set a new temp reference
    if (cmd == 'R') {
      ref_temp_index=second_digit-'0';
      Serial.println("Setting ref temp OK");
    }
  }
  //P: print temperatures ans status
  if (Serial.available() == 1) {
    cmd=Serial.read();
    if (cmd == 'P') {
        Serial.print("TG: ");
        Serial.println(garage_temp);
        Serial.print("TE: ");
        Serial.println(eastroom_temp);
        Serial.print("TO: ");
        Serial.println(outside_temp);
        Serial.print("TA: ");
        Serial.println(ambiant_temp);
        Serial.print("TW: ");
        Serial.println(water_temp);
        Serial.print("REF: ");
        Serial.println(ref_temp);
        Serial.print("HEATER: ");
        Serial.println(heater);
        // Print servo position angle
        Serial.print("SP: ");
        Serial.println(Angle);
        // Print water temperature setpoint
        Serial.print("SW: ");
        Serial.println(Setpoint);
        // Print ambiant temperature setpoint
        Serial.print("SA: ");
        Serial.println(ambiant_setpoint);
        // Print alarm
        Serial.print("AL: ");
        Serial.println(alarm);
        // Print number of errors
        Serial.print("ER: ");
        Serial.println(errors);
        // Print reference temp index
        Serial.print("REFIDX: ");
        Serial.println(ref_temp_index);
    }
  }
  
  // Hard reset the bus if there's a problem
  if (alarm == 1) {
    errors++;
    digitalWrite(OK_LED_PIN, LOW);
    digitalWrite(POWER_RELAY_PIN, LOW);
    delay(3000);
    digitalWrite(POWER_RELAY_PIN, HIGH);
    delay(2000);
  }else {
     digitalWrite(OK_LED_PIN, HIGH);
  }
  
  // Clean serial buffer
  for (i=0;i<Serial.available();i++)
  {
    trash=Serial.read();
  }
  
  delay(5000);
}

