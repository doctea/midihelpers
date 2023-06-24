# midihelpers library

Helpers for some MIDI stuff common to my [usb_midi_clocker](https://github.com/doctea/usb_midi_clock) and [Microlidian](https://github.com/doctea/Microlidian) projects.

- Handle MIDI clock and timing, helper functions/macros to find the current MIDI time/position
- Functions to find the name for a note
- [mymenu](https://github.com/doctea/mymenu) widgets for displaying/adjusting BPM, showing track position, and selecting clock source between Internal/External/None.
- Defines for GM drum note numbers
- For RP2040 boards, set up MIDI USB with TinyUSB


# Setting up MIDI and USB MIDI on RP2040 boards

## USB MIDI device

- add -DUSE_TINYUSB
- define -DUSB_MIDI_VID=0x1337 vendor ID
- define -DUSB_MIDI_PID=0xBEEF product ID
- define -DUSB_MIDI_MANUFACTURER="\"TyrellCorp\"" manufacturer string
- define -DUSB_MIDI_PRODUCT="\"FunkyDevice\"" product string

## Serial/DIN MIDI output

- define -DUSE_DINMIDI
- define -DMIDI_SERIAL_SPIO
- define -DMIDI_SERIAL_OUT_PIN=D6 or whatever pin you're using for output

## todo

- a class to make it simple to track which notes are held by an input or output
