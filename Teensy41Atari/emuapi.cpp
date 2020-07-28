extern "C" {
  #include "emuapi.h"
  #include "iopins.h"
}

#include "tft_t_dma.h"

#ifdef USE_SDFS
#include "uSDFS.h"
static FATFS fatfs;
static FIL file; 
#else

#ifdef USE_SDFAT
#include "SdFat.h"
#include "sdios.h"

#ifndef SDCARD_SS_PIN
const uint8_t SD_CS_PIN = SS;

#else  // SDCARD_SS_PIN
// Assume built-in SD is used.
const uint8_t SD_CS_PIN = SDCARD_SS_PIN;
#endif  // SDCARD_SS_PIN

#define SD_CONFIG SdioConfig(FIFO_SDIO)

// Try to select the best SD card configuration.
#if HAS_SDIO_CLASS
#define SD_CONFIG SdioConfig(FIFO_SDIO)
#elif ENABLE_DEDICATED_SPI
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI)
#else  // HAS_SDIO_CLASS
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, SHARED_SPI)
#endif  // HAS_SDIO_CLASS


#if SD_FAT_TYPE == 0
#elif SD_FAT_TYPE == 1
#define SdFat SdFat32
#define File File32
#elif SD_FAT_TYPE == 2
#define SdFat SdExFat
#define File ExFile
#elif SD_FAT_TYPE == 3
#define SdFat SdFs
#define File FsFile
#endif  // SD_FAT_TYPE

static SdFat SD;
#else
#include <SD.h>
#endif
static File file;
#endif

int usbarrowup;    // 82 key218 Ctrl
int usbarrowdown;  // 81 key217 Ctrl
int usbarrowright; //  
int usbarrowleft;  //   
int usbenter;

/////////////////////////////////////////////////////////////////
// First Menu keys and size before operating system active
/////////////////////////////////////////////////////////////////
#define MKEY_RIGHT          18
#define MKEY_LEFT           19
#define MKEY_UP             20
#define MKEY_DOWN           21
#define MKEY_JOY            22                                 // Used to swop joysticks
#define MKEY_TFT            23

#define MAX_FILES           32                                 // Increase this
#define MAX_FILENAME_SIZE   40                                 // 40*8=320 but dos names = 8.3 = 12 char used?
#define MAX_MENULINES       15                                 // 15 
#define TEXT_HEIGHT         16                                 // 240/16 = 15 16 is doubleheight fonts
#define TEXT_WIDTH          8                                  // 320/8 = 40
#define MENU_FILE_XOFFSET   0                                  // 
#define MENU_FILE_YOFFSET   0                                  // start at x,y 0,0
#define MENU_FILE_W         (MAX_FILENAME_SIZE*TEXT_WIDTH)     // 40*8=320
#define MENU_FILE_H         (MAX_MENULINES*TEXT_HEIGHT)        // 15*16=240
#define MENU_FILE_BGCOLOR   RGBVAL16(0x00,0x00,0x90)           // dark blue normal background
#define MENU_FILE_FGCOLOR   RGBVAL16(0xE0,0xEF,0xFF)           // white blue normal text
#define MENU_FILE_CGCOLOR   RGBVAL16(0xE0,0xEF,0xFF)           // white blue Reverse colours for selected text
#define MENU_FILE_SGCOLOR   RGBVAL16(0x00,0x00,0x90)           // dark blue

#define RGB16_BLACK         RGBVAL16(0x00,0x00,0x00)           // black
#define RGB16_WHITE         RGBVAL16(0xff,0xff,0xff)           // white
#define RGB16_RED           RGBVAL16(0xff,0x00,0x00)           // red
#define RGB16_GREEN         RGBVAL16(0x00,0xff,0x00)           // green
#define RGB16_BLUE          RGBVAL16(0x00,0x00,0xff)           // blue

extern TFT_T_DMA tft;

