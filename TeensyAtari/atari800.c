#include "atari800.h"
#include <string.h>
#include "memory.h"
#include "cpu.h"
#include "atari.h"
#include "akey.h"
#include "pokey.h"
#include <Arduino.h>
//#include "romatariosa.h"
#include "romatariosb.h"
//#include "romatarixl.h"
#include "antic.h"
#include "gtia.h"
#include "pia.h"
#include "colours.h"
#include "emuapi.h"
#if HAS_SND
#include "pokeysnd.h"
#endif



// Controllers
typedef struct
{ 
  // All values are 1 or 0, or perhaps not...
  int left;
  int right;
  int up;
  int down;
  int trig;

} CONTROLLER;





// global variables
unsigned char mem[MEMORY_SIZE];  //65k+1 or 48k+1
unsigned char * memory = mem;
int tv_mode = TV_PAL;
UBYTE INPUT_key_consol;

int usbkey;
int usboldkey;
int usbshiftkey;   // 104  108
int usbcontrolkey; // 103  107
int usbcapslock;   // 57
int usbwindowskey; // 106
int usbaltkey;     // 105 109
int usbarrowup;    // 82 key218 Ctrl
int usbarrowdown;  // 81 key217 Ctrl
int usbarrowright; // 79 key215 Ctrl
int usbarrowleft;  // 80 key216 Ctrl
int SHFLOK;        // Location 702 0, 64 (all uppercase), 128, 196 Shift Ctrl
int usbF6;         // 63 key199
int usbF7;         // 64 key200
int usbF8;         // 65 key201
int usbF9;         // 66 key202
int usbenter;      // 

// local variables
static CONTROLLER cont1, cont2;
static int INPUT_key_code = AKEY_NONE; // -1
static int INPUT_key_shift = 0;

//////////////////////////////////////////////////////////////////////////
// MEMORY MAP                   INDEX  (0 is invalid)
// 0-3FFF    RAM          read/write      16k 0-16383
// 4000-7FFF RAM          read/write      16k 16384-32767
// 8000-9FFF BASIC        right CART      8k  32768-40958
// A000-BFFF BASIC        left  CART      8k  40958-49151
// C000-CFFF                              4k  49152-53247
// D000-D0FF GTIA regs
// D200-D2FF POKEY regs
// D300-D3FF PIA  regs
// D400-D4FF ANTIC regs
// D800-FFFF OS ROM       read-only       10k 55296-65535 Note 10k not 16k
// E000                   I/O expansion       57344
///////////////////////////////////////////////////////////////////////////

/////////////////////////////////////
// memory CPU read (load) handler
/////////////////////////////////////
uint8 Atari_GetByte(uint16 addr)
{
  if (addr < MEMORY_SIZE) {                          // MAPPER_RAM
      return(memory[addr]);                          // 0x8000 RAM up to 32k
  }
  else if ( (addr >= 0xD000) && (addr < 0xD100) ) {  // MAPPER_GTIA
      return GTIA_GetByte(addr,1);
  } 
  else if ( (addr >= 0xD200) && (addr < 0xD300) ) {  // MAPPER_POKEY
      return POKEY_GetByte(addr,1);       
  } 
  else if ( (addr >= 0xD300) && (addr < 0xD400) ) {  // MAPPER_PIA
      return PIA_GetByte(addr);       
  } 
  else if ( (addr >= 0xD400) && (addr < 0xD500) ) {  // MAPPER_ANTIC
      return ANTIC_GetByte(addr,1);
  }
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Use RomOS as below - directly from Flash memory read - no copy to RAM
  // return(romos[addr-0xD800]) => return romos[0] to romos[10k-1] directly
  // No pgm_read or strcpy_ anywhere in code
  // 
  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  else if ((addr >= 0xD800) && (addr < 0x10000)) {  // MAPPER_ROM (bios) // 10k range here - 16k Rom how?
      //return(memory[addr]);
      return(romos[addr-0xD800]);                   // romos[0-10k-1] cannot use this for 800xl
  }

  //case MAPPER_IOEXP:  // I/O exp read
  //  return IOEXPread(addr);

  return 0xff;
}

// memory CPU write (store) handler
void Atari_PutByte(uint16 addr, uint8 byte)
{
  if (addr < 0x8000) {                               // MAPPER_RAM
      memory[addr] = byte;                           // 0x8000 RAM up to 32k
  }
  else if (addr < 0xC000) {                          // MAPPER_ROM 49k do nothing no write
  }
  else if ( (addr >= 0xD000) && (addr < 0xD100) ) {  // MAPPER_GTIA
      GTIA_PutByte(addr, byte);    
  } 
  else if ( (addr >= 0xD200) && (addr < 0xD300)  ) {  // MAPPER_POKEY
      POKEY_PutByte(addr, byte);                     
  } 
  else if ( (addr >= 0xD300) && (addr < 0xD400)  ) {  // MAPPER_PIA
      PIA_PutByte(addr, byte);                     
  } 
  else if ( (addr >= 0xD400) && (addr < 0xD500) ) {  // MAPPER_ANTIC
      ANTIC_PutByte(addr, byte);
  }
  
  //case MAPPER_IOEXP:  // I/O exp write
  //  IOEXPwrite(addr, byte);
}

