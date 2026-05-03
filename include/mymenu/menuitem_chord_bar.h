#pragma once

#ifdef ENABLE_SCREEN
#ifdef ENABLE_ARRANGER

#include <Arduino.h>
#include <functional-vlpp.h>
#include "menuitems.h"
#include "scales.h"

// ChordBarMenuItem: a single-object per-bar chord editor.
//
// In single-column mode (my_r_bar < 0): one bar per row.
// In two-column mode   (my_r_bar >= 0): two adjacent bars side-by-side per row,
//   using half the vertical space.  Use the two-column constructor.
//
// The active field is highlighted in GREEN instead of a ">" prefix, saving
// 3 characters of row width — which makes the two-column layout fit.
//
// Interaction:
//   open (select from menu) → enter edit mode, active field = DEGREE (left bar)
//   knob left / right       → adjust active field value (committed immediately)
//   button_select           → cycle active field across all fields (3 or 6)
//   button_back             → returns false → menu closes this item
class ChordBarMenuItem : public MenuItem {
public:
    enum Field : uint8_t {
        FIELD_DEGREE   = 0, FIELD_TYPE   = 1, FIELD_INVERSION   = 2,
        FIELD_R_DEGREE = 3, FIELD_R_TYPE = 4, FIELD_R_INVERSION = 5
    };

    // Left (or only) bar
    vl::Func<void(int8_t)>      degree_setter;
    vl::Func<int8_t()>          degree_getter;
    vl::Func<void(CHORD::Type)> type_setter;
    vl::Func<CHORD::Type()>     type_getter;
    vl::Func<void(int8_t)>      inversion_setter;
    vl::Func<int8_t()>          inversion_getter;

    // Right bar (only used in two-column mode)
    vl::Func<void(int8_t)>      r_degree_setter;
    vl::Func<int8_t()>          r_degree_getter;
    vl::Func<void(CHORD::Type)> r_type_setter;
    vl::Func<CHORD::Type()>     r_type_getter;
    vl::Func<void(int8_t)>      r_inversion_setter;
    vl::Func<int8_t()>          r_inversion_getter;

    int8_t  my_section;
    int8_t  my_bar;
    int8_t  my_r_bar = -1;   // -1 means single-column mode
    int8_t *current_section;
    int8_t *current_bar;

    Field active_field = FIELD_DEGREE;

    uint8_t field_count() const {
        return (my_r_bar >= 0 && (bool)r_degree_getter) ? 6 : 3;
    }

    // Single-column constructor
    ChordBarMenuItem(
        const char *label,
        vl::Func<void(int8_t)>      degree_setter,
        vl::Func<int8_t()>          degree_getter,
        vl::Func<void(CHORD::Type)> type_setter,
        vl::Func<CHORD::Type()>     type_getter,
        vl::Func<void(int8_t)>      inversion_setter,
        vl::Func<int8_t()>          inversion_getter,
        int8_t  my_section,
        int8_t  my_bar,
        int8_t *current_section,
        int8_t *current_bar
    ) : MenuItem(label, true, false),
        degree_setter(degree_setter),   degree_getter(degree_getter),
        type_setter(type_setter),       type_getter(type_getter),
        inversion_setter(inversion_setter), inversion_getter(inversion_getter),
        my_section(my_section), my_bar(my_bar),
        current_section(current_section), current_bar(current_bar)
    {}

    // Two-column constructor
    ChordBarMenuItem(
        const char *label,
        vl::Func<void(int8_t)>      degree_setter,
        vl::Func<int8_t()>          degree_getter,
        vl::Func<void(CHORD::Type)> type_setter,
        vl::Func<CHORD::Type()>     type_getter,
        vl::Func<void(int8_t)>      inversion_setter,
        vl::Func<int8_t()>          inversion_getter,
        int8_t  my_section,
        int8_t  my_bar,
        vl::Func<void(int8_t)>      r_degree_setter,
        vl::Func<int8_t()>          r_degree_getter,
        vl::Func<void(CHORD::Type)> r_type_setter,
        vl::Func<CHORD::Type()>     r_type_getter,
        vl::Func<void(int8_t)>      r_inversion_setter,
        vl::Func<int8_t()>          r_inversion_getter,
        int8_t  my_r_bar,
        int8_t *current_section,
        int8_t *current_bar
    ) : MenuItem(label, true, false),
        degree_setter(degree_setter),   degree_getter(degree_getter),
        type_setter(type_setter),       type_getter(type_getter),
        inversion_setter(inversion_setter), inversion_getter(inversion_getter),
        r_degree_setter(r_degree_setter), r_degree_getter(r_degree_getter),
        r_type_setter(r_type_setter),     r_type_getter(r_type_getter),
        r_inversion_setter(r_inversion_setter), r_inversion_getter(r_inversion_getter),
        my_section(my_section), my_bar(my_bar), my_r_bar(my_r_bar),
        current_section(current_section), current_bar(current_bar)
    {}

    virtual bool action_opened() override {
        active_field = FIELD_DEGREE;
        return true;
    }

    virtual bool is_openable() override { return true; }

    virtual bool button_select() override {
        active_field = (Field)((active_field + 1) % field_count());
        return false;   // stay open
    }

    virtual bool button_back() override {
        return false;
    }