static char romspath[64];                                      // folder + name + extension
static int nbFiles=0;
static int curFile=0;
static int topFile=0;
static char selection[MAX_FILENAME_SIZE+1]="";
static char files[MAX_FILES][MAX_FILENAME_SIZE];               // files [32][64] = 2k
static bool menuRedraw=true;

static int keypadval=0; 
static boolean joySwapped = false;
static uint16_t bLastState;
static int xRef;
static int yRef;
static uint8_t prev_zt=0; 

static bool menuOn=true;                                       // In file ROM selection phase

///////////////////////////////////////
// emu_printx
///////////////////////////////////////
void emu_printf(char * text)
{ Serial.println(text); }

void emu_printf(int val)
{ Serial.println(val); }

void emu_printi(int val)
{ Serial.println(val); }

void emu_printh(int val)
{ Serial.println(val,HEX); }

static int malbufpt = 0;
static char malbuf[EXTRA_HEAP]; // 0x10 256

////////////////////////////////////////////
// size = 0-0xC000 48k+1 or 0-0x10000 65k+1
////////////////////////////////////////////
void * emu_Malloc(int size)
{
  void * retval =  malloc(size);
  if (!retval) 
     { emu_printf("failed to allocate");
       emu_printf(size);
       // emu_printf("fallback");
       if ( (malbufpt+size) < sizeof(malbuf) ) 
          { retval = (void *)&malbuf[malbufpt];
            malbufpt += size;      
          } else 
          { emu_printf("failure to allocate");
          }
     } else 
     { emu_printf("could allocate ");
       emu_printf(size);    
     }

  emu_printf(size);
  return retval;

} // End emu_Malloc

///////////////////////////
void emu_Free(void * pt)
{ free(pt); }

////////////////////////////////////////
int emu_ReadAnalogJoyX(int min, int max) 
{
  int val = analogRead(PIN_JOY2_A1X);
#if INVX
  val = 4095 - val;
#endif
  val = val-xRef;
  val = ((val*140)/100);
  if ( (val > -512) && (val < 512) ) val = 0;
  val = val+2048;
  return (val*(max-min))/4096;
}

//////////////////////////////////////////
int emu_ReadAnalogJoyY(int min, int max) 
{
  int val = analogRead(PIN_JOY2_A2Y);
#if INVY
  val = 4095 - val;
#endif
  val = val-yRef;
  val = ((val*120)/100);
  if ( (val > -512) && (val < 512) ) val = 0;
  //val = (val*(max-min))/4096;
  val = val+2048;
  //return val+(max-min)/2;
  return (val*(max-min))/4096;
}

//////////////////////////////////////////////
static uint16_t readAnalogJoystick(void)
{
  uint16_t joysval = 0;
  int xReading = 20;
  int yReading = 21;
#ifdef HAS_ANALOG_JOY 
  int xReading = emu_ReadAnalogJoyX(0,256);
  if (xReading > 128) joysval |= MASK_JOY2_LEFT;
  else if (xReading < 128) joysval |= MASK_JOY2_RIGHT;
  
   emu_ReadAnalogJoyY(0,256);
  if (yReading < 128) joysval |= MASK_JOY2_UP;
  else if (yReading > 128) joysval |= MASK_JOY2_DOWN;
#endif
#ifdef PIN_JOY2_BTN  
  joysval |= (digitalRead(PIN_JOY2_BTN) == HIGH ? 0 : MASK_JOY2_BTN);
#endif
  return (joysval);     
}

/////////////////////////////////////////////////
int emu_SwapJoysticks(int statusOnly) {
  if (!statusOnly) {
    if (joySwapped) {
      joySwapped = false;
    }
    else {
      joySwapped = true;
    }
  }
  return(joySwapped?1:0);
}

/////////////////////
int emu_GetPad(void) 
{
  return(keypadval/*|((joySwapped?1:0)<<7)*/);
}

