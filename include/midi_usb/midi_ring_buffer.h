#pragma once

#include <stdint.h>  // uint8_t, uint32_t

// ── MIDI ring buffer ──────────────────────────────────────────────────────────
// Non-blocking SPSC (single-producer / single-consumer) queue for deferred MIDI
// output.
//
// Producers: the uClock timer ISR (do_tick) and ATOMIC() regions on Core 0.
// Consumer:  RP2040DualMIDIOutputWrapper::drain(), called from the main loop
//            outside any ATOMIC() block so that the USB drain ISR can fire
//            freely, preventing TX-FIFO stalls.
//
// Safety model (RP2040 / Cortex-M0+)
// ────────────────────────────────────
// All producers run on Core 0.  The timer ISR preempts main-loop ATOMIC()
// regions atomically — only one producer ever executes at a time.  drain() is
// also on Core 0 main-loop and can be preempted by the ISR, but the ISR only
// writes `head` (and buf[head]) while drain() only writes `tail`.  These index
// variables are disjoint, so no mutex is required.
//
// Core 1 never calls send*() methods, so it never enqueues.
//
// `volatile` on head/tail prevents the compiler from hoisting reads/writes out
// of loops.  Cortex-M0+ has no store buffer, so stores are globally visible in
// program order — no memory barrier instructions are needed.


// ── Destination bitmask ───────────────────────────────────────────────────────
#define MIDI_DEST_USB   0x01u
#define MIDI_DEST_DIN   0x02u
#define MIDI_DEST_BOTH  (MIDI_DEST_USB | MIDI_DEST_DIN)

// ── Message type ─────────────────────────────────────────────────────────────
enum MidiMsgType : uint8_t {
    MIDI_MSG_NONE           = 0,
    MIDI_MSG_NOTE_ON        = 1,
    MIDI_MSG_NOTE_OFF       = 2,
    MIDI_MSG_CONTROL_CHANGE = 3,
    MIDI_MSG_CLOCK          = 4,
    MIDI_MSG_START          = 5,
    MIDI_MSG_STOP           = 6,
    MIDI_MSG_CONTINUE       = 7,
};

// ── Message struct ────────────────────────────────────────────────────────────
// 8 bytes, naturally aligned → single-instruction copy on Cortex-M0+ with LDM.
// Destination routing (USB / DIN / both) is resolved at enqueue time so that
// conditions like the DIN clock divider check are evaluated while `ticks` is
// current.
struct MidiMessage {
    MidiMsgType type;         // which message
    uint8_t     destinations; // MIDI_DEST_USB | MIDI_DEST_DIN
    uint8_t     channel;      // 1–16
    uint8_t     data1;        // pitch / CC number / unused
    uint8_t     data2;        // velocity / CC value / unused
    uint8_t     _pad[3];      // pad to 8 bytes
};
static_assert(sizeof(MidiMessage) == 8, "MidiMessage must be 8 bytes");

// ── Ring buffer ───────────────────────────────────────────────────────────────
// Size must be a power of two.  512 × 8 = 4 096 bytes of SRAM.
// At 120 BPM / PPQN_24 the timer fires ~48 times/sec; each tick produces at
// most a handful of messages.  drain() runs at main-loop rate (~1 kHz), keeping
// occupancy very low in normal operation.  If the buffer ever fills (overflow_count > 0)
// increase MIDI_RING_SIZE — but "never drop" means we evict the oldest entry on
// overflow rather than silently discarding the new one, because a lost note-off
// is worse than a lost note-on.
#define MIDI_RING_SIZE 512u
#define MIDI_RING_MASK (MIDI_RING_SIZE - 1u)

struct MidiRingBuffer {
    MidiMessage       buf[MIDI_RING_SIZE];

    // `head` is the next slot the producer will write.  Written only by producer.
    // `tail` is the next slot the consumer will read.   Written only by consumer.
    volatile uint32_t head = 0;
    volatile uint32_t tail = 0;

    // Diagnostic: how many times the buffer was full at enqueue time.
    // Should always remain zero; non-zero means MIDI_RING_SIZE is undersized.
    uint32_t overflow_count = 0;

    // ── Producer API ─────────────────────────────────────────────────────────
    // Must never block.  Safe to call from ISR or ATOMIC() on Core 0.
    void enqueue(const MidiMessage &msg) {
        uint32_t h    = head;
        uint32_t next = (h + 1u) & MIDI_RING_MASK;

        if (next == tail) {
            // Buffer full.  Evict the oldest entry so newer (potentially more
            // important) messages like note-offs are accepted.
            overflow_count++;
            tail = (tail + 1u) & MIDI_RING_MASK;
        }

        buf[h] = msg;
        // Publish head AFTER writing the slot so the consumer never sees a
        // partially-written entry.  Cortex-M0+ in-order stores + volatile
        // guarantee this without a barrier instruction.
        head = next;
    }

    // ── Consumer API ─────────────────────────────────────────────────────────
    // Called only from main-loop drain().  Never call from ISR or ATOMIC().

    // Peek at the next message without consuming it.
    // Returns false if queue is empty.
    bool peek(MidiMessage &out) const {
        if (head == tail) return false;
        out = buf[tail];
        return true;
    }

    // Consume one message (call only after a successful peek confirms we can send it).
    void consume() {
        if (head != tail)
            tail = (tail + 1u) & MIDI_RING_MASK;
    }

    bool dequeue(MidiMessage &out) {
        if (head == tail) return false;
        out  = buf[tail];
        tail = (tail + 1u) & MIDI_RING_MASK;
        return true;
    }

    bool     is_empty()    const { return head == tail; }
    uint32_t occupancy()   const { return (head - tail) & MIDI_RING_MASK; }
};
