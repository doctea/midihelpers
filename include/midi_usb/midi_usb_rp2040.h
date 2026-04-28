#pragma once

#ifndef RP2040OutputWrapperClass
    #define RP2040OutputWrapperClass RP2040DualMIDIOutputWrapper
#endif

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

// MIDI + USB
#ifdef USE_TINYUSB
    #include <Adafruit_TinyUSB.h>
    #include <atomic>
#endif
#include <MIDI.h>

//usb midi
#ifdef USE_TINYUSB
    extern Adafruit_USBD_MIDI usb_midi;
    extern midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>> USBMIDI;
#endif

#ifdef MIDI_SERIAL_SPIO
    extern midi::MidiInterface<midi::SerialMIDI<SerialPIO>> DINMIDI;
#endif

#ifdef MIDI_SERIAL_HARDWARE
    extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> DINMIDI;
#endif

#ifdef MIDI_SERIAL_SOFTWARE
    #include <SoftwareSerial.h>
    extern midi::MidiInterface<midi::SerialMIDI<SoftwareSerial>> DINMIDI;
#endif

#include "midi_helpers.h"
#include "midi_usb/midi_ring_buffer.h"   // non-blocking MIDI queue: enqueue from ISR/ATOMIC, drain from main loop

void setup_midi();
void setup_usb();

#if __has_include("outputs/output.h")
    #define USE_SEQLIB_OUTPUTS  // use outputs from seqlib https://github.com/doctea/seqlib
    #include "outputs/output.h"
#endif


#ifdef ENABLE_CLOCK_INPUT_CV
    // todo: not convinced this should be handled by this library; maybe something that should be part of midihelpers instead?
    // since its really to be used by Microlidian only, in the RP2040DualMIDIOutputWrapper...
    void set_external_cv_ticks_per_pulse_values(uint32_t new_value);
    uint32_t get_external_cv_ticks_per_pulse_values();
#endif