////////////////////////////////
// emu_ReadKeys
////////////////////////////////
int emu_ReadKeys(void) 
{
  uint16_t retval;
  uint16_t j1 = readAnalogJoystick();
  uint16_t j2 = 0;
  
// Second joystick
#if INVY
#ifdef PIN_JOY1_1
  if ( digitalRead(PIN_JOY1_1) == LOW ) { Serial.println("Y1P1"); j2 |= MASK_JOY2_DOWN; }
#endif
#ifdef PIN_JOY1_2
  if ( digitalRead(PIN_JOY1_2) == LOW ) { Serial.println("Y1P2"); j2 |= MASK_JOY2_UP; }
#endif
#else
#ifdef PIN_JOY1_1
  if ( digitalRead(PIN_JOY1_1) == LOW ) { Serial.println("Y1P11"); j2 |= MASK_JOY2_UP;}
#endif
#ifdef PIN_JOY1_2
  if ( digitalRead(PIN_JOY1_2) == LOW ) { Serial.println("Y1P22"); j2 |= MASK_JOY2_DOWN; }
#endif
#endif
#if INVX
#ifdef PIN_JOY1_3
  if ( digitalRead(PIN_JOY1_3) == LOW ) { Serial.println("Y1P3"); j2 |= MASK_JOY2_LEFT; }
#endif
#ifdef PIN_JOY1_4
  if ( digitalRead(PIN_JOY1_4) == LOW ) { Serial.println("Y1P4"); j2 |= MASK_JOY2_RIGHT; }
#endif
#else
#ifdef PIN_JOY1_3
  if ( digitalRead(PIN_JOY1_3) == LOW ) { Serial.println("Y1P33"); j2 |= MASK_JOY2_RIGHT; }
#endif
#ifdef PIN_JOY1_4
  if ( digitalRead(PIN_JOY1_4) == LOW ) { Serial.println("Y1P44"); j2 |= MASK_JOY2_LEFT; }
#endif
#endif
#ifdef PIN_JOY1_BTN
  if ( digitalRead(PIN_JOY1_BTN) == LOW ) { Serial.println("Y1P0"); j2 |= MASK_JOY2_BTN; }
#endif

  if (joySwapped) {
    retval = ((j1 << 8) | j2);
  }
  else {
    retval = ((j2 << 8) | j1);
  }

/////////////////////////////////////////////////  
// Fill MASK_KEY_USERx  3210 0000 321 = Userx
// START SELECT OPTION RESET
// Teensy Pins not usb key press
////////////////////////////////////////////////
#ifdef PIN_KEY_USER1 
  if ( digitalRead(PIN_KEY_USER1) == LOW ) retval |= MASK_KEY_USER1; // retval = retval OR 0010 0000
#endif
#ifdef PIN_KEY_USER2 
  if ( digitalRead(PIN_KEY_USER2) == LOW ) retval |= MASK_KEY_USER2;
#endif
#ifdef PIN_KEY_USER3 
  if ( digitalRead(PIN_KEY_USER3) == LOW ) retval |= MASK_KEY_USER3;
#endif
#ifdef PIN_KEY_USER4 
  if ( digitalRead(PIN_KEY_USER4) == LOW ) retval |= MASK_KEY_USER4; 
#endif

  if (retval>0) Serial.println(retval,HEX); // shows 20, 40 or 80 or do a reset as below

  //////////////////////////////////////////////////
  // Reset procedure T3.X and T4.0 by Frank Boesing
  // sleep mode with interrupt wake up
  //////////////////////////////////////////////////
  if ( ((retval & (MASK_KEY_USER1+MASK_KEY_USER2)) == (MASK_KEY_USER1+MASK_KEY_USER2)) || (retval & MASK_KEY_USER4 ) )
  { emu_printf("Reset Teensy.... ");  

#if defined(__IMXRT1052__) || defined(__IMXRT1062__)    
    uint32_t tmp = SNVS_LPCR; // save control register
    SNVS_LPSR |= 1;
    
    // disable alarm
    SNVS_LPCR &= ~0x02;
    while (SNVS_LPCR & 0x02);
    
    __disable_irq();

    //get Time:
    uint32_t lsb, msb;
    do {
      msb = SNVS_LPSRTCMR;
      lsb = SNVS_LPSRTCLR;
    } while ( (SNVS_LPSRTCLR != lsb) | (SNVS_LPSRTCMR != msb) );
    
    uint32_t secs = (msb << 17) | (lsb >> 15);
    
    //set alarm
    secs += 2;
    SNVS_LPTAR = secs;
    while (SNVS_LPTAR != secs);
    
    SNVS_LPCR = tmp | 0x02; // restore control register and set alarm
    while (!(SNVS_LPCR & 0x02));
    
    SNVS_LPCR |= (1 << 6); // turn off power
    while (1) asm("wfi");                         // sleep mode with interrupt wake up 
#else                                             // Could also do: 
    *(volatile uint32_t *)0xE000ED0C = 0x5FA0004; // #define sleep_cpu()  asm("wfi")
    while (true) {  ;    }                        // asm("bkpt #251") reboot to boot code 
#endif 
  }
  
  return (retval);
}

