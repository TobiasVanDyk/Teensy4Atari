/////////////////////////////////////////////////////////////////////////////////////////////
// Atari 800 emulation using a Teensy 4.0's own hub and standard USB keyboard for input. 
// This is a modification of Jean-MarcHarvengt's Multi CompUter Machine Emulator.
/////////////////////////////////////////////////////////////////////////////////////////////
// Teensy 4.1 using built-in SDCard
// Sound is working SOUND 2,100,10,10
// Not necessary to change pins_arduino.h in teensy4 folder, PIN_SPI_SS leave it at (10) 
////////////////////////////////////////////////////////////////////////////////////////////
#include "USBHost_t36.h"

USBHost myusb;
USBHub hub1(myusb);
KeyboardController keyboard1(myusb);
USBHIDParser hid1(myusb);
USBHIDParser hid2(myusb);

USBDriver *drivers[] = {&hub1, &keyboard1, &hid1, &hid2};
#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
const char * driver_names[CNT_DEVICES] = {"Hub1", "KB1", "HID1" , "HID2"};
bool driver_active[CNT_DEVICES] = {false, false, false, false};

extern int usbkey;
extern int usboldkey;     // Left Right
extern int usbshiftkey;   // 104  108
extern int usbcontrolkey; // 103  107
extern int usbcapslock;   // 57
extern int usbwindowskey; // 106
extern int usbaltkey;     // 105 109
extern int usbarrowup;    // 82 key218
extern int usbarrowdown;  // 81 key217
extern int usbarrowright; // 79 key215
extern int usbarrowleft;  // 80 key216
extern int SHFLOK;        // Location 702 0, 64 (all uppercase), 128, 196 Shift Ctrl
extern int usbF6;         // 63 key199 Start
extern int usbF7;         // 64 key200 Select
extern int usbF8;         // 65 key201 Option
extern int usbF9;         // 66 key202 Reset
extern int usbenter;      // 10 key12

extern int led15;

extern "C" {
  #include "iopins.h"  
  #include "emuapi.h"  
}

#include "tft_t_dma.h"

extern "C" {
#include "atari800.h"
}

extern bool menuActive(void);
extern char * menuSelection(void);
extern void toggleMenu(bool on);
extern int  handleMenu(uint16_t bClick);

TFT_T_DMA tft = TFT_T_DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO, TFT_TOUCH_CS, TFT_TOUCH_INT);

static IntervalTimer myTimer;
static elapsedMicros tius;

static unsigned char  palette8[PALETTE_SIZE];
static unsigned short palette16[PALETTE_SIZE];

volatile boolean vbl = true;
static int skip = 0;
static void vblCount() // toggle vbl
{ if (vbl) vbl = false; else vbl = true; }

void emu_DrawVsync(void)
{ volatile boolean vb=vbl;
  skip += 1;
  skip &= VID_FRAME_SKIP;
  while (vbl==vb) {}; // in loop until vbl changed external?
}

void emu_DrawLine(unsigned char * VBuf, int width, int height, int line) 
{ tft.writeLine(width, 1, line, VBuf, palette16); }  

void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride) 
{ if (skip==0) { tft.writeScreen(width, height-TFT_VBUFFER_YCROP, stride, VBuf+(TFT_VBUFFER_YCROP/2)*stride, palette16); } }

int emu_FrameSkip(void)
{ return skip; }

void * emu_LineBuffer(int line)
{ return (void*)tft.getLineBuffer(line); }

void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index)
{  if (index<PALETTE_SIZE) 
      { palette8[index]  = RGBVAL8(r,g,b);
        palette16[index] = RGBVAL16(r,g,b);    
      }
}

///////////////////////////////////////////////////////
// setup runs once on sketch start
///////////////////////////////////////////////////////
void setup() 
{
  tft.begin();
  
  pinMode(led15, OUTPUT); 
  
  usboldkey = 255;
  usbkey = 255;
  usbshiftkey = 0;
  usbcontrolkey = 0;
  usbcapslock = 64;
  usbarrowdown = 0 ;
  usbarrowup = 0 ;
  usbarrowright = 0 ;
  usbarrowleft = 0 ;
  usbenter = 0;
  usbF6 = usbF7 = usbF8 = usbF9 = 0 ; //Start Select Option Reset
  
  led15 = 15;
  digitalWrite(led15, LOW);
  
  emu_init();                         // lowercase init
  
  myTimer.begin(vblCount, 20000);     //to run every 20ms

  myusb.begin();
  
  keyboard1.attachPress(OnPress);
  keyboard1.attachRawPress(OnRawPress);
  keyboard1.attachRawRelease(OnRawRelease);

}

