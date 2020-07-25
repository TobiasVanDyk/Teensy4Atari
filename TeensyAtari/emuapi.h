#ifndef EMUAPI_H
#define EMUAPI_H

//#define INVX        1
#define INVY        1
#define HAS_SND     1
#define CUSTOM_SND  1
//#define TIMER_REND  1
#define EXTRA_HEAP  0x10
//#define HAS_ANALOG_JOY 1

// Title:     <                        >

#define ROMSDIR "800"             // This is for Atari 800/800XL only used in emuapi.cc 4 times

#define emu_Init(ROM) {at8_Init(); at8_Start(ROM);}
#define emu_Step(x) {at8_Step();} // used once in main loop
#define emu_Input(x) {}           // used once in main loop

#define PALETTE_SIZE         256
#define VID_FRAME_SKIP       0x3
#define TFT_VBUFFER_YCROP    0
#define SINGLELINE_RENDERING 1

#define R32(rgb) ((rgb>>16)&0xff) 
#define G32(rgb) ((rgb>>8)&0xff) 
#define B32(rgb) (rgb & 0xff) 

#define RGB16_BLACK         RGBVAL16(0x00,0x00,0x00)           // black
#define RGB16_WHITE         RGBVAL16(0xff,0xff,0xff)           // white
#define RGB16_RED           RGBVAL16(0xff,0x00,0x00)           // red
#define RGB16_GREEN         RGBVAL16(0x00,0xff,0x00)           // green
#define RGB16_BLUE          RGBVAL16(0x00,0x00,0xff)           // blue

#define ACTION_NONE     0
#define ACTION_RUNTFT   126
   
#define MASK_JOY2_RIGHT 0x0001
#define MASK_JOY2_LEFT  0x0002
#define MASK_JOY2_UP    0x0004
#define MASK_JOY2_DOWN  0x0008
#define MASK_JOY2_BTN   0x0010
#define MASK_KEY_USER1  0x0020
#define MASK_KEY_USER2  0x0040
#define MASK_KEY_USER3  0x0080
#define MASK_JOY1_RIGHT 0x0100
#define MASK_JOY1_LEFT  0x0200
#define MASK_JOY1_UP    0x0400
#define MASK_JOY1_DOWN  0x0800
#define MASK_JOY1_BTN   0x1000
#define MASK_KEY_USER4  0x2000



extern void emu_init(void);
extern void emu_printf(char * text);
extern void emu_printi(int val);
extern void * emu_Malloc(int size);
extern void emu_Free(void * pt);

extern int emu_FileOpen(char * filename);
extern int emu_FileRead(char * buf, int size);
extern unsigned char emu_FileGetc(void);
extern int emu_FileSeek(int seek);
extern void emu_FileClose(void);
extern int emu_FileSize(char * filename);
extern int emu_LoadFile(char * filename, char * buf, int size);
extern int emu_LoadFileSeek(char * filename, char * buf, int size, int seek);
extern void emu_FileTempInit(void); 
extern void emu_FileTempRead(int addr, unsigned char * val, int n); 
extern void emu_FileTempWrite(int addr, unsigned char val); 

extern void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index);
extern void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride);
extern void emu_DrawLine(unsigned char * VBuf, int width, int height, int line);
extern void emu_DrawLine16(unsigned short * VBuf, int width, int height, int line);
extern void emu_DrawVsync(void);
extern int emu_FrameSkip(void);
extern void * emu_LineBuffer(int line);

extern void emu_InitJoysticks(void);
extern int emu_SwapJoysticks(int statusOnly);
extern unsigned short emu_DebounceLocalKeys(void); // returns 0 if no new ket, >0 if new key
extern int emu_ReadKeys(void);
extern int emu_GetPad(void);
extern int emu_ReadAnalogJoyX(int min, int max);
extern int emu_ReadAnalogJoyY(int min, int max);
extern void emu_sndPlaySound(int chan, int volume, int freq);
extern void emu_sndPlayBuzz(int size, int val);
extern void emu_sndInit();
extern void emu_resetus(void);
extern int emu_us(void);

#endif
