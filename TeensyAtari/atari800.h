extern void at8_Init(void);
extern void at8_Step(void);
extern void at8_Start(char * filename);

/////////////////////////////////////////////////////////////////////////////////////////////////////
// Atari 3 x 64 byte blocks of raw scancode: normal, shift+key, ctrl+key (4th Shift+Ctrl ignored here
// USB key number start at 0 null used for CAPS pressed
// 255 means undefined and stays at 255 (not 255+1=0)
// the value from here is increased by 1 for INPUT_key_code which goes to INPUT_Frame() (why?)
/////////////////////////////////////////////////////////////////////////////////////////////////////
const unsigned short usbkey2ScanCode[] = {  
  // If 0=255 then no CAPS effect must have it = 60 but does not toggle on A-Z, does toggle on 1-0 
  // CAP=60 USB=0                              TAB, RET,                          // USB CtrlI=9=TAB CtrlQ=Ctrl1=CtrlA==17 CtrlC=3
   60, 255, 255, 255, 255, 255, 255, 255, 255,  44,  12, 255, 255, 255, 255, 255, // 0 - 15 USB ShftENTER=CtrlENTER=10
  //                                                     ESC,                     //
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  28, 255, 255, 255, 255, //16 - 31 USB 
  // Normal key ////////////////////////////////////////////////////////////////////
  // ,   !,   ",   #,   $,   %,   &,   ',   (,   ),   *,   +,   ,,   -,   .,   /, // 32 - 47 USB CtrlSPC=0MMod1 ShftSPC=0MMod2
  // ,   !,   ",   #,   $,   %,   &,   ',   (,   ),   *,   +,   ,,   -,   .,   /, // ATARI 
   33,  95,  71,  90,  88,  93, 115,   7, 112, 114, 117,  79,  32,  14,  34,  38, //
  //0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   :,   ;,   <,   =,   >,   ?, // 48 - 63 USB
  //0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   :,   ;,   <,   =,   >,   ?, // ATARI  
   50,  31,  30,  26,  24,  29,  27,  51,  53,  48,  66,   2,  96,  15,  98, 102, //
  //@,   A,   B,   C,   D,   E,   F,   G,   H,   I,   J,   K,   L,   M,   N,   O, // 64 - 79 USB
  //@,   a,   b,   c,   d,   e,   f,   g,   h,   i,   j,   k,   l,   m,   n,   o, // ATARI  
   94,  63,  21,  18,  58,  42,  56,  61,  57,  13,   1,   5,   0,  37,  35,   8, // BSL = Backslash 
  //P,   Q,   R,   S,   T,   U,   V,   W,   X,   Y,   Z,   [, BSL,   ],   ^,   ?, // 80 - 95 USB 
  //p,   q,   r,   s,   t,   u,   v,   w,   x,   y,   z,   [, BSL,   ],   ^,   ?, // ATARI  
   10,  47,  40,  62,  45,  11,  16,  46,  22,  43,  23,  54,   6,  55,  91,  78, // 
  // Shift and Key /////////////////////////////////////////////////////////////////
  //@,   a,   b,   c,   d,   e,   f,   g,   h,   i,   j,   k,   l,   m,   n,   o, // 96 - 111 USB
  //@,   A,   B,   C,   D,   E,   F,   G,   H,   I,   J,   K,   L,   M,   N,   O, // ATARI  
  118, 127,  85,  82, 122, 106, 120, 125, 121,  77,  65,  69,  64, 101,  99,  72, // BS = Backspace 118 is clear screen
  //p,   q,   r,   s,   t,   u,   v,   w,   x,   y,   z,   [, BSL,   ],   ^,  BS, // 112 - 127 USB
  //P,   Q,   R,   S,   T,   U,   V,   W,   X,   Y,   Z,   [, BSL,   ],   ^,  BS, // ATARI  
   74, 111, 104, 126, 109,  75,  80, 110,  86, 107,  87,  54,   6,  70, 119,  52, // 118 is clear screen ({ and \ + shift) 119 is insert line ~ shift
  //
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 128 - 143 USB  
  //
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 144 - 159 USB 
  // Control and Key ///////////////////////////////////////////////////////////////
  255, 191, 149, 146, 186, 170, 184, 189, 185, 141, 129, 133, 128, 165, 163, 126, // 160 - 175 USB
  138, 175, 168, 190, 173, 139, 144, 174, 150, 171, 151, 255, 255, 255, 255, 255, // 176 - 191 USB
  //         F1,  F2,  F3,  F4,  F5,  F6,  F7,  F8,  F9, F10,                     // 192 - 207 USB
  //         F1   AtariXL   F4,HELP, Start Select Option INV,                     // ATARI  
  255, 255,   3,   4,  19,  20,  17, 199, 200, 201, 202,  39, 255, 255, 255, 255, // BS = Backspace
  //                  DEL,           LFT, RGT, DWN,  UP,                          // 208 - 224 USB
  //                   BS,           LFT, RGT, DWN,  UP,                          // ATARI  
  255, 255, 255, 255,  52, 255, 255, 206, 207, 247, 246, 255, 255, 255, 255, 255}; 