// in akey.h
/*
#define AKEY_SHFT     0x40     //  64 0100 0000
#define AKEY_CTRL     0x80     // 128 1000 0000
#define AKEY_SHFTCTRL 0xc0     // 192 1100 0000

#define AKEY_WARMSTART             -2
#define AKEY_COLDSTART             -3
#define AKEY_EXIT                  -4
#define AKEY_BREAK                 -5
#define AKEY_UI                    -7
#define AKEY_SCREENSHOT            -8
#define AKEY_SCREENSHOT_INTERLACE  -9

#define AKEY_START                 -10
#define AKEY_SELECT                -11
#define AKEY_OPTION                -12

#define AKEY_PBI_BB_MENU           -13
#define AKEY_CX85_1                -14
#define AKEY_CX85_2                -15
#define AKEY_CX85_3                -16
#define AKEY_CX85_4                -17
#define AKEY_CX85_5                -18
#define AKEY_CX85_6                -19
#define AKEY_CX85_7                -20
#define AKEY_CX85_8                -21
#define AKEY_CX85_9                -22
#define AKEY_CX85_0                -23
#define AKEY_CX85_PERIOD           -24
#define AKEY_CX85_MINUS            -25
#define AKEY_CX85_PLUS_ENTER       -26
#define AKEY_CX85_ESCAPE           -27
#define AKEY_CX85_NO               -28
#define AKEY_CX85_DELETE           -29
#define AKEY_CX85_YES              -30
#define AKEY_TURBO                 -31
#ifdef USE_UI_BASIC_ONSCREEN_KEYBOARD
#define AKEY_KEYB                  -32

POKEY_KBCODE = 0xff; // 255
POKEY_SERIN  = 0x00; // or 0xff ? 
POKEY_IRQST  = 0xff;
POKEY_IRQEN  = 0x00;
POKEY_SKSTAT = 0xef; // 239 1110 1111
POKEY_SKCTL  = 0x00; // 0

#define AKEY_SHFT 0x40      // 0100 0000
#define AKEY_CTRL 0x80      // 1000 0000
#define AKEY_SHFTCTRL 0xc0  // 1100 0000
*/

/////////////////////////////////////////////////////
// If keyboard enter here with the Atari key scancode
// INPUT_key_code is 0 to 247 ("L" or UpArrow)
/////////////////////////////////////////////////////
static void INPUT_Frame(void)
{
  int i;
  static int last_key_code = AKEY_NONE;
  static int last_key_break = 0;

  if (cont1.trig) GTIA_TRIG[0]=0; else GTIA_TRIG[0]=1;

  //SHFLOK = Atari_GetByte(702);
  //if ((SHFLOK==0) && (INPUT_key_code==60)) Atari_PutByte(702, 64);

  i = (INPUT_key_code == AKEY_BREAK);
  if (i && !last_key_break) 
     { if (POKEY_IRQEN & 0x80) 
          { POKEY_IRQST &= ~0x80;
            CPU_GenerateIRQ();
          }
     }
  last_key_break = i;
  
  POKEY_SKSTAT |= 0xc;
  //if (INPUT_key_shift) POKEY_SKSTAT &= ~8;             //INPUT_key_shift always=0, never used

  if (INPUT_key_code < 0) last_key_code = AKEY_NONE;     //no key
   
  //if ((INPUT_key_code&~0x17) == AKEY_SHFTCTRL) INPUT_key_code = AKEY_NONE; // 0001 0111 not 1110 1000 and keycode 1100 0000 to 110x 0xxx == 1100 0000

  if (INPUT_key_code >= 0) 
     { POKEY_SKSTAT &= ~4;
       if ((INPUT_key_code ^ last_key_code) & ~AKEY_SHFTCTRL) // 0xc0 192 1100 0000 ^ XOR
          { 
            last_key_code = INPUT_key_code;  // ignore if only shift or control has changed its state 
            POKEY_KBCODE = (UBYTE) INPUT_key_code;
            if (POKEY_IRQEN & 0x40) 
               { if (POKEY_IRQST & 0x40) 
                    { POKEY_IRQST &= ~0x40;
                      CPU_GenerateIRQ();
                    } else POKEY_SKSTAT &= ~0x40; // keyboard over-run 
          
               }
           }
      }
}  


                
// check keyboard and set kbcode on VBI (not for A800)
void INPUT_Scanline(void)
{
//  if (cont1.trig) GTIA_TRIG[0]=0; else GTIA_TRIG[0]=1;
}

