
// Adafruit provides a convenient library!
#include <Adafruit_MCP4725.h>

//This is the I2C Address of the MCP4725, by default (A0 pulled to GND).
//For devices with A0 pulled HIGH, use 0x63
#define MCP4725_ADDR 0x62


// Instantiate the convenient class
Adafruit_MCP4725 dac;

const int HANDSHAKE = 0;
const int VOLTAGE_REQUEST = 1;
const int ON_REQUEST = 2;
const int STREAM = 3;
const int READ_DAQ_DELAY = 4;
const int SCAN_RATE = 5;
const int V_MIN = 6;
const int V_MAX = 7;
const int N_CYCLES = 8;


const int readPin1 = A0;
const int readPin2 = A1;
const int diffPin = 3;
int scanRate = 100;
int n_cycles = 10;
int cycle_count = 0;

String scanRateStr, Vdiff_minStr, Vdiff_maxStr, n_cyclesStr;

uint16_t x = (uint16_t) 0;

const unsigned long sampleDelay = 10;
unsigned long lastSampleTime = 0;
unsigned long startTime = 0;

// Initially, only send data upon request
int daqMode = ON_REQUEST;

// Default time between data acquisition is 100 ms
int daqDelay = 100;

// String to store input of DAQ delay
String daqDelayStr;


// Keep track of last data acquistion for delays
unsigned long timeOfLastDAQ = 0;

// Max and min voltage after differential amplifier
float V_min = -1.5;
float V_max = 1.5;

// Calculating V_slope for triangle wave
int V_diff = (V_max-V_min) * 4095/3.0;
float E_step = V_diff / 4095.0;
float V_slope = E_step / (scanRate/1000.0);

// Calculating shift down for differential amplifier
int V_diff_amp = -255*(V_min+V_max)/ 2;


void change_V_range(float V_min, float V_max){
  
  V_diff = (V_max-V_min) * 4095/3.0;
  E_step = V_diff / 4095.0;
  V_slope = E_step / (scanRate/1000.0);
  
  V_diff_amp = -255*(V_min+V_max)/ 2;

  cycle_count = 0;
  
  analogWrite(diffPin, V_diff_amp);
  
}

void triangleWave(unsigned long currTime){
  int time_val = int(V_slope *(currTime-startTime)) % (V_diff*2);

   if (time_val < V_diff+1){
     x = (uint16_t)(time_val);
     if(time_val==V_diff){
      cycle_count ++;
     }
    
   }
   
   else{
    x = (uint16_t)(int((V_diff*2)-time_val));
   }
  
  dac.setVoltage(x, false);
}

unsigned long printVoltage() {
  // Read value from analog pin
  int triangle = analogRead(readPin1);
  int cyclic_volt = analogRead(readPin2);

  // Get the time point
  unsigned long timeMilliseconds = millis();

  // Write the result
  if (Serial.availableForWrite()) {
    String outstr = String(String(timeMilliseconds, DEC) + "," + String(triangle, DEC) + "," + String(cyclic_volt, DEC));
    Serial.println(outstr);
  }

  // Return time of acquisition
  return timeMilliseconds;
}


void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  dac.begin(MCP4725_ADDR);

  pinMode(readPin1, INPUT);
  pinMode(readPin2, INPUT);
  startTime = millis();

  pinMode(diffPin, OUTPUT);

  analogWrite(diffPin, V_diff_amp);
}


void loop() {
  // If we're streaming
  if (daqMode == STREAM) {
    if(cycle_count < n_cycles + 1){
      unsigned long currTime = millis();
      triangleWave(currTime);
      if (millis() - timeOfLastDAQ >= daqDelay) {
        timeOfLastDAQ = printVoltage();
      }
    }
  }

  // Check if data has been sent to Arduino and respond accordingly
  if (Serial.available() > 0) {
    // Read in request
    int inByte = Serial.read();
  
    // If data is requested, fetch it and write it, or handshake
    switch(inByte) {
      case VOLTAGE_REQUEST:
        timeOfLastDAQ = printVoltage();
        break;
      case ON_REQUEST:
        daqMode = ON_REQUEST;
        break;
      case STREAM:
        daqMode = STREAM;
        break;
      case READ_DAQ_DELAY:
        // Read in delay, knowing it is appended with an x
        daqDelayStr = Serial.readStringUntil('x');
  
        // Convert to int and store
        daqDelay = daqDelayStr.toInt();
  
        break;
      case HANDSHAKE:
        if (Serial.availableForWrite()) {
          Serial.println("Message received.");
        }
        break;
      case SCAN_RATE:
        scanRateStr = Serial.readStringUntil('x');
        scanRate = scanRateStr.toInt();
        change_V_range(V_min, V_max);
        break;
      case V_MIN:
        Vdiff_minStr = Serial.readStringUntil('x');
        V_min = Vdiff_minStr.toInt();
        change_V_range(V_min, V_max);
        break;
      case V_MAX:
        Vdiff_maxStr = Serial.readStringUntil('x');
        V_max = Vdiff_maxStr.toInt();
        change_V_range(V_min, V_max);
        break;
      case N_CYCLES:
        n_cyclesStr = Serial.readStringUntil('x');
        n_cycles = n_cyclesStr.toInt();
        cycle_count = 0;
        break;
      }
    }
}