// Replaced case statement in atari.c below:  Careful no backslash characters
/*
  if (usbkey != usboldkey)
  { switch (usbkey) 
  {case 108: hk =  0; break; // L
   case  76: hk =  0; break; // l
   case 106: hk =  1; break; // J
   case  74: hk =  1; break; // j
   case  59: hk =  2; break; // ;
   case  58: hk = 66; break; // ;
   case 194: hk =  3; break; // AtariXL F1
   case 195: hk =  4; break; // AtariXL F2
   case 107: hk =  5; break; // K
   case  75: hk =  5; break; // K
   case  92: hk =  6; break; // + (PC Backslash and |)
   case 124: hk = 70; break; // Backslash is shifted + (PC backslash and |) 
   case  39: hk =  7; break; // * PC is ' " Atari is * ^ 
   case  34: hk = 71; break; // * PC is ' " Atari is * ^ 
   case 111: hk =  8; break; // O
   case 112: hk = 10; break; // P
   case 117: hk = 11; break; // U
   case  79: hk =  8; break; // O
   case  80: hk = 10; break; // P
   case  85: hk = 11; break; // U
   case  10: hk = 12; break; // RETURN
   case 105: hk = 13; break; // I
   case  73: hk = 13; break; // I
   case  45: hk = 14; break; // -
   case  61: hk = 15; break; // = PC is = + Atari is = |
   case  95: hk = 78; break; // | shifted -
   case  43: hk = 79; break; // _ shifted = PC is = + Atari is = |
   case 118: hk = 16; break; // V
   case  86: hk = 16; break; // V
   case 198: hk = 17; break; // F5 Help
   case  99: hk = 18; break; // C
   case  67: hk = 18; break; // C
   case 196: hk = 19; break; // AtariXL F3
   case 197: hk = 20; break; // AtariXL F4
   case  98: hk = 21; break; // B
   case 120: hk = 22; break; // X
   case 122: hk = 23; break; // Z
   case  66: hk = 21; break; // B
   case  88: hk = 22; break; // X
   case  90: hk = 23; break; // Z
   case  52: hk = 24; break; // 4
   case  51: hk = 26; break; // 3
   case  54: hk = 27; break; // 6
   case  36: hk = 88; break; // $ shifted 4
   case  35: hk = 90; break; // # shifted 3
   case  94: hk = 91; break; // & shifted 6
   case  27: hk = 28; break; // ESC
   case  53: hk = 29; break; // 5
   case  50: hk = 30; break; // 2
   case  49: hk = 31; break; // 1
   case  37: hk = 93; break; // % shifted 5
   case  64: hk = 94; break; // " shifted 2
   case  33: hk = 95; break; // ! shifted 1
   case  44: hk = 32; break; // PC is < Atari , [
   case  60: hk = 96; break; // Atari [ is shifted , (PC <)
   case  32: hk = 33; break; // SPACE
   case  46: hk = 34; break; // PC is > Atari . ]
   case  62: hk = 98; break; // Atari ] is shifted . (PC >)
   case 110: hk = 35; break; // N
   case 109: hk = 37; break; // M
   case  78: hk = 35; break; // N
   case  77: hk = 37; break; // M
   case  47: hk = 38; break; // /
   case  63: hk =102; break; // /
   case 203: hk = 39; break; // F10 Atari INVERSE
   case 114: hk = 40; break; // R
   case 101: hk = 42; break; // E
   case 121: hk = 43; break; // Y
   case  82: hk = 40; break; // R
   case  69: hk = 42; break; // E
   case  89: hk = 43; break; // Y
   case   9: hk = 44; break; // TAB
   case 116: hk = 45; break; // T
   case 119: hk = 46; break; // W
   case 113: hk = 47; break; // Q
   case  84: hk = 45; break; // T
   case  87: hk = 46; break; // W
   case  81: hk = 47; break; // Q
   case  57: hk = 48; break; // 9
   case  48: hk = 50; break; // 0
   case  55: hk = 51; break; // 7
   case  40: hk =112; break; // { shifted 9
   case  41: hk =114; break; // } shifted 0
   case  38: hk =115; break; // ' shifted 7
   case 212: hk = 52; break; // DELETE make it Atari backspace
   case 127: hk = 52; break; // BACKSPACE Atari backspace
   case  56: hk = 53; break; // 8
   case  42: hk =117; break; // @ shifted 8
   case  91: hk = 54; break; // <  PC is [ { Atari is < Clear
   case  93: hk = 55; break; // >  PC is ] } Atari is > Insert
   case 123: hk =118; break; // Clear Screen shifted < (PC is [)
   case 125: hk =119; break; // Insert Line shifted > (PC is ]}
   case 102: hk = 56; break; // F
   case 104: hk = 57; break; // H
   case 100: hk = 58; break; // D
   case  70: hk = 56; break; // F
   case  72: hk = 57; break; // H
   case  68: hk = 58; break; // D
   case   0: hk = 60; break; // CAPS
   case 103: hk = 61; break; // G
   case 115: hk = 62; break; // S
   case  97: hk = 63; break; // A
   case  71: hk = 61; break; // G
   case  83: hk = 62; break; // S
   case  65: hk = 63; break; // A
   //case  96: hk = 60; break; // PC is ` ~
       
   //case 194: hk =  3; break; // F1 AtariXL 
   //case 195: hk =  4; break; // F2 AtariXL
   //case 196: hk = 19; break; // F3 AtariXL
   //case 197: hk = 20; break; // F4 AtariXL
   //case 198: hk = 17; break; // F5 is Help
   case 199: j |= MASK_KEY_USER1; break; // F6 Start  Port D01F D0
   case 200: j |= MASK_KEY_USER2; break; // F7 Select Port D01F D1 
   case 201: j |= MASK_KEY_USER3; break; // F8 Option Port D01F D2
   case 202: j |= MASK_KEY_USER4; break; // F9 Reset
   //case 203: hk =  39; break; // F10 Inverse 
   //case 204: hk =  32; break; // F11 is ,
   //case 205: hk =  34; break; // F12 is .
   case 215: hk = 206; break; // LEFT
   case 216: hk = 207; break; // RIGHT
   case 217: hk = 247; break; // DOWN
   case 218: hk = 246; break; // UP
   //case 215: j |= MASK_JOY2_LEFT;  break; // LEFT
   //case 216: j |= MASK_JOY2_RIGHT; break; // RIGHT
   //case 217: j |= MASK_JOY2_DOWN;  break; // DOWN
   //case 218: j |= MASK_JOY2_UP;    break; // UP


   case 214: hk = 55; break; // PAGEUP
   case 211: hk = 56; break; // PAGEDOWN
  

   default:  break;
   }
  usboldkey = usbkey;
  
  }
*/