/////////////////////////////////////////////
// emu_DebounceLocalKeys
/////////////////////////////////////////////
unsigned short emu_DebounceLocalKeys(void)
{
  uint16_t bCurState = emu_ReadKeys();
  uint16_t bClick = bCurState & ~bLastState; // bClick = 0 if bLastStae = bCurState
  bLastState = bCurState;
  return (bClick);                           // bClick = 0 or > 0
}

/////////////////////////////////////
void emu_InitJoysticks(void) { 

  // Second Joystick   
#ifdef PIN_JOY1_1
  pinMode(PIN_JOY1_1, INPUT_PULLUP);
#endif  
#ifdef PIN_JOY1_2
  pinMode(PIN_JOY1_2, INPUT_PULLUP);
#endif  
#ifdef PIN_JOY1_3
  pinMode(PIN_JOY1_3, INPUT_PULLUP);
#endif  
#ifdef PIN_JOY1_4
  pinMode(PIN_JOY1_4, INPUT_PULLUP);
#endif  
#ifdef PIN_JOY1_BTN
  pinMode(PIN_JOY1_BTN, INPUT_PULLUP);
#endif  

#ifdef PIN_KEY_USER1
  pinMode(PIN_KEY_USER1, INPUT_PULLUP);
#endif  
#ifdef PIN_KEY_USER2
  pinMode(PIN_KEY_USER2, INPUT_PULLUP);
#endif  
#ifdef PIN_KEY_USER3
  pinMode(PIN_KEY_USER3, INPUT_PULLUP);
#endif  
#ifdef PIN_KEY_USER4
  pinMode(PIN_KEY_USER4, INPUT_PULLUP);
#endif  
#ifdef PIN_JOY2_BTN
  pinMode(PIN_JOY2_BTN, INPUT_PULLUP);
#endif  
  analogReadResolution(12);
  xRef=0; yRef=0;

#ifdef HAS_ANALOG_JOY 
  for (int i=0; i<10; i++) {
    xRef += analogRead(PIN_JOY2_A1X);
    yRef += analogRead(PIN_JOY2_A2Y);  
    delay(20);
  }

#if INVX
  xRef = 4095 -xRef/10;
#else
  xRef /= 10;
#endif
#if INVY
  yRef = 4095 -yRef/10;
#else
  yRef /= 10;
#endif

#endif   //  HAS_ANALOG_JOY 
}


