# joyemu

This is a quick hack for enabling the use of either wireless Bluetooth and/or wired USB gamepads and mice on retro 8-bit and 16-bit computers such as Commodore Amiga and C64.

It works by reading events from HID input devices and emulating the pins of DB9 joysticks and a mouse on a Raspberry Pi and sending the signals via a 16-channel I/O board using the MCP23017 expander connected via I2C.

I've tested this code with two PS3 Sixaxis controllers (both in wired USB and Bluetooth mode) and an XBOX 360 wireless controller (via the USB adapter). The mouse support has been tested with a Microsoft 3600 Bluetooth Mouse and a wired SteelSeries Sensei Raw.

Any USB and Bluetooth HID devices are supported, as long as they present a Linux event device that can be accessed as `/dev/input/event[0-9]+`.



### Compiling

For Raspbian users, install packages `libevdev-dev` and `libi2c-dev` to compile with the provided Makefile using GNU Make.



### Usage

```
Usage: joyemu [-vqh] [-i i2c_bus] [-a i2c_addr] [-d (j1|j2|m):evdev_nr] [-m mouse_port] [-j joystick_port]

	-v	add verbosity
	-q	add quietness
	-i n	set I2C bus number for I/O expander (default: 1)
	-a 0xnn	set I2C address for I/O expander as a hexadecimal byte (default: 0x20)
	-d j1:n	set event device number for joystick 1
	-d j2:n	set event device number for joystick 2
	-d m:n	set event device number for mouse
	-m n	set mouse port: 1 (default) or 2
	-j n	set first joystick port: 1 or 2 (default)
	-h	display this help
```

Note that if you log very verbosely to the console, the response - especially that of the mouse - may begin to lag behind the inputs. Only use the more verbose debugging levels for actual debugging.



### Hardware

I've developing this on a Raspberry Pi Zero W and an IO Pi Zero expander board from [AB Electronics](https://www.abelectronics.co.uk). The reason I'm using a separate I/O expander is that the Atari-style DB9 joystick ports are generally active-low, so the pins on the computer end are pulled up to +5V. On a Commodore 64 the pull-ups are internal to the CIA chips, whereas on an Amiga A500 or A1200 an inline resistor pack is used. The GPIO pins on the Raspberry Pi are **not** safe for +5V so they cannot be used unless an external level conversion is added.

Any other I/O board (or built-in GPIOs with level conversion) would probably work equally well, as long as it sends 0V..+5V and tolerates the +5V pull-ups. Of course, you'd also have to rewrite `io.c` and `io.h` accordingly.

GPIO lines on the MCP23017 are connected to DB9 pins as follows:

|Connector|up|down|left|right|fire1|fire2|
|---|---|---|---|---|---|---|
|DB9 1/2|1|2|3|4|6|9|
|GPIOA/GPIOB|0|1|2|3|4|5|

Bits 6 and 7 on GPIOA and GPIOB are unused.

I've added a 2x8 pin header on the I/O board and built a cable that connects the corresponding GPIO pins to two female DB9 connectors. Remember to also connect the ground plane on the I/O board with the ground pin on the DB9 connectors (pin 8).



### License
 
Copyright (c) Jani Halme 2017

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
