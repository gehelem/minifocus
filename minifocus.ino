 

//motor
#define MyDirX      5
#define MyStepX     2
#define MyEnable    8
#define CS_PIN      7
#define HomePin     9
//encoder
#define mult    16



#include <TMC2130Stepper.h>
TMC2130Stepper driver = TMC2130Stepper(MyEnable, MyDirX, MyStepX, CS_PIN);

#include <AccelStepper.h>
AccelStepper stepper = AccelStepper(stepper.DRIVER, MyStepX, MyDirX);



#define MAXCOMMAND 8
char inChar;
char cmd[MAXCOMMAND];         // these are for handling and processing serial commands
char param[MAXCOMMAND];
char line[MAXCOMMAND];
boolean eoc = false;    // end of command
int idx = 0;    // index into command string
 
long pos=500;
bool isEnabled;
bool isGoingHome = false;

void setup() {

  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  pinMode(HomePin, INPUT_PULLUP);
  digitalWrite(CS_PIN, HIGH);
  driver.begin();             // Initiate pins and registeries
  driver.rms_current(600);    // Set stepper current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  driver.stealthChop(1);      // Enable extremely quiet stepping
  driver.stealth_autoscale(1);
  driver.microsteps(mult);


  stepper.setMaxSpeed(4000000); // 100mm/s @ 80 steps/mm
  stepper.setAcceleration(15000); // 2000mm/s^2
  stepper.setEnablePin(MyEnable);
  stepper.setPinsInverted(false, false, true);
  //stepper.disableOutputs();
  stepper.enableOutputs();
  isEnabled = true;
  
  stepper.setCurrentPosition(mult*pos);

  Serial.begin (9600);
  //stepper.setSpeed(4000000);
 
 
}
 
void loop() {

  
  if (!stepper.isRunning() && isEnabled) {
      stepper.disableOutputs();
      delay(100);
      isEnabled = false;
  }
  stepper.run();

  if (isGoingHome) {
    int val = digitalRead(HomePin);
    if (val==HIGH)  
    {
    } 
    else 
    {
      stepper.setCurrentPosition(0);
      stepper.disableOutputs();      
      isGoingHome=false;
      pos=0;
    }
  }
 
  while (Serial.available() && !eoc) {
    inChar = Serial.read();
    //Serial.print(inChar);
    if (inChar != '#' && inChar != ':') {
      line[idx++] = inChar;
      if (idx >= MAXCOMMAND) {
        idx = MAXCOMMAND - 1;
      }
      
    } else {
      if (inChar == '#') {
        eoc = true;
      }
    }
  }  

  // process the command string when a hash arrives:
  if (eoc) {
    memset(cmd, 0, MAXCOMMAND);
    memset(param, 0, MAXCOMMAND);
    
    int len = strlen(line);
    if (len >= 2) {
      strncpy(cmd, line, 2);
    }
    if (len > 2) {
      strncpy(param, line + 2, len - 2);
    }
    
    memset(line, 0, MAXCOMMAND);
    eoc = false;
    idx=0;

    // --------------------------------------------------------------------------------
    // get the current focuser position
    if (!strcasecmp(cmd, "GP")) {
      char tempString[6];
      sprintf(tempString, "%04X", stepper.currentPosition()/mult);
      Serial.print(tempString);
      Serial.print("#");
    }  

    // whether half-step is enabled or not, always return "00"
    else if (!strcasecmp(cmd, "GH")) { Serial.print("00#"); }
    // version
    else if (!strcasecmp(cmd, "GV")) { Serial.print("01#"); }    

    // --------------------------------------------------------------------------------
    // get the current temperature - moonlite compatible
    else if (!strcasecmp(cmd, "GT")) {
      char tempString[6];
      sprintf(tempString, "%04X", 20);
      Serial.print(tempString);
      Serial.print("#");
    } 
    
    // --------------------------------------------------------------------------------
    // get the current motor step delay, only values of 02, 04, 08, 10, 20
    // not used so just return 02
    else if (!strcasecmp(cmd, "GD")) {
      char tempString[6];
      sprintf(tempString, "%02X", 2);
      Serial.print(tempString);
      Serial.print("#");
    }

    // --------------------------------------------------------------------------------
    // motor is moving - 1 if moving, 0 otherwise
    else if (!strcasecmp(cmd, "GI")) {
      if (stepper.isRunning() ) {
        Serial.print("01#");
      }
      else {
        Serial.print("00#");
      }
    }

    // --------------------------------------------------------------------------------
    // initiate a move to the target position
    else if (!strcasecmp(cmd, "FG")) {
      stepper.enableOutputs();
      delay(100);      
      isEnabled=true; 
      stepper.moveTo(pos*mult);
    }

    // --------------------------------------------------------------------------------
    // handle home command
    else if (!strcasecmp(cmd, "PH")) {
    int val = digitalRead(HomePin);
      if (val==HIGH)  
      {
        stepper.setCurrentPosition(65535*mult);
        stepper.enableOutputs();
        delay(100);      
        stepper.moveTo(-65535);
        isEnabled = true;
        isGoingHome=true; 
      }       
    }

    // --------------------------------------------------------------------------------
    // stop a move - HALT
    else if (!strcasecmp(cmd, "FQ")) {
      stepper.stop();
      stepper.disableOutputs();
      delay(100);         
      isEnabled=false; 

    }

    // --------------------------------------------------------------------------------
    // set current position to received position - no move SPXXXX
    // in INDI driver, only used to set to 0 SP0000 in reset()
    else if (!strcasecmp(cmd, "SP")) {
      pos = hexstr2long(param);
      if ( pos > 65535 ) pos = 65535;
      if ( pos < 0 ) pos = 0;
      stepper.setCurrentPosition(mult*pos);
    }

    // --------------------------------------------------------------------------------
    // set new target position SNXXXX - this is a move command
    // but must be followed by a FG command to start the move
    else if (!strcasecmp(cmd, "SN")) {
      pos = hexstr2long(param);
      if ( pos > 65535 ) pos = 65535;
      if ( pos < 0 ) pos = 0;
      //serialposition = pos;
    }

    // --------------------------------------------------------------------------------
    else if (!strcasecmp(cmd, "SD")) {
      //pos = hexstr2long(param);
      //stepdelay = (int) pos;
    }
    

  }    
 
}
 


long hexstr2long(char *line) {
  long ret = 0;

  ret = strtol(line, NULL, 16);
  return (ret);
}