static int readNbFiles(void) {
  int totalFiles = 0;

#ifdef USE_SDFS
  DIR dir;
  FILINFO entry; 
  f_opendir(&dir, romspath);  
  while ( (true) && (totalFiles<MAX_FILES) ) {
    f_readdir(&dir, &entry);
    if (!entry.fname[0]) {
      // no more files
      break;
    }
    char * filename = entry.fname;
    Serial.println(filename);     
    if ( !(entry.fattrib & AM_DIR) ) {
      strncpy(&files[totalFiles][0], filename, MAX_FILENAME_SIZE-1);
      totalFiles++;
    }
    else {
      if ( (strcmp(filename,".")) && (strcmp(filename,"..")) ) {
        strncpy(&files[totalFiles][0], filename, MAX_FILENAME_SIZE-1);
        totalFiles++;
      }
    }  
  } 
  f_closedir(&dir);
#else
  File entry;    
  file = SD.open(romspath);
  while ( (true) && (totalFiles<MAX_FILES) ) {
    entry = file.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
#ifdef USE_SDFAT
  char filename[MAX_FILENAME_SIZE];
  entry.getName(&filename[0], MAX_FILENAME_SIZE); 
#else
  char * filename = entry.name();
#endif 
    Serial.println(filename); 
    if (!entry.isDirectory())  {
      strncpy(&files[totalFiles][0], filename, MAX_FILENAME_SIZE-1);
      totalFiles++;
    }
    else {
      if ( (strcmp(filename,".")) && (strcmp(filename,"..")) ) {
        strncpy(&files[totalFiles][0], filename, MAX_FILENAME_SIZE-1);
        totalFiles++;
      }
    }  
    entry.close();
  }
  file.close();
#endif  
  return totalFiles;  
}  

void backgroundMenu(void) {
    menuRedraw=true;  
    tft.fillScreenNoDma(RGB16_BLACK);
}

