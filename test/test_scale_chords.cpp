#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "scales.h"

static const scale_t major_scale = { "Major", nullptr, 0, { 0, 2, 4, 5, 7, 9, 11 } };
static const scale_t minor_scale = { "Natural minor", nullptr, 0, { 0, 2, 3, 5, 7, 8, 10 } };
static const scale_t harmonic_minor_scale = { "Harmonic minor", nullptr, 0, { 0, 2, 3, 5, 7, 8, 11 } };
const scale_t *scales[] = { &major_scale, &minor_scale, &harmonic_minor_scale, nullptr };
const size_t NUMBER_SCALES = sizeof(scales) / sizeof(scales[0]);

static void require(bool condition, const char *message, int degree) {
    if (!condition) {
        std::fprintf(stderr, "%s (degree %d)\n", message, degree);
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    const char *const expected_romans[3][PITCHES_PER_SCALE] = {
        { "I ", "ii ", "iii ", "IV ", "V ", "vi ", "vii\xF7" },
        { "i ", "ii\xF7", "III ", "iv ", "v ", "VI ", "VII " },
        { "i ", "ii\xF7", "III ", "iv ", "V ", "VI ", "vii\xF7" }
    };
    const bool expected_major[3][PITCHES_PER_SCALE] = {
        { true, false, false, true, true, false, false },
        { false, false, true, false, false, true, true },
        { false, false, true, false, true, true, false }
    };
    const bool expected_diminished[3][PITCHES_PER_SCALE] = {
        { false, false, false, false, false, false, true },
        { false, true, false, false, false, false, false },
        { false, true, false, false, false, false, true }
    };

    scale_identity_t scale;
    chord_identity_t chord;
    int checked = 0;
    for (int scale_index = 0; scale_index < 3; ++scale_index) {
        scale.scale_number = scale_index;
        for (int root = 0; root < 12; ++root) {
            scale.root_note = root;
            for (int degree = 1; degree <= PITCHES_PER_SCALE; ++degree) {
                chord.degree = degree;
                require(scale.is_chord_major(degree) == expected_major[scale_index][degree - 1], "Incorrect major classification", degree);
                require(scale.is_chord_diminished(degree) == expected_diminished[scale_index][degree - 1], "Incorrect diminished classification", degree);
                for (int type = CHORD::TRIAD; type <= CHORD::NONE; ++type) {
                    chord.type = type;
                    for (int inversion = 0; inversion < MAXIMUM_INVERSIONS; ++inversion) {
                        chord.inversion = inversion;
                        require(std::strcmp(scale.get_degree_roman(chord), expected_romans[scale_index][degree - 1]) == 0, "Incorrect Roman numeral", degree);
                        ++checked;
                    }
                }
            }
        }
    }

    const int invalid_degrees[] = { -128, -1, 0, 8, 127 };
    scale.scale_number = SCALE_MAJOR;
    for (int degree : invalid_degrees) {
        chord.degree = degree;
        require(!scale.is_chord_major(degree), "Invalid degree classified as major", degree);
        require(!scale.is_chord_diminished(degree), "Invalid degree classified as diminished", degree);
        require(std::strcmp(scale.get_degree_roman(chord), "? ") == 0, "Invalid degree did not return placeholder", degree);
    }

    const int invalid_scales[] = { -128, SCALE_GLOBAL, 3, 4, 127 };
    chord.degree = 7;
    for (int scale_index : invalid_scales) {
        scale.scale_number = scale_index;
        require(!scale.is_chord_major(chord.degree), "Invalid scale classified as major", chord.degree);
        require(!scale.is_chord_diminished(chord.degree), "Invalid scale classified as diminished", chord.degree);
        require(std::strcmp(scale.get_degree_roman(chord), "? ") == 0, "Invalid scale did not return placeholder", chord.degree);
    }

    std::printf("Scale chord checks passed: %d Roman numeral cases plus classification and invalid-input checks.\n", checked);
}