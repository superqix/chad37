/* SPDX-License-Identifier: MPL-2.0
 * Easy Chord: a root key plus the keys just below it picks the chord. No hardware calls, so
 * tests/run_host_tests.sh can check it against Chordcade's engine on a computer. */
#include "easy_chord.h"
#include <string.h>

enum { W1=1, W2=2, B1=4, B2=8 };

/* Combinations not listed here (three or more with both black keys) stay major. */
static const uint8_t modifier_types[16]={
    [W1]=EASY_CHORD_MAJ7, [B1]=EASY_CHORD_MINOR, [W2]=EASY_CHORD_DOM7, [B2]=EASY_CHORD_SUS4,
    [W1|B1]=EASY_CHORD_MIN7, [W1|W2]=EASY_CHORD_MAJ9, [B1|B2]=EASY_CHORD_DIM7,
    [W2|B1]=EASY_CHORD_ADD9, [W2|B2]=EASY_CHORD_7SUS4, [W1|B2]=EASY_CHORD_SUS4,
    [W1|W2|B1]=EASY_CHORD_MIN9, [W1|W2|B2]=EASY_CHORD_MAJ9,
};

static const struct { uint8_t count, intervals[EASY_CHORD_MAX_TONES]; const char *suffix; }
types[EASY_CHORD_TYPES]={
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

static unsigned is_held(const uint8_t set[16], int note)
{
    return note>=0 && (set[note>>3]>>(note&7)&1);
}

static unsigned is_black(int note)
{
    return (0x54aU>>(note%12))&1;
}

/* The two nearest white and two nearest black keys below root, as W1, W2, B1, B2; -1 if none. */
static void modifier_keys(int root, int keys[4])
{
    unsigned whites=0, blacks=0;
    keys[0]=keys[1]=keys[2]=keys[3]=-1;
    for(int note=root-1; note>=0 && note>=root-16; note--) {
        if(is_black(note)) {
            if(blacks<2)keys[2+blacks++]=note;
        } else if(whites<2)keys[whites++]=note;
    }
}

unsigned easy_chord_tones(unsigned root, unsigned type, uint8_t tones[EASY_CHORD_MAX_TONES])
{
    unsigned count=types[type].count;
    for(unsigned i=0;i<count;i++) {
        unsigned note=root+types[type].intervals[i];
        tones[i]=note>127?127:(uint8_t)note;
    }
    return count;
}

unsigned easy_chord_resolve(const uint8_t held[16], struct easy_chord *out, unsigned capacity)
{
    uint8_t consumed[16]={0};
    unsigned count=0;
    /* High to low, because a key one chord uses as a modifier cannot also start a chord. */
    for(int root=127; root>=0 && count<capacity; root--) {
        int keys[4];
        unsigned mask=0;
        if(!is_held(held,root) || is_held(consumed,root))continue;
        modifier_keys(root,keys);
        for(unsigned i=0;i<4;i++) {
            if(is_held(held,keys[i]) && !is_held(consumed,keys[i])) {
                mask|=1U<<i;
                consumed[keys[i]>>3]|=(uint8_t)(1U<<(keys[i]&7));
            }
        }
        out[count].root=(uint8_t)root;
        out[count].type=modifier_types[mask];
        out[count].tone_count=(uint8_t)easy_chord_tones((unsigned)root,out[count].type,out[count].tones);
        count++;
    }
    return count;
}

const char *easy_chord_suffix(unsigned type)
{
    return types[type].suffix;
}

void easy_chord_name(unsigned root, unsigned type, char name[12])
{
    static const char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    const char *pitch=notes[root%12];
    size_t length=strlen(pitch);
    memcpy(name,pitch,length);
    name[length]=' ';
    strcpy(name+length+1,types[type].suffix);
}
