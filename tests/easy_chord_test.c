/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 Michael Wilson
 * Checks app/easy_chord.c against results generated from Chordcade's engine. */
#include "easy_chord.h"
#include <stdio.h>
#include <string.h>

struct expected_chord {
    int root;
    const char *suffix;
    int tone_count;
    int tones[EASY_CHORD_MAX_TONES];
};

static const struct {
    int held_count;
    int held[37];
    int chord_count;
    struct expected_chord chords[37];
} vectors[]={
#include "easy_chord_vectors.inc"
};

static unsigned failures;

static void fail(unsigned index, const char *what)
{
    printf("vector %u: %s\n",index,what);
    failures++;
}

static void check_vector(unsigned index)
{
    uint8_t held[16]={0};
    struct easy_chord chords[37];
    for(int i=0;i<vectors[index].held_count;i++) {
        int note=vectors[index].held[i];
        held[note>>3]|=(uint8_t)(1U<<(note&7));
    }
    unsigned count=easy_chord_resolve(held,chords,37);
    if((int)count!=vectors[index].chord_count) {
        fail(index,"chord count");
        return;
    }
    for(unsigned c=0;c<count;c++) {
        const struct expected_chord *want=&vectors[index].chords[c];
        if(chords[c].root!=want->root)fail(index,"root");
        if(strcmp(easy_chord_suffix(chords[c].type),want->suffix))fail(index,"type");
        if(chords[c].tone_count!=want->tone_count) {
            fail(index,"tone count");
            continue;
        }
        for(int t=0;t<want->tone_count;t++) {
            if(chords[c].tones[t]!=want->tones[t])fail(index,"tones");
        }
    }
}

static void check_capacity(void)
{
    uint8_t held[16]={0};
    struct easy_chord chords[2];
    /* Three roots far enough apart to stay separate chords. */
    held[60>>3]|=1U<<(60&7);
    held[72>>3]|=1U<<(72&7);
    held[84>>3]|=1U<<(84&7);
    if(easy_chord_resolve(held,chords,2)!=2 || chords[0].root!=84 || chords[1].root!=72) {
        puts("capacity keeps the highest roots");
        failures++;
    }
}

static void check_top_note_clamps(void)
{
    uint8_t tones[EASY_CHORD_MAX_TONES];
    unsigned count=easy_chord_tones(120,EASY_CHORD_MAJ9,tones);
    if(count!=5 || tones[3]!=127 || tones[4]!=127) {
        puts("tones clamp at 127");
        failures++;
    }
}

static void check_names(void)
{
    char name[12];
    easy_chord_name(61,EASY_CHORD_MIN7,name);
    if(strcmp(name,"C# Min7")) {
        puts("name C# Min7");
        failures++;
    }
    /* The longest possible name must fit. */
    easy_chord_name(61,EASY_CHORD_7SUS4,name);
    if(strcmp(name,"C# 7sus4")) {
        puts("name C# 7sus4");
        failures++;
    }
}

static void check_bounds(void)
{
    uint8_t tones[EASY_CHORD_MAX_TONES];
    char name[12]={"untouched"};
    if(easy_chord_tones(128,EASY_CHORD_MAJOR,tones)!=0 ||
       easy_chord_tones(60,EASY_CHORD_TYPES,tones)!=0 ||
       easy_chord_tones(60,EASY_CHORD_MAJOR,0)!=0) {
        puts("tones bounds check");
        failures++;
    }
    if(strcmp(easy_chord_suffix(EASY_CHORD_TYPES),"")!=0) {
        puts("suffix bounds check");
        failures++;
    }
    easy_chord_name(128,EASY_CHORD_MAJOR,name);
    if(name[0]!=0) {
        puts("name root bounds check");
        failures++;
    }
    name[0]='x';
    easy_chord_name(60,EASY_CHORD_TYPES,name);
    if(name[0]!=0) {
        puts("name type bounds check");
        failures++;
    }
}

int main(void)
{
    unsigned count=sizeof vectors/sizeof vectors[0];
    for(unsigned i=0;i<count;i++)check_vector(i);
    check_capacity();
    check_top_note_clamps();
    check_names();
    check_bounds();
    printf("%u vectors, %u failures\n",count,failures);
    return failures?1:0;
}
