# Cam87-Astrogatines-Edition

## This repository contains all files needed to build the astronomy Cam87 color camera with a APSC CCD sensor. The resolution of this camera is 6M pixels (3000x2000). Original wiring diagram was made by grim from Astroclub.kiev.ua.

## Here are the implementation I made :

- Keep gastight Hammond aluminium enclosure for best low humidity performance and interference shield.
- Power board with all high frequency components are outside the metal box.
- New PCB checked with 4 layers. All logic components on the back side of the sensor side, 3 layers of tracks and a inner full copper plane for best grounding.
- New PID controlled by an ESP32 including a webserver and a new webapp ! Access to sensor and case temperature, box humidity and a temperature graph with your phone or a computer (wifi connexion).
- New design with a aluminium cold finger pressed against the metal box with a 3D printed part. No machining anymore, the .step file is inclued to let it jlccnc do the job !
- Stl files provided for power board holder and enclosure with ESP32 location. Black sensor mask, Cold finger insulator. I provided also a shim to help drilling the lid at the right place.

## Folders :

First folders are numbered for each installation step (Look at the tutorial for each step) : 

- 1 - Drivers FT2232 : Drivers for Windows for the FT2232. I provided 2 drivers, you can check for updates on FTDI website, type FT2232HL and install lastest drivers. WARNING : Since several years FTDI brick non genuine chips. If you purchased your chip on an official reseller, you can update to last driver. If your chip comes from Aliexpress or EBay, be careful about that, if you have any doubt, install the 2.08 provided driver.

- 2 - Flash FT2232 : This is the firmware and the programming utility for flashing the FT2232 chip

- 3a - Viewer : This is the standalone viewer to use with this camera, developped by grim.

- 3b - Flash STM32 : This is the STM32 flash file and the ST-LINK utility for flashing.

- ESP32 PID : In this folder you can found the .ino file for the ESP32 PID and the files to upload on the board for the webserver.

- Gerbers : gerbers files to make the 4 layers PCB (choose 1 oz for each layer)

- Old ASCOM Platform : This is the ASCOM platform version 6.2 that works with this camera, I  haven't tried newer versions.

- Parts : Here you have the list of components and screen captures to see what parts you need. Also some diagrams.

- STL and STEP Files : This is the stl files for 3D printing and the step file for aluminium CNC machining.

- Wiring diagram : The corrected wiring diagram including the new ESP32 PID and the new cooling diagram.


	- SSID for PID : Cam87 Cooling
	- Password : Value of the speed of the photons that reach your nice camera, in m/s ;-)
	- IP for access : 192.168.4.1

FTDI : https://ftdichip.com/
Astroccd : http://astroccd.org/
Ukrainian Astropolis topic : https://www.astroclub.kiev.ua/forum/index.php?PHPSESSID=su8vh9h1n0l7lrnate4rmb50v5&topic=28929.0