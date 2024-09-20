#if defined(USE_TINYUSB) && defined(ARDUINO_ARCH_RP2040)

#if !defined(USB_MIDI_VID) || !defined(USB_MIDI_PID) || !defined(USB_MIDI_MANUFACTURER) || !defined(USB_MIDI_PRODUCT)
	#error "USB_TINYUSB and ARDUINO_ARCH_RP2040 defined, but missing one or more of USB_MIDI_VID, USB_MIDI_PID, USB_MIDI_MANUFACTURER or USB_MIDI_PRODUCT"
#endif

//#include "Config.h"
#include "midi_usb/midi_usb_rp2040.h"

//#include "debug.h"

// midihelpers library clock handling
#include <clock.h>
#include <bpm.h>

#ifdef USE_TINYUSB
    Adafruit_USBD_MIDI usb_midi;
    MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, USBMIDI);
#endif

#ifdef USE_DINMIDI
    #ifdef MIDI_SERIAL_SOFTWARE
        #include <SoftwareSerial.h>
        SoftwareSerial SoftSerial(MIDI_SERIAL_OUT_PIN, -1, false);
        MIDI_CREATE_INSTANCE(SoftwareSerial, SoftSerial, DINMIDI);
    #endif
    #ifdef MIDI_SERIAL_SPIO
        SerialPIO spio(MIDI_SERIAL_OUT_PIN, SerialPIO::NOPIN);
        MIDI_CREATE_INSTANCE(SerialPIO, spio, DINMIDI);
    #endif
    #ifdef MIDI_SERIAL_HARDWARE
        MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, DINMIDI);
    #endif

    void set_din_midi_clock_output_divider(uint32_t v) {
        output_wrapper->set_din_midi_clock_output_divider(v);
    }
    uint32_t get_din_midi_clock_output_divider() {
        return output_wrapper->get_din_midi_clock_output_divider();
} 

#endif


void setup_usb() {
    #if defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040)
        // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
        TinyUSB_Device_Init(0);
    #endif

    USBDevice.setID(USB_MIDI_VID, USB_MIDI_PID);
    USBDevice.setManufacturerDescriptor(USB_MIDI_MANUFACTURER); //"TyrellCorp");
    USBDevice.setProductDescriptor(USB_MIDI_PRODUCT); //"Microlidian");

    //while( !TinyUSBDevice.mounted() ) delay(1);
}

void messages_log_add(String);

void auto_handle_start() {
    messages_log_add("auto_handle_start()!");

    if(clock_mode != CLOCK_EXTERNAL_USB_HOST) {
        // automatically switch to using external USB clock if we receive a START message from the usb host
        change_clock_mode(CLOCK_EXTERNAL_USB_HOST);
        //ticks = 0;
        //messages_log_add(String("Auto-switched to CLOCK_EXTERNAL_USB_HOST"));
    }
    pc_usb_midi_handle_start();
}

void auto_handle_start_wrapper();

void setup_midi() {
    // setup USB MIDI connection
    #ifdef USE_TINYUSB
        USBMIDI.begin(MIDI_CHANNEL_OMNI);
        USBMIDI.turnThruOff();

        // callbacks for messages received from USB MIDI host
        USBMIDI.setHandleClock(pc_usb_midi_handle_clock);
        USBMIDI.setHandleStart(auto_handle_start); //pc_usb_midi_handle_start);
        //USBMIDI.setHandleStart(auto_handle_start_wrapper);
        USBMIDI.setHandleStop(pc_usb_midi_handle_stop);
        USBMIDI.setHandleContinue(pc_usb_midi_handle_continue);
    #endif

    #ifdef USE_DINMIDI
        // setup serial MIDI output on standard UART pins; send only
        DINMIDI.begin(0);
        DINMIDI.turnThruOff();
    #endif
}


#ifdef ENABLE_SCREEN
    #include "menuitems_lambda.h"
    #include "mymenu_items/ParameterMenuItems_lowmemory.h"

    extern uint32_t external_cv_ticks_per_pulse_values[9];
    #define NUM_EXTERNAL_CV_TICKS_VALUES (sizeof(external_cv_ticks_per_pulse_values)/sizeof(uint32_t))

    void set_external_cv_ticks_per_pulse_values(uint32_t new_value);
    uint32_t get_external_cv_ticks_per_pulse_values();

    void RP2040DualMIDIOutputWrapper::create_menu_items() {
        // controls for cv-to-midi outputs..

        // todo: probably move this to another more generic 'settings' page
        menu->add_page("MIDI Output");
        LambdaSelectorControl<OUTPUT_TYPE> *output_mode_selector = new LambdaSelectorControl<OUTPUT_TYPE>(
            "DIN output mode", 
            [=](OUTPUT_TYPE a) -> void { this->set_output_mode(a); },
            [=]() -> OUTPUT_TYPE { return this->get_output_mode(); },
            nullptr, 
            true
        );
        for (int i = 0 ; i < sizeof(available_output_types)/sizeof(output_type_t) ; i++) {
            output_mode_selector->add_available_value(available_output_types[i].type_id, available_output_types[i].label);
        }
        menu->add(output_mode_selector);

        #ifdef ENABLE_CLOCK_INPUT_CV
            SelectorControl<uint32_t> *external_cv_ticks_per_pulse_selector = new SelectorControl<uint32_t>("External CV clock: Pulses per tick");
            external_cv_ticks_per_pulse_selector->available_values = external_cv_ticks_per_pulse_values;
            external_cv_ticks_per_pulse_selector->num_values = NUM_EXTERNAL_CV_TICKS_VALUES;
            external_cv_ticks_per_pulse_selector->f_setter = set_external_cv_ticks_per_pulse_values;
            external_cv_ticks_per_pulse_selector->f_getter = get_external_cv_ticks_per_pulse_values;
            menu->add(external_cv_ticks_per_pulse_selector);
        #endif

        // todo: make this a 'horizontal selector' like the SelectorControl above; maybe make a LambdaSelectorControl...
        // todo: actually, maybe make SelectorControl capable of accepting callback lambdas so that we don't need to waste memory on a LinkedList implementation
        /*LambdaSelectorControl<uint32_t> *din_midi_clock_output_divider = new LambdaSelectorControl<uint32_t>(
            "DIN MIDI: send clock every X pulses", 
            [=](uint32_t v) -> void { set_din_midi_clock_output_divider(v); },
            [=](void) -> uint32_t { return this->get_din_midi_clock_output_divider(); },
            nullptr,
            true,
            true
        );
        for (int i = 0 ; i < NUM_EXTERNAL_CV_TICKS_VALUES ; i++) {
            din_midi_clock_output_divider->add_available_value(
                external_cv_ticks_per_pulse_values[i], 
                (new String(external_cv_ticks_per_pulse_values[i]))->c_str()
            );
        }*/
        SelectorControl<uint32_t> *din_midi_clock_output_divider = new SelectorControl<uint32_t>("DIN MIDI: send clock every X pulses");
        din_midi_clock_output_divider->available_values = external_cv_ticks_per_pulse_values;
        din_midi_clock_output_divider->num_values = NUM_EXTERNAL_CV_TICKS_VALUES;
        din_midi_clock_output_divider->f_setter = ::set_din_midi_clock_output_divider;
        din_midi_clock_output_divider->f_getter = ::get_din_midi_clock_output_divider;
        menu->add(din_midi_clock_output_divider);
    }
#endif

#endif
