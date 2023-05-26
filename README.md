# CyBorg-Southbridge
[![Build Status](https://github.com/cyrusbuilt/CyBorg-Southbridge/actions/workflows/ci.yml/badge.svg)](https://github.com/cyrusbuilt/CyBorg-Southbridge/actions?query=workflow%3APlatformIO)

Firmware for the [CyBorg](https://github.com/cyrusbuilt/CyBorg) Southbridge

## What does it do?
- Provides ATX power management functions
- Provides an integrated terminal for CyBorg which includes:
- Mono audio output
- PS/2 keyboard and mouse support
- VGA display output
- Functions almost identical to [CyrUX](https://github.com/cyrusbuilt/CyrUX)

## How do I use it?
This project was built using [PlatformIO](https://platformio.org/), so you'll need to follow the instructions to download and install it if you haven't already. Then clone this repo and in a terminal cd to the project directory.  Now to build build and flash it:

First, you'll need an appropriate FTDI cable (like [this one](https://www.adafruit.com/product/70?gclid=CjwKCAjwscGjBhAXEiwAswQqNB2296-7Bv-Y7F56Dpd_glp_bBPHJRNljWBkb1Cpdc8x13ulUFtHbBoCCFYQAvD_BwE)). Then remove jumper JP1 (ATX PWR EN) and jumpers JP3 and JP4 (CONSOLE ENABLE) from the CyBorg board. Now plug the female dupont connector of the FTDI cable into male pin header H1 (RS-232 DBG/PGM) making sure to align the pins correctly (make sure the pin with the black wire goes to the header pin marked GND). Now plug the other end of the cable into a USB port on your computer. On the Huzzah ESP32 module, press and hold the RESET button. Now quickly press the GPIO0 button and release both at the same time. If you did it right, the module should be in program mode (see details [here](https://www.adafruit.com/product/4172?gclid=CjwKCAjwscGjBhAXEiwAswQqNMcS2f32s5RfFQoHY0PYp_EiqRDCuYAiohc85fFMKXRpA3U_ImASvRoCA4sQAvD_BwE)). Once in program mode, make sure the `upload_port` and `monitor_port` value in `platformio.ini` matches the device path/name for your FTDI cable. Now run the following command from the terminal:

```sh
pio run -t upload -t monitor
```

When the command finishes, the firmware should be successfully uploaded. At this point, you should be able to just press and release the RESET button on the Soutbridge (Huzzah ESP32) and see it boot up in the terminal. If all went well, you can press CTRL+C to terminate the monitor command and then disconnect the FTDI cable and put the jumpers back.
