#!/usr/bin/python3

SCALE_ROOT_C       = 0
SCALE_ROOT_C_SHARP = 1
SCALE_ROOT_D       = 2
SCALE_ROOT_D_SHARP = 3
SCALE_ROOT_E       = 4
SCALE_ROOT_F       = 5
SCALE_ROOT_F_SHARP = 6
SCALE_ROOT_G       = 7
SCALE_ROOT_G_SHARP = 8
SCALE_ROOT_A       = 9
SCALE_ROOT_A_SHARP = 10
SCALE_ROOT_B       = 11

SCALE_SIZE = 7

def get_note_name(pitch):
    note_names = [ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" ]
    octave = int(pitch / 12)
    chromatic_degree = pitch % 12
    return "%s%s\t(%s)" % (note_names[chromatic_degree], octave, pitch)

"""
def quantise_pitch(pitch, root_note = SCALE_ROOT_G, scale_number = 0):
    print("----")
        
    #print("quantise_pitch passed\t%s,\t%s,\t%s.." % (get_note_name(pitch), get_note_name(root_note), scale_number))

    valid_chromatic_pitches = [
        [ 0, 2, 4, 5, 7, 9, 11 ],
        [ 0, 2, 3, 5, 6, 9, 10 ],
        [ 0, 1, 3, 4, 7, 9, 11 ]
    ]

    octave = int(pitch / 12)
    target_chromatic_pitch = pitch % 12
    root_note_mod = root_note % 12

    #print("\tgot octave=%s,\tchromatic_pitch=%s,\troot_note_mod=%s.." % (octave, chromatic_pitch, root_note_mod))

    nearest_below_index = 0
    for i in range(SCALE_SIZE):
        interval = valid_chromatic_pitches[scale_number][i]
        check_chromatic_interval = (root_note + interval)

        #print("\tfor scale degree %s: got interval\t%s with chromatic_interval %s" % (i, interval, get_note_name(chromatic_interval)))
        print("\tfor scale degree index %s: got interval\t%s which becomes check_chromatic_interval %s (compared to target_chromatic_pitch %s)" 
              % (i, interval, get_note_name(check_chromatic_interval), target_chromatic_pitch))

        if (check_chromatic_interval == target_chromatic_pitch):
            #print("\t=>Matched scale interval %s to %s,\t returning %s" % (chromatic_interval, chromatic_pitch, pitch))
            return pitch

        #if (((root_note_mod + chromatic_interval)%12) > chromatic_pitch and nearest_below_index == -1):
        if (check_chromatic_interval < target_chromatic_pitch): # and nearest_below_index == -1):
            #print("\t\tfound first where chromatic_interval < chromatic_pitch")
            nearest_below_index = i
            #return (octave*12) + chromatic_interval

    #result = (root_note_mod + valid_chromatic_pitches[scale_number][nearest_below_index])
    #result -= root_note
    #result += (octave*12)
    result = root_note + (octave*12) + valid_chromatic_pitches[scale_number][nearest_below_index]
    if (result!=pitch-1):
        print("\tFor\t%s, Got result\t%s.  should be\t%s?" % (pitch, result, pitch-1))
        print("\tDifference is %s" % (result - (pitch-1)))
    #print("\t\tOvershot - using last index %s,\treturning %s" % (nearest_below_index, result))
    #print("\t%s\tis not in scale, changed to interval number\t%s, giving \t%s" % (get_note_name(pitch), nearest_below_index, get_note_name(result)))
    return result
"""
valid_chromatic_pitches = [
    [ 0, 2, 4, 5, 7, 9, 11 ],
    [ 0, 2, 3, 5, 6, 9, 10 ],
    [ 0, 1, 3, 4, 7, 9, 11 ]
]

"""def quantise_pitch(pitch, root_note = SCALE_ROOT_A, scale_number = 0):
    note_table = []

    for octave in range(0,10):
        for interval in valid_chromatic_pitches[scale_number]:
            value = (octave*12) + root_note + interval
            note_table.append(value)

    for valid_note in note_table:
        if valid_note==pitch:
            return pitch
        if pitch < valid_note:
            return last_valid_note
        last_valid_note = valid_note

    return note_table[0]"""


def quantise_pitch_2(pitch, root_note = SCALE_ROOT_A, scale_number = 0):
    octave = int(pitch/12)
    chromatic_pitch = pitch % 12    # 0-12 C-B
    relative_pitch = chromatic_pitch - root_note
    if (relative_pitch<0):
        relative_pitch += 12
        relative_pitch %= 12

    for interval in valid_chromatic_pitches[scale_number]:
        if (interval == relative_pitch):
            return pitch
        if (relative_pitch < interval):
            v = last_interval + (octave*12) + root_note
            if v - pitch > 7:
                v-=12
            return v
        last_interval = interval

    print("\t\t\t!!!!!default case..!")
    return last_interval

    print("quantise_pitch_2 didn't find a pitch for %s!" % (pitch))

for scale in range(3):
    for root in range(12):
        print("-------\n\n\nRoot = %s, scale = %s\n---------" % (get_note_name(root), scale))
        last_pitch = -1
        for pitch in range(root,127):
            #result = quantise_pitch(pitch, root)
            #print("quantise_pitch: For\t%s,\tgot result\t%s - %s" % (get_note_name(pitch), get_note_name(result), "in scale" if pitch==result else "should be %s?" %get_note_name(last_pitch) ))
            result = quantise_pitch_2(pitch, root, scale)
            if (result == pitch):
                last_pitch = pitch
            print("quantise_pitch_2: For\t%s,\tgot result\t%s - %s" % (get_note_name(pitch), get_note_name(result), "in scale" if pitch==result else "should be %s?" %get_note_name(last_pitch) ))
            #last_pitch = pitch
            #print("\n")
