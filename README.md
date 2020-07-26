# USB Keyboard modification of M.CU.M.E = Multi CompUter Machine Emulator  

All credit to [**MCUME**](https://github.com/Jean-MarcHarvengt/MCUME) for the emulation software and [**PJRC**](https://www.pjrc.com/teensy/) for the hardware and libraries.

For the fidelity of this simulation refer to the photo below, which compares an upgraded 600XL with the simulated Atari Graphics Mode 0 as presented here.
<p align="center">
<img src="real-simulated.jpg" width="940" />  
<br>

I have added a standard usb keyboard as an input device to the MCUME emulator, for a Teensy 4.0 with a external SDCard (first and second photos below), Teensy 4.1 using either its built-in SDCard (third photo), or an external SDCard, and a Teensy 3.6 using its built-in SDCard (fourth and fifth photos), where the USB keyboard is attached to their onboard USB hubs. The Teensy 4.0 used the two bottom USB hub pins D- and D+ and +5v for Vusb, for the hub. I intend to further modify and develop the Atari 800/800XL emulations in the future, in part because these are the only working original 1980's hardware (and original OS-ROMs), I have - also see [**Atari600XL-Upgrades**](https://github.com/TobiasVanDyk/Atari600XL-Upgrades).

The code (as inside the Teensy40Atari and Teensy41Atari folders), is fully functional - including working sound - for the Teensy 4.1, and 3.6, and partly functional for the Teensy 4.0 - it still needs to have its sound enabled. All were tested running Atari Basic plus a number of games. The display is an [**Adafruit ST7789 240x320 IPS display**](https://learn.adafruit.com/2-0-inch-320-x-240-color-ips-tft-display), which also has an SDCard holder, used for the Teensy 4.0.

As mentioned, for the Teensy 4.0 the SDcard adapter on the display was used, and SDCS was connected to pin 8 (Teensy 4.0) and MISO to pin 12 (Teensy 4.0). This is the configuration as used in the Teensy40Atari folder. To use the Teensy 4.1 and Teensy 3.6 onboard SDCard select the required option in platform_config.h (comment out 
EXTERNAL_SD), which is the configuration used in the Teensy41tari folder.

### Schematic 
If the built-in SDCard of the Teensy 4.1 or 3.6 is used the the connections to pin 8 and pin 12 are not required. None of the joystick switches are essential as the ROM selection can be done using the usbkeyboard. For all three Teensy's the schematic is then as below, with all possible input and output devices:
<br>
<p align="center">
<img src="schematic2.jpg" width="900" />  
<br>
  
### Testing (Teensy 4.0):
<br>
<p align="center">
<img src="Teensy40a.jpg" width="840" />  
<br>

<br>
<p align="center">
<img src="Teensy40b.jpg" width="840" />  
<br>
  
### Testing (Teensy 4.1):
<br>
<p align="center">
<img src="photo4a.jpg" width="840" />  
<br>
  
### Testing (Teensy 3.6):
<br>
<p align="center">
<img src="photo1a.jpg" width="840" />  
<br>

<br>
<p align="center">
<img src="photo3a.jpg" width="840" />  
<br>

### Adafruit ST7789 240x320 IPS display 
From [**Adafruit**](https://learn.adafruit.com/2-0-inch-320-x-240-color-ips-tft-display):
<br>
<p align="center">
<img src="Adafruit 2inch 320x240 Color IPS TFT Display.jpg" width="640" />  
<br>

### An alternative solution for the Teensy 4.0 SDCard: 
From [**PZ1-6502-Laptop**](https://hackaday.io/project/171471-pz1-6502-laptop):
<br>
<p align="center">
<img src="teensynano6502.jpg" width="640" />  
<br>


**To do 1**: *Define, Break, Reset keys properly - all other usb keyboard keys are now working as they should.*

**To do 2**: **Sound on the Teensy 4.1 using its built-in SDcard is working (BASIC: SOUND 2,100,10,10).** 

*But when using  an external SDCard for both the 4.0 and 4.1 the sound is not working. I have changed as suggested the #define PIN_SPI_SS (10) to (22) and also tried (8) in pins_arduino.h in the teensy4 folder, but there is still no MQSR output from pin 10.*

### Teensy 4.1 playing sound:

<br>
<p align="center">
<img src="Teensy41Sound.jpg" width="940" />  
<br>