//////////////////////////////////////
// Load Cartridge ROM into memory
//////////////////////////////////////
static void load_CART(char * cartname) 
{
  int flen = emu_FileSize(cartname);  // in emuapi.cpp 
  emu_FileOpen(cartname);
  if (flen < 16384) {
    emu_printf("8k");
    for(int i=0; i<flen; i++) memory[0xA000 + i] = emu_FileGetc();  // From 40960 to 48k  
  }
  else {
    emu_printf("16k");    
    for(int i=0; i<flen; i++) memory[0x8000 + i] = emu_FileGetc();  // From 32768 to 48k
  }

  emu_FileClose();                   // in emuapi.cpp 
}

  


void add_esc(UWORD address, UBYTE esc_code)
{
  memory[address++] = 0xf2; /* ESC */
  memory[address++] = esc_code; /* ESC CODE */
  memory[address] = 0x60;   /* RTS */
}

int Atari_PORT(int num)
{
  int nibble_0 = 0xff;
  int nibble_1 = 0xff;

  if (num == 0)
  {  
    if (cont1.left) nibble_0 &= ~0x04;   
    if (cont1.right) nibble_0 &= ~0x08;   
    if (cont1.up) nibble_0 &= ~0x01;   
    if (cont1.down) nibble_0 &= ~0x02;   
    if (cont2.left) nibble_1 &= ~0x04;   
    if (cont2.right) nibble_1 &= ~0x08;   
    if (cont2.up) nibble_1 &= ~0x01;   
    if (cont2.down) nibble_1 &= ~0x02;   
  }
  return (nibble_1 << 4) | nibble_0;
}

/////////////////////////////
static void Initialise(void)
/////////////////////////////
{
  int i;

  emu_printf("Initialising All");
  ///////////////////////////////////
  // Set up memory area
  ///////////////////////////////////
  memset(memory, 0, MEMORY_SIZE);       // Set=0 0-0xC000 48k or 0-0x10000 65k overlap with below
  memset(memory+0x8000, 0xff, 0x4000);  // Set to 255 32k + 16k = 255

  // init controllers
  memset(&cont1, 0, sizeof(CONTROLLER));  // 5 x int
  memset(&cont2, 0, sizeof(CONTROLLER));  // 5 x int
  
  ///////////////////////////////////
  // Load bios to address 0xF800   
  // 63488
  ///////////////////////////////////
  // for (i = 0; i < 0x2800; i++)   // 0x2800 = 10k RM
  //     { memory[0xD800 + i] = romos[i]; }  
  // add_esc(0xe459, ESC_SIOV);  

} // End Initialise

////////////////////////
void at8_Init(void)
////////////////////////
{
   int i;

  // Palette
  for (i = 0; i < PALETTE_SIZE; i++) emu_SetPaletteEntry(R32(colourtable[i]), G32(colourtable[i]), B32(colourtable[i]), i);

#if HAS_SND
  emu_sndInit();                                                    // mymixer.start in .ino setup
  POKEYSND_Init(POKEYSND_FREQ_17_APPROX, 44100, 1, POKEYSND_BIT16); // in pokeysnd
#endif

  emu_printf("Allocating RAM");
  if (memory == NULL) memory = emu_Malloc(MEMORY_SIZE); // 0-0xC000 48k+1 or 0-0x10000 65k+1
                                                        // Normally no emu_Malloc directly to Initialise
  Initialise();  // Do a memset
}