// todo: port usb_midi_clocker's OutputWrapper to work here?
// wrapper class to wrap different MIDI output types; handles both USB MIDI and DIN MIDI outputs
class RP2040DualMIDIOutputWrapper : virtual public IMIDINoteAndCCTarget 
    #ifdef ENABLE_STORAGE
        , public SHDynamic<0, 8> // no children; 4 settings
    #endif
    {
    public:

    RP2040DualMIDIOutputWrapper() {
        #ifdef ENABLE_STORAGE
             this->set_path_segment("MIDIWrapper");
        #endif
    }

    #ifdef USE_TINYUSB
        midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>> *usbmidi = &USBMIDI;
    #endif
    #ifdef USE_DINMIDI
        midi::MidiInterface<midi::SerialMIDI<SerialPIO>> *dinmidi = &DINMIDI;
    #endif

    #ifdef USE_SEQLIB_OUTPUTS
        OUTPUT_TYPE output_mode = DEFAULT_OUTPUT_TYPE;
        void set_output_mode(int m) {
            this->output_mode = (OUTPUT_TYPE)m;
            // todo: if changed then we need to kill all the playing notes to avoid them getting stuck
        }
        OUTPUT_TYPE get_output_mode() {
            return this->output_mode;
        }
    #endif

    uint32_t din_midi_clock_output_divider = 1; //PPQN; // by default, send a clock message for every internal clock tick; but allow this to be changed via the menu, to make the clock output more useful for midi->cv output situations
    void set_din_midi_clock_output_divider(uint32_t new_value) {
        din_midi_clock_output_divider = new_value;
    }
    uint32_t get_din_midi_clock_output_divider() {
        return din_midi_clock_output_divider;
    }

    // Enqueues note-on for deferred send from drain().
    // DIN converter_func is applied in drain() at send time.
    virtual void sendNoteOn(uint8_t pitch, uint8_t velocity, uint8_t channel) override {
        if (!is_valid_note(pitch)) return;
        uint8_t dest = 0;
        #ifdef USE_TINYUSB
            dest |= MIDI_DEST_USB;
        #endif
        #ifdef USE_DINMIDI
            dest |= MIDI_DEST_DIN;
        #endif
        midi_queue.enqueue({ MIDI_MSG_NOTE_ON, dest, channel, pitch, velocity });
    }
    // Enqueues note-off for deferred send from drain().
    // DIN converter_func is applied in drain() at send time.
    virtual void sendNoteOff(uint8_t pitch, uint8_t velocity, uint8_t channel) override {
        if (!is_valid_note(pitch)) return;
        uint8_t dest = 0;
        #ifdef USE_TINYUSB
            dest |= MIDI_DEST_USB;
        #endif
        #ifdef USE_DINMIDI
            dest |= MIDI_DEST_DIN;
        #endif
        midi_queue.enqueue({ MIDI_MSG_NOTE_OFF, dest, channel, pitch, velocity });
    }

    // Non-blocking MIDI output queue.
    // Producers (ISR, ATOMIC regions on Core 0) call enqueue() — fast, never blocks.
    // drain() (main loop, outside ATOMIC) does the actual USB/DIN sends.
    MidiRingBuffer midi_queue;

    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) override {
        if (!this->is_cc_output_enabled()) return;
        if (!is_valid_note(number)) return;
        uint8_t dest = 0;
        #ifdef USE_TINYUSB
            dest |= MIDI_DEST_USB;
        #endif
        #ifdef USE_DINMIDI
            dest |= MIDI_DEST_DIN;
        #endif
        midi_queue.enqueue({ MIDI_MSG_CONTROL_CHANGE, dest, channel, number, value });
    }

    virtual void sendClock() {
        // Destination is resolved now while `ticks` is current (DIN clock divider check).
        uint8_t dest = 0;
        #ifdef USE_TINYUSB
            dest |= MIDI_DEST_USB;
        #endif
        #ifdef USE_DINMIDI
            if (ticks % this->get_din_midi_clock_output_divider() == 0)
                dest |= MIDI_DEST_DIN;
        #endif
        if (dest) midi_queue.enqueue({ MIDI_MSG_CLOCK, dest, 0, 0, 0 });
    }
    virtual void sendStart() {
        uint8_t dest = 0;
        #ifdef USE_TINYUSB
            dest |= MIDI_DEST_USB;
        #endif
        #ifdef USE_DINMIDI
            dest |= MIDI_DEST_DIN;
        #endif
        if (dest) midi_queue.enqueue({ MIDI_MSG_START, dest, 0, 0, 0 });
    }
    virtual void sendStop() {
        uint8_t dest = 0;
        #ifdef USE_TINYUSB
            dest |= MIDI_DEST_USB;
        #endif
        #ifdef USE_DINMIDI
            dest |= MIDI_DEST_DIN;
        #endif
        if (dest) midi_queue.enqueue({ MIDI_MSG_STOP, dest, 0, 0, 0 });
    }

    // ── MIDI drain ───────────────────────────────────────────────────────────
    // Call from main loop() before any ATOMIC() blocks so the USB TX FIFO drain
    // ISR (USBCTRL_IRQ) can preempt freely.
    // This is the ONLY site where actual USB/DIN hardware sends happen.
    //
    // max_micros bounds the worst-case time spent per loop() call.  If a send
    // blocks (USB SOF wait), drain() stops after the budget expires and remaining
    // messages stay in the ring buffer for the next iteration.
    //
    // Returns number of messages processed this call.
    int drain(uint32_t max_micros = 800) {
        uint32_t start = micros();
        int count = 0;
        MidiMessage msg;
        while (midi_queue.dequeue(msg)) {
            count++;

            #ifdef USE_TINYUSB
            if (msg.destinations & MIDI_DEST_USB) {
                switch (msg.type) {
                    case MIDI_MSG_NOTE_ON:
                        usbmidi->sendNoteOn(msg.data1, msg.data2, msg.channel);
                        break;
                    case MIDI_MSG_NOTE_OFF:
                        usbmidi->sendNoteOff(msg.data1, msg.data2, msg.channel);
                        break;
                    case MIDI_MSG_CONTROL_CHANGE:
                        usbmidi->sendControlChange(msg.data1, msg.data2, msg.channel);
                        break;
                    case MIDI_MSG_CLOCK:
                        usbmidi->sendClock();
                        break;
                    case MIDI_MSG_START:
                        usbmidi->sendStart();
                        break;
                    case MIDI_MSG_STOP:
                        usbmidi->sendStop();
                        break;
                    default:
                        break;
                }
            }
            #endif  // USE_TINYUSB

            #ifdef USE_DINMIDI
            if (msg.destinations & MIDI_DEST_DIN) {
                switch (msg.type) {
                    case MIDI_MSG_NOTE_ON: {
                        int8_t p = msg.data1, v = msg.data2, c = msg.channel;
                        #ifdef USE_SEQLIB_OUTPUTS
                        if (available_output_types[output_mode].converter_func != nullptr) {
                            note_message_t r = available_output_types[output_mode].converter_func(p, v, c);
                            p = r.pitch; v = r.velocity; c = r.channel;
                        }
                        #endif
                        if (is_valid_note(p)) dinmidi->sendNoteOn(p, v, c);
                        break;
                    }
                    case MIDI_MSG_NOTE_OFF: {
                        int8_t p = msg.data1, v = msg.data2, c = msg.channel;
                        #ifdef USE_SEQLIB_OUTPUTS
                        if (available_output_types[output_mode].converter_func != nullptr) {
                            note_message_t r = available_output_types[output_mode].converter_func(p, v, c);
                            p = r.pitch; v = r.velocity; c = r.channel;
                        }
                        #endif
                        if (is_valid_note(p)) dinmidi->sendNoteOff(p, v, c);
                        break;
                    }
                    case MIDI_MSG_CONTROL_CHANGE:
                        dinmidi->sendControlChange(msg.data1, msg.data2, msg.channel);
                        break;
                    case MIDI_MSG_CLOCK:
                        dinmidi->sendClock();
                        break;
                    case MIDI_MSG_START:
                        dinmidi->sendStart();
                        break;
                    case MIDI_MSG_STOP:
                        dinmidi->sendStop();
                        break;
                    default:
                        break;
                }
            }
            #endif  // USE_DINMIDI

            // Stop after budget expires; remaining messages drain next loop iteration.
            if ((micros() - start) >= max_micros) break;
        }
        return count;
    }

    #ifdef ENABLE_SCREEN
        FLASHMEM
        void create_menu_items();
    #endif

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            //Serial.println("MIDIOutputWrapper#setup_saveable_settings()!");
            ISaveableSettingHost::setup_saveable_settings();

            // add settings for this class
            register_setting(
                new LSaveableSetting<OUTPUT_TYPE>(
                    "DIN MIDI Output Type",
                    "HW MIDI Settings",
                    &this->output_mode,
                    [=](OUTPUT_TYPE value) -> void {
                        this->set_output_mode(value);
                    },
                    [=](void) -> OUTPUT_TYPE {
                        return this->get_output_mode();
                    }
                ), SL_SCOPE_SYSTEM
            );
            register_setting(
                new LSaveableSetting<uint32_t>(
                    "DIN MIDI Clock Output Divider",
                    "HW MIDI Settings",
                    &this->din_midi_clock_output_divider,
                    [=](uint32_t value) -> void {
                        this->set_din_midi_clock_output_divider(value);
                    },
                    [=](void) -> uint32_t {
                        return this->get_din_midi_clock_output_divider();
                    }
                ), SL_SCOPE_SYSTEM
            );
            // todo: potentially move this up into IMIDINoteAndCCTarget, if we want to be able to enable/disable CC output for all MIDI outputs, not just the DIN MIDI output on the RP2040DualMIDIOutputWrapper
            register_setting(
                new LSaveableSetting<bool>(
                    "DIN MIDI CC Output Enabled",
                    "HW MIDI Settings",
                    &this->enable_cc_output,
                    [=](bool value) -> void {
                        this->set_cc_output_enabled(value);
                    },
                    [=](void) -> bool {
                        return this->is_cc_output_enabled();
                    }
                ), SL_SCOPE_SYSTEM
            );
            #ifdef ENABLE_CLOCK_INPUT_CV
                register_setting(
                    new LSaveableSetting<uint32_t>(
                        "CV Pulses Per Tick",
                        "HW MIDI Settings",
                        nullptr,
                        [=](uint32_t value) -> void {
                            set_external_cv_ticks_per_pulse_values(value);
                        },
                        [=](void) -> uint32_t {
                            return get_external_cv_ticks_per_pulse_values();
                        }
                    ), SL_SCOPE_SYSTEM
                );
            #endif

        }
    #endif
};

#if RP2040OutputWrapperClass == RP2040DualMIDIOutputWrapper
    extern RP2040DualMIDIOutputWrapper *output_wrapper;
#endif

void set_din_midi_clock_output_divider(uint32_t v);
uint32_t get_din_midi_clock_output_divider();

