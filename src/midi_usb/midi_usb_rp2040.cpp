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

void auto_handle_start() {
    if(clock_mode != CLOCK_EXTERNAL_USB_HOST) {
        // automatically switch to using external USB clock if we receive a START message from the usb host
        // todo: probably move this into the midihelper library?
        //set_clock_mode(CLOCK_EXTERNAL_USB_HOST);
        if (__clock_mode_changed_callback!=nullptr)
            __clock_mode_changed_callback(clock_mode, CLOCK_EXTERNAL_USB_HOST);
        clock_mode = CLOCK_EXTERNAL_USB_HOST;
        //ticks = 0;
        //messages_log_add(String("Auto-switched to CLOCK_EXTERNAL_USB_HOST"));
    }
    pc_usb_midi_handle_start();
}

void setup_midi() {
    // setup USB MIDI connection
    #ifdef USE_TINYUSB
        USBMIDI.begin(MIDI_CHANNEL_OMNI);
        USBMIDI.turnThruOff();

        // callbacks for messages recieved from USB MIDI host
        USBMIDI.setHandleClock(pc_usb_midi_handle_clock);
        USBMIDI.setHandleStart(auto_handle_start); //pc_usb_midi_handle_start);
        USBMIDI.setHandleStop(pc_usb_midi_handle_stop);
        USBMIDI.setHandleContinue(pc_usb_midi_handle_continue);
    #endif

    #ifdef USE_DINMIDI
        // setup serial MIDI output on standard UART pins; send only
        DINMIDI.begin(0);
        DINMIDI.turnThruOff();
    #endif
}

#endif