void at8_Step(void)
{
  //emu_printf("step");
  int j=emu_ReadKeys();
  //int k=emu_GetPad();
  int k = 0;
  
  //////////////////////////////////
  // Replacement for geti2ckeyboard
  //////////////////////////////////

  ////////////////////////////////////////////////////////////
  // Case key pressed: usbkey = 0 to 255
  //                          = 0 to 247 ("L" to "arrow down")
  ////////////////////////////////////////////////////////////

  if (usbkey != usboldkey)                            // any key pressed
     { INPUT_key_code = usbkey2ScanCode[usbkey];      // usbkeyboard ASCII keycode -> Atari scancode
       if (INPUT_key_code < 255) usboldkey = usbkey;  // valid scancode < 255
     } else INPUT_key_code = AKEY_NONE;               // -1

  //int mem702read = Atari_GetByte(702);
  //int mem702write = mem702read;
   
     if (usbcontrolkey>0) INPUT_key_code = INPUT_key_code | 128;
  /*
  if (usbcontrolkey > 0) 
     { int mem702write = mem702read | 128;
       Atari_PutByte(702, mem702write); 
     } else
     { int mem702write = mem702read & 128;
       Atari_PutByte(702, mem702write); 
     }

  if (usbshiftkey > 0) 
     { int mem702write = mem702read | 64;
       Atari_PutByte(702, mem702write); 
     } else 
     { int mem702write = mem702read & 64;
       Atari_PutByte(702, mem702write); 
     }
 */
 
  /*
  if ((INPUT_key_code==60) && (usbcapslock==0)) 
     { Atari_PutByte(702, AKEY_SHFT); 
       usbcapslock = 1; 
     } 
  if ((INPUT_key_code==60) && (usbcapslock==1)) 
     { Atari_PutByte(702, 0); 
       usbcapslock = 0; 
     } 
  */

/*
#define MASK_JOY2_RIGHT 0x0001
#define MASK_JOY2_LEFT  0x0002
#define MASK_JOY2_UP    0x0004
#define MASK_JOY2_DOWN  0x0008
#define MASK_JOY2_BTN   0x0010
#define MASK_KEY_USER1  0x0020 0010 0000
#define MASK_KEY_USER2  0x0040 0100 0000
#define MASK_KEY_USER3  0x0080 1000 0000
#define MASK_JOY1_RIGHT 0x0100
#define MASK_JOY1_LEFT  0x0200
#define MASK_JOY1_UP    0x0400
#define MASK_JOY1_DOWN  0x0800
#define MASK_JOY1_BTN   0x1000
#define MASK_KEY_USER4  0x2000 
*/

  CONTROLLER * which;
  
  if (j & 0x8000) which=&cont2;
  else which=&cont1;
  
  //////////////////////////////////////////////////////////////////
  // If pressed is a 0                             INPUT_key_consol
  // START    Bit 0    PEEK(53279)=6   0110 011x   0000 0001
  // SELECT   Bit 1    PEEK(53279)=5   0101 01y1   0000 0010  
  // OPTION   Bit 2    PEEK(53279)=3   0011 0z11   0000 0100
  // No key pressd = 7                 0111        0000 0000
  /////////////////////////////////////////////////////////
  // Start  
  if (j & MASK_KEY_USER1)       // j = xxxx 0000 0x0020 0010 0000
    INPUT_key_consol &= ~0x01;  // NOT 0000 0001 = 1111 1110
  else
    INPUT_key_consol |= 0x01;   // 0000 0001 // no Start pressed
  if (INPUT_key_code==199) INPUT_key_consol &= ~0x01;

  // Select
  if (j & MASK_KEY_USER2)
    INPUT_key_consol &= ~0x02;
  else
    INPUT_key_consol |= 0x02;   // 0000 0010
  if (INPUT_key_code==200) INPUT_key_consol &= ~0x02;

  // OPTION
  if (j & MASK_KEY_USER3)
    INPUT_key_consol &= ~0x04;
  else
    INPUT_key_consol |= 0x04;   // 0000 0100
  if (INPUT_key_code==201) INPUT_key_consol &= ~0x04;

  if ((INPUT_key_code>198) && (INPUT_key_code<205)) 
       { emu_printf("INPUT_key_consol: ");
         emu_printf(INPUT_key_consol);
       }

///////////////////////////////////
// Keyboard
// JOY2 trig down up left right 0,1
// CONTROLLER * which;
///////////////////////////////////
INPUT_Frame();
  if (j & MASK_JOY2_BTN) 
      which->trig = 1;    
  else
      which->trig = 0;
  if (j & MASK_JOY2_DOWN)
      which->down = 1;
  else
      which->down = 0;
  if (j & MASK_JOY2_UP)
      which->up = 1;
  else
      which->up = 0;
  if (j & MASK_JOY2_RIGHT)
      which->left = 1;
  else
      which->left = 0;
  if (j & MASK_JOY2_LEFT)
      which->right = 1;
  else
      which->right = 0; 

  GTIA_Frame();
  ANTIC_Frame(1); 
  emu_DrawVsync();
  POKEY_Frame();
  
  //int i;
  //for (i=0xC000; i< 0x10000; i++)
  //  if (memory[i] !=0) emu_printf("bug");  
}


//////////////////////////////////
void at8_Start(char * cartname)
//////////////////////////////////
{
  load_CART(cartname);

  emu_printf("antic");
  ANTIC_Initialise();
  emu_printf("gtia");  
  GTIA_Initialise();
  emu_printf("pia");
  PIA_Initialise();  
  emu_printf("pokey");
  POKEY_Initialise();
  PORTA = 0x00;
  ANTIC_NMIEN = 0x00;
  ANTIC_NMIST = 0x00;
  memory[0x244] = 1;  
  GTIA_TRIG[0]=1;
  GTIA_TRIG[1]=1;
  GTIA_TRIG[2]=1;
  GTIA_TRIG[3]=1;
  emu_printf("6502 reset");
  CPU_Reset();
    
  emu_printf("init done");   
}
