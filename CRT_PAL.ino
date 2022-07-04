#include <IRremote.h>
#include <Wire.h>

#define IR_RECEIVE_PIN  11
#define PUSHBUTTON_PIN  2

#define MODE_NOP        0
#define MODE_50HZ       1
#define MODE_PAL_COLOR  2


const int FIELD_RATE_TIMEOUT_DELAY = 1000; //50Hz setting is sticky, so only need to transmit every time input switches
const int PAL_COLOR_TIMEOUT_DELAY = 1;   //PAL color is not sticky, so need to transmit often or else TV MCU resets it

char gMode = 0;
int gFieldRateTimeout = 0;
int gPalColorTimeout = 0;


void setup() {
  Serial.begin(115200);               // start serial for output
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PUSHBUTTON_PIN, INPUT);
  gMode = MODE_NOP;
  IrReceiver.begin(IR_RECEIVE_PIN);
  IrReceiver.start();  
  Wire.begin();
}

void xmitPALCOLOR()
{
  //Disable color killer circuitry (otherwise software will think color PLL unlocked)
  Wire.beginTransmission(byte(0x8A)>>1);
  Wire.write(byte(0x21));
  Wire.write(byte(0x03));
  Wire.endTransmission();
  //Set decoding mode to PAL
  Wire.beginTransmission(byte(0x8A)>>1);
  Wire.write(byte(0x20));
  Wire.write(byte(0x2C));
  Wire.endTransmission();
}

void xmitENABLE50()
{ 
  //Allow 50Hz field rate
  Wire.beginTransmission(byte(0x8A)>>1);
  Wire.write(byte(0x25));
  Wire.write(byte(0x0C));
  Wire.endTransmission();
}

void blink (int times)
{
  for (int i = 0; i < times; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay (50);
    digitalWrite(LED_BUILTIN, LOW);
    delay (50);
  }
}

void nextMode ()
{
   switch (gMode) {
      case MODE_NOP:
        //MODE_NOP
        blink (1);
        gMode = MODE_50HZ;
        break;
      case MODE_50HZ:
        blink (2);
        gMode = MODE_PAL_COLOR;
        break;
      case MODE_PAL_COLOR:
        blink (3);
        gMode = MODE_NOP;
        break;
       }
}

void loop() {
  //do stuff based on mode
  if (gMode != MODE_NOP)
  {
    if (gMode == MODE_PAL_COLOR){
      gPalColorTimeout ++;
      if (gPalColorTimeout >= PAL_COLOR_TIMEOUT_DELAY)
      {
        xmitPALCOLOR(); //enable PAL color after enough cycles
        gPalColorTimeout = 0;
      } 
    }
    gFieldRateTimeout ++;
    if (gFieldRateTimeout >= FIELD_RATE_TIMEOUT_DELAY)
    {
      xmitENABLE50(); //enable 50Hz after enough cycles
      gFieldRateTimeout = 0;
    }
  }
 
  //read switch input (active low)
  if (digitalRead(PUSHBUTTON_PIN) == LOW)
  {
    //lazy debounce
    delay(10);
    if (digitalRead(PUSHBUTTON_PIN) == LOW)
    {
      nextMode();
    }
  }
  //check IR remote
  if (IrReceiver.decode())
  {
    //Prentend to be Monoprice Blackbird 4K 4:1 HDMI Switch
    if (IrReceiver.decodedIRData.address == 0x80)
    {
      switch (IrReceiver.decodedIRData.command) {
        case 0x1A:
          //MODE_NOP
          blink (1);
          gMode = MODE_50HZ;
          break;
        case 0x1E:
          blink (2);
          gMode = MODE_PAL_COLOR;
          break;
        case 0x12:
          blink (3);
          gMode = MODE_NOP;
          break;
         }
    }
    IrReceiver.resume();
  }
}