////////////////////////////////
// handleMenu
////////////////////////////////
int handleMenu(uint16_t bClick)
{
  int action = ACTION_NONE;

  char newpath[80];            // [64]
  strcpy(newpath, romspath);   // 
  strcat(newpath, "/");        // newpath/
  strcat(newpath, selection);  // newpath/selection
  int NumL = MAX_MENULINES;    // 15

  int rx=0,ry=0,rw=0,rh=0;
  char c = 0; 
  if ( usbarrowdown==1)  { c = MKEY_DOWN ; usbarrowdown = 0;  }
  if ( usbarrowup==1 )   { c = MKEY_UP ;   usbarrowup = 0;    }
  if ( usbarrowright==1) { c = MKEY_DOWN ; usbarrowright = 0; }
  if ( usbarrowleft==1 ) { c = MKEY_UP ;   usbarrowleft = 0;  }
  if ( usbenter==1 )     { c = MKEY_TFT ;  usbenter = 0;      }
  
  ////////////////////////////////////////////////////
  // Press Enter or Firebutton on selected file
  ////////////////////////////////////////////////////
  if ( (bClick & MASK_JOY2_BTN) || (c == MKEY_TFT) ) {     
      emu_printf(newpath);
#ifdef USE_SDFS
      FILINFO entry;
      f_stat(newpath, &entry);
      if ( (entry.fattrib & AM_DIR) ) {
#else
      File file = SD.open(newpath);
      if (file.isDirectory())  {
#endif      
        strcpy(romspath,newpath);
        curFile = 0;
        nbFiles = readNbFiles();             
      }
      else {
        action = ACTION_RUNTFT;               
      }
      menuRedraw=true;
  }
  ///////////////////////////////////////////////////
  // Keyboard Up arrow or Joystick Up
  ///////////////////////////////////////////////////
  else if ((bClick & MASK_JOY2_UP) || (c==MKEY_UP)) {
    if (curFile!=0) {
      menuRedraw=true;
      curFile--;
    }
  }
  /////////////////////////////////////////////////////////////////////////////////////////
  // Previous Page Keyboard Right Arrow or Joystick Right
  /////////////////////////////////////////////////////////////////////////////////////////
  else if ( (bClick & MASK_JOY2_RIGHT) || (bClick & MASK_JOY1_RIGHT) || (c==MKEY_RIGHT) ) {
      if ((curFile-NumL)>=0) {
      menuRedraw=true;
      curFile -= NumL;
     } else if (curFile!=0) {
      menuRedraw=true;
      curFile--;
     }
  }  
  ///////////////////////////////////////////////////
  // Keyboard Down arrow or Joystick Down
  ///////////////////////////////////////////////////
  else if ((bClick & MASK_JOY2_DOWN) || (c==MKEY_DOWN))  {
    if ((curFile<(nbFiles-1)) && (nbFiles)) {
      curFile++;
      menuRedraw=true;
    }
  }
  ////////////////////////////////////////////////////////////////////////////////////////
  // Next Page Keyboard Left Arrow or Joystick Left
  ////////////////////////////////////////////////////////////////////////////////////////
  else if ( (bClick & MASK_JOY2_LEFT) || (bClick & MASK_JOY1_LEFT) || (c==MKEY_LEFT) ) {
    if ((curFile<(nbFiles-NumL)) && (nbFiles)) {
      curFile += NumL;
      menuRedraw=true;
    }
    else if ((curFile<(nbFiles-1)) && (nbFiles)) {
      curFile++;
      menuRedraw=true;
    }
  }
  else if ( (bClick & MASK_KEY_USER2) || (c == MKEY_JOY) ) {
    emu_SwapJoysticks(0);
    menuRedraw=true;  
  }   
    
  if (menuRedraw && nbFiles) 
  { int fileIndex = 0;
    tft.drawRectNoDma(MENU_FILE_XOFFSET,MENU_FILE_YOFFSET, MENU_FILE_W, MENU_FILE_H, MENU_FILE_BGCOLOR);
    if (curFile <= (MAX_MENULINES-1)) topFile=0;
    else topFile=curFile-(MAX_MENULINES/2);

    int i=0;
    while (i<MAX_MENULINES) 
          { if (fileIndex>=nbFiles) break; // no more files
            char * filename = &files[fileIndex][0];    
            if (fileIndex >= topFile) 
               { if ((i+topFile) < nbFiles ) 
                    { if ((i+topFile)==curFile) 
                         //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         // Change true to false in next two drawTextNoDma calls for sinle height text llines spaced apart by one text line
                         //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                         { tft.drawTextNoDma(MENU_FILE_XOFFSET,i*TEXT_HEIGHT+MENU_FILE_YOFFSET, filename, MENU_FILE_SGCOLOR, MENU_FILE_CGCOLOR, true);
                           strcpy(selection,filename);            
                         } else { tft.drawTextNoDma(MENU_FILE_XOFFSET,i*TEXT_HEIGHT+MENU_FILE_YOFFSET, filename, MENU_FILE_FGCOLOR, MENU_FILE_BGCOLOR, true); }
                    }
                  i++; 
                }
             fileIndex++;    
           }                   // end while
      menuRedraw=false;     
  }                            // if (menuRedraw && nbFiles)
  return (action);  
}

////////////////////////////////
bool menuActive(void) 
{ return (menuOn); }

////////////////////////////////
void toggleMenu(bool on) 
{ if (on) 
     { menuOn=true;
       backgroundMenu();
     } else { menuOn = false; }
}

///////////////////////////////
char * menuSelection(void)
{ return (selection); }
  