///////////////////////////////////////////////////////
// Main Loop 
// 1. menuActive => initial ROM selection
// 2. Then input from keyboard or joystick or buttons
///////////////////////////////////////////////////////
void loop(void) 
{
  // 1. ///////////////////////////////////////////////
  if (menuActive()) 
     { uint16_t bClick = emu_DebounceLocalKeys();             // 0 or > 0
       int action = handleMenu(bClick);                       // action = ACTION_NONE or ACTION_RUNTFT
       char * filename = menuSelection();    
       if (action == ACTION_RUNTFT) 
          { toggleMenu(false);                               // Switch off initial menu
            emu_Init(filename);
            tft.fillScreenNoDma( RGB16_BLACK );              // black
            tft.startDMA(); 
          } 
     // 2. ///////////////////////////////////////////////   
     } else                                             // if (!menuActive()) 
     { emu_Step();                                      // same as at8_Step
       uint16_t bClick = emu_DebounceLocalKeys();       // 0 or > 0 
       emu_Input(bClick);                               // { }; do nothing
     }
  
  myusb.Task();   // see teensy_usbhost
  
}


////////////// HAS_SND Start ////////////////////////
#ifdef HAS_SND

#include <Audio.h>
#include "AudioPlaySystem.h"
AudioPlaySystem mymixer;

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)    
    AudioOutputMQS  mqs;
    AudioConnection patchCord9(mymixer, 0, mqs, 1);
 #else
    AudioOutputAnalog dac1;
    AudioConnection   patchCord1(mymixer, dac1);
#endif

void emu_sndInit() 
{ Serial.println("Sound init");  
  AudioMemory(16);
  mymixer.start();
}

void emu_sndPlaySound(int chan, int volume, int freq)
{ if (chan < 6) mymixer.sound(chan, freq, volume); }

void emu_sndPlayBuzz(int size, int val) 
{ mymixer.buzz(size,val); }

#endif
////////////// HAS_SND End ////////////////////////

void OnRawPress(uint8_t keycode)
{ Serial.print("raw key press: ");
  Serial.println((int)keycode);

  if (((keycode==104) || (keycode==108)) && (usbshiftkey==0))   usbshiftkey   = keycode; // Only stays >0 while pressed down
  if (((keycode==103) || (keycode==107)) && (usbcontrolkey==0)) usbcontrolkey = keycode; // Only stays >0 while pressed down
  if (keycode==57) { if (usbcapslock==64)  usbcapslock = 0; else usbcapslock = 64; }     // Toggle 64 or 0 CAPSLOCK 64 0x40 0100 00 00
  if (keycode==40) usbenter = 1;
}

void OnRawRelease(uint8_t keycode)
{ Serial.print("raw key release: ");
  Serial.println((int)keycode);

  if (((keycode==104) || (keycode==108)) && (usbshiftkey!=0))  usbshiftkey = 0;     // Only stays >0 while pressed down
  if (((keycode==103) || (keycode==107)) && (usbcontrolkey!=0)) usbcontrolkey = 0;  // Only stays >0 while pressed down
}

void OnPress(int key)
{ usbkey = key;                  // Also used in main loop
  usboldkey = 255;               // Can then press the same key more than once
  
  Serial.print("KEY:");
  Serial.println(key);
  
  if (key==215) { usbarrowright = 1 ; }
  if (key==216) { usbarrowleft = 1 ; }
  if (key==217) { usbarrowdown = 1 ; }
  if (key==218) { usbarrowup = 1 ; }

  if (key==199) { usbF6 = 1 ; }
  if (key==200) { usbF7 = 1 ; }
  if (key==201) { usbF8 = 1 ; }
  if (key==202) { usbF9 = 1 ; }

  //if ((usbF6>0) || (usbF7>0) || (usbF8>0) || (usbF9>0)) Serial.println("F6-F9 pressed");
}
