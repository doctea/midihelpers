This work is licensed under [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/).

# midihelpers library

Helpers for some MIDI stuff common to my [usb_midi_clocker](https://github.com/doctea/usb_midi_clock) and [Microlidian](https://github.com/doctea/Microlidian) projects.

- Handle MIDI clock and timing, helper functions/macros to find the current MIDI time/position
- Optionally (define #USE_UCLOCK) use [uClock](https://github.com/midilab/uClock) for accurate & solid timing
  - Supports Teensy 4.0/4.1 and RP2040 platforms
- Functions to find the name of a MIDI note number (eg C5, D#3, etc)
- Keys and scales, with quantising and chord-building
- [mymenu](https://github.com/doctea/mymenu) widgets for:
  - displaying/adjusting BPM
  - showing track position
  - switching clock sources between Internal/External/None.
  - adjusting root key, scale, chords and inversions
- [parameters](https://github.com/doctea/parameters) library uses features of this for the 1v/oct tracking quantisation etc
- Defines for GM drum note numbers
- For RP2040 boards, set up MIDI USB with TinyUSB

# Requirements

- [vortigont/LinkedList](https://github.com/vortigont/LinkedList) library
- (if using USE_UCLOCK mode) [midiliab/uClock](https://github.com/midilab/uClock) library (version 2.0.0 or above)

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

# todo

- a class to make it simple to track which notes are held by an input or output
- ~~chord inversions~~
- fix envelopes
 - 'hold' stage doesnt work properly when inverted

---

# Native unit tests

Tests live in `test/native/` and run on the host (no hardware needed) via PlatformIO's native environment.

## Running tests

```bash
# From the midihelpers/ directory:
pio test -e native

# Or use the helper script (works from any directory):
bash scripts/run_tests.sh

# Verbose output (shows individual PASS/FAIL lines):
pio test -e native -vv
```

The test binary is built, run, and results printed automatically.  
All `test_*.cpp` files in `test/native/` are discovered automatically by PlatformIO.

## Test infrastructure

| Path | Purpose |
|---|---|
| `platformio.ini` | Defines the `native` env: build flags, include paths, saveloadlib dep |
| `test/native/arranger_test_stubs.cpp` | Global variable definitions needed by saveloadlib and bpm.h |
| `test/native/stubs/Arduino.h` | Arduino types (`String`, `SerialStub`, `F()`, `constrain()`) |
| `test/native/stubs/bpm.h` | Forwarder → real `include/bpm.h` (already native-compatible) |
| `test/native/stubs/LinkedList.h` | std::vector-backed LinkedList stub |
| `test/native/stubs/functional-vlpp.h` | std::function-backed vl::Func stub |
| `test/native/stubs/scales.h` | Unused (real scales.h guards Arduino includes itself) |

## Writing a new test

1. Add a `void test_my_feature()` function in `test/native/test_arranger.cpp`  
   (or create a new `test/native/test_foo.cpp` for a different class).

2. Use Unity assertion macros:
   ```cpp
   TEST_ASSERT_EQUAL_INT(expected, actual);
   TEST_ASSERT_EQUAL_UINT8(expected, actual);
   TEST_ASSERT_TRUE(expr);
   TEST_ASSERT_FALSE(expr);
   TEST_ASSERT_NULL(ptr);
   TEST_ASSERT_NOT_NULL(ptr);
   ```

3. Register it in `main()`:
   ```cpp
   RUN_TEST(test_my_feature);
   ```

4. Use the `tick_bars(Arranger* a, int n)` helper to advance time:
   ```cpp
   void test_my_feature() {
       Arranger a;
       a.setup_saveable_settings();
       // configure a.song_sections / a.playlist as needed
       a.on_restart();          // sets playlist_position=0, applies section 0
       tick_bars(&a, 4);        // advance 4 bars (calls on_bar() 4 times)
       TEST_ASSERT_EQUAL_INT(1, a.playlist_position);
   }
   ```

5. `setUp()` (called before every test) resets `ticks`, `ts_phase_offset`, and  
   `current_time_signature` to defaults automatically — no manual cleanup needed.

## Build flags (from platformio.ini)

```
-DENABLE_STORAGE -DENABLE_ARRANGER -DENABLE_TIME_SIGNATURE -DENABLE_SCALES
```

Add further `-DENABLE_*` flags there if testing code guarded by other defines.