    virtual bool knob_left() override {
        switch (active_field) {
            case FIELD_DEGREE: {
                int8_t v = degree_getter();
                if (--v < -1) v = PITCHES_PER_SCALE;
                degree_setter(v);
                break;
            }
            case FIELD_TYPE: {
                int8_t v = (int8_t)type_getter() - 1;
                if (v < 0) v = NUMBER_CHORDS - 1;
                type_setter((CHORD::Type)v);
                break;
            }
            case FIELD_INVERSION: {
                int8_t v = inversion_getter() - 1;
                if (v < 0) v = MAX_INVERSIONS;
                inversion_setter(v);
                break;
            }
            case FIELD_R_DEGREE: {
                int8_t v = r_degree_getter();
                if (--v < -1) v = PITCHES_PER_SCALE;
                r_degree_setter(v);
                break;
            }
            case FIELD_R_TYPE: {
                int8_t v = (int8_t)r_type_getter() - 1;
                if (v < 0) v = NUMBER_CHORDS - 1;
                r_type_setter((CHORD::Type)v);
                break;
            }
            case FIELD_R_INVERSION: {
                int8_t v = r_inversion_getter() - 1;
                if (v < 0) v = MAX_INVERSIONS;
                r_inversion_setter(v);
                break;
            }
            default: break;
        }
        return true;
    }

    virtual bool knob_right() override {
        switch (active_field) {
            case FIELD_DEGREE: {
                int8_t v = degree_getter();
                if (++v > PITCHES_PER_SCALE) v = -1;
                degree_setter(v);
                break;
            }
            case FIELD_TYPE: {
                int8_t v = (int8_t)type_getter() + 1;
                if (v >= NUMBER_CHORDS) v = 0;
                type_setter((CHORD::Type)v);
                break;
            }
            case FIELD_INVERSION: {
                int8_t v = inversion_getter() + 1;
                if (v > MAX_INVERSIONS) v = 0;
                inversion_setter(v);
                break;
            }
            case FIELD_R_DEGREE: {
                int8_t v = r_degree_getter();
                if (++v > PITCHES_PER_SCALE) v = -1;
                r_degree_setter(v);
                break;
            }
            case FIELD_R_TYPE: {
                int8_t v = (int8_t)r_type_getter() + 1;
                if (v >= NUMBER_CHORDS) v = 0;
                r_type_setter((CHORD::Type)v);
                break;
            }
            case FIELD_R_INVERSION: {
                int8_t v = r_inversion_getter() + 1;
                if (v > MAX_INVERSIONS) v = 0;
                r_inversion_setter(v);
                break;
            }
            default: break;
        }
        return true;
    }

    // Format the degree as "G" (global=-1), "-" (none=0), or "1"–"7"
    static const char *format_degree(int8_t degree) {
        static char buf[3];
        if (degree < 0)       return "G";
        else if (degree == 0) return "-";
        else {
            buf[0] = '0' + degree;
            buf[1] = '\0';
            return buf;
        }
    }

    // Format chord type label; guard against CHORD::NONE (=8, out of bounds)
    static const char *format_type(CHORD::Type t) {
        if ((int)t >= 0 && (int)t < NUMBER_CHORDS)
            return chords[(int)t].label;
        return "None ";
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        const bool two_col = (my_r_bar >= 0 && (bool)r_degree_getter);

        // Pick text size to fit the full row content
        // const char *size_probe = two_col ? "1 Triad 0*|2 Sus 2 0 " : "1 Triad 0*";
        // int pixel_width = max_character_width * tft->currentCharacterWidth();
        // uint8_t ts = tft->get_textsize_for_width(size_probe, pixel_width);
        bool wrap_was = tft->isTextWrap();
        // tft->setTextSize(ts);
        tft->setTextSize(2);
        tft->setTextWrap(false);

        // Apply GREEN when this field is active+opened, else normal colours
        auto fcol = [this, opened, selected](Field f) {
            if (opened && active_field == f)
                tft->setTextColor(GREEN, default_bg);
            else
                colours(selected || opened);
        };

        auto is_cur = [this](int8_t bar) -> bool {
            return current_section && current_bar &&
                   my_section == *current_section && bar == *current_bar;
        };

        // Padded type string (always 5 chars + space)
        char type_buf[7];

        // --- left bar ---
        fcol(FIELD_DEGREE);
        tft->print(format_degree(degree_getter()));
        tft->print(" ");
        fcol(FIELD_TYPE);
        snprintf(type_buf, sizeof(type_buf), "%-5s ", format_type(type_getter()));
        tft->print(type_buf);
        fcol(FIELD_INVERSION);
        tft->printf("%d", (int)inversion_getter());
        colours(selected || opened);
        tft->print(is_cur(my_bar) ? "*" : " ");

        if (two_col) {
            colours(selected || opened);
            // tft->print("|");

            // --- right bar ---
            fcol(FIELD_R_DEGREE);
            tft->print(format_degree(r_degree_getter()));
            tft->print(" ");
            fcol(FIELD_R_TYPE);
            snprintf(type_buf, sizeof(type_buf), "%-5s ", format_type(r_type_getter()));
            tft->print(type_buf);
            fcol(FIELD_R_INVERSION);
            tft->printf("%d", (int)r_inversion_getter());
            colours(selected || opened);
            tft->print(is_cur(my_r_bar) ? "*" : " ");
        }

        tft->setTextWrap(wrap_was);
        return tft->getCursorY();
    }
};

#endif // ENABLE_ARRANGER
#endif // ENABLE_SCREEN
