#pragma once

//#include "Config.h"

// MIDI + USB
#ifdef USE_TINYUSB
    #include <Adafruit_TinyUSB.h>
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


#include "outputs/output.h"

// todo: port usb_midi_clocker's OutputWrapper to work here?
// wrapper class to wrap different MIDI output types; handles both USB MIDI and DIN MIDI outputs
class RP2040DualMIDIOutputWrapper : virtual public IMIDINoteAndCCTarget {
    public:
    #ifdef USE_TINYUSB
        midi::MidiInterface<midi::SerialMIDI<Adafruit_USBD_MIDI>> *usbmidi = &USBMIDI;
    #endif
    #ifdef USE_DINMIDI
        midi::MidiInterface<midi::SerialMIDI<SerialPIO>> *dinmidi = &DINMIDI;
    #endif

    OUTPUT_TYPE output_mode = DEFAULT_OUTPUT_TYPE;
    void set_output_mode(int m) {
        this->output_mode = (OUTPUT_TYPE)m;
        // todo: if changed then we need to kill all the playing notes to avoid them getting stuck
    }
    OUTPUT_TYPE get_output_mode() {
        return this->output_mode;
    }

    uint32_t din_midi_clock_output_divider = 24;
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
            //if (is_bpm_on_beat(ticks))  // todo: make clock tick sends to din MIDI use custom divisor value
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

    #ifdef ENABLE_CV_INPUT
        #define NUM_MIDI_CC_PARAMETERS 6
        MIDICCParameter<> midi_cc_parameters[NUM_MIDI_CC_PARAMETERS] = {
            MIDICCParameter<> ("A",     this, 1, 1, true),
            MIDICCParameter<> ("B",     this, 2, 1, true),
            MIDICCParameter<> ("C",     this, 3, 1, true),
            MIDICCParameter<> ("Mix1",  this, 4, 1, true),
            MIDICCParameter<> ("Mix2",  this, 5, 1, true),
            MIDICCParameter<> ("Mix3",  this, 6, 1, true),
        };

        void setup_parameters() {
            for (int i = 0 ; i < 6 ; i++) {
                parameter_manager->addParameter(&midi_cc_parameters[i]);
            }
            midi_cc_parameters[0].connect_input(0/*parameter_manager->getInputForName("A")*/, 1.0f);
            midi_cc_parameters[0].connect_input(1/*parameter_manager->getInputForName("A")*/, 0.f);
            midi_cc_parameters[0].connect_input(2/*parameter_manager->getInputForName("A")*/, 0.f);

            midi_cc_parameters[1].connect_input(0/*parameter_manager->getInputForName("B")*/, 0.f);
            midi_cc_parameters[1].connect_input(1/*parameter_manager->getInputForName("B")*/, 1.0f);
            midi_cc_parameters[1].connect_input(2/*parameter_manager->getInputForName("B")*/, 0.f);

            midi_cc_parameters[2].connect_input(0/*parameter_manager->getInputForName("C")*/, 0.f);
            midi_cc_parameters[2].connect_input(1/*parameter_manager->getInputForName("C")*/, 0.f);
            midi_cc_parameters[2].connect_input(2/*parameter_manager->getInputForName("C")*/, 1.0f);

            midi_cc_parameters[3].connect_input(0, 1.0f);
            midi_cc_parameters[3].connect_input(1, 1.0f);
            midi_cc_parameters[3].connect_input(2, .0f);

            midi_cc_parameters[4].connect_input(0, .0f);
            midi_cc_parameters[4].connect_input(1, 1.0f);
            midi_cc_parameters[4].connect_input(2, 1.0f);

            midi_cc_parameters[5].connect_input(0, 1.0f);
            midi_cc_parameters[5].connect_input(1, .0f);
            midi_cc_parameters[5].connect_input(2, 1.0f);
        }
    #endif

    #ifdef ENABLE_SCREEN
        void create_menu_items();
    #endif

    #ifdef ENABLE_STORAGE
        LinkedList<String> *add_all_save_lines(LinkedList<String> *lines) {
            //if (Serial) Serial.println("MIDIOutputWrapper#add_all_save_lines()!");
            lines->add(String("dinmidi_output_type=") + String(this->get_output_mode()));

            return lines;
        }
        bool load_parse_key_value(String key, String value) {
            //if (Serial) Serial.printf("MIDIOutputWrapper#load_parse_key_value('%s','%s')\n", key.c_str(), value.c_str());
            if (key.equals("dinmidi_output_type")) {                
                this->set_output_mode((OUTPUT_TYPE)(uint8_t)value.toInt());
                return true;
            }
            return false;
        }
    #endif
};


extern RP2040DualMIDIOutputWrapper *output_wrapper;

void set_din_midi_clock_output_divider(uint32_t v);
uint32_t get_din_midi_clock_output_divider();
