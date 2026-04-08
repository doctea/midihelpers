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

    virtual void sendNoteOn(uint8_t pitch, uint8_t velocity, uint8_t channel) override {
        //Serial.printf("MIDIOutputWrapper#sendNoteOn(%i, %i, %i)\n", pitch, velocity, channel);
        if (!is_valid_note(pitch)) 
            return;

        #ifdef USE_TINYUSB
            cc_locked.lock();
            usbmidi->sendNoteOn(pitch, velocity, channel);
            cc_locked.unlock();
        #endif
        #ifdef USE_DINMIDI
            int8_t pitch_to_send = pitch;
            int8_t velocity_to_send = velocity;
            int8_t channel_to_send = channel;

            if (available_output_types[output_mode].converter_func!=nullptr) {
                note_message_t r = available_output_types[output_mode].converter_func(pitch_to_send, velocity_to_send, channel_to_send);
                pitch_to_send = r.pitch;
                velocity_to_send = r.velocity;
                channel_to_send = r.channel;
            }

            if (is_valid_note(pitch_to_send)) {
                dinmidi->sendNoteOn(
                    pitch_to_send,
                    velocity_to_send, 
                    channel_to_send
                );
            }
        #endif
    }
    virtual void sendNoteOff(uint8_t pitch, uint8_t velocity, uint8_t channel) override {
        if (!is_valid_note(pitch)) 
            return;
            
        #ifdef USE_TINYUSB
            cc_locked.lock();
            usbmidi->sendNoteOff(pitch, velocity, channel);
            cc_locked.unlock();
        #endif
        #ifdef USE_DINMIDI
            int8_t pitch_to_send = pitch;
            int8_t velocity_to_send = velocity;
            int8_t channel_to_send = channel;

            if (available_output_types[output_mode].converter_func!=nullptr) {
                note_message_t r = available_output_types[output_mode].converter_func(pitch_to_send, velocity_to_send, channel_to_send);
                pitch_to_send = r.pitch;
                velocity_to_send = r.velocity;
                channel_to_send = r.channel;
            }

            if (is_valid_note(pitch_to_send)) {
                dinmidi->sendNoteOff(
                    pitch_to_send,
                    velocity_to_send, 
                    channel_to_send
                );
            }
        #endif
    }

    #if defined(USE_TINYUSB) && defined(ARDUINO_ARCH_RP2040)
        struct Mutex {
            mutex_t mutex;
            std::atomic<bool> locked = false;
            Mutex() {
                mutex_init(&mutex);
            }
            public:
            void lock() {
                mutex_enter_blocking(&mutex);
                locked = true;
            }
            void unlock() {
                mutex_exit(&mutex);
                locked = false;
            }
            bool is_locked() {
                return locked;
            }
        };
        Mutex cc_locked;
    #endif

    virtual void sendControlChange(uint8_t number, uint8_t value, uint8_t channel) override {
        if (!this->is_cc_output_enabled())
            return;
        if (!is_valid_note(number))
            return;
        #ifdef USE_TINYUSB
            cc_locked.lock();
            if (number < 0 || number > 127 || value < 0 || value > 127 || channel < 1 || channel > 16) {
                Serial.printf("RP2040DualMIDIOutputWrapper#sendControlChange got an CC message: cc=%i,\t value=%i,\t channel=%i\n", number, value, channel);
            }
            usbmidi->sendControlChange(number, value, channel);
            cc_locked.unlock();
        #endif
        #ifdef USE_DINMIDI
            dinmidi->sendControlChange(number, value, channel);
        #endif        
    }

    virtual void sendClock() {
        #ifdef USE_TINYUSB
            cc_locked.lock();
            usbmidi->sendClock();
            cc_locked.unlock();
        #endif
        #ifdef USE_DINMIDI
            if (ticks % this->get_din_midi_clock_output_divider() == 0)
                dinmidi->sendClock();   // send divisions of clock to muso, to make the clock output more useful
        #endif
    }
    virtual void sendStart() {
        #ifdef USE_TINYUSB
            cc_locked.lock();
            usbmidi->sendStart();
            cc_locked.unlock();
        #endif
        #ifdef USE_DINMIDI
            dinmidi->sendStart();
        #endif
    }
    virtual void sendStop() {
        #ifdef USE_TINYUSB
            cc_locked.lock();
            usbmidi->sendStop();
            cc_locked.unlock();
        #endif
        #ifdef USE_DINMIDI
            dinmidi->sendStop();
        #endif
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

//extern RP2040OutputWrapperClass *output_wrapper;

void set_din_midi_clock_output_divider(uint32_t v);
uint32_t get_din_midi_clock_output_divider();

