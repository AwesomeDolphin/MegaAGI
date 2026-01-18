/***************************************************************************
    MEGA65-AGI -- Sierra AGI interpreter for the MEGA65
    Copyright (C) 2025  Keith Henrickson

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
***************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mega65.h>

#include "sound.h"
#include "irq.h"
#include "logic.h"
#include "main.h"
#include "volume.h"

#define FARSID1 (*(volatile struct __sid __far *)0xffd3400)

volatile uint8_t __huge *sound_file;
volatile uint16_t voice_offsets[3] = {0};
volatile uint8_t voice_stopped[3] = {1};
volatile uint16_t durations[3] = {0};
volatile uint8_t voice_holds[3];
volatile uint8_t sound_flag_end;
volatile uint8_t sound_running;
volatile bool request_stop;
volatile bool sound_flag_needs_set;
volatile uint8_t next_sound_flag_end;
volatile uint8_t __huge * next_sound_file;
volatile bool sound_queued;

void sound_play(uint8_t sound_num, uint8_t flag_at_end) {
    uint16_t length;
    if (sound_running) {
        sound_stop();
    }

    next_sound_file = locate_volume_object(voSound, sound_num, &length);
    if (next_sound_file == NULL) {
        return;
    }

    next_sound_flag_end = flag_at_end;
    sound_queued = true;
}

static void sound_play_queued(void) {
    sound_queued = false;
    sound_file = next_sound_file;
    sound_flag_end = next_sound_flag_end;
    logic_reset_flag(sound_flag_end);

    voice_offsets[0] = (sound_file[1] << 8) | sound_file[0];
    voice_offsets[1] = (sound_file[3] << 8) | sound_file[2];
    voice_offsets[2] = (sound_file[5] << 8) | sound_file[4];

    SID1.amp = 0x0f;
    SID1.v1.ad = 0x1b;
    SID1.v1.sr = 0x48;
    SID1.v2.pw = 0x400;
    SID1.v2.ad = 0x1a;
    SID1.v2.sr = 0x28;
    SID1.v2.pw = 0x200;
    SID1.v3.ad = 0x1a;
    SID1.v3.sr = 0x28;
    SID1.v2.pw = 0x200;

    sound_running = 1;
    request_stop = false;

    durations[0] = 1;
    durations[1] = 1;
    durations[2] = 1;
    voice_stopped[0] = 0;
    voice_stopped[1] = 0;
    voice_stopped[2] = 0;
}

void sound_stop(void) {
    if (sound_running) {
        request_stop = true;
    }
}

void sound_interrupt_handler(void) {
    uint8_t acted = 0;
    for (uint8_t voice=0; voice < 3; voice++) {
        --durations[voice];
        if (!voice_stopped[voice]) {
            acted = 1;
            uint16_t voice_offset = voice_offsets[voice];
            volatile struct __sid_voice __far *sid;
            switch(voice) {
                case 0:
                    sid = &FARSID1.v1;
                    break;
                case 1:
                    sid = &FARSID1.v2;
                    break;
                case 2:
                    sid = &FARSID1.v3;
                    break;
            }
            if (durations[voice] < voice_holds[voice]) {
                sid->ctrl = 0x00;
            }
            if (durations[voice] == 0) {
                uint16_t duration = (sound_file[voice_offset+1] << 8);
                duration = duration + sound_file[voice_offset];
                if (duration == 0xffff) {
                    sid->ctrl = 0x00;
                    voice_stopped[voice] = 1;
                    continue;
                }
                uint16_t fnum = (((sound_file[voice_offset + 2] & 0x3f) << 4) + (sound_file[voice_offset + 3] & 0x0f));
                uint16_t vol = sound_file[voice_offset + 4];
                voice_offsets[voice] = voice_offsets[voice] + 5;
                if ((vol & 0x0f) == 0x0f) {
                    sid->ctrl = 0x00;
                } else {
                    // First note value is 165, which VisualAGI says is an E-6. However, the frequency I get in the formula for Tandy chips is 677Hz, which is close to (but higher than) E-5.
                    // I'll assume that E-5 is right, and revise the conversion. The SID note value is 10814, and we can thus convert by multplying the two.
                    // 10814 * 165 = 1786455
                    uint16_t sidctrl = 1786455 / fnum;
                    sid->freq = sidctrl;
                    sid->ctrl = 0x41;
                }
                durations[voice] = duration;
                if (duration < 6) {
                    voice_holds[voice] = 0;
                } else {
                    voice_holds[voice] = 3;
                }
            }
        }
    }
    if (request_stop) {
        voice_stopped[0] = 1;
        voice_stopped[1] = 1;
        voice_stopped[2] = 1;
    }
    if (sound_running && !acted) {
        FARSID1.v1.ctrl = 0x00;
        FARSID1.v2.ctrl = 0x00;
        FARSID1.v3.ctrl = 0x00;
        FARSID1.amp = 0x00;
        sound_flag_needs_set = true;
        sound_running = 0;
    }

    if (!sound_running && sound_queued) {
        sound_play_queued();
    }
}