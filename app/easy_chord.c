/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson
 * Easy Chord: a root key plus the keys just below it picks the chord. No hardware calls, so
 * tests/run_host_tests.sh can check it against Chordcade's engine on a computer. */
#include "easy_chord.h"

enum { EC_W1=1, EC_W2=2, EC_B1=4, EC_B2=8 };

/* Combinations not listed here (three or more with both black keys) stay major. */
static const uint8_t ec_modifier_types[16]={
    [EC_W1]=EASY_CHORD_MAJ7, [EC_B1]=EASY_CHORD_MINOR, [EC_W2]=EASY_CHORD_DOM7, [EC_B2]=EASY_CHORD_SUS4,
    [EC_W1|EC_B1]=EASY_CHORD_MIN7, [EC_W1|EC_W2]=EASY_CHORD_MAJ9, [EC_B1|EC_B2]=EASY_CHORD_DIM7,
    [EC_W2|EC_B1]=EASY_CHORD_ADD9, [EC_W2|EC_B2]=EASY_CHORD_7SUS4, [EC_W1|EC_B2]=EASY_CHORD_SUS4,
    [EC_W1|EC_W2|EC_B1]=EASY_CHORD_MIN9, [EC_W1|EC_W2|EC_B2]=EASY_CHORD_MAJ9,
};

static const struct { uint8_t count, intervals[EASY_CHORD_MAX_TONES]; const char *suffix; }
ec_types[EASY_CHORD_TYPES]={
    [EASY_CHORD_MAJOR]={3,{0,4,7},"Maj"},
    [EASY_CHORD_MINOR]={3,{0,3,7},"Min"},
    [EASY_CHORD_DOM7]={4,{0,4,7,10},"7"},
    [EASY_CHORD_MAJ7]={4,{0,4,7,11},"Maj7"},
    [EASY_CHORD_MIN7]={4,{0,3,7,10},"Min7"},
    [EASY_CHORD_MAJ9]={5,{0,4,7,11,14},"Maj9"},
    [EASY_CHORD_MIN9]={5,{0,3,7,10,14},"Min9"},
    [EASY_CHORD_ADD9]={4,{0,4,7,14},"Add9"},
    [EASY_CHORD_SUS4]={3,{0,5,7},"Sus4"},
    [EASY_CHORD_7SUS4]={4,{0,5,7,10},"7sus4"},
    [EASY_CHORD_DIM7]={4,{0,3,6,9},"Dim7"},
};

static unsigned ec_is_held(const uint8_t set[16], int note)
{
    return note>=0 && note<128 && (set[note>>3]>>(note&7)&1);
}

static unsigned ec_is_black(int note)
{
    return (0x54aU>>(note%12))&1;
}

/* The two nearest white and two nearest black keys below root, as EC_W1, EC_W2, EC_B1, EC_B2; -1 if none. */
static void ec_modifier_keys(int root, int keys[4])
{
    unsigned whites=0, blacks=0;
    keys[0]=keys[1]=keys[2]=keys[3]=-1;
    for(int note=root-1; note>=0 && note>=root-16; note--) {
        if(ec_is_black(note)) {
            if(blacks<2)keys[2+blacks++]=note;
        } else if(whites<2)keys[whites++]=note;
    }
}

unsigned easy_chord_tones(unsigned root, unsigned type, uint8_t tones[EASY_CHORD_MAX_TONES])
{
    if(root>127 || type>=EASY_CHORD_TYPES || !tones)return 0;
    unsigned count=ec_types[type].count;
    for(unsigned i=0;i<count;i++) {
        unsigned note=root+ec_types[type].intervals[i];
        tones[i]=note>127?127:(uint8_t)note;
    }
    return count;
}

unsigned easy_chord_resolve(const uint8_t held[16], struct easy_chord *out, unsigned capacity)
{
    uint8_t consumed[16]={0};
    unsigned count=0;
    if(!held || (!out && capacity))return 0;
    /* High to low, because a key one chord uses as a modifier cannot also start a chord. */
    for(int root=127; root>=0 && count<capacity; root--) {
        int keys[4];
        unsigned mask=0;
        if(!ec_is_held(held,root) || ec_is_held(consumed,root))continue;
        ec_modifier_keys(root,keys);
        for(unsigned i=0;i<4;i++) {
            if(ec_is_held(held,keys[i]) && !ec_is_held(consumed,keys[i])) {
                mask|=1U<<i;
                consumed[keys[i]>>3]|=(uint8_t)(1U<<(keys[i]&7));
            }
        }
        out[count].root=(uint8_t)root;
        out[count].type=ec_modifier_types[mask];
        out[count].tone_count=(uint8_t)easy_chord_tones((unsigned)root,out[count].type,out[count].tones);
        count++;
    }
    return count;
}

const char *easy_chord_suffix(unsigned type)
{
    return type<EASY_CHORD_TYPES?ec_types[type].suffix:"";
}

void easy_chord_name(unsigned root, unsigned type, char name[12])
{
    static const char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    if(!name)return;
    if(root>127 || type>=EASY_CHORD_TYPES) {
        name[0]=0;
        return;
    }
    const char *pitch=notes[root%12], *suf=ec_types[type].suffix;
    unsigned i=0;
    while(*pitch)name[i++]=*pitch++;
    name[i++]=' ';
    while(*suf)name[i++]=*suf++;
    name[i]=0;
}
