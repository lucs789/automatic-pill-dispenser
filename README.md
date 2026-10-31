# Automatic Pill Dispenser

An Arduino-based educational prototype for scheduled pill dispensing, developed by a three-student robotics team for **COC Floripa´s 2026 science fair** (Jornada Científica).

The project addresses the difficulty of remembering multiple medication times, especially for older adults. At a configured day and time, a stepper motor rotates a 3D-printed compartment wheel. An LCD and buzzer provide a visual and audible reminder. **This is a classroom proof of concept**, not a validated medical device: it must not be relied upon to store, organize, or dispense real medication.

**Science fair theme:** *Education in Focus: Science, Environment, and Quality of Life* (translated from Portuguese).

<img width="1708" height="1012" alt="image" src="https://github.com/user-attachments/assets/7fb96133-722a-4f36-86c1-1c77987d6cc6" />

## How it works

- The DS3231 real-time clock (RTC) supplies the current date and time.
- A single pushbutton and the 16×2 LCD allow a user to configure recurring weekly alarms.
- The Arduino checks whether the current weekday and time match a saved alarm.
- On a match, the firmware writes a record of the triggered minute to EEPROM (this guarantees the system won´t trigger multiple times while alarm and clock are in the same minute), then commands a 28BYJ-48 stepper motor to advance the dispensing wheel by **256 steps** (nominally 1/8 of a revolution using the firmware's 2,048-step assumption).
- The display prompts the user to collect the pill, while the buzzer and LCD backlight provide an alert. The user presses the button to acknowledge it.

[**youtubeShowcase**](https://youtu.be/HrkETQ9qOc4): a short video that shows the prototype working.


### Implemented in the supplied firmware

| Feature | Behavior |
| --- | --- |
| Passive display | Shows weekday, date, time, and the **RTC´s chip temperature** (not pill temperature); LCD backlight is off. |
| Weekly scheduling | Up to **24 alarms**; minutes selectable in 15-minute increments. Alarms are stored in the Arduino's EEPROM. |
| Alarm activation | Moves the stepper by 256 steps, then displays an alert with buzzer and flashing LCD backlight. |
| Clock configuration | Date and time can be configured using the pushbutton and LCD. **Setting the RTC clears all existing alarms.** |
| Idle behavior | Configuration screens time out after 60 seconds without interaction. |

These capabilities are identifiable in the source code; **end-to-end reliability and dispensing accuracy have not been established by a formal test campaign**.

## Hardware

| Component | Purpose |
| --- | --- |
| Arduino UNO R3 | Runs the firmware and controls the electronics. |
| DS3231 RTC module | Keeps time and provides a temperature reading from itself. |
| 28BYJ-48 stepper motor and ULN2003 driver | Rotates the dispensing mechanism. |
| 16×2 parallel LCD | Shows time, menus and alerts. |
| Pushbutton | Configures settings and acknowledges reminders. |
| Buzzer | Provides an audible notification. |
| Breadboard, jumpers and power source | Connects and powers the prototype. |
| 3D-printed compartment wheel | Holds positions for items to be released by rotation. |

The **firmware-derived pin mapping** is in [hardware/connections.md](hardware/connections.md).

## 3D-printed mechanism

The source file is in [3d-model/wheel.stl](3d-model/wheel.stl). The supplied mesh has a bounding box of approximately **80 × 80 × 20 model units**. STL files do not embed measurement units: be aware that, before printing, each model unit should be equivalent to **1 millimeter**.

The firmware assumes eight indexed dispensing positions, advancing 256 of an assumed 2,048 steps for each dispense. Actual angular travel and alignment must be verified on the finished assembly.

## Firmware and software requirements

- [**PillDispenser.ino**](firmware/PillDispenser.ino): scheduled operation and menu interface.
- [**PillDispenserPrototype.ino**](firmware/PillDispenserPrototype.ino): an earlier diagnostic sketch showing RTC readings, LCD output, button/buzzer behavior, LCD backlight switching and motor testing.

### Required Arduino libraries:

- `Wire` (Arduino core)
- `RTClib` (Adafruit)
- `LiquidCrystal` (Arduino)
- `Stepper` (Arduino)
- `EEPROM` (Arduino)

### Uploading the firmware

1. Install the Arduino IDE and, if necessary, the board package for your particular Arduino-compatible board.
2. Install the Adafruit `RTClib` library and its dependencies through the Library Manager.
3. Open `firmware/PillDispenser.ino`, select the correct board and port, and upload.
4. Check the RTC and use the clock-setting interface if the RTC reports an invalid time or lost power.
5. Configure an alarm and test the device with **inert substitute items** only.

## One-button controls

| Current screen | Interaction | Action |
| --- | --- | --- |
| Passive | Hold 2–5 seconds, then release | Begin creating an alarm. |
| Passive | Hold 5 seconds or longer, then release | Begin adjusting the RTC. |
| Alarm settings | Short press | Increment minutes (15 min), hours (1 hour), or weekday, depending on the step. |
| Alarm settings | Hold 2 seconds or longer, then release | Advance to next field or save the alarm. |
| Clock settings | Short press | Increment the currently selected time/date field. |
| Clock settings | Hold 2 seconds or longer, then release | Advance to the next field or save the clock settings. |
| Active alert | Press and release | Acknowledge the reminder. |

## Limitations and potential improvements

The current interface does not expose a menu for listing, deleting or editing **individual** saved alarms. Reconfiguring the RTC clears all saved alarms.

Every time the 28BYJ-48 stepper motor rotates, **a voltage loss normally occurs**, so it is necessary to re-start the LCD screen right after it stops.

Possible future improvements include a better alarm-management interface, power-loss testing, wheel position feedback and the addition of an external motor power supply. These are **proposals**, not implemented features.

## Development and contributions

This project was completed as part of COC Floripa´s robotics elective by three students. It was inspired by the Science Buddies automatic pill dispenser reference below. The group adapted the mechanism and implementation, including a stepper motor and a custom 3D-printed wheel.

**Team credits**

- Student 1: **Lucas Mussi Cordioli** - electronics, firmware, testing.
- Student 2: **Heitor Zenari Santos Oliveira** - initial design, testing.
- Student 3: **Victor Lambert** - 3D design, 3D printing, testing.
- Teacher: **Carlos Henrique Ries** - Guide/advisor, hardware supplier.

**AI disclosure:** ChatGPT was used as an assistant during development of the final Arduino firmware. The group remains responsible for testing, understanding, and accurately describing the implemented behavior.

## References

- Science Buddies - *Automatic Pill Dispenser*, the project's initial inspiration: [**ScienceBuddies**](https://www.sciencebuddies.org/science-fair-projects/project-ideas/Elec_p105/electricity-electronics/automatic-pill-dispenser)
- RTClib - Adafruit´s Library for the RTC-DS3231: [**RTClib**](https://github.com/adafruit/rtclib)

The included source files and mechanical model are contributions from the student project and **are not copies of Science Buddies' diagrams or text**.
