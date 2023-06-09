//  With code borrowed and adapted from here
//  https://diyelectromusic.wordpress.com/2021/06/23/arduino-mozzi-sample-drum-sequencer/
#include <MozziGuts.h>
#include <Sample.h>
#include "d_kit.h"

#define OUTPUTSCALING 9 // 10 bit scaling factor

#define tempoPotentiometer   A0

#define DRUMS 4 // four different drum types
#define BD  1   // bass drum
#define SD  2   // snare drum
#define CH  3   // closed hi-hat
#define OH  4   // open hi-hat
int drums[DRUMS] = {BD,SD,OH,CH};

Sample <BD_NUM_CELLS, AUDIO_RATE> bassDrum(BD_DATA);
Sample <SD_NUM_CELLS, AUDIO_RATE> snareDrum(SD_DATA);
Sample <CH_NUM_CELLS, AUDIO_RATE> closedHat(CH_DATA);
Sample <OH_NUM_CELLS, AUDIO_RATE> openHat(OH_DATA);

// #define LED_PIN LED_BUILTIN
#define NUM_KEYS 32

#define BEATS 8
uint8_t pattern[BEATS][DRUMS];
unsigned long nexttick; // for keeping track of time during loop (tick until next step)
int step; // step of the sequence out of 8
uint16_t tempo = 260;
// int led_num = 2; // for turning on leds while going through
byte loopPattern;
byte playMode;

uint8_t keys[NUM_KEYS] = {
  0x14,0x24,0x34,0x44,0x54,0x64,0x74,0x84,  // Drum 4, beats 1,2,3,4,5,6,7,8
  0x13,0x23,0x33,0x43,0x53,0x63,0x73,0x83,  // Drum 3, beats 1,2,3,4,5,6,7,8
  0x12,0x22,0x32,0x42,0x52,0x62,0x72,0x82,  // Drum 2, beats 1,2,3,4,5,6,7,8
  0x11,0x21,0x31,0x41,0x51,0x61,0x71,0x81,  // Drum 1, beats 1,2,3,4,5,6,7,8
};

void startDrum (int drum) {
  switch (drum) {
    case BD: bassDrum.start(); break;
    case SD: snareDrum.start(); break;
    case CH: closedHat.start(); break;
    case OH: openHat.start(); break;
  }
}
void(* resetFunction) (void) = 0; //declare reset function @ address 0

void setup () {
  pinMode(tempoPotentiometer, INPUT);
  startMozzi();
  bassDrum.setFreq((float) D_SAMPLERATE / (float) BD_NUM_CELLS);
  snareDrum.setFreq((float) D_SAMPLERATE / (float) SD_NUM_CELLS);
  closedHat.setFreq((float) D_SAMPLERATE / (float) CH_NUM_CELLS);
  openHat.setFreq((float) D_SAMPLERATE / (float) OH_NUM_CELLS);
  loopPattern = 0; 
  Serial.begin(115200);  // uses serial to set pattern
}

void updateControl() {
  // timing mechanism
  if(playMode == 1) {
    unsigned long timenow = millis();
    if (timenow >= nexttick) {                            // if current time exceeds time reuired for beat
      nexttick = millis() + (unsigned long)(60000/tempo); // this beat should take (1 minute)/(set tempo) amount of time
      step++; // increment step
      if (step >= BEATS){ // if step has reached 8, restart at 0
        step = 0;
      }
      for (int d = 0; d<DRUMS; d++) {   // for all the drums at the given step, if set to be on then play to audio
        if (pattern[step][d] != 0) {
          startDrum(pattern[step][d]);
        }
      }
    }
    if(Serial.available()){
      char inputChar = Serial.readString().charAt(0);
      if(inputChar == '9') {
        playMode = 0;
        stopMozzi();
      } else if(inputChar == '1') {
        resetFunction();
      }
    }
  } else {
  // The char layout for the drum machine
  /*
    |     (X)  (Y)  (Z)  ([)  (\)  (])  (^)  (_)    |
    |     (`)  (a)  (b)  (c)  (d)  (e)  (f)  (g)    |
    |     (h)  (i)  (j)  (k)  (l)  (m)  (n)  (o)    |
    |     (p)  (q)  (r)  (s)  (t)  (u)  (v)  (w)    |
  */
    while(Serial.available()) { 
      char ch = Serial.read();
      Serial.println(ch); 
      if (ch >= 'X' && ch <= 'x') { 
        uint8_t charVal = ch - 'X';       // char provided in serial as index of keys array
        uint8_t key = keys[charVal]; 
        int drum = (key & 0x0F) - 1;      // drum number from key value (out of 4)
        int beat = (key >> 4) - 1;        // beat number (out of 8)
        if (pattern[beat][drum] != 0) {   // if beat is on, then turn off
          pattern[beat][drum] = 0;
        } else { 
          pattern[beat][drum] = drums[drum]; // otherwise turn on (1-4 for the different drum types)
        } 
      } else if(ch == '1') {
        resetFunction();
      } else if(ch == '9') {
        playMode = 1;
        startMozzi();
      }
    }
  } 
      
  int newTempo = 50 + (mozziAnalogRead (tempoPotentiometer) >> 2);
  if (newTempo != tempo) {
      tempo = newTempo;  // Tempo range is 50 to 305.
  }
}

AudioOutput_t updateAudio(){
  // combine all currently recorded samples at their current phases and reset to zero
  int16_t blended_sample = bassDrum.next() + snareDrum.next() + closedHat.next() + openHat.next(); 
  return MonoOutput::fromNBit(OUTPUTSCALING, blended_sample); // (bs*40)>>8 scale to 10 bit value, reduced volume
}

void loop(){
  audioHook();
}
