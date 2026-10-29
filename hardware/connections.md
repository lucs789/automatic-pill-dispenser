# Hardware Connections

This document records the pin assignments of the Arduino-based pill dispenser prototype. It is based on the final firmware [**PillDispenser.ino**](../firmware/PillDispenser.ino).

## Overview: Arduino signal pins

| Arduino pin | Connected component | Signal |
| --- | --- | --- |
| **D2** | Pushbutton | Button input, active LOW (`INPUT_PULLUP`) |
| **D3** | LCD 16×2 | LCD data D6 |
| **D4** | LCD 16×2 | LCD data D7 |
| **D5** | LCD 16×2 | Backlight positive, through a **220 Ω resistor** |
| **D8** | ULN2003 driver | **IN4** |
| **D9** | ULN2003 driver | **IN3** |
| **D10** | ULN2003 driver | **IN2** |
| **D11** | ULN2003 driver | **IN1** |
| **D12** | Buzzer | Positive terminal |
| **A0** | LCD 16×2 | RS (register select) |
| **A1** | LCD 16×2 | E (enable) |
| **A2** | LCD 16×2 | LCD data D4 |
| **A3** | LCD 16×2 | LCD data D5 |
| **A4** | DS3231 RTC | SDA (I²C data) |
| **A5** | DS3231 RTC | SCL (I²C clock) |


## DS3231 real-time clock (RTC)

| DS3231 connection | Connect to |
| --- | --- |
| GND (negative) | GND |
| VCC | 5 V |
| SDA | Arduino **A4** |
| SCL | Arduino **A5** |

The RTC communicates with the Arduino over I²C. The corresponding firmware uses `Wire.h` and `RTClib.h`.


## Pushbutton

| Button connection | Connect to |
| --- | --- |
| Ground side | GND |
| Signal side | Arduino **D2** |


## Buzzer

| Buzzer connection | Connect to |
| --- | --- |
| Negative (−) | GND |
| Positive (+) | Arduino **D12** |


## 28BYJ-48 stepper motor and ULN2003 driver

| ULN2003 driver connection | Connect to |
| --- | --- |
| GND (negative) | GND |
| VCC (positive) | **5 V** |
| IN1 | Arduino **D11** |
| IN2 | Arduino **D10** |
| IN3 | Arduino **D9** |
| IN4 | Arduino **D8** |

The stepper motor connects to the **ULN2003 driver board**, not directly to the Arduino's output pins.

As stated in [limitations and potential improvements](/README.md#limitations-and-potential-improvements), the addition of an **external 5 V power supply** is recommended. Also make sure that, if an external supply is used, its **GND must be connected to the Arduino GND** so the control signals have a common reference.


## 16×2 LCD (parallel, 4-bit mode)

| LCD pin | Label | Connect to | Description |
| --- | --- | --- | --- |
| 1 | VSS | **GND** | Power ground |
| 2 | VDD | **5 V** | Power supply |
| 3 | V0 | **GND** | Contrast input |
| 4 | RS | Arduino **A0** | Register select |
| 5 | RW | **GND** | Write-only operation |
| 6 | E | Arduino **A1** | Enable |
| 7 | D0 | Not connected | Unused in 4-bit mode |
| 8 | D1 | Not connected | Unused in 4-bit mode |
| 9 | D2 | Not connected | Unused in 4-bit mode |
| 10 | D3 | Not connected | Unused in 4-bit mode |
| 11 | D4 | Arduino **A2** | Data |
| 12 | D5 | Arduino **A3** | Data |
| 13 | D6 | Arduino **D3** | Data |
| 14 | D7 | Arduino **D4** | Data |
| 15 | LED+ | Arduino **D5**, through **220 Ω** | Backlight positive, controlled by firmware |
| 16 | LED− | **GND** | Backlight negative |

The firmware controls backlight output on **D5**. Grounding V0 is the current contrast configuration; other displays may need a potentiometer for readable contrast. Confirm that the installed LCD backlight and the Arduino pin can operate within the current ratings, including the 220 Ω series resistor.