//////////////////////////////////
int emu_FileOpen(char * filename)
//////////////////////////////////
{
  int retval = 0;

  char filepath[80];
  strcpy(filepath, romspath);
  strcat(filepath, "/");
  strcat(filepath, filename);
  emu_printf("FileOpen...");
  emu_printf(filepath);
#ifdef USE_SDFS
  if( !(f_open(&file, filepath, FA_READ)) ) {
#else    
  if ((file = SD.open(filepath, O_READ))) {
#endif
    retval = 1;  
  }
  else {
    emu_printf("FileOpen failed");
  }
  return (retval);
}

//////////////////////////////////////
int emu_FileRead(char * buf, int size)
{
  unsigned char buffer[256];
  
  int remaining = size;
  int byteread = 0;
  int retval=0; 
  if (size < 256) {
#ifdef USE_SDFS
    if( !(f_read (&file, buffer, size, &retval)) )
#else
    retval = file.read(buffer, size);
#endif
   if (retval>0) {
      memcpy(buf,buffer,retval);
      byteread += retval;   
   }   
  }
  else {
    while (remaining>0) {
#ifdef USE_SDFS
      if( !(f_read (&file, buffer, 256, &retval)) )
      //f_read (&file, buffer, 256, &retval);
#else
      retval = file.read(buffer, 256);
#endif
      if (retval>0) {
        //emu_printi(retval);
        memcpy(buf,buffer,retval);
        buf += retval;
        byteread += retval;     
        remaining -= retval;
      }
      else {
        break;
      }
    }    
  }

  return byteread; 
}

///////////////////////////////////
unsigned char emu_FileGetc(void) {
  unsigned char c;
#ifdef USE_SDFS
  int retval=0;
  if( !(f_read (&file, &c, 1, &retval)) )
#else
  int retval = file.read(&c, 1);
#endif  
  if (retval != 1) {
    emu_printf("emu_FileGetc failed");
  }  
  return c; 
}

////////////////////////////
void emu_FileClose(void)
{
#ifdef USE_SDFS
  f_close(&file); 
#else
  file.close();  
#endif  
}

//////////////////////////////////
int emu_FileSize(char * filename) 
//////////////////////////////////
// FileSize...
// 800/BASICC.ROM
// filesize is...
// 8192
// FileOpen...
// 800/BASICC.ROM
// 8k
/////////////////////////////////
{
  int filesize=0;
  char filepath[80];
  strcpy(filepath, romspath);
  strcat(filepath, "/");
  strcat(filepath, filename);
  emu_printf("FileSize...");
  emu_printf(filepath);
#ifdef USE_SDFS
  FILINFO entry;
  f_stat(filepath, &entry);
  filesize = entry.fsize; 
#else
  if ((file = SD.open(filepath, O_READ))) 
  {
    emu_printf("filesize is...");
    filesize = file.size(); 
    emu_printf(filesize);
    file.close();    
  }
#endif
 
  return(filesize);    
}

///////////////////////////
int emu_FileSeek(int seek) 
{
#ifdef USE_SDFS
  f_lseek(&file, seek);
#else
  file.seek(seek);
#endif
  return (seek);
}

//////////////////////////
int emu_FileTell(void) 
{
#ifdef USE_SDFS
  return (f_tell(&file));
#else
  return (50);
#endif
}

//////////////////////////////////////////////////////////
int emu_LoadFile(char * filename, char * buf, int size)
{
  int filesize = 0;
    
  char filepath[80];
  strcpy(filepath, romspath);
  strcat(filepath, "/");
  strcat(filepath, filename);
  emu_printf("LoadFile...");
  emu_printf(filepath);
#ifdef USE_SDFS
  if( !(f_open(&file, filepath, FA_READ)) ) {
    filesize = f_size(&file);
    emu_printf(filesize);
    if (size >= filesize)
    {
      int retval=0;
      if( (f_read (&file, buf, filesize, &retval)) ) {
        emu_printf("File read failed");        
      }
    }
    f_close(&file);
  }
#else
  if ((file = SD.open(filepath, O_READ))) 
  {
    filesize = file.size(); 
    emu_printf(filesize);
    
    if (size >= filesize)
    {
      if (emu_FileRead(buf, filesize) != filesize) 
      {
        emu_printf("File read failed");
      }        
    }
    file.close();
  }
#endif  
  
  return(filesize);
}

////////////////////////////////////////////////////////////////////////
int emu_LoadFileSeek(char * filename, char * buf, int size, int seek)
{
  int filesize = 0;
    
  char filepath[80];
  strcpy(filepath, romspath);
  strcat(filepath, "/");
  strcat(filepath, filename);
  emu_printf("LoadFileSeek...");
  emu_printf(filepath);
#ifdef USE_SDFS
  if( !(f_open(&file, filepath, FA_READ)) ) {
    f_lseek(&file, seek);
    emu_printf(size);
    if (size >= filesize)
    {
      int retval=0;
      if( (!f_read (&file, buf, size, &retval)) ) 
      if (retval != size)
      {
        emu_printf("File read failed");      
      }
    }
    f_close(&file);
  }
#else
  if ((file = SD.open(filepath, O_READ))) 
  {
    file.seek(seek);
    emu_printf(size);
    if (file.read(buf, size) != size) {
      emu_printf("File read failed");
    }        
    file.close();
  }
#endif  
  
  return(filesize);
}


const uint32_t FILE_SIZE_MB = 8;
const uint32_t FILE_SIZE = 1000000UL*FILE_SIZE_MB;
#ifdef USE_SDFS
static FIL tempfile; 
#else
static File tempfile;
#endif

///////////////////////////////
void emu_FileTempInit(void) 
{
#ifdef USE_SDFS
 if( (f_open(&tempfile, SDFSDEV "/bench.dat", FA_READ| FA_WRITE )) ) {
#else
#ifdef USE_SDFAT
  if (!tempfile.open("/bench.dat", O_RDWR /*| O_CREAT | O_TRUNC*/)) {
#else
  if ((tempfile = SD.open("/bench.dat", O_RDWR))) {
#endif
#endif
    Serial.println("psram open failed");
  }
/*
  else {
    uint8_t buf[256];
    Serial.println("psram creating");    
    file = SD.open("/bench.dat", O_RDWR | O_CREAT | O_TRUNC);
    if (file) {
      Serial.println("psram opened");    
      file.truncate(0);

      if (!file.preAllocate(FILE_SIZE)) {
        Serial.println("allocate failed");  
      }      
      for(int i=0;i<FILE_SIZE/256;i++){
        file.write(buf, 256);
        Serial.println(i);
        
      }
      Serial.println("psram created");
    }
    
    //file.close();
  }
*/  
#ifdef USE_SDFS
  FILINFO entry;
  f_stat(SDFSDEV "/bench.dat", &entry);
  int fs=entry.fsize;
#else
  int fs=tempfile.size();
#endif
  Serial.print("psram size is ");
  Serial.println(fs); 
}

////////////////////////////////////////////////////////////
void emu_FileTempRead(int addr, unsigned char * val, int n) 
{
#ifdef USE_SDFS
  f_lseek(&file, addr);
  int retval=0;
  f_read (&file, val, n, &retval);
#else
  tempfile.seek(addr);
  tempfile.read(val, n);
#endif    
}

/////////////////////////////////////////////////////////
void emu_FileTempWrite(int addr, unsigned char val) 
{
  uint8_t v=val;
#ifdef USE_SDFS
  f_lseek(&tempfile, addr);
  int retval=0;
  f_write (&tempfile, &v, 1, &retval);
#else
  tempfile.seek(addr);
  tempfile.write(&v, 1);
#endif   
}

/////////////////////////
void emu_init(void)
/////////////////////////
{
  Serial.begin(115200);

#ifdef USE_SDFS
  strcpy(romspath,SDFSDEV);
  strcat(romspath,ROMSDIR);    
  FRESULT rc;
  if((rc = f_mount (&fatfs, romspath, 1))) {
    emu_printf("Fail mounting SDFS");  
  }
  else { 
    rc = f_chdrive(ROMSDIR);
  }
#else  
#ifdef USE_SDFAT
  while (!SD.begin(SD_CONFIG))
#else
  while (!SD.begin(SD_CS))
#endif
  {
    Serial.println("SD begin failed, retrying...");
    delay(1000);    
  }
  strcpy(romspath,ROMSDIR);
#endif

  nbFiles = readNbFiles(); 

  Serial.print("SD initialized, files found: ");  // Normally here
  Serial.println(nbFiles);

  
  emu_InitJoysticks();
#ifdef SWAP_JOYSTICK
  joySwapped = true;   
#else
  joySwapped = false;   
#endif  
  
}
